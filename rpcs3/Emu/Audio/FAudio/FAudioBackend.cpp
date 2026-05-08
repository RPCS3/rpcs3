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
	FAudio *instance = nullptr;

	const u32 res = FAudioCreate(&instance, 0, FAUDIO_DEFAULT_PROCESSOR);
	if (res != 0)
	{
		FAudio_.error("FAudioCreate() failed with error code 0x%08x", res);
		return;
	}

	if (!instance)
	{
		FAudio_.error("FAudioCreate() succeeded but returned null instance");
		return;
	}

	// Initialize callbacks
	OnProcessingPassStart = nullptr;
	OnProcessingPassEnd = nullptr;
	OnCriticalError = OnCriticalError_func;

	const u32 callback_res = FAudio_RegisterForCallbacks(instance, this);
	if (callback_res != 0)
	{
		// Some error recovery functionality will be lost, but otherwise backend is operational
		FAudio_.warning("FAudio_RegisterForCallbacks() failed with error code 0x%08x - error recovery may be limited", callback_res);
	}

	// All succeeded, commit the instance
	m_instance = instance;
	FAudio_.success("FAudio backend initialized successfully");
}

FAudioBackend::~FAudioBackend()
{
	Close();

	if (m_instance)
	{
		// Unregister callbacks before stopping engine
		FAudio_UnregisterForCallbacks(m_instance, this);
		
		FAudio_StopEngine(m_instance);
		FAudio_Release(m_instance);
		m_instance = nullptr;
	}
	
	FAudio_.notice("FAudio backend destroyed successfully");
}

void FAudioBackend::Play()
{
	if (!m_source_voice)
	{
		FAudio_.error("Play() called with uninitialized source voice");
		return;
	}

	std::lock_guard lock(m_cb_mutex);
	if (!m_playing)
	{
		m_playing = true;
		FAudio_.trace("Audio playback started");
	}
}

void FAudioBackend::Pause()
{
	if (!m_source_voice)
	{
		FAudio_.error("Pause() called with uninitialized source voice");
		return;
	}

	bool was_playing = false;
	{
		std::lock_guard lock(m_cb_mutex);
		was_playing = m_playing;
		if (m_playing)
		{
			m_playing = false;
			m_last_sample.fill(0);
		}
	}

	if (was_playing)
	{
		const u32 res = FAudioSourceVoice_FlushSourceBuffers(m_source_voice);
		if (res != 0)
		{
			FAudio_.error("FAudioSourceVoice_FlushSourceBuffers() failed with error code 0x%08x", res);
		}
		else
		{
			FAudio_.trace("Audio playback paused and buffers flushed");
		}
	}
}

void FAudioBackend::CloseUnlocked()
{
	if (m_source_voice)
	{
		const u32 stop_res = FAudioSourceVoice_Stop(m_source_voice, 0, FAUDIO_COMMIT_NOW);
		if (stop_res != 0)
		{
			FAudio_.error("FAudioSourceVoice_Stop() failed with error code 0x%08x", stop_res);
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
	m_data_buf.clear();
	m_data_buf.shrink_to_fit(); // Free memory
	
	FAudio_.trace("FAudio voices and resources cleaned up");
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
	return m_source_voice != nullptr && m_instance != nullptr && !m_reset_req.observe();
}

bool FAudioBackend::Open(std::string_view dev_id, AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt, audio_channel_layout layout)
{
	if (!Initialized())
	{
		FAudio_.error("Open() called on uninitialized backend");
		return false;
	}

	FAudio_.notice("Opening FAudio device: dev_id='%s', freq=%u, sample_size=%u, channels=%u", 
		dev_id.empty() ? "default" : std::string(dev_id).c_str(), 
		static_cast<u32>(freq), static_cast<u32>(sample_size), static_cast<u32>(ch_cnt));

	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();

	const bool use_default_dev = dev_id.empty() || dev_id == audio_device_enumerator::DEFAULT_DEV_ID;
	u64 devid = 0;
	
	if (!use_default_dev)
	{
		if (!try_to_uint64(&devid, dev_id, 0, UINT32_MAX))
		{
			FAudio_.error("Invalid device ID format: '%s'", dev_id);
			return false;
		}
	}

	const u32 master_res = FAudio_CreateMasteringVoice(m_instance, &m_master_voice, 
		FAUDIO_DEFAULT_CHANNELS, FAUDIO_DEFAULT_SAMPLERATE, 0, static_cast<u32>(devid), nullptr);
	if (master_res != 0)
	{
		FAudio_.error("FAudio_CreateMasteringVoice() failed with error code 0x%08x", master_res);
		m_master_voice = nullptr;
		return false;
	}

	FAudioVoiceDetails vd{};
	FAudioVoice_GetVoiceDetails(m_master_voice, &vd);

	if (vd.InputChannels == 0)
	{
		FAudio_.error("Master voice reports invalid channel count of 0");
		CloseUnlocked();
		return false;
	}

	FAudio_.notice("Master voice created with %u input channels", vd.InputChannels);

	// Store audio parameters
	m_sampling_rate = freq;
	m_sample_size = sample_size;

	// Setup channel layout with validation
	setup_channel_layout(static_cast<u32>(ch_cnt), vd.InputChannels, layout, FAudio_);

	// Configure wave format
	FAudioWaveFormatEx waveformatex{};
	waveformatex.wFormatTag = get_convert_to_s16() ? FAUDIO_FORMAT_PCM : FAUDIO_FORMAT_IEEE_FLOAT;
	waveformatex.nChannels = get_channels();
	waveformatex.nSamplesPerSec = get_sampling_rate();
	waveformatex.nAvgBytesPerSec = static_cast<u32>(get_sampling_rate() * get_channels() * get_sample_size());
	waveformatex.nBlockAlign = get_channels() * get_sample_size();
	waveformatex.wBitsPerSample = get_sample_size() * 8;
	waveformatex.cbSize = 0;

	FAudio_.trace("Wave format: %s, %u channels, %u Hz, %u bits per sample", 
		get_convert_to_s16() ? "PCM" : "IEEE_FLOAT", 
		waveformatex.nChannels, waveformatex.nSamplesPerSec, waveformatex.wBitsPerSample);

	// Initialize voice callbacks
	OnVoiceProcessingPassStart = OnVoiceProcessingPassStart_func;
	OnVoiceProcessingPassEnd = nullptr;
	OnStreamEnd = nullptr;
	OnBufferStart = nullptr;
	OnBufferEnd = nullptr;
	OnLoopEnd = nullptr;
	OnVoiceError = nullptr;

	const u32 source_res = FAudio_CreateSourceVoice(m_instance, &m_source_voice, &waveformatex, 
		0, FAUDIO_DEFAULT_FREQ_RATIO, this, nullptr, nullptr);
	if (source_res != 0)
	{
		FAudio_.error("FAudio_CreateSourceVoice() failed with error code 0x%08x", source_res);
		CloseUnlocked();
		return false;
	}

	const u32 start_res = FAudioSourceVoice_Start(m_source_voice, 0, FAUDIO_COMMIT_NOW);
	if (start_res != 0)
	{
		FAudio_.error("FAudioSourceVoice_Start() failed with error code 0x%08x", start_res);
		CloseUnlocked();
		return false;
	}

	const u32 volume_res = FAudioVoice_SetVolume(m_source_voice, 1.0f, FAUDIO_COMMIT_NOW);
	if (volume_res != 0)
	{
		FAudio_.warning("FAudioVoice_SetVolume() failed with error code 0x%08x", volume_res);
	}

	// Pre-allocate audio buffer with some safety margin
	const usz buffer_size = static_cast<usz>(get_sampling_rate()) * get_sample_size() * get_channels() * INTERNAL_BUF_SIZE_MS / 1000;
	m_data_buf.reserve(buffer_size + 1024); // Extra safety margin
	m_data_buf.resize(buffer_size);

	FAudio_.success("FAudio device opened successfully - %u Hz, %u channels, %u-bit %s", 
		get_sampling_rate(), get_channels(), get_sample_size() * 8, 
		get_convert_to_s16() ? "PCM" : "float");

	return true;
}

f64 FAudioBackend::GetCallbackFrameLen()
{
	constexpr f64 fallback_10ms = 0.01;

	if (!m_instance)
	{
		FAudio_.error("GetCallbackFrameLen() called on uninitialized instance");
		return fallback_10ms;
	}

	u32 samples_per_quantum = 0;
	u32 frequency = 0;
	
	FAudio_GetProcessingQuantum(m_instance, &samples_per_quantum, &frequency);

	if (frequency == 0 || samples_per_quantum == 0)
	{
		FAudio_.warning("Invalid processing quantum values: samples=%u, freq=%u", samples_per_quantum, frequency);
		return fallback_10ms;
	}

	const f64 calculated_latency = static_cast<f64>(samples_per_quantum) / frequency;
	const f64 final_latency = std::max(calculated_latency, fallback_10ms);
	
	FAudio_.trace("Calculated frame length: %.3f ms (quantum: %u samples @ %u Hz)", 
		final_latency * 1000.0, samples_per_quantum, frequency);

	return final_latency;
}

void FAudioBackend::OnVoiceProcessingPassStart_func(FAudioVoiceCallback *cb_obj, u32 BytesRequired)
{
	if (!cb_obj || BytesRequired == 0)
	{
		return; // Nothing to do
	}

	FAudioBackend *faudio = static_cast<FAudioBackend *>(cb_obj);
	
	// Validate backend state
	if (!faudio->m_source_voice || faudio->m_reset_req.observe())
	{
		return; // Backend is in invalid state
	}

	// Use a shorter timeout to avoid blocking the audio thread
	std::unique_lock lock(faudio->m_cb_mutex, std::defer_lock);
	if (!lock.try_lock_for(std::chrono::microseconds{100}))
	{
		return; // Could not acquire lock in reasonable time
	}

	// Double-check state after acquiring lock
	if (!faudio->m_write_callback || !faudio->m_playing)
	{
		return; // No callback or not playing
	}

	// Validate buffer size
	if (BytesRequired > faudio->m_data_buf.size())
	{
		// This should never happen with proper initialization, but let's be safe
		FAudio_.error("FAudio internal buffer is too small (%u requested, %zu available). Report to developers!", 
			BytesRequired, faudio->m_data_buf.size());
		return;
	}

	const u32 sample_size = faudio->get_sample_size() * faudio->get_channels();
	if (sample_size == 0)
	{
		return; // Invalid configuration
	}

	// Get audio data from callback
	u32 written = 0;
	if (faudio->m_write_callback)
	{
		written = faudio->m_write_callback(BytesRequired, faudio->m_data_buf.data());
		written = std::min(written, BytesRequired);
		written -= written % sample_size; // Align to sample boundary
	}

	// Update last sample for padding if we got any data
	if (written >= sample_size)
	{
		const usz last_sample_offset = written - sample_size;
		if (last_sample_offset + sample_size <= faudio->m_data_buf.size())
		{
			std::memcpy(faudio->m_last_sample.data(), 
				faudio->m_data_buf.data() + last_sample_offset, 
				std::min(sample_size, static_cast<u32>(faudio->m_last_sample.size())));
		}
	}

	// Pad remaining buffer with last sample to avoid audio glitches
	for (u32 i = written; i < BytesRequired; i += sample_size)
	{
		const u32 copy_size = std::min(sample_size, BytesRequired - i);
		if (i + copy_size <= faudio->m_data_buf.size())
		{
			std::memcpy(faudio->m_data_buf.data() + i, faudio->m_last_sample.data(), copy_size);
		}
	}

	// Submit buffer to FAudio
	FAudioBuffer buffer{};
	buffer.AudioBytes = BytesRequired;
	buffer.pAudioData = static_cast<const u8*>(faudio->m_data_buf.data());
	buffer.Flags = 0;
	buffer.PlayBegin = 0;
	buffer.PlayLength = 0;
	buffer.LoopBegin = 0;
	buffer.LoopLength = 0;
	buffer.LoopCount = 0;
	buffer.pContext = nullptr;

	// Submit buffer - errors are handled by the error callback
	FAudioSourceVoice_SubmitSourceBuffer(faudio->m_source_voice, &buffer, nullptr);
}

void FAudioBackend::OnCriticalError_func(FAudioEngineCallback *cb_obj, u32 Error)
{
	// Log the critical error
	FAudio_.error("OnCriticalError() - FAudio engine encountered critical error 0x%08x", Error);

	if (!cb_obj)
	{
		FAudio_.error("OnCriticalError() called with null callback object");
		return;
	}

	FAudioBackend *faudio = static_cast<FAudioBackend *>(cb_obj);
	
	// Use try_lock to avoid potential deadlock in error conditions
	std::unique_lock lock(faudio->m_state_cb_mutex, std::try_to_lock);
	if (!lock.owns_lock())
	{
		FAudio_.warning("Could not acquire state callback mutex during critical error handling");
		return;
	}

	// Set reset request flag and notify application if we haven't already
	if (!faudio->m_reset_req.test_and_set() && faudio->m_state_callback)
	{
		faudio->m_state_callback(AudioStateEvent::UNSPECIFIED_ERROR);
	}
}
