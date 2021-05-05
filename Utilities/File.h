#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/types.hpp"
#include "util/shared_ptr.hpp"
#include "bit_set.h"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace fs
{
#ifdef _WIN32
	static constexpr auto& delim = "/\\";
	static constexpr auto& wdelim = L"/\\";
	using native_handle = void*;
#else
	static constexpr auto& delim = "/";
	static constexpr auto& wdelim = L"/";
	using native_handle = int;
#endif

	// File open mode flags
	enum class open_mode : u32
	{
		read,
		write,
		append,
		create,
		trunc,
		excl,
		lock,
		unread,

		__bitset_enum_max
	};

	constexpr auto read    = +open_mode::read; // Enable reading
	constexpr auto write   = +open_mode::write; // Enable writing
	constexpr auto append  = +open_mode::append; // Always append to the end of the file
	constexpr auto create  = +open_mode::create; // Create file if it doesn't exist
	constexpr auto trunc   = +open_mode::trunc; // Clear opened file if it's not empty
	constexpr auto excl    = +open_mode::excl; // Failure if the file already exists (used with `create`)
	constexpr auto lock    = +open_mode::lock; // Prevent opening the file more than once
	constexpr auto unread  = +open_mode::unread; // Aggressively prevent reading the opened file (do not use)

	constexpr auto rewrite = open_mode::write + open_mode::create + open_mode::trunc;

	// File seek mode
	enum class seek_mode : u32
	{
		seek_set,
		seek_cur,
		seek_end,
	};

	constexpr auto seek_set = seek_mode::seek_set; // From beginning
	constexpr auto seek_cur = seek_mode::seek_cur; // From current position
	constexpr auto seek_end = seek_mode::seek_end; // From end

	// File attributes (TODO)
	struct stat_t
	{
		bool is_directory;
		bool is_writable;
		u64 size;
		s64 atime;
		s64 mtime;
		s64 ctime;
	};

	// Helper, layout is equal to iovec struct
	struct iovec_clone
	{
		const void* iov_base;
		usz iov_len;
	};

	// File handle base
	struct file_base
	{
		virtual ~file_base();

		[[noreturn]] virtual stat_t stat();
		virtual void sync();
		virtual bool trunc(u64 length) = 0;
		virtual u64 read(void* buffer, u64 size) = 0;
		virtual u64 write(const void* buffer, u64 size) = 0;
		virtual u64 seek(s64 offset, seek_mode whence) = 0;
		virtual u64 size() = 0;
		virtual native_handle get_handle();
		virtual u64 write_gather(const iovec_clone* buffers, u64 buf_count);
	};

	// Directory entry (TODO)
	struct dir_entry : stat_t
	{
		std::string name{};

		dir_entry()
			: stat_t{}
		{
		}
	};

	// Directory handle base
	struct dir_base
	{
		virtual ~dir_base();

		virtual bool read(dir_entry&) = 0;
		virtual void rewind() = 0;
	};

	// Device information
	struct device_stat
	{
		u64 block_size;
		u64 total_size;
		u64 total_free; // Total size of free space
		u64 avail_free; // Free space available to unprivileged user
	};

	// Virtual device
	struct device_base
	{
		const std::string fs_prefix;

		device_base();
		virtual ~device_base();

		virtual bool stat(const std::string& path, stat_t& info) = 0;
		virtual bool statfs(const std::string& path, device_stat& info) = 0;
		virtual bool remove_dir(const std::string& path);
		virtual bool create_dir(const std::string& path);
		virtual bool rename(const std::string& from, const std::string& to);
		virtual bool remove(const std::string& path);
		virtual bool trunc(const std::string& path, u64 length);
		virtual bool utime(const std::string& path, s64 atime, s64 mtime);

		virtual std::unique_ptr<file_base> open(const std::string& path, bs_t<open_mode> mode) = 0;
		virtual std::unique_ptr<dir_base> open_dir(const std::string& path) = 0;
	};

	[[noreturn]] void xnull(const src_loc&);
	[[noreturn]] void xfail(const src_loc&);
	[[noreturn]] void xovfl();

	constexpr struct pod_tag_t{} pod_tag;

	// Get virtual device for specified path (nullptr for real path)
	shared_ptr<device_base> get_virtual_device(const std::string& path);

	// Set virtual device with specified name (nullptr for deletion)
	shared_ptr<device_base> set_virtual_device(const std::string& name, shared_ptr<device_base> device);

	// Try to get parent directory (returns empty string on failure)
	std::string get_parent_dir(const std::string& path);

	// Get file information
	bool stat(const std::string& path, stat_t& info);

	// Check whether a file or a directory exists (not recommended, use is_file() or is_dir() instead)
	bool exists(const std::string& path);

	// Check whether the file exists and is NOT a directory
	bool is_file(const std::string& path);

	// Check whether the directory exists and is NOT a file
	bool is_dir(const std::string& path);

	// Get filesystem information
	bool statfs(const std::string& path, device_stat& info);

	// Delete empty directory
	bool remove_dir(const std::string& path);

	// Create directory
	bool create_dir(const std::string& path);

	// Create directories
	bool create_path(const std::string& path);

	// Rename (move) file or directory
	bool rename(const std::string& from, const std::string& to, bool overwrite);

	// Copy file contents
	bool copy_file(const std::string& from, const std::string& to, bool overwrite);

	// Delete file
	bool remove_file(const std::string& path);

	// Change file size (possibly appending zeros)
	bool truncate_file(const std::string& path, u64 length);

	// Set file access/modification time
	bool utime(const std::string& path, s64 atime, s64 mtime);

	// Synchronize filesystems (TODO)
	void sync();

	class file final
	{
		std::unique_ptr<file_base> m_file{};

		bool strict_read_check(u64 size, u64 type_size) const;

	public:
		// Default constructor
		file() = default;

		// Open file with specified mode
		explicit file(const std::string& path, bs_t<open_mode> mode = ::fs::read);

		// Open memory for read
		explicit file(const void* ptr, usz size);

		// Open file with specified args (forward to constructor)
		template <typename... Args>
		bool open(Args&&... args)
		{
			*this = fs::file(std::forward<Args>(args)...);
			return m_file.operator bool();
		}

		// Check whether the handle is valid (opened file)
		explicit operator bool() const
		{
			return m_file.operator bool();
		}

		// Close the file explicitly
		void close()
		{
			m_file.reset();
		}

		void reset(std::unique_ptr<file_base>&& ptr)
		{
			m_file = std::move(ptr);
		}

		std::unique_ptr<file_base> release()
		{
			return std::move(m_file);
		}

		// Change file size (possibly appending zero bytes)
		bool trunc(u64 length,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->trunc(length);
		}

		// Get file information
		stat_t stat(
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->stat();
		}

		// Sync file buffers
		void sync(
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->sync();
		}

		// Read the data from the file and return the amount of data written in buffer
		u64 read(void* buffer, u64 count,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->read(buffer, count);
		}

		// Write the data to the file and return the amount of data actually written
		u64 write(const void* buffer, u64 count,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->write(buffer, count);
		}

		// Change current position, returns resulting position
		u64 seek(s64 offset, seek_mode whence = seek_set,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->seek(offset, whence);
		}

		// Get file size
		u64 size(
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->size();
		}

		// Get current position
		u64 pos(
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->seek(0, seek_cur);
		}

		// Write std::basic_string unconditionally
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, const file&> write(const std::basic_string<T>& str,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (write(str.data(), str.size() * sizeof(T), line, col, file, func) != str.size() * sizeof(T)) xfail({line, col, file, func});
			return *this;
		}

		// Write POD unconditionally
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, const file&> write(const T& data,
			pod_tag_t = pod_tag,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (write(std::addressof(data), sizeof(T), line, col, file, func) != sizeof(T)) xfail({line, col, file, func});
			return *this;
		}

		// Write POD std::vector unconditionally
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, const file&> write(const std::vector<T>& vec,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (write(vec.data(), vec.size() * sizeof(T), line, col, file, func) != vec.size() * sizeof(T)) xfail({line, col, file, func});
			return *this;
		}

		// Read std::basic_string, size must be set by resize() method
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(std::basic_string<T>& str,
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION(),
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN()) const
		{
			return read(&str[0], str.size() * sizeof(T), line, col, file, func) == str.size() * sizeof(T);
		}

		// Read std::basic_string
		template <bool IsStrict = false, typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(std::basic_string<T>& str, usz _size,
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION(),
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN()) const
		{
			if (!m_file) xnull({line, col, file, func});
			if (!_size) return true;

			if constexpr (IsStrict)
			{
				// If _size arg is too high std::bad_alloc may happen in resize and then we cannot error check
				if (!strict_read_check(_size, sizeof(T))) return false;
			}

			str.resize(_size);
			return read(str.data(), sizeof(T) * _size, line, col, file, func) == sizeof(T) * _size;
		}

		// Read POD, sizeof(T) is used
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(T& data,
			pod_tag_t = pod_tag,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			return read(&data, sizeof(T), line, col, file, func) == sizeof(T);
		}

		// Read POD std::vector, size must be set by resize() method
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(std::vector<T>& vec,
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION(),
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN()) const
		{
			return read(vec.data(), sizeof(T) * vec.size(), line, col, file, func) == sizeof(T) * vec.size();
		}

		// Read POD std::vector
		template <bool IsStrict = false, typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(std::vector<T>& vec, usz _size,
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION(),
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN()) const
		{
			if (!m_file) xnull({line, col, file, func});
			if (!_size) return true;

			if constexpr (IsStrict)
			{
				if (!strict_read_check(_size, sizeof(T))) return false;
			}

			vec.resize(_size);
			return read(vec.data(), sizeof(T) * _size, line, col, file, func) == sizeof(T) * _size;
		}

		// Read POD (experimental)
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, T> read(
			pod_tag_t = pod_tag,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			T result;
			if (!read(result, pod_tag, line, col, file, func)) xfail({line, col, file, func});
			return result;
		}

		// Read full file to std::basic_string
		template <typename T = char>
		std::basic_string<T> to_string(
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			std::basic_string<T> result;
			result.resize(size() / sizeof(T));
			if (seek(0), !read(result, file, func, line, col)) xfail({line, col, file, func});
			return result;
		}

		// Read full file to std::vector
		template<typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, std::vector<T>> to_vector(
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			std::vector<T> result;
			result.resize(size() / sizeof(T));
			if (seek(0), !read(result, file, func, line, col)) xfail({line, col, file, func});
			return result;
		}

		// Get native handle if available
		native_handle get_handle() const;

		// Gathered write
		u64 write_gather(const iovec_clone* buffers, u64 buf_count,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_file) xnull({line, col, file, func});
			return m_file->write_gather(buffers, buf_count);
		}
	};

	class dir final
	{
		std::unique_ptr<dir_base> m_dir{};

	public:
		dir() = default;

		// Open dir handle
		explicit dir(const std::string& path)
		{
			open(path);
		}

		// Open specified directory
		bool open(const std::string& path);

		// Check whether the handle is valid (opened directory)
		explicit operator bool() const
		{
			return m_dir.operator bool();
		}

		// Close the directory explicitly
		void close()
		{
			m_dir.reset();
		}

		void reset(std::unique_ptr<dir_base>&& ptr)
		{
			m_dir = std::move(ptr);
		}

		std::unique_ptr<dir_base> release()
		{
			return std::move(m_dir);
		}

		// Get next directory entry
		bool read(dir_entry& out,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_dir) xnull({line, col, file, func});
			return m_dir->read(out);
		}

		// Reset to the beginning
		void rewind(
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION()) const
		{
			if (!m_dir) xnull({line, col, file, func});
			return m_dir->rewind();
		}

		class iterator
		{
			const dir* m_parent;
			dir_entry m_entry{};

		public:
			enum class mode
			{
				from_first,
				from_current
			};

			iterator(const dir* parent, mode mode_ = mode::from_first)
				: m_parent(parent)
			{
				if (!m_parent)
				{
					return;
				}

				if (mode_ == mode::from_first)
				{
					m_parent->rewind();
				}

				if (!m_parent->read(m_entry))
				{
					m_parent = nullptr;
				}
			}

			iterator(const iterator&) = default;

			iterator(iterator&&) = default;

			iterator& operator=(const iterator&) = default;

			iterator& operator=(iterator&&) = default;

			dir_entry& operator *()
			{
				return m_entry;
			}

			iterator& operator++()
			{
				*this = {m_parent, mode::from_current};
				return *this;
			}

			bool operator !=(const iterator& rhs) const
			{
				return m_parent != rhs.m_parent;
			}
		};

		iterator begin() const
		{
			return {m_dir ? this : nullptr};
		}

		iterator end() const
		{
			return {nullptr};
		}
	};

	// Get configuration directory
	const std::string& get_config_dir();

	// Get common cache directory
	const std::string& get_cache_dir();

	// Temporary directory
	const std::string& get_temp_dir();

	// Unique pending file creation destined to be renamed to the destination file
	struct pending_file
	{
		fs::file file{};

		// This is meant to modify files atomically, overwriting is likely
		bool commit(bool overwrite = true);

		pending_file(const std::string& path);
		~pending_file();

	private:
		std::string m_path{}; // Pending file path
		std::string m_dest{}; // Destination file path
	};

	// Delete directory and all its contents recursively
	bool remove_all(const std::string& path, bool remove_root = true);

	// Get size of all files recursively
	u64 get_dir_size(const std::string& path, u64 rounding_alignment = 1);

	enum class error : uint
	{
		ok = 0,

		inval,
		noent,
		exist,
		acces,
		notempty,
		readonly,
		isdir,
		toolong,
		nospace,
		unknown
	};

	// Error code returned
	extern thread_local error g_tls_error;

	template <typename T>
	struct container_stream final : file_base
	{
		// T can be a reference, but this is not recommended
		using value_type = typename std::remove_reference_t<T>::value_type;

		T obj;
		u64 pos;

		container_stream(T&& obj)
			: obj(std::forward<T>(obj))
			, pos(0)
		{
		}

		~container_stream() override
		{
		}

		bool trunc(u64 length) override
		{
			obj.resize(length);
			return true;
		}

		u64 read(void* buffer, u64 size) override
		{
			const u64 end = obj.size();

			if (pos < end)
			{
				// Get readable size
				if (const u64 max = std::min<u64>(size, end - pos))
				{
					std::copy(obj.cbegin() + pos, obj.cbegin() + pos + max, static_cast<value_type*>(buffer));
					pos = pos + max;
					return max;
				}
			}

			return 0;
		}

		u64 write(const void* buffer, u64 size) override
		{
			const u64 old_size = obj.size();

			if (old_size + size < old_size || pos + size < pos)
			{
				xovfl();
			}

			if (pos > old_size)
			{
			 	// Reserve memory
				obj.reserve(pos + size);

				// Fill gap if necessary (default-initialized)
				obj.resize(pos);
			}

			const auto src = static_cast<const value_type*>(buffer);

			// Overwrite existing part
			const u64 overlap = std::min<u64>(obj.size() - pos, size);
			std::copy(src, src + overlap, obj.begin() + pos);

			// Append new data
			obj.insert(obj.end(), src + overlap, src + size);
			pos += size;

			return size;
		}

		u64 seek(s64 offset, seek_mode whence) override
		{
			const s64 new_pos =
				whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? offset + pos :
				whence == fs::seek_end ? offset + size() : -1;

			if (new_pos < 0)
			{
				fs::g_tls_error = fs::error::inval;
				return -1;
			}

			pos = new_pos;
			return pos;
		}

		u64 size() override
		{
			return obj.size();
		}
	};

	template <typename T>
	file make_stream(T&& container = T{})
	{
		file result;
		result.reset(std::make_unique<container_stream<T>>(std::forward<T>(container)));
		return result;
	}

	template <bool Flush = false, typename... Args>
	bool write_file(const std::string& path, bs_t<fs::open_mode> mode, const Args&... args)
	{
		// Always use write flag, remove read flag
		if (fs::file f{path, mode + fs::write - fs::read})
		{
			if constexpr (sizeof...(args) == 2u && (std::is_pointer_v<Args> || ...))
			{
				// Specialization for [const void*, usz] args
				f.write(args...);
			}
			else
			{
				// Write args sequentially
				(f.write(args), ...);
			}

			if constexpr (Flush)
			{
				f.sync();
			}

			return true;
		}

		return false;
	}

	file make_gather(std::vector<file>);
}
