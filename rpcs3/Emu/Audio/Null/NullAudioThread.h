#pragma once

#include "Emu/Audio/AudioThread.h"

class NullAudioThread : public AudioThread
{
public:
	NullAudioThread() {}
	virtual ~NullAudioThread() {}

	virtual void Open() {};
	virtual void Close() {};

	virtual void Play() {};
	virtual void Pause() {};
	virtual bool IsPlaying() { return true; };

	virtual bool AddData(const void* src, int size) { return true; };
	virtual void Flush() {};
};
