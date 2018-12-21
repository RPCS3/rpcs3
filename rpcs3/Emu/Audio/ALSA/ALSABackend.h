#pragma once

#ifdef HAVE_ALSA

#include "Emu/Audio/AudioBackend.h"

#include <alsa/asoundlib.h>

class ALSABackend : public AudioBackend
{
	snd_pcm_t* s_tls_handle{nullptr};
	snd_pcm_hw_params_t* s_tls_hw_params{nullptr};
	snd_pcm_sw_params_t* s_tls_sw_params{nullptr};
	
public:
	ALSABackend();
	virtual ~ALSABackend() override;

	virtual const char* GetName() const override { return "ALSA"; };

	static const u32 capabilities = 0;
	virtual u32 GetCapabilities() const override { return capabilities; };

	virtual void Open(u32) override;
	virtual void Close() override;
	
	virtual bool AddData(const void* src, u32 num_samples) override;
};

#endif
