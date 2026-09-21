#include "stdafx.h"
#include "overlay_big_picture_game_info.h"
#include "Emu/RSX/Overlays/overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		std::unique_ptr<image_info> big_picture_game_info::load_icon(const std::string& icon_path, bool in_archive) const
		{
			return image_info::load_icon(icon_path, in_archive ? path : "");
		}

		std::unique_ptr<image_info> big_picture_game_info::load_icon() const
		{
			return image_info::load_icon(icon_path, icon_in_archive ? path : "");
		}
	}
}
