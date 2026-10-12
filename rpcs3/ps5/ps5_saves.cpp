// PS5: the launcher's saves and their backups (ps5_saves.h).

#include "stdafx.h"
#include "ps5_saves.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Loader/PSF.h"
#include "Utilities/File.h"

#include <algorithm>
#include <ctime>
#include <map>

LOG_CHANNEL(saves_log, "Saves");

namespace ps5_saves
{
	namespace
	{
		bool skipped(const std::string& name)
		{
			// The folder itself, its parent, and this file's own work in progress
			return name.empty() || name == "." || name == ".." || name[0] == '.';
		}

		// A folder's files: their size, and the latest change
		void measure(const std::string& path, u64& size, s64& time)
		{
			for (const auto& entry : fs::dir(path))
			{
				if (skipped(entry.name))
				{
					continue;
				}
				if (entry.is_directory)
				{
					measure(path + entry.name + "/", size, time);
					continue;
				}
				size += entry.size;
				time = std::max(time, entry.mtime);
			}
		}

		// A copy of a folder, made from its files one by one
		bool copy_folder(const std::string& from, const std::string& to)
		{
			if (!fs::create_path(to))
			{
				return false;
			}
			for (const auto& entry : fs::dir(from))
			{
				if (skipped(entry.name))
				{
					continue;
				}
				if (entry.is_directory ? !copy_folder(from + entry.name + "/", to + entry.name + "/") : !fs::copy_file(from + entry.name, to + entry.name, true))
				{
					return false;
				}
			}
			return true;
		}

		// PARAM.SFO's words, where the folder has one
		void read_sfo(const std::string& path, save& into)
		{
			const psf::registry sfo = psf::load_object(path + "PARAM.SFO");
			if (sfo.empty())
			{
				return;
			}
			into.title = std::string(psf::get_string(sfo, "TITLE"));
			into.subtitle = std::string(psf::get_string(sfo, "SUB_TITLE"));
			into.detail = std::string(psf::get_string(sfo, "DETAIL"));
		}

		// "2026-10-12_21-50-07", in the console's local time
		std::string stamp()
		{
			const std::time_t now = std::time(nullptr);
			std::tm parts{};
			if (!localtime_r(&now, &parts))
			{
				gmtime_r(&now, &parts);
			}
			return fmt::format("%04d-%02d-%02d_%02d-%02d-%02d", parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday, parts.tm_hour, parts.tm_min, parts.tm_sec);
		}
	}

	std::string folder()
	{
		const std::string user = Emu.GetUsr().empty() ? std::string("00000001") : Emu.GetUsr();
		return rpcs3::utils::get_hdd0_dir() + "home/" + user + "/savedata/";
	}

	std::string backup_folder()
	{
		return fs::get_config_dir() + "save_backups/";
	}

	std::vector<save> read_all()
	{
		std::map<std::string, save> found;

		const std::string saves = folder();
		for (const auto& entry : fs::dir(saves))
		{
			if (skipped(entry.name) || !entry.is_directory)
			{
				continue;
			}
			save& item = found[entry.name];
			item.dir = entry.name;
			item.path = saves + entry.name + "/";
			read_sfo(item.path, item);
			if (fs::is_file(item.path + "ICON0.PNG"))
			{
				item.icon_path = item.path + "ICON0.PNG";
			}
			measure(item.path, item.size, item.time);
		}

		// Each save's backups, newest first; those of a save that is gone list it
		const std::string backups = backup_folder();
		for (const auto& entry : fs::dir(backups))
		{
			if (skipped(entry.name) || !entry.is_directory)
			{
				continue;
			}
			std::vector<backup> list;
			for (const auto& made : fs::dir(backups + entry.name + "/"))
			{
				if (skipped(made.name) || !made.is_directory)
				{
					continue;
				}
				backup item;
				item.path = backups + entry.name + "/" + made.name + "/";
				item.time = made.mtime;
				s64 latest = 0;
				measure(item.path, item.size, latest);
				list.push_back(std::move(item));
			}
			if (list.empty())
			{
				continue;
			}
			std::sort(list.begin(), list.end(), [](const backup& a, const backup& b) { return a.time > b.time; });

			save& item = found[entry.name];
			if (item.dir.empty())
			{
				// Gone: described by its newest backup
				item.dir = entry.name;
				read_sfo(list.front().path, item);
				if (fs::is_file(list.front().path + "ICON0.PNG"))
				{
					item.icon_path = list.front().path + "ICON0.PNG";
				}
				item.size = list.front().size;
				item.time = list.front().time;
			}
			item.backups = std::move(list);
		}

		std::vector<save> result;
		std::map<std::string, s64> latest; // each game's latest save
		for (auto& [name, item] : found)
		{
			item.serial = item.dir.substr(0, 9);
			latest[item.serial] = std::max(latest[item.serial], item.time);
			result.push_back(std::move(item));
		}
		std::sort(result.begin(), result.end(), [&](const save& a, const save& b)
		{
			if (a.serial != b.serial)
			{
				const s64 la = latest[a.serial], lb = latest[b.serial];
				return la != lb ? la > lb : a.serial < b.serial;
			}
			return a.time != b.time ? a.time > b.time : a.dir < b.dir;
		});
		return result;
	}

	std::string back_up(const save& target)
	{
		if (target.path.empty() || !fs::is_dir(target.path))
		{
			return "That save is not there any more.";
		}
		std::string to = backup_folder() + target.dir + "/" + stamp();
		for (u32 n = 2; fs::is_dir(to + "/"); n++)
		{
			to = backup_folder() + target.dir + "/" + stamp() + fmt::format("_%u", n);
		}
		to += "/";
		if (!copy_folder(target.path, to))
		{
			const std::string error = fmt::format("%s", fs::g_tls_error);
			fs::remove_all(to);
			saves_log.error("Could not back up %s to %s: %s", target.path, to, error);
			return "The backup could not be made (" + error + ").";
		}
		saves_log.success("Backed up %s to %s", target.path, to);
		return {};
	}

	std::string restore(const save& target, const backup& from)
	{
		// The save as it is, backed up first: a restore can be undone
		if (!target.path.empty() && fs::is_dir(target.path))
		{
			if (std::string error = back_up(target); !error.empty())
			{
				return error;
			}
		}

		// Copied beside it under another name, then swapped in, so a stop part
		// way leaves either the save or the backup's copy whole
		const std::string saves = folder();
		const std::string fresh = saves + ".restoring-" + target.dir + "/";
		const std::string old = saves + ".replaced-" + target.dir;
		const std::string place = saves + target.dir;
		fs::remove_all(fresh);
		fs::remove_all(old + "/");
		if (!copy_folder(from.path, fresh))
		{
			const std::string error = fmt::format("%s", fs::g_tls_error);
			fs::remove_all(fresh);
			saves_log.error("Could not restore %s from %s: %s", place, from.path, error);
			return "The backup could not be copied back (" + error + ").";
		}
		const bool had = fs::is_dir(place + "/");
		if (had && !fs::rename(place, old, false))
		{
			const std::string error = fmt::format("%s", fs::g_tls_error);
			fs::remove_all(fresh);
			return "The save could not be replaced (" + error + ").";
		}
		if (!fs::rename(fresh.substr(0, fresh.size() - 1), place, false))
		{
			const std::string error = fmt::format("%s", fs::g_tls_error);
			if (had)
			{
				fs::rename(old, place, false);
			}
			fs::remove_all(fresh);
			return "The save could not be replaced (" + error + ").";
		}
		if (had)
		{
			fs::remove_all(old + "/");
		}
		saves_log.success("Restored %s from %s", place, from.path);
		return {};
	}

	std::string remove(const save& target)
	{
		if (target.path.empty() || !fs::is_dir(target.path))
		{
			return "That save is not there any more.";
		}
		// Backed up first, so a save deleted by mistake can come back
		if (std::string error = back_up(target); !error.empty())
		{
			return error;
		}
		if (!fs::remove_all(target.path))
		{
			const std::string error = fmt::format("%s", fs::g_tls_error);
			saves_log.error("Could not delete %s: %s", target.path, error);
			return "The save could not be deleted (" + error + ").";
		}
		saves_log.success("Deleted %s (backed up first)", target.path);
		return {};
	}

	std::string remove_backups(const save& target)
	{
		const std::string path = backup_folder() + target.dir + "/";
		if (!fs::is_dir(path))
		{
			return "It has no backups.";
		}
		if (!fs::remove_all(path))
		{
			const std::string error = fmt::format("%s", fs::g_tls_error);
			saves_log.error("Could not delete %s: %s", path, error);
			return "The backups could not be deleted (" + error + ").";
		}
		saves_log.success("Deleted the backups in %s", path);
		return {};
	}
}
