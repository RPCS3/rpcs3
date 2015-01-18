#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "rpcs3/Ini.h"

#include "OpenALThread.h"

ALenum g_last_al_error = AL_NO_ERROR;
ALCenum g_last_alc_error = ALC_NO_ERROR;

#define checkForAlError(sit) if((g_last_al_error = alGetError()) != AL_NO_ERROR) printAlError(g_last_al_error, sit)
#define checkForAlcError(sit) if((g_last_alc_error = alcGetError(m_device)) != ALC_NO_ERROR) printAlcError(g_last_alc_error, sit)

void printAlError(ALenum err, const char* situation)
{
	if (err != AL_NO_ERROR)
	{
		LOG_ERROR(HLE, "%s: OpenAL error 0x%04x", situation, err);
		Emu.Pause();
	}
}

void printAlcError(ALCenum err, const char* situation)
{
	if (err != ALC_NO_ERROR)
	{
		LOG_ERROR(HLE, "%s: OpenALC error 0x%04x", situation, err);
		Emu.Pause();
	}
}

OpenALThread::~OpenALThread()
{
	Quit();
}

void OpenALThread::Init()
{
	m_device = alcOpenDevice(nullptr);
	checkForAlcError("alcOpenDevice");

	m_context = alcCreateContext(m_device, nullptr);
	checkForAlcError("alcCreateContext");

	alcMakeContextCurrent(m_context);
	checkForAlcError("alcMakeContextCurrent");
}

void OpenALThread::Quit()
{
	m_context = alcGetCurrentContext();
	m_device = alcGetContextsDevice(m_context);
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(m_context);
	alcCloseDevice(m_device);
}

void OpenALThread::Play()
{
	ALint state;
	alGetSourcei(m_source, AL_SOURCE_STATE, &state);
	checkForAlError("OpenALThread::Play -> alGetSourcei");

	if (state != AL_PLAYING)
	{
		alSourcePlay(m_source);
		checkForAlError("alSourcePlay");
	}
}

void OpenALThread::Close()
{
	alSourceStop(m_source);
	checkForAlError("alSourceStop");
	if (alIsSource(m_source))
		alDeleteSources(1, &m_source);

	alDeleteBuffers(g_al_buffers_count, m_buffers);
	checkForAlError("alDeleteBuffers");
}

void OpenALThread::Stop()
{
	alSourceStop(m_source);
	checkForAlError("alSourceStop");
}

void OpenALThread::Open(const void* src, int size)
{
	alGenSources(1, &m_source);
	checkForAlError("alGenSources");

	alGenBuffers(g_al_buffers_count, m_buffers);
	checkForAlError("alGenBuffers");

	alSourcei(m_source, AL_LOOPING, AL_FALSE);
	checkForAlError("OpenALThread::Open ->alSourcei");

	m_buffer_size = size;

	for (uint i = 0; i<g_al_buffers_count; ++i)
	{
		alBufferData(m_buffers[i], Ini.AudioConvertToU16.GetValue() ? AL_FORMAT_71CHN16 : AL_FORMAT_71CHN32, src, m_buffer_size, 48000);
		checkForAlError("alBufferData");
	}

	alSourceQueueBuffers(m_source, g_al_buffers_count, m_buffers);
	checkForAlError("alSourceQueueBuffers");
	Play();
}

void OpenALThread::AddData(const void* src, int size)
{
	const char* bsrc = (const char*)src;
	ALuint buffer;
	ALint buffers_count;

	alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &buffers_count);
	checkForAlError("OpenALThread::AddData -> alGetSourcei");

	while (size)
	{
		if (buffers_count-- <= 0)
		{
			Play();

			alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &buffers_count);
			checkForAlError("OpenALThread::AddData(in loop) -> alGetSourcei");

			continue;
		}

		alSourceUnqueueBuffers(m_source, 1, &buffer);
		checkForAlError("alSourceUnqueueBuffers");

		int bsize = size < m_buffer_size ? size : m_buffer_size;

		alBufferData(buffer, Ini.AudioConvertToU16.GetValue() ? AL_FORMAT_71CHN16 : AL_FORMAT_71CHN32, bsrc, bsize, 48000);
		checkForAlError("alBufferData");

		alSourceQueueBuffers(m_source, 1, &buffer);
		checkForAlError("alSourceQueueBuffers");

		size -= bsize;
		bsrc += bsize;
	}

	Play();
}