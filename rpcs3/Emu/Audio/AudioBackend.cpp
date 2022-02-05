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

void AudioBackend::convert_to_s16(u32 cnt, const f32* src, void* dst)
{
	for (usz i = 0; i < cnt; i++)
	{
		static_cast<s16 *>(dst)[i] = static_cast<s16>(std::clamp(src[i] * 32768.5f, -32768.0f, 32767.0f));
	}
}
