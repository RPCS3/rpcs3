#pragma once

#ifdef __linux__

#include "Emu/Audio/AudioThread.h"

class ALSAThread : public AudioThread
{
public:
	ALSAThread();
	virtual ~ALSAThread() override;

	virtual void Play() override;
	virtual void Open(const void* src, int size) override;
	virtual void Close() override;
	virtual void Stop() override;
	virtual void AddData(const void* src, int size) override;
};

#endif
