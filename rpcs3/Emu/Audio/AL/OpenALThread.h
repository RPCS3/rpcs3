#pragma once

#include "OpenAL/include/al.h"
#include "OpenAL/include/alc.h"
#include <map>

extern ALenum g_last_al_error;
extern ALCenum g_last_alc_error;

void printAlError(ALenum err, const char* situation);
void printAlcError(ALCenum err, const char* situation);

#define checkForAlError(sit) if((g_last_al_error = alGetError()) != AL_NO_ERROR) printAlError(g_last_al_error, sit)
#define checkForAlcError(sit) if((g_last_alc_error = alcGetError(pDevice)) != ALC_NO_ERROR) printAlcError(g_last_alc_error, sit)

struct SampleInfo
{
	uint mBufferID;
	uint mFreq;
	uint mFormat;
};

typedef std::map<ALuint, SampleInfo> SampleBuffer;

#define NUM_OF_BUFFERS	16


extern ALCdevice*		pDevice;
extern ALCcontext*		pContext;

class OpenALThread
{
private:
	ALuint			mSource;
	SampleBuffer	mBuffers;
	SampleInfo		mBuffer;
	ALint			mProcessed;
	u16				mTempBuffer[512];

public:
	void Init();
	void Quit();
	void Play();
	void Open(const void* src, ALsizei size);
	void Close();
	void Stop();
	bool AddBlock(ALuint bufferID, ALsizei size, const void* src);
	void AddData(const void* src, ALsizei size);
};

