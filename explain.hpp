#pragma once

#include <stdexcept>
#include <string>

#include <windows.h>

#include <comdef.h>

class windows_error: public std::runtime_error {
    std::wstring _msg;
    DWORD _code;

public:
    windows_error(std::wstring msg, DWORD code):
        runtime_error("windows_error"),
        _msg(msg),
        _code(code)
    {}

    virtual const char* what() const throw() {
        return "[windows_exception::what() not implemented";
    }

    const std::wstring& msg() const throw() {
        return _msg;
    }

    DWORD code() const throw() {
        return _code;
    }
};

std::wstring wstrerror(DWORD error);

void explain(const wchar_t* msg, DWORD e = GetLastError(), HWND hWnd = nullptr);
void explain(const windows_error& e);
void explain(const _com_error& e, HWND hWnd = nullptr);
