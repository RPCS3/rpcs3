#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellAudioOut.h"

LOG_CHANNEL(cellSysutil);

template<>
void fmt_class_string<CellAudioOutError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_AUDIO_OUT_ERROR_NOT_IMPLEMENTED);
			STR_CASE(CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION);
			STR_CASE(CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER);
			STR_CASE(CELL_AUDIO_OUT_ERROR_PARAMETER_OUT_OF_RANGE);
			STR_CASE(CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND);
			STR_CASE(CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT);
			STR_CASE(CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE);
			STR_CASE(CELL_AUDIO_OUT_ERROR_CONDITION_BUSY);
		}

		return unknown;
	});
}

error_code cellAudioOutGetSoundAvailability(u32 audioOut, u32 type, u32 fs, u32 option)
{
	cellSysutil.warning("cellAudioOutGetSoundAvailability(audioOut=%d, type=%d, fs=0x%x, option=%d)", audioOut, type, fs, option);

	s32 available = 8; // should be at least 2

	switch (fs)
	{
	case CELL_AUDIO_OUT_FS_32KHZ:
	case CELL_AUDIO_OUT_FS_44KHZ:
	case CELL_AUDIO_OUT_FS_48KHZ:
	case CELL_AUDIO_OUT_FS_88KHZ:
	case CELL_AUDIO_OUT_FS_96KHZ:
	case CELL_AUDIO_OUT_FS_176KHZ:
	case CELL_AUDIO_OUT_FS_192KHZ:
		break;

	default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch (type)
	{
	case CELL_AUDIO_OUT_CODING_TYPE_LPCM: break;
	case CELL_AUDIO_OUT_CODING_TYPE_AC3: available = 0; break;
	case CELL_AUDIO_OUT_CODING_TYPE_DTS: available = 0; break;

	default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY: return not_an_error(available);
	case CELL_AUDIO_OUT_SECONDARY: return not_an_error(0);
	}

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION;
}

error_code cellAudioOutGetSoundAvailability2(u32 audioOut, u32 type, u32 fs, u32 ch, u32 option)
{
	cellSysutil.warning("cellAudioOutGetSoundAvailability2(audioOut=%d, type=%d, fs=0x%x, ch=%d, option=%d)", audioOut, type, fs, ch, option);

	s32 available = 8; // should be at least 2

	switch (fs)
	{
	case CELL_AUDIO_OUT_FS_32KHZ:
	case CELL_AUDIO_OUT_FS_44KHZ:
	case CELL_AUDIO_OUT_FS_48KHZ:
	case CELL_AUDIO_OUT_FS_88KHZ:
	case CELL_AUDIO_OUT_FS_96KHZ:
	case CELL_AUDIO_OUT_FS_176KHZ:
	case CELL_AUDIO_OUT_FS_192KHZ:
		break;

	default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch (ch)
	{
	case 2: break;
	case 6: available = 0; break;
	case 8: available = 0; break;

	default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch (type)
	{
	case CELL_AUDIO_OUT_CODING_TYPE_LPCM: break;
	case CELL_AUDIO_OUT_CODING_TYPE_AC3: available = 0; break;
	case CELL_AUDIO_OUT_CODING_TYPE_DTS: available = 0; break;

	default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY: return not_an_error(available);
	case CELL_AUDIO_OUT_SECONDARY: return not_an_error(0);
	default:
		break;
	}

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION;
}

error_code cellAudioOutGetState(u32 audioOut, u32 deviceIndex, vm::ptr<CellAudioOutState> state)
{
	cellSysutil.warning("cellAudioOutGetState(audioOut=0x%x, deviceIndex=0x%x, state=*0x%x)", audioOut, deviceIndex, state);

	if (!state)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	*state = {};

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		state->state = CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;
		state->encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		state->downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
		state->soundMode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		state->soundMode.channel = CELL_AUDIO_OUT_CHNUM_8;
		state->soundMode.fs = CELL_AUDIO_OUT_FS_48KHZ;
		state->soundMode.reserved = 0;
		state->soundMode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;

		return CELL_OK;

	case CELL_AUDIO_OUT_SECONDARY:
		state->state = CELL_AUDIO_OUT_OUTPUT_STATE_DISABLED;

		return CELL_OK;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

error_code cellAudioOutConfigure(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option, u32 waitForEvent)
{
	cellSysutil.warning("cellAudioOutConfigure(audioOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", audioOut, config, option, waitForEvent);

	if (!config)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		if (config->channel)
		{
			//Emu.GetAudioManager().GetInfo().mode.channel = config->channel;
		}

		//Emu.GetAudioManager().GetInfo().mode.encoder = config->encoder;

		if (config->downMixer)
		{
			//Emu.GetAudioManager().GetInfo().mode.downMixer = config->downMixer;
		}

		return CELL_OK;

	case CELL_AUDIO_OUT_SECONDARY:
		return CELL_OK;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

error_code cellAudioOutGetConfiguration(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option)
{
	cellSysutil.warning("cellAudioOutGetConfiguration(audioOut=%d, config=*0x%x, option=*0x%x)", audioOut, config, option);

	if (!config)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (option)
	{
		*option = {};
	}

	*config = {};

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		config->channel = CELL_AUDIO_OUT_CHNUM_8;
		config->encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		config->downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;

		return CELL_OK;

	case CELL_AUDIO_OUT_SECONDARY:

		return CELL_OK;
	default:
		break;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

error_code cellAudioOutGetNumberOfDevice(u32 audioOut)
{
	cellSysutil.warning("cellAudioOutGetNumberOfDevice(audioOut=%d)", audioOut);

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
		return not_an_error(1);
	case CELL_AUDIO_OUT_SECONDARY:
		return not_an_error(0);
	default:
		break;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

error_code cellAudioOutGetDeviceInfo(u32 audioOut, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo> info)
{
	cellSysutil.todo("cellAudioOutGetDeviceInfo(audioOut=%d, deviceIndex=%d, info=*0x%x)", audioOut, deviceIndex, info);

	if (!info)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	if (deviceIndex)
	{
		return CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND;
	}

	info->portType = CELL_AUDIO_OUT_PORT_HDMI;
	info->availableModeCount = 2;
	info->state = CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE;
	info->latency = 1000;
	info->availableModes[0].type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	info->availableModes[0].channel = CELL_AUDIO_OUT_CHNUM_8;
	info->availableModes[0].fs = CELL_AUDIO_OUT_FS_48KHZ;
	info->availableModes[0].layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;
	info->availableModes[1].type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	info->availableModes[1].channel = CELL_AUDIO_OUT_CHNUM_2;
	info->availableModes[1].fs = CELL_AUDIO_OUT_FS_48KHZ;
	info->availableModes[1].layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH;

	return CELL_OK;
}

error_code cellAudioOutSetCopyControl(u32 audioOut, u32 control)
{
	cellSysutil.warning("cellAudioOutSetCopyControl(audioOut=%d, control=%d)", audioOut, control);

	if (control > CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
	case CELL_AUDIO_OUT_SECONDARY:
		break;
	default:
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
	}

	return CELL_OK;
}

error_code cellAudioOutConfigure2()
{
	cellSysutil.todo("cellAudioOutConfigure2()");
	return CELL_OK;
}

error_code cellAudioOutGetConfiguration2()
{
	cellSysutil.todo("cellAudioOutGetConfiguration2()");
	return CELL_OK;
}

error_code cellAudioOutRegisterCallback()
{
	cellSysutil.todo("cellAudioOutRegisterCallback()");
	return CELL_OK;
}

error_code cellAudioOutUnregisterCallback()
{
	cellSysutil.todo("cellAudioOutUnregisterCallback()");
	return CELL_OK;
}


void cellSysutil_AudioOut_init()
{
	REG_FUNC(cellSysutil, cellAudioOutGetState);
	REG_FUNC(cellSysutil, cellAudioOutConfigure);
	REG_FUNC(cellSysutil, cellAudioOutConfigure2);
	REG_FUNC(cellSysutil, cellAudioOutGetSoundAvailability);
	REG_FUNC(cellSysutil, cellAudioOutGetSoundAvailability2);
	REG_FUNC(cellSysutil, cellAudioOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellAudioOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellAudioOutGetConfiguration);
	REG_FUNC(cellSysutil, cellAudioOutGetConfiguration2);
	REG_FUNC(cellSysutil, cellAudioOutSetCopyControl);
	REG_FUNC(cellSysutil, cellAudioOutRegisterCallback);
	REG_FUNC(cellSysutil, cellAudioOutUnregisterCallback);
}
