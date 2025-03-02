#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/serialization.hpp"
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
		isfile,

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
	constexpr auto isfile  = +open_mode::isfile; // Ensure valid fs::file handle is not of directory

	constexpr auto write_new = write + create + excl;
	constexpr auto rewrite = write + create + trunc;

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
		bool is_symlink;
		bool is_writable;
		u64 size;
		s64 atime;
		s64 mtime;
		s64 ctime;

		using enable_bitcopy = std::true_type;

		constexpr bool operator==(const stat_t&) const = default;
	};

	static_assert(utils::Bitcopy<stat_t>);

	// Helper, layout is equal to iovec struct
	struct iovec_clone
	{
		const void* iov_base;
		usz iov_len;
	};

	struct file_id
	{
		std::string type;
		std::vector<u8> data;

		explicit operator bool() const;
		bool is_mirror_of(const file_id&) const;
		bool is_coherent_with(const file_id&) const;
	};

	// File handle base
	struct file_base
	{
		virtual ~file_base();

		[[noreturn]] virtual stat_t get_stat();
		virtual void sync();
		virtual bool trunc(u64 length) = 0;
		virtual u64 read(void* buffer, u64 size) = 0;
		virtual u64 read_at(u64 offset, void* buffer, u64 size) = 0;
		virtual u64 write(const void* buffer, u64 size) = 0;
		virtual u64 seek(s64 offset, seek_mode whence) = 0;
		virtual u64 size() = 0;
		virtual native_handle get_handle();
		virtual file_id get_id();
		virtual u64 write_gather(const iovec_clone* buffers, u64 buf_count);
		virtual void release()
		{
		}
	};

	// Directory entry (TODO)
	struct dir_entry : stat_t
	{
		std::string name{};

		dir_entry()
			: stat_t{}
		{
		}

		using enable_bitcopy = std::false_type;
	};

	static_assert(!utils::Bitcopy<dir_entry>);

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
		virtual bool create_symlink(const std::string& path);
		virtual bool rename(const std::string& from, const std::string& to);
		virtual bool remove(const std::string& path);
		virtual bool trunc(const std::string& path, u64 length);
		virtual bool utime(const std::string& path, s64 atime, s64 mtime);

		virtual std::unique_ptr<file_base> open(const std::string& path, bs_t<open_mode> mode) = 0;
		virtual std::unique_ptr<dir_base> open_dir(const std::string& path) = 0;
	};

	[[noreturn]] void xnull(std::source_location);
	[[noreturn]] void xfail(std::source_location);
	[[noreturn]] void xovfl();

	constexpr struct pod_tag_t{} pod_tag;

	// Get virtual device for specified path (nullptr for real path)
	shared_ptr<device_base> get_virtual_device(const std::string& path);

	// Set virtual device with specified name (nullptr for deletion)
	shared_ptr<device_base> set_virtual_device(const std::string& name, shared_ptr<device_base> device);

	// Try to get parent directory
	std::string_view get_parent_dir_view(std::string_view path, u32 parent_level = 1);

	// String (typical use) version
	inline std::string get_parent_dir(std::string_view path, u32 parent_level = 1)
	{
		return std::string{get_parent_dir_view(path, parent_level)};
	}

	// Get file information
	bool get_stat(const std::string& path, stat_t& info);

	// Check whether a file or a directory exists (not recommended, use is_file() or is_dir() instead)
	bool exists(const std::string& path);

	// Check whether the file exists and is NOT a directory
	bool is_file(const std::string& path);

	// Check whether the directory exists and is NOT a file
	bool is_dir(const std::string& path);

	// Check whether the path points to an existing symlink
	bool is_symlink(const std::string& path);

	// Get filesystem information
	bool statfs(const std::string& path, device_stat& info);

	// Delete empty directory
	bool remove_dir(const std::string& path);

	// Create directory
	bool create_dir(const std::string& path);

	// Create directories
	bool create_path(const std::string& path);

	// Create symbolic link
	bool create_symlink(const std::string& path, const std::string& target);

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

	public:
		// Default constructor
		file() = default;

		// Open file with specified mode
		explicit file(const std::string& path, bs_t<open_mode> mode = ::fs::read);

		static file from_native_handle(native_handle handle);

		// Open memory for read
		explicit file(const void* ptr, usz size);

		// Open file with specified args (forward to constructor)
		template <typename... Args>
		bool open(Args&&... args)
		{
			m_file.reset();
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

		void release_handle()
		{
			if (m_file)
			{
				release()->release();
			}
		}

		std::unique_ptr<file_base> release()
		{
			return std::exchange(m_file, nullptr);
		}

		// Change file size (possibly appending zero bytes)
		bool trunc(u64 length, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->trunc(length);
		}

		// Get file information
		stat_t get_stat(std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->get_stat();
		}

		// Sync file buffers
		void sync(std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->sync();
		}

		// Check if the handle is capable of reading (size * type_size) of bytes at the time of calling
		bool strict_read_check(u64 offset, u64 size, u64 type_size) const;

		// Read the data from the file and return the amount of data written in buffer
		u64 read(void* buffer, u64 count, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->read(buffer, count);
		}

		// Read the data from the file at specified offset in thread-safe manner
		u64 read_at(u64 offset, void* buffer, u64 count, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->read_at(offset, buffer, count);
		}

		// Write the data to the file and return the amount of data actually written
		u64 write(const void* buffer, u64 count, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->write(buffer, count);
		}

		// Change current position, returns resulting position
		u64 seek(s64 offset, seek_mode whence = seek_set, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->seek(offset, whence);
		}

		// Get file size
		u64 size(std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->size();
		}

		// Get current position
		u64 pos(std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->seek(0, seek_cur);
		}

		// Write std::basic_string unconditionally
		template <typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		const file& write(const std::basic_string<T>& str, std::source_location src_loc = std::source_location::current()) const
		{
			if (write(str.data(), str.size() * sizeof(T), src_loc) != str.size() * sizeof(T)) xfail(src_loc);
			return *this;
		}

		// Write POD unconditionally
		template <typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		const file& write(const T& data, std::source_location src_loc = std::source_location::current()) const
		{
			if (write(std::addressof(data), sizeof(T), src_loc) != sizeof(T)) xfail(src_loc);
			return *this;
		}

		// Write POD std::vector unconditionally
		template <typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		const file& write(const std::vector<T>& vec, std::source_location src_loc = std::source_location::current()) const
		{
			if (write(vec.data(), vec.size() * sizeof(T), src_loc) != vec.size() * sizeof(T)) xfail(src_loc);
			return *this;
		}

		// Read std::basic_string
		template <typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		bool read(std::basic_string<T>& str, usz _size = umax, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);

			if (_size != umax)
			{
				// If _size arg is too high std::bad_alloc may happen during resize and then we cannot error check
				if ((_size >= 0x10'0000 / sizeof(T)) && !strict_read_check(pos(), _size, sizeof(T)))
				{
					return false;
				}

				str.resize(_size);
			}

			return read(str.data(), sizeof(T) * str.size(), src_loc) == sizeof(T) * str.size();
		}

		// Read POD, sizeof(T) is used
		template <typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		bool read(T& data,
			pod_tag_t = pod_tag, std::source_location src_loc = std::source_location::current()) const
		{
			return read(std::addressof(data), sizeof(T), src_loc) == sizeof(T);
		}

		// Read POD std::vector
		template <typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		bool read(std::vector<T>& vec, usz _size = umax, bool use_offs = false, usz offset = umax, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);

			if (_size != umax)
			{
				// If _size arg is too high std::bad_alloc may happen during resize and then we cannot error check
				if ((_size >= 0x10'0000 / sizeof(T)) && !strict_read_check(use_offs ? offset : pos(), _size, sizeof(T)))
				{
					return false;
				}

				vec.resize(_size);
			}

			if (use_offs)
			{
				return read_at(offset, vec.data(), sizeof(T) * vec.size(), src_loc) == sizeof(T) * vec.size();
			}

			return read(vec.data(), sizeof(T) * vec.size(), src_loc) == sizeof(T) * vec.size();
		}

		// Read POD (experimental)
		template <typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		T read(pod_tag_t = pod_tag, std::source_location src_loc = std::source_location::current()) const
		{
			T result;
			if (!read(result, pod_tag, src_loc)) xfail(src_loc);
			return result;
		}

		// Read full file to std::basic_string
		template <typename T = char>
		std::basic_string<T> to_string(std::source_location src_loc = std::source_location::current()) const
		{
			std::basic_string<T> result;
			result.resize(size() / sizeof(T));
			seek(0);
			if (!read(result, result.size(), src_loc)) xfail(src_loc);
			return result;
		}

		// Read full file to std::vector
		template<typename T> requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		std::vector<T> to_vector(std::source_location src_loc = std::source_location::current()) const
		{
			std::vector<T> result;
			result.resize(size() / sizeof(T));
			if (!read(result, result.size(), true, 0, src_loc)) xfail(src_loc);
			return result;
		}

		// Get native handle if available
		native_handle get_handle() const;

		// Get file ID information (custom ID)
		file_id get_id() const;

		// Gathered write
		u64 write_gather(const iovec_clone* buffers, u64 buf_count, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_file) xnull(src_loc);
			return m_file->write_gather(buffers, buf_count);
		}
	};

	class dir final
	{
		std::unique_ptr<dir_base> m_dir{};

	public:
		dir() = default;

		// Open dir handle
		explicit dir(const std::string& path) noexcept
		{
			open(path);
		}

		// Open specified directory
		bool open(const std::string& path);

		// Check whether the handle is valid (opened directory)
		explicit operator bool() const noexcept
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
		bool read(dir_entry& out, std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_dir) xnull(src_loc);
			return m_dir->read(out);
		}

		// Reset to the beginning
		void rewind(std::source_location src_loc = std::source_location::current()) const
		{
			if (!m_dir) xnull(src_loc);
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

			iterator(const dir* parent, mode mode_ = mode::from_first) noexcept
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

			iterator operator++(int)
			{
				iterator old = *this;
				*this = {m_parent, mode::from_current};
				return old;
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

	// Get executable path
	std::string get_executable_path();

	// Get executable containing directory
	std::string get_executable_dir();

	// Get configuration directory. Set get_config_subdirectory to true to get the nested config dir on windows.
	const std::string& get_config_dir(bool get_config_subdirectory = false);

	// Get common cache directory
	const std::string& get_cache_dir();

	// Get common log directory
	const std::string& get_log_dir();

	// Temporary directory
	const std::string& get_temp_dir();

	std::string generate_neighboring_path(std::string_view source, u64 seed);

	// Unique pending file creation destined to be renamed to the destination file
	struct pending_file
	{
		fs::file file{};

		// This is meant to modify files atomically, overwriting is likely
		bool commit(bool overwrite = true);
		bool open(std::string_view path);

		pending_file() noexcept = default;

		pending_file(std::string_view path) noexcept
		{
			open(path);
		}

		pending_file(const pending_file&) = delete;
		pending_file& operator=(const pending_file&) = delete;
		~pending_file();

		const std::string& get_temp_path() const
		{
			return m_path;
		}

	private:
		std::string m_path{}; // Pending file path
		std::string m_dest{}; // Destination file path
	};

	// Delete directory and all its contents recursively
	bool remove_all(const std::string& path, bool remove_root = true, bool is_no_dir_ok = false);

	// Get size of all files recursively
	u64 get_dir_size(const std::string& path, u64 rounding_alignment = 1, atomic_t<bool>* cancel_flag = nullptr);

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
		xdev,
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

		container_stream(T&& obj, const stat_t& init_stat = {})
			: obj(std::forward<T>(obj))
			, pos(0)
			, m_stat(init_stat)
		{
		}

		~container_stream() override
		{
		}

		bool trunc(u64 length) override
		{
			obj.resize(length);
			update_time(true);
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
					update_time();
					return max;
				}
			}

			return 0;
		}

		u64 read_at(u64 offset, void* buffer, u64 size) override
		{
			const u64 end = obj.size();

			if (offset < end)
			{
				// Get readable size
				if (const u64 max = std::min<u64>(size, end - offset))
				{
					std::copy(obj.cbegin() + offset, obj.cbegin() + offset + max, static_cast<value_type*>(buffer));
					update_time();
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

			if (size) update_time(true);
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

		stat_t get_stat() override
		{
			return m_stat;
		}

	private:
		stat_t m_stat{};

		void update_time(bool write = false)
		{
			// TODO: Accurate timestamps
			m_stat.atime++;

			if (write)
			{
				m_stat.mtime++;
				m_stat.mtime = std::max(m_stat.atime, m_stat.mtime);
				m_stat.ctime = m_stat.mtime;
			}
		}
	};

	template <typename T>
	file make_stream(T&& container = T{}, const stat_t& stat = stat_t{})
	{
		file result;
		result.reset(std::make_unique<container_stream<T>>(std::forward<T>(container), stat));
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

	stx::generator<dir_entry&> list_dir_recursively(const std::string& path);
}
