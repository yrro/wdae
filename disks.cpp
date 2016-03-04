#include "disks.hpp"

#include <memory>
#include <sstream>

#include "com_manager.hpp"
#include "explain.hpp"

#include <aclapi.h>
#include <sddl.h>

namespace {
    _variant_t prop_get(IWbemClassObject* o, LPCWSTR name) {
        VARIANT t;
        com_manager::CheckError(o->Get(name, 0, &t, nullptr, nullptr));
        return _variant_t(t, false);
    }

    std::wstring get_current_sddl(std::wstring device_id) {
        std::unique_ptr<void, decltype(&CloseHandle)> f(nullptr, CloseHandle);
        {
            HANDLE h = CreateFileW(
                device_id.c_str(),
                READ_CONTROL, FILE_SHARE_READ,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
            );
            if (h == INVALID_HANDLE_VALUE) {
                throw windows_error(L"CreateFile", GetLastError());
            }
            f.reset(h);
        }

        const SECURITY_INFORMATION i = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

        std::unique_ptr<void, decltype(&LocalFree)> sd(nullptr, LocalFree);
        {
            PSECURITY_DESCRIPTOR sdp = nullptr;
            DWORD r = GetSecurityInfo(
                f.get(), SE_FILE_OBJECT,
                i,
                nullptr, nullptr, nullptr, nullptr,
                &sdp
            );
            if (r != ERROR_SUCCESS) {
                throw windows_error(L"GetSecurityInfo", r);
            }
            sd.reset(sdp);
        }

        std::unique_ptr<wchar_t, decltype(&LocalFree)> sddl(nullptr, LocalFree);
        {
            wchar_t* p;
            if (!ConvertSecurityDescriptorToStringSecurityDescriptor(sd.get(), SDDL_REVISION_1, i, &p, nullptr)) {
                throw windows_error(L"ConvertSecurityDescriptorToStringSecurityDescriptor", GetLastError());
            }
            sddl.reset(p);
        }

        return std::wstring(sddl.get());
    }

    std::wstring get_setup_sddl(std::wstring pnp_device_id) {
        return L"dummy";
    }
}

disk_lister::disk_lister() {
    HRESULT hr;

    com_manager::CheckError(loc.CreateInstance(CLSID_WbemLocator));

    com_manager::CheckError(hr = loc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
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

void disk_lister::for_each_disk(std::function<void(const disk&)> f) {
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
        } catch (const _com_error& e) {
            continue;
        }
        try {
            d.model = _bstr_t(prop_get(obj, L"Model"));
        } catch (const _com_error& e) {}
        try {
            d.size = prop_get(obj, L"Size");
        } catch (const _com_error& e) {}
        try {
            d.serial = _bstr_t(prop_get(obj, L"SerialNumber"));
        } catch (const _com_error& e) {}
        try {
            d.pnp_device_id = _bstr_t(prop_get(obj, L"PNPDeviceID"));
        } catch (const _com_error& e) {}
        try {
            d.current_sddl = get_current_sddl(d.device_id);
        } catch (const windows_error& e) {
            std::wostringstream ss;
            ss << "[" << e.msg() << ": " << wstrerror(e.code()) << " (" << e.code() << ")]";
            d.current_sddl = ss.str();
        }
        try {
            d.setup_sddl = get_setup_sddl(d.pnp_device_id);
        } catch (const windows_error& e) {
            std::wostringstream ss;
            ss << "[" << e.msg() << ": " << wstrerror(e.code()) << " (" << e.code() << ")]";
            d.setup_sddl = ss.str();
        }
        f(d);
    }
}
