#pragma once

/*#include "Emu/Audio/AudioBackend.h"
#include "3rdparty/OpenAL/include/alext.h"
#include <memory>

class OpenALThread : public AudioBackend
{
private:
	ALint m_format;
	ALuint m_source;
	std::unique_ptr<ALuint[]> m_buffers;
	ALsizei m_buffer_size;

public:
	OpenALThread();
	virtual ~OpenALThread() override;

	virtual void Play() override;
	virtual void Open(const void* src, int size) override;
	virtual void Close() override;
	virtual void Stop() override;
	virtual void AddData(const void* src, int size) override;
};
*/