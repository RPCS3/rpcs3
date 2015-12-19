#include "stdafx.h"
#include "File.h"

#ifdef _WIN32

#include <cwchar>
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#include <Windows.h>

static std::unique_ptr<wchar_t[]> to_wchar(const std::string& source)
{
	const auto buf_size = source.size() + 1; // size + null terminator

	const int size = source.size() < INT_MAX ? static_cast<int>(buf_size) : throw EXCEPTION("Invalid source length (0x%llx)", source.size());

	std::unique_ptr<wchar_t[]> buffer(new wchar_t[buf_size]); // allocate buffer assuming that length is the max possible size

	if (!MultiByteToWideChar(CP_UTF8, 0, source.c_str(), size, buffer.get(), size))
	{
		throw EXCEPTION("MultiByteToWideChar() failed (0x%x).", GetLastError());
	}

	return buffer;
}

static void to_utf8(std::string& result, const wchar_t* source)
{
	const auto length = std::wcslen(source);

	const int buf_size = length <= INT_MAX / 3 ? static_cast<int>(length) * 3 + 1 : throw EXCEPTION("Invalid source length (0x%llx)", length);

	result.resize(buf_size); // set max possible length for utf-8 + null terminator

	if (const int nwritten = WideCharToMultiByte(CP_UTF8, 0, source, static_cast<int>(length) + 1, &result.front(), buf_size, NULL, NULL))
	{
		result.resize(nwritten - 1); // fix the size, remove null terminator
	}
	else
	{
		throw EXCEPTION("WideCharToMultiByte() failed (0x%x).", GetLastError());
	}
}

static time_t to_time(const ULARGE_INTEGER& ft)
{
	return ft.QuadPart / 10000000ULL - 11644473600ULL;
}

static time_t to_time(const LARGE_INTEGER& ft)
{
	ULARGE_INTEGER v;
	v.LowPart = ft.LowPart;
	v.HighPart = ft.HighPart;

	return to_time(v);
}

static time_t to_time(const FILETIME& ft)
{
	ULARGE_INTEGER v;
	v.LowPart = ft.dwLowDateTime;
	v.HighPart = ft.dwHighDateTime;

	return to_time(v);
}

#else

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#include <mach-o/dyld.h>
#else
#include <sys/sendfile.h>
#endif

#endif

std::string fs::get_parent_dir(const std::string& path)
{
	// Search upper bound (set to the last character, npos for empty string)
	auto last = path.size() - 1;

#ifdef _WIN32
	const auto& delim = "/\\";
#else
	const auto& delim = "/";
#endif

	while (true)
	{
		const auto pos = path.find_last_of(delim, last, sizeof(delim) - 1);

		// Contiguous slashes are ignored at the end
		if (std::exchange(last, pos - 1) != pos)
		{
			// Return empty string if the path doesn't contain at least 2 elements
			return path.substr(0, pos != -1 && path.find_last_not_of(delim, pos, sizeof(delim) - 1) != -1 ? pos : 0);
		}
	}
}

static const auto test_get_parent_dir = []() -> bool
{
	// Success:
	CHECK_ASSERTION(fs::get_parent_dir("/x/y///") == "/x");
	CHECK_ASSERTION(fs::get_parent_dir("/x/y/") == "/x");
	CHECK_ASSERTION(fs::get_parent_dir("/x/y") == "/x");
	CHECK_ASSERTION(fs::get_parent_dir("x:/y") == "x:");
	CHECK_ASSERTION(fs::get_parent_dir("//x/y") == "//x");

	// Failure:
	CHECK_ASSERTION(fs::get_parent_dir("").empty());
	CHECK_ASSERTION(fs::get_parent_dir("x/").empty());
	CHECK_ASSERTION(fs::get_parent_dir("x").empty());
	CHECK_ASSERTION(fs::get_parent_dir("x///").empty());
	CHECK_ASSERTION(fs::get_parent_dir("/x/").empty());
	CHECK_ASSERTION(fs::get_parent_dir("/x").empty());
	CHECK_ASSERTION(fs::get_parent_dir("/").empty());
	CHECK_ASSERTION(fs::get_parent_dir("//").empty());
	CHECK_ASSERTION(fs::get_parent_dir("//x").empty());
	CHECK_ASSERTION(fs::get_parent_dir("//x/").empty());
	CHECK_ASSERTION(fs::get_parent_dir("///").empty());
	CHECK_ASSERTION(fs::get_parent_dir("///x").empty());
	CHECK_ASSERTION(fs::get_parent_dir("///x/").empty());

	return false;
}();

bool fs::stat(const std::string& path, stat_t& info)
{
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA attrs;
	if (!GetFileAttributesExW(to_wchar(path).get(), GetFileExInfoStandard, &attrs))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}

	info.is_directory = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.is_writable = (attrs.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = (u64)attrs.nFileSizeLow | ((u64)attrs.nFileSizeHigh << 32);
	info.atime = to_time(attrs.ftLastAccessTime);
	info.mtime = to_time(attrs.ftLastWriteTime);
	info.ctime = to_time(attrs.ftCreationTime);
#else
	struct ::stat file_info;
	if (::stat(path.c_str(), &file_info) != 0)
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
	if (GetFileAttributesW(to_wchar(path).get()) == INVALID_FILE_ATTRIBUTES)
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}

	return true;
#else
	struct ::stat file_info;
	return !::stat(path.c_str(), &file_info);
#endif
}

bool fs::is_file(const std::string& path)
{
#ifdef _WIN32
	const DWORD attrs = GetFileAttributesW(to_wchar(path).get());
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}
#else
	struct ::stat file_info;
	if (::stat(path.c_str(), &file_info) != 0)
	{
		return false;
	}
#endif

	// TODO: correct file type check
#ifdef _WIN32
	if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
#else
	if (S_ISDIR(file_info.st_mode))
#endif
	{
		errno = EEXIST;
		return false;
	}

	return true;
}

bool fs::is_dir(const std::string& path)
{
#ifdef _WIN32
	const DWORD attrs = GetFileAttributesW(to_wchar(path).get());
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		}

		return false;
	}
#else
	struct ::stat file_info;
	if (::stat(path.c_str(), &file_info) != 0)
	{
		return false;
	}
#endif

#ifdef _WIN32
	if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0)
#else
	if (!S_ISDIR(file_info.st_mode))
#endif
	{
		errno = EEXIST;
		return false;
	}

	return true;
}

bool fs::create_dir(const std::string& path)
{
#ifdef _WIN32
	if (!CreateDirectoryW(to_wchar(path).get(), NULL))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_ALREADY_EXISTS: errno = EEXIST; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}

	return true;
#else
	return !::mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}

bool fs::create_path(const std::string& path)
{
	const auto& parent = get_parent_dir(path);

	if (!parent.empty() && !is_dir(parent) && !create_path(parent))
	{
		return false;
	}

	return create_dir(path);
}

bool fs::remove_dir(const std::string& path)
{
#ifdef _WIN32
	if (!RemoveDirectoryW(to_wchar(path).get()))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}

	return true;
#else
	return !::rmdir(path.c_str());
#endif
}

bool fs::rename(const std::string& from, const std::string& to)
{
#ifdef _WIN32
	if (!MoveFileW(to_wchar(from).get(), to_wchar(to).get()))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.\nFrom: %s\nTo: %s", error, from, to);
		}

		return false;
	}

	return true;
#else
	return !::rename(from.c_str(), to.c_str());
#endif
}

bool fs::copy_file(const std::string& from, const std::string& to, bool overwrite)
{
#ifdef _WIN32
	if (!CopyFileW(to_wchar(from).get(), to_wchar(to).get(), !overwrite))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.\nFrom: %s\nTo: %s", error, from, to);
		}

		return false;
	}

	return true;
#else
	/* Source: http://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c */

	const int input = ::open(from.c_str(), O_RDONLY);
	if (input == -1)
	{
		return false;
	}

	const int output = ::open(to.c_str(), O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (output == -1)
	{
		const int err = errno;

		::close(input);
		errno = err;
		return false;
	}

	// Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
	// fcopyfile works on FreeBSD and OS X 10.5+ 
	if (::fcopyfile(input, output, 0, COPYFILE_ALL))
#else
	// sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
	off_t bytes_copied = 0;
	struct ::stat fileinfo = { 0 };
	if (::fstat(input, &fileinfo) || ::sendfile(output, input, &bytes_copied, fileinfo.st_size))
#endif
	{
		const int err = errno;

		::close(input);
		::close(output);
		errno = err;
		return false;
	}

	::close(input);
	::close(output);
	return true;
#endif
}

bool fs::remove_file(const std::string& path)
{
#ifdef _WIN32
	if (!DeleteFileW(to_wchar(path).get()))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}

	return true;
#else
	return !::unlink(path.c_str());
#endif
}

bool fs::truncate_file(const std::string& path, u64 length)
{
#ifdef _WIN32
	// Open the file
	const auto handle = CreateFileW(to_wchar(path).get(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}

	LARGE_INTEGER distance;
	distance.QuadPart = length;

	// Seek and truncate
	if (!SetFilePointerEx(handle, distance, NULL, FILE_BEGIN) || !SetEndOfFile(handle))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_NEGATIVE_SEEK: errno = EINVAL; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (length=0x%llx).", error, length);
		}

		CloseHandle(handle);
		return false;
	}

	CloseHandle(handle);
	return true;
#else
	return !::truncate(path.c_str(), length);
#endif
}

fs::file::~file()
{
	if (m_fd != null)
	{
#ifdef _WIN32
		CloseHandle((HANDLE)m_fd);
#else
		::close(m_fd);
#endif
	}
}

bool fs::file::open(const std::string& path, u32 mode)
{
	this->close();

#ifdef _WIN32
	DWORD access = 0;
	switch (mode & (fom::read | fom::write | fom::append))
	{
	case fom::read: access |= GENERIC_READ; break;
	case fom::read | fom::append: access |= GENERIC_READ; break;
	case fom::write: access |= GENERIC_WRITE; break;
	case fom::write | fom::append: access |= FILE_APPEND_DATA; break;
	case fom::read | fom::write: access |= GENERIC_READ | GENERIC_WRITE; break;
	case fom::read | fom::write | fom::append: access |= GENERIC_READ | FILE_APPEND_DATA; break;
	}

	DWORD disp = 0;
	switch (mode & (fom::create | fom::trunc | fom::excl))
	{
	case 0: disp = OPEN_EXISTING; break;
	case fom::create: disp = OPEN_ALWAYS; break;
	case fom::trunc: disp = TRUNCATE_EXISTING; break;
	case fom::create | fom::trunc: disp = CREATE_ALWAYS; break;
	case fom::create | fom::excl: disp = CREATE_NEW; break;
	case fom::create | fom::excl | fom::trunc: disp = CREATE_NEW; break;
	default:
		errno = EINVAL;
		return false;
	}

	m_fd = (std::intptr_t)CreateFileW(to_wchar(path).get(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_fd == null)
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_FILE_NOT_FOUND: errno = ENOENT; break;
		case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
		case ERROR_FILE_EXISTS:    errno = EEXIST; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x (%s).", error, path);
		}

		return false;
	}

	return true;
#else
	int flags = 0;

	switch (mode & (fom::read | fom::write))
	{
	case fom::read: flags |= O_RDONLY; break;
	case fom::write: flags |= O_WRONLY; break;
	case fom::read | fom::write: flags |= O_RDWR; break;
	}

	if (mode & fom::append) flags |= O_APPEND;
	if (mode & fom::create) flags |= O_CREAT;
	if (mode & fom::trunc) flags |= O_TRUNC;
	if (mode & fom::excl) flags |= O_EXCL;

	m_fd = ::open(path.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	return m_fd != null;
#endif
}

bool fs::file::trunc(u64 size) const
{
#ifdef _WIN32
	LARGE_INTEGER old, pos;

	pos.QuadPart = 0;
	if (!SetFilePointerEx((HANDLE)m_fd, pos, &old, FILE_CURRENT)) // get old position
	{
		switch (DWORD error = GetLastError())
		{
		case ERROR_INVALID_HANDLE: errno = EBADF; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return false;
	}

	pos.QuadPart = size;
	if (!SetFilePointerEx((HANDLE)m_fd, pos, NULL, FILE_BEGIN)) // set new position
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_NEGATIVE_SEEK: errno = EINVAL; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return false;
	}

	const BOOL result = SetEndOfFile((HANDLE)m_fd); // change file size

	if (!result)
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case 0:
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}
	}

	if (!SetFilePointerEx((HANDLE)m_fd, old, NULL, FILE_BEGIN) && result) // restore position
	{
		if (DWORD error = GetLastError())
		{
			throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}
	}

	return result != FALSE;
#else
	return !::ftruncate(m_fd, size);
#endif
}

bool fs::file::stat(stat_t& info) const
{
#ifdef _WIN32
	FILE_BASIC_INFO basic_info;
	if (!GetFileInformationByHandleEx((HANDLE)m_fd, FileBasicInfo, &basic_info, sizeof(FILE_BASIC_INFO)))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_INVALID_HANDLE: errno = EBADF; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return false;
	}

	info.is_directory = (basic_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.is_writable = (basic_info.FileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = this->size();
	info.atime = to_time(basic_info.LastAccessTime);
	info.mtime = to_time(basic_info.ChangeTime);
	info.ctime = to_time(basic_info.CreationTime);
#else
	struct ::stat file_info;
	if (::fstat(m_fd, &file_info) < 0)
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

void fs::file::close()
{
	if (m_fd == null)
	{
		return /*true*/;
	}

	const auto fd = m_fd;
	m_fd = null;

#ifdef _WIN32
	if (!CloseHandle((HANDLE)fd))
	{
		throw EXCEPTION("CloseHandle() failed (fd=0x%llx, 0x%x)", fd, GetLastError());
	}
#else
	if (::close(fd) != 0)
	{
		throw EXCEPTION("close() failed (fd=0x%llx, errno=%d)", fd, errno);
	}
#endif
}

u64 fs::file::read(void* buffer, u64 count) const
{
	// TODO (call ReadFile multiple times if count is too big)
	const int size = count <= INT_MAX ? static_cast<int>(count) : throw EXCEPTION("Invalid count (0x%llx)", count);

#ifdef _WIN32
	DWORD nread;
	if (!ReadFile((HANDLE)m_fd, buffer, size, &nread, NULL))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_INVALID_HANDLE: errno = EBADF; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return -1;
	}

	return nread;
#else
	return ::read(m_fd, buffer, size);
#endif
}

u64 fs::file::write(const void* buffer, u64 count) const
{
	// TODO (call WriteFile multiple times if count is too big)
	const int size = count <= INT_MAX ? static_cast<int>(count) : throw EXCEPTION("Invalid count (0x%llx)", count);

#ifdef _WIN32
	DWORD nwritten;
	if (!WriteFile((HANDLE)m_fd, buffer, size, &nwritten, NULL))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_INVALID_HANDLE: errno = EBADF; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return -1;
	}

	return nwritten;
#else
	return ::write(m_fd, buffer, size);
#endif
}

u64 fs::file::seek(s64 offset, seek_mode whence) const
{
#ifdef _WIN32
	LARGE_INTEGER pos;
	pos.QuadPart = offset;

	DWORD mode;
	switch (whence)
	{
	case seek_set: mode = FILE_BEGIN; break;
	case seek_cur: mode = FILE_CURRENT; break;
	case seek_end: mode = FILE_END; break;
	default:
	{
		errno = EINVAL;
		return -1;
	}
	}

	if (!SetFilePointerEx((HANDLE)m_fd, pos, &pos, mode))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_INVALID_HANDLE: errno = EBADF; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return -1;
	}

	return pos.QuadPart;
#else
	int mode;
	switch (whence)
	{
	case seek_set: mode = SEEK_SET; break;
	case seek_cur: mode = SEEK_CUR; break;
	case seek_end: mode = SEEK_END; break;
	default:
	{
		errno = EINVAL;
		return -1;
	}
	}

	return ::lseek(m_fd, offset, mode);
#endif
}

u64 fs::file::size() const
{
#ifdef _WIN32
	LARGE_INTEGER size;
	if (!GetFileSizeEx((HANDLE)m_fd, &size))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_INVALID_HANDLE: errno = EBADF; break;
		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return -1;
	}

	return size.QuadPart;
#else
	struct ::stat file_info;
	if (::fstat(m_fd, &file_info) != 0)
	{
		return -1;
	}

	return file_info.st_size;
#endif
}

fs::dir::~dir()
{
	if (m_path)
	{
#ifdef _WIN32
		if (m_dd != -1) FindClose((HANDLE)m_dd);
#else
		::closedir((DIR*)m_dd);
#endif
	}
}

void fs::file_read_map::reset(const file& f)
{
	reset();

	if (f)
	{
#ifdef _WIN32
		const HANDLE handle = ::CreateFileMapping((HANDLE)f.m_fd, NULL, PAGE_READONLY, 0, 0, NULL);
		m_ptr = (char*)::MapViewOfFile(handle, FILE_MAP_READ, 0, 0, 0);
		m_size = f.size();
		::CloseHandle(handle);
#else
		m_ptr = (char*)::mmap(nullptr, m_size = f.size(), PROT_READ, MAP_SHARED, f.m_fd, 0);
		if (m_ptr == (void*)-1) m_ptr = nullptr;
#endif
	}
}

void fs::file_read_map::reset()
{
	if (m_ptr)
	{
#ifdef _WIN32
		::UnmapViewOfFile(m_ptr);
#else
		::munmap(m_ptr, m_size);
#endif
	}
}

bool fs::dir::open(const std::string& dirname)
{
	this->close();

#ifdef _WIN32
	if (!is_dir(dirname))
	{
		return false;
	}

	m_dd = -1;
#else
	const auto ptr = ::opendir(m_path.get());
	if (!ptr)
	{
		return false;
	}

	m_dd = reinterpret_cast<std::intptr_t>(ptr);
#endif

	m_path.reset(new char[dirname.size() + 1]);
	std::memcpy(m_path.get(), dirname.c_str(), dirname.size() + 1);

	return true;
}

void fs::dir::close()
{
	if (!m_path)
	{
		return /*true*/;
	}

	m_path.reset();

#ifdef _WIN32
	CHECK_ASSERTION(m_dd == -1 || FindClose((HANDLE)m_dd));
#else
	CHECK_ASSERTION(!::closedir((DIR*)m_dd));
#endif
}

bool fs::dir::read(std::string& name, stat_t& info)
{
	if (!m_path)
	{
		errno = EINVAL;
		return false;
	}

#ifdef _WIN32
	const bool is_first = m_dd == -1;

	WIN32_FIND_DATAW found;

	if (is_first)
	{
		m_dd = (std::intptr_t)FindFirstFileW(to_wchar(m_path.get() + "/*"s).get(), &found);
	}

	if (is_first && m_dd == -1 || !is_first && !FindNextFileW((HANDLE)m_dd, &found))
	{
		// TODO: convert Win32 error code to errno
		switch (DWORD error = GetLastError())
		{
		case ERROR_NO_MORE_FILES:
		{
			name.clear();
			return true;
		}

		default: throw EXCEPTION("Unknown Win32 error: 0x%x.", error);
		}

		return false;
	}

	to_utf8(name, found.cFileName);

	info.is_directory = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.is_writable = (found.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = ((u64)found.nFileSizeHigh << 32) | (u64)found.nFileSizeLow;
	info.atime = to_time(found.ftLastAccessTime);
	info.mtime = to_time(found.ftLastWriteTime);
	info.ctime = to_time(found.ftCreationTime);
#else
	const auto found = ::readdir((DIR*)m_dd);
	if (!found)
	{
		name.clear();
		return true;
	}

	struct ::stat file_info;
	if (::fstatat(::dirfd((DIR*)m_dd), found->d_name, &file_info, 0) != 0)
	{
		return false;
	}

	name = found->d_name;

	info.is_directory = S_ISDIR(file_info.st_mode);
	info.is_writable = file_info.st_mode & 0200; // HACK: approximation
	info.size = file_info.st_size;
	info.atime = file_info.st_atime;
	info.mtime = file_info.st_mtime;
	info.ctime = file_info.st_ctime;
#endif

	return true;
}

bool fs::dir::first(std::string& name, stat_t& info)
{
#ifdef _WIN32
	if (m_path && m_dd != -1)
	{
		CHECK_ASSERTION(FindClose((HANDLE)m_dd));
		m_dd = -1;
	}
#else
	if (m_path)
	{
		::rewinddir((DIR*)m_dd);
	}
#endif

	return read(name, info);
}

const std::string& fs::get_config_dir()
{
	// Use magic static for dir initialization
	static const std::string s_dir = []
	{
#ifdef _WIN32
		return get_executable_dir(); // ?
#else
		std::string dir;

		if (const char* home = ::getenv("XDG_CONFIG_HOME"))
			dir = home;
		else if (const char* home = ::getenv("HOME"))
			dir = home + "/.config"s;
		else // Just in case
			dir = "./config";

		dir += "/rpcs3/";

		if (!is_dir(dir) && !create_path(dir))
		{
			std::printf("Failed to create configuration directory '%s' (%d).\n", dir.c_str(), errno);
			return get_executable_dir();
		}

		return dir;
#endif
	}();

	return s_dir;
}

const std::string& fs::get_executable_dir()
{
	// Use magic static for dir initialization
	static const std::string s_dir = []
	{
		std::string dir;

#ifdef _WIN32
		wchar_t buf[2048];
		if (GetModuleFileName(NULL, buf, ::size32(buf)) - 1 >= ::size32(buf) - 1)
		{
			MessageBoxA(0, fmt::format("GetModuleFileName() failed (0x%x).", GetLastError()).c_str(), "fs::get_config_dir()", MB_ICONERROR);
			return dir; // empty
		}
	
		to_utf8(dir, buf); // Convert to UTF-8

		std::replace(dir.begin(), dir.end(), '\\', '/');

#elif __APPLE__
		char buf[4096];
		u32 size = sizeof(buf);
		if (_NSGetExecutablePath(buf, &size))
		{
			std::printf("_NSGetExecutablePath() failed (size=0x%x).\n", size);
			return dir; // empty
		}

		dir = buf;
#else
		char buf[4096];
		const auto size = ::readlink("/proc/self/exe", buf, sizeof(buf));
		if (size <= 0 || size >= sizeof(buf))
		{
			std::printf("readlink(/proc/self/exe) failed (%d).\n", errno);
			return dir; // empty
		}

		dir.assign(buf, size);
#endif
	
		// Leave only path
		dir.resize(dir.rfind('/') + 1);
		return dir;
	}();

	return s_dir;
}
