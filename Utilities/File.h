#pragma once

#include "types.h"
#include "bit_set.h"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>


namespace fs
{
#ifdef _WIN32
	using native_handle = void*;
#else
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
		std::size_t iov_len;
	};

	// File handle base
	struct file_base
	{
		virtual ~file_base();

		virtual stat_t stat();
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
		std::string name;
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
		virtual ~device_base();

		virtual bool stat(const std::string& path, stat_t& info) = 0;
		virtual bool statfs(const std::string& path, device_stat& info) = 0;
		virtual bool remove_dir(const std::string& path) = 0;
		virtual bool create_dir(const std::string& path) = 0;
		virtual bool rename(const std::string& from, const std::string& to) = 0;
		virtual bool remove(const std::string& path) = 0;
		virtual bool trunc(const std::string& path, u64 length) = 0;
		virtual bool utime(const std::string& path, s64 atime, s64 mtime) = 0;

		virtual std::unique_ptr<file_base> open(const std::string& path, bs_t<open_mode> mode) = 0;
		virtual std::unique_ptr<dir_base> open_dir(const std::string& path) = 0;
	};

	// Get virtual device for specified path (nullptr for real path)
	std::shared_ptr<device_base> get_virtual_device(const std::string& path);

	// Set virtual device with specified name (nullptr for deletion)
	std::shared_ptr<device_base> set_virtual_device(const std::string& root_name, const std::shared_ptr<device_base>&);

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

	class file final
	{
		std::unique_ptr<file_base> m_file;

		[[noreturn]] void xnull() const;
		[[noreturn]] void xfail() const;

	public:
		// Default constructor
		file() = default;

		// Open file with specified mode
		explicit file(const std::string& path, bs_t<open_mode> mode = ::fs::read);

		// Open memory for read
		explicit file(const void* ptr, std::size_t size);

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
		bool trunc(u64 length) const
		{
			if (!m_file) xnull();
			return m_file->trunc(length);
		}

		// Get file information
		stat_t stat() const
		{
			if (!m_file) xnull();
			return m_file->stat();
		}

		// Sync file buffers
		void sync() const
		{
			if (!m_file) xnull();
			return m_file->sync();
		}

		// Read the data from the file and return the amount of data written in buffer
		u64 read(void* buffer, u64 count) const
		{
			if (!m_file) xnull();
			return m_file->read(buffer, count);
		}

		// Write the data to the file and return the amount of data actually written
		u64 write(const void* buffer, u64 count) const
		{
			if (!m_file) xnull();
			return m_file->write(buffer, count);
		}

		// Change current position, returns resulting position
		u64 seek(s64 offset, seek_mode whence = seek_set) const
		{
			if (!m_file) xnull();
			return m_file->seek(offset, whence);
		}

		// Get file size
		u64 size() const
		{
			if (!m_file) xnull();
			return m_file->size();
		}

		// Get current position
		u64 pos() const
		{
			if (!m_file) xnull();
			return m_file->seek(0, seek_cur);
		}

		// Write std::string unconditionally
		const file& write(const std::string& str) const
		{
			if (write(str.data(), str.size()) != str.size()) xfail();
			return *this;
		}

		// Write POD unconditionally
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, const file&> write(const T& data) const
		{
			if (write(std::addressof(data), sizeof(T)) != sizeof(T)) xfail();
			return *this;
		}

		// Write POD std::vector unconditionally
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, const file&> write(const std::vector<T>& vec) const
		{
			if (write(vec.data(), vec.size() * sizeof(T)) != vec.size() * sizeof(T)) xfail();
			return *this;
		}

		// Read std::string, size must be set by resize() method
		bool read(std::string& str) const
		{
			return read(&str[0], str.size()) == str.size();
		}

		// Read std::string
		bool read(std::string& str, std::size_t size) const
		{
			str.resize(size);
			return read(&str[0], size) == size;
		}

		// Read POD, sizeof(T) is used
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(T& data) const
		{
			return read(&data, sizeof(T)) == sizeof(T);
		}

		// Read POD std::vector, size must be set by resize() method
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(std::vector<T>& vec) const
		{
			return read(vec.data(), sizeof(T) * vec.size()) == sizeof(T) * vec.size();
		}

		// Read POD std::vector
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, bool> read(std::vector<T>& vec, std::size_t size) const
		{
			vec.resize(size);
			return read(vec.data(), sizeof(T) * size) == sizeof(T) * size;
		}

		// Read POD (experimental)
		template <typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, T> read() const
		{
			T result;
			if (!read(result)) xfail();
			return result;
		}

		// Read full file to std::string
		std::string to_string() const
		{
			std::string result;
			result.resize(size());
			if (seek(0), !read(result)) xfail();
			return result;
		}

		// Read full file to std::vector
		template<typename T>
		std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, std::vector<T>> to_vector() const
		{
			std::vector<T> result;
			result.resize(size() / sizeof(T));
			if (seek(0), !read(result)) xfail();
			return result;
		}

		// Get native handle if available
		native_handle get_handle() const;

		// Gathered write
		u64 write_gather(const iovec_clone* buffers, u64 buf_count) const
		{
			if (!m_file) xnull();
			return m_file->write_gather(buffers, buf_count);
		}
	};

	class dir final
	{
		std::unique_ptr<dir_base> m_dir;

		[[noreturn]] void xnull() const;

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
		bool read(dir_entry& out) const
		{
			if (!m_dir) xnull();
			return m_dir->read(out);
		}

		// Reset to the beginning
		void rewind() const
		{
			if (!m_dir) xnull();
			return m_dir->rewind();
		}

		class iterator
		{
			const dir* m_parent;
			dir_entry m_entry;

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

			if (old_size + size < old_size)
			{
				fmt::raw_error("fs::container_stream<>::write(): overflow");
			}

			if (pos > old_size)
			{
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

	template <typename... Args>
	bool write_file(const std::string& path, bs_t<fs::open_mode> mode, const Args&... args)
	{
		if (fs::file f{path, mode})
		{
			// Write args sequentially
			(f.write(args), ...);
			return true;
		}

		return false;
	}

	file make_gather(std::vector<file>);
}
