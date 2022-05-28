#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_rsxaudio.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"

#include "cellAudioOut.h"
#include "cellAudio.h"

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

audio_out_configuration::audio_out_configuration()
{
	CellAudioOutSoundMode mode{};

	mode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	mode.channel = CELL_AUDIO_OUT_CHNUM_8;
	mode.fs = CELL_AUDIO_OUT_FS_48KHZ;
	mode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy;

	out.at(CELL_AUDIO_OUT_PRIMARY).sound_modes.push_back(mode);
	out.at(CELL_AUDIO_OUT_SECONDARY).sound_modes.push_back(mode);

	mode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	mode.channel = CELL_AUDIO_OUT_CHNUM_6;
	mode.fs = CELL_AUDIO_OUT_FS_48KHZ;
	mode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr;

	out.at(CELL_AUDIO_OUT_PRIMARY).sound_modes.push_back(mode);
	out.at(CELL_AUDIO_OUT_SECONDARY).sound_modes.push_back(mode);

	mode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
	mode.channel = CELL_AUDIO_OUT_CHNUM_2;
	mode.fs = CELL_AUDIO_OUT_FS_48KHZ;
	mode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH;

	out.at(CELL_AUDIO_OUT_PRIMARY).sound_modes.push_back(mode);
	out.at(CELL_AUDIO_OUT_SECONDARY).sound_modes.push_back(mode);
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


	const auto num = cellAudioOutGetNumberOfDevice(audioOut);

	if (num < 0)
	{
		return num;
	}

	CellAudioOutState _state{};

	if (deviceIndex >= num + 0u)
	{
		if (audioOut == CELL_AUDIO_OUT_SECONDARY)
		{
			// Error codes are not returned here
			// Random (uninitialized) data from the stack seems to be returned here
			// Although it was constant on my tests so let's write that
			_state.state = 0x10;
			_state.soundMode.layout = 0xD00C1680;
			*state = _state;
			return CELL_OK;
		}

		return CELL_AUDIO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY:
	case CELL_AUDIO_OUT_SECONDARY:
	{
		const AudioChannelCnt channels = AudioBackend::get_channel_count(g_cfg.audio.audio_channel_downmix);

		audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
		std::lock_guard lock(cfg.mtx);
		audio_out_configuration::audio_out& out = cfg.out.at(audioOut);

		const auto it = std::find_if(out.sound_modes.cbegin(), out.sound_modes.cend(), [&channels, &out](const CellAudioOutSoundMode& mode)
		{
			if (mode.type == out.encoder)
			{
				switch (mode.type)
				{
				case CELL_AUDIO_OUT_CODING_TYPE_LPCM:
					return mode.channel == static_cast<u8>(channels);
				case CELL_AUDIO_OUT_CODING_TYPE_AC3:
				case CELL_AUDIO_OUT_CODING_TYPE_DTS:
					return true; // We currently only have one possible sound mode for these types
				default:
					return false;
				}
			}

			return false;
		});

		ensure(it != out.sound_modes.cend());

		_state.state = out.state;
		_state.encoder = out.encoder;
		_state.downMixer = out.downmixer;
		_state.soundMode = *it;
		break;
	}
	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	*state = _state;
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
		break;
	case CELL_AUDIO_OUT_SECONDARY:
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT;
	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	audio_out_configuration::audio_out out_old;
	audio_out_configuration::audio_out out_new;

	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	{
		std::lock_guard lock(cfg.mtx);

		audio_out_configuration::audio_out& out = cfg.out.at(audioOut);

		if (out.sound_modes.cend() == std::find_if(out.sound_modes.cbegin(), out.sound_modes.cend(), [&config](const CellAudioOutSoundMode& mode)
			{
				return mode.channel == config->channel && mode.type == config->encoder && config->downMixer <= CELL_AUDIO_OUT_DOWNMIXER_TYPE_B;
			}))
		{
			return CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION; // TODO: confirm
		}

		out_old = out;

		out.channels = config->channel;
		out.encoder = config->encoder;
		out.downmixer = config->downMixer;

		out_new = out;
	}

	if (g_cfg.audio.audio_channel_downmix == audio_downmix::use_application_settings &&
		std::memcmp(&out_old, &out_new, sizeof(audio_out_configuration::audio_out)) != 0)
	{
		const auto reset_audio = [audioOut]() -> void
		{
			audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
			{
				std::lock_guard lock(cfg.mtx);
				cfg.out.at(audioOut).state = CELL_AUDIO_OUT_OUTPUT_STATE_DISABLED;
			}

			audio::configure_audio();
			audio::configure_rsxaudio();

			{
				std::lock_guard lock(cfg.mtx);
				cfg.out.at(audioOut).state = CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;
			}
		};

		if (waitForEvent)
		{
			reset_audio();
		}
		else
		{
			Emu.CallFromMainThread(reset_audio);
		}
	}

	cellSysutil.notice("cellAudioOutConfigure: channels=%d, encoder=%d, downMixer=%d", config->channel, config->encoder, config->downMixer);

	return CELL_OK;
}

error_code cellAudioOutGetConfiguration(u32 audioOut, vm::ptr<CellAudioOutConfiguration> config, vm::ptr<CellAudioOutOption> option)
{
	cellSysutil.warning("cellAudioOutGetConfiguration(audioOut=%d, config=*0x%x, option=*0x%x)", audioOut, config, option);

	if (!config)
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

	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(cfg.mtx);

	CellAudioOutConfiguration _config{};

	audio_out_configuration::audio_out& out = cfg.out.at(audioOut);
	_config.channel = out.channels;
	_config.encoder = out.encoder;
	_config.downMixer = out.downmixer;

	*config = _config;
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
			*info = {};
			return CELL_OK;
		}

		return CELL_AUDIO_OUT_ERROR_PARAMETER_OUT_OF_RANGE;
	}

	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(cfg.mtx);
	ensure(audioOut < cfg.out.size());
	audio_out_configuration::audio_out& out = cfg.out.at(audioOut);
	ensure(out.sound_modes.size() <= 16);

	CellAudioOutDeviceInfo _info{};

	_info.portType = CELL_AUDIO_OUT_PORT_HDMI;
	_info.availableModeCount = ::narrow<u8>(out.sound_modes.size());
	_info.state = CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE;
	_info.latency = 1000;

	for (usz i = 0; i < out.sound_modes.size(); i++)
	{
		_info.availableModes[i] = out.sound_modes.at(i);
	}

	*info = _info;
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

	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(cfg.mtx);

	cfg.out.at(audioOut).copy_control = control;

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
