#pragma once
#include "sysutil_audio.h"
#include "AL\OpenALThread.h"

struct AudioInfo
{
	struct
	{
		u8 type; 
		u8 channel;
		u8 encoder; 
		u8 fs;
		u32 layout; 
		u32 downMixer; 
	} mode;

	AudioInfo()
	{
	}

	void Init()
	{
		mode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		mode.channel = CELL_AUDIO_OUT_CHNUM_2;
		mode.fs = CELL_AUDIO_OUT_FS_48KHZ;
		mode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH;
		mode.encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
		mode.downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
	}
};

class AudioManager
{
	AudioInfo m_audio_info;
public:
	AudioManager();

	void Init();
	void Close();

	AudioInfo& GetInfo() { return m_audio_info; }

	u8 GetState();
};

extern OpenALThread* m_audio_out;