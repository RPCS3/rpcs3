#pragma once

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
		read = 1 << 0,
		write = 1 << 1,
		append = 1 << 2,
		create = 1 << 3,
		trunc = 1 << 4,
		excl = 1 << 5,
	};
};

enum class fse : u32 // filesystem (file or dir) error
{
	ok, // no error
	invalid_arguments,
};

enum : u32 // obsolete flags
{
	o_read = fom::read,
	o_write = fom::write,
	o_append = fom::append,
	o_create = fom::create,
	o_trunc = fom::trunc,
	o_excl = fom::excl,
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

	bool stat(const std::string& path, stat_t& info);
	bool exists(const std::string& path);
	bool is_file(const std::string& file);
	bool is_dir(const std::string& dir);
	bool remove_dir(const std::string& dir);
	bool create_dir(const std::string& dir);
	bool create_path(const std::string& path);
	bool rename(const std::string& from, const std::string& to);
	bool copy_file(const std::string& from, const std::string& to, bool overwrite);
	bool remove_file(const std::string& file);
	bool truncate_file(const std::string& file, u64 length);

	struct file final
	{
		using handle_type = std::intptr_t;

		static const handle_type null = -1;

	private:
		handle_type m_fd = null;

	public:
		file() = default;
		~file();
		explicit file(const std::string& filename, u32 mode = fom::read) { open(filename, mode); }

		file(const file&) = delete;
		file(file&&) = delete; // possibly TODO

		file& operator =(const file&) = delete;
		file& operator =(file&&) = delete; // possibly TODO

		operator bool() const { return m_fd != null; }

		void import(handle_type fd) { this->~file(); m_fd = fd; }

		bool open(const std::string& filename, u32 mode = fom::read);
		bool is_opened() const { return m_fd != null; }
		bool trunc(u64 size) const; // change file size (possibly appending zero bytes)
		bool stat(stat_t& info) const; // get file info
		bool close();

		u64 read(void* buffer, u64 count) const;
		u64 write(const void* buffer, u64 count) const;
		u64 seek(s64 offset, fsm seek_mode = fsm::begin) const;
		u64 size() const;
	};

	struct dir final
	{
#ifdef _WIN32
		using handle_type = intptr_t;
		using name_type = std::unique_ptr<wchar_t[]>;

		static const handle_type null = -1;
#else
		using handle_type = intptr_t;
		using name_type = std::unique_ptr<char[]>;

		static const handle_type null = 0;
#endif

	private:
		handle_type m_dd = null;
		name_type m_path;

	public:
		dir() = default;
		~dir();
		explicit dir(const std::string& dirname) { open(dirname); }

		dir(const dir&) = delete;
		dir(dir&&) = delete; // possibly TODO

		dir& operator =(const dir&) = delete;
		dir& operator =(dir&&) = delete; // possibly TODO

		operator bool() const { return m_path.operator bool(); }

		void import(handle_type dd, const std::string& path);

		bool open(const std::string& dirname);
		bool is_opened() const { return *this; }
		bool close();

		bool get_first(std::string& name, stat_t& info);
		//bool get_first(std::string& name);
		bool get_next(std::string& name, stat_t& info);
		//bool get_next(std::string& name);
	};
}
