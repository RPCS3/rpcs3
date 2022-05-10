#include "stdafx.h"
#include "AudioDumper.h"

#include "Utilities/date_time.h"
#include "Emu/System.h"

#include <bit>

AudioDumper::AudioDumper()
{
}

AudioDumper::~AudioDumper()
{
	Close();
}

void AudioDumper::Open(AudioChannelCnt ch, AudioFreq sample_rate, AudioSampleSize sample_size)
{
	Close();

	m_header = WAVHeader(ch, sample_rate, sample_size);
	std::string path = fs::get_cache_dir() + "audio_";
	if (const std::string id = Emu.GetTitleID(); !id.empty())
	{
		path += id + "_";
	}
	path += date_time::current_time_narrow<'_'>() + ".wav";
	m_output.open(path, fs::rewrite);
	m_output.seek(sizeof(m_header));
}

void AudioDumper::Close()
{
	if (GetCh())
	{
		if (m_header.Size & 1)
		{
			const u8 pad_byte = 0;
			m_output.write(pad_byte);
			m_header.RIFF.Size += 1;
		}

		m_output.seek(0);
		m_output.write(m_header); // write file header
		m_output.close();
		m_header.FMT.NumChannels = 0;
	}
}

void AudioDumper::WriteData(const void* buffer, u32 size)
{
	if (GetCh() && size && buffer)
	{
		const u32 blk_size = GetCh() * GetSampleSize();
		const u32 sample_cnt_per_ch = size / blk_size;

		ensure(size - sample_cnt_per_ch * blk_size == 0);

		if constexpr (std::endian::big == std::endian::native)
		{
			std::vector<u8> tmp_buf(size);

			if (GetSampleSize() == sizeof(f32))
			{
				for (u32 sample_idx = 0; sample_idx < sample_cnt_per_ch * GetCh(); sample_idx++)
				{
					std::bit_cast<f32*>(tmp_buf.data())[sample_idx] = static_cast<const be_t<f32>*>(buffer)[sample_idx];
				}
			}
			else
			{
				for (u32 sample_idx = 0; sample_idx < sample_cnt_per_ch * GetCh(); sample_idx++)
				{
					std::bit_cast<s16*>(tmp_buf.data())[sample_idx] = static_cast<const be_t<s16>*>(buffer)[sample_idx];
				}
			}

			ensure(m_output.write(tmp_buf.data(), size) == size);
		}
		else
		{
			ensure(m_output.write(buffer, size) == size);
		}

		m_header.Size += size;
		m_header.RIFF.Size += size;
		m_header.FACT.SampleLength += sample_cnt_per_ch;
	}
}
