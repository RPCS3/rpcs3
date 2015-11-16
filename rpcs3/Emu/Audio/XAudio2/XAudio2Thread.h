#pragma once

#include "Emu/Audio/AudioThread.h"
#include "minidx12/Include/SDKDDKVer.h" // Use last SDKDDKVer in SDK 10.240 for Win10
#if defined (_WIN32)

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) // XAudio2 2.8 and 2.9 available only on Win8+ and Win10.
#include "minidx12/Include/XAudio2.h"

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
#pragma comment(lib,"../minidx12/Lib/xaudio2.lib") // Lib 2.9 for Only Windows 10
#else
#pragma comment(lib,"../minidx12/Lib/xaudio2_8.lib") // Lib 2.8 for Only Windows 8+
#endif

#else 
#include "minidx9/Include/XAudio2.h" // XAudio 2.7 for Windows 7 Only
#endif

class XAudio2Thread : public AudioThread
{
private:
	IXAudio2* m_xaudio2_instance;
	IXAudio2MasteringVoice* m_master_voice;
	IXAudio2SourceVoice* m_source_voice;

public:
	virtual ~XAudio2Thread();
	XAudio2Thread();

	virtual void Init();
	virtual void Quit();
	virtual void Play();
	virtual void Open(const void* src, int size);
	virtual void Close();
	virtual void Stop();
	virtual void AddData(const void* src, int size);
};
#endif