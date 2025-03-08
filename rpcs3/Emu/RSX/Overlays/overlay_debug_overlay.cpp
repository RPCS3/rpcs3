#include "stdafx.h"
#include "overlay_manager.h"
#include "overlay_debug_overlay.h"
#include "Emu/system_config.h"

namespace rsx
{
	namespace overlays
	{
		debug_overlay::debug_overlay()
		{
			text_display.set_size(1260, 40);
			text_display.set_pos(10, 10);
			text_display.set_font("n023055ms.ttf", 10);
			text_display.align_text(overlay_element::text_align::left);
			text_display.set_wrap_text(true);
			text_display.fore_color = { 0.3f, 1.f, 0.3f, 1.f };
			text_display.back_color.a = 0.f;
		}

		compiled_resource debug_overlay::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			if (const auto [dirty, text] = text_guard.get_text(); dirty)
			{
				text_display.set_text(text);
			}

			compiled_resource result;
			result.add(text_display.get_compiled());
			return result;
		}

		void debug_overlay::set_text(std::string&& text)
		{
			text_guard.set_text(std::move(text));
			visible = true;
		}

		extern void reset_debug_overlay()
		{
			if (!g_cfg.misc.use_native_interface)
				return;

			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				auto overlay = manager->get<rsx::overlays::debug_overlay>();

				if (g_cfg.video.debug_overlay || g_cfg.io.debug_overlay)
				{
					if (!overlay)
					{
						overlay = manager->create<rsx::overlays::debug_overlay>();
					}
				}
				else if (overlay)
				{
					manager->remove<rsx::overlays::debug_overlay>();
				}
			}
		}

		extern void set_debug_overlay_text(std::string&& text)
		{
			if (!g_cfg.misc.use_native_interface || (!g_cfg.video.debug_overlay && !g_cfg.io.debug_overlay))
				return;

			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				if (auto overlay = manager->get<rsx::overlays::debug_overlay>())
				{
					overlay->set_text(std::move(text));
				}
			}
		}
	} // namespace overlays
} // namespace rsx
