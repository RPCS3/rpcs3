// PS5: the PS3's system cache kept in memory (ps5_memory_cache.h)

#include "stdafx.h"
#include "ps5_memory_cache.h"

#include "Utilities/File.h"

#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace
{
	// The device's name, as fs::get_virtual_device wants one: "/vfsv0_", then
	// a '_' at index 29; the device manager knows it by what follows the
	// second '_'
	constexpr std::string_view c_name = "/vfsv0_virtual_ps5_hdd1_cache_dev";
	static_assert(c_name[29] == '_');

	constexpr u64 c_limit = 2ull << 30;
	constexpr u64 c_chunk = 256 * 1024;

	// Bytes held in files' chunks, all files together
	atomic_t<u64> g_used = 0;

	s64 now()
	{
		return static_cast<s64>(std::time(nullptr));
	}

	// A folder, or a file: its bytes in chunks of c_chunk, made when first
	// written (FIOS writes its cache file here and there: a chunk never
	// written reads as zeros and holds no memory). A file deleted while open
	// stays readable through its handles, as on the PS3
	struct node
	{
		const bool is_dir;
		s64 atime = now();
		s64 mtime = atime;
		s64 ctime = atime;

		std::shared_mutex mutex; // the bytes and the size
		std::vector<std::unique_ptr<u8[]>> chunks;
		u64 size = 0;

		explicit node(bool dir)
			: is_dir(dir)
		{
		}

		~node()
		{
			resize(0);
		}

		// Under the unique lock
		void resize(u64 length)
		{
			const usz keep = static_cast<usz>((length + c_chunk - 1) / c_chunk);
			for (usz i = keep; i < chunks.size(); i++)
			{
				if (chunks[i])
				{
					chunks[i].reset();
					g_used -= c_chunk;
				}
			}
			if (chunks.size() > keep)
			{
				chunks.resize(keep);
			}

			// What a shrink leaves of the last chunk reads as zeros if it grows again
			if (length < size && length % c_chunk && keep && chunks[keep - 1])
			{
				const u64 from = length % c_chunk;
				std::memset(chunks[keep - 1].get() + from, 0, c_chunk - from);
			}

			size = length;
		}

		// Under the shared lock
		u64 read_at(u64 offset, void* buffer, u64 count) const
		{
			if (offset >= size)
			{
				return 0;
			}
			count = std::min(count, size - offset);
			u8* out = static_cast<u8*>(buffer);
			for (u64 done = 0; done < count;)
			{
				const u64 at = offset + done;
				const usz index = static_cast<usz>(at / c_chunk);
				const u64 within = at % c_chunk;
				const u64 n = std::min(count - done, c_chunk - within);
				if (index < chunks.size() && chunks[index])
				{
					std::memcpy(out + done, chunks[index].get() + within, n);
				}
				else
				{
					std::memset(out + done, 0, n);
				}
				done += n;
			}
			return count;
		}

		// Under the unique lock. All or nothing: a write the limit would not
		// hold writes nothing and says the device is full
		u64 write_at(u64 offset, const void* buffer, u64 count)
		{
			if (!count)
			{
				return 0;
			}
			const u64 end = offset + count;
			if (end < offset)
			{
				fs::g_tls_error = fs::error::inval;
				return 0;
			}

			const usz first = static_cast<usz>(offset / c_chunk);
			const usz last = static_cast<usz>((end - 1) / c_chunk);
			u64 needed = 0;
			for (usz i = first; i <= last; i++)
			{
				if (i >= chunks.size() || !chunks[i])
				{
					needed += c_chunk;
				}
			}
			if (needed && g_used + needed > c_limit)
			{
				fs::g_tls_error = fs::error::nospace;
				return 0;
			}

			if (chunks.size() <= last)
			{
				chunks.resize(last + 1);
			}
			const u8* in = static_cast<const u8*>(buffer);
			for (u64 done = 0; done < count;)
			{
				const u64 at = offset + done;
				const usz index = static_cast<usz>(at / c_chunk);
				const u64 within = at % c_chunk;
				const u64 n = std::min(count - done, c_chunk - within);
				if (!chunks[index])
				{
					chunks[index] = std::make_unique<u8[]>(c_chunk);
					g_used += c_chunk;
				}
				std::memcpy(chunks[index].get() + within, in + done, n);
				done += n;
			}
			size = std::max(size, end);
			mtime = now();
			return count;
		}

		fs::stat_t stat()
		{
			fs::stat_t info{};
			info.is_directory = is_dir;
			info.is_writable = true;
			if (!is_dir)
			{
				std::shared_lock lock(mutex);
				info.size = size;
			}
			info.atime = atime;
			info.mtime = mtime;
			info.ctime = ctime;
			return info;
		}
	};

	class memory_file final : public fs::file_base
	{
		std::shared_ptr<node> m_node;
		u64 m_pos = 0;
		const bool m_append;

	public:
		memory_file(std::shared_ptr<node> file, bool append)
			: m_node(std::move(file))
			, m_append(append)
		{
		}

		fs::stat_t get_stat() override
		{
			return m_node->stat();
		}

		void sync() override
		{
		}

		bool trunc(u64 length) override
		{
			std::unique_lock lock(m_node->mutex);
			m_node->resize(length);
			m_node->mtime = now();
			return true;
		}

		u64 read(void* buffer, u64 count) override
		{
			const u64 n = read_at(m_pos, buffer, count);
			m_pos += n;
			return n;
		}

		u64 read_at(u64 offset, void* buffer, u64 count) override
		{
			std::shared_lock lock(m_node->mutex);
			return m_node->read_at(offset, buffer, count);
		}

		u64 write(const void* buffer, u64 count) override
		{
			std::unique_lock lock(m_node->mutex);
			if (m_append)
			{
				m_pos = m_node->size;
			}
			const u64 n = m_node->write_at(m_pos, buffer, count);
			m_pos += n;
			return n;
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			const s64 base = whence == fs::seek_mode::seek_set ? 0
				: whence == fs::seek_mode::seek_cur ? static_cast<s64>(m_pos)
				: static_cast<s64>(size());
			const s64 pos = base + offset;
			if (pos < 0)
			{
				fs::g_tls_error = fs::error::inval;
				return -1;
			}
			m_pos = static_cast<u64>(pos);
			return m_pos;
		}

		u64 size() override
		{
			std::shared_lock lock(m_node->mutex);
			return m_node->size;
		}
	};

	class memory_dir final : public fs::dir_base
	{
		std::vector<fs::dir_entry> m_entries;
		usz m_pos = 0;

	public:
		explicit memory_dir(std::vector<fs::dir_entry> entries)
			: m_entries(std::move(entries))
		{
		}

		bool read(fs::dir_entry& entry) override
		{
			if (m_pos >= m_entries.size())
			{
				return false;
			}
			entry = m_entries[m_pos++];
			return true;
		}

		void rewind() override
		{
			m_pos = 0;
		}
	};

	class memory_device final : public fs::device_base
	{
		std::shared_mutex m_mutex; // the tree
		std::map<std::string, std::shared_ptr<node>> m_nodes; // by path below the device, "a/b"; the root is ""

		// A path below the device: no prefix, no empty or "." parts, ".." taken back
		std::string key(std::string_view path) const
		{
			if (path.starts_with(fs_prefix))
			{
				path.remove_prefix(fs_prefix.size());
			}
			std::vector<std::string_view> parts;
			while (!path.empty())
			{
				const usz slash = path.find_first_of("/\\");
				const std::string_view part = path.substr(0, slash);
				path = slash == umax ? std::string_view{} : path.substr(slash + 1);
				if (part.empty() || part == ".")
				{
					continue;
				}
				if (part == "..")
				{
					if (!parts.empty())
					{
						parts.pop_back();
					}
					continue;
				}
				parts.push_back(part);
			}
			std::string result;
			for (std::string_view part : parts)
			{
				if (!result.empty())
				{
					result += '/';
				}
				result += part;
			}
			return result;
		}

		static std::string parent(const std::string& path)
		{
			const usz slash = path.rfind('/');
			return slash == umax ? std::string{} : path.substr(0, slash);
		}

		// Under either lock
		std::shared_ptr<node> find(const std::string& path) const
		{
			const auto it = m_nodes.find(path);
			return it == m_nodes.end() ? nullptr : it->second;
		}

		bool is_dir(const std::string& path) const
		{
			if (path.empty())
			{
				return true;
			}
			const auto found = find(path);
			return found && found->is_dir;
		}

		bool has_children(const std::string& path) const
		{
			const std::string below = path + '/';
			const auto it = m_nodes.lower_bound(below);
			return it != m_nodes.end() && it->first.starts_with(below);
		}

	public:
		explicit memory_device(std::string_view name)
		{
			fs_prefix = std::string{name};
		}

		bool stat(const std::string& path, fs::stat_t& info) override
		{
			const std::string at = key(path);
			std::shared_lock lock(m_mutex);
			if (at.empty())
			{
				info = {};
				info.is_directory = true;
				info.is_writable = true;
				return true;
			}
			const auto found = find(at);
			if (!found)
			{
				fs::g_tls_error = fs::error::noent;
				return false;
			}
			info = found->stat();
			return true;
		}

		bool statfs(const std::string& path, fs::device_stat& info) override
		{
			fs::stat_t ignored;
			if (!stat(path, ignored))
			{
				return false;
			}
			const u64 used = std::min<u64>(g_used, c_limit);
			info.block_size = 4096;
			info.total_size = c_limit;
			info.total_free = c_limit - used;
			info.avail_free = c_limit - used;
			return true;
		}

		bool create_dir(const std::string& path) override
		{
			const std::string at = key(path);
			std::unique_lock lock(m_mutex);
			if (at.empty() || find(at))
			{
				fs::g_tls_error = fs::error::exist;
				return false;
			}
			if (!is_dir(parent(at)))
			{
				fs::g_tls_error = fs::error::noent;
				return false;
			}
			m_nodes.emplace(at, std::make_shared<node>(true));
			return true;
		}

		bool remove_dir(const std::string& path) override
		{
			const std::string at = key(path);
			std::unique_lock lock(m_mutex);
			if (at.empty())
			{
				fs::g_tls_error = fs::error::acces;
				return false;
			}
			const auto found = find(at);
			if (!found)
			{
				fs::g_tls_error = fs::error::noent;
				return false;
			}
			if (!found->is_dir)
			{
				fs::g_tls_error = fs::error::notdir;
				return false;
			}
			if (has_children(at))
			{
				fs::g_tls_error = fs::error::notempty;
				return false;
			}
			m_nodes.erase(at);
			return true;
		}

		bool create_symlink(const std::string&) override
		{
			fs::g_tls_error = fs::error::acces;
			return false;
		}

		bool rename(const std::string& from, const std::string& to) override
		{
			const std::string src = key(from);
			const std::string dst = key(to);
			std::unique_lock lock(m_mutex);
			const auto moving = find(src);
			if (src.empty() || !moving)
			{
				fs::g_tls_error = fs::error::noent;
				return false;
			}
			if (src == dst)
			{
				return true;
			}
			if (dst.empty() || dst.starts_with(src + '/'))
			{
				fs::g_tls_error = fs::error::inval;
				return false;
			}
			if (!is_dir(parent(dst)))
			{
				fs::g_tls_error = fs::error::noent;
				return false;
			}
			if (const auto there = find(dst))
			{
				// As POSIX: a file replaces a file, a folder an empty folder
				if (there->is_dir != moving->is_dir)
				{
					fs::g_tls_error = there->is_dir ? fs::error::isdir : fs::error::notdir;
					return false;
				}
				if (there->is_dir && has_children(dst))
				{
					fs::g_tls_error = fs::error::notempty;
					return false;
				}
				m_nodes.erase(dst);
			}

			// The node, and a folder's whole tree under it
			std::vector<std::pair<std::string, std::shared_ptr<node>>> moved;
			moved.emplace_back(dst, moving);
			const std::string below = src + '/';
			for (auto it = m_nodes.lower_bound(below); it != m_nodes.end() && it->first.starts_with(below); ++it)
			{
				moved.emplace_back(dst + it->first.substr(src.size()), it->second);
			}
			m_nodes.erase(src);
			auto last = m_nodes.lower_bound(below);
			const auto first = last;
			while (last != m_nodes.end() && last->first.starts_with(below))
			{
				++last;
			}
			m_nodes.erase(first, last);
			for (auto& [name, item] : moved)
			{
				m_nodes.insert_or_assign(std::move(name), std::move(item));
			}
			return true;
		}

		bool remove(const std::string& path) override
		{
			const std::string at = key(path);
			std::unique_lock lock(m_mutex);
			const auto found = find(at);
			if (at.empty() || !found)
			{
				fs::g_tls_error = fs::error::noent;
				return false;
			}
			if (found->is_dir)
			{
				fs::g_tls_error = fs::error::isdir;
				return false;
			}
			m_nodes.erase(at);
			return true;
		}

		bool trunc(const std::string& path, u64 length) override
		{
			const std::string at = key(path);
			std::shared_lock lock(m_mutex);
			const auto found = find(at);
			if (!found || found->is_dir)
			{
				fs::g_tls_error = found ? fs::error::isdir : fs::error::noent;
				return false;
			}
			std::unique_lock file_lock(found->mutex);
			found->resize(length);
			found->mtime = now();
			return true;
		}

		bool utime(const std::string& path, s64 atime, s64 mtime) override
		{
			const std::string at = key(path);
			std::shared_lock lock(m_mutex);
			const auto found = find(at);
			if (!found)
			{
				fs::g_tls_error = fs::error::noent;
				return false;
			}
			found->atime = atime;
			found->mtime = mtime;
			return true;
		}

		std::unique_ptr<fs::file_base> open(const std::string& path, bs_t<fs::open_mode> mode) override
		{
			const std::string at = key(path);
			std::unique_lock lock(m_mutex);
			if (at.empty())
			{
				fs::g_tls_error = fs::error::isdir;
				return nullptr;
			}
			std::shared_ptr<node> file = find(at);
			if (!file)
			{
				if (!(mode & fs::create))
				{
					fs::g_tls_error = fs::error::noent;
					return nullptr;
				}
				if (!is_dir(parent(at)))
				{
					fs::g_tls_error = fs::error::noent;
					return nullptr;
				}
				file = std::make_shared<node>(false);
				m_nodes.emplace(at, file);
			}
			else
			{
				if (file->is_dir)
				{
					fs::g_tls_error = fs::error::isdir;
					return nullptr;
				}
				if ((mode & fs::create) && (mode & fs::excl))
				{
					fs::g_tls_error = fs::error::exist;
					return nullptr;
				}
				if ((mode & fs::trunc) && (mode & fs::write))
				{
					std::unique_lock file_lock(file->mutex);
					file->resize(0);
					file->mtime = now();
				}
			}
			return std::make_unique<memory_file>(std::move(file), !!(mode & fs::append));
		}

		std::unique_ptr<fs::dir_base> open_dir(const std::string& path) override
		{
			const std::string at = key(path);
			std::shared_lock lock(m_mutex);
			if (!is_dir(at))
			{
				fs::g_tls_error = find(at) ? fs::error::notdir : fs::error::noent;
				return nullptr;
			}

			// What is directly in it
			std::vector<fs::dir_entry> entries;
			const std::string below = at.empty() ? std::string{} : at + '/';
			for (auto it = m_nodes.lower_bound(below); it != m_nodes.end() && it->first.starts_with(below); ++it)
			{
				const std::string_view name = std::string_view(it->first).substr(below.size());
				if (name.empty() || name.find('/') != umax)
				{
					continue;
				}
				fs::dir_entry& entry = entries.emplace_back();
				static_cast<fs::stat_t&>(entry) = it->second->stat();
				entry.name = std::string{name};
			}
			return std::make_unique<memory_dir>(std::move(entries));
		}
	};
}

const std::string& ps5_syscache_root()
{
	static const std::string root = []
	{
		const std::string name{c_name};
		const auto device = stx::make_shared<memory_device>(name);
		fs::set_virtual_device(name.substr(name.find('_', 7) + 1), device);
		device->create_dir(name + "/caches");
		return name + "/caches/";
	}();
	return root;
}

u64 ps5_syscache_bytes()
{
	return g_used;
}

u64 ps5_syscache_limit()
{
	return c_limit;
}
