#pragma once

#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "Emu/Audio/audio_device_enumerator.h"
#include "FAudio.h"

class faudio_enumerator final : public audio_device_enumerator
{
public:

	faudio_enumerator();
	~faudio_enumerator() override;

	std::vector<audio_device> get_output_devices() override;

private:

	FAudio* instance{};
};
