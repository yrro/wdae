#include <memory>

#include <windows.h>

#include <commctrl.h>

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
    {
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
