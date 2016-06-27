#include "File.h"
#include "StrFmt.h"
#include "Macro.h"
#include "SharedMutex.h"

#include <unordered_map>
#include <algorithm>
#include <cerrno>

#ifdef _WIN32

#include <cwchar>
#include <Windows.h>

static std::unique_ptr<wchar_t[]> to_wchar(const std::string& source)
{
	const auto buf_size = source.size() + 1; // size + null terminator

	const int size = source.size() < INT_MAX ? static_cast<int>(buf_size) : throw fmt::exception("to_wchar(): invalid source length (0x%llx)", source.size());

	std::unique_ptr<wchar_t[]> buffer(new wchar_t[buf_size]); // allocate buffer assuming that length is the max possible size

	if (!MultiByteToWideChar(CP_UTF8, 0, source.c_str(), size, buffer.get(), size))
	{
		throw fmt::exception("to_wchar(): MultiByteToWideChar() failed: error %u.", GetLastError());
	}

	return buffer;
}

static void to_utf8(std::string& result, const wchar_t* source)
{
	const auto length = std::wcslen(source);

	const int buf_size = length <= INT_MAX / 3 ? static_cast<int>(length) * 3 + 1 : throw fmt::exception("to_utf8(): invalid source length (0x%llx)", length);

	result.resize(buf_size); // set max possible length for utf-8 + null terminator

	if (const int nwritten = WideCharToMultiByte(CP_UTF8, 0, source, static_cast<int>(length) + 1, &result.front(), buf_size, NULL, NULL))
	{
		result.resize(nwritten - 1); // fix the size, remove null terminator
	}
	else
	{
		throw fmt::exception("to_utf8(): WideCharToMultiByte() failed: error %u.", GetLastError());
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

static fs::error to_error(DWORD e)
{
	switch (e)
	{
	case ERROR_FILE_NOT_FOUND: return fs::error::noent;
	case ERROR_PATH_NOT_FOUND: return fs::error::noent;
	case ERROR_ALREADY_EXISTS: return fs::error::exist;
	case ERROR_FILE_EXISTS: return fs::error::exist;
	case ERROR_NEGATIVE_SEEK: return fs::error::inval;
	default: throw fmt::exception("Unknown Win32 error: %u.", e);
	}
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

static fs::error to_error(int e)
{
	switch (e)
	{
	case ENOENT: return fs::error::noent;
	case EEXIST: return fs::error::exist;
	case EINVAL: return fs::error::inval;
	default: throw fmt::exception("Unknown system error: %d.", e);
	}
}

#endif

namespace fs
{
	thread_local error g_tls_error = error::ok;

	class device_manager final
	{
		mutable shared_mutex m_mutex;

		std::unordered_map<std::string, std::shared_ptr<device_base>> m_map;

	public:
		std::shared_ptr<device_base> get_device(const std::string& path);
		std::shared_ptr<device_base> set_device(const std::string& name, const std::shared_ptr<device_base>&);
	};

	static device_manager& get_device_manager()
	{
		// Use magic static
		static device_manager instance;
		return instance;
	}
}

std::shared_ptr<fs::device_base> fs::device_manager::get_device(const std::string& path)
{
	reader_lock lock(m_mutex);

	const auto found = m_map.find(path.substr(0, path.find_first_of('/', 2)));

	if (found == m_map.end())
	{
		return nullptr;
	}

	return found->second;
}

std::shared_ptr<fs::device_base> fs::device_manager::set_device(const std::string& name, const std::shared_ptr<device_base>& device)
{
	writer_lock lock(m_mutex);

	return m_map[name] = device;
}

std::shared_ptr<fs::device_base> fs::get_virtual_device(const std::string& path)
{
	// Every virtual device path must have "//" at the beginning
	if (path.size() > 2 && reinterpret_cast<const u16&>(path.front()) == "//"_u16)
	{
		return get_device_manager().get_device(path);
	}

	return nullptr;
}

std::shared_ptr<fs::device_base> fs::set_virtual_device(const std::string& name, const std::shared_ptr<device_base>& device)
{
	EXPECTS(name.size() > 2 && name[0] == '/' && name[1] == '/' && name.find('/', 2) == -1);

	return get_device_manager().set_device(name, device);
}

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
	VERIFY(fs::get_parent_dir("/x/y///") == "/x");
	VERIFY(fs::get_parent_dir("/x/y/") == "/x");
	VERIFY(fs::get_parent_dir("/x/y") == "/x");
	VERIFY(fs::get_parent_dir("x:/y") == "x:");
	VERIFY(fs::get_parent_dir("//x/y") == "//x");

	// Failure:
	VERIFY(fs::get_parent_dir("").empty());
	VERIFY(fs::get_parent_dir("x/").empty());
	VERIFY(fs::get_parent_dir("x").empty());
	VERIFY(fs::get_parent_dir("x///").empty());
	VERIFY(fs::get_parent_dir("/x/").empty());
	VERIFY(fs::get_parent_dir("/x").empty());
	VERIFY(fs::get_parent_dir("/").empty());
	VERIFY(fs::get_parent_dir("//").empty());
	VERIFY(fs::get_parent_dir("//x").empty());
	VERIFY(fs::get_parent_dir("//x/").empty());
	VERIFY(fs::get_parent_dir("///").empty());
	VERIFY(fs::get_parent_dir("///x").empty());
	VERIFY(fs::get_parent_dir("///x/").empty());

	return false;
}();

bool fs::stat(const std::string& path, stat_t& info)
{
	if (auto device = get_virtual_device(path))
	{
		return device->stat(path, info);
	}

#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA attrs;
	if (!GetFileAttributesExW(to_wchar(path).get(), GetFileExInfoStandard, &attrs))
	{
		g_tls_error = to_error(GetLastError());
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
		g_tls_error = to_error(errno);
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
	if (auto device = get_virtual_device(path))
	{
		stat_t info;
		return device->stat(path, info);
	}

#ifdef _WIN32
	if (GetFileAttributesW(to_wchar(path).get()) == INVALID_FILE_ATTRIBUTES)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	return true;
#else
	struct ::stat file_info;
	if (::stat(path.c_str(), &file_info) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
#endif
}

bool fs::is_file(const std::string& path)
{
	if (auto device = get_virtual_device(path))
	{
		stat_t info;
		if (!device->stat(path, info))
		{
			return false;
		}

		if (info.is_directory)
		{
			g_tls_error = error::exist;
			return false;
		}

		return true;
	}

#ifdef _WIN32
	const DWORD attrs = GetFileAttributesW(to_wchar(path).get());
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}
#else
	struct ::stat file_info;
	if (::stat(path.c_str(), &file_info) != 0)
	{
		g_tls_error = to_error(errno);
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
		g_tls_error = error::exist;
		return false;
	}

	return true;
}

bool fs::is_dir(const std::string& path)
{
	if (auto device = get_virtual_device(path))
	{
		stat_t info;
		if (!device->stat(path, info))
		{
			return false;
		}

		if (info.is_directory == false)
		{
			g_tls_error = error::exist;
			return false;
		}

		return true;
	}

#ifdef _WIN32
	const DWORD attrs = GetFileAttributesW(to_wchar(path).get());
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}
#else
	struct ::stat file_info;
	if (::stat(path.c_str(), &file_info) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}
#endif

#ifdef _WIN32
	if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0)
#else
	if (!S_ISDIR(file_info.st_mode))
#endif
	{
		g_tls_error = error::exist;
		return false;
	}

	return true;
}

bool fs::create_dir(const std::string& path)
{
	if (auto device = get_virtual_device(path))
	{
		return device->create_dir(path);
	}

#ifdef _WIN32
	if (!CreateDirectoryW(to_wchar(path).get(), NULL))
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	return true;
#else
	if (::mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
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
	if (auto device = get_virtual_device(path))
	{
		return device->remove_dir(path);
	}

#ifdef _WIN32
	if (!RemoveDirectoryW(to_wchar(path).get()))
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	return true;
#else
	if (::rmdir(path.c_str()) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
#endif
}

bool fs::rename(const std::string& from, const std::string& to)
{
	const auto device = get_virtual_device(from);

	if (device != get_virtual_device(to))
	{
		throw fmt::exception("fs::rename() between different devices not implemented.\nFrom: %s\nTo: %s", from, to);
	}

	if (device)
	{
		return device->rename(from, to);
	}

#ifdef _WIN32
	if (!MoveFileW(to_wchar(from).get(), to_wchar(to).get()))
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	return true;
#else
	if (::rename(from.c_str(), to.c_str()) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
#endif
}

bool fs::copy_file(const std::string& from, const std::string& to, bool overwrite)
{
	const auto device = get_virtual_device(from);

	if (device != get_virtual_device(to) || device) // TODO
	{
		throw fmt::exception("fs::copy_file() for virtual devices not implemented.\nFrom: %s\nTo: %s", from, to);
	}

#ifdef _WIN32
	if (!CopyFileW(to_wchar(from).get(), to_wchar(to).get(), !overwrite))
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	return true;
#else
	/* Source: http://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c */

	const int input = ::open(from.c_str(), O_RDONLY);
	if (input == -1)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	const int output = ::open(to.c_str(), O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (output == -1)
	{
		const int err = errno;

		::close(input);
		g_tls_error = to_error(err);
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
		g_tls_error = to_error(err);
		return false;
	}

	::close(input);
	::close(output);
	return true;
#endif
}

bool fs::remove_file(const std::string& path)
{
	if (auto device = get_virtual_device(path))
	{
		return device->remove(path);
	}

#ifdef _WIN32
	if (!DeleteFileW(to_wchar(path).get()))
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	return true;
#else
	if (::unlink(path.c_str()) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
#endif
}

bool fs::truncate_file(const std::string& path, u64 length)
{
	if (auto device = get_virtual_device(path))
	{
		return device->trunc(path, length);
	}

#ifdef _WIN32
	// Open the file
	const auto handle = CreateFileW(to_wchar(path).get(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	LARGE_INTEGER distance;
	distance.QuadPart = length;

	// Seek and truncate
	if (!SetFilePointerEx(handle, distance, NULL, FILE_BEGIN) || !SetEndOfFile(handle))
	{
		g_tls_error = to_error(GetLastError());
		CloseHandle(handle);
		return false;
	}

	CloseHandle(handle);
	return true;
#else
	if (::truncate(path.c_str(), length) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
#endif
}

void fs::file::xnull() const
{
	throw std::logic_error("fs::file is null");
}

void fs::file::xfail() const
{
	throw fmt::exception("Unexpected fs::error %u", g_tls_error);
}

bool fs::file::open(const std::string& path, bitset_t<open_mode> mode)
{
	if (auto device = get_virtual_device(path))
	{
		if (auto&& _file = device->open(path, mode))
		{
			m_file = std::move(_file);
			return true;
		}

		return false;
	}

#ifdef _WIN32
	DWORD access = 0;
	if (mode & fs::read) access |= GENERIC_READ;
	if (mode & fs::write) access |= mode & fs::append ? FILE_APPEND_DATA : GENERIC_WRITE;

	DWORD disp = 0;
	if (mode & fs::create)
	{
		disp =
			mode & fs::excl ? CREATE_NEW :
			mode & fs::trunc ? CREATE_ALWAYS : OPEN_ALWAYS;
	}
	else
	{
		if (mode & fs::excl)
		{
			g_tls_error = error::inval;
			return false;
		}

		disp = mode & fs::trunc ? TRUNCATE_EXISTING : OPEN_EXISTING;
	}

	const HANDLE handle = CreateFileW(to_wchar(path).get(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	class windows_file final : public file_base
	{
		const HANDLE m_handle;

	public:
		windows_file(HANDLE handle)
			: m_handle(handle)
		{
		}

		~windows_file() override
		{
			CloseHandle(m_handle);
		}

		stat_t stat() override
		{
			FILE_BASIC_INFO basic_info;
			if (!GetFileInformationByHandleEx(m_handle, FileBasicInfo, &basic_info, sizeof(FILE_BASIC_INFO)))
			{
				throw fmt::exception("Win32 error: %u." HERE, GetLastError());
			}

			stat_t info;
			info.is_directory = (basic_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			info.is_writable = (basic_info.FileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
			info.size = this->size();
			info.atime = to_time(basic_info.LastAccessTime);
			info.mtime = to_time(basic_info.ChangeTime);
			info.ctime = to_time(basic_info.CreationTime);

			return info;
		}

		bool trunc(u64 length) override
		{
			LARGE_INTEGER old, pos;

			pos.QuadPart = 0;
			if (!SetFilePointerEx(m_handle, pos, &old, FILE_CURRENT)) // get old position
			{
				g_tls_error = to_error(GetLastError());
				return false;
			}

			pos.QuadPart = length;
			if (!SetFilePointerEx(m_handle, pos, NULL, FILE_BEGIN)) // set new position
			{
				g_tls_error = to_error(GetLastError());
				return false;
			}

			const BOOL result = SetEndOfFile(m_handle); // change file size

			if (!result || !SetFilePointerEx(m_handle, old, NULL, FILE_BEGIN)) // restore position
			{
				throw fmt::exception("Win32 error: %u." HERE, GetLastError());
			}

			return result != FALSE;
		}

		u64 read(void* buffer, u64 count) override
		{
			// TODO (call ReadFile multiple times if count is too big)
			const int size = ::narrow<int>(count, "Too big count" HERE);
			EXPECTS(size >= 0);

			DWORD nread;
			if (!ReadFile(m_handle, buffer, size, &nread, NULL))
			{
				throw fmt::exception("Win32 error: %u." HERE, GetLastError());
			}

			return nread;
		}

		u64 write(const void* buffer, u64 count) override
		{
			// TODO (call WriteFile multiple times if count is too big)
			const int size = ::narrow<int>(count, "Too big count" HERE);
			EXPECTS(size >= 0);

			DWORD nwritten;
			if (!WriteFile(m_handle, buffer, size, &nwritten, NULL))
			{
				throw fmt::exception("Win32 error: %u." HERE, GetLastError());
			}

			return nwritten;
		}

		u64 seek(s64 offset, seek_mode whence) override
		{
			LARGE_INTEGER pos;
			pos.QuadPart = offset;

			const DWORD mode =
				whence == seek_set ? FILE_BEGIN :
				whence == seek_cur ? FILE_CURRENT :
				whence == seek_end ? FILE_END :
				throw fmt::exception("Invalid whence (0x%x)" HERE, whence);

			if (!SetFilePointerEx(m_handle, pos, &pos, mode))
			{
				throw fmt::exception("Win32 error: %u." HERE, GetLastError());
			}

			return pos.QuadPart;
		}

		u64 size() override
		{
			LARGE_INTEGER size;
			if (!GetFileSizeEx(m_handle, &size))
			{
				throw fmt::exception("Win32 error: %u." HERE, GetLastError());
			}

			return size.QuadPart;
		}
	};

	m_file = std::make_unique<windows_file>(handle);
#else
	int flags = 0;

	if (mode & fs::read && mode & fs::write) flags |= O_RDWR;
	else if (mode & fs::read) flags |= O_RDONLY;
	else if (mode & fs::write) flags |= O_WRONLY;

	if (mode & fs::append) flags |= O_APPEND;
	if (mode & fs::create) flags |= O_CREAT;
	if (mode & fs::trunc) flags |= O_TRUNC;
	if (mode & fs::excl) flags |= O_EXCL;

	const int fd = ::open(path.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (fd == -1)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	class unix_file final : public file_base
	{
		const int m_fd;

	public:
		unix_file(int fd)
			: m_fd(fd)
		{
		}

		~unix_file() override
		{
			::close(m_fd);
		}

		stat_t stat() override
		{
			struct ::stat file_info;
			if (::fstat(m_fd, &file_info) != 0)
			{
				throw fmt::exception("System error: %d." HERE, errno);
			}

			stat_t info;
			info.is_directory = S_ISDIR(file_info.st_mode);
			info.is_writable = file_info.st_mode & 0200; // HACK: approximation
			info.size = file_info.st_size;
			info.atime = file_info.st_atime;
			info.mtime = file_info.st_mtime;
			info.ctime = file_info.st_ctime;

			return info;
		}

		bool trunc(u64 length) override
		{
			if (::ftruncate(m_fd, length) != 0)
			{
				g_tls_error = to_error(errno);
				return false;
			}

			return true;
		}

		u64 read(void* buffer, u64 count) override
		{
			const auto result = ::read(m_fd, buffer, count);
			if (result == -1)
			{
				throw fmt::exception("System error: %d." HERE, errno);
			}

			return result;
		}

		u64 write(const void* buffer, u64 count) override
		{
			const auto result = ::write(m_fd, buffer, count);
			if (result == -1)
			{
				throw fmt::exception("System error: %d." HERE, errno);
			}

			return result;
		}

		u64 seek(s64 offset, seek_mode whence) override
		{
			const int mode =
				whence == seek_set ? SEEK_SET :
				whence == seek_cur ? SEEK_CUR :
				whence == seek_end ? SEEK_END :
				throw fmt::exception("Invalid whence (0x%x)" HERE, whence);

			const auto result = ::lseek(m_fd, offset, mode);
			if (result == -1)
			{
				throw fmt::exception("System error: %d." HERE, errno);
			}

			return result;
		}

		u64 size() override
		{
			struct ::stat file_info;
			if (::fstat(m_fd, &file_info) != 0)
			{
				throw fmt::exception("System error: %d." HERE, errno);
			}

			return file_info.st_size;
		}
	};

	m_file = std::make_unique<unix_file>(fd);
#endif

	return true;
}

fs::file::file(const void* ptr, std::size_t size)
{
	class memory_stream : public file_base
	{
		u64 m_pos{};

		const char* const m_ptr;
		const u64 m_size;

	public:
		memory_stream(const void* ptr, u64 size)
			: m_ptr(static_cast<const char*>(ptr))
			, m_size(size)
		{
		}

		fs::stat_t stat() override
		{
			throw std::logic_error("Not supported" HERE);
		}

		bool trunc(u64 length) override
		{
			throw std::logic_error("Not allowed" HERE);
		}

		u64 read(void* buffer, u64 count) override
		{
			const u64 start = m_pos;
			const u64 end = seek(count, fs::seek_cur);
			const u64 read_size = end >= start ? end - start : throw std::logic_error("Stream overflow" HERE);
			std::memcpy(buffer, m_ptr + start, read_size);
			return read_size;
		}

		u64 write(const void* buffer, u64 count) override
		{
			throw std::logic_error("Not allowed" HERE);
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			return
				whence == fs::seek_set ? m_pos = std::min<u64>(offset, m_size) :
				whence == fs::seek_cur ? m_pos = std::min<u64>(offset + m_pos, m_size) :
				whence == fs::seek_end ? m_pos = std::min<u64>(offset + m_size, m_size) :
				throw fmt::exception("Invalid whence (0x%x)" HERE, whence);
		}

		u64 size() override
		{
			return m_size;
		}
	};

	m_file = std::make_unique<memory_stream>(ptr, size);
}

void fs::dir::xnull() const
{
	throw std::logic_error("fs::dir is null");
}

bool fs::dir::open(const std::string& path)
{
	if (auto device = get_virtual_device(path))
	{
		if (auto&& _dir = device->open_dir(path))
		{
			m_dir = std::move(_dir);
			return true;
		}

		return false;
	}

#ifdef _WIN32
	WIN32_FIND_DATAW found;
	const auto handle = FindFirstFileW(to_wchar(path + "/*").get(), &found);

	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	class windows_dir final : public dir_base
	{
		const HANDLE m_handle;

		std::vector<dir_entry> m_entries;
		std::size_t m_pos = 0;

		void add_entry(const WIN32_FIND_DATAW& found)
		{
			dir_entry info;

			to_utf8(info.name, found.cFileName);
			info.is_directory = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			info.is_writable = (found.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
			info.size = ((u64)found.nFileSizeHigh << 32) | (u64)found.nFileSizeLow;
			info.atime = to_time(found.ftLastAccessTime);
			info.mtime = to_time(found.ftLastWriteTime);
			info.ctime = to_time(found.ftCreationTime);

			m_entries.emplace_back(std::move(info));
		}

	public:
		windows_dir(HANDLE handle, const WIN32_FIND_DATAW& found)
			: m_handle(handle)
		{
			add_entry(found);
		}

		~windows_dir()
		{
			FindClose(m_handle);
		}

		bool read(dir_entry& out) override
		{
			if (m_pos == m_entries.size())
			{
				WIN32_FIND_DATAW found;
				if (!FindNextFileW(m_handle, &found))
				{
					if (ERROR_NO_MORE_FILES == GetLastError())
					{
						return false;
					}

					throw fmt::exception("Win32 error: %u." HERE, GetLastError());
				}

				add_entry(found);
			}

			out = m_entries[m_pos++];
			return true;
		}

		void rewind() override
		{
			m_pos = 0;
		}
	};

	m_dir = std::make_unique<windows_dir>(handle, found);
#else
	::DIR* const ptr = ::opendir(path.c_str());

	if (!ptr)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	class unix_dir final : public dir_base
	{
		::DIR* m_dd;

	public:
		unix_dir(::DIR* dd)
			: m_dd(dd)
		{
		}

		~unix_dir() override
		{
			::closedir(m_dd);
		}

		bool read(dir_entry& info) override
		{
			const auto found = ::readdir(m_dd);
			if (!found)
			{
				return false;
			}

			struct ::stat file_info;
			if (::fstatat(::dirfd(m_dd), found->d_name, &file_info, 0) != 0)
			{
				throw fmt::exception("System error: %d." HERE, errno);
			}

			info.name = found->d_name;
			info.is_directory = S_ISDIR(file_info.st_mode);
			info.is_writable = file_info.st_mode & 0200; // HACK: approximation
			info.size = file_info.st_size;
			info.atime = file_info.st_atime;
			info.mtime = file_info.st_mtime;
			info.ctime = file_info.st_ctime;

			return true;
		}

		void rewind() override
		{
			::rewinddir(m_dd);
		}
	};

	m_dir = std::make_unique<unix_dir>(ptr);
#endif

	return true;
}

const std::string& fs::get_config_dir()
{
	// Use magic static
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
	// Use magic static
	static const std::string s_dir = []
	{
		std::string dir;

#ifdef _WIN32
		wchar_t buf[2048];
		if (GetModuleFileName(NULL, buf, ::size32(buf)) - 1 >= ::size32(buf) - 1)
		{
			MessageBoxA(0, fmt::format("GetModuleFileName() failed: error %u.", GetLastError()).c_str(), "fs::get_executable_dir()", MB_ICONERROR);
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

void fs::remove_all(const std::string& path, bool remove_root)
{
	for (const auto& entry : dir(path))
	{
		if (entry.name == "." || entry.name == "..")
		{
			continue;
		}

		if (entry.is_directory == false)
		{
			remove_file(path + '/' + entry.name);
		}

		if (entry.is_directory == true)
		{
			remove_all(path + '/' + entry.name);
		}
	}

	if (remove_root)
	{
		remove_dir(path);
	}
}

u64 fs::get_dir_size(const std::string& path)
{
	u64 result = 0;

	for (const auto entry : dir(path))
	{
		if (entry.name == "." || entry.name == "..")
		{
			continue;
		}

		if (entry.is_directory == false)
		{
			result += entry.size;
		}

		if (entry.is_directory == true)
		{
			result += get_dir_size(path + '/' + entry.name);
		}
	}

	return result;
}
