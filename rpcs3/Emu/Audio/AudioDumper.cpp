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
	if(m_output.Open("audio.wav", wxFile::write))
		return true;
	return false;
}

void AudioDumper::WriteHeader()
{
	m_output.Write(&m_header, sizeof(m_header)); // write file header
}

size_t AudioDumper::WriteData(const void* buffer, size_t size)
{
	size_t ret = m_output.Write(buffer, size);
	return ret;
}

void AudioDumper::UpdateHeader(size_t size)
{
	m_header.Size += size;
	m_header.RIFF.Size += size;
}

void AudioDumper::Finalize()
{
	m_output.Seek(0);
	m_output.Write(&m_header, sizeof(m_header)); // write fixed file header
	m_output.Close();
}