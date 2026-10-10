// PS5: a game's controls remapped (ps5_pad_remap.h).

#include "stdafx.h"
#include "ps5_pad_remap.h"

#include "Utilities/File.h"

#include <mutex>
#include <sstream>
#include <sys/stat.h>

LOG_CHANNEL(remap_log, "PauseMenu");

namespace ps5_pad_remap
{
	namespace
	{
		// Each name as the file writes it, and as the menu shows it
		constexpr const char* c_keys[count + 1] = {"cross", "circle", "square", "triangle", "l1", "r1", "l2", "r2", "l3", "r3", "start", "select", "up", "down", "left", "right", "none"};
		constexpr const char* c_ps3[count] = {"Cross", "Circle", "Square", "Triangle", "L1", "R1", "L2", "R2", "L3", "R3", "Start", "Select", "Up", "Down", "Left", "Right"};
		constexpr const char* c_sources[count + 1] = {"Cross", "Circle", "Square", "Triangle", "L1", "R1", "L2", "R2", "L3", "R3", "OPTIONS", "Touch pad", "Up", "Down", "Left", "Right", "Nothing"};

		atomic_t<u8> g_active[count]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
		atomic_t<bool> g_held_off = false;

		std::string path(const std::string& serial)
		{
			return fs::get_config_dir() + "controls/" + serial + ".txt";
		}

		u8 key_index(std::string_view key)
		{
			for (u8 i = 0; i <= count; i++)
			{
				if (key == c_keys[i])
				{
					return i;
				}
			}
			return 0xff;
		}
	}

	const char* ps3_name(u8 button)
	{
		return button < count ? c_ps3[button] : "";
	}

	const char* source_name(u8 source)
	{
		return source <= count ? c_sources[source] : "";
	}

	map identity()
	{
		map result{};
		for (u8 i = 0; i < count; i++)
		{
			result[i] = i;
		}
		return result;
	}

	bool is_identity(const map& m)
	{
		return m == identity();
	}

	map load(const std::string& serial)
	{
		map result = identity();
		if (serial.empty())
		{
			return result;
		}
		const fs::file file(path(serial));
		if (!file)
		{
			return result;
		}
		std::istringstream lines(file.to_string());
		for (std::string line; std::getline(lines, line);)
		{
			std::istringstream words(line);
			std::string button, source;
			words >> button >> source;
			const u8 b = key_index(button), s = key_index(source);
			if (b < count && s <= count)
			{
				result[b] = s;
			}
		}
		return result;
	}

	void save(const std::string& serial, const map& m)
	{
		if (serial.empty())
		{
			return;
		}
		if (is_identity(m))
		{
			fs::remove_file(path(serial));
			return;
		}
		std::string text = "# PS5: this game's controls (ps5_pad_remap.h): each of the PS3's buttons, then the controller's button that presses it\n";
		for (u8 i = 0; i < count; i++)
		{
			if (m[i] != i)
			{
				fmt::append(text, "%s %s\n", c_keys[i], c_keys[std::min<u8>(m[i], count)]);
			}
		}
		const std::string dir = fs::get_config_dir() + "controls/";
		fs::create_path(dir);
		::chmod(dir.c_str(), 0777);
		if (!fs::write_file(path(serial), fs::rewrite, text))
		{
			remap_log.error("Could not write %s: %s", path(serial), fs::g_tls_error);
			return;
		}
		::chmod(path(serial).c_str(), 0666);
	}

	void set_active(const map& m)
	{
		for (u8 i = 0; i < count; i++)
		{
			g_active[i] = m[i];
		}
	}

	map active()
	{
		map result{};
		for (u8 i = 0; i < count; i++)
		{
			result[i] = g_active[i];
		}
		return result;
	}

	void hold_off(bool held)
	{
		g_held_off = held;
	}

	bool held_off()
	{
		return g_held_off;
	}
}
