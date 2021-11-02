#include "stdafx.h"
#include "audio_device_listener.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"
#include "Emu/Cell/Modules/cellAudio.h"
#include "Emu/IdManager.h"

LOG_CHANNEL(IO);

audio_device_listener::audio_device_listener()
{
#ifdef _WIN32
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		m_com_init_success = true;
	}

	// Try to register a listener for device changes
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_device_enumerator));
	if (hr != S_OK)
	{
		IO.error("CoCreateInstance() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}
	else if (m_device_enumerator)
	{
		hr = m_device_enumerator->RegisterEndpointNotificationCallback(this);
		if (FAILED(hr))
		{
			IO.error("RegisterEndpointNotificationCallback() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			m_device_enumerator->Release();
		}
	}
	else
	{
		IO.error("Device enumerator invalid");
	}
#endif
}

audio_device_listener::~audio_device_listener()
{
#ifdef _WIN32
	if (m_device_enumerator != nullptr)
	{
		m_device_enumerator->UnregisterEndpointNotificationCallback(this);
		m_device_enumerator->Release();
	}

	if (m_com_init_success)
	{
		CoUninitialize();
	}
#endif
}

bool audio_device_listener::input_device_changed()
{
	return m_input_device_changed.test_and_reset();
}

bool audio_device_listener::output_device_changed()
{
	return m_output_device_changed.test_and_reset();
}

#ifdef _WIN32
template <>
void fmt_class_string<ERole>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case eConsole: return "eConsole";
		case eMultimedia: return "eMultimedia";
		case eCommunications: return "eCommunications";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<EDataFlow>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case eRender: return "eRender";
		case eCapture: return "eCapture";
		case eAll: return "eAll";
		}

		return unknown;
	});
}

HRESULT audio_device_listener::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR new_default_device_id)
{
	IO.notice("OnDefaultDeviceChanged(flow=%s, role=%s, new_default_device_id=0x%x)", flow, role, new_default_device_id);

	if (!new_default_device_id)
	{
		IO.notice("OnDefaultDeviceChanged(): new_default_device_id empty");
		return S_OK;
	}

	// Listen only for one device role, otherwise we're going to receive more than one
	// notification for flow type
	if (role != eConsole)
	{
		IO.notice("OnDefaultDeviceChanged(): we don't care about this device");
		return S_OK;
	}

	const std::wstring tmp(new_default_device_id);
	const std::string new_device_id = wchar_to_utf8(tmp.c_str());

	if (flow == eRender || flow == eAll)
	{
		if (output_device_id != new_device_id)
		{
			output_device_id = new_device_id;

			IO.warning("Default output device changed: new device = '%s'", output_device_id);

			m_output_device_changed = true;
		}
	}

	if (flow == eCapture || flow == eAll)
	{
		if (input_device_id != new_device_id)
		{
			input_device_id = new_device_id;

			IO.warning("Default input device changed: new device = '%s'", input_device_id);

			m_input_device_changed = true;
		}
	}

	return S_OK;
}
#endif
