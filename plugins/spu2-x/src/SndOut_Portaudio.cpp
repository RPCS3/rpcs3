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

	wstring m_Api;
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
	s32 Init()
	{
		started=false;
		stream=NULL;

		PaError err = Pa_Initialize();
		if( err != paNoError )
		{
			fprintf(stderr," * SPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			return -1;
		}
		started=true;

		err = Pa_OpenDefaultStream( &stream,
			0, 2, paInt32, 48000,
			SndOutPacketSize,
			PaCallback, NULL );
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
	}

	void WriteSettings() const
	{
	}

} static PA;

SndOutModule *PortaudioOut = &PA;
