#include "stdafx.h"
#include "Crypto/sha1.h"
#include "IdManager.h"
#include "VFS.h"

#include <regex>

struct vfs_manager
{
	shared_mutex mutex;

	// Device name -> Real path
	std::unordered_map<std::string, std::string> mounted;
	// Virtual Link Hash -> Virtual Path (these can be hundreds in count)
	std::unordered_map<std::string, std::string> virtual_links;
};

const std::regex s_regex_ps3("^/+(.*?)(?:$|/)(.*)", std::regex::optimize);
const std::string s_link_device("vlink");

bool vfs::mount(const std::string& dev_name, const std::string& path)
{
	const auto table = fxm::get_always<vfs_manager>();

	safe_writer_lock lock(table->mutex);

	return table->mounted.emplace(dev_name, path).second;
}

std::string vfs::get(const std::string& vpath, const std::string* prev, std::size_t pos)
{
	const auto table = fxm::get_always<vfs_manager>();

	safe_reader_lock lock(table->mutex);

	std::smatch match;

	if (!std::regex_match(vpath.begin() + pos, vpath.end(), match, s_regex_ps3))
	{
		const auto found = table->mounted.find("");

		if (found == table->mounted.end())
		{
			LOG_WARNING(GENERAL, "vfs::get(): no default directory: %s", vpath);
			return {};
		}

		return found->second + vfs::escape(vpath);
	}

	if (match.length(1) + pos == 0)
	{
		return "/";
	}

	std::string dev;

	if (prev)
	{
		dev += *prev;
		dev += '/';
	}

	dev += match.str(1);

	const auto found = table->mounted.find(dev);

	if (found == table->mounted.end())
	{
		if (!s_link_device.compare(match.str(1)))
		{
			std::string lhash(vpath.c_str() + vpath.find_last_of("/") + 1, vpath.c_str() + vpath.find_last_of("."));
			auto found = table->virtual_links.find(lhash);
			if (found != table->virtual_links.end())
			{
				return vfs::get(found->second);
			}
			LOG_ERROR(GENERAL, "vfs::get(): virtual link key was not found: %s", lhash);
			return {};
		}

		if (match.length(2))
		{
			return vfs::get(vpath, &dev, pos + match.position(1) + match.length(1));
		}

		LOG_WARNING(GENERAL, "vfs::get(): device not found: %s", vpath);
		return {};
	}

	if (found->second.empty())
	{
		// Don't escape /host_root (TODO)
		return match.str(2);
	}

	// Escape and concatenate
	return found->second + vfs::escape(match.str(2));
}


std::string vfs::escape(const std::string& path)
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

std::string vfs::unescape(const std::string& path)
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


bool vfs::link(const std::string& vpath, std::string& vlinkout)
{
	u8 hash[20];
	sha1((u8*)vpath.data(), vpath.size(), &hash[0]);
	std::string hash_str;
	for (auto b : hash)
	{
		hash_str += fmt::format("%02X", b);
	}

	const auto table = fxm::get_always<vfs_manager>();

	safe_reader_lock lock1(table->mutex);

	const auto found = table->virtual_links.find(hash_str);
	if (found == table->virtual_links.end())
	{
		safe_writer_lock lock2(table->mutex);
		vlinkout.assign(fmt::format("/%s/%s%s", s_link_device, hash_str, vpath.substr(vpath.find_last_of("."))));
		return table->virtual_links.emplace(hash_str, vpath).second;
	}
	else
	{
		vlinkout.assign(found->second);
		return true;
	}
}

bool vfs::unlink(const std::string& vlink)
{
	const auto table = fxm::get_always<vfs_manager>();

	safe_reader_lock lock(table->mutex);

	std::string lhash(vlink.c_str() + vlink.find_last_of("/") + 1, vlink.c_str() + vlink.find_last_of("."));
	const auto found = table->virtual_links.find(lhash);
	if (found != table->virtual_links.end())
	{
		table->virtual_links.erase(found);
		return true;
	}

	return false;
}


