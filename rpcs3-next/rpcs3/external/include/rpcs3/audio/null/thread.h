#pragma once

#include "Emu/Audio/AudioThread.h"

class NullAudioThread : public AudioThread
{
public:
	NullAudioThread() {}
	virtual ~NullAudioThread() {}

	virtual void Init() {}
	virtual void Quit() {}
	virtual void Play() {}
	virtual void Open(const void* src, int size) {}
	virtual void Close() {}
	virtual void Stop() {}
	virtual void AddData(const void* src, int size) {}
};
