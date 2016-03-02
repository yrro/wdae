#include "disks.hpp"

#include "com_manager.hpp"
#include "explain.hpp"

namespace {
    _variant_t prop_get(IWbemClassObject* o, LPCWSTR name) {
        VARIANT t;
        com_manager::CheckError(o->Get(name, 0, &t, nullptr, nullptr));
        return _variant_t(t, false);
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
            com_manager::CheckError(enu->Next(WBEM_INFINITE, 1, &obj, &uReturn));
            if (uReturn == 0)
                break;
        }

        disk d;
        d.device_id = _bstr_t(prop_get(obj, L"DeviceID"));
        d.model = _bstr_t(prop_get(obj, L"Model"));
        d.size = prop_get(obj, L"Size");
        d.serial = _bstr_t(prop_get(obj, L"SerialNumber"));
        d.pnp_device_id = _bstr_t(prop_get(obj, L"PNPDeviceID"));
        f(d);
    }
}
