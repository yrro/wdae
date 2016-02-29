#pragma once

#include <functional>

#include <memory>

#include <windows.h>

#include <comdef.h>
#include <comdefsp.h>
#include <comip.h>
#include <wbemcli.h>

#include "explain.hpp"

_COM_SMARTPTR_TYPEDEF(IWbemLocator, IID_IWbemLocator);
_COM_SMARTPTR_TYPEDEF(IWbemServices, IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, IID_IEnumWbemClassObject);
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, IID_IWbemClassObject);

struct disk {
    std::wstring device_id;
    std::wstring model;
    std::wstring size;
    std::wstring serial;
    std::wstring pnp_device_id;
};

class disk_lister {
    IWbemLocatorPtr loc;
    IWbemServicesPtr svc;

public:
    disk_lister();
    void for_each_disk(std::function<void(const disk&)>);
};
