#pragma once

#include <functional>
#include <string>

#include <windows.h>

#include <comdef.h>
#include <wbemcli.h>

_COM_SMARTPTR_TYPEDEF(IWbemLocator, IID_IWbemLocator);
_COM_SMARTPTR_TYPEDEF(IWbemServices, IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, IID_IEnumWbemClassObject);
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, IID_IWbemClassObject);

struct disk {
    std::wstring device_id;
    std::wstring model;
    std::uint64_t size;
    std::wstring serial;
    std::wstring pnp_device_id;
    std::wstring dacl;
};

class disk_lister {
    IWbemLocatorPtr loc;
    IWbemServicesPtr svc;

public:
    disk_lister();
    void for_each_disk(std::function<void(const disk&)>);
};
