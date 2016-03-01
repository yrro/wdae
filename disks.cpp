#include "disks.hpp"

#include "explain.hpp"

namespace {
    std::wstring prop_string(const IWbemClassObject* o, LPCWSTR name) {
        _variant_t v;
        {
            VARIANT t;
            HRESULT hr = const_cast<IWbemClassObject*>(o)->Get(name, 0, &t, nullptr, nullptr);
            if (FAILED(hr))
                return L"";
            v.Attach(t);
        }
        return std::wstring(static_cast<_bstr_t>(v));
    }
}

disk_lister::disk_lister() {
    {
        HRESULT hr = loc.CreateInstance(CLSID_WbemLocator);
        if (FAILED(hr)) {
            throw windows_error(L"CoCreateInstance(CLSID_WbemLocator", hr);
        }
    }

    {
        IWbemServices* p;
        HRESULT hr = loc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"),
            nullptr, nullptr,
            nullptr,
            0,
            nullptr,
            nullptr,
            &p
        );
        if (FAILED(hr)) {
            throw windows_error(L"ConnectServer failed", hr);
        }
        svc = decltype(svc)(p, false);
    }

    {
        HRESULT hr = CoSetProxyBlanket(
           svc,
           RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
           nullptr,
           RPC_C_AUTHN_LEVEL_CALL,
           RPC_C_IMP_LEVEL_IMPERSONATE,
           nullptr,
           EOAC_NONE
        );
        if (FAILED(hr)) {
            throw windows_error(L"CoSetProxyBlanket failed", hr);
        }
    }
}

void disk_lister::for_each_disk(std::function<void(const disk&)> f) {
    IEnumWbemClassObjectPtr enu;
    {
        IEnumWbemClassObject* p;
        HRESULT hr = svc->ExecQuery(
            _bstr_t(L"WQL"),
            _bstr_t(L"SELECT * FROM Win32_DiskDrive"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            nullptr,
            &p);
        if (FAILED(hr)) {
            explain(L"ExecQuery failed", hr);
            return;
        }
        enu = decltype(enu)(p, false);
    }

    for (;;) {
        IWbemClassObjectPtr obj;
        {
            IWbemClassObject* p;
            ULONG uReturn = 0;
            HRESULT hr = enu->Next(WBEM_INFINITE, 1, &p, &uReturn);
            if (FAILED(hr) || uReturn == 0)
                break;
            obj = decltype(obj)(p, false);
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
