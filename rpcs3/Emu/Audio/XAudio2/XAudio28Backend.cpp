#ifdef _WIN32

#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"
#include "Emu/System.h"

#include "XAudio2Backend.h"
#include "3rdparty/minidx12/Include/xaudio2.h"

static thread_local HMODULE s_tls_xaudio2_lib{};
static thread_local IXAudio2* s_tls_xaudio2_instance{};
static thread_local IXAudio2MasteringVoice* s_tls_master_voice{};
static thread_local IXAudio2SourceVoice* s_tls_source_voice{};

void XAudio2Backend::xa28_init(void* lib)
{
	s_tls_xaudio2_lib = (HMODULE)lib;

	const auto create = (XAudio2Create)GetProcAddress(s_tls_xaudio2_lib, "XAudio2Create");

	HRESULT hr = S_OK;

	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : CoInitializeEx() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}	

	hr = create(&s_tls_xaudio2_instance, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : XAudio2Create() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}

	hr = s_tls_xaudio2_instance->CreateMasteringVoice(&s_tls_master_voice, g_cfg.audio.downmix_to_2ch ? 2 : 8, 48000);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : CreateMasteringVoice() failed(0x%08x)", (u32)hr);
		s_tls_xaudio2_instance->Release();
		Emu.Pause();
	}
}

void XAudio2Backend::xa28_destroy()
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

void XAudio2Backend::xa28_play()
{
	AUDIT(s_tls_source_voice != nullptr);

	HRESULT hr = s_tls_source_voice->Start();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : Start() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Backend::xa28_flush()
{
	AUDIT(s_tls_source_voice != nullptr);

	HRESULT hr = s_tls_source_voice->FlushSourceBuffers();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : FlushSourceBuffers() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

void XAudio2Backend::xa28_stop()
{
	AUDIT(s_tls_source_voice != nullptr);

	HRESULT hr = s_tls_source_voice->Stop();
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : Stop() failed(0x%08x)", (u32)hr);
		Emu.Pause();
	}
}

bool XAudio2Backend::xa28_is_playing()
{
	AUDIT(s_tls_source_voice != nullptr);

	XAUDIO2_VOICE_STATE state;
	s_tls_source_voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	return state.BuffersQueued > 0 || state.pCurrentBufferContext != nullptr;
}

void XAudio2Backend::xa28_open()
{
	HRESULT hr;

	const u32 sample_size = get_sample_size();
	const u32 channels = get_channels();
	const u32 sampling_rate = get_sampling_rate();

	WAVEFORMATEX waveformatex;
	waveformatex.wFormatTag = g_cfg.audio.convert_to_u16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = channels;
	waveformatex.nSamplesPerSec = sampling_rate;
	waveformatex.nAvgBytesPerSec = static_cast<DWORD>(sampling_rate * channels * sample_size);
	waveformatex.nBlockAlign = channels * sample_size;
	waveformatex.wBitsPerSample = sample_size * 8;
	waveformatex.cbSize = 0;

	hr = s_tls_xaudio2_instance->CreateSourceVoice(&s_tls_source_voice, &waveformatex, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : CreateSourceVoice() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return;
	}

	AUDIT(s_tls_source_voice != nullptr);
	s_tls_source_voice->SetVolume(channels == 2 ? 1.0 : 4.0);
}

bool XAudio2Backend::xa28_add(const void* src, int size)
{
	AUDIT(s_tls_source_voice != nullptr);

	XAUDIO2_VOICE_STATE state;
	s_tls_source_voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	if (state.BuffersQueued >= MAX_AUDIO_BUFFERS)
	{
		LOG_WARNING(GENERAL, "XAudio2Backend : too many buffers enqueued (%d, pos=%u)", state.BuffersQueued, state.SamplesPlayed);
		return false;
	}

	XAUDIO2_BUFFER buffer;

	buffer.AudioBytes = size * get_sample_size();
	buffer.Flags = 0;
	buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
	buffer.LoopCount = 0;
	buffer.LoopLength = 0;
	buffer.pAudioData = (const BYTE*)src;
	buffer.pContext = 0;
	buffer.PlayBegin = 0;
	buffer.PlayLength = AUDIO_BUFFER_SAMPLES;

	HRESULT hr = s_tls_source_voice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr))
	{
		LOG_ERROR(GENERAL, "XAudio2Backend : AddData() failed(0x%08x)", (u32)hr);
		Emu.Pause();
		return false;
	}

	return true;
}

u64 XAudio2Backend::xa28_enqueued_samples()
{
	XAUDIO2_VOICE_STATE state;
	s_tls_source_voice->GetState(&state);

	// all buffers contain AUDIO_BUFFER_SAMPLES, so we can easily calculate how many samples there are remaining
	return (AUDIO_BUFFER_SAMPLES - state.SamplesPlayed % AUDIO_BUFFER_SAMPLES) + (state.BuffersQueued * AUDIO_BUFFER_SAMPLES);
}

#endif
