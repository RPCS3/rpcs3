#pragma once

#include "overlays.h"

namespace rsx
{
	namespace overlays
	{
		class debug_overlay : public user_interface
		{
			label text_display;
			text_guard_t text_guard{};

		public:
			debug_overlay();

			compiled_resource get_compiled() override;

			void set_text(std::string&& text);
		};

		void reset_debug_overlays();
		void set_debug_overlay_text(std::string&& text);
	}
}
