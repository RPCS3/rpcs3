#pragma once

#ifdef HAVE_PULSE
#include <pulse/simple.h>
#include "Emu/Audio/AudioThread.h"

class PulseThread : public AudioBackend
{
public:
	PulseThread();
	virtual ~PulseThread() override;

	virtual void Play() override;
	virtual void Open(const void* src, int size) override;
	virtual void Close() override;
	virtual void Stop() override;
	virtual void AddData(const void* src, int size) override;

private:
	pa_simple *connection = nullptr;
};

#endif
