#pragma once

#include <string>

static constexpr usz max_midi_devices = 3;

enum class midi_device_type
{
	keyboard,
	guitar,
	guitar_22fret,
	drums,
};

struct midi_device
{
	midi_device_type type{};
	std::string name;

	static midi_device from_string(const std::string& str);
};
