#pragma once

struct FileInfo
{
	bool exists;
	bool isDirectory;
	bool isWritable;
	uint64_t size;
	time_t atime;
	time_t mtime;
	time_t ctime;
};

bool get_file_info(const std::string& path, FileInfo& fileInfo);
bool rExists(const std::string& path);
bool rIsFile(const std::string& file);
bool rIsDir(const std::string& dir);
bool rRmDir(const std::string& dir);
bool rMkDir(const std::string& dir);
bool rMkPath(const std::string& path);
bool rRename(const std::string& from, const std::string& to);
bool rCopy(const std::string& from, const std::string& to, bool overwrite);
bool rRemoveFile(const std::string& file);
bool rTruncate(const std::string& file, uint64_t length);

enum rfile_seek_mode : u32
{
	from_begin,
	from_cur,
	from_end,
};

enum rfile_open_mode : u32
{
	o_read = 1 << 0,
	o_write = 1 << 1,
	o_append = 1 << 2,
	o_create = 1 << 3,
	o_trunc = 1 << 4,
	o_excl = 1 << 5,
};

struct rfile_t final
{
#ifdef _WIN32
	using handle_type = void*;
#else
	using handle_type = intptr_t;
#endif

private:
	handle_type fd;

public:
	rfile_t();
	~rfile_t();
	explicit rfile_t(const std::string& filename, u32 mode = o_read);

	rfile_t(const rfile_t&) = delete;
	rfile_t(rfile_t&&) = delete; // possibly TODO

	rfile_t& operator =(const rfile_t&) = delete;
	rfile_t& operator =(rfile_t&&) = delete; // possibly TODO

	operator bool() const; // check is_opened()

	void import(handle_type fd); // replace file handle

	bool open(const std::string& filename, u32 mode = o_read);
	bool is_opened() const; // check whether the file is opened
	bool trunc(u64 size) const; // change file size (possibly appending zero bytes)
	bool stat(FileInfo& info) const; // get file info
	bool close();

	u64 read(void* buffer, u64 count) const;
	u64 write(const void* buffer, u64 count) const;
	u64 seek(u64 offset, u32 mode = from_begin) const;
	u64 size() const;
};

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
