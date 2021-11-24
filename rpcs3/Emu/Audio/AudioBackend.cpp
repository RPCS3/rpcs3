#include "stdafx.h"
#include "AudioBackend.h"
#include "Emu/system_config.h"

AudioBackend::AudioBackend() {}

/*
 * Helper methods
 */
u32 AudioBackend::get_sampling_rate() const
{
	return static_cast<std::underlying_type_t<decltype(m_sampling_rate)>>(m_sampling_rate);
}

u32 AudioBackend::get_sample_size() const
{
	return static_cast<std::underlying_type_t<decltype(m_sample_size)>>(m_sample_size);
}

u32 AudioBackend::get_channels() const
{
	return static_cast<std::underlying_type_t<decltype(m_channels)>>(m_channels);
}

bool AudioBackend::get_convert_to_s16() const
{
	return m_sample_size == AudioSampleSize::S16;
}

bool AudioBackend::has_capability(u32 cap) const
{
	return (cap & GetCapabilities()) == cap;
}

void AudioBackend::dump_capabilities(std::string& out) const
{
	u32 count = 0;
	const u32 capabilities = GetCapabilities();

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
