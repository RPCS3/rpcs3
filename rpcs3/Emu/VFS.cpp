#include "stdafx.h"
#include "IdManager.h"
#include "System.h"
#include "VFS.h"

#include "Cell/lv2/sys_fs.h"

#include "Utilities/mutex.h"
#include "Utilities/StrUtil.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <thread>
#include <map>

LOG_CHANNEL(vfs_log, "VFS");

struct vfs_directory
{
	// Real path (empty if root or not exists)
	std::string path;

	// Virtual subdirectories
	std::map<std::string, std::unique_ptr<vfs_directory>> dirs;
};

struct vfs_manager
{
	shared_mutex mutex{};

	// VFS root
	vfs_directory root{};
};

bool vfs::mount(std::string_view vpath, std::string_view path, bool is_dir)
{
	if (vpath.empty())
	{
		// Empty relative path, should set relative path base; unsupported
		vfs_log.error("Cannot mount empty path to \"%s\"", path);
		return false;
	}

	// Initialize vfs_manager if not yet initialized (e.g. g_fxo->reset() was previously invoked)
	g_fxo->need<vfs_manager>();

	auto& table = g_fxo->get<vfs_manager>();

	// TODO: scan roots of mounted devices for undeleted vfs::host::unlink remnants, and try to delete them (_WIN32 only)

	std::lock_guard lock(table.mutex);

	const std::string_view vpath_backup = vpath;

	for (std::vector<vfs_directory*> list{&table.root};;)
	{
		// Skip one or more '/'
		const auto pos = vpath.find_first_not_of('/');

		if (pos == 0)
		{
			// Mounting relative path is not supported
			vfs_log.error("Cannot mount relative path \"%s\" to \"%s\"", vpath_backup, path);
			return false;
		}

		if (pos == umax)
		{
			// Mounting completed; fixup for directories due to resolve_path messing with trailing /
			list.back()->path = Emu.GetCallbacks().resolve_path(path);
			if (list.back()->path.empty())
				list.back()->path = std::string(path); // Fallback when resolving failed
			if (is_dir && !list.back()->path.ends_with('/'))
				list.back()->path += '/';
			if (!is_dir && list.back()->path.ends_with('/'))
				vfs_log.error("File mounted with trailing /.");

			if (path == "/") // Special
				list.back()->path = "/";

			vfs_log.notice("Mounted path \"%s\" to \"%s\"", vpath_backup, list.back()->path);
			return true;
		}

		// Get fragment name
		const auto name = vpath.substr(pos, vpath.find_first_of('/', pos) - pos);
		vpath.remove_prefix(name.size() + pos);

		if (name == ".")
		{
			// Keep current
			continue;
		}

		if (name == "..")
		{
			// Root parent is root
			if (list.size() == 1)
			{
				continue;
			}

			// Go back one level
			list.pop_back();
			continue;
		}

		// Find or add
		vfs_directory* last = list.back();

		for (auto& [path, dir] : last->dirs)
		{
			if (path == name)
			{
				list.push_back(dir.get());
				break;
			}
		}

		if (last == list.back())
		{
			// Add new entry
			std::unique_ptr<vfs_directory> new_entry = std::make_unique<vfs_directory>();
			list.push_back(new_entry.get());
			last->dirs.emplace(name, std::move(new_entry));
		}
	}
}

bool vfs::unmount(std::string_view vpath)
{
	if (vpath.empty())
	{
		vfs_log.error("Cannot unmount empty path");
		return false;
	}

	const std::vector<std::string> entry_list = fmt::split(vpath, {"/"});

	if (entry_list.empty())
	{
		vfs_log.error("Cannot unmount path: '%s'", vpath);
		return false;
	}

	vfs_log.notice("About to unmount '%s'", vpath);

	if (!g_fxo->is_init<vfs_manager>())
	{
		return false;
	}

	auto& table = g_fxo->get<vfs_manager>();

	std::lock_guard lock(table.mutex);

	// Search entry recursively and remove it (including all children)
	std::function<void(vfs_directory&, usz)> unmount_children;
	unmount_children = [&entry_list, &unmount_children](vfs_directory& dir, usz depth) -> void
	{
		if (depth >= entry_list.size())
		{
			return;
		}

		// Get the current name based on the depth
		const std::string& name = ::at32(entry_list, depth);

		// Go through all children of this node
		for (auto it = dir.dirs.begin(); it != dir.dirs.end();)
		{
			// Find the matching node
			if (it->first == name)
			{
				// Remove the matching node if we reached the maximum depth
				if (depth + 1 == entry_list.size())
				{
					vfs_log.notice("Unmounting '%s' = '%s'", it->first, it->second->path);
					it = dir.dirs.erase(it);
					continue;
				}

				// Otherwise continue searching in the next level of depth
				unmount_children(*it->second, depth + 1);
			}

			++it;
		}
	};
	unmount_children(table.root, 0);

	return true;
}

std::string vfs::get(std::string_view vpath, std::vector<std::string>* out_dir, std::string* out_path)
{
	// Just to make the code more robust.
	// It should never happen because we take care to initialize Emu (and so also vfs_manager) with Emu.Init() before this function is invoked
	if (!g_fxo->is_init<vfs_manager>())
	{
		fmt::throw_exception("vfs_manager not initialized");
	}

	auto& table = g_fxo->get<vfs_manager>();

	reader_lock lock(table.mutex);

	// Resulting path fragments: decoded ones
	std::vector<std::string_view> result;
	result.reserve(vpath.size() / 2);

	// Mounted path
	std::string_view result_base;

	if (vpath.empty())
	{
		// Empty relative path (reuse further return)
		vpath = ".";
	}

	// Fragments for out_path
	std::vector<std::string_view> name_list;

	if (out_path)
	{
		name_list.reserve(vpath.size() / 2);
	}

	for (std::vector<const vfs_directory*> list{&table.root};;)
	{
		// Skip one or more '/'
		const auto pos = vpath.find_first_not_of('/');

		if (pos == 0)
		{
			// Relative path: point to non-existent location
			return fs::get_config_dir() + "delete_this_dir.../delete_this...";
		}

		if (pos == umax)
		{
			// Absolute path: finalize
			for (auto it = list.rbegin(), rend = list.rend(); it != rend; it++)
			{
				if (auto* dir = *it; dir && (!dir->path.empty() || list.size() == 1))
				{
					// Save latest valid mount path
					result_base = dir->path;

					// Erase unnecessary path fragments
					result.erase(result.begin(), result.begin() + (std::distance(it, rend) - 1));

					// Extract mounted subdirectories (TODO)
					if (out_dir)
					{
						for (auto& pair : dir->dirs)
						{
							if (!pair.second->path.empty())
							{
								out_dir->emplace_back(pair.first);
							}
						}
					}

					break;
				}
			}

			if (!vpath.empty())
			{
				// Finalize path with '/'
				result.emplace_back("");
			}

			break;
		}

		// Get fragment name
		const auto name = vpath.substr(pos, vpath.find_first_of('/', pos) - pos);
		vpath.remove_prefix(name.size() + pos);

		// Process special directories
		if (name == ".")
		{
			// Keep current
			continue;
		}

		if (name == "..")
		{
			// Root parent is root
			if (list.size() == 1)
			{
				continue;
			}

			// Go back one level
			if (out_path)
			{
				name_list.pop_back();
			}

			list.pop_back();
			result.pop_back();
			continue;
		}

		const auto last = list.back();
		list.push_back(nullptr);

		if (out_path)
		{
			name_list.push_back(name);
		}

		result.push_back(name);

		if (!last)
		{
			continue;
		}

		for (auto& [path, dir] : last->dirs)
		{
			if (path == name)
			{
				list.back() = dir.get();

				if (dir->path == "/"sv)
				{
					if (vpath.size() <= 1)
					{
						return fs::get_config_dir() + "delete_this_dir.../delete_this...";
					}

					// Handle /host_root (not escaped, not processed)
					if (out_path)
					{
						out_path->clear();
						*out_path += '/';
						*out_path += fmt::merge(name_list, "/");
						*out_path += vpath;
					}

					return std::string{vpath.substr(1)};
				}

				break;
			}
		}
	}

	if (result_base.empty())
	{
		// Not mounted
		return {};
	}

	// Merge path fragments
	if (out_path)
	{
		out_path->clear();
		*out_path += '/';
		*out_path += fmt::merge(name_list, "/");
	}

	// Escape for host FS
	std::vector<std::string> escaped;
	escaped.reserve(result.size());
	for (auto& sv : result)
		escaped.emplace_back(vfs::escape(sv));

	return std::string{result_base} + fmt::merge(escaped, "/");
}

using char2 = char8_t;

std::string vfs::retrieve(std::string_view path, const vfs_directory* node, std::vector<std::string_view>* mount_path)
{
	// Just to make the code more robust.
	// It should never happen because we take care to initialize Emu (and so also vfs_manager) with Emu.Init() before this function is invoked
	if (!g_fxo->is_init<vfs_manager>())
	{
		fmt::throw_exception("vfs_manager not initialized");
	}

	auto& table = g_fxo->get<vfs_manager>();

	if (!node)
	{
		if (path.starts_with(".") || path.empty())
		{
			return {};
		}

		reader_lock lock(table.mutex);

		std::vector<std::string_view> mount_path_empty;

		const std::string rpath = Emu.GetCallbacks().resolve_path(path);

		if (!rpath.empty())
		{
			if (std::string res = vfs::retrieve(rpath, &table.root, &mount_path_empty); !res.empty())
			{
				return res;
			}
		}

		mount_path_empty.clear();

		return vfs::retrieve(path, &table.root, &mount_path_empty);
	}

	mount_path->emplace_back();

	// Try to extract host root mount point name (if exists)
	std::string_view host_root_name;

	std::string result;
	std::string result_dir;

	for (const auto& [name, dir] : node->dirs)
	{
		mount_path->back() = name;

		if (std::string res = vfs::retrieve(path, dir.get(), mount_path); !res.empty())
		{
			// Avoid app_home
			// Prefer dev_bdvd over dev_hdd0
			if (result.empty() || (name == "app_home") < (result_dir == "app_home") ||
				(name == "dev_bdvd") > (result_dir == "dev_bdvd"))
			{
				result = std::move(res);
				result_dir = name;
			}
		}

		if (dir->path == "/"sv)
		{
			host_root_name = name;
		}
	}

	if (!result.empty())
	{
		return result;
	}

	mount_path->pop_back();

	if (node->path.size() > 1 && path.starts_with(node->path))
	{
		auto unescape_path = [](std::string_view path)
		{
			// Unescape from host FS
			std::vector<std::string> escaped = fmt::split(path, {std::string_view{&fs::delim[0], 1}, std::string_view{&fs::delim[1], 1}});
			std::vector<std::string> result;
			for (auto& sv : escaped)
				result.emplace_back(vfs::unescape(sv));

			return fmt::merge(result, "/");
		};

		std::string result{"/"};

		for (const auto& name : *mount_path)
		{
			result += name;
			result += '/';
		}

		result += unescape_path(path.substr(node->path.size()));
		return result;
	}

	if (!host_root_name.empty())
	{
		// If failed to find mount point for path and /host_root is mounted
		// Prepend "/host_root" to path and return the constructed string
		result.clear();
		result += '/';

		for (const auto& name : *mount_path)
		{
			result += name;
			result += '/';
		}

		result += host_root_name;
		result += '/';

		result += path;
		return result;
	}

	return result;
}

std::string vfs::escape(std::string_view name, bool escape_slash)
{
	std::string result;

	if (name.size() <= 2 && name.find_first_not_of('.') == umax)
	{
		// Return . or .. as is
		result = name;
		return result;
	}

	// Emulate NTS (limited)
	auto get_char = [&](usz pos) -> char2
	{
		if (pos < name.size())
		{
			return name[pos];
		}
		else
		{
			return '\0';
		}
	};

	// Escape NUL, LPT ant other trash
	if (name.size() > 2)
	{
		// Pack first 3 characters
		const u32 triple = std::bit_cast<le_t<u32>, u32>(toupper(name[0]) | toupper(name[1]) << 8 | toupper(name[2]) << 16);

		switch (triple)
		{
		case "COM"_u32:
		case "LPT"_u32:
		{
			if (name.size() >= 4 && name[3] >= '1' && name[3] <= '9')
			{
				if (name.size() == 4 || name[4] == '.')
				{
					// Escape first character (C or L)
					result = reinterpret_cast<const char*>(u8"！");
				}
			}

			break;
		}
		case "NUL"_u32:
		case "CON"_u32:
		case "AUX"_u32:
		case "PRN"_u32:
		{
			if (name.size() == 3 || name[3] == '.')
			{
				result = reinterpret_cast<const char*>(u8"！");
			}

			break;
		}
		}
	}

	result.reserve(result.size() + name.size());

	for (usz i = 0, s = name.size(); i < s; i++)
	{
		switch (char2 c = name[i])
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		{
			result += reinterpret_cast<const char*>(u8"０");
			result.back() += c;
			break;
		}
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		{
			result += reinterpret_cast<const char*>(u8"Ａ");
			result.back() += c;
			result.back() -= 10;
			break;
		}
		case '<':
		{
			result += reinterpret_cast<const char*>(u8"＜");
			break;
		}
		case '>':
		{
			result += reinterpret_cast<const char*>(u8"＞");
			break;
		}
		case ':':
		{
			result += reinterpret_cast<const char*>(u8"：");
			break;
		}
		case '"':
		{
			result += reinterpret_cast<const char*>(u8"＂");
			break;
		}
		case '\\':
		{
			result += reinterpret_cast<const char*>(u8"＼");
			break;
		}
		case '|':
		{
			result += reinterpret_cast<const char*>(u8"｜");
			break;
		}
		case '?':
		{
			result += reinterpret_cast<const char*>(u8"？");
			break;
		}
		case '*':
		{
			result += reinterpret_cast<const char*>(u8"＊");
			break;
		}
		case '/':
		{
			if (escape_slash)
			{
				result += reinterpret_cast<const char*>(u8"／");
				break;
			}

			result += c;
			break;
		}
		case '.':
		case ' ':
		{
			if (!get_char(i + 1))
			{
				switch (c)
				{
				// Directory name ended with a space or a period, not allowed on Windows.
				case '.': result += reinterpret_cast<const char*>(u8"．"); break;
				case ' ': result += reinterpret_cast<const char*>(u8"＿"); break;
				}

				break;
			}

			result += c;
			break;
		}
		case char2{u8"！"[0]}:
		{
			// Escape full-width characters 0xFF01..0xFF5e with ！ (0xFF01)
			switch (get_char(i + 1))
			{
			case char2{u8"！"[1]}:
			{
				const uchar c3 = get_char(i + 2);

				if (c3 >= 0x81 && c3 <= 0xbf)
				{
					result += reinterpret_cast<const char*>(u8"！");
				}

				break;
			}
			case char2{u8"｀"[1]}:
			{
				const uchar c3 = get_char(i + 2);

				if (c3 >= 0x80 && c3 <= 0x9e)
				{
					result += reinterpret_cast<const char*>(u8"！");
				}

				break;
			}
			default: break;
			}

			result += c;
			break;
		}
		default:
		{
			result += c;
			break;
		}
		}
	}

	return result;
}

std::string vfs::unescape(std::string_view name)
{
	std::string result;
	result.reserve(name.size());

	// Emulate NTS
	auto get_char = [&](usz pos) -> char2
	{
		if (pos < name.size())
		{
			return name[pos];
		}
		else
		{
			return '\0';
		}
	};

	for (usz i = 0, s = name.size(); i < s; i++)
	{
		switch (char2 c = name[i])
		{
		case char2{u8"！"[0]}:
		{
			switch (get_char(i + 1))
			{
			case char2{u8"！"[1]}:
			{
				const uchar c3 = get_char(i + 2);

				if (c3 >= 0x81 && c3 <= 0xbf)
				{
					switch (static_cast<char2>(c3))
					{
					case char2{u8"０"[2]}:
					case char2{u8"１"[2]}:
					case char2{u8"２"[2]}:
					case char2{u8"３"[2]}:
					case char2{u8"４"[2]}:
					case char2{u8"５"[2]}:
					case char2{u8"６"[2]}:
					case char2{u8"７"[2]}:
					case char2{u8"８"[2]}:
					case char2{u8"９"[2]}:
					{
						result += static_cast<char>(c3);
						result.back() -= u8"０"[2];
						continue;
					}
					case char2{u8"Ａ"[2]}:
					case char2{u8"Ｂ"[2]}:
					case char2{u8"Ｃ"[2]}:
					case char2{u8"Ｄ"[2]}:
					case char2{u8"Ｅ"[2]}:
					case char2{u8"Ｆ"[2]}:
					case char2{u8"Ｇ"[2]}:
					case char2{u8"Ｈ"[2]}:
					case char2{u8"Ｉ"[2]}:
					case char2{u8"Ｊ"[2]}:
					case char2{u8"Ｋ"[2]}:
					case char2{u8"Ｌ"[2]}:
					case char2{u8"Ｍ"[2]}:
					case char2{u8"Ｎ"[2]}:
					case char2{u8"Ｏ"[2]}:
					case char2{u8"Ｐ"[2]}:
					case char2{u8"Ｑ"[2]}:
					case char2{u8"Ｒ"[2]}:
					case char2{u8"Ｓ"[2]}:
					case char2{u8"Ｔ"[2]}:
					case char2{u8"Ｕ"[2]}:
					case char2{u8"Ｖ"[2]}:
					{
						result += static_cast<char>(c3);
						result.back() -= u8"Ａ"[2];
						result.back() += 10;
						continue;
					}
					case char2{u8"！"[2]}:
					{
						if (const char2 c4 = get_char(i + 3))
						{
							// Escape anything but null character
							result += c4;
						}
						else
						{
							return result;
						}

						i += 3;
						continue;
					}
					case char2{u8"＿"[2]}:
					{
						result += ' ';
						break;
					}
					case char2{u8"．"[2]}:
					{
						result += '.';
						break;
					}
					case char2{u8"＜"[2]}:
					{
						result += '<';
						break;
					}
					case char2{u8"＞"[2]}:
					{
						result += '>';
						break;
					}
					case char2{u8"："[2]}:
					{
						result += ':';
						break;
					}
					case char2{u8"＂"[2]}:
					{
						result += '"';
						break;
					}
					case char2{u8"＼"[2]}:
					{
						result += '\\';
						break;
					}
					case char2{u8"？"[2]}:
					{
						result += '?';
						break;
					}
					case char2{u8"＊"[2]}:
					{
						result += '*';
						break;
					}
					case char2{u8"＄"[2]}:
					{
						if (i == 0)
						{
							// Special case: filename starts with full-width $ likely created by vfs::host::unlink
							result.resize(1, '.');
							return result;
						}

						[[fallthrough]];
					}
					default:
					{
						// Unrecognized character (ignored)
						break;
					}
					}

					i += 2;
				}
				else
				{
					result += c;
				}

				break;
			}
			case char2{u8"｀"[1]}:
			{
				const uchar c3 = get_char(i + 2);

				if (c3 >= 0x80 && c3 <= 0x9e)
				{
					switch (static_cast<char2>(c3))
					{
					case char2{u8"｜"[2]}:
					{
						result += '|';
						break;
					}
					default:
					{
						// Unrecognized character (ignored)
						break;
					}
					}

					i += 2;
				}
				else
				{
					result += c;
				}

				break;
			}
			default:
			{
				result += c;
				break;
			}
			}
			break;
		}
		case 0:
		{
			// NTS detected
			return result;
		}
		default:
		{
			result += c;
			break;
		}
		}
	}

	return result;
}

std::string vfs::host::hash_path(const std::string& path, const std::string& dev_root, std::string_view prefix)
{
	return fmt::format(u8"%s/＄%s%s%s", dev_root, fmt::base57(std::hash<std::string>()(path)), prefix, fmt::base57(utils::get_unique_tsc()));
}

bool vfs::host::rename(const std::string& from, const std::string& to, const lv2_fs_mount_point* mp, bool overwrite, bool lock)
{
	// Lock mount point, close file descriptors, retry
	const auto from0 = std::string_view(from).substr(0, from.find_last_not_of(fs::delim) + 1);

	std::vector<std::pair<shared_ptr<lv2_file>, std::string>> escaped_real;

	std::unique_lock mp_lock(mp->mutex, std::defer_lock);

	if (lock)
	{
		mp_lock.lock();
	}

	if (fs::rename(from, to, overwrite))
	{
		return true;
	}

	if (fs::g_tls_error != fs::error::acces)
	{
		return false;
	}

	const auto escaped_from = Emu.GetCallbacks().resolve_path(from);

	auto check_path = [&](std::string_view path)
	{
		return path.starts_with(from) && (path.size() == from.size() || path[from.size()] == fs::delim[0] || path[from.size()] == fs::delim[1]);
	};

	idm::select<lv2_fs_object, lv2_file>([&](u32 id, lv2_file& file)
	{
		if (file.mp != mp)
		{
			return;
		}

		std::string escaped = Emu.GetCallbacks().resolve_path(file.real_path);

		if (check_path(escaped))
		{
			if (!file.file)
			{
				return;
			}

			file.restore_data.seek_pos = file.file.pos();

			file.file.close(); // Actually close it!
			escaped_real.emplace_back(ensure(idm::get_unlocked<lv2_fs_object, lv2_file>(id)), std::move(escaped));
		}
	});

	bool res = false;

	for (;; std::this_thread::yield())
	{
		if (fs::rename(from, to, overwrite))
		{
			res = true;
			break;
		}

		if (Emu.IsStopped() || fs::g_tls_error != fs::error::acces)
		{
			res = false;
			break;
		}
	}

	const auto fs_error = fs::g_tls_error;

	for (const auto& [file_ptr, real_path] : escaped_real)
	{
		lv2_file& file = *file_ptr;
		{
			// Update internal path
			if (res)
			{
				file.real_path = to + (real_path != escaped_from ? '/' + file.real_path.substr(from0.size()) : ""s);
			}

			// Reopen with ignored TRUNC, APPEND, CREATE and EXCL flags
			auto res0 = lv2_file::open_raw(file.real_path, file.flags & CELL_FS_O_ACCMODE, file.mode, file.type, file.mp);
			file.file = std::move(res0.file);
			ensure(file.file.operator bool());
			file.file.seek(file.restore_data.seek_pos);
		}
	}

	fs::g_tls_error = fs_error;
	return res;
}

bool vfs::host::unlink(const std::string& path, [[maybe_unused]] const std::string& dev_root)
{
#ifdef _WIN32
	if (auto device = fs::get_virtual_device(path))
	{
		return device->remove(path);
	}
	else
	{
		// Rename to special dummy name which will be ignored by VFS (but opened file handles can still read or write it)
		std::string dummy = hash_path(path, dev_root, "file");

		while (true)
		{
			if (fs::rename(path, dummy, false))
			{
				break;
			}

			if (fs::g_tls_error != fs::error::exist)
			{
				return false;
			}

			dummy = hash_path(path, dev_root, "file");
		}

		if (fs::file f{dummy, fs::read + fs::write})
		{
			// Set to delete on close on last handle
			FILE_DISPOSITION_INFO disp;
			disp.DeleteFileW = true;
			SetFileInformationByHandle(f.get_handle(), FileDispositionInfo, &disp, sizeof(disp));
			return true;
		}

		// TODO: what could cause this and how to handle it
		return true;
	}
#else
	return fs::remove_file(path);
#endif
}

bool vfs::host::remove_all(const std::string& path, [[maybe_unused]] const std::string& dev_root, [[maybe_unused]] const lv2_fs_mount_point* mp, [[maybe_unused]] bool remove_root, [[maybe_unused]] bool lock, [[maybe_unused]] bool force_atomic)
{
#ifndef _WIN32
	if (!force_atomic)
	{
		return fs::remove_all(path, remove_root);
	}
#endif

	if (remove_root)
	{
		// Rename to special dummy folder which will be ignored by VFS (but opened file handles can still read or write it)
		std::string dummy = hash_path(path, dev_root, "dir");

		while (true)
		{
			if (vfs::host::rename(path, dummy, mp, false, lock))
			{
				break;
			}

			if (fs::g_tls_error != fs::error::exist)
			{
				return false;
			}

			dummy = hash_path(path, dev_root, "dir");
		}

		if (!vfs::host::remove_all(dummy, dev_root, mp, false, lock))
		{
			return false;
		}

		if (!fs::remove_dir(dummy))
		{
			return false;
		}
	}
	else
	{
		const auto root_dir = fs::dir(path);

		if (!root_dir)
		{
			return false;
		}

		for (const auto& entry : root_dir)
		{
			if (entry.name == "." || entry.name == "..")
			{
				continue;
			}

			if (!entry.is_directory)
			{
				if (!vfs::host::unlink(path + '/' + entry.name, dev_root))
				{
					return false;
				}
			}
			else
			{
				if (!vfs::host::remove_all(path + '/' + entry.name, dev_root, mp, true, lock))
				{
					return false;
				}
			}
		}
	}

	return true;
}
