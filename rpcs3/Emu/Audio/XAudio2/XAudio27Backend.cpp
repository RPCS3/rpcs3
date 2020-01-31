#ifndef _WIN32
#error "XAudio27 can only be built on Windows."
#endif

#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"
#include "Emu/System.h"

#include "XAudio2Backend.h"
#include "3rdparty/XAudio2_7/XAudio2.h"

LOG_CHANNEL(XAudio);

class XAudio27Library : public XAudio2Backend::XAudio2Library
{
	const HMODULE tls_xaudio2_lib;
	IXAudio2* tls_xaudio2_instance{};
	IXAudio2MasteringVoice* tls_master_voice{};
	IXAudio2SourceVoice* tls_source_voice{};

public:
	XAudio27Library(void* lib2_7)
		: tls_xaudio2_lib(static_cast<HMODULE>(lib2_7))
	{
		HRESULT hr = S_OK;

		hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(hr))
		{
			XAudio.error("CoInitializeEx() failed(0x%08x)", (u32)hr);
			Emu.Pause();
			return;
		}

		hr = XAudio2Create(&tls_xaudio2_instance, 0, XAUDIO2_DEFAULT_PROCESSOR);
		if (FAILED(hr))
		{
			XAudio.error("XAudio2Create() failed(0x%08x)", (u32)hr);
			Emu.Pause();
			return;
		}

		hr = tls_xaudio2_instance->CreateMasteringVoice(&tls_master_voice, g_cfg.audio.downmix_to_2ch ? 2 : 8, 48000);
		if (FAILED(hr))
		{
			XAudio.error("CreateMasteringVoice() failed(0x%08x)", (u32)hr);
			tls_xaudio2_instance->Release();
			Emu.Pause();
		}
	}

	~XAudio27Library()
	{
		if (tls_source_voice != nullptr)
		{
			tls_source_voice->Stop();
			tls_source_voice->DestroyVoice();
		}

		if (tls_master_voice != nullptr)
		{
			tls_master_voice->DestroyVoice();
		}

		if (tls_xaudio2_instance != nullptr)
		{
			tls_xaudio2_instance->StopEngine();
			tls_xaudio2_instance->Release();
		}

		CoUninitialize();

		FreeLibrary(tls_xaudio2_lib);
	}

	virtual void play() override
	{
		HRESULT hr = tls_source_voice->Start();
		if (FAILED(hr))
		{
			XAudio.error("Start() failed(0x%08x)", (u32)hr);
			Emu.Pause();
		}
	}

	virtual void flush() override
	{
		HRESULT hr = tls_source_voice->FlushSourceBuffers();
		if (FAILED(hr))
		{
			XAudio.error("FlushSourceBuffers() failed(0x%08x)", (u32)hr);
			Emu.Pause();
		}
	}

	virtual void stop() override
	{
		HRESULT hr = tls_source_voice->Stop();
		if (FAILED(hr))
		{
			XAudio.error("Stop() failed(0x%08x)", (u32)hr);
			Emu.Pause();
		}
	}

	virtual bool is_playing() override
	{
		XAUDIO2_VOICE_STATE state;
		tls_source_voice->GetState(&state);

		return state.BuffersQueued > 0 || state.pCurrentBufferContext != nullptr;
	}

	virtual void open() override
	{
		HRESULT hr;

		const u32 sample_size = AudioBackend::get_sample_size();
		const u32 channels = AudioBackend::get_channels();
		const u32 sampling_rate = AudioBackend::get_sampling_rate();

		WAVEFORMATEX waveformatex;
		waveformatex.wFormatTag = g_cfg.audio.convert_to_u16 ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
		waveformatex.nChannels = channels;
		waveformatex.nSamplesPerSec = sampling_rate;
		waveformatex.nAvgBytesPerSec = static_cast<DWORD>(sampling_rate * channels * sample_size);
		waveformatex.nBlockAlign = channels * sample_size;
		waveformatex.wBitsPerSample = sample_size * 8;
		waveformatex.cbSize = 0;

		hr = tls_xaudio2_instance->CreateSourceVoice(&tls_source_voice, &waveformatex, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
		if (FAILED(hr))
		{
			XAudio.error("CreateSourceVoice() failed(0x%08x)", (u32)hr);
			Emu.Pause();
			return;
		}

		tls_source_voice->SetVolume(channels == 2 ? 1.0f : 4.0f);
	}

	virtual bool add(const void* src, u32 num_samples) override
	{
		XAUDIO2_VOICE_STATE state;
		tls_source_voice->GetState(&state);

		// XAudio 2.7 bug workaround, when it says "SimpList: non-growable list ran out of room for new elements" and hits int 3
		if (state.BuffersQueued >= MAX_AUDIO_BUFFERS)
		{
			XAudio.warning("Too many buffers enqueued (%d, pos=%u)", state.BuffersQueued, state.SamplesPlayed);
			return false;
		}

		XAUDIO2_BUFFER buffer;

		buffer.AudioBytes = num_samples * AudioBackend::get_sample_size();
		buffer.Flags = 0;
		buffer.LoopBegin = XAUDIO2_NO_LOOP_REGION;
		buffer.LoopCount = 0;
		buffer.LoopLength = 0;
		buffer.pAudioData = (const BYTE*)src;
		buffer.pContext = 0;
		buffer.PlayBegin = 0;
		buffer.PlayLength = AUDIO_BUFFER_SAMPLES;

		HRESULT hr = tls_source_voice->SubmitSourceBuffer(&buffer);
		if (FAILED(hr))
		{
			XAudio.error("AddData() failed(0x%08x)", (u32)hr);
			Emu.Pause();
			return false;
		}

		return true;
	}

	virtual u64 enqueued_samples() override
	{
		XAUDIO2_VOICE_STATE state;
		tls_source_voice->GetState(&state);

		// all buffers contain AUDIO_BUFFER_SAMPLES, so we can easily calculate how many samples there are remaining
		return (AUDIO_BUFFER_SAMPLES - state.SamplesPlayed % AUDIO_BUFFER_SAMPLES) + (state.BuffersQueued * AUDIO_BUFFER_SAMPLES);
	}

	virtual f32 set_freq_ratio(f32 new_ratio) override
	{
		new_ratio = std::clamp(new_ratio, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_DEFAULT_FREQ_RATIO);

		HRESULT hr = tls_source_voice->SetFrequencyRatio(new_ratio);
		if (FAILED(hr))
		{
			XAudio.error("SetFrequencyRatio() failed(0x%08x)", (u32)hr);
			Emu.Pause();
			return 1.0f;
		}

		return new_ratio;
	}
};

XAudio2Backend::XAudio2Library* XAudio2Backend::xa27_init(void* lib2_7)
{
	return new XAudio27Library(lib2_7);
}
