#include "main_window.hpp"

#include <windowsx.h>

#include "explain.hpp"

namespace {
    BOOL on_create(HWND hWnd, LPCREATESTRUCT /*lpcs*/) {
        return TRUE;
    }

    BOOL on_destroy(HWND hWnd) {
        PostQuitMessage(0);
        return 0;
    }

    BOOL on_activate(HWND hWnd) {
        SetForegroundWindow(hWnd);
        ShowWindow(hWnd, SW_RESTORE);
        return 0;
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
    default:
        if (uMsg == msg_activate) {
            return on_activate(hWnd);
        } else {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}
