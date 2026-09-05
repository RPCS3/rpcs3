#pragma once

#include "util/types.hpp"
#include "util/sysinfo.hpp"
#include "Utilities/mutex.h"
#include "System.h"
#include "system_utils.hpp"
#include "vfs_config.h"
#include "Loader/ISO.h"
#include "Loader/iso_cache.h"

#include <string>
#include <set>
#include <unordered_set>
#include <vector>

LOG_CHANNEL(sys_log, "SYS");

template <typename game_info_type>
class game_enumeration
{
public:
	struct path_entry
	{
		std::string path;
		bool is_from_yml{};
	};

	void set_localization(s32 index, std::string&& unknown, std::function<std::string(const std::string&)> get_localized_title);
	void set_show_custom_icons(bool enabled) { m_show_custom_icons = enabled; }
	void set_prefer_game_data_icons(bool enabled) { m_prefer_game_data_icons = enabled; }
	void set_play_hover_movies(bool enabled) { m_play_hover_movies = enabled; }
	void set_play_hover_music(bool enabled) { m_play_hover_music = enabled; }
	void set_canceled_callback(std::function<bool()> callback) { m_canceled_callback = std::move(callback); }

	void initialize_paths();
	void parse_directories();
	void parse_entry(const path_entry& entry);
	void apply_patches();

	void add_vfs_entry();

	void remove_duplicates();
	void clear(bool clear_iso_paths);
	void cleanup_iso_cache();

	const std::vector<path_entry>& path_entries() const { return m_path_entries; }
	const std::vector<game_info_type>& games() const { return m_games; }
	std::vector<game_info_type> take_games() { return std::move(m_games); }

private:
	// "shared_archive" is the archive the caller has already built for "dir_or_elf", if any: constructing another one
	// walks the whole file system of the disc again
	std::optional<game_info_type> get_game_info(const std::string& dir_or_elf, const std::string& game_dir, bool is_iso, bool is_raw, const std::shared_ptr<iso_archive>& shared_archive = {});

	bool was_canceled() const { return m_canceled_callback && m_canceled_callback(); }

	void push_path(const std::string& path, std::vector<std::string>& legit_paths);
	void add_game(const std::string& path, const std::string& game_dir = "PS3_GAME", bool is_iso = false, bool is_raw = false, const std::shared_ptr<iso_archive>& shared_archive = {});
	virtual void add_game_apply_extras([[maybe_unused]] game_info_type& game) {}
	void add_disc_dir(const std::string& path, std::vector<std::string>& legit_paths);

	shared_mutex m_games_mutex;
	std::vector<game_info_type> m_games;

	shared_mutex m_path_mutex;
	std::vector<path_entry> m_path_entries;
	std::set<std::string> m_path_list;
	std::unordered_set<std::string> m_scanned_iso_paths;

	std::function<bool()> m_canceled_callback {};

	std::string m_hdd0;
	std::string m_dev_flash;
	std::string m_game_icon_path;

	s32 m_language_index = 0;
	std::function<std::string(const std::string&)> m_get_localized_title;
	std::string m_localized_unknown;
	std::string m_localized_title;
	std::string m_localized_icon;
	std::string m_localized_movie;

	bool m_show_custom_icons = true;
	bool m_prefer_game_data_icons = true;
	bool m_play_hover_movies = true;
	bool m_play_hover_music = true;
};

template <typename game_info_type>
void game_enumeration<game_info_type>::initialize_paths()
{
	m_hdd0 = Emu.GetCallbacks().resolve_path(rpcs3::utils::get_hdd0_dir()) + '/';
	m_dev_flash = g_cfg_vfs.get_dev_flash();
	m_game_icon_path = fs::get_config_dir() + "/Icons/game_icons/";
}

template <typename game_info_type>
void game_enumeration<game_info_type>::set_localization(s32 index, std::string&& unknown, std::function<std::string(const std::string&)> get_localized_title)
{
	m_language_index = index;
	m_localized_unknown = std::move(unknown);
	m_get_localized_title = std::move(get_localized_title);
	m_localized_title = fmt::format("TITLE_%02d", m_language_index);
	m_localized_icon = fmt::format("ICON0_%02d.PNG", m_language_index);
	m_localized_movie = fmt::format("ICON1_%02d.PAM", m_language_index);
}

template <typename game_info_type>
std::optional<game_info_type> game_enumeration<game_info_type>::get_game_info(const std::string& dir_or_elf, const std::string& game_dir, bool is_iso, bool is_raw, const std::shared_ptr<iso_archive>& shared_archive)
{
	game_info_type info{};

	std::shared_ptr<iso_archive> archive;
	iso_metadata_cache_entry cache_entry{};
	bool is_raw_device = is_raw;
	// The caller provides the archive only for a path it has already recognized, so checking it again here would
	// read the volume descriptor of the disc once more ("is_raw_device" is the flag the caller passed in)
	info.is_iso_file = is_iso && (shared_archive || is_iso_file(dir_or_elf, nullptr, &is_raw_device));
	const bool is_ps3_game = game_dir == "PS3_GAME";

	if (info.is_iso_file)
	{
		const std::string iso_cache_key = is_ps3_game ? dir_or_elf : dir_or_elf + "//" + game_dir;
		// Only construct iso_archive (which walks the full directory tree) in case of raw device or
		// when no valid cache entry exists for this ISO path + mtime
		if (is_raw_device || !iso_cache::load(dir_or_elf, iso_cache_key, cache_entry))
		{
			// Reuse the archive the caller has already built for this path (a raw device never uses the cache)
			archive = shared_archive ? shared_archive : std::make_shared<iso_archive>(dir_or_elf);
			if (!archive->is_valid()) return std::nullopt;
		}

		// Track this ISO path for cache cleanup after scan completes.
		std::lock_guard lock(m_path_mutex);
		m_scanned_iso_paths.insert(dir_or_elf);
	}

	const auto file_exists = [&archive, &cache_entry](const std::string& path)
	{
		if (archive) return archive->is_file(path);
		// On cache hit, paths inside the ISO are not accessible via fs::is_file.
		// Return false here — cache hit paths are handled separately.
		if (!cache_entry.psf_data.empty()) return false;
		return fs::is_file(path);
	};

	info.path = dir_or_elf;
	info.game_dir = is_ps3_game ? "" : game_dir;

	const std::string sfo_dir = (archive || !cache_entry.psf_data.empty()) ? game_dir : rpcs3::utils::get_sfo_dir_from_game_path(dir_or_elf);
	const std::string sfo_path = sfo_dir + "/PARAM.SFO";

	// Load PSF: from archive on cache miss, rehydrate from cached SFO bytes on hit.
	psf::registry psf{};
	if (!cache_entry.psf_data.empty())
	{
		psf = psf::load_object(fs::make_stream<std::vector<u8>>(std::vector<u8>(cache_entry.psf_data)), sfo_path);
		// Fallback to archive scan if cached PSF is corrupted or missing critical fields.
		const bool psf_valid = !psf::get_string(psf, "TITLE_ID", "").empty()
			&& !psf::get_string(psf, "TITLE", "").empty()
			&& !psf::get_string(psf, "CATEGORY", "").empty();
		if (!psf_valid)
		{
			sys_log.warning("Cached psf for iso not valid: '%s'", info.path);
			archive = shared_archive ? shared_archive : std::make_shared<iso_archive>(dir_or_elf);
			if (!archive->is_valid()) return std::nullopt;

			cache_entry = {}; // Reset so the cache gets rewritten after scan.
			psf = {};
		}
	}

	if (psf.empty())
	{
		if (archive)
		{
			psf = archive->open_psf(sfo_path);
		}
		else
		{
			psf = psf::load_object(sfo_path);
		}
	}

	const std::string_view title_id = psf::get_string(psf, "TITLE_ID", "");

	if (title_id.empty())
	{
		if (!fs::is_file(dir_or_elf))
		{
			// Do not care about invalid entries
			return std::nullopt;
		}

		info.serial = dir_or_elf.substr(dir_or_elf.find_last_of(fs::delim) + 1);
		info.category = "/OS"; // Key for operating system executables
		info.version = utils::get_firmware_version();
		info.app_ver = info.version;
		info.fw = info.version;
		info.bootable = 1;
		info.icon_path = m_dev_flash + "vsh/resource/explore/icon/icon_home.png";

		if (dir_or_elf.starts_with(m_dev_flash))
		{
			std::string path_vfs = dir_or_elf.substr(m_dev_flash.size());

			if (const usz pos = path_vfs.find_first_not_of(fs::delim); pos != umax && pos != 0)
			{
				path_vfs = path_vfs.substr(pos);
			}

			ensure(m_get_localized_title);
			info.name = m_get_localized_title(path_vfs);
		}

		if (info.name.empty())
		{
			info.name = info.serial;
		}
	}
	else
	{
		std::string_view name = psf::get_string(psf, m_localized_title);
		if (name.empty()) name = psf::get_string(psf, "TITLE", m_localized_unknown);

		info.serial       = std::string(title_id);
		info.name         = std::string(name);
		info.app_ver      = std::string(psf::get_string(psf, "APP_VER", m_localized_unknown));
		info.version      = std::string(psf::get_string(psf, "VERSION", m_localized_unknown));
		info.category     = std::string(psf::get_string(psf, "CATEGORY", "Unknown"));
		info.fw           = std::string(psf::get_string(psf, "PS3_SYSTEM_VER", m_localized_unknown));
		info.parental_lvl = psf::get_integer(psf, "PARENTAL_LEVEL", 0);
		info.resolution   = psf::get_integer(psf, "RESOLUTION", 0);
		info.sound_format = psf::get_integer(psf, "SOUND_FORMAT", 0);
		info.bootable     = psf::get_integer(psf, "BOOTABLE", 0);
		info.attr         = psf::get_integer(psf, "ATTRIBUTE", 0);
	}

	if (m_show_custom_icons)
	{
		if (std::string icon_path = m_game_icon_path + info.serial + "/ICON0.PNG"; fs::is_file(icon_path))
		{
			info.icon_path = std::move(icon_path);
			info.has_custom_icon = true;
		}
	}

	if (info.icon_path.empty())
	{
		if (!cache_entry.icon_path.empty())
		{
			// Cache hit — icon path already resolved on a previous scan.
			info.icon_path = cache_entry.icon_path;
			info.icon_in_archive = true;
		}
		else if (std::string icon_path = sfo_dir + "/" + m_localized_icon; file_exists(icon_path))
		{
			info.icon_path = std::move(icon_path);
			info.icon_in_archive = archive && archive->exists(info.icon_path);
		}
		else
		{
			info.icon_path = sfo_dir + "/ICON0.PNG";
			info.icon_in_archive = archive && archive->exists(info.icon_path);
		}
	}

	if (m_play_hover_movies)
	{
		if (std::string movie_path = m_game_icon_path + info.serial + "/hover.gif"; file_exists(movie_path))
		{
			info.movie_path = std::move(movie_path);
		}
		else if (!cache_entry.movie_path.empty() && !archive)
		{
			// Cache hit — restore previously resolved movie path.
			info.movie_path = cache_entry.movie_path;
			info.movie_in_archive = true;
		}
		else if (std::string movie_path = sfo_dir + "/" + m_localized_movie; file_exists(movie_path))
		{
			info.movie_path = std::move(movie_path);
			info.movie_in_archive = archive && archive->exists(info.movie_path);
		}
		else if (std::string movie_path = sfo_dir + "/ICON1.PAM"; file_exists(movie_path))
		{
			info.movie_path = std::move(movie_path);
			info.movie_in_archive = archive && archive->exists(info.movie_path);
		}
	}

	if (m_play_hover_music)
	{
		if (!cache_entry.audio_path.empty() && !archive)
		{
			// Cache hit — restore previously resolved audio path.
			info.audio_path = cache_entry.audio_path;
			info.audio_in_archive = true;
		}
		else if (std::string audio_path = sfo_dir + "/SND0.AT3"; file_exists(audio_path))
		{
			info.audio_path = std::move(audio_path);
			info.audio_in_archive = archive && archive->exists(info.audio_path);
		}
	}

	// With the exception of raw device, on cache miss for an ISO, persist the resolved metadata so subsequent
	// launches skip iso_archive construction entirely
	if (archive && info.is_iso_file && !is_raw_device)
	{
		fs::stat_t iso_stat{};
		if (fs::get_stat(dir_or_elf, iso_stat))
		{
			cache_entry.mtime      = iso_stat.mtime;
			cache_entry.psf_data   = psf::save_object(psf);
			cache_entry.icon_path  = info.icon_path;
			cache_entry.movie_path = info.movie_path;
			cache_entry.audio_path = info.audio_path;

			// Cache raw icon bytes so load_iso_icon can skip archive open.
			if (info.icon_in_archive)
			{
				auto icon_file = archive->open(info.icon_path);

				if (icon_file && icon_file->size() > 0)
				{
					cache_entry.icon_data.resize(icon_file->size());
					icon_file->read(cache_entry.icon_data.data(), icon_file->size());
				}
			}

			iso_cache::save(dir_or_elf, is_ps3_game ? dir_or_elf : dir_or_elf + "//" + game_dir, cache_entry);
		}
	}

	return info;
}

template <typename game_info_type>
void game_enumeration<game_info_type>::push_path(const std::string& path, std::vector<std::string>& legit_paths)
{
	{
		std::lock_guard lock(m_path_mutex);
		if (!m_path_list.insert(path).second)
		{
			return;
		}
	}
	legit_paths.push_back(path);
}

template <typename game_info_type>
void game_enumeration<game_info_type>::add_game(const std::string& path, const std::string& game_dir, bool is_iso, bool is_raw, const std::shared_ptr<iso_archive>& shared_archive)
{
	if (std::optional<game_info_type> game = get_game_info(path, game_dir, is_iso, is_raw, shared_archive))
	{
		add_game_apply_extras(*game);

		std::lock_guard lock(m_games_mutex);
		m_games.push_back(std::move(*game));
	}
}

template <typename game_info_type>
void game_enumeration<game_info_type>::add_disc_dir(const std::string& path, std::vector<std::string>& legit_paths)
{
	for (const auto& entry : fs::dir(path))
	{
		if (was_canceled())
		{
			break;
		}

		if (!entry.is_directory || entry.name == "." || entry.name == "..")
		{
			continue;
		}

		if (entry.name == "PS3_GAME" || rpcs3::utils::is_ps3_gm_dir_name(entry.name))
		{
			push_path(path + "/" + entry.name, legit_paths);
		}
	}
}

template <typename game_info_type>
void game_enumeration<game_info_type>::parse_directories()
{
	const std::string hdd0_game = m_hdd0 + "game/";

	for (const auto& entry : fs::dir(hdd0_game))
	{
		if (was_canceled())
		{
			break;
		}

		if (!entry.is_directory || entry.name == "." || entry.name == "..")
		{
			continue;
		}

		std::lock_guard lock(m_path_mutex);
		m_path_entries.emplace_back(path_entry{hdd0_game + entry.name, false});
	}

	for (const auto& [serial, path] : Emu.GetGamesConfig().get_games())
	{
		if (was_canceled())
		{
			break;
		}

		std::string game_dir = path;
		game_dir.resize(game_dir.find_last_not_of('/') + 1);

		if (game_dir.empty() || path.starts_with(hdd0_game))
		{
			continue;
		}

		// Don't use the C00 subdirectory in our game list
		if (game_dir.ends_with("/C00") || game_dir.ends_with("\\C00"))
		{
			game_dir = game_dir.substr(0, game_dir.size() - 4);
		}

		std::lock_guard lock(m_path_mutex);
		m_path_entries.emplace_back(path_entry{game_dir, true});
	}
}

template <typename game_info_type>
void game_enumeration<game_info_type>::parse_entry(const path_entry& entry)
{
	std::vector<std::string> legit_paths;

	if (entry.is_from_yml)
	{
		bool is_raw_device = false;
		if (is_iso_file(entry.path, nullptr, &is_raw_device))
		{
			std::vector<std::string> subdirs;

			if (iso_cache::load_index(entry.path, subdirs))
			{
				for (const std::string& name : subdirs)
				{
					if (was_canceled()) break;
					add_game(entry.path, name, true, is_raw_device);
				}

				return;
			}

			// Shared with "add_game()" below, so that the file system of the disc is walked only once
			const auto archive = std::make_shared<iso_archive>(entry.path);
			if (!archive->is_valid()) return;

			const iso_fs_node& root = archive->root();

			for (const auto& child : root.children)
			{
				if (was_canceled())
				{
					break;
				}
				if (!child->metadata.is_directory)
				{
					continue;
				}

				const std::string& name = child->metadata.name;
				if (name == "PS3_GAME" || rpcs3::utils::is_ps3_gm_dir_name(name))
				{
					subdirs.push_back(name);
					add_game(entry.path, name, true, is_raw_device, archive);
				}
			}
			if (subdirs.empty())
			{
				subdirs.push_back("PS3_GAME");
				add_game(entry.path, "PS3_GAME", true, is_raw_device, archive);
			}
			if (!was_canceled())
			{
				iso_cache::save_index(entry.path, subdirs);
			}

			return;
		}
		else if (fs::is_file(entry.path + "/PARAM.SFO"))
		{
			push_path(entry.path, legit_paths);
		}
		else if (fs::is_file(entry.path + "/PS3_DISC.SFB"))
		{
			add_disc_dir(entry.path, legit_paths);
		}
		else
		{
			sys_log.trace("Invalid game path registered: %s", entry.path);
			return;
		}
	}
	else if (fs::is_file(entry.path + "/PS3_DISC.SFB"))
	{
		sys_log.error("Invalid game path found in %s", entry.path);
		return;
	}
	else
	{
		push_path(entry.path, legit_paths);
	}

	for (const std::string& path : legit_paths)
	{
		add_game(path);
	}
}

template <typename game_info_type>
void game_enumeration<game_info_type>::apply_patches()
{
	// Try to update the app version for disc games if there is a patch
	// Also try to find updated game icons and movies
	for (game_info_type& info : m_games)
	{
		if (info.category != "DG") continue;

		for (const game_info_type& other : m_games)
		{
			if (other.category == "DG") continue;
			if (info.serial != other.serial) continue;

			// The patch is game data and must have the same serial and an app version
			if (other.app_ver != m_localized_unknown)
			{
				// Update the app version if it's higher than the disc's version (old games may not have an app version)
				if (info.app_ver == m_localized_unknown || rpcs3::utils::version_is_bigger(other.app_ver, info.app_ver, info.serial, false))
				{
					info.app_ver = other.app_ver;
				}
				// Update the firmware version if possible and if it's higher than the disc's version
				if (other.fw != m_localized_unknown && rpcs3::utils::version_is_bigger(other.fw, info.fw, info.serial, true))
				{
					info.fw = other.fw;
				}
				// Update the parental level if possible and if it's higher than the disc's level
				if (other.parental_lvl != 0 && other.parental_lvl > info.parental_lvl)
				{
					info.parental_lvl = other.parental_lvl;
				}
			}

			// Let's fetch the game data icon if preferred or if the path was empty for some reason
			if ((m_prefer_game_data_icons && !info.has_custom_icon) || info.icon_path.empty())
			{
				if (std::string icon_path = other.path + "/" + m_localized_icon; fs::is_file(icon_path))
				{
					info.icon_path = std::move(icon_path);
					info.icon_in_archive = false;
				}
				else if (std::string icon_path = other.path + "/ICON0.PNG"; fs::is_file(icon_path))
				{
					info.icon_path = std::move(icon_path);
					info.icon_in_archive = false;
				}
			}

			// Let's fetch the game data movie if preferred or if the path was empty
			if (m_prefer_game_data_icons || info.movie_path.empty())
			{
				if (std::string movie_path = other.path + "/" + m_localized_icon; fs::is_file(movie_path))
				{
					info.movie_path = std::move(movie_path);
					info.movie_in_archive = false;
				}
				else if (std::string movie_path = other.path + "/ICON1.PAM"; fs::is_file(movie_path))
				{
					info.movie_path = std::move(movie_path);
					info.movie_in_archive = false;
				}
			}
		}
	}
}

template <typename game_info_type>
void game_enumeration<game_info_type>::add_vfs_entry()
{
	std::lock_guard lock(m_path_mutex);
	m_path_entries.emplace_back(path_entry{m_dev_flash + "vsh/module/vsh.self", false});
}

template <typename game_info_type>
void game_enumeration<game_info_type>::remove_duplicates()
{
	std::lock_guard lock(m_path_mutex);
	std::sort(m_path_entries.begin(), m_path_entries.end(), [](const path_entry& l, const path_entry& r){return l.path < r.path;});
	m_path_entries.erase(unique(m_path_entries.begin(), m_path_entries.end(), [](const path_entry& l, const path_entry& r){return l.path == r.path;}), m_path_entries.end());
}

template <typename game_info_type>
void game_enumeration<game_info_type>::clear(bool clear_iso_paths)
{
	std::scoped_lock lock(m_games_mutex, m_path_mutex);
	m_path_entries.clear();
	m_path_list.clear();
	m_games.clear();

	if (clear_iso_paths)
	{
		m_scanned_iso_paths.clear();
	}
}

template <typename game_info_type>
void game_enumeration<game_info_type>::cleanup_iso_cache()
{
	iso_cache::cleanup(m_scanned_iso_paths);
}
