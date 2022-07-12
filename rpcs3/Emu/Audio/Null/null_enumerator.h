#pragma once

#include "Emu/Audio/audio_device_enumerator.h"

class null_enumerator final : public audio_device_enumerator
{
public:

	null_enumerator() {};
	~null_enumerator() override {};

	std::vector<audio_device> get_output_devices() override { return {}; }
};
