#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Utilities/StrUtil.h"

#include "cellMic.h"
#include <Emu/IdManager.h>
#include <Emu/Cell/lv2/sys_event.h>

LOG_CHANNEL(cellMic);

template<>
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

template<>
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

void mic_context::operator()()
{
	while (thread_ctrl::state() != thread_state::aborting)
	{
		// The time between processing is copied from audio thread
		// Might be inaccurate for mic thread
		if (Emu.IsPaused())
		{
			thread_ctrl::wait_for(1000); // hack
			continue;
		}

		const u64 stamp0   = get_guest_system_time();
		const u64 time_pos = stamp0 - start_time - Emu.GetPauseTime();

		const u64 expected_time = m_counter * 256 * 1000000 / 48000;
		if (expected_time >= time_pos)
		{
			thread_ctrl::wait_for(1000); // hack
			continue;
		}
		m_counter++;

		// Process signals
		{
			std::lock_guard lock(mutex);

			for (auto& mic_entry : mic_list)
			{
				auto& mic = mic_entry.second;
				mic.update_audio();
			}

			auto mic_queue = lv2_event_queue::find(event_queue_key);
			if (!mic_queue)
				continue;

			for (auto& mic_entry : mic_list)
			{
				auto& mic = mic_entry.second;
				if (mic.has_data())
				{
					mic_queue->send(0, CELLMIC_DATA, mic_entry.first, 0);
				}
			}
		}
	}

	// Cleanup
	for (auto& mic_entry : mic_list)
	{
		mic_entry.second.close_microphone();
	}
}

void mic_context::load_config_and_init()
{
	auto device_list = fmt::split(g_cfg.audio.microphone_devices.to_string(), {"@@@"});

	if (!device_list.empty() && mic_list.empty())
	{
		switch (g_cfg.audio.microphone_type)
		{
		case microphone_handler::standard:
		{
			for (s32 index = 0; index < static_cast<s32>(device_list.size()); index++)
			{
				mic_list.emplace(std::piecewise_construct, std::forward_as_tuple(index), std::forward_as_tuple(microphone_handler::standard));
				mic_list.at(index).add_device(device_list[index]);
			}
			break;
		}
		case microphone_handler::singstar:
		{
			mic_list.emplace(std::piecewise_construct, std::forward_as_tuple(0), std::forward_as_tuple(microphone_handler::singstar));
			mic_list.at(0).add_device(device_list[0]);
			if (device_list.size() >= 2)
				mic_list.at(0).add_device(device_list[1]);
			break;
		}
		case microphone_handler::real_singstar:
		case microphone_handler::rocksmith:
		{
			mic_list.emplace(std::piecewise_construct, std::forward_as_tuple(0), std::forward_as_tuple(g_cfg.audio.microphone_type));
			mic_list.at(0).add_device(device_list[0]);
			break;
		}
		case microphone_handler::null:
		default: break;
		}
	}
}

// Static functions

void microphone_device::variable_byteswap(const void* src, void* dst, const u32 bytesize)
{
	switch (bytesize)
	{
	case 4: *static_cast<u32*>(dst) = *static_cast<const be_t<u32>*>(src); break;
	case 2: *static_cast<u16*>(dst) = *static_cast<const be_t<u16>*>(src); break;
	}
}

// Public functions

microphone_device::microphone_device(microphone_handler type)
{
	device_type = type;
}

void microphone_device::add_device(const std::string& name)
{
	device_name.push_back(name);
}

error_code microphone_device::open_microphone(const u8 type, const u32 dsp_r, const u32 raw_r, const u8 channels)
{
	signal_types     = type;
	dsp_samplingrate = dsp_r;
	raw_samplingrate = raw_r;
	num_channels     = channels;

	// Adjust number of channels depending on microphone type
	switch (device_type)
	{
	case microphone_handler::standard:
		if (num_channels > 2)
		{
			cellMic.warning("Reducing number of mic channels from %d to 2 for %s", num_channels, device_name[0]);
			num_channels = 2;
		}
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
	case microphone_handler::rocksmith: num_channels = 1; break;
	case microphone_handler::null:
	default: ensure(false); break;
	}

	ALCenum num_al_channels;
	switch (num_channels)
	{
	case 1: num_al_channels = AL_FORMAT_MONO16; break;
	case 2:
		// If we're using SingStar each device needs to be mono
		if (device_type == microphone_handler::singstar)
			num_al_channels = AL_FORMAT_MONO16;
		else
			num_al_channels = AL_FORMAT_STEREO16;
		break;
	case 4:
		if (alcIsExtensionPresent(nullptr, "AL_EXT_MCFORMATS") == AL_TRUE)
		{
			num_al_channels = AL_FORMAT_QUAD16;
		}
		else
		{
			cellMic.error("Requested 4 channels but AL_EXT_MCFORMATS not available, settling down to 2");
			num_channels    = 2;
			num_al_channels = AL_FORMAT_STEREO16;
		}
		break;
	default:
		cellMic.fatal("Requested an invalid number of channels: %d", num_channels);
		num_al_channels = AL_FORMAT_STEREO16;
		break;
	}

	ALCdevice* device = alcCaptureOpenDevice(device_name[0].c_str(), raw_samplingrate, num_al_channels, inbuf_size);

	if (alcGetError(device) != ALC_NO_ERROR)
	{
		cellMic.error("Error opening capture device %s", device_name[0]);
#ifdef _WIN32
		cellMic.error("Make sure microphone use is authorized under \"Microphone privacy settings\" in windows configuration");
#endif
		return CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT;
	}

	input_devices.push_back(device);
	internal_bufs.emplace_back();
	internal_bufs[0].resize(inbuf_size, 0);
	temp_buf.resize(inbuf_size, 0);

	if (device_type == microphone_handler::singstar && device_name.size() >= 2)
	{
		// Open a 2nd microphone into the same device
		device = alcCaptureOpenDevice(device_name[1].c_str(), raw_samplingrate, AL_FORMAT_MONO16, inbuf_size);
		if (alcGetError(device) != ALC_NO_ERROR)
		{
			// Ignore it and move on
			cellMic.error("Error opening 2nd SingStar capture device %s", device_name[1]);
		}
		else
		{
			input_devices.push_back(device);
			internal_bufs.emplace_back();
			internal_bufs[1].resize(inbuf_size, 0);
		}
	}

	sample_size = (bit_resolution / 8) * num_channels;

	mic_opened = true;
	return CELL_OK;
}

error_code microphone_device::close_microphone()
{
	if (mic_started)
	{
		stop_microphone();
	}

	for (const auto& micdevice : input_devices)
	{
		if (alcCaptureCloseDevice(micdevice) != ALC_TRUE)
		{
			cellMic.error("Error closing capture device");
		}
	}

	input_devices.clear();
	internal_bufs.clear();

	mic_opened = false;

	return CELL_OK;
}

error_code microphone_device::start_microphone()
{
	for (const auto& micdevice : input_devices)
	{
		alcCaptureStart(micdevice);
		if (alcGetError(micdevice) != ALC_NO_ERROR)
		{
			cellMic.error("Error starting capture");
			stop_microphone();
			return CELL_MICIN_ERROR_FATAL;
		}
	}

	mic_started = true;

	return CELL_OK;
}

error_code microphone_device::stop_microphone()
{
	for (const auto& micdevice : input_devices)
	{
		alcCaptureStop(micdevice);
		if (alcGetError(micdevice) != ALC_NO_ERROR)
		{
			cellMic.error("Error stopping capture");
		}
	}

	mic_started = false;

	return CELL_OK;
}

void microphone_device::update_audio()
{
	if (mic_opened && mic_started)
	{
		if (signal_types == CELLMIC_SIGTYPE_NULL)
			return;

		const u32 num_samples = capture_audio();

		if (signal_types & CELLMIC_SIGTYPE_RAW)
			get_raw(num_samples);
		if (signal_types & CELLMIC_SIGTYPE_DSP)
			get_dsp(num_samples);
		// TODO: aux?
	}
}

bool microphone_device::has_data() const
{
	return mic_opened && mic_started && (rbuf_raw.has_data() || rbuf_dsp.has_data());
}

u32 microphone_device::capture_audio()
{
	ensure(sample_size > 0);

	u32 num_samples = inbuf_size / sample_size;

	for (const auto micdevice : input_devices)
	{
		ALCint samples_in = 0;
		alcGetIntegerv(micdevice, ALC_CAPTURE_SAMPLES, 1, &samples_in);
		if (alcGetError(micdevice) != ALC_NO_ERROR)
		{
			cellMic.error("Error getting number of captured samples");
			return CELL_MICIN_ERROR_FATAL;
		}
		num_samples = std::min<u32>(num_samples, samples_in);
	}

	for (u32 index = 0; index < input_devices.size(); index++)
	{
		alcCaptureSamples(input_devices[index], internal_bufs[index].data(), num_samples);
	}

	return num_samples;
}

// Private functions

void microphone_device::get_raw(const u32 num_samples)
{
	u8* tmp_ptr = temp_buf.data();

	switch (device_type)
	{
	case microphone_handler::real_singstar:
		// Straight copy from device
		memcpy(tmp_ptr, internal_bufs[0].data(), num_samples * (bit_resolution / 8) * num_channels);
		break;
	case microphone_handler::standard:
	case microphone_handler::rocksmith:
		// BE Translation
		for (u32 index = 0; index < num_samples; index++)
		{
			for (u32 indchan = 0; indchan < num_channels; indchan++)
			{
				const u32 curindex = (index * sample_size) + indchan * (bit_resolution / 8);
				microphone_device::variable_byteswap(internal_bufs[0].data() + curindex, tmp_ptr + curindex, bit_resolution / 8);
			}
		}
		break;
	case microphone_handler::singstar:
		ensure(sample_size == 4);

		// Mixing the 2 mics as if channels
		if (input_devices.size() == 2)
		{
			for (u32 index = 0; index < (num_samples * 4); index += 4)
			{
				tmp_ptr[index]     = internal_bufs[0][(index / 2)];
				tmp_ptr[index + 1] = internal_bufs[0][(index / 2) + 1];
				tmp_ptr[index + 2] = internal_bufs[1][(index / 2)];
				tmp_ptr[index + 3] = internal_bufs[1][(index / 2) + 1];
			}
		}
		else
		{
			for (u32 index = 0; index < (num_samples * 4); index += 4)
			{
				tmp_ptr[index]     = internal_bufs[0][(index / 2)];
				tmp_ptr[index + 1] = internal_bufs[0][(index / 2) + 1];
				tmp_ptr[index + 2] = 0;
				tmp_ptr[index + 3] = 0;
			}
		}

		break;
	case microphone_handler::null:
	default: ensure(false); break;
	}

	rbuf_raw.write_bytes(tmp_ptr, num_samples * sample_size);
};

void microphone_device::get_dsp(const u32 num_samples)
{
	u8* tmp_ptr = temp_buf.data();

	switch (device_type)
	{
	case microphone_handler::real_singstar:
		// Straight copy from device
		memcpy(tmp_ptr, internal_bufs[0].data(), num_samples * (bit_resolution / 8) * num_channels);
		break;
	case microphone_handler::standard:
	case microphone_handler::rocksmith:
		// BE Translation
		for (u32 index = 0; index < num_samples; index++)
		{
			for (u32 indchan = 0; indchan < num_channels; indchan++)
			{
				const u32 curindex = (index * sample_size) + indchan * (bit_resolution / 8);
				microphone_device::variable_byteswap(internal_bufs[0].data() + curindex, tmp_ptr + curindex, bit_resolution / 8);
			}
		}
		break;
	case microphone_handler::singstar:
		ensure(sample_size == 4);

		// Mixing the 2 mics as if channels
		if (input_devices.size() == 2)
		{
			for (u32 index = 0; index < (num_samples * 4); index += 4)
			{
				tmp_ptr[index]     = internal_bufs[0][(index / 2)];
				tmp_ptr[index + 1] = internal_bufs[0][(index / 2) + 1];
				tmp_ptr[index + 2] = internal_bufs[1][(index / 2)];
				tmp_ptr[index + 3] = internal_bufs[1][(index / 2) + 1];
			}
		}
		else
		{
			for (u32 index = 0; index < (num_samples * 4); index += 4)
			{
				tmp_ptr[index]     = internal_bufs[0][(index / 2)];
				tmp_ptr[index + 1] = internal_bufs[0][(index / 2) + 1];
				tmp_ptr[index + 2] = 0;
				tmp_ptr[index + 3] = 0;
			}
		}

		break;
	case microphone_handler::null:
	default: ensure(false); break;
	}

	rbuf_dsp.write_bytes(tmp_ptr, num_samples * sample_size);
};

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

	return CELL_OK;
}

/// Open/Close Microphone Functions

error_code cellMicOpen(s32 dev_num, s32 sampleRate)
{
	cellMic.trace("cellMicOpen(dev_num=%d sampleRate=%d)", dev_num, sampleRate);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	if (device.is_opened())
		return CELL_MICIN_ERROR_ALREADY_OPEN;

	return device.open_microphone(CELLMIC_SIGTYPE_DSP, sampleRate, sampleRate);
}

error_code cellMicOpenRaw(s32 dev_num, s32 sampleRate, s32 maxChannels)
{
	cellMic.trace("cellMicOpenRaw(dev_num=%d, sampleRate=%d, maxChannels=%d)", dev_num, sampleRate, maxChannels);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	if (device.is_opened())
		return CELL_MICIN_ERROR_ALREADY_OPEN;

	return device.open_microphone(CELLMIC_SIGTYPE_DSP | CELLMIC_SIGTYPE_RAW, sampleRate, sampleRate, maxChannels);
}

error_code cellMicOpenEx(s32 dev_num, s32 rawSampleRate, s32 rawChannel, s32 DSPSampleRate, s32 bufferSizeMS, u8 signalType)
{
	cellMic.trace("cellMicOpenEx(dev_num=%d, rawSampleRate=%d, rawChannel=%d, DSPSampleRate=%d, bufferSizeMS=%d, signalType=0x%x)",
		dev_num, rawSampleRate, rawChannel, DSPSampleRate, bufferSizeMS, signalType);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	if (device.is_opened())
		return CELL_MICIN_ERROR_ALREADY_OPEN;

	// TODO: bufferSizeMS

	return device.open_microphone(signalType, DSPSampleRate, rawSampleRate, rawChannel);
}

u8 cellMicIsOpen(s32 dev_num)
{
	cellMic.trace("cellMicIsOpen(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return false;

	if (!mic_thr.mic_list.count(dev_num))
		return false;

	return mic_thr.mic_list.at(dev_num).is_opened();
}

s32 cellMicIsAttached(s32 dev_num)
{
	cellMic.notice("cellMicIsAttached(dev_num=%d)", dev_num);
	return 1;
}

error_code cellMicClose(s32 dev_num)
{
	cellMic.trace("cellMicClose(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	return device.close_microphone();
}

/// Starting/Stopping Microphone Functions

error_code cellMicStart(s32 dev_num)
{
	cellMic.trace("cellMicStart(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	return device.start_microphone();
}

error_code cellMicStartEx(s32 dev_num, u32 iflags)
{
	cellMic.todo("cellMicStartEx(dev_num=%d, iflags=%d)", dev_num, iflags);

	// TODO: flags

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	if (!device.is_opened())
		return CELL_MICIN_ERROR_NOT_OPEN;

	cellMic.success("We're getting started mate!");

	return device.start_microphone();
}

error_code cellMicStop(s32 dev_num)
{
	cellMic.trace("cellMicStop(dev_num=%d)", dev_num);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

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

	if (!arg1 || (!arg2 && deviceAttributes == CELLMIC_DEVATTR_CHANVOL))
		return CELL_MICIN_ERROR_PARAM;

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	switch (deviceAttributes)
	{
	case CELLMIC_DEVATTR_LED: *arg1 = device.attr_led; break;
	case CELLMIC_DEVATTR_GAIN: *arg1 = device.attr_gain; break;
	case CELLMIC_DEVATTR_VOLUME: *arg1 = device.attr_volume; break;
	case CELLMIC_DEVATTR_AGC: *arg1 = device.attr_agc; break;
	case CELLMIC_DEVATTR_CHANVOL: *arg1 = device.attr_volume; break;
	case CELLMIC_DEVATTR_DSPTYPE: *arg1 = device.attr_dsptype; break;
	default: return CELL_MICIN_ERROR_PARAM;
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

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	switch (deviceAttributes)
	{
	case CELLMIC_DEVATTR_CHANVOL:
		// Used by SingStar to set the volume of each mic
		if (arg1 > 2)
			return CELL_MICIN_ERROR_PARAM;
		device.attr_chanvol[arg1] = arg2;
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
	return CELL_OK;
}

error_code cellMicSetSignalAttr(s32 dev_num, CellMicSignalAttr sig_attrib, vm::ptr<void> value)
{
	cellMic.todo("cellMicSetSignalAttr(dev_num=%d, sig_attrib=%d, value=*0x%x)", dev_num, +sig_attrib, value);
	return CELL_OK;
}

error_code cellMicGetSignalState(s32 dev_num, CellMicSignalState sig_state, vm::ptr<void> value)
{
	cellMic.todo("cellMicGetSignalState(dev_num=%d, sig_state=%d, value=*0x%x)", dev_num, +sig_state, value);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	be_t<u32>* ival = vm::_ptr<u32>(value.addr());
	be_t<f32>* fval = vm::_ptr<f32>(value.addr());

	switch (sig_state)
	{
	case CELLMIC_SIGSTATE_LOCTALK:
		*ival = 9; // Someone is probably talking
		break;
	case CELLMIC_SIGSTATE_FARTALK:
		// TODO
		break;
	case CELLMIC_SIGSTATE_NSR:
		// TODO
		break;
	case CELLMIC_SIGSTATE_AGC:
		// TODO
		break;
	case CELLMIC_SIGSTATE_MICENG:
		*fval = 40.0f; // 40 decibels
		break;
	case CELLMIC_SIGSTATE_SPKENG:
		// TODO
		break;
	default: return CELL_MICIN_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellMicGetFormatRaw(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.trace("cellMicGetFormatRaw(dev_num=%d, format=0x%x)", dev_num, format);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& device = mic_thr.mic_list.at(dev_num);

	format->subframeSize  = device.get_bit_resolution() / 8; // Probably?
	format->bitResolution = device.get_bit_resolution();
	format->sampleRate    = device.get_raw_samplingrate();
	format->channelNum    = device.get_num_channels();
	format->dataType      = device.get_datatype();

	return CELL_OK;
}

error_code cellMicGetFormatAux(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.todo("cellMicGetFormatAux(dev_num=%d, format=0x%x)", dev_num, format);

	return cellMicGetFormatRaw(dev_num, format);
}

error_code cellMicGetFormatDsp(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.todo("cellMicGetFormatDsp(dev_num=%d, format=0x%x)", dev_num, format);

	return cellMicGetFormatRaw(dev_num, format);
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

	for (auto& mic_entry : mic_thr.mic_list)
	{
		mic_queue->send(0, CELLMIC_ATTACH, mic_entry.first, 0);
	}

	return CELL_OK;
}

error_code cellMicSetNotifyEventQueue2(u64 key, u64 source, u64 flag)
{
	// TODO: Actually do things with the source variable
	cellMic.todo("cellMicSetNotifyEventQueue2(key=0x%llx, source=0x%llx, flag=0x%llx", key, source, flag);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	// default mic queue size = 4
	auto mic_queue = lv2_event_queue::find(key);
	if (!mic_queue)
		return CELL_MICIN_ERROR_EVENT_QUEUE;

	mic_queue->send(0, CELLMIC_ATTACH, 0, 0);
	mic_thr.event_queue_key = key;

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

	return CELL_OK;
}

/// Reading Functions

error_code cellMicReadRaw(s32 dev_num, vm::ptr<void> data, s32 max_bytes)
{
	cellMic.trace("cellMicReadRaw(dev_num=%d, data=0x%x, maxBytes=%d)", dev_num, data, max_bytes);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& mic = mic_thr.mic_list.at(dev_num);

	if (!mic.is_opened() || !(mic.get_signal_types() & CELLMIC_SIGTYPE_RAW))
		return CELL_MICIN_ERROR_NOT_OPEN;

	return mic.read_raw(vm::_ptr<u8>(data.addr()), max_bytes);
}

error_code cellMicRead(s32 dev_num, vm::ptr<void> data, u32 max_bytes)
{
	cellMic.todo("cellMicRead(dev_num=%d, data=0x%x, maxBytes=0x%x)", dev_num, data, max_bytes);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
		return CELL_MICIN_ERROR_NOT_INIT;

	if (!mic_thr.mic_list.count(dev_num))
		return CELL_MICIN_ERROR_DEVICE_NOT_FOUND;

	auto& mic = mic_thr.mic_list.at(dev_num);

	if (!mic.is_opened() || !(mic.get_signal_types() & CELLMIC_SIGTYPE_DSP))
		return CELL_MICIN_ERROR_NOT_OPEN;

	return mic.read_dsp(vm::_ptr<u8>(data.addr()), max_bytes);
}

error_code cellMicReadAux(s32 dev_num, vm::ptr<void> data, s32 max_bytes)
{
	cellMic.todo("cellMicReadAux(dev_num=%d, data=0x%x, max_bytes=0x%x)", dev_num, data, max_bytes);
	return CELL_OK;
}

error_code cellMicReadDsp(s32 dev_num, vm::ptr<void> data, s32 max_bytes)
{
	cellMic.todo("cellMicReadDsp(dev_num=%d, data=0x%x, max_bytes=0x%x)", dev_num, data, max_bytes);
	return CELL_OK;
}

/// Unimplemented Functions

error_code cellMicReset(s32 dev_num)
{
	cellMic.todo("cellMicReset(dev_num=%d)", dev_num);
	return CELL_OK;
}

error_code cellMicGetDeviceGUID(s32 dev_num, vm::ptr<u32> ptr_guid)
{
	cellMic.todo("cellMicGetDeviceGUID(dev_num=%d ptr_guid=*0x%x)", dev_num, ptr_guid);
	return CELL_OK;
}

error_code cellMicGetType(s32 dev_num, vm::ptr<s32> ptr_type)
{
	cellMic.todo("cellMicGetType(dev_num=%d, ptr_type=*0x%x)", dev_num, ptr_type);

	if (!ptr_type)
		return CELL_MICIN_ERROR_PARAM;

	*ptr_type = CELLMIC_TYPE_USBAUDIO;

	return CELL_OK;
}

error_code cellMicGetStatus(s32 dev_num, vm::ptr<CellMicStatus> status)
{
	cellMic.todo("cellMicGetStatus(dev_num=%d, status=*0x%x)", dev_num, status);
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

error_code cellMicGetFormat(s32 dev_num, vm::ptr<CellMicInputFormatI> format)
{
	cellMic.todo("cellMicGetFormat(dev_num=%d, format=*0x%x)", dev_num, format);
	return CELL_OK;
}

error_code cellMicSetMultiMicNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

error_code cellMicGetFormatEx()
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

error_code cellMicGetDeviceIdentifier()
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
