#pragma once

#ifndef _WIN32
#error "XAudio2 can only be built on Windows."
#endif

#include <memory>
#include "Utilities/mutex.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/audio_device_listener.h"

#include <xaudio2redist.h>
#include <wrl/client.h>

class XAudio2Backend final : public AudioBackend, public IXAudio2VoiceCallback, public IXAudio2EngineCallback
{
public:
	XAudio2Backend();
	~XAudio2Backend() override;

	XAudio2Backend(const XAudio2Backend&) = delete;
	XAudio2Backend& operator=(const XAudio2Backend&) = delete;

	std::string_view GetName() const override { return "XAudio2"sv; }

	static const u32 capabilities = SET_FREQUENCY_RATIO;
	u32 GetCapabilities() const override { return capabilities;	}

	bool Initialized() override;
	bool Operational() override;

	void Open(AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt) override;
	void Close() override;

	void SetWriteCallback(std::function<u32(u32, void *)> cb) override;
	f64 GetCallbackFrameLen() override;

	void Play() override;
	void Pause() override;
	bool IsPlaying() override;

	f32 SetFrequencyRatio(f32 new_ratio) override;

private:
	static constexpr u32 INTERNAL_BUF_SIZE_MS = 25;

	Microsoft::WRL::ComPtr<IXAudio2> m_xaudio2_instance{};
	IXAudio2MasteringVoice* m_master_voice{};
	IXAudio2SourceVoice* m_source_voice{};
	bool m_com_init_success = false;

	shared_mutex m_cb_mutex{};
	std::function<u32(u32, void *)> m_write_callback{};
	std::unique_ptr<u8[]> m_data_buf{};
	u64 m_data_buf_len = 0;
	u8 m_last_sample[sizeof(float) * static_cast<u32>(AudioChannelCnt::SURROUND_7_1)]{};

	bool m_playing = false;
	atomic_t<bool> m_reset_req = false;

	audio_device_listener m_dev_listener{};

	// XAudio voice callbacks
	void OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
	void OnVoiceProcessingPassEnd() override {}
	void OnStreamEnd() override {}
	void OnBufferStart(void* /* pBufferContext */) override {}
	void OnBufferEnd(void* /* pBufferContext*/) override {}
	void OnLoopEnd(void* /* pBufferContext */) override {}
	void OnVoiceError(void* /* pBufferContext */, HRESULT /* Error */) override {}

	// XAudio engine callbacks
	void OnProcessingPassStart() override {};
	void OnProcessingPassEnd() override {};
	void OnCriticalError(HRESULT Error) override;

	void CloseUnlocked();
};
