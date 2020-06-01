#include "stdafx.h"
#include "AudioBackend.h"
#include "Emu/system_config.h"

/*
 * Helper methods
 */
u32 AudioBackend::get_sampling_rate()
{
	const u32 sampling_period_multiplier_u32 = g_cfg.audio.sampling_period_multiplier;

	if (sampling_period_multiplier_u32 == 100)
		return DEFAULT_AUDIO_SAMPLING_RATE;

	const f32 sampling_period_multiplier = sampling_period_multiplier_u32 / 100.0f;
	const f32 sampling_rate_multiplier = 1.0f / sampling_period_multiplier;
	return static_cast<u32>(f32{DEFAULT_AUDIO_SAMPLING_RATE} * sampling_rate_multiplier);
}

u32 AudioBackend::get_sample_size()
{
	return g_cfg.audio.convert_to_u16 ? sizeof(u16) : sizeof(float);
}

u32 AudioBackend::get_channels()
{
	const audio_channels channels = g_cfg.audio.audio_channel_downmix.get();

	switch (channels)
	{
	case audio_channels::use_application_settings:
		return 2; // TODO
	case audio_channels::downmix_to_stereo:
		return 2;
	case audio_channels::downmix_to_5_1:
		return 6;
	case audio_channels::surround_7_1:
		return 8;
	default:
		fmt::throw_exception("Unknown audio channel mode %s (%d)", channels, static_cast<int>(channels));
	}
}

bool AudioBackend::has_capability(u32 cap) const
{
	return (cap & GetCapabilities()) == cap;
}

void AudioBackend::dump_capabilities(std::string& out) const
{
	u32 count = 0;
	u32 capabilities = GetCapabilities();

	if (capabilities & PLAY_PAUSE_FLUSH)
	{
		fmt::append(out, "PLAY_PAUSE_FLUSH");
		count++;
	}

	if (capabilities & IS_PLAYING)
	{
		fmt::append(out, "%sIS_PLAYING", count > 0 ? " | " : "");
		count++;
	}

	if (capabilities & GET_NUM_ENQUEUED_SAMPLES)
	{
		fmt::append(out, "%sGET_NUM_ENQUEUED_SAMPLES", count > 0 ? " | " : "");
		count++;
	}

	if (capabilities & SET_FREQUENCY_RATIO)
	{
		fmt::append(out, "%sSET_FREQUENCY_RATIO", count > 0 ? " | " : "");
		count++;
	}

	if (count == 0)
	{
		fmt::append(out, "NONE");
	}
}
