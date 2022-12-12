#pragma once

#ifndef _WIN32
#error "XAudio2 can only be built on Windows."
#endif

#include "Emu/Audio/audio_device_enumerator.h"

class xaudio2_enumerator final : public audio_device_enumerator
{
public:

	xaudio2_enumerator();
	~xaudio2_enumerator() override;

	std::vector<audio_device> get_output_devices() override;
};
