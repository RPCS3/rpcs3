#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "stdafx.h"
#include "FAudioBackend.h"
#include "Emu/System.h"
#include "Emu/Audio/audio_device_enumerator.h"
#include "Utilities/StrUtil.h"

LOG_CHANNEL(FAudio_, "FAudio");

FAudioBackend::FAudioBackend()
	: AudioBackend()
{
	FAudio *instance;

	if (u32 res = FAudioCreate(&instance, 0, FAUDIO_DEFAULT_PROCESSOR))
	{
		FAudio_.error("FAudioCreate() failed(0x%08x)", res);
		return;
	}

	OnProcessingPassStart = nullptr;
	OnProcessingPassEnd = nullptr;
	OnCriticalError = OnCriticalError_func;

	if (u32 res = FAudio_RegisterForCallbacks(instance, this))
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

	std::lock_guard lock(m_cb_mutex);
	m_playing = true;
}

void FAudioBackend::Pause()
{
	if (m_source_voice == nullptr)
	{
		FAudio_.error("Pause() called uninitialized");
		return;
	}

	if (!m_playing) return;

	{
		std::lock_guard lock(m_cb_mutex);
		m_playing = false;
		m_last_sample.fill(0);
	}

	if (u32 res = FAudioSourceVoice_FlushSourceBuffers(m_source_voice))
	{
		FAudio_.error("FAudioSourceVoice_FlushSourceBuffers() failed(0x%08x)", res);
	}
}

void FAudioBackend::CloseUnlocked()
{
	if (m_source_voice != nullptr)
	{
		if (u32 res = FAudioSourceVoice_Stop(m_source_voice, 0, FAUDIO_COMMIT_NOW))
		{
			FAudio_.error("FAudioSourceVoice_Stop() failed(0x%08x)", res);
		}

		FAudioVoice_DestroyVoice(m_source_voice);
		m_source_voice = nullptr;
	}

	if (m_master_voice)
	{
		FAudioVoice_DestroyVoice(m_master_voice);
		m_master_voice = nullptr;
	}

	m_playing = false;
	m_last_sample.fill(0);
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
	return m_source_voice != nullptr && !m_reset_req.observe();
}

bool FAudioBackend::Open(std::string_view dev_id, AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt, audio_channel_layout layout)
{
	if (!Initialized())
	{
		FAudio_.error("Open() called uninitialized");
		return false;
	}

	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();

	const bool use_default_dev = dev_id.empty() || dev_id == audio_device_enumerator::DEFAULT_DEV_ID;
	u64 devid{};
	if (!use_default_dev)
	{
		if (!try_to_uint64(&devid, dev_id, 0, UINT32_MAX))
		{
			FAudio_.error("Invalid device id - %s", dev_id);
			return false;
		}
	}

	if (u32 res = FAudio_CreateMasteringVoice(m_instance, &m_master_voice, FAUDIO_DEFAULT_CHANNELS, FAUDIO_DEFAULT_SAMPLERATE, 0, static_cast<u32>(devid), nullptr))
	{
		FAudio_.error("FAudio_CreateMasteringVoice() failed(0x%08x)", res);
		m_master_voice = nullptr;
		return false;
	}

	FAudioVoiceDetails vd{};
	FAudioVoice_GetVoiceDetails(m_master_voice, &vd);

	if (vd.InputChannels == 0)
	{
		FAudio_.error("Channel count of 0 is invalid");
		CloseUnlocked();
		return false;
	}

	FAudio_.notice("Channel count is %d", vd.InputChannels);

	m_sampling_rate = freq;
	m_sample_size = sample_size;

	setup_channel_layout(static_cast<u32>(ch_cnt), vd.InputChannels, layout, FAudio_);

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

	if (u32 res = FAudio_CreateSourceVoice(m_instance, &m_source_voice, &waveformatex, 0, FAUDIO_DEFAULT_FREQ_RATIO, this, nullptr, nullptr))
	{
		FAudio_.error("FAudio_CreateSourceVoice() failed(0x%08x)", res);
		CloseUnlocked();
		return false;
	}

	if (u32 res = FAudioSourceVoice_Start(m_source_voice, 0, FAUDIO_COMMIT_NOW))
	{
		FAudio_.error("FAudioSourceVoice_Start() failed(0x%08x)", res);
		CloseUnlocked();
		return false;
	}

	if (u32 res = FAudioVoice_SetVolume(m_source_voice, 1.0f, FAUDIO_COMMIT_NOW))
	{
		FAudio_.error("FAudioVoice_SetVolume() failed(0x%08x)", res);
	}

	m_data_buf.resize(get_sampling_rate() * get_sample_size() * get_channels() * INTERNAL_BUF_SIZE_MS / 1000);

	return true;
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
	if (BytesRequired && !faudio->m_reset_req.observe() && lock.try_lock_for(std::chrono::microseconds{50}) && faudio->m_write_callback && faudio->m_playing)
	{
		ensure(BytesRequired <= faudio->m_data_buf.size(), "FAudio internal buffer is too small. Report to developers!");

		const u32 sample_size = faudio->get_sample_size() * faudio->get_channels();
		u32 written = std::min(faudio->m_write_callback(BytesRequired, faudio->m_data_buf.data()), BytesRequired);
		written -= written % sample_size;

		if (written >= sample_size)
		{
			memcpy(faudio->m_last_sample.data(), faudio->m_data_buf.data() + written - sample_size, sample_size);
		}

		for (u32 i = written; i < BytesRequired; i += sample_size)
		{
			memcpy(faudio->m_data_buf.data() + i, faudio->m_last_sample.data(), sample_size);
		}

		FAudioBuffer buffer{};
		buffer.AudioBytes = BytesRequired;
		buffer.pAudioData = static_cast<const u8*>(faudio->m_data_buf.data());
		// Avoid logging in callback and assume that this always succeeds, all errors are caught by error callback anyway
		FAudioSourceVoice_SubmitSourceBuffer(faudio->m_source_voice, &buffer, nullptr);
	}
}

void FAudioBackend::OnCriticalError_func(FAudioEngineCallback *cb_obj, u32 Error)
{
	FAudio_.error("OnCriticalError() failed(0x%08x)", Error);

	FAudioBackend *faudio = static_cast<FAudioBackend *>(cb_obj);

	std::lock_guard lock(faudio->m_state_cb_mutex);

	if (!faudio->m_reset_req.test_and_set() && faudio->m_state_callback)
	{
		faudio->m_state_callback(AudioStateEvent::UNSPECIFIED_ERROR);
	}
}
