#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/rsx_utils.h"
#include "Utilities/StrUtil.h"

#include "cellAudioIn.h"
#include "cellAudioOut.h"
#include "cellVideoOut.h"
#include "cellSysutil.h"

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
	std::vector<CellAudioInDeviceInfo> devices;

	void copy_device_info(u32 num, vm::ptr<CellAudioInDeviceInfo> info);
	avconf_manager();
};

avconf_manager::avconf_manager()
{
	u32 curindex = 0;

	auto mic_list = fmt::split(g_cfg.audio.microphone_devices, {"@@@"});
	if (!mic_list.empty())
	{
		switch (g_cfg.audio.microphone_type)
		{
		case microphone_handler::standard:
			for (u32 index = 0; index < mic_list.size(); index++)
			{
				devices.emplace_back();

				devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
				devices[curindex].availableModeCount        = 1;
				devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
				devices[curindex].deviceId                  = 0xE11CC0DE + curindex;
				devices[curindex].type                      = 0xC0DEE11C;
				devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
				devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_2;
				devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
				devices[curindex].deviceNumber              = curindex;
				strcpy_trunc(devices[curindex].name, mic_list[index]);

				curindex++;
			}
			break;
		case microphone_handler::real_singstar:
		case microphone_handler::singstar:
			// Only one device for singstar device
			devices.emplace_back();

			devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
			devices[curindex].availableModeCount        = 1;
			devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
			devices[curindex].deviceId                  = 0x57A3C0DE;
			devices[curindex].type                      = 0xC0DE57A3;
			devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
			devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_2;
			devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
			devices[curindex].deviceNumber              = curindex;
			strcpy_trunc(devices[curindex].name, mic_list[0]);

			curindex++;
			break;
		case microphone_handler::rocksmith:
			devices.emplace_back();

			devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
			devices[curindex].availableModeCount        = 1;
			devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
			devices[curindex].deviceId                  = 0x12BA00FF; // Specific to rocksmith usb input
			devices[curindex].type                      = 0xC0DE73C4;
			devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
			devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_1;
			devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
			devices[curindex].deviceNumber              = curindex;
			strcpy_trunc(devices[curindex].name, mic_list[0]);

			curindex++;
			break;
		default: break;
		}
	}

	if (g_cfg.io.camera != camera_handler::null)
	{
		devices.emplace_back();

		devices[curindex].portType                  = CELL_AUDIO_IN_PORT_USB;
		devices[curindex].availableModeCount        = 1;
		devices[curindex].state                     = CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE;
		devices[curindex].deviceId                  = 0xDEADBEEF;
		devices[curindex].type                      = 0xBEEFDEAD;
		devices[curindex].availableModes[0].type    = CELL_AUDIO_IN_CODING_TYPE_LPCM;
		devices[curindex].availableModes[0].channel = CELL_AUDIO_IN_CHNUM_NONE;
		devices[curindex].availableModes[0].fs      = CELL_AUDIO_IN_FS_8KHZ | CELL_AUDIO_IN_FS_12KHZ | CELL_AUDIO_IN_FS_16KHZ | CELL_AUDIO_IN_FS_24KHZ | CELL_AUDIO_IN_FS_32KHZ | CELL_AUDIO_IN_FS_48KHZ;
		devices[curindex].deviceNumber              = curindex;
		strcpy_trunc(devices[curindex].name, "USB Camera");

		curindex++;
	}
}

void avconf_manager::copy_device_info(u32 num, vm::ptr<CellAudioInDeviceInfo> info)
{
	memset(info.get_ptr(), 0, sizeof(CellAudioInDeviceInfo));

	info->portType                  = devices[num].portType;
	info->availableModeCount        = devices[num].availableModeCount;
	info->state                     = devices[num].state;
	info->deviceId                  = devices[num].deviceId;
	info->type                      = devices[num].type;
	info->availableModes[0].type    = devices[num].availableModes[0].type;
	info->availableModes[0].channel = devices[num].availableModes[0].channel;
	info->availableModes[0].fs      = devices[num].availableModes[0].fs;
	info->deviceNumber              = devices[num].deviceNumber;
	strcpy_trunc(info->name, devices[num].name);
}

error_code cellAudioOutUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioOutUnregisterDevice(deviceNumber=0x%x)", deviceNumber);
	return CELL_OK;
}

error_code cellAudioOutGetDeviceInfo2(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetDeviceInfo2(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);
	return CELL_OK;
}

error_code cellVideoOutSetXVColor()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

error_code cellVideoOutSetupDisplay()
{
	UNIMPLEMENTED_FUNC(cellAvconfExt);
	return CELL_OK;
}

error_code cellAudioInGetDeviceInfo(u32 deviceNumber, u32 deviceIndex, vm::ptr<CellAudioInDeviceInfo> info)
{
	cellAvconfExt.todo("cellAudioInGetDeviceInfo(deviceNumber=0x%x, deviceIndex=0x%x, info=*0x%x)", deviceNumber, deviceIndex, info);

	if (deviceIndex != 0 || !info)
	{
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER;
	}

	auto av_manager = g_fxo->get<avconf_manager>();

	if (deviceNumber >= av_manager->devices.size())
		return CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND;

	av_manager->copy_device_info(deviceNumber, info);

	return CELL_OK;
}

error_code cellVideoOutConvertCursorColor(u32 videoOut, s32 displaybuffer_format, f32 gamma, s32 source_buffer_format, vm::ptr<void> src_addr, vm::ptr<u32> dest_addr, s32 num)
{
	cellAvconfExt.todo("cellVideoOutConvertCursorColor(videoOut=%d, displaybuffer_format=0x%x, gamma=0x%x, source_buffer_format=0x%x, src_addr=*0x%x, dest_addr=*0x%x, num=0x%x)", videoOut,
			displaybuffer_format, gamma, source_buffer_format, src_addr, dest_addr, num);
	return CELL_OK;
}

error_code cellVideoOutGetGamma(u32 videoOut, vm::ptr<f32> gamma)
{
	cellAvconfExt.warning("cellVideoOutGetGamma(videoOut=%d, gamma=*0x%x)", videoOut, gamma);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	auto conf = g_fxo->get<rsx::avconf>();
	*gamma    = conf->gamma;

	return CELL_OK;
}

error_code cellAudioInGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioInDeviceInfo> device_info)
{
	cellAvconfExt.todo("cellAudioInGetAvailableDeviceInfo(count=0x%x, info=*0x%x)", count, device_info);

	if (count > 16 || !device_info)
	{
		return CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER;
	}

	auto av_manager = g_fxo->get<avconf_manager>();

	u32 num_devices_returned = std::min<u32>(count, ::size32(av_manager->devices));

	for (u32 index = 0; index < num_devices_returned; index++)
	{
		av_manager->copy_device_info(index, device_info + index);
	}

	return not_an_error(num_devices_returned);
}

error_code cellAudioOutGetAvailableDeviceInfo(u32 count, vm::ptr<CellAudioOutDeviceInfo2> info)
{
	cellAvconfExt.todo("cellAudioOutGetAvailableDeviceInfo(count=0x%x, info=*0x%x)", count, info);
	return not_an_error(0); // number of available devices
}

error_code cellVideoOutSetGamma(u32 videoOut, f32 gamma)
{
	cellAvconfExt.warning("cellVideoOutSetGamma(videoOut=%d, gamma=%f)", videoOut, gamma);

	if (!(gamma >= 0.8f && gamma <= 1.2f))
	{
		return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	auto conf   = g_fxo->get<rsx::avconf>();
	conf->gamma = gamma;

	return CELL_OK;
}

error_code cellAudioOutRegisterDevice(u64 deviceType, vm::cptr<char> name, vm::ptr<CellAudioOutRegistrationOption> option, vm::ptr<CellAudioOutDeviceConfiguration> config)
{
	cellAvconfExt.todo("cellAudioOutRegisterDevice(deviceType=0x%llx, name=%s, option=*0x%x, config=*0x%x)", deviceType, name, option, config);
	return not_an_error(0); // device number
}

error_code cellAudioOutSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioOutSetDeviceMode(deviceMode=0x%x)", deviceMode);
	return CELL_OK;
}

error_code cellAudioInSetDeviceMode(u32 deviceMode)
{
	cellAvconfExt.todo("cellAudioInSetDeviceMode(deviceMode=0x%x)", deviceMode);
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

	return not_an_error(0); // device number
}

error_code cellAudioInUnregisterDevice(u32 deviceNumber)
{
	cellAvconfExt.todo("cellAudioInUnregisterDevice(deviceNumber=0x%x)", deviceNumber);
	return CELL_OK;
}

error_code cellVideoOutGetScreenSize(u32 videoOut, vm::ptr<f32> screenSize)
{
	cellAvconfExt.warning("cellVideoOutGetScreenSize(videoOut=%d, screenSize=*0x%x)", videoOut, screenSize);

	if (videoOut != CELL_VIDEO_OUT_PRIMARY)
	{
		return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
	}

	if (g_cfg.video.enable_3d)
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
	return CELL_OK;
}

error_code cellVideoOutConfigure2()
{
	cellAvconfExt.todo("cellVideoOutConfigure2()");
	return CELL_OK;
}

error_code cellAudioOutGetConfiguration2()
{
	cellAvconfExt.todo("cellAudioOutGetConfiguration2()");
	return CELL_OK;
}

error_code cellAudioOutConfigure2()
{
	cellAvconfExt.todo("cellAudioOutConfigure2()");
	return CELL_OK;
}

error_code cellVideoOutGetResolutionAvailability2()
{
	cellAvconfExt.todo("cellVideoOutGetResolutionAvailability2()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAvconfExt)
("cellSysutilAvconfExt", []() {
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
