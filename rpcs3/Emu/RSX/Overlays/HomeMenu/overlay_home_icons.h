#pragma once

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/RSX/Overlays/overlay_controls.h"

namespace rsx::overlays::home_menu
{
	enum class fa_icon
	{
		none = 0,
		home,
		settings,
		back,
		floppy,
		maximize,
		play,
		poweroff,
		restart,
		screenshot,
		video_camera,
		trophy,
		friends,
		audio,
		video,
		gamepad,
		settings_sliders,
		settings_gauge,
		bug,
	};

	const image_info* get_icon(fa_icon icon);
}
