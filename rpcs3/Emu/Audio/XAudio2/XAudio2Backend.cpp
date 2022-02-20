#ifndef _WIN32
#error "XAudio2 can only be built on Windows."
#endif

#include <algorithm>
#include "util/logs.hpp"
#include "Emu/System.h"

#include "XAudio2Backend.h"
#include <Windows.h>
#include <system_error>

#pragma comment(lib, "xaudio2_9redist.lib")

LOG_CHANNEL(XAudio);

XAudio2Backend::XAudio2Backend()
	: AudioBackend()
{
	Microsoft::WRL::ComPtr<IXAudio2> instance;

	// In order to prevent errors on CreateMasteringVoice, apparently we need CoInitializeEx according to:
	// https://docs.microsoft.com/en-us/windows/win32/api/xaudio2fx/nf-xaudio2fx-xaudio2createvolumemeter
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		m_com_init_success = true;
	}

	hr = XAudio2Create(instance.GetAddressOf(), 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		XAudio.error("XAudio2Create() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return;
	}


	hr = instance->RegisterForCallbacks(this);
	if (FAILED(hr))
	{
		// Some error recovery functionality will be lost, but otherwise backend is operational
		XAudio.error("RegisterForCallbacks() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}

	// All succeeded, "commit"
	m_xaudio2_instance = std::move(instance);
}

XAudio2Backend::~XAudio2Backend()
{
	Close();

	if (m_xaudio2_instance != nullptr)
	{
		m_xaudio2_instance->StopEngine();

		// TODO: Enabling this might crash afterwards in ComPtr::InternalRelease.
		//       Maybe it's both trying to do the same thing?
		//m_xaudio2_instance->Release();
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
	if (m_dev_listener.output_device_changed())
	{
		m_reset_req = true;
	}

	return m_xaudio2_instance != nullptr && m_source_voice != nullptr && !m_reset_req.observe();
}

void XAudio2Backend::Play()
{
	if (m_source_voice == nullptr)
	{
		XAudio.error("Play() called uninitialized");
		return;
	}

	if (m_playing) return;

	const HRESULT hr = m_source_voice->Start();
	if (FAILED(hr))
	{
		XAudio.error("Start() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return;
	}

	std::lock_guard lock(m_cb_mutex);
	m_playing = true;
}

void XAudio2Backend::CloseUnlocked()
{
	if (m_source_voice != nullptr)
	{
		const HRESULT hr = m_source_voice->Stop();
		if (FAILED(hr))
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

	m_playing = false;
	m_data_buf = nullptr;
	m_data_buf_len = 0;
	m_last_sample.fill(0);
}

void XAudio2Backend::Close()
{
	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();
}

void XAudio2Backend::Pause()
{
	if (m_source_voice)
	{
		HRESULT hr = m_source_voice->Stop();
		if (FAILED(hr))
		{
			XAudio.error("Stop() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		}

		hr = m_source_voice->FlushSourceBuffers();
		if (FAILED(hr))
		{
			XAudio.error("FlushSourceBuffers() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		}
	}
	else
	{
		XAudio.error("Pause() called uninitialized");
	}

	std::lock_guard lock(m_cb_mutex);
	m_playing = false;
	m_last_sample.fill(0);
}

void XAudio2Backend::Open(AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt)
{
	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();

	if (m_xaudio2_instance == nullptr)
	{
		XAudio.error("Open() called unitiliazed");
		return;
	}

	HRESULT hr = m_xaudio2_instance->CreateMasteringVoice(&m_master_voice);
	if (FAILED(hr))
	{
		XAudio.error("CreateMasteringVoice() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		XAudio.error("Failed to open audio backend. Make sure that no other application is running that might block audio access (e.g. Netflix).");
		m_reset_req = true;
		return;
	}

	m_sampling_rate = freq;
	m_sample_size = sample_size;
	m_channels = ch_cnt;

	WAVEFORMATEX waveformatex{};
	waveformatex.wFormatTag = get_convert_to_s16() ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = get_channels();
	waveformatex.nSamplesPerSec = get_sampling_rate();
	waveformatex.nAvgBytesPerSec = static_cast<DWORD>(get_sampling_rate() * get_channels() * get_sample_size());
	waveformatex.nBlockAlign = get_channels() * get_sample_size();
	waveformatex.wBitsPerSample = get_sample_size() * 8;
	waveformatex.cbSize = 0;

	hr = m_xaudio2_instance->CreateSourceVoice(&m_source_voice, &waveformatex, 0, XAUDIO2_DEFAULT_FREQ_RATIO, this);
	if (FAILED(hr))
	{
		XAudio.error("CreateSourceVoice() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return;
	}

	ensure(m_source_voice != nullptr);
	m_source_voice->SetVolume(1.0f);

	m_data_buf_len = get_sampling_rate() * get_sample_size() * get_channels() * INTERNAL_BUF_SIZE_MS * static_cast<u32>(XAUDIO2_DEFAULT_FREQ_RATIO) / 1000;
	m_data_buf = std::make_unique<u8[]>(m_data_buf_len);
}

bool XAudio2Backend::IsPlaying()
{
	return m_playing;
}

void XAudio2Backend::SetWriteCallback(std::function<u32(u32, void *)> cb)
{
	std::lock_guard lock(m_cb_mutex);
	m_write_callback = cb;
}

f64 XAudio2Backend::GetCallbackFrameLen()
{
	constexpr f64 _10ms = 0.01;

	if (m_source_voice == nullptr)
	{
		XAudio.error("GetCallbackFrameLen() called uninitialized");
		return _10ms;
	}

	void *ext;
	f64 min_latency{};

	HRESULT hr = m_xaudio2_instance->QueryInterface(IID_IXAudio2Extension, &ext);
	if (FAILED(hr))
	{
		XAudio.error("QueryInterface() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	}
	else
	{
		u32 samples_per_q = 0, freq = 0;
		static_cast<IXAudio2Extension *>(ext)->GetProcessingQuantum(&samples_per_q, &freq);

		if (freq)
		{
			min_latency = static_cast<f64>(samples_per_q) / freq;
		}
	}

	return std::max<f64>(min_latency, _10ms); // 10ms is the minimum for XAudio
}

void XAudio2Backend::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{
	std::unique_lock lock(m_cb_mutex, std::defer_lock);
	if (BytesRequired && lock.try_lock() && m_write_callback && m_playing)
	{
		ensure(BytesRequired <= m_data_buf_len, "XAudio internal buffer is too small. Report to developers!");

		const u32 sample_size = get_sample_size() * get_channels();
		u32 written = std::min(m_write_callback(BytesRequired, m_data_buf.get()), BytesRequired);
		written -= written % sample_size;

		if (written >= sample_size)
		{
			memcpy(m_last_sample.data(), m_data_buf.get() + written - sample_size, sample_size);
		}

		for (u32 i = written; i < BytesRequired; i += sample_size)
		{
			memcpy(m_data_buf.get() + i, m_last_sample.data(), sample_size);
		}

		XAUDIO2_BUFFER buffer{};
		buffer.AudioBytes = BytesRequired;
		buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
		buffer.pAudioData = static_cast<const BYTE*>(m_data_buf.get());

		const HRESULT hr = m_source_voice->SubmitSourceBuffer(&buffer);
		if (FAILED(hr))
		{
			XAudio.error("SubmitSourceBuffer() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		}
	}
}

void XAudio2Backend::OnCriticalError(HRESULT Error)
{
	XAudio.error("OnCriticalError() called: %s (0x%08x)", std::system_category().message(Error), static_cast<u32>(Error));
	m_reset_req = true;
}
