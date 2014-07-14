#pragma once

#include <string>

enum rSeekMode
{
	rFromStart,
	rFromCurrent,
	rFromEnd
};

class rFile
{
public:
	enum OpenMode
	{
		read,
		write,
		read_write,
		write_append,
		write_excl
	};
	rFile();
	rFile(const rFile& other) = delete;
	~rFile();
	rFile(const std::string& filename, rFile::OpenMode open = rFile::read);
	rFile(int fd);
	static bool Access(const std::string &filename, rFile::OpenMode mode);
	size_t 	Write(const void *buffer, size_t count);
	bool Write(const std::string &text);
	bool Close();
	bool Create(const std::string &filename, bool overwrite = false, int access = 0666);
	bool Open(const std::string &filename, rFile::OpenMode mode = rFile::read, int access = 0666);
	static bool Exists(const std::string&);
	bool 	IsOpened() const;
	size_t	Length() const;
	size_t  Read(void *buffer, size_t count);
	size_t 	Seek(size_t ofs, rSeekMode mode = rFromStart);
	size_t Tell() const;

	void *handle;
};

std::string rGetCwd();
bool rRmdir(const std::string& dir);
bool rMkdir(const std::string& dir);
bool rMkpath(const std::string& path);
bool rRename(const std::string &from, const std::string &to);
bool rExists(const std::string &path);
bool rDirExists(const std::string &path);
bool rFileExists(const std::string &path);
bool rRemoveFile(const std::string &path);

bool rIsWritable(const std::string& path);
bool rIsReadable(const std::string& path);
bool rIsExecutable(const std::string& path);

struct rDir
{
	rDir();
	~rDir();
	rDir(const rDir& other) = delete;
	rDir(const std::string &path);
	bool Open(const std::string& path);
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
