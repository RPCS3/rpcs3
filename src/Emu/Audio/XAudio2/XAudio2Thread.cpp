#include "stdafx.h"
#ifdef _MSC_VER
#include "Emu/System.h"
#include "Emu/state.h"

#include "XAudio2Thread.h"

XAudio2Thread::~XAudio2Thread()
{
	Quit();
}

XAudio2Thread::XAudio2Thread() : m_xaudio2_instance(nullptr), m_master_voice(nullptr), m_source_voice(nullptr)
{
}

void XAudio2Thread::Init()
{
	HRESULT hr = S_OK;

	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : CoInitializeEx() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}

	hr = XAudio2Create(&m_xaudio2_instance, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : XAudio2Create() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}

	hr = m_xaudio2_instance->CreateMasteringVoice(&m_master_voice);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : CreateMasteringVoice() failed(0x%08x)", (u32)hr);
		m_xaudio2_instance->Release();
		Emu.Pause();
	}
}

void XAudio2Thread::Quit()
{
	if (m_source_voice != nullptr) 
	{
		Stop();
		m_source_voice->DestroyVoice();
		m_source_voice = nullptr;
	}
	if (m_master_voice != nullptr)
	{
		m_master_voice->DestroyVoice();
		m_master_voice = nullptr;
	}
	if (m_xaudio2_instance != nullptr)
	{
		m_xaudio2_instance->StopEngine();
		m_xaudio2_instance->Release();
		m_xaudio2_instance = nullptr;
	}

	CoUninitialize();
}

void XAudio2Thread::Play()
{
	HRESULT hr = m_source_voice->Start();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : Start() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Thread::Close()
{
	Stop();
	HRESULT hr = m_source_voice->FlushSourceBuffers();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : FlushSourceBuffers() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Thread::Stop()
{
	HRESULT hr = m_source_voice->Stop();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : Stop() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Thread::Open(const void* src, int size)
{
	HRESULT hr;

	WORD sample_size = rpcs3::config.audio.convert_to_u16.value() ? sizeof(u16) : sizeof(float);
	WORD channels = 8;

	WAVEFORMATEX waveformatex;
	waveformatex.wFormatTag = rpcs3::config.audio.convert_to_u16.value() ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = channels;
	waveformatex.nSamplesPerSec = 48000;
	waveformatex.nAvgBytesPerSec = 48000 * (DWORD)channels * (DWORD)sample_size;
	waveformatex.nBlockAlign = channels * sample_size;
	waveformatex.wBitsPerSample = sample_size * 8;
	waveformatex.cbSize = 0;

	hr = m_xaudio2_instance->CreateSourceVoice(&m_source_voice, &waveformatex, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : CreateSourceVoice() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}

	AddData(src, size);
	Play();
}

void XAudio2Thread::AddData(const void* src, int size)
{
	XAUDIO2_BUFFER buffer;

	buffer.AudioBytes = size;
	buffer.Flags = 0;
	buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
	buffer.LoopCount = 0;
	buffer.LoopLength = 0;
	buffer.pAudioData = (const BYTE*)src;
	buffer.pContext = 0;
	buffer.PlayBegin = 0;
	buffer.PlayLength = 256;

	HRESULT hr = m_source_voice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : AddData() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}
#endif
