#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellAudioOut.h"

extern Module cellSysutil;

s32 cellAudioOutGetSoundAvailability(u32 audioOut, u32 type, u32 fs, u32 option)
{
	cellSysutil.Warning("cellAudioOutGetSoundAvailability(audioOut=%d, type=%d, fs=0x%x, option=%d)", audioOut, type, fs, option);

	option = 0;

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
	case CELL_AUDIO_OUT_PRIMARY: return available;
	case CELL_AUDIO_OUT_SECONDARY: return 0;
	}

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION;
}

s32 cellAudioOutGetSoundAvailability2(u32 audioOut, u32 type, u32 fs, u32 ch, u32 option)
{
	cellSysutil.Warning("cellAudioOutGetSoundAvailability2(audioOut=%d, type=%d, fs=0x%x, ch=%d, option=%d)", audioOut, type, fs, ch, option);

	option = 0;

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
	case CELL_AUDIO_OUT_PRIMARY: return available;
	case CELL_AUDIO_OUT_SECONDARY: return 0;
	}

	return CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION;
}

s32 cellAudioOutGetState(u32 audioOut, u32 deviceIndex, vm::ptr<CellAudioOutState> state)
{
	cellSysutil.Warning("cellAudioOutGetState(audioOut=0x%x, deviceIndex=0x%x, state=*0x%x)", audioOut, deviceIndex, state);

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

s32 cellAudioOutConfigure(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option, u32 waitForEvent)
{
	cellSysutil.Warning("cellAudioOutConfigure(audioOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", audioOut, config, option, waitForEvent);

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

s32 cellAudioOutGetConfiguration(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option)
{
	cellSysutil.Warning("cellAudioOutGetConfiguration(audioOut=%d, config=*0x%x, option=*0x%x)", audioOut, config, option);

	if (option) *option = {};
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
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

s32 cellAudioOutGetNumberOfDevice(u32 audioOut)
{
	cellSysutil.Warning("cellAudioOutGetNumberOfDevice(audioOut=%d)", audioOut);

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY: return 1;
	case CELL_AUDIO_OUT_SECONDARY: return 0;
	}

	return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
}

s32 cellAudioOutGetDeviceInfo(u32 audioOut, u32 deviceIndex, vm::ptr<CellAudioOutDeviceInfo> info)
{
	cellSysutil.Todo("cellAudioOutGetDeviceInfo(audioOut=%d, deviceIndex=%d, info=*0x%x)", audioOut, deviceIndex, info);

	if (deviceIndex) return CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND;

	info->portType = CELL_AUDIO_OUT_PORT_HDMI;
	info->availableModeCount = 1;
	info->state = CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE;
	info->latency = 1000;
	info->availableModes[0].type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	info->availableModes[0].channel = CELL_AUDIO_OUT_CHNUM_8;
	info->availableModes[0].fs = CELL_AUDIO_OUT_FS_48KHZ;
	info->availableModes[0].layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;

	return CELL_OK;
}

s32 cellAudioOutSetCopyControl(u32 audioOut, u32 control)
{
	cellSysutil.Warning("cellAudioOutSetCopyControl(audioOut=%d, control=%d)", audioOut, control);

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
	case CELL_AUDIO_OUT_SECONDARY:
		break;

	default: return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
	}

	switch (control)
	{
	case CELL_AUDIO_OUT_COPY_CONTROL_COPY_FREE:
	case CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE:
	case CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER:
		break;

	default: return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

s32 cellAudioOutRegisterCallback()
{
	throw EXCEPTION("");
}

s32 cellAudioOutUnregisterCallback()
{
	throw EXCEPTION("");
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
