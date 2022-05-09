#include "Emu/Audio/Cubeb/CubebBackend.h"

#include <algorithm>
#include "util/logs.hpp"
#include "Emu/Audio/audio_device_enumerator.h"

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
	if (HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); SUCCEEDED(hr))
	{
		m_com_init_success = true;
	}
#endif

	if (int err = cubeb_init(&m_ctx, "RPCS3", nullptr))
	{
		Cubeb.error("cubeb_init() failed: %i", err);
		m_ctx = nullptr;
		return;
	}

	if (int err = cubeb_register_device_collection_changed(m_ctx, CUBEB_DEVICE_TYPE_OUTPUT, device_collection_changed_cb, this))
	{
		Cubeb.error("cubeb_register_device_collection_changed() failed: %i", err);
	}
	else
	{
		m_dev_collection_cb_enabled = true;
	}

	Cubeb.notice("Using backend %s", cubeb_get_backend_id(m_ctx));
}

CubebBackend::~CubebBackend()
{
	Close();

	if (m_dev_collection_cb_enabled)
	{
		if (int err = cubeb_register_device_collection_changed(m_ctx, CUBEB_DEVICE_TYPE_OUTPUT, nullptr, this))
		{
			Cubeb.error("cubeb_register_device_collection_changed() failed: %i", err);
		}
	}

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
	return m_stream != nullptr && !m_reset_req.observe();
}

bool CubebBackend::DefaultDeviceChanged()
{
	std::lock_guard lock{m_dev_sw_mutex};
	return !m_reset_req.observe() && m_default_dev_changed;
}

bool CubebBackend::Open(std::string_view dev_id, AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt)
{
	if (!Initialized())
	{
		Cubeb.error("Open() called uninitialized");
		return false;
	}

	std::lock_guard lock(m_cb_mutex);
	std::lock_guard dev_sw_lock{m_dev_sw_mutex};
	CloseUnlocked();

	const bool use_default_device = dev_id.empty() || dev_id == audio_device_enumerator::DEFAULT_DEV_ID;
	auto [dev_handle, dev_ident, dev_ch_cnt] = GetDevice(use_default_device ? "" : dev_id);

	if (!dev_handle)
	{
		if (use_default_device)
		{
			std::tie(dev_handle, dev_ident, dev_ch_cnt) = GetDefaultDeviceAlt(freq, sample_size, ch_cnt);

			if (!dev_handle)
			{
				Cubeb.error("Cannot detect default device. Channel count detection unavailable.");
			}
		}
		else
		{
			Cubeb.error("Device with id=%s not found", dev_id);
			return false;
		}
	}

	if (dev_ch_cnt == 0)
	{
		Cubeb.error("Device reported invalid channel count, using stereo instead");
		dev_ch_cnt = 2;
	}

	m_sampling_rate = freq;
	m_sample_size = sample_size;
	m_channels = static_cast<AudioChannelCnt>(std::min(static_cast<u32>(convert_channel_count(dev_ch_cnt)), static_cast<u32>(ch_cnt)));
	full_sample_size = get_channels() * get_sample_size();

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
			fmt::throw_exception("Invalid audio channel count");
		}
	}();
	stream_param.prefs = m_dev_collection_cb_enabled && dev_handle ? CUBEB_STREAM_PREF_DISABLE_DEVICE_SWITCHING : CUBEB_STREAM_PREF_NONE;

	u32 min_latency{};
	if (int err = cubeb_get_min_latency(m_ctx, &stream_param, &min_latency))
	{
		Cubeb.error("cubeb_get_min_latency() failed: %i", err);
		min_latency = 0;
	}

	const u32 stream_latency = std::max(static_cast<u32>(AUDIO_MIN_LATENCY * get_sampling_rate()), min_latency);

	if (int err = cubeb_stream_init(m_ctx, &m_stream, "Main stream", nullptr, nullptr, dev_handle, &stream_param, stream_latency, data_cb, state_cb, this))
	{
		Cubeb.error("cubeb_stream_init() failed: %i", err);
		m_stream = nullptr;
		return false;
	}

	if (int err = cubeb_stream_start(m_stream))
	{
		Cubeb.error("cubeb_stream_start() failed: %i", err);
		CloseUnlocked();
		return false;
	}

	if (int err = cubeb_stream_set_volume(m_stream, 1.0))
	{
		Cubeb.error("cubeb_stream_set_volume() failed: %i", err);
	}

	if (use_default_device)
	{
		m_current_device = dev_ident;
	}

	return true;
}

void CubebBackend::CloseUnlocked()
{
	if (m_stream != nullptr)
	{
		if (int err = cubeb_stream_stop(m_stream))
		{
			Cubeb.error("cubeb_stream_stop() failed: %i", err);
		}

		cubeb_stream_destroy(m_stream);
		m_stream = nullptr;
	}

	m_playing = false;
	m_last_sample.fill(0);

	m_default_dev_changed = false;
	m_current_device = "";
}

void CubebBackend::Close()
{
	std::lock_guard lock(m_cb_mutex);
	std::lock_guard dev_sw_lock{m_dev_sw_mutex};
	CloseUnlocked();
}

void CubebBackend::Play()
{
	if (m_stream == nullptr)
	{
		Cubeb.error("Play() called uninitialized");
		return;
	}

	if (m_playing) return;

	std::lock_guard lock(m_cb_mutex);
	m_playing = true;
}

void CubebBackend::Pause()
{
	if (m_stream == nullptr)
	{
		Cubeb.error("Pause() called uninitialized");
		return;
	}

	if (!m_playing) return;

	std::lock_guard lock(m_cb_mutex);
	m_playing = false;
	m_last_sample.fill(0);
}

f64 CubebBackend::GetCallbackFrameLen()
{
	if (m_stream == nullptr)
	{
		Cubeb.error("GetCallbackFrameLen() called uninitialized");
		return AUDIO_MIN_LATENCY;
	}

	u32 stream_latency{};
	if (int err = cubeb_stream_get_latency(m_stream, &stream_latency))
	{
		Cubeb.error("cubeb_stream_get_latency() failed: %i", err);
		stream_latency = 0;
	}

	return std::max<f64>(AUDIO_MIN_LATENCY, static_cast<f64>(stream_latency) / get_sampling_rate());
}

std::tuple<cubeb_devid, std::string, u32> CubebBackend::GetDevice(std::string_view dev_id)
{
	const bool default_dev = dev_id == "";

	cubeb_device_collection dev_collection{};
	if (int err = cubeb_enumerate_devices(m_ctx, CUBEB_DEVICE_TYPE_OUTPUT, &dev_collection))
	{
		Cubeb.error("cubeb_enumerate_devices() failed: %i", err);
		return {};
	}

	if (dev_collection.count == 0)
	{
		Cubeb.error("No output devices available");
		if (int err = cubeb_device_collection_destroy(m_ctx, &dev_collection))
		{
			Cubeb.error("cubeb_device_collection_destroy() failed: %i", err);
		}
		return {};
	}

	std::tuple<cubeb_devid, std::string, u32> result{};

	for (u64 dev_idx = 0; dev_idx < dev_collection.count; dev_idx++)
	{
		const cubeb_device_info& dev_info = dev_collection.device[dev_idx];
		const std::string dev_ident{dev_info.device_id};

		if (dev_ident.empty())
		{
			Cubeb.error("device_id is missing from device");
			continue;
		}

		if (default_dev)
		{
			if (dev_info.preferred & CUBEB_DEVICE_PREF_MULTIMEDIA)
			{
				result = {dev_info.devid, dev_ident, dev_info.max_channels};
				break;
			}
		}
		else if (dev_ident == dev_id)
		{
			result = {dev_info.devid, dev_ident, dev_info.max_channels};
			break;
		}
	}

	if (int err = cubeb_device_collection_destroy(m_ctx, &dev_collection))
	{
		Cubeb.error("cubeb_device_collection_destroy() failed: %i", err);
	}

	return result;
};

std::tuple<cubeb_devid, std::string, u32> CubebBackend::GetDefaultDeviceAlt(AudioFreq freq, AudioSampleSize sample_size, AudioChannelCnt ch_cnt)
{
	cubeb_stream_params param =
	{
		.format   = sample_size == AudioSampleSize::S16 ? CUBEB_SAMPLE_S16NE : CUBEB_SAMPLE_FLOAT32NE,
		.rate     = static_cast<u32>(freq),
		.channels = static_cast<u32>(ch_cnt),
		.layout   = CUBEB_LAYOUT_UNDEFINED,
		.prefs    = CUBEB_STREAM_PREF_DISABLE_DEVICE_SWITCHING
	};

	u32 min_latency{};
	if (int err = cubeb_get_min_latency(m_ctx, &param, &min_latency))
	{
		Cubeb.error("cubeb_get_min_latency() failed: %i", err);
		min_latency = 100;
	}

	cubeb_stream* tmp_stream{};
	static auto dummy_data_cb = [](cubeb_stream*, void*, void const*, void*, long) -> long { return 0; };
	static auto dummy_state_cb = [](cubeb_stream*, void*, cubeb_state) {};

	if (int err = cubeb_stream_init(m_ctx, &tmp_stream, "Default device detector", nullptr, nullptr, nullptr, &param, min_latency, dummy_data_cb, dummy_state_cb, nullptr))
	{
		Cubeb.error("cubeb_stream_init() failed: %i", err);
		return {};
	}

	cubeb_device* crnt_dev{};

	if (int err = cubeb_stream_get_current_device(tmp_stream, &crnt_dev))
	{
		Cubeb.error("cubeb_stream_get_current_device() failed: %i", err);
		cubeb_stream_destroy(tmp_stream);
		return {};
	}

	const std::string out_dev_name{crnt_dev->output_name};

	if (int err = cubeb_stream_device_destroy(tmp_stream, crnt_dev))
	{
		Cubeb.error("cubeb_stream_device_destroy() failed: %i", err);
	}

	cubeb_stream_destroy(tmp_stream);

	if (out_dev_name.empty())
	{
		return {};
	}

	return GetDevice(out_dev_name);
}

long CubebBackend::data_cb(cubeb_stream* /* stream */, void* user_ptr, void const* /* input_buffer */, void* output_buffer, long nframes)
{
	CubebBackend* const cubeb = static_cast<CubebBackend*>(user_ptr);
	std::unique_lock lock(cubeb->m_cb_mutex, std::defer_lock);

	if (nframes && !cubeb->m_reset_req.observe() && lock.try_lock() && cubeb->m_write_callback && cubeb->m_playing)
	{
		const u32 sample_size = cubeb->full_sample_size.observe();
		const u32 bytes_req = nframes * sample_size;
		u32 written = std::min(cubeb->m_write_callback(bytes_req, output_buffer), bytes_req);
		written -= written % sample_size;

		if (written >= sample_size)
		{
			memcpy(cubeb->m_last_sample.data(), static_cast<u8*>(output_buffer) + written - sample_size, sample_size);
		}

		for (u32 i = written; i < bytes_req; i += sample_size)
		{
			memcpy(static_cast<u8*>(output_buffer) + i, cubeb->m_last_sample.data(), sample_size);
		}
	}
	else
	{
		// Stream parameters are modified only after stream_destroy. stream_destroy will return
		// only after this callback returns, so it's safe to access full_sample_size here.
		memset(output_buffer, 0, nframes * cubeb->full_sample_size.observe());
	}

	return nframes;
}

void CubebBackend::state_cb(cubeb_stream* /* stream */, void* user_ptr, cubeb_state state)
{
	CubebBackend* const cubeb = static_cast<CubebBackend*>(user_ptr);

	if (state == CUBEB_STATE_ERROR)
	{
		Cubeb.error("Stream entered error state");

		std::lock_guard lock(cubeb->m_state_cb_mutex);

		if (!cubeb->m_reset_req.test_and_set() && cubeb->m_state_callback)
		{
			cubeb->m_state_callback(AudioStateEvent::UNSPECIFIED_ERROR);
		}
	}
}

void CubebBackend::device_collection_changed_cb(cubeb* /* context */, void* user_ptr)
{
	CubebBackend* const cubeb = static_cast<CubebBackend*>(user_ptr);

	Cubeb.notice("Device collection changed");
	std::lock_guard lock{cubeb->m_dev_sw_mutex};

	// Non default device is used (or default device cannot be detected)
	if (cubeb->m_current_device == "")
	{
		return;
	}

	auto [handle, dev_id, ch_cnt] = cubeb->GetDevice();
	if (!handle)
	{
		std::tie(handle, dev_id, ch_cnt) = cubeb->GetDefaultDeviceAlt(cubeb->m_sampling_rate, cubeb->m_sample_size, cubeb->m_channels);
	}

	std::lock_guard cb_lock{cubeb->m_state_cb_mutex};

	if (!handle)
	{
		// No devices available
		if (!cubeb->m_reset_req.test_and_set() && cubeb->m_state_callback)
		{
			cubeb->m_state_callback(AudioStateEvent::UNSPECIFIED_ERROR);
		}
	}
	else if (!cubeb->m_reset_req.observe() && dev_id != cubeb->m_current_device)
	{
		cubeb->m_default_dev_changed = true;

		if (cubeb->m_state_callback)
		{
			cubeb->m_state_callback(AudioStateEvent::DEFAULT_DEVICE_CHANGED);
		}
	}
}
