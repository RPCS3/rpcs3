// PS5: the launcher's record of each game (ps5_library.h).

#include "stdafx.h"
#include "ps5_library.h"

#include "Utilities/File.h"
#include "Emu/Cell/timers.hpp"

#include <ctime>
#include <map>
#include <mutex>
#include <sstream>

LOG_CHANNEL(library_log, "Launcher");

namespace ps5_library
{
	namespace
	{
		std::mutex g_mutex;
		bool g_loaded = false;
		bool g_dirty = false;
		u64 g_written_play_us = 0; // play time is written once a minute at most
		std::map<std::string, entry> g_entries;
		order g_order = order::recent;

		constexpr const char* c_order_names[] = {"recent", "name", "most_played", "favourites"};

		std::string path()
		{
			return fs::get_config_dir() + "library.txt";
		}

		void load()
		{
			if (g_loaded)
			{
				return;
			}
			g_loaded = true;
			const fs::file file(path());
			if (!file)
			{
				return;
			}
			std::istringstream lines(file.to_string());
			for (std::string line; std::getline(lines, line);)
			{
				std::istringstream words(line);
				std::string key;
				words >> key;
				if (key == "order")
				{
					std::string name;
					words >> name;
					for (u8 i = 0; i < static_cast<u8>(order::count); i++)
					{
						if (name == c_order_names[i])
						{
							g_order = static_cast<order>(i);
						}
					}
				}
				else if (key == "game")
				{
					std::string serial;
					words >> serial;
					if (serial.empty())
					{
						continue;
					}
					entry& e = g_entries[serial];
					for (std::string field; words >> field;)
					{
						u64 number = 0;
						words >> number;
						if (field == "favourite") e.favourite = number != 0;
						else if (field == "last") e.last_played = number;
						else if (field == "played") e.play_seconds = number;
					}
				}
			}
		}

		void save()
		{
			std::string text = "# PS5: the launcher's record of each game (ps5_library.h): favourite, last played (seconds since 1970), seconds played\n";
			text += fmt::format("order %s\n", c_order_names[static_cast<u8>(g_order)]);
			for (const auto& [serial, e] : g_entries)
			{
				fmt::append(text, "game %s favourite %u last %u played %u\n", serial, e.favourite ? 1 : 0, e.last_played, e.play_seconds);
			}
			fs::pending_file file(path());
			if (!file.file || file.file.write(text.data(), text.size()) < text.size() || !file.commit())
			{
				library_log.error("Could not write %s: %s", path(), fs::g_tls_error);
				return;
			}
			g_dirty = false;
		}
	}

	entry get(const std::string& serial)
	{
		std::lock_guard lock(g_mutex);
		load();
		const auto it = g_entries.find(serial);
		return it == g_entries.end() ? entry{} : it->second;
	}

	void set_favourite(const std::string& serial, bool favourite)
	{
		if (serial.empty())
		{
			return;
		}
		std::lock_guard lock(g_mutex);
		load();
		g_entries[serial].favourite = favourite;
		save();
	}

	void started(const std::string& serial)
	{
		if (serial.empty())
		{
			return;
		}
		std::lock_guard lock(g_mutex);
		load();
		g_entries[serial].last_played = static_cast<u64>(std::time(nullptr));
		save();
	}

	void add_play(const std::string& serial, u32 seconds)
	{
		if (serial.empty() || !seconds)
		{
			return;
		}
		std::lock_guard lock(g_mutex);
		load();
		g_entries[serial].play_seconds += seconds;
		g_dirty = true;
		if (const u64 now = get_system_time(); now - g_written_play_us >= 60'000'000)
		{
			g_written_play_us = now;
			save();
		}
	}

	order get_order()
	{
		std::lock_guard lock(g_mutex);
		load();
		return g_order;
	}

	void set_order(order next)
	{
		std::lock_guard lock(g_mutex);
		load();
		g_order = next;
		save();
	}

	const char* order_text(order kind)
	{
		switch (kind)
		{
		case order::recent: return "Recently played";
		case order::name: return "A to Z";
		case order::most_played: return "Most played";
		case order::favourites: return "Favourites first";
		default: return "";
		}
	}

	void flush()
	{
		std::lock_guard lock(g_mutex);
		if (g_loaded && g_dirty)
		{
			save();
		}
	}

	std::string play_text(u64 seconds)
	{
		if (seconds < 60)
		{
			return "under a minute";
		}
		const u64 minutes = seconds / 60;
		if (minutes < 60)
		{
			return fmt::format("%u min", minutes);
		}
		return minutes % 60 ? fmt::format("%u h %u min", minutes / 60, minutes % 60) : fmt::format("%u h", minutes / 60);
	}

	std::string date_text(u64 seconds)
	{
		if (!seconds)
		{
			return {};
		}
		const std::time_t when = static_cast<std::time_t>(seconds);
		std::tm parts{};
		if (!localtime_r(&when, &parts) && !gmtime_r(&when, &parts))
		{
			return {};
		}
		static constexpr const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		return fmt::format("%d %s %d", parts.tm_mday, months[std::clamp(parts.tm_mon, 0, 11)], parts.tm_year + 1900);
	}
}
