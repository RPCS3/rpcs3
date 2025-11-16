#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Utilities/StrUtil.h"

#include "cellMic.h"
#include <Emu/IdManager.h>
#include <Emu/Cell/lv2/sys_event.h>

#include <numeric>
#include <cmath>

#ifndef WITHOUT_OPENAL
#include "alext.h"
#endif

LOG_CHANNEL(cellMic);

template <>
void fmt_class_string<CellMicInError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MICIN_ERROR_ALREADY_INIT);
			STR_CASE(CELL_MICIN_ERROR_DEVICE);
			STR_CASE(CELL_MICIN_ERROR_NOT_INIT);
			STR_CASE(CELL_MICIN_ERROR_PARAM);
			STR_CASE(CELL_MICIN_ERROR_PORT_FULL);
			STR_CASE(CELL_MICIN_ERROR_ALREADY_OPEN);
			STR_CASE(CELL_MICIN_ERROR_NOT_OPEN);
			STR_CASE(CELL_MICIN_ERROR_NOT_RUN);
			STR_CASE(CELL_MICIN_ERROR_TRANS_EVENT);
			STR_CASE(CELL_MICIN_ERROR_OPEN);
			STR_CASE(CELL_MICIN_ERROR_SHAREDMEMORY);
			STR_CASE(CELL_MICIN_ERROR_MUTEX);
			STR_CASE(CELL_MICIN_ERROR_EVENT_QUEUE);
			STR_CASE(CELL_MICIN_ERROR_DEVICE_NOT_FOUND);
			STR_CASE(CELL_MICIN_ERROR_FATAL);
			STR_CASE(CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellMicInErrorDsp>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MICIN_ERROR_DSP);
			STR_CASE(CELL_MICIN_ERROR_DSP_ASSERT);
			STR_CASE(CELL_MICIN_ERROR_DSP_PATH);
			STR_CASE(CELL_MICIN_ERROR_DSP_FILE);
			STR_CASE(CELL_MICIN_ERROR_DSP_PARAM);
			STR_CASE(CELL_MICIN_ERROR_DSP_MEMALLOC);
			STR_CASE(CELL_MICIN_ERROR_DSP_POINTER);
			STR_CASE(CELL_MICIN_ERROR_DSP_FUNC);
			STR_CASE(CELL_MICIN_ERROR_DSP_MEM);
			STR_CASE(CELL_MICIN_ERROR_DSP_ALIGN16);
			STR_CASE(CELL_MICIN_ERROR_DSP_ALIGN128);
			STR_CASE(CELL_MICIN_ERROR_DSP_EAALIGN128);
			STR_CASE(CELL_MICIN_ERROR_DSP_LIB_HANDLER);
			STR_CASE(CELL_MICIN_ERROR_DSP_LIB_INPARAM);
			STR_CASE(CELL_MICIN_ERROR_DSP_LIB_NOSPU);
			STR_CASE(CELL_MICIN_ERROR_DSP_LIB_SAMPRATE);
		}

		return unknown;
	});
}

namespace fmt
{
	struct alc_error
	{
		ALCdevice* device{};
		ALCenum error{};
	};
}

template <>
void fmt_class_string<fmt::alc_error>::format(std::string& out, u64 arg)
{
	const fmt::alc_error& obj = get_object(arg);
	fmt::append(out, "0x%x='%s'", obj.error, alcGetString(obj.device, obj.error));
}

void mic_context::operator()()
{
	// Timestep in microseconds
	constexpr u64 TIMESTEP = 256ull * 1'000'000ull / 48000ull;
	u64 timeout = 0;

	while (thread_ctrl::state() != thread_state::aborting)
	{
		if (timeout != 0)
		{
			thread_ctrl::wait_on(wakey, 0, timeout);
			wakey.store(0);
		}

		std::lock_guard lock(mutex);

		if (std::none_of(mic_list.begin(), mic_list.end(), [](const microphone_device& dev) { return dev.is_registered(); }))
		{
			timeout = umax;
			continue;
		}
		else
		{
			timeout = TIMESTEP - (std::chrono::duration_cast<std::chrono::microseconds>(steady_clock::now().time_since_epoch()).count() % TIMESTEP);
		}

		for (auto& mic_entry : mic_list)
		{
			mic_entry.update_audio();
		}

		auto mic_queue = lv2_event_queue::find(event_queue_key);
		if (!mic_queue)
			continue;

		for (usz dev_num = 0; dev_num < mic_list.size(); dev_num++)
		{
			microphone_device& device = ::at32(mic_list, dev_num);
			if (device.has_data())
			{
				mic_queue->send(event_queue_source, CELLMIC_DATA, dev_num, 0);
			}
		}
	}

	// Cleanup
	std::lock_guard lock(mutex);
	for (auto& mic_entry : mic_list)
	{
		mic_entry.close_microphone();
	}
}

void mic_context::wake_up()
{
	wakey.store(1);
	wakey.notify_one();
}

void mic_context::load_config_and_init()
{
	mic_list = {};

	const std::vector<std::string> device_list = fmt::split(g_cfg.audio.microphone_devices.to_string(), {"@@@"});

	if (!device_list.empty())
	{
		// We only register the first device. The rest is registered with cellAudioInRegisterDevice.
		if (g_cfg.audio.microphone_type == microphone_handler::singstar)
		{
			microphone_device& device = ::at32(mic_list, 0);
			device = microphone_device(microphone_handler::singstar);
			device.set_registered(true);
			device.add_device(device_list[0]);

			// Singstar uses the same device for 2 players
			if (device_list.size() >= 2)
			{
				device.add_device(device_list[1]);
			}

			wake_up();
		}
		else
		{
			[[maybe_unused]] const u32 index = register_device(device_list[0]);
		}
	}
}

u32 mic_context::register_device(const std::string& device_name)
{
	usz index = mic_list.size();
	for (usz i = 0; i < mic_list.size(); i++)
	{
		microphone_device& device = ::at32(mic_list, i);
		if (!device.is_registered())
		{
			if (index == mic_list.size())
			{
				index = i;
			}
		}
		else if (device_name == device.get_device_name())
		{
			// TODO: what happens if the device is registered twice?
			return ::narrow<u32>(i);
		}
	}

	// TODO: Check max mics properly
	ensure(index < mic_list.size(), "cellMic max mics exceeded during registration");

	switch (g_cfg.audio.microphone_type)
	{
	case microphone_handler::standard:
	case microphone_handler::real_singstar:
	case microphone_handler::rocksmith:
	{
		microphone_device& device = ::at32(mic_list, index);
		device = microphone_device(g_cfg.audio.microphone_type.get());
		device.set_registered(true);
		device.add_device(device_name);

		if (auto mic_queue = lv2_event_queue::find(event_queue_key))
		{
			mic_queue->send(event_queue_source, CELLMIC_ATTACH, index, 0);
		}

		break;
	}
	case microphone_handler::singstar:
	case microphone_handler::null:
	default:
		break;
	}

	wake_up();

	return ::narrow<u32>(index);
}

void mic_context::unregister_device(u32 dev_num)
{
	// Don't allow to unregister the first device for now.
	if (dev_num == 0 || dev_num >= mic_list.size())
	{
		return;
	}

	microphone_device& device = ::at32(mic_list, dev_num);
	device = microphone_device();

	if (auto mic_queue = lv2_event_queue::find(event_queue_key))
	{
		mic_queue->send(event_queue_source, CELLMIC_DETACH, dev_num, 0);
	}
}

bool mic_context::check_device(u32 dev_num)
{
	if (dev_num >= mic_list.size())
		return false;

	microphone_device& device = ::at32(mic_list, dev_num);
	return device.is_registered();
}

// Static functions

template <u32 bytesize>
inline void microphone_device::variable_byteswap(const void* src, void* dst)
{
	if constexpr (bytesize == 4)
	{
		*static_cast<u32*>(dst) = *static_cast<const be_t<u32>*>(src);
	}
	else if constexpr (bytesize == 2)
	{
		*static_cast<u16*>(dst) = *static_cast<const be_t<u16>*>(src);
	}
	else
	{
		fmt::throw_exception("variable_byteswap with bytesize %d unimplemented", bytesize);
	}
}

inline u32 microphone_device::convert_16_bit_pcm_to_float(const std::vector<u8>& buffer, u32 num_bytes)
{
	static_assert((float_buf_size % sizeof(f32)) == 0);

	float_buf.resize(float_buf_size, 0);
	const u32 bytes_to_write = static_cast<u32>(num_bytes * (sizeof(f32) / sizeof(s16)));
	ensure(bytes_to_write <= float_buf.size());

	const be_t<s16>* src = utils::bless<const be_t<s16>>(buffer.data());
	be_t<f32>* dst = reinterpret_cast<be_t<f32>*>(float_buf.data());

	for (usz i = 0; i < num_bytes / sizeof(s16); i++)
	{
		const be_t<s16> sample = *src++;

		const be_t<f32> normalized_sample_be = std::clamp(static_cast<f32>(sample) / std::numeric_limits<s16>::max(), -1.0f, 1.0f);

		*dst++ = normalized_sample_be;
	}

	return bytes_to_write;
}

// Public functions

microphone_device::microphone_device(microphone_handler type)
{
	device_type = type;
}

void microphone_device::add_device(const std::string& name)
{
	devices.push_back(mic_device{
		.name = name
	});
}

error_code microphone_device::open_microphone(const u8 type, const u32 dsp_r, const u32 raw_r, const u8 channels)
{
	signal_types     = type;
	dsp_samplingrate = dsp_r;
	raw_samplingrate = raw_r;
	num_channels     = channels;

#ifndef WITHOUT_OPENAL
	enumerate_devices();

	// Adjust number of channels depending on microphone type
	switch (device_type)
	{
	case microphone_handler::standard:
		break;
	case microphone_handler::singstar:
	case microphone_handler::real_singstar:
		// SingStar mic has always 2 channels, each channel represent a physical microphone
		ensure(num_channels >= 2);
		if (num_channels > 2)
		{
			cellMic.error("Tried to open a SingStar-type device with num_channels = %d", num_channels);
			num_channels = 2;
		}
		break;
	case microphone_handler::rocksmith:
		num_channels = 1;
		break;
	case microphone_handler::null:
	default:
		ensure(false);
		break;
	}

	ALCenum num_al_channels;
	switch (num_channels)
	{
	case 1:
		num_al_channels = AL_FORMAT_MONO16;
		break;
	case 2:
		// If we're using SingStar each device needs to be mono
		if (device_type == microphone_handler::singstar)
			num_al_channels = AL_FORMAT_MONO16;
		else
			num_al_channels = AL_FORMAT_STEREO16;
		break;
	case 4:
		num_al_channels = AL_FORMAT_QUAD16;
		break;
	default:
		cellMic.warning("Requested an invalid number of %d channels. Defaulting to 4 channels instead.", num_channels);
		num_al_channels = AL_FORMAT_QUAD16;
		num_channels = 4;
		break;
	}

	// Real firmware tries 4, 2 and then 1 channels if the channel count is not supported
	// TODO: The used channel count may vary for Sony's camera devices
	for (bool found_valid_channels = false; !found_valid_channels;)
	{
		switch (num_al_channels)
		{
		case AL_FORMAT_QUAD16:
			if (alcIsExtensionPresent(nullptr, "AL_EXT_MCFORMATS") == AL_TRUE)
			{
				found_valid_channels = true;
				break;
			}

			cellMic.warning("Requested 4 channels but AL_EXT_MCFORMATS not available, trying 2 channels next");
			num_al_channels = AL_FORMAT_STEREO16;
			num_channels = 2;
			break;
		case AL_FORMAT_STEREO16:
			if (true) // TODO: check stereo capability
			{
				found_valid_channels = true;
				break;
			}

			cellMic.warning("Requested 2 channels but extension is not available, trying 1 channel next");
			num_al_channels = AL_FORMAT_MONO16;
			num_channels = 1;
			break;
		case AL_FORMAT_MONO16:
			found_valid_channels = true;
			break;
		default:
			ensure(false);
			break;
		}
	}

	// Ensure that the code above delivers what it should
	switch (num_al_channels)
	{
	case AL_FORMAT_QUAD16:
		ensure(num_channels == 4 && device_type != microphone_handler::singstar && device_type != microphone_handler::real_singstar);
		break;
	case AL_FORMAT_STEREO16:
		ensure(num_channels == 2 && device_type != microphone_handler::singstar);
		break;
	case AL_FORMAT_MONO16:
		ensure(num_channels == 1 || (num_channels == 2 && device_type == microphone_handler::singstar));
		break;
	default:
		ensure(false);
		break;
	}

	ALCdevice* device = nullptr;

	// Make sure we use a proper sampling rate
	// TODO: The used sample rate may vary for Sony's camera devices
	const std::array<u32, 7> samplingrates = { raw_samplingrate, 48000u, 32000u, 24000u, 16000u, 12000u, 8000u };

	for (u32 samplingrate : samplingrates)
	{
		if (!std::any_of(samplingrates.cbegin() + 1, samplingrates.cend(), [samplingrate](u32 r){ return r == samplingrate; }))
		{
			cellMic.warning("Requested sampling rate %d, but we do not support it. Trying next sampling rate...", samplingrate);
			continue;
		}

		cellMic.notice("Trying sampling rate %d with %d channel(s)", samplingrate, num_channels);

		device = open_device(devices[0].name, samplingrate, num_al_channels, inbuf_size);
		if (!device)
		{
			continue;
		}

		// Use this sampling rate
		raw_samplingrate = samplingrate;
		cellMic.notice("Using sampling rate %d and %d channel(s)", raw_samplingrate, num_channels);
		break;
	}

	if (!device)
	{
		cellMic.error("Failed to open capture device '%s' (raw_samplingrate=%d, num_al_channels=0x%x, inbuf_size=%d)", devices[0].name, raw_samplingrate, num_al_channels, inbuf_size);
#ifdef _WIN32
		cellMic.error("Make sure microphone use is authorized under \"Microphone privacy settings\" in windows configuration");
#endif
		return CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT;
	}

	aux_samplingrate = dsp_samplingrate = raw_samplingrate; // Same rate for now

	ensure(!devices.empty());

	devices[0].device = device;
	devices[0].buf.resize(inbuf_size, 0);

	if (device_type == microphone_handler::singstar && devices.size() >= 2)
	{
		// Open a 2nd microphone into the same device
		num_al_channels = AL_FORMAT_MONO16;
		device = open_device(devices[1].name, raw_samplingrate, num_al_channels, inbuf_size);

		if (!device)
		{
			// Ignore it and move on
			cellMic.error("Failed to open 2nd SingStar capture device '%s' (raw_samplingrate=%d, num_al_channels=0x%x, inbuf_size=%d)", devices[1].name, raw_samplingrate, num_al_channels, inbuf_size);
		}
		else
		{
			devices[1].device = device;
			devices[1].buf.resize(inbuf_size, 0);
		}
	}

	if (device_type != microphone_handler::real_singstar)
	{
		temp_buf.resize(inbuf_size, 0);
	}

	sample_size = (bit_resolution / 8) * num_channels;

	mic_opened = true;
	return CELL_OK;
#else
	return CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT;
#endif
}

error_code microphone_device::close_microphone()
{
#ifndef WITHOUT_OPENAL
	if (mic_started)
	{
		stop_microphone();
	}

	for (mic_device& micdevice : devices)
	{
		if (alcCaptureCloseDevice(micdevice.device) != ALC_TRUE)
		{
			cellMic.error("Error closing capture device '%s'", micdevice.name);
		}

		micdevice.device = nullptr;
		micdevice.buf.clear();
	}

	temp_buf.clear();
	mic_opened = false;
#endif

	return CELL_OK;
}

error_code microphone_device::start_microphone()
{
#ifndef WITHOUT_OPENAL
	for (const mic_device& micdevice : devices)
	{
		alcCaptureStart(micdevice.device);
		if (ALCenum err = alcGetError(micdevice.device); err != ALC_NO_ERROR)
		{
			cellMic.error("Error starting capture of device '%s' (error=%s)", micdevice.name, fmt::alc_error{micdevice.device, err});
			stop_microphone();
			return CELL_MICIN_ERROR_FATAL;
		}
	}
#endif

	mic_started = true;
	return CELL_OK;
}

error_code microphone_device::stop_microphone()
{
#ifndef WITHOUT_OPENAL
	for (const mic_device& micdevice : devices)
	{
		alcCaptureStop(micdevice.device);
		if (ALCenum err = alcGetError(micdevice.device); err != ALC_NO_ERROR)
		{
			cellMic.error("Error stopping capture of device '%s' (error=%s)", micdevice.name, fmt::alc_error{micdevice.device, err});
		}
	}
#endif

	mic_started = false;
	return CELL_OK;
}

void microphone_device::update_audio()
{
	if (mic_registered && mic_opened && mic_started)
	{
		if (signal_types == CELLMIC_SIGTYPE_NULL)
			return;

		const u32 num_samples = capture_audio();

		if ((signal_types & CELLMIC_SIGTYPE_RAW) ||
			(signal_types & CELLMIC_SIGTYPE_DSP))
		{
			get_data(num_samples);
		}

		if (signal_types & CELLMIC_SIGTYPE_RAW)
		{
			get_raw(num_samples);
		}

		if (signal_types & CELLMIC_SIGTYPE_DSP)
		{
			get_dsp(num_samples);
		}

		// TODO: aux?
	}
}

bool microphone_device::has_data() const
{
	return mic_registered && mic_opened && mic_started && (rbuf_raw.has_data() || rbuf_dsp.has_data());
}

f32 microphone_device::calculate_energy_level()
{
	const auto& buffer = device_type == microphone_handler::real_singstar ? ::at32(devices, 0).buf : temp_buf;
	const size_t num_samples = buffer.size() / sizeof(s16);

	f64 sum_squares = 0.0;

	for (usz i = 0; i < num_samples; i++)
	{
		const be_t<s16> sample = read_from_ptr<be_t<s16>>(buffer, i * sizeof(s16));
		const f64 normalized_sample = static_cast<f64>(sample) / -std::numeric_limits<s16>::min();
		sum_squares += normalized_sample * normalized_sample;
	}

	const f32 rms = num_samples > 0 ? static_cast<f32>(std::sqrt(sum_squares / num_samples)) : 0.0f;
	constexpr f32 decibels_max = 90.0f;
	const f32 decibels_relative = 20.0f * std::log10(std::max(rms, 0.00001f));
	const f32 decibels = decibels_max + (decibels_relative * 0.5f);

	return std::clamp(decibels, 40.0f, decibels_max);
}

u32 microphone_device::capture_audio()
{
#ifndef WITHOUT_OPENAL
	ensure(sample_size > 0);

	u32 num_samples = inbuf_size / sample_size;

	for (const mic_device& micdevice : devices)
	{
		ALCint samples_in = 0;
		alcGetIntegerv(micdevice.device, ALC_CAPTURE_SAMPLES, 1, &samples_in);

		if (ALCenum err = alcGetError(micdevice.device); err != ALC_NO_ERROR)
		{
			cellMic.error("Error getting number of captured samples of device '%s' (error=%s)", micdevice.name, fmt::alc_error{micdevice.device, err});
			return CELL_MICIN_ERROR_FATAL;
		}

		num_samples = std::min<u32>(num_samples, samples_in);
	}

	if (num_samples == 0)
	{
		return 0;
	}

	for (mic_device& micdevice : devices)
	{
		alcCaptureSamples(micdevice.device, micdevice.buf.data(), num_samples);

		if (ALCenum err = alcGetError(micdevice.device); err != ALC_NO_ERROR)
		{
			cellMic.error("Error capturing samples of device '%s' (error=%s)", micdevice.name, fmt::alc_error{micdevice.device, err});
		}
	}

	return num_samples;
#else
	return 0;
#endif
}

// Private functions

#ifndef WITHOUT_OPENAL
void microphone_device::enumerate_devices()
{
	cellMic.notice("Enumerating capture devices...");
	enumerated_devices.clear();

	if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") == AL_TRUE)
	{
		if (const char* alc_devices = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER))
		{
			while (alc_devices && *alc_devices != 0)
			{
				cellMic.notice("Found capture device: '%s'", alc_devices);
				enumerated_devices.push_back(alc_devices);
				alc_devices += strlen(alc_devices) + 1;
			}
		}
	}
	else
	{
		// Without enumeration we can only use one device
		cellMic.error("OpenAl extension ALC_ENUMERATION_EXT not supported. The enumerated capture devices will only contain the default capture device.");

		if (const char* alc_device = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER))
		{
			cellMic.notice("Found default capture device: '%s'", alc_device);
			enumerated_devices.push_back(alc_device);
		}
	}
}

ALCdevice* microphone_device::open_device(const std::string& name, u32 samplingrate, ALCenum num_al_channels, u32 buf_size)
{
	if (std::none_of(enumerated_devices.cbegin(), enumerated_devices.cend(), [&name](const std::string& dev){ return dev == name; }))
	{
		cellMic.error("Capture device '%s' not in enumerated devices", name);
	}

	ALCdevice* device = alcCaptureOpenDevice(name.c_str(), samplingrate, num_al_channels, buf_size);

	if (ALCenum err = alcGetError(device); err != ALC_NO_ERROR || !device)
	{
		cellMic.warning("Failed to open capture device '%s' (error=%s, device=*0x%x, samplingrate=%d, num_al_channels=0x%x, buf_size=%d)", name, fmt::alc_error{device, err}, device, samplingrate, num_al_channels, buf_size);
		device = nullptr;
	}

	return device;
}
#endif

void microphone_device::get_data(const u32 num_samples)
{
	if (num_samples == 0)
	{
		return;
	}

	switch (device_type)
	{
	case microphone_handler::real_singstar:
	{
		// Straight copy from device. No need for intermediate buffer.
		break;
	}
	case microphone_handler::standard:
	case microphone_handler::rocksmith:
	{
		constexpr u8 channel_size = bit_resolution / 8;
		const usz bufsize = num_samples * sample_size;
		const std::vector<u8>& buf = ::at32(devices, 0).buf;
		ensure(bufsize <= buf.size());
		ensure(bufsize <= temp_buf.size());

		u8* tmp_ptr = temp_buf.data();

		// BE Translation
		for (u32 index = 0; index < bufsize; index += channel_size)
		{
			microphone_device::variable_byteswap<channel_size>(buf.data() + index, tmp_ptr + index);
		}
		break;
	}
	case microphone_handler::singstar:
	{
		ensure(sample_size == 4);

		// Each device buffer contains 16 bit mono samples
		const usz bufsize = num_samples * sizeof(u16);
		const std::vector<u8>& buf_0 = ::at32(devices, 0).buf;
		ensure(bufsize <= buf_0.size());

		u8* tmp_ptr = temp_buf.data();

		// Mixing the 2 mics into the 2 destination channels
		if (devices.size() == 2)
		{
			const std::vector<u8>& buf_1 = ::at32(devices, 1).buf;
			ensure(bufsize <= buf_1.size());

			for (u32 index = 0; index < (num_samples * 4); index += 4)
			{
				const u32 src_index = index / 2;

				tmp_ptr[index]     = buf_0[src_index];
				tmp_ptr[index + 1] = buf_0[src_index + 1];
				tmp_ptr[index + 2] = buf_1[src_index];
				tmp_ptr[index + 3] = buf_1[src_index + 1];
			}
		}
		else
		{
			for (u32 index = 0; index < (num_samples * 4); index += 4)
			{
				const u32 src_index = index / 2;

				tmp_ptr[index]     = buf_0[src_index];
				tmp_ptr[index + 1] = buf_0[src_index + 1];
				tmp_ptr[index + 2] = 0;
				tmp_ptr[index + 3] = 0;
			}
		}

		break;
	}
	case microphone_handler::null:
		ensure(false);
		break;
	}
}

void microphone_device::get_raw(const u32 num_samples)
{
	if (num_samples == 0)
	{
		return;
	}

	const std::vector<u8>& buf = device_type == microphone_handler::real_singstar ? ::at32(devices, 0).buf : temp_buf;
	const u32 bufsize = num_samples * sample_size;
	ensure(bufsize <= buf.size());

	rbuf_raw.write_bytes(buf.data(), bufsize);
}

void microphone_device::get_dsp(const u32 num_samples)
{
	if (num_samples == 0)
	{
		return;
	}

	const std::vector<u8>& buf = device_type == microphone_handler::real_singstar ? ::at32(devices, 0).buf : temp_buf;
	const u32 bufsize = num_samples * sample_size;
	ensure(bufsize <= buf.size());

	if (attr_dsptype != 0x01)
	{
		// Convert 16-bit PCM audio data to 32-bit float (DSP format)
		const u32 bufsize_float = convert_16_bit_pcm_to_float(buf, bufsize);
		rbuf_dsp.write_bytes(float_buf.data(), bufsize_float);
	}
	else
	{
		// The same as device RAW stream format, except that the data byte is always big-endian
		rbuf_dsp.write_bytes(buf.data(), bufsize);
	}
}

/// Initialization/Shutdown Functions

error_code cellMicInit()
{
	cellMic.notice("cellMicInit()");

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (mic_thr.init)
		return CELL_MICIN_ERROR_ALREADY_INIT;

	mic_thr.load_config_and_init();
	mic_thr.init = 1;

	return CELL_OK;
}

error_code cellMicEnd()
{
	cellMic.notice("cellMicEnd()");

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	// TODO
	mic_thr.init = 0;
	mic_thr.event_queue_key = 0;
	mic_thr.event_queue_source = 0;

	return CELL_OK;
}

/// Open/Close Microphone Functions

error_code cellMicOpenEx(s32 dev_num, s32 rawSampleRate, s32 rawChannel, s32 DSPSampleRate, s32 bufferSizeMS, u8 signalType)
{
	cellMic.notice("cellMicOpenEx(dev_num=%d, rawSampleRate=%d, rawChannel=%d, DSPSampleRate=%d, bufferSizeMS=%d, signalType=0x%x)",
		dev_num, rawSampleRate, rawChannel, DSPSampleRate, bufferSizeMS, signalType);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	if (device.is_opened())
		return CELL_MICIN_ERROR_ALREADY_OPEN;

	// TODO: bufferSizeMS

	return device.open_microphone(signalType, DSPSampleRate, rawSampleRate, rawChannel);
}

error_code cellMicOpen(s32 dev_num, s32 sampleRate)
{
	cellMic.trace("cellMicOpen(dev_num=%d sampleRate=%d)", dev_num, sampleRate);

	return cellMicOpenEx(dev_num, sampleRate, 1, sampleRate, 0x80, CELLMIC_SIGTYPE_DSP);
}

error_code cellMicOpenRaw(s32 dev_num, s32 sampleRate, s32 maxChannels)
{
	cellMic.trace("cellMicOpenRaw(dev_num=%d, sampleRate=%d, maxChannels=%d)", dev_num, sampleRate, maxChannels);

	return cellMicOpenEx(dev_num, sampleRate, maxChannels, sampleRate, 0x80, CELLMIC_SIGTYPE_RAW);
}

s32 cellMicIsOpen(s32 dev_num)
{
	cellMic.trace("cellMicIsOpen(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return false;

	if (!mic_thr.check_device(dev_num))
		return false;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);
	return device.is_opened();
}

s32 cellMicIsAttached(s32 dev_num)
{
	cellMic.trace("cellMicIsAttached(dev_num=%d)", dev_num);

	// TODO
	return 1;
}

error_code cellMicClose(s32 dev_num)
{
	cellMic.trace("cellMicClose(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	return device.close_microphone();
}

/// Starting/Stopping Microphone Functions

error_code cellMicStartEx(s32 dev_num, u32 iflags)
{
	cellMic.todo("cellMicStartEx(dev_num=%d, iflags=%d)", dev_num, iflags);

	if ((iflags & 0xfffffffc) != 0) // iflags > 3
		return CELL_MICIN_ERROR_PARAM;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	// TODO: flags

	cellMic.success("We're getting started mate!");

	return device.start_microphone();
}

error_code cellMicStart(s32 dev_num)
{
	cellMic.trace("cellMicStart(dev_num=%d)", dev_num);
	return cellMicStartEx(dev_num, 0);
}

error_code cellMicStop(s32 dev_num)
{
	cellMic.trace("cellMicStop(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	if (device.is_started())
	{
		return device.stop_microphone();
	}

	return CELL_OK;
}

/// Microphone Attributes/States Functions

error_code cellMicGetDeviceAttr(s32 dev_num, CellMicDeviceAttr deviceAttributes, vm::ptr<s32> arg1, vm::ptr<s32> arg2)
{
	cellMic.trace("cellMicGetDeviceAttr(dev_num=%d, deviceAttribute=%d, arg1=*0x%x, arg2=*0x%x)", dev_num, +deviceAttributes, arg1, arg2);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (dev_num < 0)
		return CELL_MICIN_ERROR_PARAM;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	if (arg1)
	{
		switch (deviceAttributes)
		{
		case CELLMIC_DEVATTR_CHANVOL:
			if (*arg1 > 2 || !arg2)
				return CELL_MICIN_ERROR_PARAM;

			if (*arg1 == 0)
			{
				// Calculate average volume of the channels
				*arg2 = std::accumulate(device.attr_chanvol.begin(), device.attr_chanvol.end(), 0u) / ::size32(device.attr_chanvol);
			}
			else
			{
				*arg2 = ::at32(device.attr_chanvol, *arg1 - 1);
			}

			break;
		case CELLMIC_DEVATTR_LED: *arg1 = device.attr_led; break;
		case CELLMIC_DEVATTR_GAIN: *arg1 = device.attr_gain; break;
		case CELLMIC_DEVATTR_VOLUME: *arg1 = device.attr_volume; break;
		case CELLMIC_DEVATTR_AGC: *arg1 = device.attr_agc; break;
		case CELLMIC_DEVATTR_DSPTYPE: *arg1 = device.attr_dsptype; break;
		default: return CELL_MICIN_ERROR_PARAM;
		}
	}

	return CELL_OK;
}

error_code cellMicSetDeviceAttr(s32 dev_num, CellMicDeviceAttr deviceAttributes, u32 arg1, u32 arg2)
{
	cellMic.trace("cellMicSetDeviceAttr(dev_num=%d, deviceAttributes=%d, arg1=%d, arg2=%d)", dev_num, +deviceAttributes, arg1, arg2);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	switch (deviceAttributes)
	{
	case CELLMIC_DEVATTR_CHANVOL:
		// Used by SingStar to set the volume of each mic
		if (arg1 > 2)
			return CELL_MICIN_ERROR_PARAM;

		if (arg1 == 0)
		{
			device.attr_chanvol.fill(arg2);
		}
		else
		{
			::at32(device.attr_chanvol, arg1 - 1) = arg2;
		}

		break;
	case CELLMIC_DEVATTR_LED: device.attr_led = arg1; break;
	case CELLMIC_DEVATTR_GAIN: device.attr_gain = arg1; break;
	case CELLMIC_DEVATTR_VOLUME: device.attr_volume = arg1; break;
	case CELLMIC_DEVATTR_AGC: device.attr_agc = arg1; break;
	case CELLMIC_DEVATTR_DSPTYPE: device.attr_dsptype = arg1; break;
	default: return CELL_MICIN_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellMicGetSignalAttr(s32 dev_num, CellMicSignalAttr sig_attrib, vm::ptr<void> value)
{
	cellMic.todo("cellMicGetSignalAttr(dev_num=%d, sig_attrib=%d, value=*0x%x)", dev_num, +sig_attrib, value);

	if (!value)
		return CELL_MICIN_ERROR_PARAM;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);
	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	// TODO

	return CELL_OK;
}

error_code cellMicSetSignalAttr(s32 dev_num, CellMicSignalAttr sig_attrib, vm::ptr<void> value)
{
	cellMic.todo("cellMicSetSignalAttr(dev_num=%d, sig_attrib=%d, value=*0x%x)", dev_num, +sig_attrib, value);

	if (!value)
		return CELL_MICIN_ERROR_PARAM;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);
	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	// TODO

	return CELL_OK;
}

error_code cellMicGetSignalState(s32 dev_num, CellMicSignalState sig_state, vm::ptr<void> value)
{
	cellMic.trace("cellMicGetSignalState(dev_num=%d, sig_state=%d, value=*0x%x)", dev_num, +sig_state, value);

	if (!value)
		return CELL_MICIN_ERROR_PARAM;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);
	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	be_t<u32>* ival = vm::_ptr<u32>(value.addr());
	be_t<f32>* fval = vm::_ptr<f32>(value.addr());

	// TODO

	switch (sig_state)
	{
	case CELLMIC_SIGSTATE_LOCTALK:
		*ival = 9; // Someone is probably talking (0 to 10)
		break;
	case CELLMIC_SIGSTATE_FARTALK:
		*ival = 1; // The speakers are probably off (0 to 10)
		break;
	case CELLMIC_SIGSTATE_NSR:
		*fval = 0.0f; // No noise reduction
		break;
	case CELLMIC_SIGSTATE_AGC:
		*fval = 1.0f; // No gain applied
		break;
	case CELLMIC_SIGSTATE_MICENG:
		*fval = device.calculate_energy_level();
		break;
	case CELLMIC_SIGSTATE_SPKENG:
		*fval = 10.0f; // 10 decibels
		break;
	default:
		return CELL_MICIN_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellMicGetFormatEx(s32 dev_num, vm::ptr<CellMicInputFormatI> format, /*CellMicSignalType*/u32 type)
{
	cellMic.trace("cellMicGetFormatEx(dev_num=%d, format=*0x%x, type=0x%x)", dev_num, format, type);

	if (!format)
		return CELL_MICIN_ERROR_PARAM;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);
	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	// TODO: type

	format->subframeSize  = device.get_bit_resolution() / 8; // Probably?
	format->bitResolution = device.get_bit_resolution();
	format->sampleRate    = device.get_raw_samplingrate();
	format->channelNum    = device.get_num_channels();
	format->dataType      = device.get_datatype();

	return CELL_OK;
}

error_code cellMicGetFormat(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.todo("cellMicGetFormat(dev_num=%d, format=*0x%x)", dev_num, format);
	return cellMicGetFormatEx(dev_num, format, CELLMIC_SIGTYPE_DSP);
}

error_code cellMicGetFormatRaw(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.trace("cellMicGetFormatRaw(dev_num=%d, format=0x%x)", dev_num, format);
	return cellMicGetFormatEx(dev_num, format, CELLMIC_SIGTYPE_RAW);
}

error_code cellMicGetFormatAux(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.todo("cellMicGetFormatAux(dev_num=%d, format=0x%x)", dev_num, format);
	return cellMicGetFormatEx(dev_num, format, CELLMIC_SIGTYPE_AUX);
}

error_code cellMicGetFormatDsp(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.todo("cellMicGetFormatDsp(dev_num=%d, format=0x%x)", dev_num, format);
	return cellMicGetFormatEx(dev_num, format, CELLMIC_SIGTYPE_DSP);
}

/// Event Queue Functions

error_code cellMicSetNotifyEventQueue(u64 key)
{
	cellMic.todo("cellMicSetNotifyEventQueue(key=0x%llx)", key);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	// default mic queue size = 4
	auto mic_queue = lv2_event_queue::find(key);
	if (!mic_queue)
		return CELL_MICIN_ERROR_EVENT_QUEUE;

	mic_thr.event_queue_key = key;

	// TODO: Properly generate/handle mic events
	for (usz i = 0; i < mic_thr.mic_list.size(); i++)
	{
		microphone_device& device = ::at32(mic_thr.mic_list, i);
		if (device.is_registered())
		{
			mic_queue->send(0, CELLMIC_ATTACH, i, 0);
		}
	}

	return CELL_OK;
}

error_code cellMicSetNotifyEventQueue2(u64 key, u64 source, u64 flag)
{
	cellMic.todo("cellMicSetNotifyEventQueue2(key=0x%llx, source=0x%llx, flag=0x%llx", key, source, flag);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	// default mic queue size = 4
	auto mic_queue = lv2_event_queue::find(key);
	if (!mic_queue)
		return CELL_MICIN_ERROR_EVENT_QUEUE;

	mic_thr.event_queue_key = key;
	mic_thr.event_queue_source = source;

	// TODO: Properly generate/handle mic events
	for (usz i = 0; i < mic_thr.mic_list.size(); i++)
	{
		microphone_device& device = ::at32(mic_thr.mic_list, i);
		if (device.is_registered())
		{
			mic_queue->send(source, CELLMIC_ATTACH, i, 0);
		}
	}

	return CELL_OK;
}

error_code cellMicRemoveNotifyEventQueue(u64 key)
{
	cellMic.warning("cellMicRemoveNotifyEventQueue(key=0x%llx)", key);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	mic_thr.event_queue_key = 0;
	mic_thr.event_queue_source = 0;

	return CELL_OK;
}

/// Reading Functions

error_code cell_mic_read(s32 dev_num, vm::ptr<void> data, s32 max_bytes, /*CellMicSignalType*/u32 type)
{
	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	if (!device.is_opened() || !(device.get_signal_types() & type))
		return CELL_MICIN_ERROR_NOT_OPEN;

	if (!data)
		return not_an_error(0);

	switch (type)
	{
	case CELLMIC_SIGTYPE_DSP: return not_an_error(device.read_dsp(vm::_ptr<u8>(data.addr()), max_bytes));
	case CELLMIC_SIGTYPE_AUX: return not_an_error(0); // TODO
	case CELLMIC_SIGTYPE_RAW: return not_an_error(device.read_raw(vm::_ptr<u8>(data.addr()), max_bytes));
	default:
		fmt::throw_exception("Invalid CELLMIC_SIGTYPE %d", type);
	}

	return not_an_error(0);
}

error_code cellMicReadRaw(s32 dev_num, vm::ptr<void> data, s32 max_bytes)
{
	cellMic.trace("cellMicReadRaw(dev_num=%d, data=0x%x, maxBytes=%d)", dev_num, data, max_bytes);
	return cell_mic_read(dev_num, data, max_bytes, CELLMIC_SIGTYPE_RAW);
}

error_code cellMicRead(s32 dev_num, vm::ptr<void> data, u32 max_bytes)
{
	cellMic.warning("cellMicRead(dev_num=%d, data=0x%x, maxBytes=0x%x)", dev_num, data, max_bytes);
	return cell_mic_read(dev_num, data, max_bytes, CELLMIC_SIGTYPE_DSP);
}

error_code cellMicReadAux(s32 dev_num, vm::ptr<void> data, s32 max_bytes)
{
	cellMic.todo("cellMicReadAux(dev_num=%d, data=0x%x, max_bytes=0x%x)", dev_num, data, max_bytes);
	return cell_mic_read(dev_num, data, max_bytes, CELLMIC_SIGTYPE_AUX);
}

error_code cellMicReadDsp(s32 dev_num, vm::ptr<void> data, s32 max_bytes)
{
	cellMic.warning("cellMicReadDsp(dev_num=%d, data=0x%x, max_bytes=0x%x)", dev_num, data, max_bytes);
	return cell_mic_read(dev_num, data, max_bytes, CELLMIC_SIGTYPE_DSP);
}

/// Unimplemented Functions

error_code cellMicReset(s32 dev_num)
{
	cellMic.todo("cellMicReset(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.check_device(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	microphone_device& device = ::at32(mic_thr.mic_list, dev_num);

	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	// TODO

	return CELL_OK;
}

error_code cellMicGetDeviceGUID(s32 dev_num, vm::ptr<u32> ptr_guid)
{
	cellMic.todo("cellMicGetDeviceGUID(dev_num=%d ptr_guid=*0x%x)", dev_num, ptr_guid);

	if (!ptr_guid)
		return CELL_MICIN_ERROR_PARAM;

	*ptr_guid = 0xffffffff;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	// TODO

	return CELL_OK;
}

error_code cellMicGetDeviceIdentifier(s32 dev_num, vm::ptr<u32> ptr_id)
{
	cellMic.todo("cellMicGetDeviceIdentifier(dev_num=%d, ptr_id=*0x%x)", dev_num, ptr_id);

	if (ptr_id)
		*ptr_id = 0x0;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	// TODO

	return CELL_OK;
}

error_code cellMicGetType(s32 dev_num, vm::ptr<s32> ptr_type)
{
	cellMic.trace("cellMicGetType(dev_num=%d, ptr_type=*0x%x)", dev_num, ptr_type);

	if (!ptr_type)
		return CELL_MICIN_ERROR_PARAM;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	// TODO: get proper type (log message is trace because of massive spam)
	*ptr_type = CELLMIC_TYPE_USBAUDIO; // Needed for Guitar Hero: Warriors of Rock (BLUS30487)

	return CELL_OK;
}

error_code cellMicGetStatus(s32 dev_num, vm::ptr<CellMicStatus> status)
{
	cellMic.todo("cellMicGetStatus(dev_num=%d, status=*0x%x)", dev_num, status);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (dev_num < 0 || !status)
		return CELL_MICIN_ERROR_PARAM;

	// TODO

	if (dev_num < static_cast<s32>(mic_thr.mic_list.size()))
	{
		const microphone_device& device = ::at32(mic_thr.mic_list, dev_num);
		status->raw_samprate = device.get_raw_samplingrate();
		status->dsp_samprate = device.get_raw_samplingrate();
		status->isStart = device.is_started();
		status->isOpen = device.is_opened();
		status->dsp_volume = 5; // TODO: 0 - 5 volume
		status->local_voice = 10; // TODO: 0 - 10 confidence
		status->remote_voice = 0; // TODO: 0 - 10 confidence
		status->mic_energy = 60; // TODO: Db
		status->spk_energy = 60; // TODO: Db
	}

	return CELL_OK;
}

error_code cellMicStopEx()
{
	cellMic.fatal("cellMicStopEx: unexpected function");
	return CELL_OK;
}

error_code cellMicSysShareClose()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicSetMultiMicNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicSysShareStop()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicSysShareOpen()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicCommand()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicSysShareStart()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicSysShareInit()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicSysShareEnd()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellMic)("cellMic", []()
{
	REG_FUNC(cellMic, cellMicInit);
	REG_FUNC(cellMic, cellMicEnd);
	REG_FUNC(cellMic, cellMicOpen);
	REG_FUNC(cellMic, cellMicClose);
	REG_FUNC(cellMic, cellMicGetDeviceGUID);
	REG_FUNC(cellMic, cellMicGetType);
	REG_FUNC(cellMic, cellMicIsAttached);
	REG_FUNC(cellMic, cellMicIsOpen);
	REG_FUNC(cellMic, cellMicGetDeviceAttr);
	REG_FUNC(cellMic, cellMicSetDeviceAttr);
	REG_FUNC(cellMic, cellMicGetSignalAttr);
	REG_FUNC(cellMic, cellMicSetSignalAttr);
	REG_FUNC(cellMic, cellMicGetSignalState);
	REG_FUNC(cellMic, cellMicStart);
	REG_FUNC(cellMic, cellMicRead);
	REG_FUNC(cellMic, cellMicStop);
	REG_FUNC(cellMic, cellMicReset);
	REG_FUNC(cellMic, cellMicSetNotifyEventQueue);
	REG_FUNC(cellMic, cellMicSetNotifyEventQueue2);
	REG_FUNC(cellMic, cellMicRemoveNotifyEventQueue);
	REG_FUNC(cellMic, cellMicOpenEx);
	REG_FUNC(cellMic, cellMicStartEx);
	REG_FUNC(cellMic, cellMicGetFormatRaw);
	REG_FUNC(cellMic, cellMicGetFormatAux);
	REG_FUNC(cellMic, cellMicGetFormatDsp);
	REG_FUNC(cellMic, cellMicOpenRaw);
	REG_FUNC(cellMic, cellMicReadRaw);
	REG_FUNC(cellMic, cellMicReadAux);
	REG_FUNC(cellMic, cellMicReadDsp);
	REG_FUNC(cellMic, cellMicGetStatus);
	REG_FUNC(cellMic, cellMicStopEx); // this function shouldn't exist
	REG_FUNC(cellMic, cellMicSysShareClose);
	REG_FUNC(cellMic, cellMicGetFormat);
	REG_FUNC(cellMic, cellMicSetMultiMicNotifyEventQueue);
	REG_FUNC(cellMic, cellMicGetFormatEx);
	REG_FUNC(cellMic, cellMicSysShareStop);
	REG_FUNC(cellMic, cellMicSysShareOpen);
	REG_FUNC(cellMic, cellMicCommand);
	REG_FUNC(cellMic, cellMicSysShareStart);
	REG_FUNC(cellMic, cellMicSysShareInit);
	REG_FUNC(cellMic, cellMicSysShareEnd);
	REG_FUNC(cellMic, cellMicGetDeviceIdentifier);
});
