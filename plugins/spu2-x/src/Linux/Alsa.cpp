/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
 //  Adapted from ZeroSPU2 code by Zerofrog. Heavily modified by Arcum42.

#include <alsa/asoundlib.h>

#define ALSA_MEM_DEF
#include "Alsa.h"

static snd_pcm_t *handle = NULL;
static snd_pcm_uframes_t buffer_size;

int AlsaSetupSound()
{
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_status_t *status;
	unsigned int pspeed = SAMPLE_RATE;
	int pchannels = 2;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	unsigned int buffer_time = SOUNDSIZE;
	unsigned int period_time= buffer_time / 4;
	int err;
	
	err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if(err < 0) 
	{
		ERROR_LOG("Audio open error: %s\n", snd_strerror(err));
		return -1;
	}
	
	err = snd_pcm_nonblock(handle, 0);
	if(err < 0) 
	{
		ERROR_LOG("Can't set blocking mode: %s\n", snd_strerror(err));
		return -1;
	}
    
	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);
	
	err = snd_pcm_hw_params_any(handle, hwparams);
	if (err < 0) 
	{
		ERROR_LOG("Broken configuration for this PCM: %s\n", snd_strerror(err));
		return -1;
	}
	
	err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) 
	{
		ERROR_LOG("Access type not available: %s\n", snd_strerror(err));
		return -1;
	}

	err = snd_pcm_hw_params_set_format(handle, hwparams, format);
	if (err < 0) 
	{
		ERROR_LOG("Sample format not available: %s\n", snd_strerror(err));
		return -1;
	}
	
	err = snd_pcm_hw_params_set_channels(handle, hwparams, pchannels);
	if (err < 0) 
	{
		ERROR_LOG("Channels count not available: %s\n", snd_strerror(err));
		return -1;
	}
	err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &pspeed, 0);
	if (err < 0) 
	{
		ERROR_LOG("Rate not available: %s\n", snd_strerror(err));
		return -1;
	}
	
	err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, 0);
	if(err < 0) {
		ERROR_LOG("Buffer time error: %s\n", snd_strerror(err));
		return -1;
	}
   
	err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, 0);
	if (err < 0)
	{
		ERROR_LOG("Period time error: %s\n", snd_strerror(err));
		return -1;
	}
	
	err = snd_pcm_hw_params(handle, hwparams);
	if (err < 0) 
	{
		ERROR_LOG("Unable to install hw params: %s\n", snd_strerror(err));
		return -1;
	}
    
	snd_pcm_status_alloca(&status);
	err = snd_pcm_status(handle, status);
	if(err < 0) 
	{
		ERROR_LOG("Unable to get status: %s\n", snd_strerror(err));
		return -1;
	}
    
	buffer_size=snd_pcm_status_get_avail(status);

	return 0;
}

void AlsaRemoveSound()
{
	if(handle != NULL) {
		snd_pcm_drop(handle);
		snd_pcm_close(handle);
		handle = NULL;
	}
}

int AlsaSoundGetBytesBuffered()
{
	int l;
	
	// failed to open?
	if(handle == NULL) return SOUNDSIZE; 
	
	l = snd_pcm_avail_update(handle); 
	
	if (l<0) 
		return 0;
	else if (l<buffer_size/2)                                 // can we write in at least the half of fragments?
		l=SOUNDSIZE;                                   // -> no? wait
	else 
		l=0;                                           // -> else go on
    
	return l;
}

void AlsaSoundFeedVoiceData(unsigned char* pSound,long lBytes)
{
	if (handle == NULL) return;
    
	if (snd_pcm_state(handle) == SND_PCM_STATE_XRUN)
		snd_pcm_prepare(handle);
	snd_pcm_writei(handle,pSound, lBytes/4);
}
