#include "stdafx.h"
#include "Log.h"
#pragma warning(disable : 4996)
#include <wx/dir.h>
#include <wx/filename.h>
#include "rFile.h"

#ifdef _WIN32
#include <Windows.h>

#define GET_API_ERROR static_cast<u64>(GetLastError())

std::unique_ptr<wchar_t> ConvertUTF8ToWChar(const std::string& source)
{
	const size_t length = source.size() + 1; // size + null terminator

	const int size = source.size() < INT_MAX ? static_cast<int>(length) : throw std::length_error(__FUNCTION__);

	std::unique_ptr<wchar_t> buffer(new wchar_t[length]); // allocate buffer assuming that length is the max possible size

	if (!MultiByteToWideChar(CP_UTF8, 0, source.c_str(), size, buffer.get(), size))
	{
		LOG_ERROR(GENERAL, "ConvertUTF8ToWChar(source='%s') failed: 0x%llx", source, GET_API_ERROR);
	}

	return buffer;
}

time_t to_time_t(const FILETIME& ft)
{
	ULARGE_INTEGER v;
	v.LowPart = ft.dwLowDateTime;
	v.HighPart = ft.dwHighDateTime;

	return v.QuadPart / 10000000ULL - 11644473600ULL;
}

bool truncate_file(const std::string& file, uint64_t length)
{
	// open the file
	const auto handle = CreateFileW(ConvertUTF8ToWChar(file).get(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	LARGE_INTEGER distance;
	distance.QuadPart = length;

	// seek and truncate
	if (!SetFilePointerEx(handle, distance, NULL, FILE_BEGIN) || !SetEndOfFile(handle))
	{
		const auto error = GetLastError();
		CloseHandle(handle);
		SetLastError(error);
		return false;
	}

	return CloseHandle(handle);
}

#else
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#else
#include <sys/sendfile.h>
#endif
#include "errno.h"

#define GET_API_ERROR static_cast<u64>(errno)

#endif

bool get_file_info(const std::string& path, FileInfo& info)
{
	// TODO: Expand relative paths?
	info.fullName = path;

#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA attrs;
	if (!GetFileAttributesExW(ConvertUTF8ToWChar(path).get(), GetFileExInfoStandard, &attrs))
	{
		info.exists = false;
		info.isDirectory = false;
		info.isWritable = false;
		info.size = 0;
		return false;
	}

	info.exists = true;
	info.isDirectory = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.isWritable = (attrs.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = (uint64_t)attrs.nFileSizeLow | ((uint64_t)attrs.nFileSizeHigh << 32);
	info.atime = to_time_t(attrs.ftLastAccessTime);
	info.mtime = to_time_t(attrs.ftLastWriteTime);
	info.ctime = to_time_t(attrs.ftCreationTime);
#else
	struct stat64 file_info;
	if (stat64(path.c_str(), &file_info) < 0)
	{
		info.exists = false;
		info.isDirectory = false;
		info.isWritable = false;
		info.size = 0;
		return false;
	}

	info.exists = true;
	info.isDirectory = S_ISDIR(file_info.st_mode);
	info.isWritable = file_info.st_mode & 0200; // HACK: approximation
	info.size = file_info.st_size;
	info.atime = file_info.st_atime;
	info.mtime = file_info.st_mtime;
	info.ctime = file_info.st_ctime;
#endif
	return true;
}

bool rIsDir(const std::string& dir)
{
#ifdef _WIN32
	DWORD attrs;
	if ((attrs = GetFileAttributesW(ConvertUTF8ToWChar(dir).get())) == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	struct stat64 file_info;
	if (stat64(dir.c_str(), &file_info) < 0)
	{
		return false;
	}

	return S_ISDIR(file_info.st_mode);
#endif
}

bool rMkdir(const std::string& dir)
{
#ifdef _WIN32
	if (!CreateDirectoryW(ConvertUTF8ToWChar(dir).get(), NULL))
#else
	if (mkdir(dir.c_str(), 0777))
#endif
	{
		LOG_ERROR(GENERAL, "Error creating directory '%s': 0x%llx", dir, GET_API_ERROR);
		return false;
	}

	return true;
}

bool rMkpath(const std::string& path)
{
	size_t start=0, pos;
	std::string dir;
	bool ret;

	while (true) {
		if ((pos = path.find_first_of("/\\", start)) == std::string::npos)
			pos = path.length();

		dir = path.substr(0,pos++);
		start = pos;
		if(dir.size() == 0)
			continue;
#ifdef _WIN32
		if((ret = _mkdir(dir.c_str()) != 0) && errno != EEXIST){
#else
		if((ret = mkdir(dir.c_str(), 0777) != 0) && errno != EEXIST){
#endif
			return !ret;
		}
		if (pos >= path.length())
			return true;
	}
	return true;
}

bool rRmdir(const std::string& dir)
{
#ifdef _WIN32
	if (!RemoveDirectoryW(ConvertUTF8ToWChar(dir).get()))
#else
	if (rmdir(dir.c_str()))
#endif
	{
		LOG_ERROR(GENERAL, "Error deleting directory '%s': 0x%llx", dir, GET_API_ERROR);
		return false;
	}

	return true;
}

bool rRename(const std::string& from, const std::string& to)
{
	// TODO: Deal with case-sensitivity
#ifdef _WIN32
	if (!MoveFileW(ConvertUTF8ToWChar(from).get(), ConvertUTF8ToWChar(to).get()))
#else
	if (rename(from.c_str(), to.c_str()))
#endif
	{
		LOG_ERROR(GENERAL, "Error renaming '%s' to '%s': 0x%llx", from, to, GET_API_ERROR);
		return false;
	}

	return true;
}

#ifndef _WIN32

int OSCopyFile(const char* source, const char* destination, bool overwrite)
{
	/* This function was taken from http://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c */

	int input, output;
	if ((input = open(source, O_RDONLY)) == -1)
	{
		return -1;
	}
	if ((output = open(destination, O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL), 0666)) == -1)
	{
		close(input);
		return -1;
	}

	//Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
	//fcopyfile works on FreeBSD and OS X 10.5+ 
	int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
	//sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
	off_t bytesCopied = 0;
	struct stat fileinfo = { 0 };
	fstat(input, &fileinfo);
	int result = sendfile(output, input, &bytesCopied, fileinfo.st_size) == -1 ? -1 : 0;
#endif

	close(input);
	close(output);

	return result;
}
#endif

bool rCopy(const std::string& from, const std::string& to, bool overwrite)
{
#ifdef _WIN32
	if (!CopyFileW(ConvertUTF8ToWChar(from).get(), ConvertUTF8ToWChar(to).get(), !overwrite))
#else
	if (OSCopyFile(from.c_str(), to.c_str(), overwrite))
#endif
	{
		LOG_ERROR(GENERAL, "Error copying '%s' to '%s': 0x%llx", from, to, GET_API_ERROR);
		return false;
	}

	return true;
}

bool rExists(const std::string& file)
{
#ifdef _WIN32
	return GetFileAttributesW(ConvertUTF8ToWChar(file).get()) != 0xFFFFFFFF;
#else
	struct stat buffer;
	return stat(file.c_str(), &buffer) == 0;
#endif
}

bool rRemoveFile(const std::string& file)
{
#ifdef _WIN32
	if (!DeleteFileW(ConvertUTF8ToWChar(file).get()))
#else
	if (unlink(file.c_str()))
#endif
	{
		LOG_ERROR(GENERAL, "Error deleting file '%s': 0x%llx", file, GET_API_ERROR);
		return false;
	}

	return true;
}

bool rTruncate(const std::string& file, uint64_t length)
{
#ifdef _WIN32
	if (!truncate_file(file, length))
#else
	if (truncate64(file.c_str(), length))
#endif
	{
		LOG_ERROR(GENERAL, "Error resizing file '%s' to 0x%llx: 0x%llx", file, length, GET_API_ERROR);
		return false;
	}

	return true;
}

rfile_t::rfile_t()
#ifdef _WIN32
	: fd(INVALID_HANDLE_VALUE)
#else
	: fd(-1)
#endif
{
}

rfile_t::~rfile_t()
{
#ifdef _WIN32
	if (fd != INVALID_HANDLE_VALUE)
	{
		CloseHandle(fd);
	}
#else
	if (fd != -1)
	{
		::close(fd);
	}
#endif
}

rfile_t::rfile_t(const std::string& filename, u32 mode)
#ifdef _WIN32
	: fd(INVALID_HANDLE_VALUE)
#else
	: fd(-1)
#endif
{
	open(filename, mode);
}

rfile_t::operator bool() const
{
#ifdef _WIN32
	return fd != INVALID_HANDLE_VALUE;
#else
	return fd != -1;
#endif
}

void rfile_t::import(handle_type handle)
{
	this->~rfile_t();
	fd = handle;
}

bool rfile_t::open(const std::string& filename, u32 mode)
{
	this->~rfile_t();

#ifdef _WIN32
	DWORD access = 0;
	switch (mode & (o_read | o_write))
	{
	case o_read: access |= GENERIC_READ; break;
	case o_write: access |= GENERIC_WRITE; break;
	case o_read | o_write: access |= GENERIC_READ | GENERIC_WRITE; break;
	default:
	{
		LOG_ERROR(GENERAL, "rfile_t::open(): neither o_read nor o_write specified");
		return false;
	}
	}

	DWORD disp = 0;
	switch (mode & (o_create | o_trunc | o_excl))
	{
	case 0: disp = OPEN_EXISTING; break;
	case o_create: disp = OPEN_ALWAYS; break;
	case o_trunc: disp = TRUNCATE_EXISTING; break;
	case o_create | o_trunc: disp = CREATE_ALWAYS; break;
	case o_create | o_excl: disp = CREATE_NEW; break;
	}

	if (!disp || (mode & ~(o_read | o_write | o_create | o_trunc | o_excl)))
	{
		LOG_ERROR(GENERAL, "rfile_t::open(): unknown mode specified (0x%x)", mode);
		return false;
	}

	fd = CreateFileW(ConvertUTF8ToWChar(filename).get(), access, FILE_SHARE_READ, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
#else
	int flags = 0;

	switch (mode & (o_read | o_write))
	{
	case o_read: flags |= O_READ; break;
	case o_write: flags |= O_WRITE; break;
	case o_read | o_write: flags |= O_RDWR; break;
	default:
	{
		LOG_ERROR(GENERAL, "rfile_t::open(): neither o_read nor o_write specified");
		return false;
	}
	}

	if (mode & o_create) flags |= O_CREAT;
	if (mode & o_trunc) flags |= O_TRUNC;
	if (mode & o_excl) flags |= O_EXCL;

	if (((mode & o_excl) && (!(mode & o_create) || (mode & o_trunc))) || (mode & ~(o_read | o_write | o_create | o_trunc | o_excl)))
	{
		LOG_ERROR(GENERAL, "rfile_t::open(): unknown mode specified (0x%x)", mode);
		return false;
	}

	fd = ::open(filename.c_str(), flags, 0666);
#endif

	return is_opened();
}

bool rfile_t::is_opened() const
{
	return *this;
}

bool rfile_t::trunc(u64 size) const
{
#ifdef _WIN32
	LARGE_INTEGER old, pos;

	pos.QuadPart = 0;
	SetFilePointerEx(fd, pos, &old, FILE_CURRENT); // get old position

	pos.QuadPart = size;
	SetFilePointerEx(fd, pos, NULL, FILE_BEGIN); // set new position

	SetEndOfFile(fd); // change file size
	SetFilePointerEx(fd, old, NULL, FILE_BEGIN); // restore position

	return true; // TODO
#else
	return !ftruncate64(fd, size);
#endif
}

bool rfile_t::close()
{
#ifdef _WIN32
	if (CloseHandle(fd))
	{
		fd = INVALID_HANDLE_VALUE;
		return true;
	}
#else
	if (!::close(fd))
	{
		fd = -1;
		return true;
	}
#endif

	return false;
}

u64 rfile_t::read(void* buffer, u64 count) const
{
#ifdef _WIN32
	DWORD nread;
	if (!ReadFile(fd, buffer, count, &nread, NULL))
	{
		return -1;
	}

	return nread;
#else
	return read64(fd, buffer, count);
#endif
}

u64 rfile_t::write(const void* buffer, u64 count) const
{
#ifdef _WIN32
	DWORD nwritten;
	if (!WriteFile(fd, buffer, count, &nwritten, NULL))
	{
		return -1;
	}

	return nwritten;
#else
	return write64(fd, buffer, count);
#endif
}

u64 rfile_t::seek(u64 offset, u32 mode) const
{
	assert(mode < 3);

#ifdef _WIN32
	LARGE_INTEGER pos;
	pos.QuadPart = offset;

	if (!SetFilePointerEx(fd, pos, &pos, mode))
	{
		return -1;
	}

	return pos.QuadPart;
#else
	return lseek64(fd, offset, mode);
#endif
}

u64 rfile_t::size() const
{
#ifdef _WIN32
	LARGE_INTEGER size;
	if (!GetFileSizeEx(fd, &size))
	{
		return -1;
	}

	return size.QuadPart;
#else
	struct stat64 file_info;
	if (fstat64(fd, &file_info) < 0)
	{
		return -1;
	}

	return file_info.st_size;
#endif
}

rDir::rDir()
{
	handle = reinterpret_cast<void*>(new wxDir());
}

rDir::~rDir()
{
	delete reinterpret_cast<wxDir*>(handle);
}

rDir::rDir(const std::string &path)
{
	handle = reinterpret_cast<void*>(new wxDir(fmt::FromUTF8(path)));
}

bool rDir::Open(const std::string& path)
{
	return reinterpret_cast<wxDir*>(handle)->Open(fmt::FromUTF8(path));
}

bool rDir::IsOpened() const
{
	return reinterpret_cast<wxDir*>(handle)->IsOpened();
}

bool rDir::GetFirst(std::string *filename) const
{
	wxString str;
	bool res;
	res = reinterpret_cast<wxDir*>(handle)->GetFirst(&str);
	*filename = str.ToStdString();
	return res;
}

bool rDir::GetNext(std::string *filename) const
{
	wxString str;
	bool res;
	res = reinterpret_cast<wxDir*>(handle)->GetNext(&str);
	*filename = str.ToStdString();
	return res;
}
	
rFileName::rFileName()
{
	handle = reinterpret_cast<void*>(new wxFileName());
}

rFileName::~rFileName()
{
	delete reinterpret_cast<wxFileName*>(handle);
}

rFileName::rFileName(const rFileName& filename)
{
	handle = reinterpret_cast<void*>(new wxFileName(*reinterpret_cast<wxFileName*>(filename.handle)));
}


rFileName::rFileName(const std::string& name)
{
	handle = reinterpret_cast<void*>(new wxFileName(fmt::FromUTF8(name)));
}

std::string rFileName::GetFullPath()
{
	return fmt::ToUTF8(reinterpret_cast<wxFileName*>(handle)->GetFullPath());
}

std::string rFileName::GetPath()
{
	return fmt::ToUTF8(reinterpret_cast<wxFileName*>(handle)->GetPath());
}

std::string rFileName::GetName()
{
	return fmt::ToUTF8(reinterpret_cast<wxFileName*>(handle)->GetName());
}

std::string rFileName::GetFullName()
{
	return fmt::ToUTF8(reinterpret_cast<wxFileName*>(handle)->GetFullName());
}

bool rFileName::Normalize()
{
	return reinterpret_cast<wxFileName*>(handle)->Normalize();
}
