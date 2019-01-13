#include "stdafx.h"
#include "AudioDumper.h"


AudioDumper::AudioDumper(u16 ch)
	: m_header(ch)
{
	if (GetCh())
	{
		m_output.open(fs::get_config_dir() + "audio.wav", fs::rewrite);
		m_output.write(m_header); // write initial file header
	}
}

AudioDumper::~AudioDumper()
{
	if (GetCh())
	{
		m_output.seek(0);
		m_output.write(m_header); // rewrite file header
	}
}

void AudioDumper::WriteData(const void* buffer, u32 size)
{
	if (GetCh())
	{
		verify(HERE), size, m_output.write(buffer, size) == size;
		m_header.Size += size;
		m_header.RIFF.Size += size;
	}
}
