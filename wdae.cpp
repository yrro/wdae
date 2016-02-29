#include <memory>

#include <windows.h>

#include <comdef.h>
#include <comdefsp.h>
#include <comip.h>
#include <commctrl.h>
#include <wbemcli.h>

#include "com_manager.hpp"
#include "explain.hpp"
#include "main_window.hpp"
#include "prev.hpp"

namespace {
    const wchar_t application_mutex[] = L"{155421ec-c63a-40cc-a2e6-11694133f30b}";

    void init_common_controls() {
        INITCOMMONCONTROLSEX icc = INITCOMMONCONTROLSEX();
        icc.dwSize = sizeof icc;
        icc.dwICC = ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);
    }

    _COM_SMARTPTR_TYPEDEF(IWbemLocator, IID_IWbemLocator);
    _COM_SMARTPTR_TYPEDEF(IWbemServices, IID_IWbemServices);
    _COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, IID_IEnumWbemClassObject);
    _COM_SMARTPTR_TYPEDEF(IWbemClassObject, IID_IWbemClassObject);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int nCmdShow) {
    init_common_controls();

    prev_instance_mutex m(application_mutex);
    if (m.error()) {
        explain(L"CreateMutex failed", *(m.error()));
        return 0;
    } else if (m.prev_instance_exists()) {
        main_window_activate_prev_instance();
        return 0;
    }

    com_manager c;
    if (c.error()) {
        explain(L"CoInitializeEx failed", *(c.error()));
        return 0;
    }

    HRESULT hr;
    hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    if (FAILED(hr)) {
        explain(L"CoInitializeSecurity failed", hr);
        return 0;
    }

    IWbemLocatorPtr loc;
    hr = loc.CreateInstance(CLSID_WbemLocator);
    if (FAILED(hr)) {
        explain(L"CoCreateInstance(CLSID_WbemLocator) failed", hr);
        return 0;
    }

    IWbemServicesPtr svc;
    {
        IWbemServices* svc_raw;
        hr = loc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"),
            nullptr, nullptr,
            nullptr,
            0,
            nullptr,
            nullptr,
            &svc_raw
        );
        if (FAILED(hr)) {
            explain(L"ConnectServer failed", hr);
            return 0;
        }
        svc = decltype(svc)(svc_raw, false);
    }

    hr = CoSetProxyBlanket(
       svc,
       RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
       nullptr,
       RPC_C_AUTHN_LEVEL_CALL,
       RPC_C_IMP_LEVEL_IMPERSONATE,
       nullptr,
       EOAC_NONE
    );
    if (FAILED(hr)) {
        explain(L"CoSetProxyBlanket failed", hr);
        return 0;
    }

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
            return 0;
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

    main_window_register(hInstance);
    main_window_create(hInstance, nCmdShow);

    BOOL r;
    MSG msg;
    while ((r = GetMessage(&msg, nullptr, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (r == -1) {
        explain(L"GetMessage failed");
        return 0;
    }
    return msg.wParam;
}
