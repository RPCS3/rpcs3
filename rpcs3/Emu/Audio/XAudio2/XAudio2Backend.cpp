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
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	HRESULT hr = XAudio2Create(instance.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		XAudio.error("XAudio2Create() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return;
	}

	hr = instance->CreateMasteringVoice(&m_master_voice, m_channels, 48000);
	if (FAILED(hr))
	{
		XAudio.error("CreateMasteringVoice() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return;
	}

	// All succeeded, "commit"
	m_xaudio2_instance = std::move(instance);
}

XAudio2Backend::~XAudio2Backend()
{
	if (m_source_voice != nullptr)
	{
		m_source_voice->Stop();
		m_source_voice->DestroyVoice();
	}

	if (m_master_voice != nullptr)
	{
		m_master_voice->DestroyVoice();
	}

	if (m_xaudio2_instance != nullptr)
	{
		m_xaudio2_instance->StopEngine();
	}
}

void XAudio2Backend::Play()
{
	AUDIT(m_source_voice != nullptr);

	const HRESULT hr = m_source_voice->Start();
	if (FAILED(hr))
	{
		XAudio.error("Start() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		Emu.Pause();
	}
}

void XAudio2Backend::Close()
{
	Pause();
	Flush();
}

void XAudio2Backend::Pause()
{
	AUDIT(m_source_voice != nullptr);

	const HRESULT hr = m_source_voice->Stop();
	if (FAILED(hr))
	{
		XAudio.error("Stop() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		Emu.Pause();
	}
}

void XAudio2Backend::Open(u32 /* num_buffers */)
{
	WAVEFORMATEX waveformatex;
	waveformatex.wFormatTag = m_convert_to_u16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = m_channels;
	waveformatex.nSamplesPerSec = m_sampling_rate;
	waveformatex.nAvgBytesPerSec = static_cast<DWORD>(m_sampling_rate * m_channels * m_sample_size);
	waveformatex.nBlockAlign = m_channels * m_sample_size;
	waveformatex.wBitsPerSample = m_sample_size * 8;
	waveformatex.cbSize = 0;

	const HRESULT hr = m_xaudio2_instance->CreateSourceVoice(&m_source_voice, &waveformatex, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
	if (FAILED(hr))
	{
		XAudio.error("CreateSourceVoice() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		Emu.Pause();
		return;
	}

	AUDIT(m_source_voice != nullptr);
	m_source_voice->SetVolume(1.0f);
}

bool XAudio2Backend::IsPlaying()
{
	AUDIT(m_source_voice != nullptr);

	XAUDIO2_VOICE_STATE state;
	m_source_voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	return state.BuffersQueued > 0 || state.pCurrentBufferContext != nullptr;
}

bool XAudio2Backend::AddData(const void* src, u32 num_samples)
{
	AUDIT(m_source_voice != nullptr);

	XAUDIO2_VOICE_STATE state;
	m_source_voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	if (state.BuffersQueued >= MAX_AUDIO_BUFFERS)
	{
		XAudio.warning("Too many buffers enqueued (%d)", state.BuffersQueued);
		return false;
	}

	XAUDIO2_BUFFER buffer;

	buffer.AudioBytes = num_samples * m_sample_size;
	buffer.Flags = 0;
	buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
	buffer.LoopCount = 0;
	buffer.LoopLength = 0;
	buffer.pAudioData = static_cast<const BYTE*>(src);
	buffer.pContext = nullptr;
	buffer.PlayBegin = 0;
	buffer.PlayLength = AUDIO_BUFFER_SAMPLES;

	const HRESULT hr = m_source_voice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr))
	{
		XAudio.error("AddData() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		Emu.Pause();
		return false;
	}

	return true;
}

void XAudio2Backend::Flush()
{
	AUDIT(m_source_voice != nullptr);

	const HRESULT hr = m_source_voice->FlushSourceBuffers();
	if (FAILED(hr))
	{
		XAudio.error("FlushSourceBuffers() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		Emu.Pause();
	}
}

u64 XAudio2Backend::GetNumEnqueuedSamples()
{
	AUDIT(m_source_voice != nullptr);

	XAUDIO2_VOICE_STATE state;
	m_source_voice->GetState(&state);

	// all buffers contain AUDIO_BUFFER_SAMPLES, so we can easily calculate how many samples there are remaining
	return static_cast<u64>(AUDIO_BUFFER_SAMPLES - state.SamplesPlayed % AUDIO_BUFFER_SAMPLES) + (state.BuffersQueued * AUDIO_BUFFER_SAMPLES);
}

f32 XAudio2Backend::SetFrequencyRatio(f32 new_ratio)
{
	new_ratio = std::clamp(new_ratio, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_DEFAULT_FREQ_RATIO);

	const HRESULT hr = m_source_voice->SetFrequencyRatio(new_ratio);
	if (FAILED(hr))
	{
		XAudio.error("SetFrequencyRatio() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		Emu.Pause();
		return 1.0f;
	}

	return new_ratio;
}
