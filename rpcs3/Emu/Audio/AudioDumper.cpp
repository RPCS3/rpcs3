#include "stdafx.h"
#include "AudioDumper.h"

AudioDumper::AudioDumper() : m_header(0), m_init(false)
{
}

AudioDumper::~AudioDumper()
{
	Finalize();
}

bool AudioDumper::Init(u8 ch)
{
	if ((m_init = m_output.Open("audio.wav", rFile::write)))
	{
		m_header = WAVHeader(ch);
		WriteHeader();
	}

	return m_init;
}

void AudioDumper::WriteHeader()
{
	if (m_init)
	{
		m_output.Write(&m_header, sizeof(m_header)); // write file header
	}
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

	if (m_init && do_save)
#else
	if (m_init)
#endif
	{
		size_t ret = m_output.Write(buffer, size);
		m_header.Size += (u32)ret;
		m_header.RIFF.Size += (u32)ret;
		return ret;
	}
	
	return size;
}

void AudioDumper::Finalize()
{
	if (m_init)
	{
		m_output.Seek(0);
		m_output.Write(&m_header, sizeof(m_header)); // write fixed file header
		m_output.Close();
	}
}