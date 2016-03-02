#pragma once

#include <experimental/optional>
#include <memory>

#include <windows.h>

class prev_instance_mutex {
    std::unique_ptr<void, decltype(&CloseHandle)> m;
    std::experimental::optional<bool> prev;

public:
    prev_instance_mutex(const wchar_t* mutex_name):
        m(nullptr, CloseHandle)
    {
        m.reset(CreateMutexW(nullptr, FALSE, mutex_name));
        if (!m) {
            throw windows_error(L"CreateMutex", GetLastError());
        }
        prev = (GetLastError() == ERROR_ALREADY_EXISTS);
    }

    std::experimental::optional<bool> prev_instance_exists() const {
        return prev;
    }
};
