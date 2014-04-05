#pragma once

struct WAVHeader
{
	struct RIFFHeader
	{
		u32 ID; // "RIFF"
		u32 Size; // FileSize - 8
		u32 WAVE; // "WAVE"

		RIFFHeader(u32 size)
			: ID(*(u32*)"RIFF")
			, WAVE(*(u32*)"WAVE")
			, Size(size)
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

		FMTHeader(u8 ch)
			: ID(*(u32*)"fmt ")
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

	WAVHeader(u8 ch)
		: ID(*(u32*)"data")
		, Size(0)
		, FMT(ch)
		, RIFF(sizeof(RIFFHeader) + sizeof(FMTHeader))
	{
	}
};


class AudioDumper
{
private:
	WAVHeader m_header;
	wxFile m_output;
	
public:
	AudioDumper(u8 ch);
	~AudioDumper();

	bool Init();
	void WriteHeader();
	size_t WriteData(const void* buffer, size_t size);
	void Finalize();
	const u8 GetCh() const { return m_header.FMT.NumChannels; }
};
