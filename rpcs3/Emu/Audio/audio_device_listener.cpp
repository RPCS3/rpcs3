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
	// Try to register a listener for device changes
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_device_enumerator));
	if (hr != S_OK)
	{
		IO.error("CoCreateInstance() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}
	else if (m_device_enumerator)
	{
		m_device_enumerator->RegisterEndpointNotificationCallback(&m_listener);
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
		m_device_enumerator->Release();
	}
#endif
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

HRESULT audio_device_listener::listener::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR new_default_device_id)
{
	IO.notice("OnDefaultDeviceChanged(flow=%s, role=%s, new_default_device_id=0x%x)", flow, role, new_default_device_id);

	if (!new_default_device_id)
	{
		IO.notice("OnDefaultDeviceChanged(): new_default_device_id empty");
		return S_OK;
	}

	// Only listen for console and communication device changes.
	if ((role != eConsole && role != eCommunications) || (flow != eRender && flow != eCapture))
	{
		IO.notice("OnDefaultDeviceChanged(): we don't care about this device");
		return S_OK;
	}

	const std::wstring tmp(new_default_device_id);
	const std::string new_device_id = wchar_to_utf8(tmp.c_str());

	if (device_id != new_device_id)
	{
		device_id = new_device_id;

		IO.warning("Default device changed: new device = '%s'", device_id);

		if (auto& g_audio = g_fxo->get<cell_audio>(); g_fxo->is_init<cell_audio>())
		{
			g_audio.m_update_configuration = true;
		}
	}

	return S_OK;
}
#endif
