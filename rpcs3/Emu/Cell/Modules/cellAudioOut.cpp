#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_rsxaudio.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Loader/PSF.h"

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
	audio_out& primary_output = ::at32(out, CELL_AUDIO_OUT_PRIMARY);
	audio_out& secondary_output = ::at32(out, CELL_AUDIO_OUT_SECONDARY);

	std::vector<CellAudioOutSoundMode>& primary_modes = primary_output.sound_modes;
	std::vector<CellAudioOutSoundMode>& secondary_modes = secondary_output.sound_modes;

	const psf::registry sfo = psf::load_object(Emu.GetSfoDir(true) + "/PARAM.SFO");
	const s32 sound_format = psf::get_integer(sfo, "SOUND_FORMAT", psf::sound_format_flag::lpcm_2); // Default to Linear PCM 2 Ch.

	const bool supports_lpcm_2   = (sound_format & psf::sound_format_flag::lpcm_2);   // Linear PCM 2 Ch.
	const bool supports_lpcm_5_1 = (sound_format & psf::sound_format_flag::lpcm_5_1); // Linear PCM 5.1 Ch.
	const bool supports_lpcm_7_1 = (sound_format & psf::sound_format_flag::lpcm_7_1); // Linear PCM 7.1 Ch.
	const bool supports_ac3      = (sound_format & psf::sound_format_flag::ac3);      // Dolby Digital 5.1 Ch.
	const bool supports_dts      = (sound_format & psf::sound_format_flag::dts);      // DTS 5.1 Ch.

	if (supports_lpcm_2)   cellSysutil.notice("cellAudioOut: found support for Linear PCM 2 Ch.");
	if (supports_lpcm_5_1) cellSysutil.notice("cellAudioOut: found support for Linear PCM 5.1 Ch.");
	if (supports_lpcm_7_1) cellSysutil.notice("cellAudioOut: found support for Linear PCM 7.1 Ch.");
	if (supports_ac3)      cellSysutil.notice("cellAudioOut: found support for Dolby Digital 5.1 Ch.");
	if (supports_dts)      cellSysutil.notice("cellAudioOut: found support for DTS 5.1 Ch.");

	std::array<bool, 2> initial_mode_selected = {};

	const auto add_sound_mode = [&](u32 index, u8 type, u8 channel, u8 fs, u32 layout, bool supported)
	{
		audio_out& output = ::at32(out, index);
		bool& selected = ::at32(initial_mode_selected, index);

		CellAudioOutSoundMode mode{};
		mode.type = type;
		mode.channel = channel;
		mode.fs = fs;
		mode.layout = layout;
		output.sound_modes.push_back(std::move(mode));

		if (!selected && supported)
		{
			// Pre-select the first available sound mode
			output.channels = channel;
			output.encoder  = type;
			output.sound_mode = output.sound_modes.back();

			selected = true;
		}
	};

	// TODO: more formats:
	// - Each LPCM with other sample frequencies (we currently only support 48 kHz)
	// - AAC
	// - Dolby Digital Plus
	// - Dolby TrueHD
	// - DTS-HD High Resolution Audio
	// - DTS-HD Master Audio
	// - ...
	switch (g_cfg.audio.format)
	{
	case audio_format::stereo:
	{
		break; // Already added by default
	}
	case audio_format::surround_7_1:
	{
		// Linear PCM 7.1 Ch. 48 kHz
		add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_8, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy, supports_lpcm_7_1);
		[[fallthrough]]; // Also add all available 5.1 formats in case the game doesn't like 7.1
	}
	case audio_format::surround_5_1:
	{
		// Linear PCM 5.1 Ch. 48 kHz
		add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_lpcm_5_1);

		// Dolby Digital 5.1 Ch.
		add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_AC3, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_ac3);

		// DTS 5.1 Ch.
		add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_DTS, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_dts);
		break;
	}
	case audio_format::automatic: // Automatic based on supported formats
	{
		if (supports_lpcm_7_1) // Linear PCM 7.1 Ch.
		{
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_8, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy, supports_lpcm_7_1);
		}

		if (supports_lpcm_5_1) // Linear PCM 5.1 Ch.
		{
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_lpcm_5_1);
		}

		if (supports_ac3) // Dolby Digital 5.1 Ch.
		{
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_AC3, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_ac3);
		}

		if (supports_dts) // DTS 5.1 Ch.
		{
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_DTS, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_dts);
		}

		break;
	}
	case audio_format::manual: // Manual based on selected formats
	{
		const u32 selected_formats = g_cfg.audio.formats;

		if (selected_formats & static_cast<u32>(audio_format_flag::lpcm_7_1_48khz)) // Linear PCM 7.1 Ch. 48 kHz
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_8, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy, supports_lpcm_7_1);

		if (selected_formats & static_cast<u32>(audio_format_flag::lpcm_5_1_48khz)) // Linear PCM 5.1 Ch. 48 kHz
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_lpcm_5_1);

		if (selected_formats & static_cast<u32>(audio_format_flag::ac3)) // Dolby Digital 5.1 Ch.
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_AC3, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_ac3);

		if (selected_formats & static_cast<u32>(audio_format_flag::dts)) // DTS 5.1 Ch.
			add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_DTS, CELL_AUDIO_OUT_CHNUM_6, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr, supports_dts);

		break;
	}
	}

	// Always add Linear PCM 2 Ch. to the primary output
	add_sound_mode(CELL_AUDIO_OUT_PRIMARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_2, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH, true);

	// The secondary output only supports Linear PCM 2 Ch.
	add_sound_mode(CELL_AUDIO_OUT_SECONDARY, CELL_AUDIO_OUT_CODING_TYPE_LPCM, CELL_AUDIO_OUT_CHNUM_2, CELL_AUDIO_OUT_FS_48KHZ, CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH, true);

	ensure(!primary_modes.empty() && ::at32(initial_mode_selected, CELL_AUDIO_OUT_PRIMARY));
	ensure(!secondary_modes.empty() && ::at32(initial_mode_selected, CELL_AUDIO_OUT_SECONDARY));

	for (const CellAudioOutSoundMode& mode : primary_modes)
	{
		cellSysutil.notice("cellAudioOut: added primary mode: type=%d, channel=%d, fs=%d, layout=%d", mode.type, mode.channel, mode.fs, mode.layout);
	}

	for (const CellAudioOutSoundMode& mode : secondary_modes)
	{
		cellSysutil.notice("cellAudioOut: added secondary mode: type=%d, channel=%d, fs=%d, layout=%d", mode.type, mode.channel, mode.fs, mode.layout);
	}

	cellSysutil.notice("cellAudioOut: initial primary output configuration: channels=%d, encoder=%d, downmixer=%d", primary_output.channels, primary_output.encoder, primary_output.downmixer);
	cellSysutil.notice("cellAudioOut: initial secondary output configuration: channels=%d, encoder=%d, downmixer=%d", secondary_output.channels, secondary_output.encoder, secondary_output.downmixer);
}

audio_out_configuration::audio_out_configuration(utils::serial& ar)
	: audio_out_configuration()
{
	// Load configuartion (ar is reading)
	save(ar);
}

void audio_out_configuration::save(utils::serial& ar)
{
	GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellAudioOut);

	for (auto& state : out)
	{
		ar(state.state, state.channels, state.encoder, state.downmixer, state.copy_control, state.sound_modes, state.sound_mode);
	}
}

std::pair<AudioChannelCnt, AudioChannelCnt> audio_out_configuration::audio_out::get_channel_count_and_downmixer() const
{
	std::pair<AudioChannelCnt, AudioChannelCnt> ret;

	switch (sound_mode.channel)
	{
	case 2: ret.first = AudioChannelCnt::STEREO; break;
	case 6: ret.first = AudioChannelCnt::SURROUND_5_1; break;
	case 8: ret.first = AudioChannelCnt::SURROUND_7_1; break;
	default:
		fmt::throw_exception("Unsupported channel count in cellAudioOut sound_mode: %d", sound_mode.channel);
	}

	switch (downmixer)
	{
	case CELL_AUDIO_OUT_DOWNMIXER_NONE: ret.second = AudioChannelCnt::SURROUND_7_1; break;
	case CELL_AUDIO_OUT_DOWNMIXER_TYPE_A: ret.second = AudioChannelCnt::STEREO; break;
	case CELL_AUDIO_OUT_DOWNMIXER_TYPE_B: ret.second = AudioChannelCnt::SURROUND_5_1; break;
	default:
		fmt::throw_exception("Unsupported downmixer in cellAudioOut config: %d", downmixer);
	}

	return ret;
}

error_code cellAudioOutGetNumberOfDevice(u32 audioOut);

error_code cellAudioOutGetSoundAvailability(u32 audioOut, u32 type, u32 fs, u32 option)
{
	cellSysutil.warning("cellAudioOutGetSoundAvailability(audioOut=%d, type=%d, fs=0x%x, option=%d)", audioOut, type, fs, option);

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY: break;
	// case CELL_AUDIO_OUT_SECONDARY: break; // TODO: enable if we ever actually support peripheral output
	default: return not_an_error(0);
	}

	s32 available = 0;

	// Check if the requested audio parameters are available and find the max supported channel count
	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(cfg.mtx);
	const audio_out_configuration::audio_out& out = ::at32(cfg.out, audioOut);

	for (const CellAudioOutSoundMode& mode : out.sound_modes)
	{
		if (mode.type == type && static_cast<u32>(mode.fs) == fs)
		{
			available = std::max<u32>(available, mode.channel);
		}
	}

	return not_an_error(available);
}

error_code cellAudioOutGetSoundAvailability2(u32 audioOut, u32 type, u32 fs, u32 ch, u32 option)
{
	cellSysutil.warning("cellAudioOutGetSoundAvailability2(audioOut=%d, type=%d, fs=0x%x, ch=%d, option=%d)", audioOut, type, fs, ch, option);

	switch (audioOut)
	{
	case CELL_AUDIO_OUT_PRIMARY: break;
	// case CELL_AUDIO_OUT_SECONDARY: break; // TODO: enable if we ever actually support peripheral output
	default: return not_an_error(0);
	}

	// Check if the requested audio parameters are available
	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(cfg.mtx);
	const audio_out_configuration::audio_out& out = ::at32(cfg.out, audioOut);

	for (const CellAudioOutSoundMode& mode : out.sound_modes)
	{
		if (mode.type == type && static_cast<u32>(mode.fs) == fs && mode.channel == ch)
		{
			return not_an_error(ch);
		}
	}

	return not_an_error(0);
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
		audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
		std::lock_guard lock(cfg.mtx);
		const audio_out_configuration::audio_out& out = ::at32(cfg.out, audioOut);

		_state.state = out.state;
		_state.encoder = out.encoder;
		_state.downMixer = out.downmixer;
		_state.soundMode = out.sound_mode;
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
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT; // The secondary output only supports one format and can't be reconfigured
	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	bool needs_reset = false;

	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	{
		std::lock_guard lock(cfg.mtx);

		audio_out_configuration::audio_out& out = ::at32(cfg.out, audioOut);

		// Apparently the set config does not necessarily have to exist in the list of sound modes.

		if (out.channels != config->channel || out.encoder != config->encoder || out.downmixer != config->downMixer)
		{
			out.channels = config->channel;
			out.encoder = config->encoder;

			if (config->downMixer > CELL_AUDIO_OUT_DOWNMIXER_TYPE_B) // PS3 ignores invalid downMixer values and keeps the previous valid one instead
			{
				cellSysutil.warning("cellAudioOutConfigure: Invalid downmixing mode configured: %d. Keeping old mode: downMixer=%d",
					config->downMixer, out.downmixer);
			}
			else
			{
				out.downmixer = config->downMixer;
			}

			// Try to find the best sound mode for this configuration
			const auto it = std::find_if(out.sound_modes.cbegin(), out.sound_modes.cend(), [&out](const CellAudioOutSoundMode& mode)
				{
					return mode.type == out.encoder && mode.channel == out.channels;
				});

			if (it != out.sound_modes.cend())
			{
				out.sound_mode = *it;
			}
			else
			{
				cellSysutil.warning("cellAudioOutConfigure: Could not find an ideal sound mode for %d channel output. Keeping old mode: channels=%d, encoder=%d, fs=%d",
					config->channel, out.sound_mode.channel, out.sound_mode.type, out.sound_mode.fs);
			}

			needs_reset = true;
		}
	}

	if (needs_reset)
	{
		const auto reset_audio = [audioOut]() -> void
		{
			audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
			{
				std::lock_guard lock(cfg.mtx);
				::at32(cfg.out, audioOut).state = CELL_AUDIO_OUT_OUTPUT_STATE_DISABLED;
			}

			audio::configure_audio(true);
			audio::configure_rsxaudio();

			{
				std::lock_guard lock(cfg.mtx);
				::at32(cfg.out, audioOut).state = CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;
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
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT; // The secondary output only supports one format and can't be reconfigured
	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(cfg.mtx);

	const audio_out_configuration::audio_out& out = ::at32(cfg.out, audioOut);

	// Return the active config.
	CellAudioOutConfiguration _config{};
	_config.channel   = out.channels;
	_config.encoder   = out.encoder;
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
	const audio_out_configuration::audio_out& out = ::at32(cfg.out, audioOut);
	ensure(out.sound_modes.size() <= 16);

	CellAudioOutDeviceInfo _info{};

	_info.portType = CELL_AUDIO_OUT_PORT_HDMI;
	_info.availableModeCount = ::narrow<u8>(out.sound_modes.size());
	_info.state = CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE;
	_info.latency = 13;

	for (usz i = 0; i < out.sound_modes.size(); i++)
	{
		_info.availableModes[i] = ::at32(out.sound_modes, i);
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
		return CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT; // TODO: enable if we ever actually support peripheral output
	default:
		return CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	audio_out_configuration& cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(cfg.mtx);

	::at32(cfg.out, audioOut).copy_control = control;

	return CELL_OK;
}

error_code cellAudioOutRegisterCallback(u32 slot, vm::ptr<CellAudioOutCallback> function, vm::ptr<void> userData)
{
	cellSysutil.todo("cellAudioOutRegisterCallback(slot=%d, function=*0x%x, userData=*0x%x)", slot, function, userData);
	return CELL_OK;
}

error_code cellAudioOutUnregisterCallback(u32 slot)
{
	cellSysutil.todo("cellAudioOutUnregisterCallback(slot=%d)", slot);
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
