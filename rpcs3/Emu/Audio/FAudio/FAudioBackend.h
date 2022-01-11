#pragma once

#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include <memory>
#include "Utilities/mutex.h"
#include "Emu/Audio/AudioBackend.h"

#include "FAudio.h"

class FAudioBackend : public AudioBackend, public FAudioVoiceCallback, public FAudioEngineCallback
{
public:
	FAudioBackend();
	~FAudioBackend() override;

	FAudioBackend(const FAudioBackend&) = delete;
	FAudioBackend& operator=(const FAudioBackend&) = delete;

	std::string_view GetName() const override { return "FAudio"sv; }

	bool Initialized() override;
	bool Operational() override;

	void Open(AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt) override;
	void Close() override;

	void SetWriteCallback(std::function<u32(u32, void *)> cb) override;
	f64 GetCallbackFrameLen() override;

	void Play() override;
	void Pause() override;
	bool IsPlaying() override;

private:
	static constexpr u32 INTERNAL_BUF_SIZE_MS = 25;

	FAudio* m_instance{};
	FAudioMasteringVoice* m_master_voice{};
	FAudioSourceVoice* m_source_voice{};

	shared_mutex m_cb_mutex{};
	std::function<u32(u32, void *)> m_write_callback{};
	std::unique_ptr<u8[]> m_data_buf{};
	u64 m_data_buf_len = 0;
	std::array<u8, sizeof(float) * static_cast<u32>(AudioChannelCnt::SURROUND_7_1)> m_last_sample{};

	bool m_playing = false;
	atomic_t<bool> m_reset_req = false;

	// FAudio voice callbacks
	static void OnVoiceProcessingPassStart_func(FAudioVoiceCallback *cb_obj, u32 BytesRequired);

	// FAudio engine callbacks
	static void OnCriticalError_func(FAudioEngineCallback *cb_obj, u32 Error);

	void CloseUnlocked();
};
