#pragma once

// PS5: screenshots. As the pause menu opens, the game's next frame (without
// the menu, and before the game puts up its own pause screen) is read back and
// held (ps5_gs_frame::take_screenshot); the menu's Save screenshot writes it as
// a PNG to the config folder's screenshots/<serial>/, which the launcher's
// Gallery shows and FTP reaches. A held frame is dropped as the menu closes.

#include "util/types.hpp"

#include <string>
#include <vector>

namespace ps5_screenshots
{
	// Ask for the game's next frame, to hold
	void hold_next();

	// From the renderer: the frame asked for, RGBA or BGRA, width x height
	void take(std::vector<u8>&& pixels, u32 width, u32 height, bool bgra);

	bool held();
	void drop_held();

	// The held frame written, on a thread of its own; its path, empty if none
	std::string save_held(const std::string& serial);

	// The folder, with a trailing slash
	std::string folder();

	struct shot
	{
		std::string path;
		std::string serial; // its folder's name
		s64 time = 0;       // seconds since 1970
	};

	// Every one, the latest first
	std::vector<shot> list();
}
