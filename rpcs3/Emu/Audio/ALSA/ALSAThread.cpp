#include "stdafx.h"
#include "Utilities/GSL.h"
#include "Emu/System.h"

#include "ALSAThread.h"

#ifdef HAVE_ALSA

#include <alsa/asoundlib.h>

static thread_local snd_pcm_t* s_tls_handle{nullptr};

static void error(int err, const char* reason)
{
	fprintf(stderr, "ALSA: %s failed: %s\n", reason, snd_strerror(err));
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
	snd_pcm_hw_params_t* hw_params{nullptr};

	auto at_ret = gsl::finally([&]()
	{
		if (hw_params)
		{
			snd_pcm_hw_params_free(hw_params);
		}
	});

	if (!check(snd_pcm_open(&s_tls_handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK), "snd_pcm_open"))
		return;

	if (!check(snd_pcm_hw_params_malloc(&hw_params), "snd_pcm_hw_params_malloc"))
		return;

	if (!check(snd_pcm_hw_params_any(s_tls_handle, hw_params), "snd_pcm_hw_params_any"))
		return;

	if (!check(snd_pcm_hw_params_set_access(s_tls_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED), "snd_pcm_hw_params_set_access"))
		return;

	if (!check(snd_pcm_hw_params_set_format(s_tls_handle, hw_params, g_cfg.audio.convert_to_u16 ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_FLOAT_LE), "snd_pcm_hw_params_set_format"))
		return;

	if (!check(snd_pcm_hw_params_set_rate(s_tls_handle, hw_params, 48000, 0), "snd_pcm_hw_params_set_rate_near"))
		return;

	if (!check(snd_pcm_hw_params_set_channels(s_tls_handle, hw_params, g_cfg.audio.downmix_to_2ch ? 2 : 8), "snd_pcm_hw_params_set_channels"))
		return;

	if (!check(snd_pcm_hw_params_set_buffer_size(s_tls_handle, hw_params, 8 * 256), "snd_pcm_hw_params_set_buffer_size"))
		return;

	if (!check(snd_pcm_hw_params_set_period_size(s_tls_handle, hw_params, 256, 0), "snd_pcm_hw_params_set_period_size"))
		return;

	//if (!check(snd_pcm_hw_params_set_periods(s_tls_handle, hw_params, 2, 0), "snd_pcm_hw_params_set_periods"))
	//  return;

	if (!check(snd_pcm_hw_params(s_tls_handle, hw_params), "snd_pcm_hw_params"))
		return;

	if (!check(snd_pcm_prepare(s_tls_handle), "snd_pcm_prepare"))
		return;
}

ALSAThread::~ALSAThread()
{
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

	int res;

	while (true)
	{
		res = snd_pcm_writei(s_tls_handle, src, size);

		if (res == -EAGAIN)
		{
			continue;
		}
		else if (res == -EPIPE)
		{
			snd_pcm_prepare(s_tls_handle);
		}
		else
		{
			break;
		}
	}

	if (res != size)
	{
		error(res, "snd_pcm_writei");
	}
}

#endif
