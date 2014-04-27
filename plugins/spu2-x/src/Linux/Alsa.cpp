/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

 //  Adapted from ZeroSPU2 code by Zerofrog. Heavily modified by Arcum42.

#include <alsa/asoundlib.h>

#include "Global.h"
#include "Alsa.h"
#include "SndOut.h"
	
// Does not work, except as effectively a null plugin.
class AlsaMod: public SndOutModule
{
protected:
	static const int PacketsPerBuffer = 1;	// increase this if ALSA can't keep up with 512-sample packets
	static const int MAX_BUFFER_COUNT = 4;
	static const int NumBuffers = 4;		// TODO: this should be configurable someday -- lower values reduce latency.
	unsigned int pspeed;

	snd_pcm_t *handle;
	snd_pcm_uframes_t buffer_size;
	snd_async_handler_t *pcm_callback;

	uint period_time;
	uint buffer_time;

protected:
	// Invoked by the static ExternalCallback method below.
	void _InternalCallback()
	{
		snd_pcm_sframes_t avail;
		fprintf(stderr,"* SPU2-X:Iz in your internal callback.\n");

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
	static void ExternalCallback( snd_async_handler_t *pcm_call)
	{
		fprintf(stderr,"* SPU2-X:Iz in your external callback.\n");
		AlsaMod *data = (AlsaMod*)snd_async_handler_get_callback_private( pcm_call );

		jASSUME( data != NULL );
		//jASSUME( data->handle == snd_async_handler_get_pcm(pcm_call) );

		// Not sure if we just need an assert, or something like this:
		if (data->handle != snd_async_handler_get_pcm(pcm_call)) 
		{	
			fprintf(stderr,"* SPU2-X: Failed to handle sound.\n");
			return;
		}

		data->_InternalCallback();
	}

public:

	s32 Init()
	{
		//fprintf(stderr,"* SPU2-X: Initing Alsa\n");
		snd_pcm_hw_params_t *hwparams;
		snd_pcm_sw_params_t *swparams;
		snd_pcm_status_t *status;
		int pchannels = 2;
		snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

		handle = NULL;
		pcm_callback = NULL;
		pspeed = SAMPLE_RATE;

		// buffer time and period time are in microseconds...
		// (don't simplify the equation below -- it'll just cause integer rounding errors.
		period_time = (SndOutPacketSize*1000) / (SampleRate / 1000);
		buffer_time = period_time * NumBuffers;

		int err;

		err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC /*| SND_PCM_NONBLOCK*/);
		if(err < 0)
		{
			fprintf(stderr,"Audio open error: %s\n", snd_strerror(err));
			return -1;
		}

		err = snd_pcm_nonblock(handle, 0);
		if(err < 0)
		{
			fprintf(stderr,"Can't set blocking mode: %s\n", snd_strerror(err));
			return -1;
		}

		snd_pcm_hw_params_alloca(&hwparams);
		snd_pcm_sw_params_alloca(&swparams);

		err = snd_pcm_hw_params_any(handle, hwparams);
		if (err < 0)
		{
			fprintf(stderr,"Broken configuration for this PCM: %s\n", snd_strerror(err));
			return -1;
		}

		err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
		if (err < 0)
		{
			fprintf(stderr,"Access type not available: %s\n", snd_strerror(err));
			return -1;
		}

		err = snd_pcm_hw_params_set_format(handle, hwparams, format);
		if (err < 0)
		{
			fprintf(stderr,"Sample format not available: %s\n", snd_strerror(err));
			return -1;
		}

		err = snd_pcm_hw_params_set_channels(handle, hwparams, pchannels);
		if (err < 0)
		{
			fprintf(stderr,"Channels count not available: %s\n", snd_strerror(err));
			return -1;
		}
		err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &pspeed, 0);
		if (err < 0)
		{
			fprintf(stderr,"Rate not available: %s\n", snd_strerror(err));
			return -1;
		}

		err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, 0);
		if(err < 0) {
			fprintf(stderr,"Buffer time error: %s\n", snd_strerror(err));
			return -1;
		}

		err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, 0);
		if (err < 0)
		{
			fprintf(stderr,"Period time error: %s\n", snd_strerror(err));
			return -1;
		}

		err = snd_pcm_hw_params(handle, hwparams);
		if (err < 0)
		{
			fprintf(stderr,"Unable to install hw params: %s\n", snd_strerror(err));
			return -1;
		}

		snd_pcm_status_alloca(&status);
		err = snd_pcm_status(handle, status);
		if(err < 0)
		{
			fprintf(stderr,"Unable to get status: %s\n", snd_strerror(err));
			return -1;
		}

		// Bind our asynchronous callback magic:

		if (handle == NULL) fprintf(stderr, "No handle.");
		
		//fprintf(stderr,"* SPU2-X:Iz setting your internal callback.\n");
		// The external handler never seems to get called after this.
		snd_async_add_pcm_handler( &pcm_callback, handle, ExternalCallback, this );
		err = snd_pcm_start( handle );
		if(err < 0)
		{
			fprintf(stderr,"Pcm start failed: %s\n", snd_strerror(err));
			return -1;
		}
		// Diagnostic code:
		//buffer_size = snd_pcm_status_get_avail(status);
		
		//fprintf(stderr,"All set up.\n");
		return 0;
	}

	void Close()
	{
		//fprintf(stderr,"* SPU2-X: Closing Alsa\n");
		if(handle == NULL) return;

		snd_pcm_drop(handle);
		snd_pcm_close(handle);
		handle = NULL;
	}

	virtual void Configure(uptr parent)
	{
	}

	virtual bool Is51Out() const { return false; }

	s32  Test() const
	{
		return 0;
	}

	int GetEmptySampleCount()
	{
		if(handle == NULL) 
		{
			fprintf(stderr,"Handle is NULL!\n");
			return 0;
		}

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
		return L"Alsa";
	}

	void ReadSettings()
	{
	}

	void SetApiSettings(wxString api)
	{
	}

	void WriteSettings() const
	{
	}
} static Alsa;

SndOutModule *AlsaOut = &Alsa;
