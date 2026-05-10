#pragma once

#include "util/types.hpp"
#include "Utilities/File.h"
#include "Emu/Audio/AudioBackend.h"

struct WAVHeader
{
	struct RIFFHeader
	{
		u8 ID[4] = { 'R', 'I', 'F', 'F' };
		le_t<u32> Size{}; // FileSize - 8
		u8 WAVE[4] = { 'W', 'A', 'V', 'E' };

		RIFFHeader() = default;

		RIFFHeader(u32 size)
			: Size(size)
		{
		}
	} RIFF;

	struct FMTHeader
	{
		u8 ID[4] = { 'f', 'm', 't', ' ' };
		le_t<u32> Size = 16;
		le_t<u16> AudioFormat{}; // 1 for PCM, 3 for IEEE Floating Point
		le_t<u16> NumChannels{}; // 1, 2, 6, 8
		le_t<u32> SampleRate{}; // 44100-192000
		le_t<u32> ByteRate{}; // SampleRate * NumChannels * BitsPerSample/8
		le_t<u16> BlockAlign{}; // NumChannels * BitsPerSample/8
		le_t<u16> BitsPerSample{}; // SampleSize * 8

		FMTHeader() = default;

		FMTHeader(AudioChannelCnt ch, AudioFreq sample_rate, AudioSampleSize sample_size)
			: AudioFormat(sample_size == AudioSampleSize::FLOAT ? 3 : 1)
			, NumChannels(static_cast<u16>(ch))
			, SampleRate(static_cast<u32>(sample_rate))
			, ByteRate(SampleRate * NumChannels * static_cast<u32>(sample_size))
			, BlockAlign(NumChannels * static_cast<u32>(sample_size))
			, BitsPerSample(static_cast<u32>(sample_size) * 8)
		{
		}
	} FMT;

	struct FACTChunk
	{
		u8 ID[4] = { 'f', 'a', 'c', 't' };
		le_t<u32> ChunkLength = 4;
		le_t<u32> SampleLength = 0; // total samples per channel

		FACTChunk() = default;

		FACTChunk(u32 sample_len)
			: SampleLength(sample_len)
		{
		}
	} FACT;

	u8 ID[4] = { 'd', 'a', 't', 'a' };
	le_t<u32> Size{}; // size of data (256 * NumChannels * sizeof(f32))

	WAVHeader() = default;

	WAVHeader(AudioChannelCnt ch, AudioFreq sample_rate, AudioSampleSize sample_size)
		: RIFF(sizeof(RIFFHeader) + sizeof(FMTHeader))
		, FMT(ch, sample_rate, sample_size)
		, FACT(0)
		, Size(0)
	{
	}
};

class AudioDumper
{
	WAVHeader m_header{};
	fs::file m_output{};

public:
	AudioDumper();
	~AudioDumper();

	void Open(AudioChannelCnt ch, AudioFreq sample_rate, AudioSampleSize sample_size);
	void Close();

	void WriteData(const void* buffer, u32 size);
	u16 GetCh() const { return m_header.FMT.NumChannels; }
	u16 GetSampleSize() const { return m_header.FMT.BitsPerSample / 8; }
};
