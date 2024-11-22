#pragma once

#include "overlay_home_menu_page.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_savestate : public home_menu_page
		{
		public:
			home_menu_savestate(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);

		private:
			std::vector<std::shared_ptr<home_menu_page>> m_savestate_pages;
		};
	}
}
