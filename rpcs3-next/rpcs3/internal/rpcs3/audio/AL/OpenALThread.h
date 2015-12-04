#pragma once

#include "Emu/Audio/AudioThread.h"
#include "OpenAL/include/alext.h"

class OpenALThread : public AudioThread
{
private:
	static const uint g_al_buffers_count = 16;

	ALuint m_source;
	ALuint m_buffers[g_al_buffers_count];
	ALCdevice* m_device;
	ALCcontext* m_context;
	ALsizei m_buffer_size;

public:
	virtual ~OpenALThread();

	virtual void Init();
	virtual void Quit();
	virtual void Play();
	virtual void Open(const void* src, int size);
	virtual void Close();
	virtual void Stop();
	virtual void AddData(const void* src, int size);
};
