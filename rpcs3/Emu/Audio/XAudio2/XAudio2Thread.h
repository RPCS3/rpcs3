#pragma once

#include "Emu/Audio/AudioThread.h"
#if defined (_WIN32)

// forced define Win7, delete this for using XAudio2 2.8
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#else
#include "XAudio2.h" // XAudio2 2.8 available only on Win8+, used XAudio2 2.7 from dxsdk
#endif

class XAudio2Thread : public AudioThread
{
private:
	IXAudio2* m_xaudio2_instance;
	IXAudio2MasteringVoice* m_master_voice;
	IXAudio2SourceVoice* m_source_voice;

public:
	virtual ~XAudio2Thread();

	virtual void Init();
	virtual void Quit();
	virtual void Play();
	virtual void Open(const void* src, int size);
	virtual void Close();
	virtual void Stop();
	virtual void AddData(const void* src, int size);
};
#endif