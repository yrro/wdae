#pragma once

#include <windows.h>

#include "explain.hpp"

_COM_SMARTPTR_TYPEDEF(ISupportErrorInfo, IID_ISupportErrorInfo);
_COM_SMARTPTR_TYPEDEF(IErrorInfo, IID_IErrorInfo);

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

    // Can't use _com_util::CheckError:
    // <https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=816427>
    inline static void CheckError(HRESULT hr) {
        if (FAILED(hr)) {
            throw _com_error(hr);
        }
    }

    // T must be a _com_ptr_t<_com_IIID_getter<Isomething, __Isomething_IID_getter>>
    template<typename T>
    inline static void CheckError(HRESULT hr, T i, GUID g) {
        if (FAILED(hr)) {
            IErrorInfoPtr ei;
            {
                ISupportErrorInfoPtr sei;
                if (SUCCEEDED(i.QueryInterface(IID_ISupportErrorInfo, &sei))) {
                    if (sei->InterfaceSupportsErrorInfo(g) == S_OK) {
                        GetErrorInfo(0, &ei);
                    }
                }
            }
            throw _com_error(hr, ei);
        }
    }
};
