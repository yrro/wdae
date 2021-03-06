#include "main_window.hpp"

#include <experimental/optional>
#include <iomanip>
#include <cassert>
#include <memory>
#include <sstream>
#include <vector>

#include <windows.h>

#include <commctrl.h>
#include <windowsx.h>

#include "disks.hpp"
#include "explain.hpp"

namespace {
    const int control_margin = 10;

    enum idc {
        idc_disks_refresh,
        idc_disks_list,
        idc_vm_list
    };

    enum disk_listview_sub_item {
        disk_listview_sub_device_id = 0,
        disk_listview_sub_model,
        disk_listview_sub_size10,
        disk_listview_sub_serial,
        disk_listview_sub_state
    };

    struct window_data {
        NONCLIENTMETRICS metrics;
        std::unique_ptr<HFONT__, decltype(&DeleteObject)> message_font;

        disk_lister lister;
        std::vector<disk> disks;

        HWND disk_refresh_button;
        HWND disk_listview;
        HWND vm_listview;

        window_data()
            :message_font(nullptr, DeleteObject)
        {}
    };

    BOOL on_create(HWND hWnd, LPCREATESTRUCT /*lpcs*/) {
        std::unique_ptr<window_data> wd;
        try {
            wd = std::make_unique<window_data>();
        } catch (const _com_error& e) {
            explain(e);
            return FALSE;
        }

        {
            wd->metrics = NONCLIENTMETRICSW();
            wd->metrics.cbSize = sizeof wd->metrics;
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof wd->metrics, &wd->metrics, 0);
            wd->message_font.reset(CreateFontIndirect(&wd->metrics.lfMessageFont));
        }

        if ((wd->disk_refresh_button = CreateWindow(WC_BUTTON, L"Refresh disks",
            WS_CHILD | WS_VISIBLE,
            control_margin, control_margin, 100, 100,
            hWnd, reinterpret_cast<HMENU>(idc_disks_refresh),
            nullptr, nullptr
        ))) {
            SetWindowFont(wd->disk_refresh_button, wd->message_font.get(), true);
            PostMessage(hWnd, WM_COMMAND, MAKELONG(idc_disks_refresh, BN_CLICKED), reinterpret_cast<LPARAM>(wd->disk_refresh_button));
        }

        if ((wd->disk_listview = CreateWindow(WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            50, 300, 300, 100,
            hWnd, reinterpret_cast<HMENU>(idc_disks_list),
            nullptr, nullptr
        ))) {
            (void)ListView_SetExtendedListViewStyle(wd->disk_listview, LVS_EX_FULLROWSELECT);

            LVCOLUMNW c;
            c.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;

            c.fmt = LVCFMT_LEFT;
            c.pszText = const_cast<LPWSTR>(L"Device");
            c.cx = 130;
            (void)ListView_InsertColumn(wd->disk_listview, disk_listview_sub_device_id, &c);

            c.fmt = LVCFMT_LEFT;
            c.pszText = const_cast<LPWSTR>(L"Model");
            c.cx = 250;
            (void)ListView_InsertColumn(wd->disk_listview, disk_listview_sub_model, &c);

            c.fmt = LVCFMT_RIGHT;
            c.pszText = const_cast<LPWSTR>(L"Size (GB)");
            c.cx = 60;
            (void)ListView_InsertColumn(wd->disk_listview, disk_listview_sub_size10, &c);

            c.fmt = LVCFMT_LEFT;
            c.pszText = const_cast<LPWSTR>(L"Serial");
            c.cx = 160;
            (void)ListView_InsertColumn(wd->disk_listview, disk_listview_sub_serial, &c);

            c.fmt = LVCFMT_LEFT;
            c.pszText = const_cast<LPWSTR>(L"State");
            c.cx = 180;
            (void)ListView_InsertColumn(wd->disk_listview, disk_listview_sub_state, &c);
        }

        wd->vm_listview = CreateWindow(WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | LVS_LIST | LVS_SINGLESEL,
            50, 410, 300, 100,
            hWnd, reinterpret_cast<HMENU>(idc_vm_list),
            nullptr, nullptr
        );

        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wd.release()));
        return TRUE;
    }

    void on_destroy(HWND hWnd) {
        delete reinterpret_cast<window_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        PostQuitMessage(0);
    }

    void on_activate(HWND hWnd) {
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    }

    void refresh_disks(HWND hWnd, window_data* wd) {
        try {
            (void)ListView_DeleteAllItems(wd->disk_listview);
            wd->disks.clear();

            wd->lister.for_each_disk([&](const disk& disk) noexcept {
                {
                    LVITEMW item;
                    item.mask = LVIF_TEXT;
                    item.iItem = wd->disks.size();
                    item.iSubItem = disk_listview_sub_device_id;
                    item.pszText = const_cast<wchar_t*>(disk.device_id.c_str());
                    (void)ListView_InsertItem(wd->disk_listview, &item);
                }

                ListView_SetItemText(wd->disk_listview, wd->disks.size(), disk_listview_sub_model, const_cast<wchar_t*>(disk.model.c_str()));
                {
                    std::wostringstream ss;
                    ss << std::fixed << std::setprecision(0) << disk.size / 1'000'000'000.;
                    ListView_SetItemText(wd->disk_listview, wd->disks.size(), disk_listview_sub_size10, const_cast<wchar_t*>(ss.str().c_str()));
                }

                ListView_SetItemText(wd->disk_listview, wd->disks.size(), disk_listview_sub_serial, const_cast<wchar_t*>(disk.serial.c_str()));

                switch(disk.state) {
                case accessible:
                    ListView_SetItemText(wd->disk_listview, wd->disks.size(), disk_listview_sub_state, const_cast<wchar_t*>(L"accessible"));
                    break;
                case inaccessible:
                    ListView_SetItemText(wd->disk_listview, wd->disks.size(), disk_listview_sub_state, const_cast<wchar_t*>(L"inaccessible"));
                    break;
                }

                wd->disks.emplace_back(disk);
            });
        } catch (const _com_error& e) {
            explain(e, hWnd);
        }
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

    void on_size(HWND hWnd, UINT, int width, int height) {
        window_data* wd = reinterpret_cast<window_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        {
            SIZE s;
            if (Button_GetIdealSize(wd->disk_refresh_button, &s)) {
                MoveWindow(wd->disk_refresh_button, control_margin, control_margin, s.cx, s.cy, false);
            }
        }

        {
            RECT r;
            if (GetClientRect(wd->disk_refresh_button, &r)) {
                POINT p = {0, r.bottom + control_margin};
                if (MapWindowPoints(wd->disk_refresh_button, hWnd, &p, 1)) {
                    int list_heights = height - p.y - 2 * control_margin;
                    MoveWindow(wd->disk_listview, control_margin, p.y, width - 2 * control_margin, list_heights / 2 - control_margin / 2, false);
                    MoveWindow(wd->vm_listview, control_margin, p.y + list_heights/2 + control_margin, width - 2 * control_margin, list_heights / 2 - control_margin / 2, false);
                }
            }
        }
    }

    void on_getminmaxinfo(HWND, MINMAXINFO* m) {
        m->ptMinTrackSize = {640, 480};
    }

    void on_disk_choose(HWND hWnd, std::experimental::optional<int> index) {
        window_data* wd = reinterpret_cast<window_data*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        LVITEMW item;
        item.mask = LVIF_TEXT;
        item.iItem = 0; // size of vm list
        item.iSubItem = 0;
        item.pszText = const_cast<wchar_t*>(index ? wd->disks[*index].device_id.c_str() : L"nothing");
        (void)ListView_InsertItem(wd->vm_listview, &item);
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
    BroadcastSystemMessage(BSF_ALLOWSFW | BSF_POSTMESSAGE | BSF_IGNORECURRENTTASK, &recipients, msg_activate, 0, 0);
}

LRESULT CALLBACK main_window_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    HANDLE_MSG(hWnd, WM_CREATE, on_create);
    HANDLE_MSG(hWnd, WM_DESTROY, on_destroy);
    HANDLE_MSG(hWnd, WM_COMMAND, on_command);
    HANDLE_MSG(hWnd, WM_SIZE, on_size);
    HANDLE_MSG(hWnd, WM_GETMINMAXINFO, on_getminmaxinfo);
    case WM_NOTIFY:
        switch (LOWORD(wParam)) {
        case idc_disks_list:
            switch (reinterpret_cast<NMHDR*>(lParam)->code) {
            case LVN_ITEMCHANGED:
                // doesn't catch selected item being removed
                const NMLISTVIEW* info = reinterpret_cast<NMLISTVIEW*>(lParam);
                if (info->uChanged & LVIF_STATE) {
                    if (!(info->uOldState & LVIS_SELECTED) && (info->uNewState & LVIS_SELECTED)) {
                        on_disk_choose(hWnd, info->iItem);
                    } else if ((info->uOldState & LVIS_SELECTED) && !(info->uNewState & LVIS_SELECTED)) {
                        on_disk_choose(hWnd, std::experimental::nullopt);
                    }
                }
                return 0;
            }
            return 0;
        }
        return 0;
    }
    if (uMsg == msg_activate) {
        return on_activate(hWnd), 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
