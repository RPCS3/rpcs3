#pragma once

#include "Emu/Audio/AudioBackend.h"

class NullAudioBackend : public AudioBackend
{
public:
	NullAudioBackend() {}
	virtual ~NullAudioBackend() {}

	virtual const char* GetName() const override { return "Null";  }

	static const u32 capabilities = NON_BLOCKING;
	virtual u32 GetCapabilities() const override { return capabilities; };

	virtual void Open(u32) override {};
	virtual void Close() override {};

	virtual void Play() override {};
	virtual void Pause() override {};

	virtual bool AddData(const void*, u32) override { return true; };
	virtual void Flush() override {};
};
