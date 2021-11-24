#pragma once

#ifdef _WIN32
#include <MMDeviceAPI.h>
#endif

#include "util/atomic.hpp"

#ifdef _WIN32
class audio_device_listener : public IMMNotificationClient
#else
class audio_device_listener
#endif
{
public:
	audio_device_listener();
	~audio_device_listener();

	bool input_device_changed();
	bool output_device_changed();

private:
#ifdef _WIN32
	std::string input_device_id;
	std::string output_device_id;

	IFACEMETHODIMP_(ULONG) AddRef() override { return 1; };
	IFACEMETHODIMP_(ULONG) Release() override { return 1; };
	IFACEMETHODIMP QueryInterface(REFIID /*iid*/, void** /*object*/) override { return S_OK; };
	IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR /*device_id*/, const PROPERTYKEY /*key*/) override { return S_OK; };
	IFACEMETHODIMP OnDeviceAdded(LPCWSTR /*device_id*/) override { return S_OK; };
	IFACEMETHODIMP OnDeviceRemoved(LPCWSTR /*device_id*/) override { return S_OK; };
	IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR /*device_id*/, DWORD /*new_state*/) override { return S_OK; };
	IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR new_default_device_id) override;

	IMMDeviceEnumerator* m_device_enumerator = nullptr;
	bool m_com_init_success = false;
#endif

	atomic_t<bool> m_input_device_changed = false;
	atomic_t<bool> m_output_device_changed = false;
};
