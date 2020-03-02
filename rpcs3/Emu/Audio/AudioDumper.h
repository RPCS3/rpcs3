#pragma once

#include "Utilities/types.h"
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
		u16 BitsPerSample; // sizeof(float) * 8

		FMTHeader() = default;

		FMTHeader(u16 ch)
			: ID("fmt "_u32)
			, Size(16)
			, AudioFormat(3)
			, NumChannels(ch)
			, SampleRate(48000)
			, ByteRate(SampleRate * ch * sizeof(float))
			, BlockAlign(ch * sizeof(float))
			, BitsPerSample(sizeof(float) * 8)
		{
		}
	} FMT;

	u32 ID; // "data"
	u32 Size; // size of data (256 * NumChannels * sizeof(float))

	WAVHeader() = default;

	WAVHeader(u16 ch)
		: RIFF(sizeof(RIFFHeader) + sizeof(FMTHeader))
		, FMT(ch)
		, ID("data"_u32)
		, Size(0)
	{
	}
};

class AudioDumper
{
	WAVHeader m_header;
	fs::file m_output;

public:
	AudioDumper(u16 ch);
	~AudioDumper();

	void WriteData(const void* buffer, u32 size);
	const u16 GetCh() const { return m_header.FMT.NumChannels; }
};
