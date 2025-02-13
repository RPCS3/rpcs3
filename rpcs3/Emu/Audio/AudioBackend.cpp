#include "stdafx.h"
#include "AudioBackend.h"
#include "Emu/IdManager.h"
#include "Emu//Cell/Modules/cellAudioOut.h"

AudioBackend::AudioBackend() {}

void AudioBackend::SetWriteCallback(std::function<u32(u32 /* byte_cnt */, void* /* buffer */)> cb)
{
	std::lock_guard lock(m_cb_mutex);
	m_write_callback = cb;
}

void AudioBackend::SetStateCallback(std::function<void(AudioStateEvent)> cb)
{
	std::lock_guard lock(m_state_cb_mutex);
	m_state_callback = cb;
}

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
	return m_channels;
}

audio_channel_layout AudioBackend::get_channel_layout() const
{
	return m_layout;
}

bool AudioBackend::get_convert_to_s16() const
{
	return m_sample_size == AudioSampleSize::S16;
}

void AudioBackend::convert_to_s16(u32 cnt, const f32* src, void* dst)
{
	for (u32 i = 0; i < cnt; i++)
	{
		static_cast<s16*>(dst)[i] = static_cast<s16>(std::clamp(src[i] * 32768.5f, -32768.0f, 32767.0f));
	}
}

f32 AudioBackend::apply_volume(const VolumeParam& param, u32 sample_cnt, const f32* src, f32* dst)
{
	ensure(param.ch_cnt > 1 && param.ch_cnt % 2 == 0); // Tends to produce faster code

	const f32 vol_incr = (param.target_volume - param.initial_volume) / (VOLUME_CHANGE_DURATION * param.freq);
	f32 crnt_vol = param.current_volume;
	u32 sample_idx = 0;

	if (vol_incr >= 0)
	{
		for (sample_idx = 0; sample_idx < sample_cnt && crnt_vol != param.target_volume; sample_idx += param.ch_cnt)
		{
			crnt_vol = std::min(param.current_volume + (sample_idx + 1) / param.ch_cnt * vol_incr, param.target_volume);

			for (u32 i = 0; i < param.ch_cnt; i++)
			{
				dst[sample_idx + i] = src[sample_idx + i] * crnt_vol;
			}
		}
	}
	else
	{
		for (sample_idx = 0; sample_idx < sample_cnt && crnt_vol != param.target_volume; sample_idx += param.ch_cnt)
		{
			crnt_vol = std::max(param.current_volume + (sample_idx + 1) / param.ch_cnt * vol_incr, param.target_volume);

			for (u32 i = 0; i < param.ch_cnt; i++)
			{
				dst[sample_idx + i] = src[sample_idx + i] * crnt_vol;
			}
		}
	}

	if (sample_cnt > sample_idx)
	{
		apply_volume_static(param.target_volume, sample_cnt - sample_idx, &src[sample_idx], &dst[sample_idx]);
	}

	return crnt_vol;
}

void AudioBackend::apply_volume_static(f32 vol, u32 sample_cnt, const f32* src, f32* dst)
{
	for (u32 i = 0; i < sample_cnt; i++)
	{
		dst[i] = src[i] * vol;
	}
}

void AudioBackend::normalize(u32 sample_cnt, const f32* src, f32* dst)
{
	for (u32 i = 0; i < sample_cnt; i++)
	{
		dst[i] = std::clamp<f32>(src[i], -1.0f, 1.0f);
	}
}

std::pair<AudioChannelCnt, AudioChannelCnt> AudioBackend::get_channel_count_and_downmixer(u32 device_index)
{
	audio_out_configuration& audio_out_cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(audio_out_cfg.mtx);
	const audio_out_configuration::audio_out& out = ::at32(audio_out_cfg.out, device_index);
	return out.get_channel_count_and_downmixer();
}

AudioChannelCnt AudioBackend::get_max_channel_count(u32 device_index)
{
	audio_out_configuration& audio_out_cfg = g_fxo->get<audio_out_configuration>();
	std::lock_guard lock(audio_out_cfg.mtx);
	const audio_out_configuration::audio_out& out = ::at32(audio_out_cfg.out, device_index);

	AudioChannelCnt count = AudioChannelCnt::STEREO;

	for (const CellAudioOutSoundMode& mode : out.sound_modes)
	{
		switch (mode.channel)
		{
		case 6:
			count = AudioChannelCnt::SURROUND_5_1;
			break;
		case 8:
			return AudioChannelCnt::SURROUND_7_1; // Max possible count. So let's return immediately.
		default:
			break;
		}
	}

	return count;
}

u32 AudioBackend::default_layout_channel_count(audio_channel_layout layout)
{
	switch (layout)
	{
	case audio_channel_layout::mono: return 1;
	case audio_channel_layout::stereo: return 2;
	case audio_channel_layout::stereo_lfe: return 3;
	case audio_channel_layout::quadraphonic: return 4;
	case audio_channel_layout::quadraphonic_lfe: return 5;
	case audio_channel_layout::surround_5_1: return 6;
	case audio_channel_layout::surround_7_1: return 8;
	default: fmt::throw_exception("Unsupported layout %d", static_cast<u32>(layout));
	}
}

u32 AudioBackend::layout_channel_count(u32 channels, audio_channel_layout layout)
{
	if (channels == 0)
	{
		fmt::throw_exception("Unsupported channel count");
	}

	return std::min(channels, default_layout_channel_count(layout));
}

audio_channel_layout AudioBackend::default_layout(u32 channels)
{
	switch (channels)
	{
	case 1: return audio_channel_layout::mono;
	case 2: return audio_channel_layout::stereo;
	case 3: return audio_channel_layout::stereo_lfe;
	case 4: return audio_channel_layout::quadraphonic;
	case 5: return audio_channel_layout::quadraphonic_lfe;
	case 6: return audio_channel_layout::surround_5_1;
	case 7: return audio_channel_layout::surround_5_1;
	case 8: return audio_channel_layout::surround_7_1;
	default: return audio_channel_layout::stereo;
	}
}

void AudioBackend::setup_channel_layout(u32 input_channel_count, u32 output_channel_count, audio_channel_layout layout, logs::channel& log)
{
	const u32 channels = std::min(input_channel_count, output_channel_count);

	if (layout != audio_channel_layout::automatic && output_channel_count > input_channel_count)
	{
		log.warning("Mixing from %d to %d channels is not implemented. Falling back to automatic layout.", input_channel_count, output_channel_count);
		layout = audio_channel_layout::automatic;
	}

	if (layout != audio_channel_layout::automatic && channels < default_layout_channel_count(layout))
	{
		log.warning("Can't use layout %s with %d channels. Falling back to automatic layout.", layout, channels);
		layout = audio_channel_layout::automatic;
	}

	m_layout = layout == audio_channel_layout::automatic ? default_layout(channels) : layout;
	m_channels = layout_channel_count(channels, m_layout);
}
