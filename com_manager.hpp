#pragma once

#include <experimental/optional>

#include <windows.h>

class com_manager {
    std::experimental::optional<DWORD> e;

public:
    com_manager() {
        HRESULT r = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(r))
            e = r;
    }

    ~com_manager() {
        if (e) {
            CoUninitialize();
        }
    }

    std::experimental::optional<DWORD> error() const {
        return e;
    }

    com_manager(const com_manager&) = delete;
    com_manager(com_manager&&) = delete;
    com_manager& operator=(const com_manager&) = delete;
    com_manager& operator=(com_manager&&) = delete;
};
