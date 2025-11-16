#ifndef _WIN32
#error "XAudio2 can only be built on Windows."
#endif

#include <algorithm>
#include "util/logs.hpp"
#include "Emu/System.h"
#include "Emu/Audio/audio_device_enumerator.h"
#include "Utilities/StrUtil.h"

#include "XAudio2Backend.h"
#include <Windows.h>
#include <system_error>

#ifndef XAUDIO2_USE_DEFAULT_PROCESSOR
#define XAUDIO2_USE_DEFAULT_PROCESSOR XAUDIO2_DEFAULT_PROCESSOR
#endif

LOG_CHANNEL(XAudio);

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
		case ERole_enum_count: return unknown;
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
		case EDataFlow_enum_count: return unknown;
		}

		return unknown;
	});
}

XAudio2Backend::XAudio2Backend()
	: AudioBackend()
{
	Microsoft::WRL::ComPtr<IXAudio2> instance{};
	Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator{};

	// In order to prevent errors on CreateMasteringVoice, apparently we need CoInitializeEx according to:
	// https://docs.microsoft.com/en-us/windows/win32/api/xaudio2fx/nf-xaudio2fx-xaudio2createvolumemeter
	if (HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); SUCCEEDED(hr))
	{
		m_com_init_success = true;
	}

	if (HRESULT hr = XAudio2Create(&instance, 0, XAUDIO2_USE_DEFAULT_PROCESSOR); FAILED(hr))
	{
		XAudio.error("XAudio2Create() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return;
	}

	if (HRESULT hr = instance->RegisterForCallbacks(this); FAILED(hr))
	{
		// Some error recovery functionality will be lost, but otherwise backend is operational
		XAudio.error("RegisterForCallbacks() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}

	// Try to register a listener for device changes
	if (HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumerator)); FAILED(hr))
	{
		XAudio.error("CoCreateInstance() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return;
	}

	// All succeeded, "commit"
	m_xaudio2_instance = std::move(instance);
	m_device_enumerator = std::move(enumerator);
}

XAudio2Backend::~XAudio2Backend()
{
	Close();

	if (m_device_enumerator != nullptr)
	{
		m_device_enumerator->UnregisterEndpointNotificationCallback(this);
		m_device_enumerator = nullptr;
	}

	if (m_xaudio2_instance != nullptr)
	{
		m_xaudio2_instance->StopEngine();
		m_xaudio2_instance = nullptr;
	}

	if (m_com_init_success)
	{
		CoUninitialize();
	}
}

bool XAudio2Backend::Initialized()
{
	return m_xaudio2_instance != nullptr;
}

bool XAudio2Backend::Operational()
{
	return m_source_voice != nullptr && !m_reset_req.observe();
}

bool XAudio2Backend::DefaultDeviceChanged()
{
	std::lock_guard lock{m_state_cb_mutex};
	return !m_reset_req.observe() && m_default_dev_changed;
}

void XAudio2Backend::Play()
{
	if (m_source_voice == nullptr)
	{
		XAudio.error("Play() called uninitialized");
		return;
	}

	if (m_playing) return;

	std::lock_guard lock(m_cb_mutex);
	m_playing = true;
}

void XAudio2Backend::CloseUnlocked()
{
	if (m_source_voice != nullptr)
	{
		if (HRESULT hr = m_source_voice->Stop(); FAILED(hr))
		{
			XAudio.error("Stop() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		}

		m_source_voice->DestroyVoice();
		m_source_voice = nullptr;
	}

	if (m_master_voice != nullptr)
	{
		m_master_voice->DestroyVoice();
		m_master_voice = nullptr;
	}

	m_device_enumerator->UnregisterEndpointNotificationCallback(this);

	m_playing = false;
	m_last_sample.fill(0);

	std::lock_guard lock(m_state_cb_mutex);
	m_default_dev_changed = false;
	m_current_device.clear();
}

void XAudio2Backend::Close()
{
	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();
}

void XAudio2Backend::Pause()
{
	if (m_source_voice == nullptr)
	{
		XAudio.error("Pause() called uninitialized");
		return;
	}

	if (!m_playing) return;

	{
		std::lock_guard lock(m_cb_mutex);
		m_playing = false;
		m_last_sample.fill(0);
	}

	if (HRESULT hr = m_source_voice->FlushSourceBuffers(); FAILED(hr))
	{
		XAudio.error("FlushSourceBuffers() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}
}

bool XAudio2Backend::Open(std::string_view dev_id, AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt, audio_channel_layout layout)
{
	if (!Initialized())
	{
		XAudio.error("Open() called uninitialized");
		return false;
	}

	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();

	const bool use_default_device = dev_id.empty() || dev_id == audio_device_enumerator::DEFAULT_DEV_ID;
	std::string selected_dev_id{};

	if (use_default_device)
	{
		Microsoft::WRL::ComPtr<IMMDevice> default_dev{};
		if (HRESULT hr = m_device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &default_dev); FAILED(hr))
		{
			XAudio.error("GetDefaultAudioEndpoint() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			return false;
		}

		LPWSTR default_id{};
		if (HRESULT hr = default_dev->GetId(&default_id); FAILED(hr))
		{
			XAudio.error("GetId() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			return false;
		}

		selected_dev_id = wchar_to_utf8(std::wstring_view{default_id});
		CoTaskMemFree(default_id);
		if (selected_dev_id.empty())
		{
			XAudio.error("Default device id is empty");
			return false;
		}
	}

	if (HRESULT hr = m_xaudio2_instance->CreateMasteringVoice(&m_master_voice, 0, 0, 0, utf8_to_wchar(use_default_device ? selected_dev_id : dev_id).c_str()); FAILED(hr))
	{
		XAudio.error("CreateMasteringVoice() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		m_master_voice = nullptr;
		return false;
	}

	XAUDIO2_VOICE_DETAILS vd{};
	m_master_voice->GetVoiceDetails(&vd);

	if (vd.InputChannels == 0)
	{
		XAudio.error("Channel count 0 is invalid");
		CloseUnlocked();
		return false;
	}

	XAudio.notice("Channel count is %d", vd.InputChannels);

	m_sampling_rate = freq;
	m_sample_size = sample_size;

	setup_channel_layout(static_cast<u32>(ch_cnt), vd.InputChannels, layout, XAudio);

	WAVEFORMATEX waveformatex{};
	waveformatex.wFormatTag = get_convert_to_s16() ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = get_channels();
	waveformatex.nSamplesPerSec = get_sampling_rate();
	waveformatex.nAvgBytesPerSec = static_cast<DWORD>(get_sampling_rate() * get_channels() * get_sample_size());
	waveformatex.nBlockAlign = get_channels() * get_sample_size();
	waveformatex.wBitsPerSample = get_sample_size() * 8;
	waveformatex.cbSize = 0;

	if (HRESULT hr = m_xaudio2_instance->CreateSourceVoice(&m_source_voice, &waveformatex, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this); FAILED(hr))
	{
		XAudio.error("CreateSourceVoice() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		CloseUnlocked();
		return false;
	}

	if (HRESULT hr = m_source_voice->Start(); FAILED(hr))
	{
		XAudio.error("Start() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		CloseUnlocked();
		return false;
	}

	if (HRESULT hr = m_device_enumerator->RegisterEndpointNotificationCallback(this); FAILED(hr))
	{
		XAudio.error("RegisterEndpointNotificationCallback() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		CloseUnlocked();
		return false;
	}

	if (HRESULT hr = m_source_voice->SetVolume(1.0f); FAILED(hr))
	{
		XAudio.error("SetVolume() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}

	m_data_buf.resize(get_sampling_rate() * get_sample_size() * get_channels() * INTERNAL_BUF_SIZE_MS / 1000);

	if (use_default_device)
	{
		m_current_device = selected_dev_id;
	}

	return true;
}

f64 XAudio2Backend::GetCallbackFrameLen()
{
	constexpr f64 _10ms = 0.01;

	if (m_xaudio2_instance == nullptr)
	{
		XAudio.error("GetCallbackFrameLen() called uninitialized");
		return _10ms;
	}

	Microsoft::WRL::ComPtr<IXAudio2Extension> xaudio_ext{};
	f64 min_latency{};

	if (HRESULT hr = m_xaudio2_instance.As(&xaudio_ext); FAILED(hr))
	{
		XAudio.error("QueryInterface() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}
	else
	{
		u32 samples_per_q = 0, freq = 0;
		xaudio_ext->GetProcessingQuantum(&samples_per_q, &freq);

		if (freq)
		{
			min_latency = static_cast<f64>(samples_per_q) / freq;
		}
	}

	return std::max<f64>(min_latency, _10ms); // 10ms is the minimum for XAudio
}

void XAudio2Backend::OnVoiceProcessingPassStart(UINT32 BytesRequired) noexcept
{
	std::unique_lock lock(m_cb_mutex, std::defer_lock);
	if (BytesRequired && !m_reset_req.observe() && lock.try_lock_for(std::chrono::microseconds{50}) && m_write_callback && m_playing)
	{
		ensure(BytesRequired <= m_data_buf.size(), "XAudio internal buffer is too small. Report to developers!");

		const u32 sample_size = get_sample_size() * get_channels();
		u32 written = std::min(m_write_callback(BytesRequired, m_data_buf.data()), BytesRequired);
		written -= written % sample_size;

		if (written >= sample_size)
		{
			memcpy(m_last_sample.data(), m_data_buf.data() + written - sample_size, sample_size);
		}

		for (u32 i = written; i < BytesRequired; i += sample_size)
		{
			memcpy(m_data_buf.data() + i, m_last_sample.data(), sample_size);
		}

		XAUDIO2_BUFFER buffer{};
		buffer.AudioBytes = BytesRequired;
		buffer.pAudioData = static_cast<const BYTE*>(m_data_buf.data());
		// Avoid logging in callback and assume that this always succeeds, all errors are caught by error callback anyway
		m_source_voice->SubmitSourceBuffer(&buffer);
	}
}

void XAudio2Backend::OnCriticalError(HRESULT Error) noexcept
{
	XAudio.error("OnCriticalError() called: %s (0x%08x)", std::system_category().message(Error), static_cast<u32>(Error));

	std::lock_guard lock(m_state_cb_mutex);

	if (!m_reset_req.test_and_set() && m_state_callback)
	{
		m_state_callback(AudioStateEvent::UNSPECIFIED_ERROR);
	}
}

HRESULT XAudio2Backend::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR new_default_device_id)
{
	XAudio.notice("OnDefaultDeviceChanged(flow=%s, role=%s, new_default_device_id=0x%x)", flow, role, new_default_device_id);

	if (!new_default_device_id)
	{
		XAudio.notice("OnDefaultDeviceChanged(): new_default_device_id empty");
		return S_OK;
	}

	// Listen only for one device role, otherwise we're going to receive more than one notification for flow type
	if (role != eConsole)
	{
		XAudio.notice("OnDefaultDeviceChanged(): we don't care about this device");
		return S_OK;
	}

	std::lock_guard lock(m_state_cb_mutex);

	// Non default device is used
	if (m_current_device.empty())
	{
		return S_OK;
	}

	const std::string new_device_id = wchar_to_utf8(std::wstring_view{new_default_device_id});

	if (flow == eRender || flow == eAll)
	{
		if (!m_reset_req.observe() && new_device_id != m_current_device)
		{
			m_default_dev_changed = true;

			if (m_state_callback)
			{
				m_state_callback(AudioStateEvent::DEFAULT_DEVICE_MAYBE_CHANGED);
			}
		}
	}

	return S_OK;
}
