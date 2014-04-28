#pragma once

#include "OpenAL/include/al.h"
#include "OpenAL/include/alc.h"

class OpenALThread
{
private:
	static const uint g_al_buffers_count = 16;

	ALuint m_source;
	ALuint m_buffers[g_al_buffers_count];
	ALCdevice* m_device;
	ALCcontext* m_context;
	ALsizei m_buffer_size;

public:
	~OpenALThread();

	void Init();
	void Quit();
	void Play();
	void Open(const void* src, ALsizei size);
	void Close();
	void Stop();
	bool AddBlock(const ALuint buffer_id, ALsizei size, const void* src);
	void AddData(const void* src, ALsizei size);
};

