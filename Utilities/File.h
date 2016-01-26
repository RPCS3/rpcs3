#pragma once

namespace fom // file open mode
{
	enum open_mode : u32
	{
		read = 1 << 0, // enable reading
		write = 1 << 1, // enable writing
		append = 1 << 2, // enable appending (always write to the end of file)
		create = 1 << 3, // create file if it doesn't exist
		trunc = 1 << 4, // clear opened file if it's not empty
		excl = 1 << 5, // failure if the file already exists (used with `create`)

		rewrite = write | create | trunc,
	};
};

namespace fs
{
	enum seek_mode : u32 // file seek mode
	{
		seek_set,
		seek_cur,
		seek_end,
	};

	struct stat_t
	{
		bool is_directory;
		bool is_writable;
		u64 size;
		s64 atime;
		s64 mtime;
		s64 ctime;
	};

	// Get parent directory for the path (returns empty string on failure)
	std::string get_parent_dir(const std::string& path);

	// Get file information
	bool stat(const std::string& path, stat_t& info);

	// Check whether a file or a directory exists (not recommended, use is_file() or is_dir() instead)
	bool exists(const std::string& path);

	// Check whether the file exists and is NOT a directory
	bool is_file(const std::string& path);

	// Check whether the directory exists and is NOT a file
	bool is_dir(const std::string& path);

	// Delete empty directory
	bool remove_dir(const std::string& path);

	// Create directory
	bool create_dir(const std::string& path);

	// Create directories
	bool create_path(const std::string& path);

	// Rename (move) file or directory
	bool rename(const std::string& from, const std::string& to);

	// Copy file contents
	bool copy_file(const std::string& from, const std::string& to, bool overwrite);

	// Delete file
	bool remove_file(const std::string& path);

	// Change file size (possibly appending zeros)
	bool truncate_file(const std::string& path, u64 length);

	class file final
	{
		using handle_type = std::intptr_t;

		constexpr static handle_type null = -1;

		handle_type m_fd = null;

		friend class file_read_map;
		friend class file_write_map;

	public:
		file() = default;

		explicit file(const std::string& path, u32 mode = fom::read)
		{
			open(path, mode);
		}

		file(file&& other)
			: m_fd(other.m_fd)
		{
			other.m_fd = null;
		}

		file& operator =(file&& right)
		{
			std::swap(m_fd, right.m_fd);
			return *this;
		}

		~file();

		// Check whether the handle is valid (opened file)
		bool is_opened() const
		{
			return m_fd != null;
		}

		// Check whether the handle is valid (opened file)
		explicit operator bool() const
		{
			return is_opened();
		}

		// Open specified file with specified mode
		bool open(const std::string& path, u32 mode = fom::read);

		// Change file size (possibly appending zero bytes)
		bool trunc(u64 size) const;

		// Get file information
		bool stat(stat_t& info) const;

		// Close the file explicitly (destructor automatically closes the file)
		void close();

		// Read the data from the file and return the amount of data written in buffer
		u64 read(void* buffer, u64 count) const;

		// Write the data to the file and return the amount of data actually written
		u64 write(const void* buffer, u64 count) const;

		// Move file pointer
		u64 seek(s64 offset, seek_mode whence = seek_set) const;

		// Get file size
		u64 size() const;

		// Write std::string unconditionally
		const file& write(const std::string& str) const
		{
			CHECK_ASSERTION(write(str.data(), str.size()) == str.size());
			return *this;
		}

		// Write POD unconditionally
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, const file&> write(const T& data) const
		{
			CHECK_ASSERTION(write(std::addressof(data), sizeof(T)) == sizeof(T));
			return *this;
		}

		// Write POD std::vector unconditionally
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, const file&> write(const std::vector<T>& vec) const
		{
			CHECK_ASSERTION(write(vec.data(), vec.size() * sizeof(T)) == vec.size() * sizeof(T));
			return *this;
		}

		// Read std::string, size must be set by resize() method
		bool read(std::string& str) const
		{
			return read(&str[0], str.size()) == str.size();
		}

		// Read POD, sizeof(T) is used
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, bool> read(T& data) const
		{
			return read(&data, sizeof(T)) == sizeof(T);
		}

		// Read POD std::vector, size must be set by resize() method
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, bool> read(std::vector<T>& vec) const
		{
			return read(vec.data(), sizeof(T) * vec.size()) == sizeof(T) * vec.size();
		}

		// Read POD (experimental)
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, T> read() const
		{
			T result;
			CHECK_ASSERTION(read(result));
			return result;
		}

		// Read full file to std::string
		std::string to_string() const
		{
			std::string result;
			result.resize(size());
			CHECK_ASSERTION(seek(0) != -1 && read(result));
			return result;
		}

		// Read full file to std::vector
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, std::vector<T>> to_vector() const
		{
			std::vector<T> result;
			result.resize(size() / sizeof(T));
			CHECK_ASSERTION(seek(0) != -1 && read(result));
			return result;
		}
	};

	// TODO
	class file_read_map final
	{
		char* m_ptr = nullptr;
		u64 m_size;

	public:
		file_read_map() = default;

		file_read_map(file_read_map&& right)
			: m_ptr(right.m_ptr)
			, m_size(right.m_size)
		{
			right.m_ptr = 0;
		}

		file_read_map& operator =(file_read_map&& right)
		{
			std::swap(m_ptr, right.m_ptr);
			std::swap(m_size, right.m_size);
			return *this;
		}

		file_read_map(const file& f)
		{
			reset(f);
		}

		~file_read_map()
		{
			reset();
		}

		// Open file mapping
		void reset(const file& f);
		
		// Close file mapping
		void reset();

		// Get pointer
		operator const char*() const
		{
			return m_ptr;
		}
	};

	// TODO
	class file_write_map final
	{
		char* m_ptr = nullptr;
		u64 m_size;

	public:
		file_write_map() = default;

		file_write_map(file_write_map&& right)
			: m_ptr(right.m_ptr)
			, m_size(right.m_size)
		{
			right.m_ptr = 0;
		}

		file_write_map& operator =(file_write_map&& right)
		{
			std::swap(m_ptr, right.m_ptr);
			std::swap(m_size, right.m_size);
			return *this;
		}

		file_write_map(const file& f)
		{
			reset(f);
		}

		~file_write_map()
		{
			reset();
		}

		// Open file mapping
		void reset(const file& f);

		// Close file mapping
		void reset();

		// Get pointer
		operator char*() const
		{
			return m_ptr;
		}
	};

	class dir final
	{
		std::unique_ptr<char[]> m_path;
		std::intptr_t m_dd; // handle (aux)

	public:
		dir() = default;

		explicit dir(const std::string& dirname)
		{
			open(dirname);
		}

		dir(dir&& other)
			: m_dd(other.m_dd)
			, m_path(std::move(other.m_path))
		{
		}

		dir& operator =(dir&& right)
		{
			std::swap(m_dd, right.m_dd);
			std::swap(m_path, right.m_path);
			return *this;
		}

		~dir();

		// Check whether the handle is valid (opened directory)
		bool is_opened() const
		{
			return m_path.operator bool();
		}

		// Check whether the handle is valid (opened directory)
		explicit operator bool() const
		{
			return is_opened();
		}

		// Open specified directory
		bool open(const std::string& dirname);
		
		// Close the directory explicitly (destructor automatically closes the directory)
		void close();

		// Get next directory entry (UTF-8 name and file stat)
		bool read(std::string& name, stat_t& info);

		bool first(std::string& name, stat_t& info);

		struct entry
		{
			std::string name;
			stat_t info;
		};

		class iterator
		{
			entry m_entry;
			dir* m_parent;

		public:
			enum class mode
			{
				from_first,
				from_current
			};

			iterator(dir* parent, mode mode_ = mode::from_first)
				: m_parent(parent)
			{
				if (!m_parent)
				{
					return;
				}

				if (mode_ == mode::from_first)
				{
					m_parent->first(m_entry.name, m_entry.info);
				}
				else
				{
					m_parent->read(m_entry.name, m_entry.info);
				}

				if (m_entry.name.empty())
				{
					m_parent = nullptr;
				}
			}

			entry& operator *()
			{
				return m_entry;
			}

			iterator& operator++()
			{
				*this = { m_parent, mode::from_current };
				return *this;
			}

			bool operator !=(const iterator& rhs) const
			{
				return m_parent != rhs.m_parent;
			}
		};

		iterator begin()
		{
			return{ this };
		}

		iterator end()
		{
			return{ nullptr };
		}
	};

	// Get configuration directory
	const std::string& get_config_dir();

	// Get executable directory
	const std::string& get_executable_dir();
}
