#include "stdafx.h"
#include "AudioDumper.h"

AudioDumper::AudioDumper(u8 ch) : m_header(ch)
{
}

AudioDumper::~AudioDumper()
{
}

bool AudioDumper::Init()
{
	return m_output.Open("audio.wav", wxFile::write);
}

void AudioDumper::WriteHeader()
{
	m_output.Write(&m_header, sizeof(m_header)); // write file header
}

size_t AudioDumper::WriteData(const void* buffer, size_t size)
{
#ifdef SKIP_EMPTY_AUDIO
	bool do_save = false;
	for (u32 i = 0; i < size / 8; i++)
	{
		if (((u64*)buffer)[i]) do_save = true;
	}
	for (u32 i = 0; i < size % 8; i++)
	{
		if (((u8*)buffer)[i + (size & ~7)]) do_save = true;
	}
	if (!do_save) return size; // ignore empty data
#endif
	size_t ret = m_output.Write(buffer, size);
	m_header.Size += ret;
	m_header.RIFF.Size += ret;
	return ret;
}

void AudioDumper::Finalize()
{
	m_output.Seek(0);
	m_output.Write(&m_header, sizeof(m_header)); // write fixed file header
	m_output.Close();
}