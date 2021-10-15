#pragma once

#ifndef HAVE_PULSE
#error "PulseAudio support disabled but still being built."
#endif

#include <pulse/simple.h>
#include "Emu/Audio/AudioBackend.h"

class PulseBackend : public AudioBackend
{
public:
	PulseBackend();
	virtual ~PulseBackend() override;

	PulseBackend(const PulseBackend&) = delete;
	PulseBackend& operator=(const PulseBackend&) = delete;

	virtual const char* GetName() const override { return "Pulse"; }

	static const u32 capabilities = 0;
	virtual u32 GetCapabilities() const override { return capabilities; }

	virtual void Open(u32) override;
	virtual void Close() override;

	virtual bool AddData(const void* src, u32 num_samples) override;

private:
	pa_simple* connection{};
};
