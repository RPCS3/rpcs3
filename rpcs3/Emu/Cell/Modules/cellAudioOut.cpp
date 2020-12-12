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

error_code cellAudioOutGetNumberOfDevice(u32 audioOut);

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
	case CELL_AUDIO_OUT_CODING_TYPE_AC3:
	case CELL_AUDIO_OUT_CODING_TYPE_DTS:
		available = 0; break;

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
	case CELL_AUDIO_OUT_CHNUM_2: break;
	case CELL_AUDIO_OUT_CHNUM_4:
	case CELL_AUDIO_OUT_CHNUM_6:
	case CELL_AUDIO_OUT_CHNUM_8:
		available = 0; break;

	default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE;
	}

	switch (type)
	{
	case CELL_AUDIO_OUT_CODING_TYPE_LPCM: break;
	case CELL_AUDIO_OUT_CODING_TYPE_AC3:
	case CELL_AUDIO_OUT_CODING_TYPE_DTS:
		available = 0; break;

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


	const auto num = cellAudioOutGetNumberOfDevice(audioOut);

	if (num < 0)
	{
		return num;
	}

	CellAudioOutState _state;
	std::memset(&_state, 0, sizeof(_state));

	if (deviceIndex >= num + 0u)
	{
		if (audioOut == CELL_AUDIO_OUT_SECONDARY)
		{
			// Error codes are not returned here
			// Random (uninitialized) data from the stack seems to be returned here
			// Although it was constant on my tests so let's write that
			_state.state = 0x10;
			_state.soundMode.layout = 0xD00C1680;
			std::memcpy(state.get_ptr(), &_state, state.size());
			return CELL_OK;
		}

		return CELL_AUDIO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
	case CELL_AUDIO_OUT_SECONDARY:
	{
		_state.state = CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;
		_state.encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		_state.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
		_state.soundMode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		_state.soundMode.channel = CELL_AUDIO_OUT_CHNUM_8;
		_state.soundMode.fs = CELL_AUDIO_OUT_FS_48KHZ;
		_state.soundMode.reserved = 0;
		_state.soundMode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;
		break;
	}

	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	std::memcpy(state.get_ptr(), &_state, state.size());
	return CELL_OK;
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
	{
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
	}

	case CELL_AUDIO_OUT_SECONDARY:
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
	default: break;
	}

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
}

error_code cellAudioOutGetConfiguration(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option)
{
	cellSysutil.warning("cellAudioOutGetConfiguration(audioOut=%d, config=*0x%x, option=*0x%x)", audioOut, config, option);

	if (!config)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	CellAudioOutConfiguration _config;
	std::memset(&_config, 0, sizeof(_config));

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
	{
		_config.channel = CELL_AUDIO_OUT_CHNUM_8;
		_config.encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		_config.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
		break;
	}

	case CELL_AUDIO_OUT_SECONDARY:
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	std::memcpy(config.get_ptr(), &_config, config.size());
	return CELL_OK;
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

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
}

error_code cellAudioOutGetDeviceInfo(u32 audioOut, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo> info)
{
	cellSysutil.todo("cellAudioOutGetDeviceInfo(audioOut=%d, deviceIndex=%d, info=*0x%x)", audioOut, deviceIndex, info);

	if (!info)
	{
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	const auto num = cellAudioOutGetNumberOfDevice(audioOut);

	if (num < 0)
	{
		return num;
	}

	if (deviceIndex >= num + 0u)
	{
		if (audioOut == CELL_AUDIO_OUT_SECONDARY)
		{
			// Error codes are not returned here
			std::memset(info.get_ptr(), 0, info.size());
			return CELL_OK;
		}

		return CELL_AUDIO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	CellAudioOutDeviceInfo _info;
	std::memset(&_info, 0, sizeof(_info));

	_info.portType = CELL_AUDIO_OUT_PORT_HDMI;
	_info.availableModeCount = 2;
	_info.state = CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE;
	_info.latency = 1000;
	_info.availableModes[0].type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	_info.availableModes[0].channel = CELL_AUDIO_OUT_CHNUM_8;
	_info.availableModes[0].fs = CELL_AUDIO_OUT_FS_48KHZ;
	_info.availableModes[0].layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;
	_info.availableModes[1].type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	_info.availableModes[1].channel = CELL_AUDIO_OUT_CHNUM_2;
	_info.availableModes[1].fs = CELL_AUDIO_OUT_FS_48KHZ;
	_info.availableModes[1].layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH;

	std::memcpy(info.get_ptr(), &_info, info.size());
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
		break;
	case CELL_AUDIO_OUT_SECONDARY:
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

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
	REG_FUNC(cellSysutil, cellAudioOutGetSoundAvailability);
	REG_FUNC(cellSysutil, cellAudioOutGetSoundAvailability2);
	REG_FUNC(cellSysutil, cellAudioOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellAudioOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellAudioOutGetConfiguration);
	REG_FUNC(cellSysutil, cellAudioOutSetCopyControl);
	REG_FUNC(cellSysutil, cellAudioOutRegisterCallback);
	REG_FUNC(cellSysutil, cellAudioOutUnregisterCallback);
}
