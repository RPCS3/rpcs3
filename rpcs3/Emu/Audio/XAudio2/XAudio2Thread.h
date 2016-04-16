#pragma once
#ifdef _MSC_VER

#include "Emu/Audio/AudioThread.h"

#pragma push_macro("_WIN32_WINNT")
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // This is to be sure that correct (2.7) header is included
#include "3rdparty/XAudio2_7/XAudio2.h" // XAudio2 2.8 available only on Win8+, used XAudio2 2.7 from dxsdk
#pragma pop_macro("_WIN32_WINNT")

class XAudio2Thread : public AudioThread
{
	IXAudio2* m_xaudio2_instance;
	IXAudio2MasteringVoice* m_master_voice;
	IXAudio2SourceVoice* m_source_voice;

public:
	XAudio2Thread();
	virtual ~XAudio2Thread() override;

	virtual void Play() override;
	virtual void Open(const void* src, int size) override;
	virtual void Close() override;
	virtual void Stop() override;
	virtual void AddData(const void* src, int size) override;
};
#endif
