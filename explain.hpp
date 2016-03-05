#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

#include <windows.h>

struct _com_error;

struct windows_error: public std::exception {
    // XXX use HRESULT_FROM_WIN32 to genralize for HRESULT?

    DWORD code;
    std::wstring msg;
    const char* file;
    int line;
    const char* function;

    windows_error(DWORD code, std::wstring msg=L"", const char* file=__builtin_FILE(), int line=__builtin_LINE(), const char* function=__builtin_FUNCTION()):
        code(code),
        msg(msg),
        file(file),
        line(line),
        function(function)
    {}

    virtual const char* what() const noexcept {
        return "[what() not implemented, call wwhat() instead]";
    }

    std::wstring wwhat() const noexcept {
        std::wostringstream ss;
        ss << file << L':' << line << L" (" << function << L')';
        if (!msg.empty()) {
            ss << L" ← " << msg;
        }
        return ss.str();
    }
};

std::wstring wstrerror(DWORD error);

void explain(const wchar_t* msg, DWORD e = GetLastError(), HWND hWnd = nullptr);
void explain(const windows_error& e);
void explain(const _com_error& e, HWND hWnd = nullptr);
