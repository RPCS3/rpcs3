#pragma once

struct FileInfo
{
	std::string name;
	std::string fullName;
	bool exists;
	bool isDirectory;
	bool isWritable;
	uint64_t size;
	time_t atime;
	time_t mtime;
	time_t ctime;
};

bool get_file_info(const std::string& path, FileInfo& fileInfo);
bool rIsDir(const std::string& dir);
bool rRmdir(const std::string& dir);
bool rMkdir(const std::string& dir);
bool rMkpath(const std::string& path);
bool rRename(const std::string& from, const std::string& to);
bool rCopy(const std::string& from, const std::string& to, bool overwrite);
bool rExists(const std::string& file);
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
	o_create = 1 << 2,
	o_trunc = 1 << 3,
	o_excl = 1 << 4,
};

struct rfile_t final
{
#ifdef _WIN32
	using handle_type = void*;
#else
	using handle_type = int;
#endif

private:
	handle_type fd;

public:
	rfile_t();
	~rfile_t();
	explicit rfile_t(handle_type fd);
	explicit rfile_t(const std::string& filename, u32 mode = o_read);

	rfile_t(const rfile_t&) = delete;
	rfile_t(rfile_t&&) = delete;

	rfile_t& operator =(const rfile_t&) = delete;
	rfile_t& operator =(rfile_t&&) = delete;

	operator bool() const;

	bool open(const std::string& filename, u32 mode = o_read);
	bool is_opened() const;
	bool trunc(u64 size) const;
	bool close();

	u64 read(void* buffer, u64 count) const;
	u64 write(const void* buffer, u64 count) const;
	//u64 write(const std::string& str) const;
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
struct rFileName
{
	rFileName();
	rFileName(const rFileName& other);
	~rFileName();
	rFileName(const std::string& name);
	std::string GetFullPath();
	std::string GetPath();
	std::string GetName();
	std::string GetFullName();
	bool Normalize();

	void *handle;
};

// TODO: eliminate this:

template<typename T> __forceinline u8 Read8(T& f)
{
	u8 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline u16 Read16(T& f)
{
	be_t<u16> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline u32 Read32(T& f)
{
	be_t<u32> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline u64 Read64(T& f)
{
	be_t<u64> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline u16 Read16LE(T& f)
{
	u16 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline u32 Read32LE(T& f)
{
	u32 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline u64 Read64LE(T& f)
{
	u64 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

template<typename T> __forceinline void Write8(T& f, const u8 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write8(const rfile_t& f, const u8 data)
{
	f.write(&data, sizeof(data));
}

template<typename T> __forceinline void Write16LE(T& f, const u16 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write16LE(const rfile_t& f, const u16 data)
{
	f.write(&data, sizeof(data));
}

template<typename T> __forceinline void Write32LE(T& f, const u32 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write32LE(const rfile_t& f, const u32 data)
{
	f.write(&data, sizeof(data));
}

template<typename T> __forceinline void Write64LE(T& f, const u64 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write64LE(const rfile_t& f, const u64 data)
{
	f.write(&data, sizeof(data));
}

template<typename T> __forceinline void Write16(T& f, const u16 data)
{
	Write16LE(f, re16(data));
}

template<typename T> __forceinline void Write32(T& f, const u32 data)
{
	Write32LE(f, re32(data));
}

template<typename T> __forceinline void Write64(T& f, const u64 data)
{
	Write64LE(f, re64(data));
}
