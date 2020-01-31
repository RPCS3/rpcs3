#ifndef HAVE_ALSA
#error "ALSA support disabled but still being built."
#endif

#include "stdafx.h"
#include "Emu/System.h"

#include "ALSABackend.h"

LOG_CHANNEL(ALSA);

static void error(int err, const char* reason)
{
	ALSA.error("ALSA: %s failed: %s\n", reason, snd_strerror(err));
}

static bool check(int err, const char* reason)
{
	if (err < 0)
	{
		error(err, reason);
		return false;
	}

	return true;
}

ALSABackend::ALSABackend()
{
}

ALSABackend::~ALSABackend()
{
	if (tls_sw_params || tls_hw_params || tls_handle)
	{
		Close();
	}
}

void ALSABackend::Open(u32 num_buffers)
{
	if (!check(snd_pcm_open(&tls_handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK), "snd_pcm_open"))
		return;

	if (!check(snd_pcm_hw_params_malloc(&tls_hw_params), "snd_pcm_hw_params_malloc"))
		return;

	if (!check(snd_pcm_hw_params_any(tls_handle, tls_hw_params), "snd_pcm_hw_params_any"))
		return;

	if (!check(snd_pcm_hw_params_set_access(tls_handle, tls_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED), "snd_pcm_hw_params_set_access"))
		return;

	if (!check(snd_pcm_hw_params_set_format(tls_handle, tls_hw_params, g_cfg.audio.convert_to_u16 ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_FLOAT_LE), "snd_pcm_hw_params_set_format"))
		return;

	uint rate = get_sampling_rate();
	if (!check(snd_pcm_hw_params_set_rate_near(tls_handle, tls_hw_params, &rate, nullptr), "snd_pcm_hw_params_set_rate_near"))
		return;

	if (!check(snd_pcm_hw_params_set_channels(tls_handle, tls_hw_params, get_channels()), "snd_pcm_hw_params_set_channels"))
		return;

	//uint period = 5333;
	//if (!check(snd_pcm_hw_params_set_period_time_near(tls_handle, tls_hw_params, &period, nullptr), "snd_pcm_hw_params_set_period_time_near"))
	//	return;

	//if (!check(snd_pcm_hw_params_set_periods(tls_handle, tls_hw_params, 4, 0), "snd_pcm_hw_params_set_periods"))
	//	return;

	snd_pcm_uframes_t bufsize_frames = num_buffers * AUDIO_BUFFER_SAMPLES;
	snd_pcm_uframes_t period_frames = AUDIO_BUFFER_SAMPLES;

	if (!check(snd_pcm_hw_params_set_buffer_size_near(tls_handle, tls_hw_params, &bufsize_frames), "snd_pcm_hw_params_set_buffer_size_near"))
		return;

	if (!check(snd_pcm_hw_params_set_period_size_near(tls_handle, tls_hw_params, &period_frames, 0), "snd_pcm_hw_params_set_period_size"))
		return;

	if (!check(snd_pcm_hw_params(tls_handle, tls_hw_params), "snd_pcm_hw_params"))
		return;

	if (!check(snd_pcm_hw_params_get_buffer_size(tls_hw_params, &bufsize_frames), "snd_pcm_hw_params_get_buffer_size"))
		return;

	if (!check(snd_pcm_hw_params_get_period_size(tls_hw_params, &period_frames, nullptr), "snd_pcm_hw_params_get_period_size"))
		return;

	if (!check(snd_pcm_sw_params_malloc(&tls_sw_params), "snd_pcm_sw_params_malloc"))
		return;

	if (!check(snd_pcm_sw_params_current(tls_handle, tls_sw_params), "snd_pcm_sw_params_current"))
		return;

	period_frames *= g_cfg.audio.startt;

	if (!check(snd_pcm_sw_params_set_start_threshold(tls_handle, tls_sw_params, period_frames + 1), "snd_pcm_sw_params_set_start_threshold"))
		return;

	if (!check(snd_pcm_sw_params_set_stop_threshold(tls_handle, tls_sw_params, bufsize_frames), "snd_pcm_sw_params_set_stop_threshold"))
		return;

	if (!check(snd_pcm_sw_params(tls_handle, tls_sw_params), "snd_pcm_sw_params"))
		return;

	if (!check(snd_pcm_prepare(tls_handle), "snd_pcm_prepare"))
		return;

	ALSA.notice("bufsize_frames=%u, period_frames=%u", bufsize_frames, period_frames);
}

void ALSABackend::Close()
{
	if (tls_sw_params)
	{
		snd_pcm_sw_params_free(tls_sw_params);
		tls_sw_params = nullptr;
	}

	if (tls_hw_params)
	{
		snd_pcm_hw_params_free(tls_hw_params);
		tls_hw_params = nullptr;
	}

	if (tls_handle)
	{
		snd_pcm_close(tls_handle);
		tls_handle = nullptr;
	}
}

bool ALSABackend::AddData(const void* src, u32 num_samples)
{
	u32 num_frames = num_samples / get_channels();

	int res = snd_pcm_writei(tls_handle, src, num_frames);

	if (res == -EAGAIN)
	{
		ALSA.warning("EAGAIN");
		return false;
	}

	if (res < 0)
	{
		res = snd_pcm_recover(tls_handle, res, 0);

		if (res < 0)
		{
			ALSA.warning("Failed to recover (%d)", res);
			return false;
		}

		return false;
	}

	if (res != num_frames)
	{
		ALSA.warning("Error (%d)", res);
		return false;
	}

	return true;
}
