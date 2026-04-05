#include "stdafx.h"
#include "overlay_home_icons.h"

#include "Emu/RSX/Overlays/overlay_controls.h"

#include <unordered_map>

namespace rsx::overlays::home_menu
{
	std::unordered_map<fa_icon, std::unique_ptr<image_info>> g_icons_cache;
	std::mutex g_icons_cache_lock;

	static const char* fa_icon_to_filename(fa_icon icon)
	{
		switch (icon)
		{
		default:
		case fa_icon::none:
			return "";
		case fa_icon::home:
			return "home.png";
		case fa_icon::settings:
			return "settings.png";
		case fa_icon::back:
			return "circle-left-solid.png";
		case fa_icon::floppy:
			return "floppy-disk-solid.png";
		case fa_icon::maximize:
			return "maximize-solid.png";
		case fa_icon::play:
			return "play-button-arrowhead.png";
		case fa_icon::poweroff:
			return "power-off-solid.png";
		case fa_icon::restart:
			return "rotate-left-solid.png";
		case fa_icon::screenshot:
			return "screenshot.png";
		case fa_icon::video_camera:
			return "video-camera.png";
		case fa_icon::friends:
			return "user-group-solid.png";
		case fa_icon::trophy:
			return "trophy-solid.png";
		case fa_icon::audio:
			return "headphones-solid.png";
		case fa_icon::video:
			return "display-solid.png";
		case fa_icon::gamepad:
			return "gamepad-solid.png";
		case fa_icon::settings_sliders:
			return "sliders-solid.png";
		case fa_icon::settings_gauge:
			return "gauge-solid.png";
		case fa_icon::bug:
			return "bug-solid.png";
		}
	}

	void load_icon(fa_icon icon)
	{
		const std::string image_path = fmt::format("home/32/%s", fa_icon_to_filename(icon));
		g_icons_cache[icon] = rsx::overlays::resource_config::load_icon(image_path);
	}

	const image_info* get_icon(fa_icon icon)
	{
		if (icon == fa_icon::none)
		{
			return nullptr;
		}

		std::lock_guard lock(g_icons_cache_lock);

		auto found = g_icons_cache.find(icon);
		if (found != g_icons_cache.end())
		{
			return found->second.get();
		}

		load_icon(icon);
		return g_icons_cache.at(icon).get();
	}
}
