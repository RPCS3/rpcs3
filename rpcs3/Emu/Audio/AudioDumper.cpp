#include "stdafx.h"
#include "AudioDumper.h"

#include "Utilities/date_time.h"
#include "Emu/System.h"

AudioDumper::AudioDumper(u16 ch)
	: m_header(ch)
{
	if (GetCh())
	{
		std::string path = fs::get_cache_dir() + "audio_";
		if (const std::string id = Emu.GetTitleID(); !id.empty())
		{
			path += id + "_";
		};
		path += date_time::current_time_narrow<'_'>() + ".wav";
		m_output.open(path, fs::rewrite);
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
