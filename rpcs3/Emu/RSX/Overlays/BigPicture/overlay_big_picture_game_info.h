#pragma once

#include "Emu/GameInfo.h"

namespace rsx
{
	namespace overlays
	{
		struct image_info;

		struct big_picture_game_info : public GameInfo
		{
			std::unique_ptr<image_info> load_icon(const std::string& icon_path, bool in_archive) const;
			std::unique_ptr<image_info> load_icon() const;
		};
}
}
