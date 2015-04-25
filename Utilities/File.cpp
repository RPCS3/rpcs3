#include "stdafx.h"
#include "Log.h"
#include "File.h"

#ifdef _WIN32
#include <Windows.h>

#define GET_API_ERROR static_cast<u64>(GetLastError())

static_assert(fs::file::null == intptr_t(INVALID_HANDLE_VALUE) && fs::dir::null == fs::file::null, "Check fs::file::null definition");

std::unique_ptr<wchar_t[]> ConvertUTF8ToWChar(const std::string& source)
{
	const size_t length = source.size() + 1; // size + null terminator

	const int size = source.size() < INT_MAX ? static_cast<int>(length) : throw std::length_error(__FUNCTION__);

	std::unique_ptr<wchar_t[]> buffer(new wchar_t[length]); // allocate buffer assuming that length is the max possible size

	if (!MultiByteToWideChar(CP_UTF8, 0, source.c_str(), size, buffer.get(), size))
	{
		LOG_ERROR(GENERAL, "ConvertUTF8ToWChar(source='%s') failed: 0x%llx", source, GET_API_ERROR);
	}

	return buffer;
}

std::string ConvertWCharToUTF8(const wchar_t* source)
{
	const int size = WideCharToMultiByte(CP_UTF8, 0, source, -1 /* NTS */, NULL, 0, NULL, NULL); // size

	if (size < 1) throw std::length_error(__FUNCTION__); // ???

	std::string result;

	result.resize(size - 1);

	if (!WideCharToMultiByte(CP_UTF8, 0, source, -1 /* NTS */, &result.front(), size, NULL, NULL))
	{
		LOG_ERROR(GENERAL, "ConvertWCharToUTF8() failed: 0x%llx", GET_API_ERROR);
	}

	return result;
}

time_t to_time_t(const ULARGE_INTEGER& ft)
{
	return ft.QuadPart / 10000000ULL - 11644473600ULL;
}

time_t to_time_t(const LARGE_INTEGER& ft)
{
	ULARGE_INTEGER v;
	v.LowPart = ft.LowPart;
	v.HighPart = ft.HighPart;

	return to_time_t(v);
}

time_t to_time_t(const FILETIME& ft)
{
	ULARGE_INTEGER v;
	v.LowPart = ft.dwLowDateTime;
	v.HighPart = ft.dwHighDateTime;

	return to_time_t(v);
}

bool truncate_file(const std::string& file, u64 length)
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

bool fs::stat(const std::string& path, stat_t& info)
{
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA attrs;
	if (!GetFileAttributesExW(ConvertUTF8ToWChar(path).get(), GetFileExInfoStandard, &attrs))
	{
		return false;
	}

	info.is_directory = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.is_writable = (attrs.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = (u64)attrs.nFileSizeLow | ((u64)attrs.nFileSizeHigh << 32);
	info.atime = to_time_t(attrs.ftLastAccessTime);
	info.mtime = to_time_t(attrs.ftLastWriteTime);
	info.ctime = to_time_t(attrs.ftCreationTime);
#else
	struct stat64 file_info;
	if (stat64(path.c_str(), &file_info) < 0)
	{
		return false;
	}

	info.is_directory = S_ISDIR(file_info.st_mode);
	info.is_writable = file_info.st_mode & 0200; // HACK: approximation
	info.size = file_info.st_size;
	info.atime = file_info.st_atime;
	info.mtime = file_info.st_mtime;
	info.ctime = file_info.st_ctime;
#endif

	return true;
}

bool fs::exists(const std::string& path)
{
#ifdef _WIN32
	return GetFileAttributesW(ConvertUTF8ToWChar(path).get()) != 0xFFFFFFFF;
#else
	struct stat buffer;
	return stat(path.c_str(), &buffer) == 0;
#endif
}

bool fs::is_file(const std::string& file)
{
#ifdef _WIN32
	DWORD attrs;
	if ((attrs = GetFileAttributesW(ConvertUTF8ToWChar(file).get())) == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
	struct stat64 file_info;
	if (stat64(file.c_str(), &file_info) < 0)
	{
		return false;
	}

	return !S_ISDIR(file_info.st_mode);
#endif
}

bool fs::is_dir(const std::string& dir)
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

bool fs::create_dir(const std::string& dir)
{
#ifdef _WIN32
	if (!CreateDirectoryW(ConvertUTF8ToWChar(dir).get(), NULL))
#else
	if (mkdir(dir.c_str(), 0777))
#endif
	{
		LOG_WARNING(GENERAL, "Error creating directory '%s': 0x%llx", dir, GET_API_ERROR);
		return false;
	}

	return true;
}

bool fs::create_path(const std::string& path)
{
	size_t start = 0;

	while (true)
	{
		// maybe it could be more optimal if goes from the end recursively
		size_t pos = path.find_first_of("/\\", start);

		if (pos == std::string::npos)
		{
			pos = path.length();
		}

		std::string dir = path.substr(0, pos);

		start = ++pos;

		if (dir.size() == 0)
		{
			continue;
		}

		if (!is_dir(dir))
		{
			// if doesn't exist or not a dir
			if (!create_dir(dir))
			{
				// if creating failed
				return false;
			}
		}

		if (pos >= path.length())
		{
			break;
		}
	}

	return true;
}

bool fs::remove_dir(const std::string& dir)
{
#ifdef _WIN32
	if (!RemoveDirectoryW(ConvertUTF8ToWChar(dir).get()))
#else
	if (rmdir(dir.c_str()))
#endif
	{
		LOG_WARNING(GENERAL, "Error deleting directory '%s': 0x%llx", dir, GET_API_ERROR);
		return false;
	}

	return true;
}

bool fs::rename(const std::string& from, const std::string& to)
{
	// TODO: Deal with case-sensitivity
#ifdef _WIN32
	if (!MoveFileW(ConvertUTF8ToWChar(from).get(), ConvertUTF8ToWChar(to).get()))
#else
	if (rename(from.c_str(), to.c_str()))
#endif
	{
		LOG_WARNING(GENERAL, "Error renaming '%s' to '%s': 0x%llx", from, to, GET_API_ERROR);
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

bool fs::copy_file(const std::string& from, const std::string& to, bool overwrite)
{
#ifdef _WIN32
	if (!CopyFileW(ConvertUTF8ToWChar(from).get(), ConvertUTF8ToWChar(to).get(), !overwrite))
#else
	if (OSCopyFile(from.c_str(), to.c_str(), overwrite))
#endif
	{
		LOG_WARNING(GENERAL, "Error copying '%s' to '%s': 0x%llx", from, to, GET_API_ERROR);
		return false;
	}

	return true;
}

bool fs::remove_file(const std::string& file)
{
#ifdef _WIN32
	if (!DeleteFileW(ConvertUTF8ToWChar(file).get()))
#else
	if (unlink(file.c_str()))
#endif
	{
		LOG_WARNING(GENERAL, "Error deleting file '%s': 0x%llx", file, GET_API_ERROR);
		return false;
	}

	return true;
}

bool fs::truncate_file(const std::string& file, u64 length)
{
#ifdef _WIN32
	if (!::truncate_file(file, length))
#else
	if (truncate64(file.c_str(), length))
#endif
	{
		LOG_WARNING(GENERAL, "Error resizing file '%s' to 0x%llx: 0x%llx", file, length, GET_API_ERROR);
		return false;
	}

	return true;
}

fs::file::~file()
{
	if (m_fd != null)
	{
#ifdef _WIN32
		CloseHandle((HANDLE)m_fd);
#else
		::close(fd);
#endif
	}
}

bool fs::file::open(const std::string& filename, u32 mode)
{
	this->~file();

#ifdef _WIN32
	DWORD access = 0;
	switch (mode & (o_read | o_write | o_append))
	{
	case o_read: access |= GENERIC_READ; break;
	case o_read | o_append: access |= GENERIC_READ; break;
	case o_write: access |= GENERIC_WRITE; break;
	case o_write | o_append: access |= FILE_APPEND_DATA; break;
	case o_read | o_write: access |= GENERIC_READ | GENERIC_WRITE; break;
	case o_read | o_write | o_append: access |= GENERIC_READ | FILE_APPEND_DATA; break;
	default:
	{
		LOG_ERROR(GENERAL, "fs::file::open('%s') failed: neither o_read nor o_write specified (0x%x)", filename, mode);
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
	case o_create | o_excl | o_trunc: disp = CREATE_NEW; break;
	}

	if (!disp || (mode & ~(o_read | o_write | o_append | o_create | o_trunc | o_excl)))
	{
		LOG_ERROR(GENERAL, "fs::file::open('%s') failed: unknown mode specified (0x%x)", filename, mode);
		return false;
	}

	m_fd = (intptr_t)CreateFileW(ConvertUTF8ToWChar(filename).get(), access, FILE_SHARE_READ, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
#else
	int flags = 0;

	switch (mode & (o_read | o_write))
	{
	case o_read: flags |= O_RDONLY; break;
	case o_write: flags |= O_WRONLY; break;
	case o_read | o_write: flags |= O_RDWR; break;
	default:
	{
		LOG_ERROR(GENERAL, "fs::file::open('%s') failed: neither o_read nor o_write specified (0x%x)", filename, mode);
		return false;
	}
	}

	if (mode & o_append) flags |= O_APPEND;
	if (mode & o_create) flags |= O_CREAT;
	if (mode & o_trunc) flags |= O_TRUNC;
	if (mode & o_excl) flags |= O_EXCL;

	if (((mode & o_excl) && !(mode & o_create)) || (mode & ~(o_read | o_write | o_append | o_create | o_trunc | o_excl)))
	{
		LOG_ERROR(GENERAL, "fs::file::open('%s') failed: unknown mode specified (0x%x)", filename, mode);
		return false;
	}

	m_fd = ::open(filename.c_str(), flags, 0666);
#endif

	if (m_fd == null)
	{
		LOG_WARNING(GENERAL, "fs::file::open('%s', 0x%x) failed: error 0x%llx", filename, mode, GET_API_ERROR);
		return false;
	}

	return true;
}

bool fs::file::trunc(u64 size) const
{
#ifdef _WIN32
	LARGE_INTEGER old, pos;

	pos.QuadPart = 0;
	SetFilePointerEx((HANDLE)m_fd, pos, &old, FILE_CURRENT); // get old position

	pos.QuadPart = size;
	SetFilePointerEx((HANDLE)m_fd, pos, NULL, FILE_BEGIN); // set new position

	SetEndOfFile((HANDLE)m_fd); // change file size

	SetFilePointerEx((HANDLE)m_fd, old, NULL, FILE_BEGIN); // restore position

	return true; // TODO
#else
	return !ftruncate64(m_fd, size);
#endif
}

bool fs::file::stat(stat_t& info) const
{
#ifdef _WIN32
	FILE_BASIC_INFO basic_info;

	if (!GetFileInformationByHandleEx((HANDLE)m_fd, FileBasicInfo, &basic_info, sizeof(FILE_BASIC_INFO)))
	{
		return false;
	}

	info.is_directory = (basic_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.is_writable = (basic_info.FileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = this->size();
	info.atime = to_time_t(basic_info.LastAccessTime);
	info.mtime = to_time_t(basic_info.ChangeTime);
	info.ctime = to_time_t(basic_info.CreationTime);
#else
	struct stat64 file_info;
	if (fstat64(m_fd, &file_info) < 0)
	{
		return false;
	}

	info.is_directory = S_ISDIR(file_info.st_mode);
	info.is_writable = file_info.st_mode & 0200; // HACK: approximation
	info.size = file_info.st_size;
	info.atime = file_info.st_atime;
	info.mtime = file_info.st_mtime;
	info.ctime = file_info.st_ctime;
#endif

	return true;
}

bool fs::file::close()
{
	if (m_fd == null)
	{
		return false;
	}

	auto fd = m_fd;
	m_fd = null;

#ifdef _WIN32
	return CloseHandle((HANDLE)fd);
#else
	return !::close(fd);
#endif
}

u64 fs::file::read(void* buffer, u64 count) const
{
#ifdef _WIN32
	DWORD nread;
	if (!ReadFile((HANDLE)m_fd, buffer, count, &nread, NULL))
	{
		return -1;
	}

	return nread;
#else
	return ::read(m_fd, buffer, count);
#endif
}

u64 fs::file::write(const void* buffer, u64 count) const
{
#ifdef _WIN32
	DWORD nwritten;
	if (!WriteFile((HANDLE)m_fd, buffer, count, &nwritten, NULL))
	{
		return -1;
	}

	return nwritten;
#else
	return ::write(m_fd, buffer, count);
#endif
}

u64 fs::file::seek(u64 offset, u32 mode) const
{
	assert(mode < 3);

#ifdef _WIN32
	LARGE_INTEGER pos;
	pos.QuadPart = offset;

	if (!SetFilePointerEx((HANDLE)m_fd, pos, &pos, mode))
	{
		return -1;
	}

	return pos.QuadPart;
#else
	return lseek64(m_fd, offset, mode);
#endif
}

u64 fs::file::size() const
{
#ifdef _WIN32
	LARGE_INTEGER size;
	if (!GetFileSizeEx((HANDLE)m_fd, &size))
	{
		return -1;
	}

	return size.QuadPart;
#else
	struct stat64 file_info;
	if (fstat64(m_fd, &file_info) < 0)
	{
		return -1;
	}

	return file_info.st_size;
#endif
}

fs::dir::~dir()
{
	if (m_dd != null)
	{
#ifdef _WIN32
		FindClose((HANDLE)m_dd);
#else
		::closedir(m_dd);
#endif
	}
}

void fs::dir::import(handle_type dd, const std::string& path)
{
	if (m_dd != null)
	{
#ifdef _WIN32
		FindClose((HANDLE)m_dd);
#else
		::closedir(m_dd);
#endif
	}

	m_dd = dd;

#ifdef _WIN32
	m_path = ConvertUTF8ToWChar(path);
#else
	m_path.reset(new char[path.size() + 1]);
	memcpy(m_path.get(), path.c_str(), path.size() + 1);
#endif
}

bool fs::dir::open(const std::string& dirname)
{
	if (m_dd != null)
	{
#ifdef _WIN32
		FindClose((HANDLE)m_dd);
#else
		::closedir(m_dd);
#endif
	}

	m_dd = null;

	m_path.reset();

	if (!is_dir(dirname))
	{
		return false;
	}

#ifdef _WIN32
	m_path = ConvertUTF8ToWChar(dirname + "/*");
#else
	m_path.reset(new char[dirname.size() + 1]);
	memcpy(m_path.get(), dirname.c_str(), dirname.size() + 1);
#endif

	return true;
}

bool fs::dir::close()
{
	if (m_dd == null)
	{
		if (m_path)
		{
			m_path.reset();
			return true;
		}
		else
		{
			return false;
		}
	}

	auto dd = m_dd;
	m_dd = null;

	m_path.reset();

#ifdef _WIN32
	return FindClose((HANDLE)dd);
#else
	return !::closedir(dd);
#endif
}

bool fs::dir::get_first(std::string& name, stat_t& info)
{
	if (m_dd != null) // close previous handle
	{
#ifdef _WIN32
		FindClose((HANDLE)m_dd);
#else
		::closedir(m_dd);
#endif
	}

	m_dd = null;

	if (!m_path)
	{
		return false;
	}

#ifdef _WIN32
	WIN32_FIND_DATAW found;

	m_dd = (intptr_t)FindFirstFileW(m_path.get(), &found);
#else
	m_dd = ::opendir(m_path.get());
#endif

	return get_next(name, info);
}

bool fs::dir::get_next(std::string& name, stat_t& info)
{
	if (m_dd == null)
	{
		return false;
	}

#ifdef _WIN32
	WIN32_FIND_DATAW found;

	if (!FindNextFileW((HANDLE)m_dd, &found))
	{
		return false;
	}

	name = ConvertWCharToUTF8(found.cFileName);

	info.is_directory = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.is_writable = (found.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = ((u64)found.nFileSizeHigh << 32) | (u64)found.nFileSizeLow;
	info.atime = to_time_t(found.ftLastAccessTime);
	info.mtime = to_time_t(found.ftLastWriteTime);
	info.ctime = to_time_t(found.ftCreationTime);
#else
	const auto found = ::readdir(m_dd);

	struct stat64 file_info;
	if (fstatat64(::dirfd(m_dd), found.d_name, &file_info, 0) < 0)
	{
		return false;
	}

	name = found.d_name;

	info.is_directory = S_ISDIR(file_info.st_mode);
	info.is_writable = file_info.st_mode & 0200; // HACK: approximation
	info.size = file_info.st_size;
	info.atime = file_info.st_atime;
	info.mtime = file_info.st_mtime;
	info.ctime = file_info.st_ctime;
#endif

	return true;
}
