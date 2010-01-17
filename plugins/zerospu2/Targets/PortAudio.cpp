/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
 // PortAudio support implemented by Zedr0n.
 
#include "PortAudio.h"
#include "Linux.h"

PaStream *stream;

#define FRAMES_PER_BUFFER 4096

int paSetupSound()
{
	// initialize portaudio
	PaError err = Pa_Initialize();
	if( err != paNoError) {
		ERROR_LOG("ZeroSPU2: Error initializing portaudio: %s\n", Pa_GetErrorText(err) );
		return -1;
	}
	else
		ERROR_LOG("ZeroSPU2: Initialized portaudio... \n");
	
	/* Open an audio I/O stream. */
	err = Pa_OpenDefaultStream( &stream,
															0,          /* no input channels */
															2,          /* stereo output */
															paInt16,  	/* 16 bit integer point output */
															SAMPLE_RATE,
															FRAMES_PER_BUFFER,        /* frames per buffer, i.e. the number
																								 of sample frames that PortAudio will
																								 request from the callback. Many apps
																								 may want to use
																								 paFramesPerBufferUnspecified, which
																								 tells PortAudio to pick the best,
																								 possibly changing, buffer size.*/
															NULL, /* this is your callback function */
															NULL ); /*This is a pointer that will be passed to
																								 your callback*/
	if( err != paNoError )
	{
		ERROR_LOG("ZeroSPU2: Error opening sound stream...\n");
		return -1;
	}
	else ERROR_LOG("ZeroSPU2: Created sound stream successfully...\n");	

	MaxBuffer = 0;

  err = Pa_StartStream( stream );
	if( err != paNoError )
	{
		ERROR_LOG("ZeroSPU2: Error starting sound stream...\n");
		return -1;
	}
	else ERROR_LOG("ZeroSPU2: Started sound stream successfully...\n");	

	return 0;
}

void paRemoveSound()
{	
  PaError  err = Pa_StopStream( stream );
	if( err != paNoError )
		 ERROR_LOG("ZeroSPU2: PortAudio error: %s\n", Pa_GetErrorText( err ) );
	else
		ERROR_LOG("ZeroSPU2: stream stopped... \n");

	err = Pa_Terminate();
	if( err != paNoError )
		 ERROR_LOG("PortAudio error: %s\n", Pa_GetErrorText( err ) );
	else
		ERROR_LOG("ZeroSPU2: portaudio terminated... \n");
}

int paSoundGetBytesBuffered()
{
	unsigned long l = Pa_GetStreamWriteAvailable(stream); 
	if(MaxBuffer == 0) MaxBuffer = l; 
	return MaxBuffer - l;	
}

void paSoundFeedVoiceData(unsigned char* pSound, long lBytes)
{
	Pa_WriteStream(stream, (void*) pSound, (unsigned long) lBytes / 4);
}

