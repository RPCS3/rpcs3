#pragma once

#ifndef HAVE_ALSA
#error "ALSA support disabled but still being built."
#endif

#include "Emu/Audio/AudioBackend.h"

#include <alsa/asoundlib.h>

class ALSABackend : public AudioBackend
{
	snd_pcm_t* tls_handle{};
	snd_pcm_hw_params_t* tls_hw_params{};
	snd_pcm_sw_params_t* tls_sw_params{};

public:
	ALSABackend();
	virtual ~ALSABackend() override;

	ALSABackend(const ALSABackend&) = delete;
	ALSABackend& operator=(const ALSABackend&) = delete;

	virtual const char* GetName() const override { return "ALSA"; }

	static const u32 capabilities = 0;
	virtual u32 GetCapabilities() const override { return capabilities; }

	virtual void Open(u32) override;
	virtual void Close() override;

	virtual bool AddData(const void* src, u32 num_samples) override;
};
