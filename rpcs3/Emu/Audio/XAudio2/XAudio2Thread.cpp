#ifdef _WIN32
#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"
#include "Utilities/Config.h"
#include "Emu/System.h"

#include "XAudio2Thread.h"

extern cfg::bool_entry g_cfg_audio_convert_to_u16;

XAudio2Thread::XAudio2Thread()
	: m_xaudio2_instance(nullptr)
	, m_master_voice(nullptr)
	, m_source_voice(nullptr)
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

XAudio2Thread::~XAudio2Thread()
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
		m_xaudio2_instance->Release();
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

	WORD sample_size = g_cfg_audio_convert_to_u16 ? sizeof(u16) : sizeof(float);
	WORD channels = 8;

	WAVEFORMATEX waveformatex;
	waveformatex.wFormatTag = g_cfg_audio_convert_to_u16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
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

	m_source_voice->SetVolume(4.0);

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
