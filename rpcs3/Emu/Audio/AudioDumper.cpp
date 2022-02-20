#include "stdafx.h"
#include "AudioDumper.h"

#include "Utilities/date_time.h"
#include "Emu/System.h"

AudioDumper::AudioDumper()
{
}

AudioDumper::~AudioDumper()
{
	Close();
}

void AudioDumper::Open(u16 ch, u32 sample_rate, u32 sample_size)
{
	Close();

	if (ch)
	{
		m_header = WAVHeader(ch, sample_rate, sample_size);
		std::string path = fs::get_cache_dir() + "audio_";
		if (const std::string id = Emu.GetTitleID(); !id.empty())
		{
			path += id + "_";
		}
		path += date_time::current_time_narrow<'_'>() + ".wav";
		m_output.open(path, fs::rewrite);
		m_output.write(m_header); // write initial file header
	}
}

void AudioDumper::Close()
{
	if (GetCh())
	{
		m_output.seek(0);
		m_output.write(m_header); // rewrite file header
		m_output.close();
		m_header.FMT.NumChannels = 0;
	}
}

void AudioDumper::WriteData(const void* buffer, u32 size)
{
	if (GetCh())
	{
		ensure(size);
		ensure(m_output.write(buffer, size) == size);
		m_header.Size += size;
		m_header.RIFF.Size += size;
	}
}
