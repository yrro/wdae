#pragma once

#include <experimental/optional>

#include <windows.h>

class prev_instance_mutex {
    std::unique_ptr<void, decltype(&CloseHandle)> m;
    DWORD e;

public:
    prev_instance_mutex(const wchar_t* mutex_name):
        m(nullptr, CloseHandle)
    {
        m.reset(CreateMutexW(nullptr, FALSE, mutex_name));
        e = GetLastError();
    }

    std::experimental::optional<DWORD> error() const {
        if (m)
            return std::experimental::nullopt;
        else
            return e;
    }

    bool prev_instance_exists() const {
        return m && e == ERROR_ALREADY_EXISTS;
    }
};
