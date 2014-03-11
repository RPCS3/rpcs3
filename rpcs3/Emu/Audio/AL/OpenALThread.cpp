#include "stdafx.h"
#include "OpenALThread.h"

ALenum g_last_al_error = AL_NO_ERROR;
ALCenum g_last_alc_error = ALC_NO_ERROR;

ALCdevice*		pDevice;
ALCcontext*		pContext;

void printAlError(ALenum err, const char* situation)
{
	if(err != AL_NO_ERROR)
	{
		ConLog.Error("%s: OpenAL error 0x%04x", wxString(situation).wx_str(), err);
		Emu.Pause();
	}
}

void printAlcError(ALCenum err, const char* situation)
{
	if(err != ALC_NO_ERROR)
	{
		ConLog.Error("%s: OpenALC error 0x%04x", wxString(situation).wx_str(), err);
		Emu.Pause();
	}
}

void OpenALThread::Init()
{
	pDevice = alcOpenDevice(NULL);
	checkForAlcError("alcOpenDevice");

	pContext = alcCreateContext(pDevice, NULL);
	checkForAlcError("alcCreateContext");

	alcMakeContextCurrent(pContext);
	checkForAlcError("alcMakeContextCurrent");
}

void OpenALThread::Quit()
{
	for (SampleBuffer::iterator i = mBuffers.begin(); i != mBuffers.end(); i++)
		alDeleteBuffers(1, &i->second.mBufferID);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(pContext);
	alcCloseDevice(pDevice);
}

void OpenALThread::Play()
{
	alSourcePlay(mSource);
	checkForAlError("alSourcePlay");
}

void OpenALThread::Close()
{
	alSourceStop(mSource);
	checkForAlError("alSourceStop");
	if (alIsSource(mSource)) 
		alDeleteSources(1, &mSource);
}

void OpenALThread::Stop()
{
	alSourceStop(mSource);
	checkForAlError("alSourceStop");
}

void OpenALThread::Open(const void* src, ALsizei size)
{
	alGenSources(1, &mSource);
	checkForAlError("alGenSources");

	alSourcei(mSource, AL_LOOPING, AL_FALSE);
	checkForAlError("alSourcei");

	mProcessed = 0;
	mBuffer.mFreq = 48000;
	mBuffer.mFormat	= AL_FORMAT_STEREO16;

	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		AddData(src, size);
	}
}

void OpenALThread::AddData(const void* src, ALsizei size)
{
	alGenBuffers(1, &mBuffer.mBufferID);
	checkForAlError("alGenBuffers");
	
	mBuffers[mBuffer.mBufferID] = mBuffer;
	
	AddBlock(mBuffer.mBufferID, size, src);

	alSourceQueueBuffers(mSource, 1, &mBuffer.mBufferID);
	checkForAlError("alSourceQueueBuffers");

	alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &mProcessed);
	
	while (mProcessed--)
	{
		alSourceUnqueueBuffers(mSource, 1, &mBuffer.mBufferID);
		checkForAlError("alSourceUnqueueBuffers");

		alDeleteBuffers(1,  &mBuffer.mBufferID);
		checkForAlError("alDeleteBuffers");
	}
}

bool OpenALThread::AddBlock(ALuint bufferID, ALsizei size, const void* src)
{
	memset(&mTempBuffer, 0, sizeof(mTempBuffer));
	memcpy(mTempBuffer, src, size);

	long TotalRet = 0, ret;
 
	if (size < 1) return false;

	while (TotalRet < size) 
	{
		ret = size;

		// if buffer is empty
		if (ret == 0) break;
		else if (ret < 0)
		{
			ConLog.Error("Error in bitstream!");
		}
		else
		{
			TotalRet += ret;
		}
	}
	if (TotalRet > 0)
	{
	alBufferData(bufferID, mBuffers[bufferID].mFormat, mTempBuffer, 
				TotalRet, mBuffers[bufferID].mFreq);
	checkForAlError("alBufferData");
	}

	return (ret > 0);
}