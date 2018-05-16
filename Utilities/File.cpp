#include "File.h"
#include "mutex.h"
#include "StrFmt.h"
#include "BEType.h"
#include "Crypto/sha1.h"

#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <typeinfo>

using namespace std::literals::string_literals;

#ifdef _WIN32

#include <cwchar>
#include <Windows.h>

static std::unique_ptr<wchar_t[]> to_wchar(const std::string& source)
{
	// String size + null terminator
	const std::size_t buf_size = source.size() + 1;

	// Safe size
	const int size = narrow<int>(buf_size, "to_wchar" HERE);

	// Buffer for max possible output length
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[buf_size]);

	verify("to_wchar" HERE), MultiByteToWideChar(CP_UTF8, 0, source.c_str(), size, buffer.get(), size);

	return buffer;
}

static void to_utf8(std::string& out, const wchar_t* source)
{
	// String size
	const std::size_t length = std::wcslen(source);

	// Safe buffer size for max possible output length (including null terminator)
	const int buf_size = narrow<int>(length * 3 + 1, "to_utf8" HERE);

	// Resize buffer
	out.resize(buf_size - 1);

	const int result = WideCharToMultiByte(CP_UTF8, 0, source, static_cast<int>(length) + 1, &out.front(), buf_size, NULL, NULL);

	// Fix the size
	out.resize(verify("to_utf8" HERE, result) - 1);
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

static FILETIME from_time(s64 _time)
{
	FILETIME result;

	if (_time <= -11644473600ll)
	{
		result.dwLowDateTime = 0;
		result.dwHighDateTime = 0;
	}
	else if (_time > INT64_MAX / 10000000ll - 11644473600ll)
	{
		result.dwLowDateTime = 0xffffffff;
		result.dwHighDateTime = 0x7fffffff;
	}
	else
	{
		const ullong wtime = (_time + 11644473600ull) * 10000000ull;
		result.dwLowDateTime = static_cast<DWORD>(wtime);
		result.dwHighDateTime = static_cast<DWORD>(wtime >> 32);
	}

	return result;
}

static fs::error to_error(DWORD e)
{
	switch (e)
	{
	case ERROR_FILE_NOT_FOUND: return fs::error::noent;
	case ERROR_PATH_NOT_FOUND: return fs::error::noent;
	case ERROR_ACCESS_DENIED: return fs::error::acces;
	case ERROR_ALREADY_EXISTS: return fs::error::exist;
	case ERROR_FILE_EXISTS: return fs::error::exist;
	case ERROR_NEGATIVE_SEEK: return fs::error::inval;
	case ERROR_DIRECTORY: return fs::error::inval;
	case ERROR_INVALID_NAME: return fs::error::inval;
	case ERROR_SHARING_VIOLATION: return fs::error::acces;
	case ERROR_DIR_NOT_EMPTY: return fs::error::notempty;
	case ERROR_NOT_READY: return fs::error::noent;
	//case ERROR_INVALID_PARAMETER: return fs::error::inval;
	default: fmt::throw_exception("Unknown Win32 error: %u.", e);
	}
}

#else

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/file.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>

#if defined(__APPLE__)
#include <copyfile.h>
#include <mach-o/dyld.h>
#elif defined(__DragonFly__) || defined(__FreeBSD__)
#include <sys/socket.h> // sendfile
#elif defined(__linux__) || defined(__sun)
#include <sys/sendfile.h>
#endif

static fs::error to_error(int e)
{
	switch (e)
	{
	case ENOENT: return fs::error::noent;
	case EEXIST: return fs::error::exist;
	case EINVAL: return fs::error::inval;
	case EACCES: return fs::error::acces;
	case ENOTEMPTY: return fs::error::notempty;
	default: fmt::throw_exception("Unknown system error: %d.", e);
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

	file_base::~file_base()
	{
	}

	stat_t file_base::stat()
	{
		fmt::throw_exception("fs::file::stat() not supported for %s", typeid(*this).name());
	}

	void file_base::sync()
	{
		// Do notning
	}

	dir_base::~dir_base()
	{
	}

	device_base::~device_base()
	{
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
	verify(HERE), name.size() > 2, name[0] == '/', name[1] == '/', name.find('/', 2) == -1;

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

bool fs::statfs(const std::string& path, fs::device_stat& info)
{
	if (auto device = get_virtual_device(path))
	{
		return device->statfs(path, info);
	}

#ifdef _WIN32
	ULARGE_INTEGER avail_free;
	ULARGE_INTEGER total_size;
	ULARGE_INTEGER total_free;

	if (!GetDiskFreeSpaceExW(to_wchar(path).get(), &avail_free, &total_size, &total_free))
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	info.block_size = 4096; // TODO
	info.total_size = total_size.QuadPart;
	info.total_free = total_free.QuadPart;
	info.avail_free = avail_free.QuadPart;
#else
	struct ::statvfs buf;
	if (::statvfs(path.c_str(), &buf) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	info.block_size = buf.f_frsize;
	info.total_size = info.block_size * buf.f_blocks;
	info.total_free = info.block_size * buf.f_bfree;
	info.avail_free = info.block_size * buf.f_bavail;
#endif

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
	const std::string parent = get_parent_dir(path);

	if (!parent.empty() && !create_path(parent))
	{
		return false;
	}

	if (!create_dir(path) && g_tls_error != error::exist)
	{
		return false;
	}

	return true;
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

bool fs::rename(const std::string& from, const std::string& to, bool overwrite)
{
	const auto device = get_virtual_device(from);

	if (device != get_virtual_device(to))
	{
		fmt::throw_exception("fs::rename() between different devices not implemented.\nFrom: %s\nTo: %s", from, to);
	}

	if (device)
	{
		return device->rename(from, to);
	}

#ifdef _WIN32
	const auto ws1 = to_wchar(from);
	const auto ws2 = to_wchar(to);

	if (!MoveFileExW(ws1.get(), ws2.get(), overwrite ? MOVEFILE_REPLACE_EXISTING : 0))
	{
		DWORD error1 = GetLastError();

		if (overwrite && error1 == ERROR_ACCESS_DENIED && is_dir(from) && is_dir(to))
		{
			if (RemoveDirectoryW(ws2.get()))
			{
				if (MoveFileW(ws1.get(), ws2.get()))
				{
					return true;
				}

				error1 = GetLastError();
				CreateDirectoryW(ws2.get(), NULL); // TODO
			}
			else
			{
				error1 = GetLastError();
			}
		}

		g_tls_error = to_error(error1);
		return false;
	}

	return true;
#else
	if (!overwrite && exists(to))
	{
		g_tls_error = fs::error::exist;
		return false;
	}

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
		fmt::throw_exception("fs::copy_file() for virtual devices not implemented.\nFrom: %s\nTo: %s", from, to);
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
#if defined(__APPLE__)
	// fcopyfile works on OS X 10.5+
	if (::fcopyfile(input, output, 0, COPYFILE_ALL))
#elif defined(__DragonFly__) || defined(__FreeBSD__)
	if (::sendfile(input, output, 0, 0, NULL, NULL, 0))
#elif defined(__linux__) || defined(__sun)
	// sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
	off_t bytes_copied = 0;
	struct ::stat fileinfo = { 0 };
	if (::fstat(input, &fileinfo) || ::sendfile(output, input, &bytes_copied, fileinfo.st_size))
#else // NetBSD, OpenBSD, etc.
	fmt::throw_exception("fs::copy_file() isn't implemented for this platform.\nFrom: %s\nTo: %s", from, to);
	errno = 0;
	if (true)
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
	const auto handle = CreateFileW(to_wchar(path).get(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	FILE_END_OF_FILE_INFO _eof;
	_eof.EndOfFile.QuadPart = length;

	if (!SetFileInformationByHandle(handle, FileEndOfFileInfo, &_eof, sizeof(_eof)))
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

bool fs::utime(const std::string& path, s64 atime, s64 mtime)
{
	if (auto device = get_virtual_device(path))
	{
		return device->utime(path, atime, mtime);
	}

#ifdef _WIN32
	// Open the file
	const auto handle = CreateFileW(to_wchar(path).get(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	FILETIME _atime = from_time(atime);
	FILETIME _mtime = from_time(mtime);
	if (!SetFileTime(handle, nullptr, &_atime, &_mtime))
	{
		g_tls_error = to_error(GetLastError());
		CloseHandle(handle);
		return false;
	}

	CloseHandle(handle);
	return true;
#else
	::utimbuf buf;
	buf.actime = atime;
	buf.modtime = mtime;

	if (::utime(path.c_str(), &buf) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
#endif
}

void fs::file::xnull() const
{
	fmt::throw_exception<std::logic_error>("fs::file is null");
}

void fs::file::xfail() const
{
	fmt::throw_exception("Unexpected fs::error %s", g_tls_error);
}

fs::file::file(const std::string& path, bs_t<open_mode> mode)
{
	if (auto device = get_virtual_device(path))
	{
		if (auto&& _file = device->open(path, mode))
		{
			m_file = std::move(_file);
			return;
		}

		return;
	}

#ifdef _WIN32
	DWORD access = 0;
	if (test(mode & fs::read)) access |= GENERIC_READ;
	if (test(mode & fs::write)) access |= DELETE | (test(mode & fs::append) ? FILE_APPEND_DATA : GENERIC_WRITE);

	DWORD disp = 0;
	if (test(mode & fs::create))
	{
		disp =
			test(mode & fs::excl) ? CREATE_NEW :
			test(mode & fs::trunc) ? CREATE_ALWAYS : OPEN_ALWAYS;
	}
	else
	{
		if (test(mode & fs::excl))
		{
			g_tls_error = error::inval;
			return;
		}

		disp = test(mode & fs::trunc) ? TRUNCATE_EXISTING : OPEN_EXISTING;
	}

	DWORD share = 0;
	if (!test(mode, fs::unread) || !test(mode & fs::write))
	{
		share |= FILE_SHARE_READ;
	}

	if (!test(mode, fs::lock + fs::unread) || !test(mode & fs::write))
	{
		share |= FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	}

	const HANDLE handle = CreateFileW(to_wchar(path).get(), access, share, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return;
	}

	class windows_file final : public file_base, public get_native_handle
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
			verify("file::stat" HERE), GetFileInformationByHandleEx(m_handle, FileBasicInfo, &basic_info, sizeof(FILE_BASIC_INFO));

			stat_t info;
			info.is_directory = (basic_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			info.is_writable = (basic_info.FileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
			info.size = this->size();
			info.atime = to_time(basic_info.LastAccessTime);
			info.mtime = to_time(basic_info.ChangeTime);
			info.ctime = to_time(basic_info.CreationTime);

			return info;
		}

		void sync() override
		{
			verify("file::sync" HERE), FlushFileBuffers(m_handle);
		}

		bool trunc(u64 length) override
		{
			FILE_END_OF_FILE_INFO _eof;
			_eof.EndOfFile.QuadPart = length;

			if (!SetFileInformationByHandle(m_handle, FileEndOfFileInfo, &_eof, sizeof(_eof)))
			{
				g_tls_error = to_error(GetLastError());
				return false;
			}

			return true;
		}

		u64 read(void* buffer, u64 count) override
		{
			// TODO (call ReadFile multiple times if count is too big)
			const int size = narrow<int>(count, "file::read" HERE);

			DWORD nread;
			verify("file::read" HERE), ReadFile(m_handle, buffer, size, &nread, NULL);

			return nread;
		}

		u64 write(const void* buffer, u64 count) override
		{
			// TODO (call WriteFile multiple times if count is too big)
			const int size = narrow<int>(count, "file::write" HERE);

			DWORD nwritten;
			verify("file::write" HERE), WriteFile(m_handle, buffer, size, &nwritten, NULL);

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
				(fmt::throw_exception("Invalid whence (0x%x)" HERE, whence), 0);

			if (!SetFilePointerEx(m_handle, pos, &pos, mode))
			{
				g_tls_error = to_error(GetLastError());
				return -1;
			}

			return pos.QuadPart;
		}

		u64 size() override
		{
			LARGE_INTEGER size;
			verify("file::size" HERE), GetFileSizeEx(m_handle, &size);

			return size.QuadPart;
		}

		native_handle get() override
		{
			return m_handle;
		}
	};

	m_file = std::make_unique<windows_file>(handle);
#else
	int flags = 0;

	if (test(mode & fs::read) && test(mode & fs::write)) flags |= O_RDWR;
	else if (test(mode & fs::read)) flags |= O_RDONLY;
	else if (test(mode & fs::write)) flags |= O_WRONLY;

	if (test(mode & fs::append)) flags |= O_APPEND;
	if (test(mode & fs::create)) flags |= O_CREAT;
	if (test(mode & fs::trunc) && !test(mode, fs::lock + fs::unread)) flags |= O_TRUNC;
	if (test(mode & fs::excl)) flags |= O_EXCL;

	int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if (test(mode & fs::write) && test(mode & fs::unread))
	{
		perm = 0;
	}

	const int fd = ::open(path.c_str(), flags, perm);

	if (fd == -1)
	{
		g_tls_error = to_error(errno);
		return;
	}

	if (test(mode & fs::write) && test(mode, fs::lock + fs::unread) && ::flock(fd, LOCK_EX | LOCK_NB) != 0)
	{
		g_tls_error = errno == EWOULDBLOCK ? fs::error::acces : to_error(errno);
		::close(fd);
		return;
	}

	if (test(mode & fs::trunc) && test(mode, fs::lock + fs::unread))
	{
		// Postpone truncation in order to avoid using O_TRUNC on a locked file
		::ftruncate(fd, 0);
	}

	class unix_file final : public file_base, public get_native_handle
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
			verify("file::stat" HERE), ::fstat(m_fd, &file_info) == 0;

			stat_t info;
			info.is_directory = S_ISDIR(file_info.st_mode);
			info.is_writable = file_info.st_mode & 0200; // HACK: approximation
			info.size = file_info.st_size;
			info.atime = file_info.st_atime;
			info.mtime = file_info.st_mtime;
			info.ctime = file_info.st_ctime;

			return info;
		}

		void sync() override
		{
			verify("file::sync" HERE), ::fsync(m_fd) == 0;
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
			verify("file::read" HERE), result != -1;

			return result;
		}

		u64 write(const void* buffer, u64 count) override
		{
			const auto result = ::write(m_fd, buffer, count);
			verify("file::write" HERE), result != -1;

			return result;
		}

		u64 seek(s64 offset, seek_mode whence) override
		{
			const int mode =
				whence == seek_set ? SEEK_SET :
				whence == seek_cur ? SEEK_CUR :
				whence == seek_end ? SEEK_END :
				(fmt::throw_exception("Invalid whence (0x%x)" HERE, whence), 0);

			const auto result = ::lseek(m_fd, offset, mode);

			if (result == -1)
			{
				g_tls_error = to_error(errno);
				return -1;
			}

			return result;
		}

		u64 size() override
		{
			struct ::stat file_info;
			verify("file::size" HERE), ::fstat(m_fd, &file_info) == 0;

			return file_info.st_size;
		}

		native_handle get() override
		{
			return m_fd;
		}
	};

	m_file = std::make_unique<unix_file>(fd);
#endif
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

		bool trunc(u64 length) override
		{
			return false;
		}

		u64 read(void* buffer, u64 count) override
		{
			if (m_pos < m_size)
			{
				// Get readable size
				if (const u64 result = std::min<u64>(count, m_size - m_pos))
				{
					std::memcpy(buffer, m_ptr + m_pos, result);
					m_pos += result;
					return result;
				}
			}

			return 0;
		}

		u64 write(const void* buffer, u64 count) override
		{
			return 0;
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			const s64 new_pos =
				whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? offset + m_pos :
				whence == fs::seek_end ? offset + size() :
				(fmt::raw_error("fs::file::memory_stream::seek(): invalid whence"), 0);

			if (new_pos < 0)
			{
				fs::g_tls_error = fs::error::inval;
				return -1;
			}

			m_pos = new_pos;
			return m_pos;
		}

		u64 size() override
		{
			return m_size;
		}
	};

	m_file = std::make_unique<memory_stream>(ptr, size);
}

fs::native_handle fs::file::get_handle() const
{
	if (auto getter = dynamic_cast<get_native_handle*>(m_file.get()))
	{
		return getter->get();
	}

#ifdef _WIN32
	return INVALID_HANDLE_VALUE;
#else
	return -1;
#endif
}

void fs::dir::xnull() const
{
	fmt::throw_exception<std::logic_error>("fs::dir is null");
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
	const auto handle = FindFirstFileExW(to_wchar(path + "/*").get(), FindExInfoBasic, &found, FindExSearchNameMatch, NULL, FIND_FIRST_EX_CASE_SENSITIVE | FIND_FIRST_EX_LARGE_FETCH);

	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	class windows_dir final : public dir_base
	{
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
		windows_dir(HANDLE handle, WIN32_FIND_DATAW& found)
		{
			add_entry(found);

			while (FindNextFileW(handle, &found))
			{
				add_entry(found);
			}

			verify("dir::read" HERE), ERROR_NO_MORE_FILES == GetLastError();
			FindClose(handle);
		}

		bool read(dir_entry& out) override
		{
			if (m_pos >= m_entries.size())
			{
				return false;
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
				//failed metadata (broken symlink?), ignore and skip to next file
				return read(info);
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
		std::string dir;

#ifdef _WIN32
		wchar_t buf[2048];
		if (GetModuleFileName(NULL, buf, ::size32(buf)) - 1 >= ::size32(buf) - 1)
		{
			MessageBoxA(0, fmt::format("GetModuleFileName() failed: error %u.", GetLastError()).c_str(), "fs::get_config_dir()", MB_ICONERROR);
			return dir; // empty
		}

		to_utf8(dir, buf); // Convert to UTF-8

		std::replace(dir.begin(), dir.end(), '\\', '/');

		dir.resize(dir.rfind('/') + 1);
#else
		if (const char* home = ::getenv("XDG_CONFIG_HOME"))
			dir = home;
		else if (const char* home = ::getenv("HOME"))
			dir = home + "/.config"s;
		else // Just in case
			dir = "./config";

		dir += "/rpcs3/";

		if (!create_path(dir))
		{
			std::printf("Failed to create configuration directory '%s' (%d).\n", dir.c_str(), errno);
		}
#endif

		return dir;
	}();

	return s_dir;
}

std::string fs::get_data_dir(const std::string& prefix, const std::string& location, const std::string& suffix)
{
	static const std::string s_dir = []
	{
		const std::string dir = get_config_dir() + "data/";

		if (!create_path(dir))
		{
			return get_config_dir();
		}

		return dir;
	}();

	std::vector<u8> buf;
	buf.reserve(location.size() + 1);

	// Normalize location
	for (char c : location)
	{
#ifdef _WIN32
		if (c == '/' || c == '\\')
#else
		if (c == '/')
#endif
		{
			if (buf.empty() || buf.back() != '/')
			{
				buf.push_back('/');
			}

			continue;
		}

		buf.push_back(c);
	}

	// Calculate hash
	u8 hash[20];
	sha1(buf.data(), buf.size(), hash);

	// Concatenate
	std::string result = fmt::format("%s%s/%016llx%08x-%s/", s_dir, prefix, reinterpret_cast<be_t<u64>&>(hash[0]), reinterpret_cast<be_t<u32>&>(hash[8]), suffix);

	// Create dir if necessary
	if (create_path(result))
	{
		// Acknowledge original location
		file(result + ".location", rewrite).write(buf);
	}

	return result;
}

std::string fs::get_data_dir(const std::string& prefix, const std::string& path)
{
#ifdef _WIN32
	const auto& delim = "/\\";
#else
	const auto& delim = "/";
#endif

	// Extract file name and location
	const std::string& location = fs::get_parent_dir(path);
	const std::size_t name_pos = path.find_first_not_of(delim, location.size());

	return fs::get_data_dir(prefix, location, name_pos == -1 ? std::string{} : path.substr(name_pos));
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

template<>
void fmt_class_string<fs::seek_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto arg)
	{
		switch (arg)
		{
		STR_CASE(fs::seek_mode::seek_set);
		STR_CASE(fs::seek_mode::seek_cur);
		STR_CASE(fs::seek_mode::seek_end);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<fs::error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto arg)
	{
		switch (arg)
		{
		case fs::error::ok: return "OK";

		case fs::error::inval: return "Invalid arguments";
		case fs::error::noent: return "Not found";
		case fs::error::exist: return "Already exists";
		case fs::error::acces: return "Access violation";
		case fs::error::notempty: return "Not empty";
		}

		return unknown;
	});
}
