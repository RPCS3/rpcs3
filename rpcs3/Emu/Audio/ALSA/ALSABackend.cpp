#include "stdafx.h"
#include "Emu/System.h"

#include "ALSABackend.h"

#ifdef HAVE_ALSA


static void error(int err, const char* reason)
{
	LOG_ERROR(GENERAL, "ALSA: %s failed: %s\n", reason, snd_strerror(err));
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
	if (s_tls_sw_params || s_tls_hw_params || s_tls_handle)
	{
		Close();
	}
}

void ALSABackend::Open(u32 num_buffers)
{
	if (!check(snd_pcm_open(&s_tls_handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK), "snd_pcm_open"))
		return;

	if (!check(snd_pcm_hw_params_malloc(&s_tls_hw_params), "snd_pcm_hw_params_malloc"))
		return;

	if (!check(snd_pcm_hw_params_any(s_tls_handle, s_tls_hw_params), "snd_pcm_hw_params_any"))
		return;

	if (!check(snd_pcm_hw_params_set_access(s_tls_handle, s_tls_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED), "snd_pcm_hw_params_set_access"))
		return;

	if (!check(snd_pcm_hw_params_set_format(s_tls_handle, s_tls_hw_params, g_cfg.audio.convert_to_u16 ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_FLOAT_LE), "snd_pcm_hw_params_set_format"))
		return;

	uint rate = get_sampling_rate();
	if (!check(snd_pcm_hw_params_set_rate_near(s_tls_handle, s_tls_hw_params, &rate, nullptr), "snd_pcm_hw_params_set_rate_near"))
		return;

	if (!check(snd_pcm_hw_params_set_channels(s_tls_handle, s_tls_hw_params, get_channels()), "snd_pcm_hw_params_set_channels"))
		return;

	//uint period = 5333;
	//if (!check(snd_pcm_hw_params_set_period_time_near(s_tls_handle, s_tls_hw_params, &period, nullptr), "snd_pcm_hw_params_set_period_time_near"))
	//	return;

	//if (!check(snd_pcm_hw_params_set_periods(s_tls_handle, s_tls_hw_params, 4, 0), "snd_pcm_hw_params_set_periods"))
	//	return;

	snd_pcm_uframes_t bufsize_frames = num_buffers * AUDIO_BUFFER_SAMPLES;
	snd_pcm_uframes_t period_frames = AUDIO_BUFFER_SAMPLES;

	if (!check(snd_pcm_hw_params_set_buffer_size_near(s_tls_handle, s_tls_hw_params, &bufsize_frames), "snd_pcm_hw_params_set_buffer_size_near"))
		return;

	if (!check(snd_pcm_hw_params_set_period_size_near(s_tls_handle, s_tls_hw_params, &period_frames, 0), "snd_pcm_hw_params_set_period_size"))
		return;

	if (!check(snd_pcm_hw_params(s_tls_handle, s_tls_hw_params), "snd_pcm_hw_params"))
		return;

	if (!check(snd_pcm_hw_params_get_buffer_size(s_tls_hw_params, &bufsize_frames), "snd_pcm_hw_params_get_buffer_size"))
		return;

	if (!check(snd_pcm_hw_params_get_period_size(s_tls_hw_params, &period_frames, nullptr), "snd_pcm_hw_params_get_period_size"))
		return;

	if (!check(snd_pcm_sw_params_malloc(&s_tls_sw_params), "snd_pcm_sw_params_malloc"))
		return;

	if (!check(snd_pcm_sw_params_current(s_tls_handle, s_tls_sw_params), "snd_pcm_sw_params_current"))
		return;

	period_frames *= g_cfg.audio.startt;

	if (!check(snd_pcm_sw_params_set_start_threshold(s_tls_handle, s_tls_sw_params, period_frames + 1), "snd_pcm_sw_params_set_start_threshold"))
		return;

	if (!check(snd_pcm_sw_params_set_stop_threshold(s_tls_handle, s_tls_sw_params, bufsize_frames), "snd_pcm_sw_params_set_stop_threshold"))
		return;

	if (!check(snd_pcm_sw_params(s_tls_handle, s_tls_sw_params), "snd_pcm_sw_params"))
		return;

	if (!check(snd_pcm_prepare(s_tls_handle), "snd_pcm_prepare"))
		return;

	LOG_NOTICE(GENERAL, "ALSA: bufsize_frames=%u, period_frames=%u", bufsize_frames, period_frames);
}

void ALSABackend::Close()
{
	if (s_tls_sw_params)
	{
		snd_pcm_sw_params_free(s_tls_sw_params);
		s_tls_sw_params = nullptr;
	}

	if (s_tls_hw_params)
	{
		snd_pcm_hw_params_free(s_tls_hw_params);
		s_tls_hw_params = nullptr;
	}

	if (s_tls_handle)
	{
		snd_pcm_close(s_tls_handle);
		s_tls_handle = nullptr;
	}
}

bool ALSABackend::AddData(const void* src, u32 num_samples)
{
	u32 num_frames = num_samples / get_channels();

	int res = snd_pcm_writei(s_tls_handle, src, num_frames);

	if (res == -EAGAIN)
	{
		LOG_WARNING(GENERAL, "ALSA: EAGAIN");
		return false;
	}

	if (res < 0)
	{
		res = snd_pcm_recover(s_tls_handle, res, 0);

		if (res < 0)
		{
			LOG_WARNING(GENERAL, "ALSA: failed to recover (%d)", res);
			return false;
		}

		res = snd_pcm_writei(s_tls_handle, src, num_frames);
	}

	if (res != num_frames)
	{
		LOG_WARNING(GENERAL, "ALSA: error (%d)", res);
		return false;
	}
	
	return true;
}

#endif
