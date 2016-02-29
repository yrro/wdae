#include "disks.hpp"

#include "explain.hpp"

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

void disk_lister::refresh() {
    HRESULT hr;

    IEnumWbemClassObjectPtr enu;
    {
        IEnumWbemClassObject* enu_raw;
        hr = svc->ExecQuery(
            _bstr_t(L"WQL"),
            _bstr_t(L"SELECT * FROM Win32_DiskDrive"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            nullptr,
            &enu_raw);
        if (FAILED(hr)) {
            explain(L"ExecQuery failed", hr);
            return;
        }
        enu = decltype(enu)(enu_raw, false);
    }

    for (;;) {
        IWbemClassObjectPtr obj;
        {
            IWbemClassObject* obj_raw;
            ULONG uReturn = 0;
            hr = enu->Next(WBEM_INFINITE, 1, &obj_raw, &uReturn);
            if (uReturn == 0)
                break;
            obj = decltype(obj)(obj_raw, false);
        }

        VARIANT vtProp __attribute__((cleanup(VariantClear)));
        hr = obj->Get(L"PNPDeviceId", 0, &vtProp, 0, 0);
        if (FAILED(hr)) {
            explain(L"Get failed", hr);
            continue;
        }
        MessageBoxW(0, vtProp.bstrVal, L"Windows Disk ACL Editor", MB_ICONEXCLAMATION);
    }
}
