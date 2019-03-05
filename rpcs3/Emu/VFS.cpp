#include "stdafx.h"
#include "IdManager.h"
#include "VFS.h"

#include "Utilities/mutex.h"
#include "Utilities/StrUtil.h"

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
	const auto table = fxm::get_always<vfs_manager>();

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

		if (pos == -1)
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
			list.resize(list.size() - 1);
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

std::string vfs::get(std::string_view vpath, std::vector<std::string>* out_dir)
{
	const auto table = fxm::get_always<vfs_manager>();

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

	for (std::vector<const vfs_directory*> list{&table->root};;)
	{
		// Skip one or more '/'
		const auto pos = vpath.find_first_not_of('/');

		if (pos == 0)
		{
			// Relative path: point to non-existent location
			return fs::get_config_dir() + "delete_this_dir.../delete_this...";
		}

		if (pos == -1)
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
			list.resize(list.size() - 1);
			result.resize(result.size() - 1);
			continue;
		}

		const auto last = list.back();
		list.push_back(nullptr);

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
					if (vpath.empty())
					{
						return {};
					}

					// Handle /host_root (not escaped, not processed)
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

	// Escape and merge path fragments
	return std::string{result_base} + vfs::escape(fmt::merge(result, "/"));
}

std::string vfs::escape(std::string_view path)
{
	std::string result;
	result.reserve(path.size());

	for (std::size_t i = 0, s = path.size(); i < s; i++)
	{
		switch (char c = path[i])
		{
		case '<':
		{
			result += u8"＜";
			break;
		}
		case '>':
		{
			result += u8"＞";
			break;
		}
		case ':':
		{
			result += u8"：";
			break;
		}
		case '"':
		{
			result += u8"＂";
			break;
		}
		case '\\':
		{
			result += u8"＼";
			break;
		}
		case '|':
		{
			result += u8"｜";
			break;
		}
		case '?':
		{
			result += u8"？";
			break;
		}
		case '*':
		{
			result += u8"＊";
			break;
		}
		case char{u8"！"[0]}:
		{
			// Escape full-width characters 0xFF01..0xFF5e with ！ (0xFF01)
			switch (path[i + 1])
			{
			case char{u8"！"[1]}:
			{
				const uchar c3 = reinterpret_cast<const uchar&>(path[i + 2]);

				if (c3 >= 0x81 && c3 <= 0xbf)
				{
					result += u8"！";
				}

				break;
			}
			case char{u8"｀"[1]}:
			{
				const uchar c3 = reinterpret_cast<const uchar&>(path[i + 2]);

				if (c3 >= 0x80 && c3 <= 0x9e)
				{
					result += u8"！";
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

std::string vfs::unescape(std::string_view path)
{
	std::string result;
	result.reserve(path.size());

	for (std::size_t i = 0, s = path.size(); i < s; i++)
	{
		switch (char c = path[i])
		{
		case char{u8"！"[0]}:
		{
			switch (path[i + 1])
			{
			case char{u8"！"[1]}:
			{
				const uchar c3 = reinterpret_cast<const uchar&>(path[i + 2]);

				if (c3 >= 0x81 && c3 <= 0xbf)
				{
					switch (path[i + 2])
					{
					case char{u8"！"[2]}:
					{
						i += 3;
						result += c;
						continue;
					}
					case char{u8"＜"[2]}:
					{
						result += '<';
						break;
					}
					case char{u8"＞"[2]}:
					{
						result += '>';
						break;
					}
					case char{u8"："[2]}:
					{
						result += ':';
						break;
					}
					case char{u8"＂"[2]}:
					{
						result += '"';
						break;
					}
					case char{u8"＼"[2]}:
					{
						result += '\\';
						break;
					}
					case char{u8"？"[2]}:
					{
						result += '?';
						break;
					}
					case char{u8"＊"[2]}:
					{
						result += '*';
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
			case char{u8"｀"[1]}:
			{
				const uchar c3 = reinterpret_cast<const uchar&>(path[i + 2]);

				if (c3 >= 0x80 && c3 <= 0x9e)
				{
					switch (path[i + 2])
					{
					case char{u8"｜"[2]}:
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
		default:
		{
			result += c;
			break;
		}
		}
	}

	return result;
}
