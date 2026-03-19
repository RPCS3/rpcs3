#include "stdafx.h"
#include "steam_utils.h"
#include "qt_utils.h"

#include "Loader/ISO.h"

#include <filesystem>
#include <map>
#include <QtConcurrent>
#include <QFutureWatcher>

#ifdef _WIN32
#include <Windows.h>
#include <tlhelp32.h>
#else
#include <signal.h>
#include <errno.h>
#endif

LOG_CHANNEL(sys_log, "SYS");

namespace gui::utils
{
	void steam_shortcut::add_shortcut(
			const std::string& app_name,
			const std::string& exe,
			const std::string& start_dir,
			const std::string& launch_options,
			const std::string& icon_path,
			const std::string& banner_small_path,
			const std::string& banner_large_path,
			std::shared_ptr<iso_archive> archive)
	{
		shortcut_entry entry{};
		entry.app_name = app_name;
		entry.exe = quote(fix_slashes(exe), true);
		entry.start_dir = quote(fix_slashes(start_dir), false);
		entry.launch_options = launch_options;
		entry.icon = quote(fix_slashes(icon_path), false);
		entry.banner_small = banner_small_path;
		entry.banner_large = banner_large_path;
		entry.appid = steam_appid(exe, app_name);
		entry.iso = archive;

		m_entries_to_add.push_back(std::move(entry));
	}

	void steam_shortcut::remove_shortcut(
		const std::string& app_name,
		const std::string& exe,
		const std::string& start_dir)
	{
		shortcut_entry entry{};
		entry.app_name = app_name;
		entry.exe = quote(fix_slashes(exe), true);
		entry.start_dir = quote(fix_slashes(start_dir), false);
		entry.appid = steam_appid(exe, app_name);

		m_entries_to_remove.push_back(std::move(entry));
	}

	bool steam_shortcut::parse_file(const std::string& path)
	{
		m_vdf_entries.clear();

		fs::file vdf(path);
		if (!vdf)
		{
			sys_log.error("Failed to parse steam shortcut file '%s': %s", path, fs::g_tls_error);
			return false;
		}

		const std::vector<u8> data = vdf.to_vector<u8>();
		usz last_pos = 0;
		usz pos = 0;

		const auto read_type_id = [&]() -> u8
		{
			if (pos >= data.size())
			{
				sys_log.error("Failed to parse steam shortcut file '%s' at pos 0x%x: read_type_id: end of file", path, pos);
				return umax;
			}

			last_pos = pos;
			return data[pos++];
		};

		const auto read_u32 = [&]() -> std::optional<u32>
		{
			if ((pos + sizeof(u32)) > data.size())
			{
				sys_log.error("Failed to parse steam shortcut file '%s' at pos 0x%x: read_u32: end of file", path, pos);
				return {};
			}

			last_pos = pos;

			u32 v {};
			std::memcpy(&v, &data[pos], sizeof(u32));
			pos += sizeof(u32);
			return v;
		};

		const auto read_string = [&]() -> std::optional<std::string>
		{
			if (pos >= data.size())
			{
				sys_log.error("Failed to parse steam shortcut file '%s' at pos 0x%x: read_string: end of file", path, pos);
				return {};
			}

			last_pos = pos;

			std::string str;

			while (pos < data.size())
			{
				const u8 c = data[pos++];
				if (!c) break;
				str += c;
			}

			if (pos >= data.size())
			{
				sys_log.error("Failed to parse steam shortcut file '%s' at pos 0x%x: read_string: not null terminated", path, last_pos);
				return {};
			}

			return str;
		};

		#define CHECK_VDF(cond, msg) \
			if (!(cond)) \
			{ \
				sys_log.error("Failed to parse steam shortcut file '%s' at pos 0x%x: %s", path, last_pos, msg); \
				return false; \
			}

		#define READ_VDF_STRING(name) \
			const std::optional<std::string> name##_opt = read_string(); \
			if (!name##_opt.has_value()) return false; \
			std::string name = name##_opt.value();

		#define READ_VDF_U32(name) \
			const std::optional<u32> name##_opt = read_u32(); \
			if (!name##_opt.has_value()) return false; \
			const u32 name = name##_opt.value();

		CHECK_VDF(read_type_id() == type_id::Start, "expected type_id::Start for shortcuts");
		READ_VDF_STRING(shortcuts);
		CHECK_VDF(shortcuts == "shortcuts", "expected 'shortcuts' key");

		for (usz index = 0; true; index++)
		{
			vdf_shortcut_entry entry {};

			u8 type = read_type_id();
			if (type == type_id::End)
			{
				// End of shortcuts
				break;
			}

			CHECK_VDF(type == type_id::Start, "expected type_id::Start for entry");

			READ_VDF_STRING(entry_index_str);
			u64 entry_index = 0;
			CHECK_VDF(try_to_uint64(&entry_index, entry_index_str, 0, umax), "failed to convert entry index");
			CHECK_VDF(entry_index == index, "unexpected entry index");

			type = umax;
			while (type != type_id::Start)
			{
				type = read_type_id();

				switch (type)
				{
				case type_id::String:
				{
					READ_VDF_STRING(key);
					READ_VDF_STRING(value);
					CHECK_VDF(!key.empty(), "key is empty");
					entry.values.push_back({std::move(key), std::move(value)});
					break;
				}
				case type_id::Integer:
				{
					READ_VDF_STRING(key);
					READ_VDF_U32(value);
					CHECK_VDF(!key.empty(), "key is empty");
					entry.values.push_back({std::move(key), value});
					break;
				}
				case type_id::Start:
					// Expect tags next
					break;
				default:
					sys_log.error("Failed to parse steam shortcut file '%s' at pos 0x%x: unexpected type id 0x%x", path, last_pos, type);
					return false;
				}
			}

			CHECK_VDF(type == type_id::Start, "expected type_id::Start for tags");
			READ_VDF_STRING(tags);
			CHECK_VDF(tags == "tags", "key is empty");
			type = umax;
			while (type != type_id::End)
			{
				type = read_type_id();

				switch (type)
				{
				case type_id::String:
				{
					READ_VDF_STRING(key);
					READ_VDF_STRING(value);
					CHECK_VDF(!key.empty(), "key is empty");
					entry.tags.push_back({std::move(key), std::move(value)});
					break;
				}
				case type_id::End:
					break;
				default:
					sys_log.error("Failed to parse steam shortcut file '%s' at pos 0x%x: unexpected type id 0x%x", path, last_pos, type);
					return false;
				}
			}
			CHECK_VDF(type == type_id::End, "expected type_id::End for tags");

			CHECK_VDF(read_type_id() == type_id::End, "expected type_id::End for entry");

			m_vdf_entries.push_back(std::move(entry));
		}

		CHECK_VDF(read_type_id() == type_id::End, "expected type_id::End for end of file");
		CHECK_VDF(pos == data.size(), fmt::format("bytes found at end of file (pos=%d, size=%d)", pos, data.size()));

		#undef CHECK_VDF_OPT
		#undef CHECK_VDF
		return true;
	}

	bool steam_shortcut::write_file()
	{
		if (m_entries_to_add.empty() && m_entries_to_remove.empty())
		{
			sys_log.error("Failed to create steam shortcut: No entries.");
			return false;
		}

		const std::string steam_path = get_steam_path();
		if (steam_path.empty())
		{
			sys_log.error("Failed to create steam shortcut: Steam directory not found.");
			return false;
		}

		if (!fs::is_dir(steam_path))
		{
			sys_log.error("Failed to create steam shortcut: '%s' not a directory.", steam_path);
			return false;
		}

		const std::string user_id = get_last_active_steam_user(steam_path);
		if (user_id.empty())
		{
			sys_log.error("Failed to create steam shortcut: last active user not found.");
			return false;
		}

		const std::string user_dir = steam_path + "/userdata/" + user_id + "/config/";
		if (!fs::is_dir(user_dir))
		{
			sys_log.error("Failed to create steam shortcut: '%s' not a directory.", user_dir);
			return false;
		}

		if (is_steam_running())
		{
			sys_log.error("Failed to create steam shortcut: steam is running.");
			return false;
		}

		const std::string file_path = user_dir + "shortcuts.vdf";
		const std::string backup_path = fs::get_config_dir() + "/shortcuts.vdf.backup";

		if (fs::is_file(file_path))
		{
			if (!fs::copy_file(file_path, backup_path, true))
			{
				sys_log.error("Failed to backup steam shortcut file '%s'", file_path);
				return false;
			}

			if (!parse_file(file_path))
			{
				sys_log.error("Failed to parse steam shortcut file '%s'", file_path);
				return false;
			}
		}

		std::vector<shortcut_entry> removed_entries;

		for (const shortcut_entry& entry : m_entries_to_remove)
		{
			bool removed_entry = false;
			for (auto it = m_vdf_entries.begin(); it != m_vdf_entries.end();)
			{
				const auto appid = it->value<u32>("appid");
				const auto exe = it->value<std::string>("Exe");
				const auto start_dir = it->value<std::string>("StartDir");

				if (appid.has_value() && appid.value() == entry.appid &&
					exe.has_value() && exe.value() == entry.exe &&
					start_dir.has_value() && start_dir.value() == entry.start_dir)
				{
					sys_log.notice("Removing steam shortcut for '%s'", entry.app_name);
					it = m_vdf_entries.erase(it);
					removed_entry = true;
				}
				else
				{
					it++;
				}
			}

			if (removed_entry)
			{
				removed_entries.push_back(entry);
			}

			if (m_vdf_entries.empty())
			{
				break;
			}
		}

		for (const vdf_shortcut_entry& entry : m_vdf_entries)
		{
			for (auto it = m_entries_to_add.begin(); it != m_entries_to_add.end();)
			{
				const auto appid = entry.value<u32>("appid");
				const auto exe = entry.value<std::string>("Exe");
				const auto start_dir = entry.value<std::string>("StartDir");
				const auto launch_options = entry.value<std::string>("LaunchOptions");
				const auto icon = entry.value<std::string>("icon");

				const bool appid_matches = appid.has_value() && appid.value() == it->appid;

				if (appid_matches &&
					exe.has_value() && exe.value() == it->exe &&
					start_dir.has_value() && start_dir.value() == it->start_dir &&
					launch_options.has_value() && launch_options.value() == it->launch_options &&
					icon.has_value() && icon.value() == it->icon)
				{
					sys_log.notice("Entry '%s' already exists in steam shortcut file '%s'.", it->app_name, file_path);
					it = m_entries_to_add.erase(it);
				}
				else
				{
					if (appid_matches)
					{
						sys_log.notice("Entry '%s' already exists in steam shortcut file '%s' but with other parameters. Creating an additional one...", it->app_name, file_path);
					}

					it++;
				}
			}

			if (m_entries_to_add.empty())
			{
				break;
			}
		}

		if (m_entries_to_add.empty() && removed_entries.empty())
		{
			sys_log.notice("Nothing to add or remove in steam shortcut file '%s'.", file_path);
			return true;
		}

		usz index = 0;
		std::string content;

		content += type_id::Start;
		append(content, "shortcuts");
		for (const vdf_shortcut_entry& entry : m_vdf_entries)
		{
			const auto val = entry.build_binary_blob(index++);
			if (!val.has_value())
			{
				sys_log.error("Failed to create steam shortcut '%s': '%s'", file_path, val.error());
				return false;
			}
			content += val.value();
		}
		for (const shortcut_entry& entry : m_entries_to_add)
		{
			content += entry.build_binary_blob(index++);
		}
		content += type_id::End;
		content += type_id::End; // End of file

		if (!fs::write_file(file_path, fs::rewrite, content))
		{
			sys_log.error("Failed to create steam shortcut '%s': '%s'", file_path, fs::g_tls_error);

			if (!fs::copy_file(backup_path, file_path, true))
			{
				sys_log.error("Failed to restore steam shortcuts backup: '%s'", fs::g_tls_error);
			}

			return false;
		}

		const std::string grid_dir = user_dir + "grid/";
		if (!fs::create_path(grid_dir))
		{
			sys_log.error("Failed to create steam shortcut grid dir '%s': '%s'", grid_dir, fs::g_tls_error);
		}

		QFutureWatcher<void>* future_watcher = new QFutureWatcher<void>();
		future_watcher->setFuture(QtConcurrent::map(m_entries_to_add, [&grid_dir](const shortcut_entry& entry)
		{
			std::string banner_small_path;
			std::string banner_large_path;

			const auto file_exists = [&entry](const std::string& path)
			{
				return entry.iso ? entry.iso->is_file(path) : fs::is_file(path);
			};

			for (const std::string& path : { entry.banner_small, entry.banner_large })
			{
				if (file_exists(path))
				{
					banner_small_path = path;
					break;
				}
			}

			for (const std::string& path : { entry.banner_large, entry.banner_small })
			{
				if (file_exists(path))
				{
					banner_large_path = path;
					break;
				}
			}

			if (QPixmap banner; load_icon(banner, banner_large_path, entry.iso ? entry.iso->path() : ""))
			{
				create_steam_banner(steam_banner::wide_cover, banner_large_path, banner, grid_dir, entry);
				create_steam_banner(steam_banner::background, banner_large_path, banner, grid_dir, entry);
			}
			else
			{
				sys_log.warning("Failed to load large steam banner file '%s'", banner_large_path);
			}

			if (QPixmap banner; load_icon(banner, banner_small_path, entry.iso ? entry.iso->path() : ""))
			{
				create_steam_banner(steam_banner::cover, banner_small_path, banner, grid_dir, entry);
				create_steam_banner(steam_banner::logo, banner_small_path, banner, grid_dir, entry);
				create_steam_banner(steam_banner::icon, banner_small_path, banner, grid_dir, entry);
			}
			else
			{
				sys_log.warning("Failed to load small steam banner file '%s'", banner_small_path);
			}
		}));

		future_watcher->waitForFinished();
		future_watcher->deleteLater();

		for (const shortcut_entry& entry : m_entries_to_add)
		{
			sys_log.success("Created steam shortcut for '%s'", entry.app_name);
		}

		for (const shortcut_entry& entry : removed_entries)
		{
			const auto remove_banner = [&entry, &grid_dir](steam_banner banner)
			{
				const std::string banner_path = get_steam_banner_path(banner, grid_dir, entry.appid);

				if (fs::is_file(banner_path))
				{
					if (fs::remove_file(banner_path))
					{
						sys_log.notice("Removed steam shortcut banner '%s'", banner_path);
					}
					else
					{
						sys_log.error("Failed to remove steam shortcut banner '%s': error='%s'", banner_path, fs::g_tls_error);
					}
				}
			};

			remove_banner(steam_banner::cover);
			remove_banner(steam_banner::wide_cover);
			remove_banner(steam_banner::background);
			remove_banner(steam_banner::logo);
			remove_banner(steam_banner::icon);

			sys_log.success("Removed steam shortcut(s) for '%s'", entry.app_name);
		}

		return true;
	}

	u32 steam_shortcut::crc32(const std::string& data)
	{
		u32 crc = 0xFFFFFFFF;

		for (u8 c : data)
		{
			crc ^= c;

			for (int i = 0; i < 8; i++)
			{
				crc = (crc >> 1) ^ (0xEDB88320 & -static_cast<int>(crc & 1));
			}
		}

		return ~crc;
	}

	bool steam_shortcut::steam_installed()
	{
		const std::string path = get_steam_path();
		return !path.empty() && fs::is_dir(path);
	}

	u32 steam_shortcut::steam_appid(const std::string& exe, const std::string& name)
	{
		return crc32(exe + name) | 0x80000000;
	}

	void steam_shortcut::append(std::string& s, const std::string& val)
	{
		s += val;
		s += '\0'; // append null terminator
	}

	std::string steam_shortcut::quote(const std::string& s, bool force)
	{
		if (force || s.contains(" "))
		{
			return "\"" + s + "\"";
		}

		return s;
	}

	std::string steam_shortcut::fix_slashes(const std::string& s)
	{
#ifdef _WIN32
		return fmt::replace_all(s, "/", "\\");
#else
		return s;
#endif
	}

	std::string steam_shortcut::kv_string(const std::string& key, const std::string& value)
	{
		std::string ret;
		ret += type_id::String;
		append(ret, key);
		append(ret, value);
		return ret;
	}

	std::string steam_shortcut::kv_int(const std::string& key, u32 value)
	{
		std::string str;
		str.reserve(64);
		str += type_id::Integer;
		append(str, key);
		str += static_cast<char>(value & 0xFF);
		str += static_cast<char>((value >> 8) & 0xFF);
		str += static_cast<char>((value >> 16) & 0xFF);
		str += static_cast<char>((value >> 24) & 0xFF);
		return str;
	}

	std::string steam_shortcut::shortcut_entry::build_binary_blob(usz index) const
	{
		std::string str;
		str.reserve(1024);
		str += type_id::Start;

		append(str, std::to_string(index));

		str += kv_int("appid", appid);
		str += kv_string("AppName", app_name);
		str += kv_string("Exe", exe);
		str += kv_string("StartDir", start_dir);
		str += kv_string("icon", icon);
		str += kv_string("ShortcutPath", "");
		str += kv_string("LaunchOptions", launch_options);
		str += kv_int("IsHidden", 0);
		str += kv_int("AllowDesktopConfig", 1);
		str += kv_int("AllowOverlay", 1);
		str += kv_int("OpenVR", 0);
		str += kv_int("Devkit", 0);
		str += kv_string("DevkitGameID", "");
		str += kv_int("DevkitOverrideAppID", 0);
		str += kv_int("LastPlayTime", 0);
		str += kv_string("FlatpakAppID", "");
		str += kv_string("sortas", "");

		str += type_id::Start;
		append(str, "tags");
		str += type_id::End;

		str += type_id::End;
		return str;
	}

	std::expected<std::string, std::string> steam_shortcut::vdf_shortcut_entry::build_binary_blob(usz index) const
	{
		std::string str;
		str.reserve(1024);
		str += type_id::Start;

		append(str, std::to_string(index));

		std::optional<std::string> error = std::nullopt;
		for (const auto& [key, value] : values)
		{
			std::visit([&key, &str, &error](const auto& value)
			{
				using T = std::decay_t<decltype(value)>;

				if constexpr (std::is_same_v<T, std::string>)
				{
					str += kv_string(key, value);
				}
				else if constexpr (std::is_same_v<T, u32>)
				{
					str += kv_int(key, value);
				}
				else
				{
					error = fmt::format("vdf entry for key '%s' has unexpected type '%s'", key, typeid(value).name());
				}
			},
			value);

			if (error.has_value())
			{
				return std::unexpected(error.value());
			}
		}

		str += type_id::Start;
		append(str, "tags");
		for (const auto& [key, value] : tags)
		{
			str += kv_string(key, value);
		}
		str += type_id::End;

		str += type_id::End;
		return str;
	}

#ifdef _WIN32
	std::string get_registry_string(const wchar_t* key, const wchar_t* name)
	{
		HKEY hkey = NULL;
		LSTATUS status = RegOpenKeyW(HKEY_CURRENT_USER, key, &hkey);
		if (status != ERROR_SUCCESS)
		{
			sys_log.trace("get_registry_string: RegOpenKeyW failed: %s (key='%s', name='%s')", fmt::win_error{static_cast<unsigned long>(status), nullptr}, wchar_to_utf8(key), wchar_to_utf8(name));
			return "";
		}

		std::wstring path_buffer;
		DWORD type = 0U;
		do
		{
			path_buffer.resize(path_buffer.size() + MAX_PATH);
			DWORD buffer_size = static_cast<DWORD>((path_buffer.size() - 1) * sizeof(wchar_t));
			status = RegQueryValueExW(hkey, name, NULL, &type, reinterpret_cast<LPBYTE>(path_buffer.data()), &buffer_size);
		}
		while (status == ERROR_MORE_DATA);

		const LSTATUS close_status = RegCloseKey(hkey);
		if (close_status != ERROR_SUCCESS)
		{
			sys_log.error("get_registry_string: RegCloseKey failed: %s (key='%s', name='%s')", fmt::win_error{static_cast<unsigned long>(close_status), nullptr}, wchar_to_utf8(key), wchar_to_utf8(name));
		}

		if (status != ERROR_SUCCESS)
		{
			sys_log.trace("get_registry_string: RegQueryValueExW failed: %s (key='%s', name='%s')", fmt::win_error{static_cast<unsigned long>(status), nullptr}, wchar_to_utf8(key), wchar_to_utf8(name));
			return "";
		}

		if ((type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ))
		{
			return wchar_to_utf8(path_buffer.data());
		}

		return "";
	}
#endif

	std::string steam_shortcut::steamid64_to_32(const std::string& steam_id)
	{
		u64 id = 0;
		if (!try_to_uint64(&id, steam_id, 0, umax))
		{
			sys_log.error("Failed to convert steam id '%s' to u64", steam_id);
			return "";
		}
		constexpr u64 base = 76561197960265728ULL;
		const u32 id32 = static_cast<u32>(id - base);
		return std::to_string(id32);
	}

	std::string steam_shortcut::get_steam_path()
	{
#ifdef _WIN32
		const std::string path = get_registry_string(L"Software\\Valve\\Steam", L"SteamPath");
		if (path.empty())
		{
			sys_log.notice("get_steam_path: SteamPath not found in registry");
			return "";
		}

		// The path might be lowercase... sigh
		std::error_code ec;
		const std::filesystem::path canonical_path = std::filesystem::canonical(std::filesystem::path(path), ec);
		if (ec)
		{
			sys_log.error("get_steam_path: Failed to canonicalize path '%s': %s", path, ec.message());
			return "";
		}

		const std::string path_fixed = canonical_path.string();
		sys_log.notice("get_steam_path: Found steam registry path: '%s'", path_fixed);
		return path_fixed;
#else
		if (const char* home = ::getenv("HOME"))
		{
#if __APPLE__
			const std::string path = std::string(home) + "/Library/Application Support/Steam/";
#else
			const std::string path = std::string(home) + "/.local/share/Steam/";
#endif
			return path;
		}

		return "";
#endif
	}

	std::string steam_shortcut::get_steam_banner_path(steam_banner banner, const std::string& grid_dir, u32 appid)
	{
		switch (banner)
		{
		case steam_banner::cover: return fmt::format("%s%dp.png", grid_dir, appid);
		case steam_banner::wide_cover: return fmt::format("%s%d.png", grid_dir, appid);
		case steam_banner::background: return fmt::format("%s%d_hero.png", grid_dir, appid);
		case steam_banner::logo: return fmt::format("%s%d_logo.png", grid_dir, appid);
		case steam_banner::icon: return fmt::format("%s%d_icon.png", grid_dir, appid);
		}

		return {};
	}

	void steam_shortcut::create_steam_banner(steam_banner banner, const std::string& src_path, const QPixmap& src_icon, const std::string& grid_dir, const shortcut_entry& entry)
	{
		const std::string dst_path = get_steam_banner_path(banner, grid_dir, entry.appid);
		QSize size = src_icon.size();

		if (banner == steam_banner::cover)
		{
			size = QSize(600, 900); // We want to center the icon vertically in the portrait
		}

		if (size == src_icon.size())
		{
			if (entry.iso)
			{
				// TODO: fs::copy_file does not support copy from an unmounted iso archive
				if (!src_icon.save(QString::fromStdString(dst_path)))
				{
					sys_log.error("Failed to save steam shortcut banner to '%s'", dst_path);
				}
				return;
			}
			else if (!fs::copy_file(src_path, dst_path, true))
			{
				sys_log.error("Failed to copy steam shortcut banner from '%s' to '%s': '%s'", src_path, dst_path, fs::g_tls_error);
			}
			return;
		}

		const QPixmap scaled = gui::utils::get_aligned_pixmap(src_icon, size, 1.0, Qt::SmoothTransformation, gui::utils::align_h::center, gui::utils::align_v::center);

		if (!scaled.save(QString::fromStdString(dst_path)))
		{
			sys_log.error("Failed to save steam shortcut banner to '%s'", dst_path);
		}
	}

	std::string steam_shortcut::get_last_active_steam_user(const std::string& steam_path)
	{
		const std::string vdf_path = steam_path + "/config/loginusers.vdf";
		fs::file vdf(vdf_path);
		if (!vdf)
		{
			sys_log.error("get_last_active_steam_user: Failed to parse steam loginusers file '%s': %s", vdf_path, fs::g_tls_error);
			return "";
		}

		// The file looks roughly like this. We need the numerical ID.
		// "users"
		// {
		//   "12345678901234567"
		//   {
		//     "AccountName" "myusername"
		//     "MostRecent" "1"
		//     ...
		//   }
		//   ...
		// }

		const std::string content = vdf.to_string();

		usz user_count = 0;

		const auto find_user_id = [&content, &user_count](const std::string& key, const std::string& comp) -> std::string
		{
			user_count = 0;
			usz pos = 0;
			while (true)
			{
				pos = content.find(key, pos);
				if (pos == umax) break;

				user_count++;

				const usz val_start = content.find('"', pos + key.size());
				if (val_start == umax) break;

				const usz val_end = content.find('"', val_start + 1);
				if (val_end == umax) break;

				const std::string value = content.substr(val_start + 1, val_end - val_start - 1);

				if (value != comp)
				{
					pos = val_end + 1;
					continue;
				}

				const usz pos_brace = content.rfind('{', pos - 2);
				if (pos_brace == umax) return "";

				const usz pos_end = content.rfind('"', pos_brace - 2);
				if (pos_end == umax) return "";

				const usz pos_start = content.rfind('"', pos_end - 1);
				if (pos_start == umax) return "";

				const std::string user_id_64 = content.substr(pos_start + 1, pos_end - pos_start - 1);
				return steamid64_to_32(user_id_64);
			}

			return "";
		};

		if (const std::string id = find_user_id("\"MostRecent\"", "1"); !id.empty())
		{
			return id;
		}

#ifdef _WIN32
		// Fallback to AutoLoginUser
		const std::string username = get_registry_string(L"Software\\Valve\\Steam", L"AutoLoginUser");
		if (username.empty())
		{
			sys_log.notice("get_last_active_steam_user: AutoLoginUser not found in registry");
			return "";
		}

		sys_log.notice("get_last_active_steam_user: Found steam user: '%s'", username);

		if (const std::string id = find_user_id("\"AccountName\"", username); !id.empty())
		{
			return id;
		}
#endif

		sys_log.error("get_last_active_steam_user: Failed to parse steam loginusers file '%s' (user_count=%d)", vdf_path, user_count);
		return "";
	}

	bool steam_shortcut::is_steam_running()
	{
#ifdef _WIN32
		if (HANDLE mutex = OpenMutexA(SYNCHRONIZE, FALSE, "Global\\SteamClientRunning"))
		{
			CloseHandle(mutex);
			return true;
		}

		// Fallback to check process
		PROCESSENTRY32 entry{};
		entry.dwSize = sizeof(entry);

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (Process32First(snapshot, &entry))
		{
			do
			{
				if (lstrcmpiW(entry.szExeFile, L"steam.exe") == 0)
				{
					CloseHandle(snapshot);
					return true;
				}
			} while (Process32Next(snapshot, &entry));
		}

		CloseHandle(snapshot);
#else
		std::vector<std::string> pid_paths = { get_steam_path() };
#ifdef __linux__
		if (const char* home = ::getenv("HOME"))
		{
			pid_paths.push_back(std::string(home) + "/.steam");
			pid_paths.push_back(std::string(home) + "/.steam/steam");
		}
#endif
		for (const std::string& pid_path : pid_paths)
		{
			if (fs::file pid_file(pid_path + "/steam.pid"); pid_file)
			{
				const std::string pid = pid_file.to_string();
				pid_file.close();

				if (pid.empty())
				{
					continue;
				}

				const pid_t pid_val = std::stoi(pid);
				return kill(pid_val, 0) == 0 || errno != ESRCH;
			}
		}
#endif
		return false;
	}
}
