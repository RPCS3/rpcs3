#include "Emu/Audio/Cubeb/CubebBackend.h"

#include <algorithm>
#include "util/logs.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <system_error>
#endif

LOG_CHANNEL(Cubeb);

CubebBackend::CubebBackend()
	: AudioBackend()
{
#ifdef _WIN32
	// Cubeb requires COM to be initialized on the thread calling cubeb_init on Windows
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		m_com_init_success = true;
	}
#endif

	if (int err = cubeb_init(&m_ctx, "RPCS3", nullptr))
	{
		Cubeb.error("cubeb_init() failed: 0x%08x", err);
		m_ctx = nullptr;
		return;
	}

	Cubeb.notice("Using backend %s", cubeb_get_backend_id(m_ctx));
}

CubebBackend::~CubebBackend()
{
	Close();

	if (m_ctx)
	{
		cubeb_destroy(m_ctx);
	}

#ifdef _WIN32
	if (m_com_init_success)
	{
		CoUninitialize();
	}
#endif
}

bool CubebBackend::Initialized()
{
	return m_ctx != nullptr;
}

bool CubebBackend::Operational()
{
	return m_ctx != nullptr && m_stream != nullptr && !m_reset_req.observe();
}

void CubebBackend::Open(AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt)
{
	if (m_ctx == nullptr) return;

	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();

	m_sampling_rate = freq;
	m_sample_size = sample_size;
	m_channels = ch_cnt;

	cubeb_stream_params stream_param{};
	stream_param.format = get_convert_to_s16() ? CUBEB_SAMPLE_S16NE : CUBEB_SAMPLE_FLOAT32NE;
	stream_param.rate = get_sampling_rate();
	stream_param.channels = get_channels();
	stream_param.layout = [&]()
	{
		switch (ch_cnt)
		{
		case AudioChannelCnt::STEREO:       return CUBEB_LAYOUT_STEREO;
		case AudioChannelCnt::SURROUND_5_1: return CUBEB_LAYOUT_3F2_LFE;
		case AudioChannelCnt::SURROUND_7_1: return CUBEB_LAYOUT_3F4_LFE;
		default:
			ensure(false);
			return CUBEB_LAYOUT_UNDEFINED;
		}
	}();

	u32 min_latency{};
	if (int err = cubeb_get_min_latency(m_ctx, &stream_param, &min_latency))
	{
		Cubeb.error("cubeb_get_min_latency() failed: 0x%08x", err);
	}

	if (int err = cubeb_stream_init(m_ctx, &m_stream, "Main stream", nullptr, nullptr, nullptr, &stream_param, std::max<u32>(AUDIO_MIN_LATENCY, min_latency), data_cb, state_cb, this))
	{
		m_stream = nullptr;
		Cubeb.error("cubeb_stream_init() failed: 0x%08x", err);
	}

	if (m_stream == nullptr)
	{
		Cubeb.error("Failed to open audio backend. Make sure that no other application is running that might block audio access (e.g. Netflix).");
		CloseUnlocked();
		return;
	}

	if (int err = cubeb_stream_set_volume(m_stream, 1.0))
	{
		Cubeb.error("cubeb_stream_set_volume() failed: 0x%08x", err);
	}
}

void CubebBackend::CloseUnlocked()
{
	if (m_stream == nullptr) return;

	if (int err = cubeb_stream_stop(m_stream))
	{
		Cubeb.error("cubeb_stream_stop() failed: 0x%08x", err);
	}

	cubeb_stream_destroy(m_stream);

	m_playing = false;
	m_stream = nullptr;
	memset(m_last_sample, 0, sizeof(m_last_sample));
}

void CubebBackend::Close()
{
	std::lock_guard lock(m_cb_mutex);
	CloseUnlocked();
}

void CubebBackend::Play()
{
	if (m_playing) return;

	if (int err = cubeb_stream_start(m_stream))
	{
		Cubeb.error("cubeb_stream_start() failed: 0x%08x", err);
	}

	std::lock_guard lock(m_cb_mutex);
	m_playing = true;
}

void CubebBackend::Pause()
{
	if (int err = cubeb_stream_stop(m_stream))
	{
		Cubeb.error("cubeb_stream_stop() failed: 0x%08x", err);
	}

	std::lock_guard lock(m_cb_mutex);
	m_playing = false;
}

bool CubebBackend::IsPlaying()
{
	return m_playing;
}

void CubebBackend::SetWriteCallback(std::function<u32(u32, void *)> cb)
{
	std::lock_guard lock(m_cb_mutex);
	m_write_callback = cb;
}

f64 CubebBackend::GetCallbackFrameLen()
{
	cubeb_stream_params stream_param{};
	stream_param.format = get_convert_to_s16() ? CUBEB_SAMPLE_S16NE : CUBEB_SAMPLE_FLOAT32NE;
	stream_param.rate = get_sampling_rate();
	stream_param.channels = get_channels();

	u32 min_latency{};
	if (int err = cubeb_get_min_latency(m_ctx, &stream_param, &min_latency))
	{
		Cubeb.error("cubeb_get_min_latency() failed: 0x%08x", err);
	}

	return static_cast<f64>(std::max<u32>(AUDIO_MIN_LATENCY, min_latency)) / get_sampling_rate();
}

long CubebBackend::data_cb(cubeb_stream* /* stream */, void* user_ptr, void const* /* input_buffer */, void* output_buffer, long nframes)
{
	CubebBackend* const cubeb = static_cast<CubebBackend*>(user_ptr);
	std::unique_lock lock(cubeb->m_cb_mutex, std::defer_lock);

	if (nframes && lock.try_lock() && cubeb->m_write_callback && cubeb->m_playing)
	{
		const u32 sample_size = cubeb->get_sample_size() * cubeb->get_channels();
		const u32 bytes_req = nframes * cubeb->get_sample_size() * cubeb->get_channels();
		u32 written = std::min(cubeb->m_write_callback(bytes_req, output_buffer), bytes_req);
		written -= written % sample_size;

		if (written >= sample_size)
		{
			memcpy(cubeb->m_last_sample, static_cast<u8*>(output_buffer) + written - sample_size, sample_size);
		}

		for (u32 i = written; i < bytes_req; i += sample_size)
		{
			memcpy(static_cast<u8*>(output_buffer) + i, cubeb->m_last_sample, sample_size);
		}
	}

	return nframes;
}

void CubebBackend::state_cb(cubeb_stream* /* stream */, void* user_ptr, cubeb_state state)
{
	CubebBackend* const cubeb = static_cast<CubebBackend*>(user_ptr);

	if (state == CUBEB_STATE_ERROR)
	{
		Cubeb.error("Stream entered error state");
		cubeb->m_reset_req = true;
	}
}
