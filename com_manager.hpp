#pragma once

#include <windows.h>

#include "explain.hpp"

class com_manager {
    bool initialized;

public:
    com_manager() {
        {
            HRESULT r = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (FAILED(r)) {
                throw windows_error(L"CoInitializeEx", r);
            }
        }

        {
            HRESULT hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
                RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                nullptr, EOAC_NONE, nullptr);
            if (FAILED(hr)) {
                CoUninitialize();
                throw windows_error(L"CoInitializeSecurity", hr);
            }
        }

        initialized = true;
    }

    ~com_manager() throw() {
        if (initialized) {
            CoUninitialize();
        }
    }

    com_manager(const com_manager&) = delete;
    com_manager& operator=(const com_manager&) = delete;

    com_manager(com_manager&& other):
        initialized(false)
    {
        std::swap(initialized, other.initialized);
    }

    com_manager& operator=(com_manager&& other) {
        if (this != &other) {
            if (this->initialized) {
                CoUninitialize();
                this->initialized = false;
            }
            std::swap(initialized, other.initialized);
        }
        return *this;
    }
};
