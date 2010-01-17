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

#include "Global.h"

#define _WIN32_DCOM
#include "Dialogs.h"

#include "portaudio/include/portaudio.h"

class Portaudio : public SndOutModule
{
private:
	static const uint MAX_BUFFER_COUNT = 8;
	static const int PacketsPerBuffer = 1;
	static const int BufferSize = SndOutPacketSize * PacketsPerBuffer;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Configuration Vars (unused still)

	int m_ApiId;
	wstring m_Device;

	bool m_UseHardware;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Instance vars

	int writtenSoFar;
	int writtenLastTime;
	int availableLastTime;
	
	bool started;
	PaStream* stream;

	static int PaCallback( const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData )
	{
		return PA.ActualPaCallback(inputBuffer,outputBuffer,framesPerBuffer,timeInfo,statusFlags,userData);
	}


	int ActualPaCallback( const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData )
	{
		StereoOut32* p1 = (StereoOut32*)outputBuffer;

		int packets = framesPerBuffer / SndOutPacketSize;

		for(int p=0; p<packets; p++, p1+=SndOutPacketSize )
			SndBuffer::ReadSamples( p1 );

		writtenSoFar += packets * SndOutPacketSize;

		return 0;
	}

public:
	Portaudio()
	{
		m_ApiId=-1;
	}

	s32 Init()
	{
		started=false;
		stream=NULL;

		ReadSettings();

		PaError err = Pa_Initialize();
		if( err != paNoError )
		{
			fprintf(stderr," * SPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			return -1;
		}
		started=true;

		int deviceIndex = -1;

		fprintf(stderr," * SPU2: Enumerating PortAudio devices:");
		for(int i=0;i<Pa_GetDeviceCount();i++)
		{
			const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
			
			const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(info->hostApi);

			fprintf(stderr," *** Device %d: '%s' (%s)", i, info->name, apiinfo->name);

			if(apiinfo->type == m_ApiId)
			{
#ifdef __WIN32__
				static wchar_t buffer [1000];
				MultiByteToWideChar(CP_UTF8,0,info->name,strlen(info->name),buffer,999);
				buffer[999]=0;
#else
#	error TODO
#endif

				if(m_Device == buffer)
				{
					deviceIndex = i;
					fprintf(stderr," (selected)");
				}

			}
			fprintf(stderr,"\n");
		}

		if(deviceIndex<0 && m_ApiId>=0)
		{
			for(int i=0;i<Pa_GetHostApiCount();i++)
			{
				const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(i);
				if(apiinfo->type == m_ApiId)
				{
					deviceIndex = apiinfo->defaultOutputDevice;
				}
			}
		}

		if(deviceIndex>=0)
		{
			PaStreamParameters outParams = {

			//	PaDeviceIndex device;
			//	int channelCount;
			//	PaSampleFormat sampleFormat;
			//	PaTime suggestedLatency;
			//	void *hostApiSpecificStreamInfo;
				deviceIndex,
				2,
				paInt32,
				0, //?
				NULL

			};

			err = Pa_OpenStream(&stream,
				NULL, &outParams, SampleRate,
				SndOutPacketSize,
				paNoFlag, PaCallback, NULL);
		}
		else
		{
			err = Pa_OpenDefaultStream( &stream,
				0, 2, paInt32, 48000,
				SndOutPacketSize,
				PaCallback, NULL );
		}
		if( err != paNoError )
		{
			fprintf(stderr," * SPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			Pa_Terminate();
			return -1;
		}

		err = Pa_StartStream( stream );
		if( err != paNoError )
		{
			fprintf(stderr," * SPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			Pa_CloseStream(stream);
			stream=NULL;
			Pa_Terminate();
			return -1;
		}

		return 0;
	}

	void Close()
	{
		PaError err;
		if(started)
		{
			if(stream)
			{
				if(Pa_IsStreamActive(stream))
				{
					err = Pa_StopStream(stream);
					if( err != paNoError )
						fprintf(stderr," * SPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );
				}

				err = Pa_CloseStream(stream);
				if( err != paNoError )
					fprintf(stderr," * SPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );

				stream=NULL;
			}

			PaError err = Pa_Terminate();
			if( err != paNoError )
				fprintf(stderr," * SPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );

			started=false;
		}
	}

	virtual void Configure(uptr parent)
	{
	}

	virtual bool Is51Out() const { return false; }

	s32 Test() const
	{
		return 0;
	}

	int GetEmptySampleCount()
	{
		long availableNow = Pa_GetStreamWriteAvailable(stream);

		int playedSinceLastTime = (writtenSoFar - writtenLastTime) + (availableNow - availableLastTime);
		writtenLastTime = writtenSoFar;
		availableLastTime = availableNow;

		return playedSinceLastTime;
	}

	const wchar_t* GetIdent() const
	{
		return L"portaudio";
	}

	const wchar_t* GetLongName() const
	{
		return L"Portaudio (crossplatform)";
	}
	
	void ReadSettings()
	{
		wstring api=L"EMPTYEMPTYEMPTY";
		m_Device = L"EMPTYEMPTYEMPTY";
		CfgReadStr( L"PORTAUDIO", L"HostApi", api, 254, L"Unknown" );
		CfgReadStr( L"PORTAUDIO", L"Device", m_Device, 254, L"default" );

		m_ApiId = -1;
		if(api == L"InDevelopment") m_ApiId = paInDevelopment; /* use while developing support for a new host API */
		if(api == L"DirectSound")	m_ApiId = paDirectSound;
		if(api == L"MME")			m_ApiId = paMME;
		if(api == L"ASIO")			m_ApiId = paASIO;
		if(api == L"SoundManager")	m_ApiId = paSoundManager;
		if(api == L"CoreAudio")		m_ApiId = paCoreAudio;
		if(api == L"OSS")			m_ApiId = paOSS;
		if(api == L"ALSA")			m_ApiId = paALSA;
		if(api == L"AL")			m_ApiId = paAL;
		if(api == L"BeOS")			m_ApiId = paBeOS;
		if(api == L"WDMKS")			m_ApiId = paWDMKS;
		if(api == L"JACK")			m_ApiId = paJACK;
		if(api == L"WASAPI")		m_ApiId = paWASAPI;
		if(api == L"AudioScienceHPI") m_ApiId = paAudioScienceHPI;

	}

	void WriteSettings() const
	{
		wstring api;
		switch(m_ApiId)
		{
		case paInDevelopment:	api = L"InDevelopment"; break; /* use while developing support for a new host API */
		case paDirectSound:		api = L"DirectSound"; break;
		case paMME:				api = L"MME"; break;
		case paASIO:			api = L"ASIO"; break;
		case paSoundManager:	api = L"SoundManager"; break;
		case paCoreAudio:		api = L"CoreAudio"; break;
		case paOSS:				api = L"OSS"; break;
		case paALSA:			api = L"ALSA"; break;
		case paAL:				api = L"AL"; break;
		case paBeOS:			api = L"BeOS"; break;
		case paWDMKS:			api = L"WDMKS"; break;
		case paJACK:			api = L"JACK"; break;
		case paWASAPI:			api = L"WASAPI"; break;
		case paAudioScienceHPI: api = L"AudioScienceHPI"; break;
		default: api = L"Unknown";
		}

		CfgWriteStr( L"PORTAUDIO", L"HostApi", api);
		CfgWriteStr( L"PORTAUDIO", L"Device", m_Device);
	}

} static PA;

SndOutModule *PortaudioOut = &PA;
