#pragma once

#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propsys.h>
#include <propvarutil.h>

#include <string>
#include <vector>

namespace ammod::audio {

struct AudioDevice {
    std::wstring id;
    std::wstring name;
    bool isDefault{false};
};

inline std::wstring GetDeviceFriendlyName(IMMDevice* device) {
    if (!device) return {};
    IPropertyStore* store{};
    if (FAILED(device->OpenPropertyStore(STGM_READ, &store)) || !store) return {};
    PROPVARIANT value{};
    PropVariantInit(&value);
    std::wstring result;
    if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &value)) &&
        value.vt == VT_LPWSTR && value.pwszVal) {
        result = value.pwszVal;
    }
    PropVariantClear(&value);
    store->Release();
    return result;
}

inline std::wstring GetDeviceId(IMMDevice* device) {
    if (!device) return {};
    LPWSTR id{};
    if (FAILED(device->GetId(&id)) || !id) return {};
    std::wstring result = id;
    CoTaskMemFree(id);
    return result;
}

inline std::vector<AudioDevice> EnumerateOutputDevices() {
    std::vector<AudioDevice> devices;
    IMMDeviceEnumerator* enumerator{};
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr) || !enumerator) return devices;

    std::wstring defaultId;
    IMMDevice* defaultDevice{};
    if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice)) && defaultDevice) {
        defaultId = GetDeviceId(defaultDevice);
        defaultDevice->Release();
    }

    IMMDeviceCollection* collection{};
    if (SUCCEEDED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)) && collection) {
        UINT count = 0;
        if (SUCCEEDED(collection->GetCount(&count))) {
            for (UINT i = 0; i < count; ++i) {
                IMMDevice* device{};
                if (SUCCEEDED(collection->Item(i, &device)) && device) {
                    const auto id = GetDeviceId(device);
                    auto name = GetDeviceFriendlyName(device);
                    if (name.empty()) name = id;
                    if (!id.empty()) {
                        devices.push_back({id, name, id == defaultId});
                    }
                    device->Release();
                }
            }
        }
        collection->Release();
    }
    enumerator->Release();
    return devices;
}

inline std::wstring GetDeviceNameFromId(const std::wstring& endpointId) {
    if (endpointId.empty()) return {};
    IMMDeviceEnumerator* enumerator{};
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr) || !enumerator) return {};
    IMMDevice* device{};
    std::wstring name;
    if (SUCCEEDED(enumerator->GetDevice(endpointId.c_str(), &device)) && device) {
        name = GetDeviceFriendlyName(device);
        device->Release();
    }
    enumerator->Release();
    return name;
}

} // namespace ammod::audio
