#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/rsx_utils.h"
#include "Utilities/StrUtil.h"

#include "cellMic.h"
#include "cellAudioIn.h"
#include "cellAudioOut.h"
#include "cellVideoOut.h"

#include <optional>

LOG_CHANNEL(cellAvconfExt);

template<>
void fmt_class_string<CellAudioInError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_AUDIO_IN_ERROR_NOT_IMPLEMENTED);
			STR_CASE(CELL_AUDIO_IN_ERROR_ILLEGAL_CONFIGURATION);
			STR_CASE(CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER);
			STR_CASE(CELL_AUDIO_IN_ERROR_PARAMETER_OUT_OF_RANGE);
			STR_CASE(CELL_AUDIO_IN_ERROR_DEVICE_NOT_FOUND);
			STR_CASE(CELL_AUDIO_IN_ERROR_UNSUPPORTED_AUDIO_IN);
			STR_CASE(CELL_AUDIO_IN_ERROR_UNSUPPORTED_SOUND_MODE);
			STR_CASE(CELL_AUDIO_IN_ERROR_CONDITION_BUSY);
		}

		return unknown;
	});
}

struct avconf_manager
{
	shared_mutex mutex;

	struct device_info
	{
		CellAudioInDeviceInfo info {};
		std::string full_device_name; // The device name may be too long for CellAudioInDeviceInfo, so we additionally save the full name.
	};
	std::vector<device_info> devices;
	CellAudioInDeviceMode inDeviceMode = CELL_AUDIO_IN_SINGLE_DEVICE_MODE; // TODO: use somewhere

	void copy_device_info(u32 num, vm::ptr<CellAudioInDeviceInfo> info) const;
	std::optional<device_info> get_device_info(vm::cptr<char> name) const;

	avconf_manager();

	avconf_manager(const avconf_manager&) = delete;

	avconf_manager& operator=(const avconf_manager&) = delete;
};

avconf_manager::avconf_manager()
{
	u32 curindex = 0;

	const std::vector<std::string> mic_list = fmt::split(g_cfg.audio.microphone_devices.to_string(), {"@@@"});

	if (!mic_list.empty())
	{
		switch (g_cfg.audio.microphone_type)
		{
		case microphone_handler::standard:
		{
			for (u32 index = 0; index < mic_list.size(); index++)
			{
				device_info device {};
				device.info.portType                  = CELL_AUDIO_IN_PORT_USB;
				device.info.availableModeCount        = 1;
				device.info.state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
				device.info.deviceId                  = 0xE11CC0DE + curindex;
				device.info.type                      = 0xC0DEE11C;
				device.info.availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
				device.info.availableModes[0].channel = CELL_AUDIO_IN_CHNUM_2;
				device.info.availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
				device.info.deviceNumber              = curindex;
				device.full_device_name               = mic_list[index];
				strcpy_trunc(device.info.name, device.full_device_name);

				devices.push_back(std::move(device));
				curindex++;
			}
			break;
		}
		case microphone_handler::real_singstar:
		case microphone_handler::singstar:
		{
			// Only one device for singstar device
			device_info device {};
			device.info.portType                  = CELL_AUDIO_IN_PORT_USB;
			device.info.availableModeCount        = 1;
			device.info.state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
			device.info.deviceId                  = 0x00000001;
			device.info.type                      = 0x14150000;
			device.info.availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
			device.info.availableModes[0].channel = CELL_AUDIO_IN_CHNUM_2;
			device.info.availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
			device.info.deviceNumber              = curindex;
			device.full_device_name               = mic_list[0];
			strcpy_trunc(device.info.name, device.full_device_name);

			devices.push_back(std::move(device));
			curindex++;
			break;
		}
		case microphone_handler::rocksmith:
		{
			device_info device {};
			device.info.portType                  = CELL_AUDIO_IN_PORT_USB;
			device.info.availableModeCount        = 1;
			device.info.state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
			device.info.deviceId                  = 0x12BA00FF; // Specific to rocksmith usb input
			device.info.type                      = 0xC0DE73C4;
			device.info.availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
			device.info.availableModes[0].channel = CELL_AUDIO_IN_CHNUM_1;
			device.info.availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
			device.info.deviceNumber              = curindex;
			device.full_device_name               = mic_list[0];
			strcpy_trunc(device.info.name, device.full_device_name);

			devices.push_back(std::move(device));
			curindex++;
			break;
		}
		case microphone_handler::null:
		default:
			break;
		}
	}

	if (g_cfg.io.camera != camera_handler::null)
	{
		device_info device {};
		device.info.portType                  = CELL_AUDIO_IN_PORT_USB;
		device.info.availableModeCount        = 1;
		device.info.state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
		device.info.deviceId                  = 0xDEADBEEF;
		device.info.type                      = 0xBEEFDEAD;
		device.info.availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
		device.info.availableModes[0].channel = CELL_AUDIO_IN_CHNUM_NONE;
		device.info.availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
		device.info.deviceNumber              = curindex;
		device.full_device_name               = "USB Camera";
		strcpy_trunc(device.info.name, device.full_device_name);

		devices.push_back(std::move(device));
		curindex++;
	}
}

void avconf_manager::copy_device_info(u32 num, vm::ptr<CellAudioInDeviceInfo> info) const
{
	memset(info.get_ptr(), 0, sizeof(CellAudioInDeviceInfo));
	ensure(num < devices.size());
	*info = devices[num].info;
}

std::optional<avconf_manager::device_info> avconf_manager::get_device_info(vm::cptr<char> name) const
{
	for (const device_info& device : devices)
	{
		if (strncmp(device.info.name, name.get_ptr(), sizeof(device.info.name)) == 0)
		{
			return device;
		}
	}

	return std::nullopt;
}

error_code cellAudioOutUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioOutUnregisterDevice(deviceNumber=0x%x)", deviceNumber);
	return CELL_OK;
}

error_code cellAudioOutGetDeviceInfo2(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetDeviceInfo2(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);

	if (deviceIndex != 0 || !info)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

error_code cellVideoOutSetXVColor(u32 unk1, u32 unk2, u32 unk3)
{
	cellAvconfExt.todo("cellVideoOutSetXVColor(unk1=0x%x, unk2=0x%x, unk3=0x%x)", unk1, unk2, unk3);

	if (unk1 != 0)
	{
		return CELL_VIDEO_OUT_ERROR_NOT_IMPLEMENTED;
	}

	return CELL_OK;
}

error_code cellVideoOutSetupDisplay(u32 videoOut)
{
	cellAvconfExt.todo("cellVideoOutSetupDisplay(videoOut=%d)", videoOut);

	if (videoOut != CELL_VIDEO_OUT_SECONDARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	return CELL_OK;
}

error_code cellAudioInGetDeviceInfo(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioInDeviceInfo> info)
{
	cellAvconfExt.trace("cellAudioInGetDeviceInfo(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);

	if (deviceIndex != 0 || !info)
	{
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER;
	}

	auto& av_manager = g_fxo->get<avconf_manager>();
	std::lock_guard lock(av_manager.mutex);

	if (deviceNumber >= av_manager.devices.size())
		return CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND;

	av_manager.copy_device_info(deviceNumber, info);

	return CELL_OK;
}

template <bool Is_Float, bool Range_Limited>
void convert_cursor_color(const u8* src, u8* dst, s32 num, f32 gamma)
{
	for (s32 i = 0; i < num; i++, src += 4, dst += 4)
	{
		for (s32 c = 1; c < 4; c++)
		{
			if constexpr (Is_Float)
			{
				if constexpr (Range_Limited)
				{
					const f32 val = (src[c] / 255.0f) * 219.0f + 16.0f;
					dst[c] = static_cast<u8>(val + 0.5f);
				}
				else
				{
					dst[c] = src[c];
				}
			}
			else
			{
				f32 val = std::clamp(std::pow(src[c] / 255.0f, gamma), 0.0f, 1.0f);

				if constexpr (Range_Limited)
				{
					val = val * 219.0f + 16.0f;
				}
				else
				{
					val *= 255.0f;
				}

				dst[c] = static_cast<u8>(val + 0.5f);
			}
		}
	}
}

error_code cellVideoOutConvertCursorColor(u32 videoOut, s32 displaybuffer_format, f32 gamma, s32 source_buffer_format, vm::ptr<void> src_addr, vm::ptr<u32> dest_addr, s32 num)
{
	cellAvconfExt.warning("cellVideoOutConvertCursorColor(videoOut=%d, displaybuffer_format=0x%x, gamma=%f, source_buffer_format=0x%x, src_addr=*0x%x, dest_addr=*0x%x, num=0x%x)", videoOut,
			displaybuffer_format, gamma, source_buffer_format, src_addr, dest_addr, num);

	if (!src_addr || !dest_addr)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (displaybuffer_format < 0 ||
		displaybuffer_format > CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT ||
		source_buffer_format != CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8)
	{
		return CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	if (displaybuffer_format < CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT)
	{
		if (gamma < 0.8f || gamma > 1.2f)
		{
			return CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
		}
	}

	error_code cellVideoOutGetConvertCursorColorInfo(vm::ptr<u8> rgbOutputRange); // Forward declaration

	vm::var<u8> rgbOutputRange;
	if (error_code error = cellVideoOutGetConvertCursorColorInfo(rgbOutputRange))
	{
		return error;
	}

	const u8* src = reinterpret_cast<const u8*>(src_addr.get_ptr());
	u8* dst = reinterpret_cast<u8*>(dest_addr.get_ptr());

	if (displaybuffer_format == CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT)
	{
		if (*rgbOutputRange == CELL_VIDEO_OUT_RGB_OUTPUT_RANGE_LIMITED)
		{
			convert_cursor_color<true, true>(src, dst, num, gamma);
		}
		else
		{
			convert_cursor_color<true, false>(src, dst, num, gamma);
		}
	}
	else
	{
		if (*rgbOutputRange == CELL_VIDEO_OUT_RGB_OUTPUT_RANGE_LIMITED)
		{
			convert_cursor_color<false, true>(src, dst, num, gamma);
		}
		else
		{
			convert_cursor_color<false, false>(src, dst, num, gamma);
		}
	}

	return CELL_OK;
}

error_code cellVideoOutGetGamma(u32 videoOut, vm::ptr<f32> gamma)
{
	cellAvconfExt.warning("cellVideoOutGetGamma(videoOut=%d, gamma=*0x%x)", videoOut, gamma);

	if (!gamma)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	const auto& conf = g_fxo->get<rsx::avconf>();

	*gamma = conf.gamma;

	return CELL_OK;
}

error_code cellAudioInGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioInDeviceInfo> device_info)
{
	cellAvconfExt.trace("cellAudioInGetAvailableDeviceInfo(count=%d, info=*0x%x)", count, device_info);

	if (count > 16 || !device_info)
	{
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER;
	}

	auto& av_manager = g_fxo->get<avconf_manager>();
	std::lock_guard lock(av_manager.mutex);

	u32 num_devices_returned = std::min<u32>(count, ::size32(av_manager.devices));

	for (u32 index = 0; index < num_devices_returned; index++)
	{
		av_manager.copy_device_info(index, device_info + index);
	}

	CellAudioInDeviceInfo disconnected_device{};
	disconnected_device.state = CELL_AUDIO_OUT_DEVICE_STATE_UNAVAILABLE;
	disconnected_device.deviceNumber = 0xff;

	for (u32 index = num_devices_returned; index < count; index++)
	{
		device_info[index] = disconnected_device;
	}

	return not_an_error(num_devices_returned);
}

error_code cellAudioOutGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetAvailableDeviceInfo(count=0x%x, info=*0x%x)", count, info);

	if (count > 16 || !info)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return not_an_error(0); // number of available devices
}

error_code cellVideoOutSetGamma(u32 videoOut, f32 gamma)
{
	cellAvconfExt.trace("cellVideoOutSetGamma(videoOut=%d, gamma=%f)", videoOut, gamma);

	if (gamma < 0.8f || gamma > 1.2f)
	{
		return CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	auto& conf = g_fxo->get<rsx::avconf>();
	conf.gamma = gamma;

	return CELL_OK;
}

error_code cellAudioOutRegisterDevice(u64 deviceType, vm::cptr<char> name, vm::ptr<CellAudioOutRegistrationOption> option, vm::ptr<CellAudioOutDeviceConfiguration> config)
{
	cellAvconfExt.todo("cellAudioOutRegisterDevice(deviceType=0x%llx, name=%s, option=*0x%x, config=*0x%x)", deviceType, name, option, config);

	if (option || !name)
	{
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER; // Strange choice for an error
	}

	return not_an_error(0); // device number
}

error_code cellAudioOutSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioOutSetDeviceMode(deviceMode=0x%x)", deviceMode);

	if (deviceMode > CELL_AUDIO_OUT_MULTI_DEVICE_MODE_2)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

error_code cellAudioInSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioInSetDeviceMode(deviceMode=0x%x)", deviceMode);

	switch (deviceMode)
	{
	case CELL_AUDIO_IN_SINGLE_DEVICE_MODE:
	case CELL_AUDIO_IN_MULTI_DEVICE_MODE:
	case CELL_AUDIO_IN_MULTI_DEVICE_MODE_2:
	case CELL_AUDIO_IN_MULTI_DEVICE_MODE_10:
		break;
	default:
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER;
	}

	auto& av_manager = g_fxo->get<avconf_manager>();
	std::lock_guard lock(av_manager.mutex);

	av_manager.inDeviceMode = static_cast<CellAudioInDeviceMode>(deviceMode);

	return CELL_OK;
}

error_code cellAudioInRegisterDevice(u64 deviceType, vm::cptr<char> name, vm::ptr<CellAudioInRegistrationOption> option, vm::ptr<CellAudioInDeviceConfiguration> config)
{
	cellAvconfExt.todo("cellAudioInRegisterDevice(deviceType=0x%llx, name=%s, option=*0x%x, config=*0x%x)", deviceType, name, option, config);

	// option must be null, volume can be 1 (soft) to 5 (loud) (raises question about check for volume = 0)
	if (option || !config || !name || config->volume > 5)
	{
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER;
	}

	auto& av_manager = g_fxo->get<avconf_manager>();
	const std::lock_guard lock(av_manager.mutex);

	std::optional<avconf_manager::device_info> device = av_manager.get_device_info(name);
	if (!device)
	{
		// TODO
		return CELL_AUDIO_IN_ERROR_DEVICE_NOT_FOUND;
	}

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard mic_lock(mic_thr.mutex);
	const u32 device_number = mic_thr.register_device(device->full_device_name);

	return not_an_error(device_number);
}

error_code cellAudioInUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioInUnregisterDevice(deviceNumber=0x%x)", deviceNumber);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	mic_thr.unregister_device(deviceNumber);

	return CELL_OK;
}

error_code cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<f32> screenSize)
{
	cellAvconfExt.warning("cellVideoOutGetScreenSize(videoOut=%d, screenSize=*0x%x)", videoOut, screenSize);

	if (!screenSize)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	if (g_cfg.video.stereo_render_mode != stereo_render_mode_options::disabled)
	{
		// Return Playstation 3D display value
		// Some games call this function when 3D is enabled
		*screenSize = 24.f;
		return CELL_OK;
	}

	//	TODO: Use virtual screen size
#ifdef _WIN32
	//	HDC screen = GetDC(NULL);
	//	float diagonal = roundf(sqrtf((powf(float(GetDeviceCaps(screen, HORZSIZE)), 2) + powf(float(GetDeviceCaps(screen, VERTSIZE)), 2))) * 0.0393f);
#else
	// TODO: Linux implementation, without using wx
	// float diagonal = roundf(sqrtf((powf(wxGetDisplaySizeMM().GetWidth(), 2) + powf(wxGetDisplaySizeMM().GetHeight(), 2))) * 0.0393f);
#endif

	return CELL_VIDEO_OUT_ERROR_VALUE_IS_NOT_SET;
}

error_code cellVideoOutSetCopyControl(u32 videoOut, u32 control)
{
	cellAvconfExt.todo("cellVideoOutSetCopyControl(videoOut=%d, control=0x%x)", videoOut, control);

	if (control > CELL_VIDEO_OUT_COPY_CONTROL_COPY_NEVER)
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

error_code cellVideoOutConfigure2()
{
	cellAvconfExt.todo("cellVideoOutConfigure2()");

	if (false) // TODO
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (false) // TODO
	{
		return CELL_VIDEO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	return CELL_OK;
}

error_code cellAudioOutGetConfiguration2()
{
	cellAvconfExt.todo("cellAudioOutGetConfiguration2()");

	if (false) // TODO
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

error_code cellAudioOutConfigure2()
{
	cellAvconfExt.todo("cellAudioOutConfigure2()");

	if (false) // TODO
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

error_code cellVideoOutGetResolutionAvailability2()
{
	cellAvconfExt.todo("cellVideoOutGetResolutionAvailability2()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAvconfExt)("cellSysutilAvconfExt", []()
{
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutUnregisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutGetDeviceInfo2);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetXVColor);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetupDisplay);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInGetDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutConvertCursorColor);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutGetGamma);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInGetAvailableDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutGetAvailableDeviceInfo);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetGamma);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutRegisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutSetDeviceMode);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInSetDeviceMode);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInRegisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellAudioInUnregisterDevice);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutGetScreenSize);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutSetCopyControl);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutConfigure2);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutGetConfiguration2);
	REG_FUNC(cellSysutilAvconfExt, cellAudioOutConfigure2);
	REG_FUNC(cellSysutilAvconfExt, cellVideoOutGetResolutionAvailability2);
});
