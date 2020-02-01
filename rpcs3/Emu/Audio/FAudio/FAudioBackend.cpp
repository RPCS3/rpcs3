#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "FAudioBackend.h"

LOG_CHANNEL(FAudio_, "FAudio");

FAudioBackend::FAudioBackend()
{
	u32 res;

	res = FAudioCreate(&m_instance, 0, FAUDIO_DEFAULT_PROCESSOR);
	if (res)
	{
		FAudio_.fatal("FAudioCreate() failed(0x%08x)", res);
		return;
	}

	res = FAudio_CreateMasteringVoice(m_instance, &m_master_voice, g_cfg.audio.downmix_to_2ch ? 2 : 8, 48000, 0, 0, nullptr);
	if (res)
	{
		FAudio_.fatal("FAudio_CreateMasteringVoice() failed(0x%08x)", res);
		return;
	}
}

FAudioBackend::~FAudioBackend()
{
	if (m_source_voice != nullptr)
	{
		FAudioSourceVoice_Stop(m_source_voice, 0, FAUDIO_COMMIT_NOW);
		FAudioVoice_DestroyVoice(m_source_voice);
	}

	if (m_master_voice != nullptr)
	{
		FAudioVoice_DestroyVoice(m_master_voice);
	}

	if (m_instance != nullptr)
	{
		FAudio_StopEngine(m_instance);
		FAudio_Release(m_instance);
	}
}

void FAudioBackend::Play()
{
	AUDIT(m_source_voice != nullptr);

	u32 res = FAudioSourceVoice_Start(m_source_voice, 0, FAUDIO_COMMIT_NOW);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_Start() failed(0x%08x)", res);
		Emu.Pause();
	}
}

void FAudioBackend::Pause()
{
	AUDIT(m_source_voice != nullptr);

	u32 res = FAudioSourceVoice_Stop(m_source_voice, 0, FAUDIO_COMMIT_NOW);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_Stop() failed(0x%08x)", res);
		Emu.Pause();
	}
}

void FAudioBackend::Flush()
{
	AUDIT(m_source_voice != nullptr);

	u32 res = FAudioSourceVoice_FlushSourceBuffers(m_source_voice);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_FlushSourceBuffers() failed(0x%08x)", res);
		Emu.Pause();
	}
}

bool FAudioBackend::IsPlaying()
{
	AUDIT(m_source_voice != nullptr);

	FAudioVoiceState state;
	FAudioSourceVoice_GetState(m_source_voice, &state, FAUDIO_VOICE_NOSAMPLESPLAYED);

	return state.BuffersQueued > 0 || state.pCurrentBufferContext != nullptr;
}

void FAudioBackend::Close()
{
	Pause();
	Flush();
}

void FAudioBackend::Open(u32 /* num_buffers */)
{
	const u32 sample_size   = AudioBackend::get_sample_size();
	const u32 channels      = AudioBackend::get_channels();
	const u32 sampling_rate = AudioBackend::get_sampling_rate();

	FAudioWaveFormatEx waveformatex;
	waveformatex.wFormatTag      = g_cfg.audio.convert_to_u16 ? FAUDIO_FORMAT_PCM : FAUDIO_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels       = channels;
	waveformatex.nSamplesPerSec  = sampling_rate;
	waveformatex.nAvgBytesPerSec = static_cast<u32>(sampling_rate * channels * sample_size);
	waveformatex.nBlockAlign     = channels * sample_size;
	waveformatex.wBitsPerSample  = sample_size * 8;
	waveformatex.cbSize          = 0;

	u32 res = FAudio_CreateSourceVoice(m_instance, &m_source_voice, &waveformatex, 0, FAUDIO_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
	if (res)
	{
		FAudio_.error("FAudio_CreateSourceVoice() failed(0x%08x)", res);
		Emu.Pause();
	}

	AUDIT(m_source_voice != nullptr);
	FAudioVoice_SetVolume(m_source_voice, channels == 2 ? 1.0f : 4.0f, FAUDIO_COMMIT_NOW);
}

bool FAudioBackend::AddData(const void* src, u32 num_samples)
{
	AUDIT(m_source_voice != nullptr);

	FAudioVoiceState state;
	FAudioSourceVoice_GetState(m_source_voice, &state, FAUDIO_VOICE_NOSAMPLESPLAYED);

	if (state.BuffersQueued >= MAX_AUDIO_BUFFERS)
	{
		FAudio_.warning("Too many buffers enqueued (%d)", state.BuffersQueued);
		return false;
	}

	FAudioBuffer buffer;
	buffer.AudioBytes = num_samples * AudioBackend::get_sample_size();
	buffer.Flags      = 0;
	buffer.LoopBegin  = FAUDIO_NO_LOOP_REGION;
	buffer.LoopCount  = 0;
	buffer.LoopLength = 0;
	buffer.pAudioData = static_cast<const u8*>(src);
	buffer.pContext   = 0;
	buffer.PlayBegin  = 0;
	buffer.PlayLength = AUDIO_BUFFER_SAMPLES;

	u32 res = FAudioSourceVoice_SubmitSourceBuffer(m_source_voice, &buffer, nullptr);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_SubmitSourceBuffer() failed(0x%08x)", res);
		Emu.Pause();
		return false;
	}

	return true;
}

u64 FAudioBackend::GetNumEnqueuedSamples()
{
	FAudioVoiceState state;
	FAudioSourceVoice_GetState(m_source_voice, &state, 0);

	// all buffers contain AUDIO_BUFFER_SAMPLES, so we can easily calculate how many samples there are remaining
	return (AUDIO_BUFFER_SAMPLES - state.SamplesPlayed % AUDIO_BUFFER_SAMPLES) + (state.BuffersQueued * AUDIO_BUFFER_SAMPLES);
}

f32 FAudioBackend::SetFrequencyRatio(f32 new_ratio)
{
	new_ratio = std::clamp(new_ratio, FAUDIO_MIN_FREQ_RATIO, FAUDIO_DEFAULT_FREQ_RATIO);

	u32 res = FAudioSourceVoice_SetFrequencyRatio(m_source_voice, new_ratio, FAUDIO_COMMIT_NOW);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_SetFrequencyRatio() failed(0x%08x)", res);
		Emu.Pause();
		return 1.0f;
	}

	return new_ratio;
}
