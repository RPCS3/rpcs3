#pragma once

#include "util/types.hpp"
#include <vector>
#include <string>

class audio_device_enumerator
{
public:

	static constexpr std::string_view DEFAULT_DEV_ID = "@@@default@@@";

	struct audio_device
	{
		std::string id{};
		std::string name{};
		usz max_ch{};
	};

	audio_device_enumerator() {};

	virtual ~audio_device_enumerator() = default;

	// Enumerate available output devices.
	virtual std::vector<audio_device> get_output_devices() = 0;
};
