#include "main_window.hpp"

#include <cassert>
#include <memory>

#include <windows.h>

#include <commctrl.h>
#include <windowsx.h>

#include "disks.hpp"
#include "explain.hpp"

namespace {
    const int control_margin = 10;

    enum idc {
        idc_disks_refresh,
        idc_disks_list
    };

    struct window_data {
        NONCLIENTMETRICS metrics;
        std::unique_ptr<HFONT__, decltype(&DeleteObject)> message_font;

        disk_lister disks;

        window_data()
            :message_font(nullptr, DeleteObject)
        {}
    };

    BOOL on_create(HWND hWnd, LPCREATESTRUCT /*lpcs*/) {
        std::unique_ptr<window_data> wd;
        try {
            auto temp = std::make_unique<window_data>();
            wd = std::move(temp);
        } catch (const windows_error& e) {
            explain(e);
            return FALSE;
        }

        {
            wd->metrics = NONCLIENTMETRICSW();
            wd->metrics.cbSize = sizeof wd->metrics;
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof wd->metrics, &wd->metrics, 0);
            wd->message_font.reset(CreateFontIndirect(&wd->metrics.lfMessageFont));
        }

        if (HWND b = CreateWindow(WC_BUTTON, L"Refresh disks",
            WS_CHILD | WS_VISIBLE,
            control_margin, control_margin, 200, 24,
            hWnd, reinterpret_cast<HMENU>(idc_disks_refresh),
            nullptr, nullptr)
        ) {
            SetWindowFont(b, wd->message_font.get(), true);
            PostMessage(hWnd, WM_COMMAND, MAKELONG(idc_disks_refresh, BN_CLICKED), reinterpret_cast<LPARAM>(b));
        }

        {
            //RECT rcClient;
            //GetClientRect(hWnd, &rcClient);

            if (HWND l = CreateWindow(WC_LISTVIEW, L"",
                WS_CHILD | WS_VISIBLE | LVS_REPORT,
                100, 100, 1000, 100,
                hWnd, reinterpret_cast<HMENU>(idc_disks_list),
                nullptr, nullptr)
            ) {
                LVCOLUMNW c;
                c.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

                c.iSubItem = 0;
                c.pszText = const_cast<LPWSTR>(L"Model");
                c.cx = 100;
                ListView_InsertColumn(l, 0, &c);

                c.iSubItem = 1;
                c.pszText = const_cast<LPWSTR>(L"Size");
                c.cx = 100;
                ListView_InsertColumn(l, 1, &c);

                c.iSubItem = 2;
                c.pszText = const_cast<LPWSTR>(L"Serial");
                c.cx = 100;
                ListView_InsertColumn(l, 2, &c);
            }
        }

        {
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wd.get()));
            wd.release();
        }

        return TRUE;
    }

    void on_destroy(HWND hWnd) {
        delete reinterpret_cast<window_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        PostQuitMessage(0);
    }

    LRESULT on_activate(HWND hWnd) {
        SetForegroundWindow(hWnd);
        ShowWindow(hWnd, SW_RESTORE);
        return 0;
    }

    void refresh_disks(HWND hWnd, window_data* wd) {
        wd->disks.for_each_disk([](const disk& disk) {
            explain(disk.size.c_str(), 0);
            //
        });
    }

    void on_command(HWND hWnd, WORD id, HWND /*hCtl*/, UINT codeNotify) {
        window_data* wd = reinterpret_cast<window_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        switch (id) {
        case idc_disks_refresh:
            switch (codeNotify) {
            case BN_CLICKED:
                refresh_disks(hWnd, wd);
                return;
            }
        }

        assert(0); // unknown control
    }

    const wchar_t main_window_class[] = L"{d716e220-19d9-4e82-bd5d-2b85562636d1}";

    const UINT msg_activate = RegisterWindowMessageW(L"{a921a9de-f8b9-4755-acf8-fb1bcca54c07}");
}

void main_window_register(HINSTANCE hInstance) {
    WNDCLASSEX wcex = WNDCLASSEX();
    wcex.cbSize = sizeof wcex;
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = main_window_wndproc;
    wcex.hInstance = hInstance;
    //wcex.hIcon = something;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wcex.lpszClassName = main_window_class;
    if (!RegisterClassEx(&wcex)) {
        explain(L"RegisterClassEx failed");
        PostQuitMessage(0);
        return;
    }
}

void main_window_create(HINSTANCE hInstance, int nCmdShow) {
    HWND hWnd = CreateWindowW(main_window_class, L"Windows Disk ACL Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 720,
        0, 0, hInstance, nullptr
    );
    if (!hWnd) {
        explain(L"CreateWindow failed");
        PostQuitMessage(0);
        return;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
}

void main_window_activate_prev_instance() {
    DWORD recipients = BSM_APPLICATIONS;
    BroadcastSystemMessage(BSF_POSTMESSAGE | BSF_IGNORECURRENTTASK, &recipients, msg_activate, 0, 0);
}

LRESULT CALLBACK main_window_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    HANDLE_MSG(hWnd, WM_CREATE, on_create);
    HANDLE_MSG(hWnd, WM_DESTROY, on_destroy);
    HANDLE_MSG(hWnd, WM_COMMAND, on_command);
    default:
        if (uMsg == msg_activate) {
            return on_activate(hWnd);
        } else {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}
