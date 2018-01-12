#ifdef _WIN32

#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"
#include "Emu/System.h"

#include "XAudio2Thread.h"
#include "3rdparty/XAudio2_7/XAudio2.h"

static thread_local HMODULE s_tls_xaudio2_lib{};
static thread_local IXAudio2* s_tls_xaudio2_instance{};
static thread_local IXAudio2MasteringVoice* s_tls_master_voice{};
static thread_local IXAudio2SourceVoice* s_tls_source_voice{};

void XAudio2Thread::xa27_init(void* lib2_7)
{
	s_tls_xaudio2_lib = (HMODULE)lib2_7;

	HRESULT hr = S_OK;

	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : CoInitializeEx() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}	

	hr = XAudio2Create(&s_tls_xaudio2_instance, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : XAudio2Create() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}

	hr = s_tls_xaudio2_instance->CreateMasteringVoice(&s_tls_master_voice, g_cfg.audio.downmix_to_2ch ? 2 : 8, 48000);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : CreateMasteringVoice() failed(0x%08x)", (u32)hr);
		s_tls_xaudio2_instance->Release();
		Emu.Pause();
	}
}

void XAudio2Thread::xa27_destroy()
{
	if (s_tls_source_voice != nullptr)
	{
		s_tls_source_voice->Stop();
		s_tls_source_voice->DestroyVoice();
	}

	if (s_tls_master_voice != nullptr)
	{
		s_tls_master_voice->DestroyVoice();
	}

	if (s_tls_xaudio2_instance != nullptr)
	{
		s_tls_xaudio2_instance->StopEngine();
		s_tls_xaudio2_instance->Release();
	}

	CoUninitialize();

	FreeLibrary(s_tls_xaudio2_lib);
}

void XAudio2Thread::xa27_play()
{
	HRESULT hr = s_tls_source_voice->Start();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : Start() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Thread::xa27_flush()
{
	HRESULT hr = s_tls_source_voice->FlushSourceBuffers();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : FlushSourceBuffers() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Thread::xa27_stop()
{
	HRESULT hr = s_tls_source_voice->Stop();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : Stop() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Thread::xa27_open()
{
	HRESULT hr;

	WORD sample_size = g_cfg.audio.convert_to_u16 ? sizeof(u16) : sizeof(float);
	WORD channels = g_cfg.audio.downmix_to_2ch ? 2 : 8;

	WAVEFORMATEX waveformatex;
	waveformatex.wFormatTag = g_cfg.audio.convert_to_u16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = channels;
	waveformatex.nSamplesPerSec = 48000;
	waveformatex.nAvgBytesPerSec = 48000 * (DWORD)channels * (DWORD)sample_size;
	waveformatex.nBlockAlign = channels * sample_size;
	waveformatex.wBitsPerSample = sample_size * 8;
	waveformatex.cbSize = 0;

	hr = s_tls_xaudio2_instance->CreateSourceVoice(&s_tls_source_voice, &waveformatex, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : CreateSourceVoice() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}

	s_tls_source_voice->SetVolume(g_cfg.audio.downmix_to_2ch ? 1.0f : 4.0f);
}

void XAudio2Thread::xa27_add(const void* src, int size)
{
	XAUDIO2_VOICE_STATE state;
	s_tls_source_voice->GetState(&state);

	// XAudio 2.7 bug workaround, when it says "SimpList: non-growable list ran out of room for new elements" and hits int 3
	if (state.BuffersQueued > 32)
	{
		LOG_WARNING(GENERAL, "XAudio2Thread : too many buffers enqueued (%d, pos=%u)", state.BuffersQueued, state.SamplesPlayed);

		return xa27_flush();
	}

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

	HRESULT hr = s_tls_source_voice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Thread : AddData() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

#endif
