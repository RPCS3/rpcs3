#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "stdafx.h"
#include "FAudioBackend.h"
#include "Emu/system_config.h"
#include "Emu/System.h"

LOG_CHANNEL(FAudio_, "FAudio");

FAudioBackend::FAudioBackend()
	: AudioBackend()
{
	FAudio *instance;

	u32 res = FAudioCreate(&instance, 0, FAUDIO_DEFAULT_PROCESSOR);
	if (res)
	{
		FAudio_.error("FAudioCreate() failed(0x%08x)", res);
		return;
	}

	res = FAudio_CreateMasteringVoice(instance, &m_master_voice, FAUDIO_DEFAULT_CHANNELS, FAUDIO_DEFAULT_SAMPLERATE, 0, 0, nullptr);
	if (res)
	{
		FAudio_.error("FAudio_CreateMasteringVoice() failed(0x%08x)", res);
		FAudio_StopEngine(instance);
		return;
	}

	OnProcessingPassStart = nullptr;
	OnProcessingPassEnd = nullptr;
	OnCriticalError = OnCriticalError_func;

	res = FAudio_RegisterForCallbacks(instance, this);
	if (res)
	{
		// Some error recovery functionality will be lost, but otherwise backend is operational
		FAudio_.error("FAudio_RegisterForCallbacks() failed(0x%08x)", res);
	}

	// All succeeded, "commit"
	m_instance = instance;
}

FAudioBackend::~FAudioBackend()
{
	Close();

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
	if (m_source_voice == nullptr)
	{
		FAudio_.error("Play() called uninitialized");
		return;
	}

	if (m_playing) return;

	const u32 res = FAudioSourceVoice_Start(m_source_voice, 0, FAUDIO_COMMIT_NOW);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_Start() failed(0x%08x)", res);
	}

	std::lock_guard lock(m_cb_mutex);
	m_playing = true;
}

void FAudioBackend::Pause()
{
	if (m_source_voice)
	{
		u32 res = FAudioSourceVoice_Stop(m_source_voice, 0, FAUDIO_COMMIT_NOW);
		if (res)
		{
			FAudio_.error("FAudioSourceVoice_Stop() failed(0x%08x)", res);
		}

		if (res = FAudioSourceVoice_FlushSourceBuffers(m_source_voice))
		{
			FAudio_.error("FAudioSourceVoice_FlushSourceBuffers() failed(0x%08x)", res);
		}
	}
	else
	{
		FAudio_.error("Pause() called uninitialized");
	}

	std::lock_guard lock(m_cb_mutex);
	m_playing = false;
}

void FAudioBackend::CloseUnlocked()
{
	if (m_source_voice == nullptr) return;

	const u32 res = FAudioSourceVoice_Stop(m_source_voice, 0, FAUDIO_COMMIT_NOW);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_Stop() failed(0x%08x)", res);
	}

	FAudioVoice_DestroyVoice(m_source_voice);
	m_playing = false;
	m_source_voice = nullptr;
	m_data_buf = nullptr;
	m_data_buf_len = 0;
	memset(m_last_sample, 0, sizeof(m_last_sample));
}

void FAudioBackend::Close()
{
	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();
}

bool FAudioBackend::Initialized()
{
	return m_instance != nullptr;
}

bool FAudioBackend::Operational()
{
	return m_instance != nullptr && m_source_voice != nullptr && !m_reset_req.observe();
}

void FAudioBackend::Open(AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt)
{
	if (m_instance == nullptr) return;

	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();

	m_sampling_rate = freq;
	m_sample_size = sample_size;
	m_channels = ch_cnt;

	FAudioWaveFormatEx waveformatex;
	waveformatex.wFormatTag = get_convert_to_s16() ? FAUDIO_FORMAT_PCM : FAUDIO_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = get_channels();
	waveformatex.nSamplesPerSec = get_sampling_rate();
	waveformatex.nAvgBytesPerSec = static_cast<u32>(get_sampling_rate() * get_channels() * get_sample_size());
	waveformatex.nBlockAlign = get_channels() * get_sample_size();
	waveformatex.wBitsPerSample = get_sample_size() * 8;
	waveformatex.cbSize = 0;

	OnVoiceProcessingPassStart = OnVoiceProcessingPassStart_func;
	OnVoiceProcessingPassEnd = nullptr;
	OnStreamEnd = nullptr;
	OnBufferStart = nullptr;
	OnBufferEnd = nullptr;
	OnLoopEnd = nullptr;
	OnVoiceError = nullptr;

	const u32 res = FAudio_CreateSourceVoice(m_instance, &m_source_voice, &waveformatex, 0, FAUDIO_DEFAULT_FREQ_RATIO, this, nullptr, nullptr);
	if (res)
	{
		FAudio_.error("FAudio_CreateSourceVoice() failed(0x%08x)", res);
	}

	if (m_source_voice == nullptr)
	{
		FAudio_.fatal("Failed to open audio backend. Make sure that no other application is running that might block audio access (e.g. Netflix).");
		return;
	}

	FAudioVoice_SetVolume(m_source_voice, 1.0f, FAUDIO_COMMIT_NOW);

	m_data_buf_len = get_sampling_rate() * get_sample_size() * get_channels() * INTERNAL_BUF_SIZE_MS * static_cast<u32>(FAUDIO_DEFAULT_FREQ_RATIO) / 1000;
	m_data_buf = std::make_unique<u8[]>(m_data_buf_len);
}

bool FAudioBackend::IsPlaying()
{
	return m_playing;
}

f32 FAudioBackend::SetFrequencyRatio(f32 new_ratio)
{
	new_ratio = std::clamp(new_ratio, FAUDIO_MIN_FREQ_RATIO, FAUDIO_DEFAULT_FREQ_RATIO);

	if (m_source_voice == nullptr)
	{
		FAudio_.error("SetFrequencyRatio() called uninitialized");
		return 1.0f;
	}

	const u32 res = FAudioSourceVoice_SetFrequencyRatio(m_source_voice, new_ratio, FAUDIO_COMMIT_NOW);
	if (res)
	{
		FAudio_.error("FAudioSourceVoice_SetFrequencyRatio() failed(0x%08x)", res);
		return 1.0f;
	}

	return new_ratio;
}

void FAudioBackend::SetWriteCallback(std::function<u32(u32, void *)> cb)
{
	std::lock_guard lock(m_cb_mutex);
	m_write_callback = cb;
}

f64 FAudioBackend::GetCallbackFrameLen()
{
	constexpr f64 _10ms = 0.01;

	if (m_instance == nullptr)
	{
		FAudio_.error("GetCallbackFrameLen() called uninitialized");
		return _10ms;
	}

	f64 min_latency{};

	u32 samples_per_q = 0, freq = 0;
	FAudio_GetProcessingQuantum(m_instance, &samples_per_q, &freq);

	if (freq)
	{
		min_latency = static_cast<f64>(samples_per_q) / freq;
	}

	return std::max<f64>(min_latency, _10ms);
}

void FAudioBackend::OnVoiceProcessingPassStart_func(FAudioVoiceCallback *cb_obj, u32 BytesRequired)
{
	FAudioBackend *faudio = static_cast<FAudioBackend *>(cb_obj);

	std::unique_lock lock(faudio->m_cb_mutex, std::defer_lock);
	if (BytesRequired && lock.try_lock() && faudio->m_write_callback && faudio->m_playing)
	{
		ensure(BytesRequired <= faudio->m_data_buf_len, "FAudio internal buffer is too small. Report to developers!");

		const u32 sample_size = faudio->get_sample_size() * faudio->get_channels();
		u32 written = std::min(faudio->m_write_callback(BytesRequired, faudio->m_data_buf.get()), BytesRequired);
		written -= written % sample_size;

		if (written >= sample_size)
		{
			memcpy(faudio->m_last_sample, faudio->m_data_buf.get() + written - sample_size, sample_size);
		}

		for (u32 i = written; i < BytesRequired; i += sample_size)
		{
			memcpy(faudio->m_data_buf.get() + i, faudio->m_last_sample, sample_size);
		}

		FAudioBuffer buffer{};
		buffer.AudioBytes = BytesRequired;
		buffer.LoopBegin  = FAUDIO_NO_LOOP_REGION;
		buffer.pAudioData = static_cast<const u8*>(faudio->m_data_buf.get());

		const u32 res = FAudioSourceVoice_SubmitSourceBuffer(faudio->m_source_voice, &buffer, nullptr);
		if (res)
		{
			FAudio_.error("FAudioSourceVoice_SubmitSourceBuffer() failed(0x%08x)", res);
		}
	}
}

void FAudioBackend::OnCriticalError_func(FAudioEngineCallback *cb_obj, u32 Error)
{
	FAudio_.error("OnCriticalError() failed(0x%08x)", Error);

	FAudioBackend *faudio = static_cast<FAudioBackend *>(cb_obj);
	faudio->m_reset_req = true;
}
