#include "stdafx.h"
#include "IdManager.h"
#include "System.h"
#include "VFS.h"

#include "Utilities/mutex.h"
#include "Utilities/StrUtil.h"

#ifdef _WIN32
#include <Windows.h>
#endif

struct vfs_directory
{
	// Real path (empty if root or not exists)
	std::string path;

	// Virtual subdirectories (vector because only vector allows incomplete types)
	std::vector<std::pair<std::string, vfs_directory>> dirs;
};

struct vfs_manager
{
	shared_mutex mutex;

	// VFS root
	vfs_directory root;
};

bool vfs::mount(std::string_view vpath, std::string_view path)
{
	// Workaround
	g_fxo->need<vfs_manager>();

	const auto table = g_fxo->get<vfs_manager>();

	// TODO: scan roots of mounted devices for undeleted vfs::host::unlink remnants, and try to delete them (_WIN32 only)

	std::lock_guard lock(table->mutex);

	if (vpath.empty())
	{
		// Empty relative path, should set relative path base; unsupported
		return false;
	}

	for (std::vector<vfs_directory*> list{&table->root};;)
	{
		// Skip one or more '/'
		const auto pos = vpath.find_first_not_of('/');

		if (pos == 0)
		{
			// Mounting relative path is not supported
			return false;
		}

		if (pos == umax)
		{
			// Mounting completed
			list.back()->path = path;
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
		const auto last = list.back();

		for (auto& dir : last->dirs)
		{
			if (dir.first == name)
			{
				list.push_back(&dir.second);
				break;
			}
		}

		if (last == list.back())
		{
			// Add new entry
			list.push_back(&last->dirs.emplace_back(name, vfs_directory{}).second);
		}
	}
}

std::string vfs::get(std::string_view vpath, std::vector<std::string>* out_dir, std::string* out_path)
{
	const auto table = g_fxo->get<vfs_manager>();

	reader_lock lock(table->mutex);

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

	for (std::vector<const vfs_directory*> list{&table->root};;)
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
							if (!pair.second.path.empty())
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

		for (auto& dir : last->dirs)
		{
			if (dir.first == name)
			{
				list.back() = &dir.second;

				if (dir.second.path == "/"sv)
				{
					if (vpath.size() <= 1)
					{
						return fs::get_config_dir() + "delete_this_dir.../delete_this...";
					}

					// Handle /host_root (not escaped, not processed)
					if (out_path)
					{
						*out_path =  "/";
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
		*out_path =  "/";
		*out_path += fmt::merge(name_list, "/");
	}

	// Escape for host FS
	std::vector<std::string> escaped;
	escaped.reserve(result.size());
	for (auto& sv : result)
		escaped.emplace_back(vfs::escape(sv));

	return std::string{result_base} + fmt::merge(escaped, "/");
}

#if __cpp_char8_t >= 201811
using char2 = char8_t;
#else
using char2 = char;
#endif

std::string vfs::escape(std::string_view name, bool escape_slash)
{
	std::string result;

	if (name.size() > 2 && name.find_first_not_of('.') == umax)
	{
		// Name contains only dots, not allowed on Windows.
		result.reserve(name.size() + 2);
		result += reinterpret_cast<const char*>(u8"．");
		result += name.substr(1);
		return result;
	}

	// Emulate NTS (limited)
	auto get_char = [&](std::size_t pos) -> char2
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
			if (name.size() >= 4 && name[3] >= '1' && name[4] <= '9')
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

	for (std::size_t i = 0, s = name.size(); i < s; i++)
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
		case char2{u8"！"[0]}:
		{
			// Escape full-width characters 0xFF01..0xFF5e with ！ (0xFF01)
			switch (char2 c2 = get_char(i + 1))
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
	auto get_char = [&](std::size_t pos) -> char2
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

	for (std::size_t i = 0, s = name.size(); i < s; i++)
	{
		switch (char2 c = name[i])
		{
		case char2{u8"！"[0]}:
		{
			switch (char2 c2 = get_char(i + 1))
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

bool vfs::host::rename(const std::string& from, const std::string& to, bool overwrite)
{
	while (!fs::rename(from, to, overwrite))
	{
		// Try to ignore access error in order to prevent spurious failure
		if (Emu.IsStopped() || fs::g_tls_error != fs::error::acces)
		{
			return false;
		}
	}

	return true;
}

bool vfs::host::unlink(const std::string& path, const std::string& dev_root)
{
#ifdef _WIN32
	if (auto device = fs::get_virtual_device(path))
	{
		return device->remove(path);
	}
	else
	{
		// Rename to special dummy name which will be ignored by VFS (but opened file handles can still read or write it)
		const std::string dummy = fmt::format(u8"%s/＄%s%s", dev_root, fmt::base57(std::hash<std::string>()(path)), fmt::base57(__rdtsc()));

		if (!fs::rename(path, dummy, true))
		{
			return false;
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

bool vfs::host::remove_all(const std::string& path, const std::string& dev_root, bool remove_root)
{
#ifdef _WIN32
	if (remove_root)
	{
		// Rename to special dummy folder which will be ignored by VFS (but opened file handles can still read or write it)
		const std::string dummy = fmt::format(u8"%s/＄%s%s", dev_root, fmt::base57(std::hash<std::string>()(path)), fmt::base57(__rdtsc()));

		if (!vfs::host::rename(path, dummy, false))
		{
			return false;
		}

		if (!vfs::host::remove_all(dummy, dev_root, false))
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
				if (!vfs::host::remove_all(path + '/' + entry.name, dev_root))
				{
					return false;
				}
			}
		}
	}

	return true;
#else
	return fs::remove_all(path, remove_root);
#endif
}
