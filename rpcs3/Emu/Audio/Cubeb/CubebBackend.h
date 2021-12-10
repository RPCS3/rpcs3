#pragma once

#include <memory>
#include "Utilities/mutex.h"
#include "util/atomic.hpp"
#include "Emu/Audio/AudioBackend.h"

#include "cubeb/cubeb.h"

class CubebBackend final : public AudioBackend
{
public:
	CubebBackend();
	~CubebBackend() override;

	CubebBackend(const CubebBackend&) = delete;
	CubebBackend& operator=(const CubebBackend&) = delete;

	std::string_view GetName() const override { return "Cubeb"sv; }

	static const u32 capabilities = 0;
	u32 GetCapabilities() const override { return capabilities; }

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
	cubeb* m_ctx = nullptr;
	cubeb_stream* m_stream = nullptr;
#ifdef _WIN32
	bool m_com_init_success = false;
#endif

	shared_mutex m_cb_mutex{};
	std::function<u32(u32, void *)> m_write_callback{};
	u8 m_last_sample[sizeof(float) * static_cast<u32>(AudioChannelCnt::SURROUND_7_1)]{};

	bool m_playing = false;
	atomic_t<bool> m_reset_req = false;

	// Cubeb callbacks
	static long data_cb(cubeb_stream* stream, void* user_ptr, void const* input_buffer, void* output_buffer, long nframes);
	static void state_cb(cubeb_stream* stream, void* user_ptr, cubeb_state state);

	void CloseUnlocked();
};
