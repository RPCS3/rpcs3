#include "stdafx.h"
#include "Emu/System.h"

#include "ALSAThread.h"

#ifdef HAVE_ALSA

#include <alsa/asoundlib.h>

static thread_local snd_pcm_t* s_tls_handle{nullptr};
static thread_local snd_pcm_hw_params_t* s_tls_hw_params{nullptr};
static thread_local snd_pcm_sw_params_t* s_tls_sw_params{nullptr};

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

ALSAThread::ALSAThread()
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

	uint rate = 48000;
	if (!check(snd_pcm_hw_params_set_rate_near(s_tls_handle, s_tls_hw_params, &rate, nullptr), "snd_pcm_hw_params_set_rate_near"))
		return;

	if (!check(snd_pcm_hw_params_set_channels(s_tls_handle, s_tls_hw_params, g_cfg.audio.downmix_to_2ch ? 2 : 8), "snd_pcm_hw_params_set_channels"))
		return;

	//uint period = 5333;
	//if (!check(snd_pcm_hw_params_set_period_time_near(s_tls_handle, s_tls_hw_params, &period, nullptr), "snd_pcm_hw_params_set_period_time_near"))
	//	return;

	//if (!check(snd_pcm_hw_params_set_periods(s_tls_handle, s_tls_hw_params, 4, 0), "snd_pcm_hw_params_set_periods"))
	//	return;

	snd_pcm_uframes_t bufsize_frames = g_cfg.audio.frames * 256;
	snd_pcm_uframes_t period_frames = 256;

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

	if (!check(snd_pcm_sw_params_set_start_threshold(s_tls_handle, s_tls_sw_params, period_frames), "snd_pcm_sw_params_set_start_threshold"))
		return;

	if (!check(snd_pcm_sw_params_set_stop_threshold(s_tls_handle, s_tls_sw_params, bufsize_frames), "snd_pcm_sw_params_set_stop_threshold"))
		return;

	if (!check(snd_pcm_sw_params(s_tls_handle, s_tls_sw_params), "snd_pcm_sw_params"))
		return;

	if (!check(snd_pcm_prepare(s_tls_handle), "snd_pcm_prepare"))
		return;

	LOG_NOTICE(GENERAL, "ALSA: bufsize_frames=%u, period_frames=%u", bufsize_frames, period_frames);
}

ALSAThread::~ALSAThread()
{
	if (s_tls_sw_params)
	{
		snd_pcm_sw_params_free(s_tls_sw_params);
	}

	if (s_tls_hw_params)
	{
		snd_pcm_hw_params_free(s_tls_hw_params);
	}

	if (s_tls_handle)
	{
		snd_pcm_close(s_tls_handle);
	}
}

void ALSAThread::Play()
{
}

void ALSAThread::Close()
{
}

void ALSAThread::Stop()
{
}

void ALSAThread::Open(const void* src, int size)
{
	AddData(src, size);
}

void ALSAThread::AddData(const void* src, int size)
{
	size /= g_cfg.audio.convert_to_u16 ? 2 : 4;
	size /= g_cfg.audio.downmix_to_2ch ? 2 : 8;

	int res = snd_pcm_writei(s_tls_handle, src, size);

	if (res == -EAGAIN)
	{
		LOG_WARNING(GENERAL, "ALSA: EAGAIN");
		return;
	}

	if (res < 0)
	{
		res = snd_pcm_recover(s_tls_handle, res, 0);

		if (res < 0)
		{
			LOG_WARNING(GENERAL, "ALSA: failed to recover (%d)", res);
			return;
		}

		res = snd_pcm_writei(s_tls_handle, src, size);
	}

	if (res != size)
	{
		LOG_WARNING(GENERAL, "ALSA: error (%d)", res);
	}
}

#endif
