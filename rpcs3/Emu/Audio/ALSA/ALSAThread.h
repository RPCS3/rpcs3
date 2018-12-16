#pragma once

#ifdef HAVE_ALSA

#include "Emu/Audio/AudioThread.h"

class ALSAThread : public AudioBackend
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
