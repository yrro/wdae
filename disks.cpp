#include "disks.hpp"

#include "explain.hpp"

namespace {
    std::wstring prop_string(IWbemClassObject* o, LPCWSTR name) {
        _variant_t v;
        {
            VARIANT t;
            _com_util::CheckError(o->Get(name, 0, &t, nullptr, nullptr));
            v = _variant_t(t, false);
        }
        return std::wstring(_bstr_t(v));
    }
}

disk_lister::disk_lister() {
    HRESULT hr;

    _com_util::CheckError(loc.CreateInstance(CLSID_WbemLocator));

    _com_util::CheckError(hr = loc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        nullptr, nullptr,
        nullptr,
        0,
        nullptr,
        nullptr,
        &svc
    ));

    _com_util::CheckError(CoSetProxyBlanket(
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
    _com_util::CheckError(svc->ExecQuery(
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
            _com_util::CheckError(enu->Next(WBEM_INFINITE, 1, &obj, &uReturn));
            if (uReturn == 0)
                break;
        }

        disk d;
        d.device_id = prop_string(obj, L"DeviceID");
        d.model = prop_string(obj, L"Model");
        d.size = prop_string(obj, L"Size");
        d.serial = prop_string(obj, L"SerialNumber");
        d.pnp_device_id = prop_string(obj, L"PNPDeviceID");
        f(d);
    }
}
