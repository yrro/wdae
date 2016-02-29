#pragma once

#include <experimental/optional>

#include <windows.h>

#include "explain.hpp"

class prev_instance_mutex {
    std::unique_ptr<void, decltype(&CloseHandle)> m;
    bool prev;

public:
    prev_instance_mutex(const wchar_t* mutex_name):
        m(nullptr, CloseHandle),
        prev(false)
    {
        m.reset(CreateMutexW(nullptr, FALSE, mutex_name));
        DWORD e = GetLastError();
        if (m) {
            prev = (e == ERROR_ALREADY_EXISTS);
        } else {
            throw windows_error(L"CreateMutex", e);
        }
    }

    std::experimental::optional<bool> prev_instance_exists() const {
        if (!m) {
            return std::experimental::nullopt;
        } else {
            return prev;
        }
    }
};
