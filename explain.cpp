#include "explain.hpp"

#include <memory>
#include <sstream>

std::wstring wstrerror(DWORD error) {
    std::unique_ptr<wchar_t, decltype(&LocalFree)> errmsg(nullptr, LocalFree);
    {
        wchar_t* errmsg_p = nullptr;
        if (!FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_IGNORE_INSERTS,
            0, error, 0, reinterpret_cast<wchar_t*>(&errmsg_p), 0, 0
        )) {
            std::wostringstream ss;
            ss << L"[FormatMessage failure: " << GetLastError() << ']';
            return ss.str();
        }
        errmsg.reset(errmsg_p);
    }
    return errmsg.get();
}

void explain(const wchar_t* msg, DWORD code) {
    std::wostringstream ss;
    ss << msg << ".\n\n" << code << ": " << wstrerror(code);
    MessageBoxW(0, ss.str().c_str(), L"Windows Disk ACL Editor", MB_ICONEXCLAMATION);
}

void explain(const windows_error& e) {
    explain(e.msg().c_str(), e.code());
}
