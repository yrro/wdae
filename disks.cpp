#include "disks.hpp"

#include <memory>
#include <sstream>
#include <vector>

#include "com_manager.hpp"
#include "explain.hpp"

#include <aclapi.h>
#include <setupapi.h>
#include <sddl.h>

namespace {
    template<typename T>
    struct LocalAlloc_delete {
        void operator()(T* p) const noexcept {
            LocalFree(p);
        }
    };

    // PSECURITY_DESCRIPTOR is not a SECURITY_DESCRIPTOR*
    template<>
    struct LocalAlloc_delete<SECURITY_DESCRIPTOR> {
        using pointer = PSECURITY_DESCRIPTOR;
        void operator()(pointer p) const noexcept {
            LocalFree(p);
        }
    };

    template<typename T>
    using unique_local_ptr = std::unique_ptr<T, LocalAlloc_delete<T>>;

    struct HANDLE_delete {
        void operator()(HANDLE h) const noexcept {
            CloseHandle(h);
        }
    };

    using unique_handle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HANDLE_delete>;

    _variant_t prop_get(IWbemClassObject* o, LPCWSTR name) {
        VARIANT t;
        com_manager::CheckError(o->Get(name, 0, &t, nullptr, nullptr));
        return _variant_t(t, false);
    }

    disk_state get_state(std::wstring device_id) {
        HANDLE h = CreateFile(
            device_id.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (h == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_ACCESS_DENIED) {
                return inaccessible;
            }
            throw windows_error(GetLastError(), L"CreateFile");
        }
        unique_handle f(h);
        return accessible;
    }

    std::wstring get_current_sddl(std::wstring device_id) {
        unique_handle f;
        {
            HANDLE h = CreateFile(
                device_id.c_str(),
                READ_CONTROL, 0,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
            );
            if (h == INVALID_HANDLE_VALUE) {
                throw windows_error(GetLastError(), L"CreateFile");
            }
            f = unique_handle(h);
        }

        const SECURITY_INFORMATION i = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

        unique_local_ptr<SECURITY_DESCRIPTOR> sd;
        {
            PSECURITY_DESCRIPTOR sdp = nullptr;
            DWORD r = GetSecurityInfo(
                f.get(), SE_FILE_OBJECT,
                i,
                nullptr, nullptr, nullptr, nullptr,
                &sdp
            );
            if (r != ERROR_SUCCESS) {
                throw windows_error(r, L"GetSecurityInfo");
            }
            sd.reset(sdp);
        }

        std::unique_ptr<wchar_t, decltype(&LocalFree)> sddl(nullptr, LocalFree);
        {
            wchar_t* p;
            if (!ConvertSecurityDescriptorToStringSecurityDescriptor(sd.get(), SDDL_REVISION_1, i, &p, nullptr)) {
                throw windows_error(GetLastError(), L"ConvertSecurityDescriptorToStringSecurityDescriptor");
            }
            sddl.reset(p);
        }

        return std::wstring(sddl.get());
    }

    std::experimental::optional<std::wstring> get_setup_sddl(std::wstring pnp_device_id) {
        std::unique_ptr<VOID, decltype(&SetupDiDestroyDeviceInfoList)> dev(nullptr, SetupDiDestroyDeviceInfoList);
        {
            HDEVINFO h = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_DEVICEINTERFACE);
            if (h == INVALID_HANDLE_VALUE) {
                throw windows_error(GetLastError(), L"SetupDiGetClassDevs");
            }
            dev.reset(h);
        }

        SP_DEVICE_INTERFACE_DATA intf;
        intf.cbSize = sizeof intf;
        for (int i = 0; ; ++i) {
            if (!SetupDiEnumDeviceInterfaces(dev.get(), NULL, &GUID_DEVINTERFACE_DISK, i, &intf)) {
                if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                    break;
                }
                throw windows_error(GetLastError(), L"SetupDiEnumDeviceInterfaces");
            }

            std::vector<BYTE> name_raw;
            SP_DEVICE_INTERFACE_DETAIL_DATA* name = nullptr;
            {
                DWORD s; // size in bytes
                SetupDiGetDeviceInterfaceDetailW(dev.get(), &intf, nullptr, 0, &s, nullptr);
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                    throw windows_error(GetLastError(), L"SetupDiGetDeviceInterfaceDetail 1");
                }
                name_raw.resize(s);
                name = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA* const>(&name_raw[0]);
            }

            name->cbSize = sizeof *name;
            SP_DEVINFO_DATA info;
            info.cbSize = sizeof info;
            if (!SetupDiGetDeviceInterfaceDetailW(dev.get(), &intf, name, name_raw.size(), nullptr, &info)) {
                throw windows_error(GetLastError(), L"SetupDiGetDeviceInterfaceDetail 2");
            }

            std::vector<wchar_t> instance_id;
            {
                DWORD s; // number of characters
                SetupDiGetDeviceInstanceIdW(dev.get(), &info, nullptr, 0, &s);
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                    throw windows_error(GetLastError(), L"SetupDiGetDeviceInstanceId 1");
                }
                instance_id.resize(s);
            }

            if (!SetupDiGetDeviceInstanceIdW(dev.get(), &info, &instance_id[0], instance_id.size(), nullptr)) {
                throw windows_error(GetLastError(), L"SetupDiGetDeviceInstanceId 2");
            }

            if (std::wstring(&instance_id[0]) != pnp_device_id)
                continue;

            std::vector<BYTE> sddl;
            {
                DWORD s; // size in bytes
                SetupDiGetDeviceRegistryPropertyW(dev.get(), &info, SPDRP_SECURITY_SDS, nullptr, nullptr, 0, &s);
                if (GetLastError() == ERROR_INVALID_DATA) {
                    return std::experimental::nullopt;
                } else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                    throw windows_error(GetLastError(), L"SetupDiGetDeviceRegistryProperty 1");
                }
                sddl.resize(s);
            }

            if (!SetupDiGetDeviceRegistryPropertyW(dev.get(), &info, SPDRP_SECURITY_SDS, nullptr, &sddl[0], sddl.size(), nullptr)) {
                throw windows_error(GetLastError(), L"SetupDiGetDeviceRegistryProperty 2");
            }

            return std::wstring(reinterpret_cast<wchar_t*>(&sddl[0]));
        }

        // XXX device not found... probably should be an error
        return std::experimental::nullopt;
    }
}

disk_lister::disk_lister() {
    HRESULT hr;

    com_manager::CheckError(loc.CreateInstance(CLSID_WbemLocator));

    com_manager::CheckError(hr = loc->ConnectServer(
        _bstr_t(L"\\\\.\\ROOT\\CIMV2"),
        nullptr, nullptr,
        nullptr,
        0,
        nullptr,
        nullptr,
        &svc
    ));

    com_manager::CheckError(CoSetProxyBlanket(
       svc,
       RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
       nullptr,
       RPC_C_AUTHN_LEVEL_CALL,
       RPC_C_IMP_LEVEL_IMPERSONATE,
       nullptr,
       EOAC_NONE
    ));
}

void disk_lister::for_each_disk(std::function<void(const disk&) noexcept> f) {
    IEnumWbemClassObjectPtr enu;
    com_manager::CheckError(svc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT * FROM Win32_DiskDrive"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &enu
    ));

    for (;;) {
        IWbemClassObjectPtr obj;
        {
            ULONG uReturn = 0;
            try {
                com_manager::CheckError(enu->Next(WBEM_INFINITE, 1, &obj, &uReturn));
            } catch (const _com_error& e) {
                break;
            }
            if (uReturn == 0)
                break;
        }

        disk d;
        try {
            d.device_id = _bstr_t(prop_get(obj, L"DeviceID"));
            d.model = _bstr_t(prop_get(obj, L"Model"));
            d.size = prop_get(obj, L"Size");
            d.serial = _bstr_t(prop_get(obj, L"SerialNumber"));
            d.pnp_device_id = _bstr_t(prop_get(obj, L"PNPDeviceID"));
        } catch (const _com_error& e) {
            continue;
        }
        try {
            d.state = get_state(d.device_id);
        } catch (const windows_error& e) {
            continue;
        }

        f(d);
    }
}
