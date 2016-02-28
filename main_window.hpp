#pragma once

#include <windows.h>

void main_window_register(HINSTANCE hInstance);

void main_window_create(HINSTANCE hInstance, int nCmdShow);

void main_window_activate_prev_instance();

LRESULT CALLBACK main_window_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
