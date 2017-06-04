#pragma once
#include "basic_types.h"
#include <string>
#include <memory>

namespace common
{
	enum class fsm : u32 // file seek mode
	{
		begin,
		cur,
		end,
	};

	namespace fom // file open mode
	{
		enum : u32
		{
			read = 1 << 0, // enable reading
			write = 1 << 1, // enable writing
			append = 1 << 2, // enable appending (always write to the end of file)
			create = 1 << 3, // create file if it doesn't exist
			trunc = 1 << 4, // clear opened file if it's not empty
			excl = 1 << 5, // failure if the file already exists (used with `create`)

			rewrite = write | create | trunc, // write + create + trunc
		};
	};

	enum class fse : u32 // filesystem (file or dir) error
	{
		ok, // no error
		invalid_arguments,
	};

	namespace fs
	{
		thread_local extern fse g_tls_error;

		struct stat_t
		{
			bool is_directory;
			bool is_writable;
			u64 size;
			s64 atime;
			s64 mtime;
			s64 ctime;
		};

		// Get file information
		bool stat(const std::string& path, stat_t& info);

		// Check whether a file or a directory exists (not recommended, use is_file() or is_dir() instead)
		bool exists(const std::string& path);

		// Check whether the file exists and is NOT a directory
		bool is_file(const std::string& file);

		// Check whether the directory exists and is NOT a file
		bool is_dir(const std::string& dir);

		// Delete empty directory
		bool remove_dir(const std::string& dir);

		// Create directory
		bool create_dir(const std::string& dir);

		// Create directories
		bool create_path(const std::string& path);

		// Rename (move) file or directory
		bool rename(const std::string& from, const std::string& to);

		// Copy file contents
		bool copy_file(const std::string& from, const std::string& to, bool overwrite);

		// Delete file
		bool remove_file(const std::string& file);

		// Change file size (possibly appending zeros)
		bool truncate_file(const std::string& file, u64 length);

		class file final
		{
			using handle_type = std::intptr_t;

			constexpr static handle_type null = -1;

			handle_type m_fd = null;

			friend class file_ptr;

		public:
			file() = default;

			explicit file(const std::string& filename, u32 mode = fom::read)
			{
				open(filename, mode);
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
			bool open(const std::string& filename, u32 mode = fom::read);

			// Change file size (possibly appending zero bytes)
			bool trunc(u64 size) const;

			// Get file information
			bool stat(stat_t& info) const;

			// Close the file explicitly (destructor automatically closes the file)
			bool close();

			// Read the data from the file and return the amount of data written in buffer
			u64 read(void* buffer, u64 count) const;

			// Write the data to the file and return the amount of data actually written
			u64 write(const void* buffer, u64 count) const;

			// Move file pointer
			u64 seek(s64 offset, fsm seek_mode = fsm::begin) const;

			// Get file size
			u64 size() const;

			// Write std::string
			const file& operator <<(const std::string& str) const;
		};

		class file_ptr final
		{
			char* m_ptr = nullptr;
			u64 m_size;

		public:
			file_ptr() = default;

			file_ptr(file_ptr&& right)
				: m_ptr(right.m_ptr)
				, m_size(right.m_size)
			{
				right.m_ptr = 0;
			}

			file_ptr& operator =(file_ptr&& right)
			{
				std::swap(m_ptr, right.m_ptr);
				std::swap(m_size, right.m_size);
				return *this;
			}

			file_ptr(const file& f)
			{
				reset(f);
			}

			~file_ptr()
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
			bool close();

			// Get next directory entry (UTF-8 name and file stat)
			bool read(std::string& name, stat_t& info);
		};
	}
}