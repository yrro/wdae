#pragma once

#include <string>

#include <windows.h>

std::wstring wstrerror(DWORD error);

void explain(const wchar_t* msg, DWORD e = GetLastError());
