#include "File.h"
#include "mutex.h"
#include "StrFmt.h"

#include <span>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <map>
#include <iostream>

#include "util/asm.hpp"
#include "util/coro.hpp"

using namespace std::literals::string_literals;

#ifdef ANDROID
std::string g_android_executable_dir;
std::string g_android_config_dir;
std::string g_android_cache_dir;
#endif

#ifdef _WIN32

#include "Utilities/StrUtil.h"

#include <cwchar>
#include <Windows.h>

static std::unique_ptr<wchar_t[]> to_wchar(std::string_view source)
{
	// String size + null terminator
	const usz buf_size = source.size() + 1;

	// Safe size
	const int size = narrow<int>(buf_size);

	// Buffer for max possible output length
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[buf_size + 8 + 32768]);

	// Prepend wide path prefix (4 characters)
	std::memcpy(buffer.get() + 32768, L"\\\\\?\\", 4 * sizeof(wchar_t));

	// Test whether additional UNC prefix is required
	const bool unc = source.size() > 2 && (source[0] == '\\' || source[0] == '/') && source[1] == source[0];

	if (unc)
	{
		// Use \\?\UNC\ prefix
		std::memcpy(buffer.get() + 32768 + 4, L"UNC\\", 4 * sizeof(wchar_t));
	}

	ensure(MultiByteToWideChar(CP_UTF8, 0, source.data(), size, buffer.get() + 32768 + (unc ? 8 : 4), size)); // "to_wchar"

	// Canonicalize wide path (replace '/', ".", "..", \\ repetitions, etc)
	ensure(GetFullPathNameW(buffer.get() + 32768, 32768, buffer.get(), nullptr) - 1 < 32768 - 1); // "to_wchar"

	return buffer;
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
	else if (_time > s64{smax} / 10000000ll - 11644473600ll)
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
	case ERROR_FILENAME_EXCED_RANGE: return fs::error::toolong;
	case ERROR_DISK_FULL: return fs::error::nospace;
	case ERROR_NOT_SAME_DEVICE: return fs::error::xdev;
	default: return fs::error::unknown;
	}
}

#else

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>

#if defined(__APPLE__)
#include <copyfile.h>
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(__linux__) || defined(__sun)
#include <sys/sendfile.h>
#include <sys/syscall.h>
#include <linux/fs.h>
#else
#include <fstream>
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
	case EROFS: return fs::error::readonly;
	case EISDIR: return fs::error::isdir;
	case ENOSPC: return fs::error::nospace;
	case EXDEV: return fs::error::xdev;
	default: return fs::error::unknown;
	}
}

#endif

static std::string path_append(std::string_view path, std::string_view more)
{
	std::string result;

	if (const usz src_slash_pos = path.find_last_not_of('/'); src_slash_pos != path.npos)
	{
		path.remove_suffix(path.length() - src_slash_pos - 1);
		result = path;
	}

	result.push_back('/');

	if (const usz dst_slash_pos = more.find_first_not_of('/'); dst_slash_pos != more.npos)
	{
		more.remove_prefix(dst_slash_pos);
		result.append(more);
	}

	return result;
}

namespace fs
{
	thread_local error g_tls_error = error::ok;

	class device_manager final
	{
		mutable shared_mutex m_mutex{};

		std::unordered_map<std::string, shared_ptr<device_base>> m_map{};

	public:
		shared_ptr<device_base> get_device(const std::string& path);
		shared_ptr<device_base> set_device(const std::string& name, shared_ptr<device_base>);
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4646)
#endif
	[[noreturn]] stat_t file_base::get_stat()
	{
		fmt::throw_exception("fs::file::get_stat() not supported.");
	}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	void file_base::sync()
	{
		// Do notning
	}

	fs::native_handle fs::file_base::get_handle()
	{
#ifdef _WIN32
		return INVALID_HANDLE_VALUE;
#else
		return -1;
#endif
	}

	file_id file_base::get_id()
	{
		return {};
	}

	u64 file_base::write_gather(const iovec_clone* buffers, u64 buf_count)
	{
		u64 total = 0;

		for (u64 i = 0; i < buf_count; i++)
		{
			if (!buffers[i].iov_base || buffers[i].iov_len + total < total)
			{
				g_tls_error = error::inval;
				return -1;
			}

			total += buffers[i].iov_len;
		}

		const auto buf = std::make_unique<uchar[]>(total);

		u64 copied = 0;

		for (u64 i = 0; i < buf_count; i++)
		{
			std::memcpy(buf.get() + copied, buffers[i].iov_base, buffers[i].iov_len);
			copied += buffers[i].iov_len;
		}

		return this->write(buf.get(), total);
	}

	file_id::operator bool() const
	{
		return !type.empty() && !data.empty();
	}

	// Test if identical
	// For example: when LHS writes one byte to a file at X offset, RHS file be able to read that exact byte at X offset)
	bool file_id::is_mirror_of(const file_id& rhs) const
	{
		// If either is an invalid ID we cannot compare safely, return false
		return rhs && type == rhs.type && data == rhs.data;
	}

	// Test if both files point to the same file
	// For example: if a file descriptor pointing to the complete file exists and is being truncated to 0 bytes from non-
	// -zero size state: this has to affect both RHS and LHS files.
	bool file_id::is_coherent_with(const file_id& rhs) const
	{
		struct id_view
		{
			std::string_view type_view;
			std::span<const u8> data_view;
		};

		id_view _rhs{rhs.type, {rhs.data.data(), rhs.data.size()}};
		id_view _lhs{type, {data.data(), data.size()}};

		// Peek through fs::file wrappers
		auto peek_wrapppers = [](id_view& id)
		{
			u64 offset_count = 0;
			constexpr auto lv2_view = "lv2_file::file_view: "sv;

			for (usz pos = 0; (pos = id.type_view.find(lv2_view, pos)) != umax; pos += lv2_view.size())
			{
				offset_count++;
			}

			// Remove offsets data
			id.data_view = id.data_view.subspan(sizeof(u64) * offset_count);

			// Get last category identifier
			if (usz sep = id.type_view.rfind(": "); sep != umax)
			{
				id.type_view.remove_prefix(sep + 2);
			}
		};

		peek_wrapppers(_rhs);
		peek_wrapppers(_lhs);

		// If either is an invalid ID we cannot compare safely, return false
		if (_rhs.type_view.empty() || _rhs.data_view.empty())
		{
			return false;
		}

		return _rhs.type_view == _lhs.type_view && std::equal(_rhs.data_view.begin(), _rhs.data_view.end(), _lhs.data_view.begin(), _lhs.data_view.end());
	}

	dir_base::~dir_base()
	{
	}

	device_base::device_base()
		: fs_prefix(fmt::format("/vfsv0_%s%s_", fmt::base57(reinterpret_cast<u64>(this)), fmt::base57(utils::get_unique_tsc())))
	{
	}

	device_base::~device_base()
	{
	}

	bool device_base::remove_dir(const std::string&)
	{
		g_tls_error = error::readonly;
		return false;
	}

	bool device_base::create_dir(const std::string&)
	{
		g_tls_error = error::readonly;
		return false;
	}

	bool device_base::create_symlink(const std::string&)
	{
		g_tls_error = error::readonly;
		return false;
	}

	bool device_base::rename(const std::string&, const std::string&)
	{
		g_tls_error = error::readonly;
		return false;
	}

	bool device_base::remove(const std::string&)
	{
		g_tls_error = error::readonly;
		return false;
	}

	bool device_base::trunc(const std::string&, u64)
	{
		g_tls_error = error::readonly;
		return false;
	}

	bool device_base::utime(const std::string&, s64, s64)
	{
		g_tls_error = error::readonly;
		return false;
	}

#ifdef _WIN32
	class windows_file final : public file_base
	{
		HANDLE m_handle;
		atomic_t<u64> m_pos;

	public:
		windows_file(HANDLE handle)
			: m_handle(handle)
			, m_pos(0)
		{
		}

		~windows_file() override
		{
			if (m_handle != nullptr)
			{
				CloseHandle(m_handle);
			}
		}

		stat_t get_stat() override
		{
			FILE_BASIC_INFO basic_info;
			ensure(GetFileInformationByHandleEx(m_handle, FileBasicInfo, &basic_info, sizeof(FILE_BASIC_INFO))); // "file::stat"

			stat_t info;
			info.is_directory = (basic_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			info.is_writable = (basic_info.FileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
			info.size = this->size();
			info.atime = to_time(basic_info.LastAccessTime);
			info.mtime = to_time(basic_info.LastWriteTime);
			info.ctime = info.mtime;

			if (info.atime < info.mtime)
				info.atime = info.mtime;

			return info;
		}

		void sync() override
		{
			ensure(FlushFileBuffers(m_handle)); // "file::sync"
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
			u64 nread_sum = 0;

			for (char* data = static_cast<char*>(buffer); count;)
			{
				const DWORD size = static_cast<DWORD>(std::min<u64>(count, DWORD{umax} & -4096));

				DWORD nread = 0;
				OVERLAPPED ovl{};
				const u64 pos = m_pos;
				ovl.Offset = DWORD(pos);
				ovl.OffsetHigh = DWORD(pos >> 32);
				ensure(ReadFile(m_handle, data, size, &nread, &ovl) || GetLastError() == ERROR_HANDLE_EOF); // "file::read"
				nread_sum += nread;
				m_pos += nread;

				if (nread < size)
				{
					break;
				}

				count -= size;
				data += size;
			}

			return nread_sum;
		}

		u64 read_at(u64 offset, void* buffer, u64 count) override
		{
			u64 nread_sum = 0;

			for (char* data = static_cast<char*>(buffer); count;)
			{
				const DWORD size = static_cast<DWORD>(std::min<u64>(count, DWORD{umax} & -4096));

				DWORD nread = 0;
				OVERLAPPED ovl{};
				ovl.Offset = DWORD(offset);
				ovl.OffsetHigh = DWORD(offset >> 32);
				ensure(ReadFile(m_handle, data, size, &nread, &ovl) || GetLastError() == ERROR_HANDLE_EOF); // "file::read"
				nread_sum += nread;

				if (nread < size)
				{
					break;
				}

				count -= size;
				data += size;
				offset += size;
			}

			return nread_sum;
		}

		u64 write(const void* buffer, u64 count) override
		{
			u64 nwritten_sum = 0;

			for (const char* data = static_cast<const char*>(buffer); count;)
			{
				const DWORD size = static_cast<DWORD>(std::min<u64>(count, DWORD{umax} & -4096));

				DWORD nwritten = 0;
				OVERLAPPED ovl{};
				const u64 pos = m_pos.fetch_add(size);
				ovl.Offset = DWORD(pos);
				ovl.OffsetHigh = DWORD(pos >> 32);
				ensure(WriteFile(m_handle, data, size, &nwritten, &ovl)); // "file::write"
				ensure(nwritten == size);
				nwritten_sum += nwritten;

				if (nwritten < size)
				{
					break;
				}

				count -= size;
				data += size;
			}

			return nwritten_sum;
		}

		u64 seek(s64 offset, seek_mode whence) override
		{
			if (whence > seek_end)
			{
				fmt::throw_exception("Invalid whence (0x%x)", whence);
			}

			const s64 new_pos =
				whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? offset + m_pos :
				whence == fs::seek_end ? offset + size() : -1;

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
			LARGE_INTEGER size;
			ensure(GetFileSizeEx(m_handle, &size)); // "file::size"

			return size.QuadPart;
		}

		native_handle get_handle() override
		{
			return m_handle;
		}

		file_id get_id() override
		{
			file_id id{"windows_file"};
			id.data.resize(sizeof(FILE_ID_INFO));

			FILE_ID_INFO info;

			if (!GetFileInformationByHandleEx(m_handle, FileIdInfo, &info, sizeof(info)))
			{
				// Try GetFileInformationByHandle as a fallback
				BY_HANDLE_FILE_INFORMATION info2;
				ensure(GetFileInformationByHandle(m_handle, &info2));

				info = {};
				info.VolumeSerialNumber = info2.dwVolumeSerialNumber;
				std::memcpy(&info.FileId, &info2.nFileIndexHigh, 8);
			}

			std::memcpy(id.data.data(), &info, sizeof(info));
			return id;
		}

		void release() override
		{
			m_handle = nullptr;
		}
	};
#else
	class unix_file final : public file_base
	{
		int m_fd;

	public:
		unix_file(int fd)
			: m_fd(fd)
		{
		}

		~unix_file() override
		{
			if (m_fd >= 0)
			{
				::close(m_fd);
			}
		}

		stat_t get_stat() override
		{
			struct ::stat file_info;
			ensure(::fstat(m_fd, &file_info) == 0); // "file::stat"

			stat_t info;
			info.is_directory = S_ISDIR(file_info.st_mode);
			info.is_writable = file_info.st_mode & 0200; // HACK: approximation
			info.size = file_info.st_size;
			info.atime = file_info.st_atime;
			info.mtime = file_info.st_mtime;
			info.ctime = info.mtime;

			if (info.atime < info.mtime)
				info.atime = info.mtime;

			return info;
		}

		void sync() override
		{
			ensure(::fsync(m_fd) == 0); // "file::sync"
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
			u64 result = 0;

			// Loop because (huge?) read can be processed partially
			while (auto r = ::read(m_fd, buffer, count))
			{
				ensure(r > 0); // "file::read"
				count -= r;
				result += r;
				buffer = static_cast<u8*>(buffer) + r;
				if (!count)
					break;
			}

			return result;
		}

		u64 read_at(u64 offset, void* buffer, u64 count) override
		{
			u64 result = 0;

			// For safety; see read()
			while (auto r = ::pread(m_fd, buffer, count, offset))
			{
				ensure(r > 0); // "file::read_at"
				count -= r;
				offset += r;
				result += r;
				buffer = static_cast<u8*>(buffer) + r;
				if (!count)
					break;
			}

			return result;
		}

		u64 write(const void* buffer, u64 count) override
		{
			u64 result = 0;

			// For safety; see read()
			while (auto r = ::write(m_fd, buffer, count))
			{
				ensure(r > 0); // "file::write"
				count -= r;
				result += r;
				buffer = static_cast<const u8*>(buffer) + r;
				if (!count)
					break;
			}

			return result;
		}

		u64 seek(s64 offset, seek_mode whence) override
		{
			if (whence > seek_end)
			{
				fmt::throw_exception("Invalid whence (0x%x)", whence);
			}

			const int mode =
				whence == seek_set ? SEEK_SET :
				whence == seek_cur ? SEEK_CUR : SEEK_END;

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
			ensure(::fstat(m_fd, &file_info) == 0); // "file::size"

			return file_info.st_size;
		}

		native_handle get_handle() override
		{
			return m_fd;
		}

		file_id get_id() override
		{
			struct ::stat file_info;
			ensure(::fstat(m_fd, &file_info) == 0); // "file::get_id"

			file_id id{"unix_file"};
			id.data.resize(sizeof(file_info.st_dev) + sizeof(file_info.st_ino));

			std::memcpy(id.data.data(), &file_info.st_dev, sizeof(file_info.st_dev));
			std::memcpy(id.data.data() + sizeof(file_info.st_dev), &file_info.st_ino, sizeof(file_info.st_ino));
			return id;
		}

		u64 write_gather(const iovec_clone* buffers, u64 buf_count) override
		{
			static_assert(sizeof(iovec) == sizeof(iovec_clone), "Weird iovec size");
			static_assert(offsetof(iovec, iov_len) == offsetof(iovec_clone, iov_len), "Weird iovec::iov_len offset");

			u64 result = 0;

			while (buf_count)
			{
				iovec arg[256];
				const auto count = std::min<u64>(buf_count, 256);
				std::memcpy(&arg, buffers, sizeof(iovec) * count);
				const auto added = ::writev(m_fd, arg, count);
				ensure(added != -1); // "file::write_gather"
				result += added;
				buf_count -= count;
				buffers += count;
			}

			return result;
		}

		void release() override
		{
			m_fd = -1;
		}
	};
#endif
}

shared_ptr<fs::device_base> fs::device_manager::get_device(const std::string& path)
{
	reader_lock lock(m_mutex);

	const usz prefix = path.find_first_of('_', 7) + 1;

	const auto found = m_map.find(path.substr(prefix, path.find_first_of('/', 1) - prefix));

	if (found == m_map.end() || !path.starts_with(found->second->fs_prefix))
	{
		return null_ptr;
	}

	return found->second;
}

shared_ptr<fs::device_base> fs::device_manager::set_device(const std::string& name, shared_ptr<device_base> device)
{
	std::lock_guard lock(m_mutex);

	if (device)
	{
		// Adding
		if (auto [it, ok] = m_map.try_emplace(name, std::move(device)); ok)
		{
			return it->second;
		}

		g_tls_error = error::exist;
	}
	else
	{
		// Removing
		if (auto found = m_map.find(name); found != m_map.end())
		{
			device = std::move(found->second);
			m_map.erase(found);
			return device;
		}

		g_tls_error = error::noent;
	}

	return null_ptr;
}

shared_ptr<fs::device_base> fs::get_virtual_device(const std::string& path)
{
	// Every virtual device path must have specific name at the beginning
	if (path.starts_with("/vfsv0_") && path.size() >= 8 + 22 && path[29] == '_' && path.find_first_of('/', 1) > 29)
	{
		return get_device_manager().get_device(path);
	}

	return null_ptr;
}

shared_ptr<fs::device_base> fs::set_virtual_device(const std::string& name, shared_ptr<device_base> device)
{
	return get_device_manager().set_device(name, std::move(device));
}

std::string_view fs::get_parent_dir_view(std::string_view path, u32 parent_level)
{
	std::string_view result = path;

	// Number of path components to remove
	usz to_remove = parent_level;

	while (to_remove--)
	{
		// Trim contiguous delimiters at the end
		if (usz sz = result.find_last_not_of(delim) + 1)
		{
			result = result.substr(0, sz);
		}
		else
		{
			return "/";
		}

		const auto elem = result.substr(result.find_last_of(delim) + 1);

		if (elem.empty() || elem.size() == result.size())
		{
			break;
		}

		if (elem == ".")
		{
			to_remove += 1;
		}

		if (elem == "..")
		{
			to_remove += 2;
		}

		result.remove_suffix(elem.size());
	}

	if (usz sz = result.find_last_not_of(delim) + 1)
	{
		result = result.substr(0, sz);
	}
	else
	{
		return "/";
	}

	return result;
}

bool fs::get_stat(const std::string& path, stat_t& info)
{
	// Ensure consistent information on failure
	info = {};

	if (auto device = get_virtual_device(path))
	{
		return device->stat(path, info);
	}

#ifdef _WIN32
	std::string_view epath = path;

	// '/' and '\\' Not allowed by FindFirstFileExW at the end of path but we should allow it
	if (auto not_del = epath.find_last_not_of(delim); not_del != umax && not_del != epath.size() - 1)
	{
		epath.remove_suffix(epath.size() - 1 - not_del);
	}

	// Handle drives specially
	if (epath.find_first_of(delim) == umax && epath.ends_with(':'))
	{
		WIN32_FILE_ATTRIBUTE_DATA attrs{};

		// Must end with a delimiter
		if (!GetFileAttributesExW(to_wchar(std::string(epath) + '/').get(), GetFileExInfoStandard, &attrs))
		{
			g_tls_error = to_error(GetLastError());
			return false;
		}

		info.is_directory = true; // Handle drives as directories
		info.is_writable = (attrs.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
		info.size = attrs.nFileSizeLow | (u64{attrs.nFileSizeHigh} << 32);
		info.atime = to_time(attrs.ftLastAccessTime);
		info.mtime = to_time(attrs.ftLastWriteTime);
		info.ctime = info.mtime;

		if (info.atime < info.mtime)
			info.atime = info.mtime;

		return true;
	}

	// Allowed by FindFirstFileExW but we should not allow it
	if (epath.ends_with("*"))
	{
		g_tls_error = fs::error::noent;
		return false;
	}

	WIN32_FIND_DATA attrs{};

	const auto wchar_ptr = to_wchar(std::string(epath));
	const std::wstring_view wpath_view = wchar_ptr.get();

	const HANDLE handle = FindFirstFileExW(wpath_view.data(), FindExInfoStandard, &attrs, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_CASE_SENSITIVE);

	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	struct close_t
	{
		HANDLE handle;
		~close_t() { FindClose(handle); }
	};

	for (close_t find_manage{handle}; attrs.cFileName != wpath_view.substr(wpath_view.find_last_of(wdelim) + 1);)
	{
		if (!FindNextFileW(handle, &attrs))
		{
			if (const DWORD err = GetLastError(); err != ERROR_NO_MORE_FILES)
			{
				g_tls_error = to_error(err);
				return false;
			}

			g_tls_error = fs::error::noent;
			return false;
		}
	}

	info.is_directory = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	info.is_symlink = (attrs.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
	info.is_writable = (attrs.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	info.size = attrs.nFileSizeLow | (u64{attrs.nFileSizeHigh} << 32);
	info.atime = to_time(attrs.ftLastAccessTime);
	info.mtime = to_time(attrs.ftLastWriteTime);
	info.ctime = info.mtime;
#else
	struct ::stat file_info;
	if (::stat(path.c_str(), &file_info) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	info.is_directory = S_ISDIR(file_info.st_mode);
	info.is_symlink = S_ISLNK(file_info.st_mode);
	info.is_writable = file_info.st_mode & 0200; // HACK: approximation
	info.size = file_info.st_size;
	info.atime = file_info.st_atime;
	info.mtime = file_info.st_mtime;
	info.ctime = info.mtime;
#endif

	if (info.atime < info.mtime)
		info.atime = info.mtime;

	return true;
}

bool fs::exists(const std::string& path)
{
	fs::stat_t info{};
	return fs::get_stat(path, info);
}

bool fs::is_file(const std::string& path)
{
	fs::stat_t info{};
	if (!fs::get_stat(path, info))
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

bool fs::is_dir(const std::string& path)
{
	fs::stat_t info{};
	if (!fs::get_stat(path, info))
	{
		return false;
	}

	if (!info.is_directory)
	{
		g_tls_error = error::exist;
		return false;
	}

	return true;
}

bool fs::is_symlink(const std::string& path)
{
	fs::stat_t info{};
	if (!fs::get_stat(path, info))
	{
		return false;
	}

	if (!info.is_symlink)
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

	// Convert path and return it back to the "short" format
	const bool unc = path.size() > 2 && (path[0] == '\\' || path[0] == '/') && path[1] == path[0];

	std::wstring str = to_wchar(path).get() + (unc ? 6 : 4);

	if (unc)
	{
		str[0] = '\\';
		str[1] = '\\';
	}

	// Keep cutting path from right until it's short enough
	while (str.size() > 256)
	{
		if (usz x = str.find_last_of('\\') + 1)
			str.resize(x - 1);
		else
			break;
	}

	if (!GetDiskFreeSpaceExW(str.c_str(), &avail_free, &total_size, &total_free))
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
	if (!CreateDirectoryW(to_wchar(path).get(), nullptr))
	{
		int res = GetLastError();

		if (res == ERROR_ACCESS_DENIED && is_dir(path))
		{
			// May happen on drives
			res = ERROR_ALREADY_EXISTS;
		}

		g_tls_error = to_error(res);
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

#ifdef _WIN32
	// Workaround: don't call is_dir with naked drive letter
	if (parent.size() < path.size() && (parent.empty() || (parent.back() != ':' && !is_dir(parent) && !create_path(parent))))
#else
	if (parent.size() < path.size() && !is_dir(parent) && !create_path(parent))
#endif
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
	if (path.empty())
	{
		// Don't allow removing empty path (TODO)
		g_tls_error = fs::error::noent;
		return false;
	}

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
#else
	if (::rmdir(path.c_str()) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}
#endif

	return true;
}

bool fs::create_symlink(const std::string& path, const std::string& target)
{
	if (auto device = get_virtual_device(path))
	{
		return device->create_symlink(path);
	}

#ifdef _WIN32
	const DWORD flags = is_dir(target) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0;
	if (!CreateSymbolicLinkW(to_wchar(path).get(), to_wchar(target).get(), flags))
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	return true;
#else
	if (::symlink(target.c_str(), path.c_str()) != 0)
	{
		g_tls_error = to_error(errno);
		return false;
	}

	return true;
#endif
}

bool fs::rename(const std::string& from, const std::string& to, bool overwrite)
{
	if (from.empty() || to.empty())
	{
		// Don't allow opening empty path (TODO)
		g_tls_error = fs::error::noent;
		return false;
	}

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
				CreateDirectoryW(ws2.get(), nullptr); // TODO
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

#ifdef __linux__
	if (syscall(SYS_renameat2, AT_FDCWD, from.c_str(), AT_FDCWD, to.c_str(), overwrite ? 0 : 1 /* RENAME_NOREPLACE */) == 0)
	{
		return true;
	}

	// If the filesystem doesn't support RENAME_NOREPLACE, it returns EINVAL. Retry with fallback method in that case.
	if (errno != EINVAL || overwrite)
	{
		g_tls_error = to_error(errno);
		return false;
	}
#endif

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
#elif defined(__APPLE__) || defined(__linux__) || defined(__sun)
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
#elif defined(__linux__) || defined(__sun)
	// sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
	struct ::stat fileinfo = { 0 };
	bool result = ::fstat(input, &fileinfo) != -1;
	if (result)
	{
		for (off_t bytes_copied = 0; bytes_copied < fileinfo.st_size; /* Do nothing, bytes_copied is increased by sendfile. */)
		{
			if (::sendfile(output, input, &bytes_copied, fileinfo.st_size - bytes_copied) == -1)
			{
				result = false;
				break;
			}
		}
	}
	if (!result)
#else
#error "Native file copy implementation is missing"
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
#else // fallback
	{
		std::ifstream out{to, std::ios::binary};
		if (out.good() && !overwrite)
		{
			g_tls_error = to_error(EEXIST);
			return false;
		}
	}

	std::ifstream in{from, std::ios::binary};
	std::ofstream out{to,  std::ios::binary};

	if (!in.good() || !out.good())
	{
		g_tls_error = to_error(errno);
		return false;
	}

	std::istreambuf_iterator<char> bin(in);
	std::istreambuf_iterator<char> ein;
	std::ostreambuf_iterator<char> bout(out);
	std::copy(bin, ein, bout);

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
	const auto handle = CreateFileW(to_wchar(path).get(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
	const auto handle = CreateFileW(to_wchar(path).get(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	FILETIME _atime = from_time(atime);
	FILETIME _mtime = from_time(mtime);
	if (!SetFileTime(handle, nullptr, &_atime, &_mtime))
	{
		const DWORD last_error = GetLastError();
		g_tls_error = to_error(last_error);

		// Some filesystems fail to set a date lower than 01/01/1980 00:00:00
		if (last_error == ERROR_INVALID_PARAMETER && (atime < 315532800 || mtime < 315532800))
		{
			// Try again with 01/01/1980 00:00:00
			_atime = from_time(std::max<s64>(atime, 315532800));
			_mtime = from_time(std::max<s64>(mtime, 315532800));

			if (SetFileTime(handle, nullptr, &_atime, &_mtime))
			{
				CloseHandle(handle);
				return true;
			}
		}

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

void fs::sync()
{
#ifdef _WIN32
	fs::g_tls_error = fs::error::unknown;
#else
	::sync();
	fs::g_tls_error = fs::error::ok;
#endif
}

[[noreturn]] void fs::xnull(std::source_location loc)
{
	fmt::throw_exception("Null object.%s", loc);
}

[[noreturn]] void fs::xfail(std::source_location loc)
{
	fmt::throw_exception("Unexpected fs::error %s%s", g_tls_error, loc);
}

[[noreturn]] void fs::xovfl()
{
	fmt::throw_exception("Stream overflow.");
}

fs::file::file(const std::string& path, bs_t<open_mode> mode)
{
	if (path.empty())
	{
		// Don't allow opening empty path (TODO)
		g_tls_error = fs::error::noent;
		return;
	}

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
	if (mode & fs::read) access |= GENERIC_READ;
	if (mode & fs::write) access |= DELETE | (mode & fs::append ? FILE_APPEND_DATA : GENERIC_WRITE);

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
			return;
		}

		disp = mode & fs::trunc ? TRUNCATE_EXISTING : OPEN_EXISTING;
	}

	DWORD share = FILE_SHARE_DELETE;
	if (!(mode & fs::unread) || !(mode & fs::write))
	{
		share |= FILE_SHARE_READ;
	}

	if (!(mode & (fs::lock + fs::unread)) || !(mode & fs::write))
	{
		share |= FILE_SHARE_WRITE;
	}

	const HANDLE handle = CreateFileW(to_wchar(path).get(), access, share, nullptr, disp, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return;
	}

	m_file = std::make_unique<windows_file>(handle);
#else
	int flags = O_CLOEXEC; // Ensures all files are closed on execl for auto updater

	if (mode & fs::read && mode & fs::write) flags |= O_RDWR;
	else if (mode & fs::read) flags |= O_RDONLY;
	else if (mode & fs::write) flags |= O_WRONLY;

	if (mode & fs::append) flags |= O_APPEND;
	if (mode & fs::create) flags |= O_CREAT;
	if (mode & fs::trunc && !(mode & fs::lock)) flags |= O_TRUNC;
	if (mode & fs::excl) flags |= O_EXCL;

	int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if (mode & fs::write && mode & fs::unread)
	{
		if (!(mode & (fs::excl + fs::lock)) && mode & fs::trunc)
		{
			// Alternative to truncation for "unread" flag (TODO)
			if (mode & fs::create)
			{
				::unlink(path.c_str());
			}
		}

		perm = 0;
	}

	const int fd = ::open(path.c_str(), flags, perm);

	if (fd == -1)
	{
		g_tls_error = to_error(errno);
		return;
	}

	if (mode & fs::write && mode & fs::lock && ::flock(fd, LOCK_EX | LOCK_NB) != 0)
	{
		g_tls_error = errno == EWOULDBLOCK ? fs::error::acces : to_error(errno);
		::close(fd);
		return;
	}

	if (mode & fs::trunc && mode & fs::lock && mode & fs::write)
	{
		// Postpone truncation in order to avoid using O_TRUNC on a locked file
		ensure(::ftruncate(fd, 0) == 0);
	}

	m_file = std::make_unique<unix_file>(fd);

	if (mode & fs::isfile && !(mode & fs::write) && get_stat().is_directory)
	{
		m_file.reset();
		g_tls_error = error::isdir;
	}
#endif
}



fs::file fs::file::from_native_handle(native_handle handle)
{
	fs::file result;

#ifdef _WIN32
	result.m_file = std::make_unique<windows_file>(static_cast<HANDLE>(handle));
#else
	result.m_file = std::make_unique<unix_file>(handle);
#endif

	return result;
}

fs::file::file(const void* ptr, usz size)
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

		memory_stream(const memory_stream&) = delete;

		memory_stream& operator=(const memory_stream&) = delete;

		bool trunc(u64) override
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

		u64 read_at(u64 offset, void* buffer, u64 count) override
		{
			if (offset < m_size)
			{
				// Get readable size
				if (const u64 result = std::min<u64>(count, m_size - offset))
				{
					std::memcpy(buffer, m_ptr + offset, result);
					return result;
				}
			}

			return 0;
		}

		u64 write(const void*, u64) override
		{
			return 0;
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			const s64 new_pos =
				whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? offset + m_pos :
				whence == fs::seek_end ? offset + size() : -1;

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
	if (m_file)
	{
		return m_file->get_handle();
	}

#ifdef _WIN32
	return INVALID_HANDLE_VALUE;
#else
	return -1;
#endif
}

fs::file_id fs::file::get_id() const
{
	if (m_file)
	{
		return m_file->get_id();
	}

	return {};
}

bool fs::dir::open(const std::string& path)
{
	m_dir.reset();

	if (path.empty())
	{
		// Don't allow opening empty path (TODO)
		g_tls_error = fs::error::noent;
		return false;
	}

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
	const auto handle = FindFirstFileExW(to_wchar(path + "/*").get(), FindExInfoBasic, &found, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_CASE_SENSITIVE | FIND_FIRST_EX_LARGE_FETCH);

	if (handle == INVALID_HANDLE_VALUE)
	{
		g_tls_error = to_error(GetLastError());
		return false;
	}

	class windows_dir final : public dir_base
	{
		std::vector<dir_entry> m_entries;
		usz m_pos = 0;

		void add_entry(const WIN32_FIND_DATAW& found)
		{
			dir_entry info;

			info.name = wchar_to_utf8(found.cFileName);
			info.is_directory = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			info.is_writable = (found.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
			info.size = (static_cast<u64>(found.nFileSizeHigh) << 32) | static_cast<u64>(found.nFileSizeLow);
			info.atime = to_time(found.ftLastAccessTime);
			info.mtime = to_time(found.ftLastWriteTime);
			info.ctime = info.mtime;

			if (info.atime < info.mtime)
				info.atime = info.mtime;

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

			ensure(ERROR_NO_MORE_FILES == GetLastError()); // "dir::read"
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

		unix_dir(const unix_dir&) = delete;

		unix_dir& operator=(const unix_dir&) = delete;

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
			info.ctime = info.mtime;

			if (info.atime < info.mtime)
				info.atime = info.mtime;

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

bool fs::file::strict_read_check(u64 offset, u64 _size, u64 type_size) const
{
	if (usz pos0 = offset, size0 = size(); (pos0 >= size0 ? 0 : (size0 - pos0)) / type_size < _size)
	{
		fs::g_tls_error = fs::error::inval;
		return false;
	}

	return true;
}

std::string fs::get_executable_path()
{
#ifdef ANDROID
	return g_android_executable_dir + "/dummy-rpcs3.apk";
#else
	// Use magic static
	static const std::string s_exe_path = []
	{
#if defined(_WIN32)
		constexpr DWORD size = 32767;
		std::vector<wchar_t> buffer(size);
		GetModuleFileNameW(nullptr, buffer.data(), size);
		return wchar_to_utf8(buffer.data());
#elif defined(__APPLE__)
		char bin_path[PATH_MAX];
		uint32_t bin_path_size = sizeof(bin_path);
		if (_NSGetExecutablePath(bin_path, &bin_path_size) != 0)
		{
			std::cerr << "Failed to find app binary path" << std::endl;
			return std::string{};
		}

		// App bundle directory is three levels up from the binary.
		return get_parent_dir(bin_path, 3);
#else
		if (const char* appimage_path = ::getenv("APPIMAGE"))
		{
			std::cout << "Found AppImage path: " << appimage_path << std::endl;
			return std::string(appimage_path);
		}

		std::cout << "No AppImage path found, checking for executable" << std::endl;

		char exe_path[PATH_MAX];
		const ssize_t len = ::readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

		if (len == -1)
		{
			std::cerr << "Failed to find executable path" << std::endl;
			return std::string{};
		}

		exe_path[len] = '\0';
		std::cout << "Found exec path: " << exe_path << std::endl;

		return std::string(exe_path);
#endif
	}();

	return s_exe_path;
#endif
}

std::string fs::get_executable_dir()
{
#ifdef ANDROID
	return g_android_executable_dir;
#else
	// Use magic static
	static const std::string s_exe_dir = []
	{
		std::string exe_path = get_executable_path();
		if (exe_path.empty())
		{
			return exe_path;
		}

		return get_parent_dir(exe_path) + "/";
	}();

	return s_exe_dir;
#endif
}

const std::string& fs::get_config_dir([[maybe_unused]] bool get_config_subdirectory)
{
#ifdef ANDROID
	return g_android_config_dir;
#else
	// Use magic static
	static const std::string s_dir = []
	{
		std::string dir;

		// Check if a portable directory exists.
		std::string portable_dir = get_executable_dir() + "/portable/";
		if (is_dir(portable_dir))
		{
			return portable_dir;
		}

#ifdef _WIN32
		std::vector<wchar_t> buf;

		// Check if RPCS3_CONFIG_DIR is set and get the required buffer size
		DWORD size = GetEnvironmentVariable(L"RPCS3_CONFIG_DIR", nullptr, 0);
		if (size > 0)
		{
			// Resize buffer and fetch RPCS3_CONFIG_DIR
			buf.resize(size);

			if (GetEnvironmentVariable(L"RPCS3_CONFIG_DIR", buf.data(), size) != (size - 1))
			{
				// Clear buffer on failure and notify user
				MessageBoxA(nullptr, fmt::format("GetEnvironmentVariable(RPCS3_CONFIG_DIR) failed: error: %s", fmt::win_error{GetLastError(), nullptr}).c_str(), "fs::get_config_dir()", MB_ICONERROR);
				buf.clear();
			}
		}

		// Fallback to executable path if needed
		for (DWORD buf_size = MAX_PATH; size == 0; buf_size += MAX_PATH)
		{
			buf.resize(buf_size);
			size = GetModuleFileName(nullptr, buf.data(), buf_size);

			if (size == 0)
			{
				MessageBoxA(nullptr, fmt::format("GetModuleFileName() failed: error: %s", fmt::win_error{GetLastError(), nullptr}).c_str(), "fs::get_config_dir()", MB_ICONERROR);
				return dir; // empty
			}

			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				// Try again with increased buffer size
				size = 0;
				continue;
			}
		}

		dir = wchar_to_utf8(buf.data());

		std::replace(dir.begin(), dir.end(), '\\', '/');

		dir.resize(dir.rfind('/') + 1);
#else

#ifdef __APPLE__
		if (const char* home = ::getenv("HOME"))
			dir = home + "/Library/Application Support"s;
#else
		if (const char* conf = ::getenv("XDG_CONFIG_HOME"))
			dir = conf;
		else if (const char* home = ::getenv("HOME"))
			dir = home + "/.config"s;
#endif
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

#ifdef _WIN32
	if (get_config_subdirectory)
	{
		static const std::string subdir = s_dir + "config/";
		return subdir;
	}
#endif

	return s_dir;
#endif
}

const std::string& fs::get_cache_dir()
{
#ifdef ANDROID
	return g_android_cache_dir;
#else
	static const std::string s_dir = []
	{
		std::string dir;

#ifdef _WIN32
		dir = get_config_dir();
#else

#ifdef __APPLE__
		if (const char* home = ::getenv("HOME"))
			dir = home + "/Library/Caches"s;
#else
		if (const char* cache = ::getenv("XDG_CACHE_HOME"))
			dir = cache;
		else if (const char* conf = ::getenv("XDG_CONFIG_HOME"))
			dir = conf;
		else if (const char* home = ::getenv("HOME"))
			dir = home + "/.cache"s;
#endif
		else // Just in case
			dir = "./cache";

		dir += "/rpcs3/";

		if (!create_path(dir))
		{
			std::printf("Failed to create configuration directory '%s' (%d).\n", dir.c_str(), errno);
		}
#endif

		return dir;
	}();

	return s_dir;
#endif
}

const std::string& fs::get_log_dir()
{
#ifdef _WIN32
	static const std::string s_dir = fs::get_config_dir() + "log/";
	return s_dir;
#else
	return fs::get_cache_dir();
#endif
}

const std::string& fs::get_temp_dir()
{
	static const std::string s_dir = []
	{
		std::string dir;

#ifdef _WIN32
		wchar_t buf[MAX_PATH + 2]{};
		if (GetTempPathW(MAX_PATH + 1, buf) - 1 > MAX_PATH)
		{
			MessageBoxA(nullptr, fmt::format("GetTempPath() failed: error: %s", fmt::win_error{GetLastError(), nullptr}).c_str(), "fs::get_temp_dir()", MB_ICONERROR);
			return dir; // empty
		}

		dir = wchar_to_utf8(buf);
#else
		const char* tmp_dir = getenv("TMPDIR");
		if (tmp_dir == nullptr || tmp_dir[0] == '\0')
		{
			// Fall back to cache directory
			dir = get_cache_dir();
		}
		else
		{
			dir = tmp_dir;
			if (!dir.ends_with("/"))
			{
				// Ensure path ends with a separator
				dir += "/";
			}
		}
#endif

		return dir;
	}();

	return s_dir;
}

bool fs::remove_all(const std::string& path, bool remove_root, bool is_no_dir_ok)
{
	if (const auto root_dir = dir(path))
	{
		for (const auto& entry : root_dir)
		{
			if (entry.name == "." || entry.name == "..")
			{
				continue;
			}

			if (!entry.is_directory)
			{
				if (!remove_file(path_append(path, entry.name)))
				{
					return false;
				}
			}
			else
			{
				if (!remove_all(path_append(path, entry.name)))
				{
					return false;
				}
			}
		}
	}
	else
	{
		return is_no_dir_ok;
	}

	if (remove_root)
	{
		return remove_dir(path);
	}

	return true;
}

u64 fs::get_dir_size(const std::string& path, u64 rounding_alignment, atomic_t<bool>* cancel_flag)
{
	u64 result = 0;

	const auto root_dir = dir(path);

	if (!root_dir)
	{
		return -1;
	}

	for (const auto& entry : root_dir)
	{
		if (cancel_flag && *cancel_flag)
		{
			return umax;
		}

		if (entry.name == "." || entry.name == "..")
		{
			continue;
		}

		if (!entry.is_directory)
		{
			result += utils::align(entry.size, rounding_alignment);
		}
		else
		{
			const u64 size = get_dir_size(path_append(path, entry.name), rounding_alignment);

			if (size == umax)
			{
				return size;
			}

			result += size;
		}
	}

	return result;
}

fs::file fs::make_gather(std::vector<fs::file> files)
{
	struct gather_stream : file_base
	{
		u64 pos = 0;
		u64 end = 0;
		std::vector<file> files{};
		std::map<u64, u64> ends{}; // Fragment End Offset -> Index

		gather_stream(std::vector<fs::file> arg)
			: files(std::move(arg))
		{
			// Preprocess files
			for (auto&& f : files)
			{
				end += f.size();
				ends.emplace(end, ends.size());
			}
		}

		~gather_stream() override
		{
		}

		fs::stat_t get_stat() override
		{
			fs::stat_t result{};

			if (!files.empty())
			{
				result = files[0].get_stat();
			}

			result.is_directory = false;
			result.is_writable = false;
			result.size = end;
			return result;
		}

		bool trunc(u64) override
		{
			return false;
		}

		u64 read(void* buffer, u64 size) override
		{
			if (pos < end)
			{
				// Current pos
				const u64 start = pos;

				// Get readable size
				if (const u64 max = std::min<u64>(size, end - pos))
				{
					u8* buf_out = static_cast<u8*>(buffer);
					u64 buf_max = max;

					for (auto it = ends.upper_bound(pos); it != ends.end(); ++it)
					{
						// Set position for the fragment
						files[it->second].seek(pos - it->first, fs::seek_end);

						const u64 count = std::min<u64>(it->first - pos, buf_max);
						const u64 read  = files[it->second].read(buf_out, count);

						buf_out += count;
						buf_max -= count;
						pos     += read;

						if (read < count || buf_max == 0)
						{
							break;
						}
					}

					return pos - start;
				}
			}

			return 0;
		}

		u64 read_at(u64 start, void* buffer, u64 size) override
		{
			if (start < end)
			{
				u64 pos = start;

				// Get readable size
				if (const u64 max = std::min<u64>(size, end - pos))
				{
					u8* buf_out = static_cast<u8*>(buffer);
					u64 buf_max = max;

					for (auto it = ends.upper_bound(pos); it != ends.end(); ++it)
					{
						const u64 count = std::min<u64>(it->first - pos, buf_max);
						const u64 read  = files[it->second].read_at(files[it->second].size() + pos - it->first, buf_out, count);

						buf_out += count;
						buf_max -= count;
						pos     += read;

						if (read < count || buf_max == 0)
						{
							break;
						}
					}

					return pos - start;
				}
			}

			return 0;
		}

		u64 write(const void*, u64) override
		{
			return 0;
		}

		u64 seek(s64 offset, seek_mode whence) override
		{
			const s64 new_pos =
				whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? offset + pos :
				whence == fs::seek_end ? offset + end : -1;

			if (new_pos < 0)
			{
				fs::g_tls_error = fs::error::inval;
				return -1;
			}

			pos = new_pos;
			return pos;
		}

		u64 size() override
		{
			return end;
		}
	};

	fs::file result;
	result.reset(std::make_unique<gather_stream>(std::move(files)));
	return result;
}

std::string fs::generate_neighboring_path(std::string_view source, [[maybe_unused]] u64 seed)
{
	// Seed is currently not used

	return fmt::format(u8"%s/＄%s.%s.tmp", get_parent_dir(source), source.substr(source.find_last_of(fs::delim) + 1), fmt::base57(utils::get_unique_tsc()));
}

bool fs::pending_file::open(std::string_view path)
{
	file.close();

	if (!m_path.empty())
	{
		fs::remove_file(m_path);
	}

	if (path.empty())
	{
		fs::g_tls_error = fs::error::noent;
		m_dest.clear();
		return false;
	}

	do
	{
		m_path = fs::generate_neighboring_path(path, 0);

		if (file.open(m_path, fs::create + fs::write + fs::read + fs::excl))
		{
#ifdef _WIN32
			// Auto-delete pending log file
			FILE_DISPOSITION_INFO disp;
			disp.DeleteFileW = true;
			SetFileInformationByHandle(file.get_handle(), FileDispositionInfo, &disp, sizeof(disp));
#endif
			m_dest = path;
			break;
		}

		m_path.clear();
	}
	while (fs::g_tls_error == fs::error::exist); // Only retry if failed due to existing file

	return file.operator bool();
}

fs::pending_file::~pending_file()
{
	file.close();

	if (!m_path.empty())
	{
		fs::remove_file(m_path);
	}
}

bool fs::pending_file::commit(bool overwrite)
{
	if (m_path.empty())
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	// The temporary file's contents must be on disk before rename
#ifndef _WIN32
	if (file)
	{
		file.sync();
	}
#endif

#ifdef _WIN32
	if (file)
	{
		// Disable auto-delete
		FILE_DISPOSITION_INFO disp;
		disp.DeleteFileW = false;
		ensure(SetFileInformationByHandle(file.get_handle(), FileDispositionInfo, &disp, sizeof(disp)));
	}

	std::vector<std::wstring> hardlink_paths;

	const auto ws1 = to_wchar(m_path);

	const HANDLE file_handle = !overwrite ? INVALID_HANDLE_VALUE
		: CreateFileW(ws1.get(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	while (file_handle != INVALID_HANDLE_VALUE)
	{
		// Get file ID (used to check for hardlinks)
		BY_HANDLE_FILE_INFORMATION file_info;

		if (!GetFileInformationByHandle(file_handle, &file_info) || file_info.nNumberOfLinks == 1)
		{
			CloseHandle(file_handle);
			break;
		}

		// Buffer for holding link name
		std::wstring link_name_buffer(MAX_PATH, wchar_t{});
		DWORD buffer_size{};
		HANDLE find_handle = INVALID_HANDLE_VALUE;

		while (true)
		{
			buffer_size = static_cast<DWORD>(link_name_buffer.size() - 1);
			find_handle = FindFirstFileNameW(ws1.get(), 0, &buffer_size, link_name_buffer.data());

			if (find_handle != INVALID_HANDLE_VALUE || GetLastError() != ERROR_MORE_DATA)
			{
				break;
			}

			link_name_buffer.resize(buffer_size + 1);
		}

		if (find_handle != INVALID_HANDLE_VALUE)
		{
			const std::wstring_view ws1_sv = ws1.get();

			while (true)
			{
				if (link_name_buffer.c_str() != ws1_sv)
				{
					// Note: link_name_buffer is a buffer which may contain zeroes so truncate it
					hardlink_paths.push_back(link_name_buffer.c_str());
				}

				buffer_size = static_cast<DWORD>(link_name_buffer.size() - 1);
				if (!FindNextFileNameW(find_handle, &buffer_size, link_name_buffer.data()))
				{
					if (GetLastError() != ERROR_MORE_DATA)
					{
						break;
					}

					link_name_buffer.resize(buffer_size + 1);
				}
			}
		}

		// Clean up
		FindClose(find_handle);
		CloseHandle(file_handle);
		break;
	}

	if (!hardlink_paths.empty())
	{
		// REPLACEFILE_WRITE_THROUGH is not supported
		file.sync();
	}
#endif

	file.close();

#ifdef _WIN32
	const auto wdest = to_wchar(m_dest);

	bool ok = false;

	if (hardlink_paths.empty())
	{
		ok = MoveFileExW(ws1.get(), wdest.get(), overwrite ? MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH : MOVEFILE_WRITE_THROUGH);
	}
	else
	{
		ok = ReplaceFileW(ws1.get(), wdest.get(), nullptr, 0, nullptr, nullptr);
	}

	if (ok)
	{
		for (const std::wstring& link_name : hardlink_paths)
		{
			std::unique_ptr<wchar_t[]> write_temp_path;

			do
			{
				write_temp_path = to_wchar(fs::generate_neighboring_path(m_dest, 0));

				// Generate a temporary hard linke
				if (CreateHardLinkW(wdest.get(), write_temp_path.get(), nullptr))
				{
					if (MoveFileExW(write_temp_path.get(), link_name.data(), MOVEFILE_REPLACE_EXISTING))
					{
						// Success
						write_temp_path.reset();
						break;
					}

					break;
				}
			}
			while (fs::g_tls_error == fs::error::exist); // Only retry if failed due to existing file

			if (write_temp_path)
			{
				// Failure
				g_tls_error = to_error(GetLastError());
				return false;
			}
		}

		// Disable the destructor
		m_path.clear();
		return true;
	}

	g_tls_error = to_error(GetLastError());
#else
	if (fs::rename(m_path, m_dest, overwrite))
	{
		// Disable the destructor
		m_path.clear();
		return true;
	}
#endif

	return false;
}

stx::generator<fs::dir_entry&> fs::list_dir_recursively(const std::string& path)
{
	for (auto& entry : fs::dir(path))
	{
		if (entry.name == "." || entry.name == "..")
		{
			continue;
		}

		std::string new_path = path_append(path, entry.name);

		if (entry.is_directory)
		{
			for (auto& nested : fs::list_dir_recursively(new_path))
			{
				co_yield nested;
			}
		}

		entry.name = std::move(new_path);
		co_yield entry;
	}
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
	if (arg == static_cast<u64>(fs::error::unknown))
	{
		// Note: may not be the correct error code because it only prints the last
#ifdef _WIN32
		fmt::append(out, "Unknown error [errno=%d]", GetLastError());
#else
		fmt::append(out, "Unknown error [errno=%d]", errno);
#endif
		return;
	}

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
		case fs::error::readonly: return "Read only";
		case fs::error::isdir: return "Is a directory";
		case fs::error::toolong: return "Path too long";
		case fs::error::nospace: return "Not enough space on the device";
		case fs::error::xdev: return "Device mismatch";
		case fs::error::unknown: return "Unknown system error";
		}

		return unknown;
	});
}

template<>
void fmt_class_string<fs::file_id>::format(std::string& out, u64 arg)
{
	const fs::file_id& id = get_object(arg);

	// TODO: Format data
	fmt::append(out, "{type='%s'}", id.type);
}
