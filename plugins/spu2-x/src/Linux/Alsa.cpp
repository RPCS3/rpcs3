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

class AlsaMod: public SndOutModule
{
protected:
	static const int PacketsPerBuffer = 1;	// increase this if ALSA can't keep up with 512-sample packets
	static const int MAX_BUFFER_COUNT = 4;
	static const int NumBuffers = 4;		// TODO: this should be configurable someday -- lower values reduce latency.

	snd_pcm_t *handle = NULL;
	snd_pcm_uframes_t buffer_size;
	snd_async_handler_t *pcm_callback = NULL;
	
	uint period_time;
	uint buffer_time;

protected:
	// Invoked by the static ExternalCallback method below.
	void _InternalCallback()
	{
		snd_pcm_sframes_t avail;
		int err;

		avail = snd_pcm_avail_update( handle );
		while (avail >= period_time )
		{
			StereoOut16 buff[PacketsPerBuffer * SndOutPacketSize];
			StereoOut16* p1 = buff;

			for( int p=0; p<PacketsPerBuffer; p++, p1+=SndOutPacketSize )
				SndBuffer::ReadSamples( p1 );

			snd_pcm_writei( handle, buff, period_time );
			avail = snd_pcm_avail_update(handle);
		}
	}

	// Preps and invokes the _InternalCallback above.  This provides a cdecl-compliant
	// entry point for our C++ified object state. :)
	static void ExternalCallback( snd_async_handler_t *pcm_callback )
	{
		AlsaMod *data = snd_async_handler_get_callback_private( pcm_callback );

		jASSUME( data != NULL );
		jASSUME( data->handle == snd_async_handler_get_pcm(pcm_callback) );

		// Not sure if we just need an assert, or something like this:
		//if( data->handle != snd_async_handler_get_pcm(pcm_callback) ) return;
		
		data->_InternalCallback();
	}

public:

	s32 Init()
	{
		snd_pcm_hw_params_t *hwparams;
		snd_pcm_sw_params_t *swparams;
		snd_pcm_status_t *status;
		int pchannels = 2;
		snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

		// buffer time and period time are in microseconds...
		// (don't simplify the equation below -- it'll just cause integer rounding errors.
		period_time = (SndOutPacketSize*1000) / (SampleRate / 1000);
		buffer_time = period_time * NumBuffers;

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

		// Bind our asynchronous callback magic:
		
		snd_async_add_pcm_handler( &pcm_callback, handle, ExternalCallback, this );

		snd_pcm_start( handle );
		// Diagnostic code:
		//buffer_size = snd_pcm_status_get_avail(status);

		return 0;
	}

	void Close()
	{
		if(handle == NULL) return;

		snd_pcm_drop(handle);
		snd_pcm_close(handle);
		handle = NULL;
	}

	virtual void Configure(HWND parent)
	{
	}

	virtual bool Is51Out() const { return false; }
	{
	}
	
	s32  Test() const
	{
		return 0;
	}

	int GetEmptySampleCount() const
	{
		if(handle == NULL) return 0;

		// Returns the amount of free buffer space, in samples.
		uint l = snd_pcm_avail_update(handle); 
		if( l < 0 ) return 0;
		return (l / 1000) * (SampleRate / 1000);
	}

	const wchar_t* GetIdent() const
	{
		return L"Alsa";
	}

	const wchar_t* GetLongName() const
	{
		return L"Alsa ('tis all ya get)";
	}

	void ReadSettings()
	{
	}

	void WriteSettings() const
	{
	}
} static Alsa;

SndOutModule *AlsaOut = &Alsa;
