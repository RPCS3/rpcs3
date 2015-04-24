#pragma once

enum file_seek_mode : u32
{
	from_begin,
	from_cur,
	from_end,
};

enum file_open_mode : u32
{
	o_read = 1 << 0,
	o_write = 1 << 1,
	o_append = 1 << 2,
	o_create = 1 << 3,
	o_trunc = 1 << 4,
	o_excl = 1 << 5,
};

namespace fs
{
	struct stat_t
	{
		bool exists;
		bool is_directory;
		bool is_writable;
		uint64_t size;
		time_t atime;
		time_t mtime;
		time_t ctime;
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
	bool truncate_file(const std::string& file, uint64_t length);

	struct file final
	{
#ifdef _WIN32
		using handle_type = void*;
#else
		using handle_type = intptr_t;
#endif

	private:
		handle_type fd;

	public:
		file();
		~file();
		explicit file(const std::string& filename, u32 mode = o_read);

		file(const file&) = delete;
		file(file&&) = delete; // possibly TODO

		file& operator =(const file&) = delete;
		file& operator =(file&&) = delete; // possibly TODO

		operator bool() const; // check is_opened()

		void import(handle_type fd); // replace file handle

		bool open(const std::string& filename, u32 mode = o_read);
		bool is_opened() const; // check whether the file is opened
		bool trunc(u64 size) const; // change file size (possibly appending zero bytes)
		bool stat(stat_t& info) const; // get file info
		bool close();

		u64 read(void* buffer, u64 count) const;
		u64 write(const void* buffer, u64 count) const;
		u64 seek(u64 offset, u32 mode = from_begin) const;
		u64 size() const;
	};
}

struct rDir
{
	rDir();
	~rDir();
	rDir(const rDir& other) = delete;
	rDir(const std::string &path);
	bool Open(const std::string& path);
	bool IsOpened() const;
	static bool Exists(const std::string &path);
	bool 	GetFirst(std::string *filename) const;
	bool 	GetNext(std::string *filename) const;

	void *handle;
};
