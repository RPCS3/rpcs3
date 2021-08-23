#pragma once

#ifdef _WIN32
#include <MMDeviceAPI.h>
#endif

class audio_device_listener
{
public:
	audio_device_listener();
	~audio_device_listener();

private:
#ifdef _WIN32
	struct listener : public IMMNotificationClient
	{
		std::string device_id;

		IFACEMETHODIMP_(ULONG) AddRef() override { return 1; };
		IFACEMETHODIMP_(ULONG) Release() override { return 1; };
		IFACEMETHODIMP QueryInterface(REFIID iid, void** object) override { return S_OK; };
		IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR device_id, const PROPERTYKEY key) override { return S_OK; };
		IFACEMETHODIMP OnDeviceAdded(LPCWSTR device_id) override { return S_OK; };
		IFACEMETHODIMP OnDeviceRemoved(LPCWSTR device_id) override { return S_OK; };
		IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR device_id, DWORD new_state) override { return S_OK; };
		IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR new_default_device_id) override;
	} m_listener;

	IMMDeviceEnumerator* m_device_enumerator = nullptr;
#endif
};
