#include "stdafx.h"
#include "AudioBackend.h"
#include "Emu/system_config.h"
#include "Emu/Cell/Modules/cellAudioOut.h"

AudioBackend::AudioBackend()
{
	m_convert_to_u16 = static_cast<bool>(g_cfg.audio.convert_to_u16);
	m_sample_size = m_convert_to_u16 ? sizeof(u16) : sizeof(float);
	m_start_threshold = g_cfg.audio.start_threshold;

	const u32 sampling_period_multiplier_u32 = g_cfg.audio.sampling_period_multiplier;

	if (sampling_period_multiplier_u32 == 100)
	{
		m_sampling_rate = DEFAULT_AUDIO_SAMPLING_RATE;
	}
	else
	{
		const f32 sampling_period_multiplier = sampling_period_multiplier_u32 / 100.0f;
		const f32 sampling_rate_multiplier = 1.0f / sampling_period_multiplier;
		m_sampling_rate = static_cast<u32>(f32{ DEFAULT_AUDIO_SAMPLING_RATE } *sampling_rate_multiplier);
	}

	const audio_downmix downmix = g_cfg.audio.audio_channel_downmix.get();

	switch (downmix)
	{
	case audio_downmix::no_downmix:
		m_channels = CELL_AUDIO_OUT_CHNUM_8;
		break;
	case audio_downmix::downmix_to_stereo:
		m_channels = CELL_AUDIO_OUT_CHNUM_2;
		break;
	case audio_downmix::downmix_to_3_1:
		m_channels = CELL_AUDIO_OUT_CHNUM_4;
		break;
	case audio_downmix::downmix_to_5_1:
		m_channels = CELL_AUDIO_OUT_CHNUM_6;
		break;
	case audio_downmix::use_application_settings:
		m_channels = CELL_AUDIO_OUT_CHNUM_2; // TODO
		break;
	default:
		fmt::throw_exception("Unknown audio channel mode %s (%d)", downmix, static_cast<int>(downmix));
	}
}

/*
 * Helper methods
 */
u32 AudioBackend::get_sampling_rate() const
{
	return m_sampling_rate;
}

u32 AudioBackend::get_sample_size() const
{
	return m_sample_size;
}

u32 AudioBackend::get_channels() const
{
	return m_channels;
}

bool AudioBackend::get_convert_to_u16() const
{
	return m_convert_to_u16;
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
