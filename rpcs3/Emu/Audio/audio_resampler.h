#pragma once

#include "util/types.hpp"
#include "Emu/Audio/AudioBackend.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SoundTouch.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

constexpr f64 RESAMPLER_MAX_FREQ_VAL = 1.0;
constexpr f64 RESAMPLER_MIN_FREQ_VAL = 0.1;

class audio_resampler
{
public:
	audio_resampler();
	~audio_resampler();

	void set_params(AudioChannelCnt ch_cnt, AudioFreq freq);
	f64 set_tempo(f64 new_tempo);

	void put_samples(const f32* buf, u32 sample_cnt);
	std::pair<f32* /* buffer */, u32 /* samples */> get_samples(u32 sample_cnt);

	u32 samples_available() const;
	f64 get_resample_ratio();

	void flush();

private:
	soundtouch::SoundTouch resampler{};
};
