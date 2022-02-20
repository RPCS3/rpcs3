#pragma once

#include "util/types.hpp"
#include "Utilities/File.h"

struct WAVHeader
{
	struct RIFFHeader
	{
		u32 ID; // "RIFF"
		u32 Size; // FileSize - 8
		u32 WAVE; // "WAVE"

		RIFFHeader() = default;

		RIFFHeader(u32 size)
			: ID("RIFF"_u32)
			, Size(size)
			, WAVE("WAVE"_u32)
		{
		}
	} RIFF;

	struct FMTHeader
	{
		u32 ID; // "fmt "
		u32 Size; // 16
		u16 AudioFormat; // 1 for PCM, 3 for IEEE Floating Point
		u16 NumChannels; // 1, 2, 6, 8
		u32 SampleRate; // 48000
		u32 ByteRate; // SampleRate * NumChannels * BitsPerSample/8
		u16 BlockAlign; // NumChannels * BitsPerSample/8
		u16 BitsPerSample; // SampleSize * 8

		FMTHeader() = default;

		FMTHeader(u16 ch, u32 sample_rate, u32 sample_size)
			: ID("fmt "_u32)
			, Size(16)
			, AudioFormat(sample_size == sizeof(float) ? 3 : 1)
			, NumChannels(ch)
			, SampleRate(sample_rate)
			, ByteRate(SampleRate * ch * sample_size)
			, BlockAlign(ch * sample_size)
			, BitsPerSample(sample_size * 8)
		{
		}
	} FMT;

	u32 ID; // "data"
	u32 Size; // size of data (256 * NumChannels * sizeof(float))

	WAVHeader() = default;

	WAVHeader(u16 ch, u32 sample_rate, u32 sample_size)
		: RIFF(sizeof(RIFFHeader) + sizeof(FMTHeader))
		, FMT(ch, sample_rate, sample_size)
		, ID("data"_u32)
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

	void Open(u16 ch, u32 sample_rate, u32 sample_size);
	void Close();

	void WriteData(const void* buffer, u32 size);
	u16 GetCh() const { return m_header.FMT.NumChannels; }
};
