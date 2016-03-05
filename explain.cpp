#include "explain.hpp"

#include <memory>
#include <sstream>

#include <comdef.h>

// replace with windows_category or maybe hresult_category
std::wstring wstrerror(DWORD error) {
    std::unique_ptr<wchar_t, decltype(&LocalFree)> errmsg(nullptr, LocalFree);
    {
        wchar_t* errmsg_p = nullptr;
        if (!FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<wchar_t*>(&errmsg_p),
            0, nullptr
        )) {
            std::wostringstream ss;
            ss << L"[FormatMessage failure: " << GetLastError() << ']';
            return ss.str();
        }
        errmsg.reset(errmsg_p);
    }
    return errmsg.get();
}

void explain(const wchar_t* msg, DWORD code, HWND hWnd) {
    std::wostringstream ss;
    ss << msg << ".\n\n" << code << ": " << wstrerror(code);
    MessageBoxW(hWnd, ss.str().c_str(), L"Windows Disk ACL Editor", MB_ICONEXCLAMATION);
}

void explain(const windows_error& e) {
    explain(e.msg.c_str(), e.code);
}

void explain(const _com_error& e, HWND hWnd) {
    // XXX use e.ErrorMessage and if e.ErrorInfo then the IErrorInfo functions
    std::wostringstream ss;
    ss << std::showbase << std::hex << e.Error() << '\n' << e.ErrorMessage();
    MessageBoxW(hWnd, ss.str().c_str(), L"Windows Disk ACL Editor", MB_ICONEXCLAMATION);
}
