#include "stdafx.h"
#include "cache_utils.hpp"
#include "system_utils.hpp"
#include "system_config.h"
#include "IdManager.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_log, "SYS");

namespace rpcs3::cache
{
	std::string get_ppu_cache()
	{
		const auto _main = g_fxo->try_get<main_ppu_module<lv2_obj>>();

		if (!_main || _main->cache.empty())
		{
			ppu_log.error("PPU Cache location not initialized.");
			return {};
		}

		return _main->cache;
	}

	std::string get_shader_cache()
	{
		const auto _main = g_fxo->try_get<main_ppu_module<lv2_obj>>();

		if (!_main || _main->cache.empty())
		{
			ppu_log.error("PPU Cache location not initialized.");
			return {};
		}

		// The shader cache does not depend on the executable, so it should survive game updates.
		// Store it next to the PPU-specific folders (cache/TITLEID/shaders_cache/) when a title id is available.
		// Otherwise (vsh, standalone ELFs, PS1 classics) keep it inside the PPU-specific folder.
		const std::string cache_root = rpcs3::utils::get_cache_dir();
		const std::string title_dir = rpcs3::utils::get_cache_dir(_main->path);

		if (title_dir == cache_root || title_dir == cache_root + "vsh/")
		{
			return _main->cache + "shaders_cache/";
		}

		const std::string shared_path = title_dir + "shaders_cache/";
		const std::string legacy_path = _main->cache + "shaders_cache/";

		// Reuse the existing cache of the current executable if there is no shared cache yet
		if (!fs::is_dir(shared_path) && fs::is_dir(legacy_path))
		{
			if (fs::rename(legacy_path, shared_path, false))
			{
				sys_log.notice("Moved shader cache '%s' to '%s'", legacy_path, shared_path);
			}
			else
			{
				sys_log.error("Failed to move shader cache '%s' to '%s' (%s)", legacy_path, shared_path, fs::g_tls_error);
				return legacy_path;
			}
		}

		return shared_path;
	}

	void limit_cache_size()
	{
		const std::string cache_location = rpcs3::utils::get_hdd1_dir() + "/caches";

		if (!fs::is_dir(cache_location))
		{
			sys_log.warning("Cache does not exist (%s)", cache_location);
			return;
		}

		const u64 size = fs::get_dir_size(cache_location);

		if (size == umax)
		{
			sys_log.error("Could not calculate cache directory '%s' size (%s)", cache_location, fs::g_tls_error);
			return;
		}

		const u64 max_size = static_cast<u64>(g_cfg.vfs.cache_max_size) * 1024 * 1024;

		if (max_size == 0) // Everything must go, so no need to do checks
		{
			fs::remove_all(cache_location, false);
			sys_log.success("Cleared disk cache");
			return;
		}

		if (size <= max_size)
		{
			sys_log.trace("Cache size below limit: %llu/%llu", size, max_size);
			return;
		}

		sys_log.success("Cleaning disk cache...");
		std::vector<fs::dir_entry> file_list{};
		fs::dir cache_dir(cache_location);
		if (!cache_dir)
		{
			sys_log.error("Could not open cache directory '%s' (%s)", cache_location, fs::g_tls_error);
			return;
		}

		// retrieve items to delete
		for (const auto &item : cache_dir)
		{
			if (item.name != "." && item.name != "..")
				file_list.push_back(item);
		}

		cache_dir.close();

		// sort oldest first
		std::sort(file_list.begin(), file_list.end(), FN(x.mtime < y.mtime));

		// keep removing until cache is empty or enough bytes have been cleared
		// cache is cleared down to 80% of limit to increase interval between clears
		const u64 to_remove = static_cast<u64>(size - max_size * 0.8);
		u64 removed = 0;
		for (const auto &item : file_list)
		{
			const std::string &name = cache_location + "/" + item.name;
			const bool is_dir = fs::is_dir(name);
			const u64 item_size = is_dir ? fs::get_dir_size(name) : item.size;

			if (is_dir && item_size == umax)
			{
				sys_log.error("Failed to calculate '%s' item '%s' size (%s)", cache_location, item.name, fs::g_tls_error);
				break;
			}

			if (is_dir ? !fs::remove_all(name, true, true) : !fs::remove_file(name))
			{
				sys_log.error("Could not remove cache directory '%s' item '%s' (%s)", cache_location, item.name, fs::g_tls_error);
				break;
			}

			removed += item_size;
			if (removed >= to_remove)
				break;
		}

		sys_log.success("Cleaned disk cache, removed %.2f MB", size / 1024.0 / 1024.0);
	}
}
