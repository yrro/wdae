#pragma once

#include <windows.h>

#include "explain.hpp"

class com_manager {
    bool initialized;

public:
    com_manager() {
        CheckError(CoInitializeEx(0, COINIT_MULTITHREADED));

        try {
            CheckError(CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
                RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                nullptr, EOAC_NONE, nullptr
            ));
        } catch (const _com_error& e) {
            CoUninitialize();
            throw;
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

    // https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=816427
    inline static void CheckError(HRESULT hr) {
        if (FAILED(hr)) {
            IErrorInfo* ei;
            GetErrorInfo(0, &ei); // does't actaully work!
            throw _com_error(hr, ei);
            //_com_issue_error(hr);
        }
    }
};
