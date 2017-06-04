#include "file.h"
#include "exception.h"
#include <memory>
#include <string>

#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#include <Windows.h>
#define GET_API_ERROR static_cast<u64>(GetLastError())
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
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

namespace common
{
#ifdef _WIN32
	std::unique_ptr<wchar_t[]> to_wchar(const std::string& source)
	{
		const auto length = source.size() + 1; // size + null terminator

		const int size = source.size() < INT_MAX ? static_cast<int>(length) : throw EXCEPTION("Invalid source length (0x%llx)", source.size());

		std::unique_ptr<wchar_t[]> buffer(new wchar_t[length]); // allocate buffer assuming that length is the max possible size

		if (!MultiByteToWideChar(CP_UTF8, 0, source.c_str(), size, buffer.get(), size))
		{
			throw EXCEPTION("System error 0x%x", GetLastError());
		}

		return buffer;
	}

	void to_utf8(std::string& result, const wchar_t* source)
	{
		const int length = lstrlenW(source); // source length

		if (length == 0)
		{
			return result.clear();
		}

		const int size = WideCharToMultiByte(CP_UTF8, 0, source, length, NULL, 0, NULL, NULL); // output size

		if (size <= 0)
		{
			throw EXCEPTION("System error 0x%x", GetLastError());
		}

		result.resize(size);

		if (!WideCharToMultiByte(CP_UTF8, 0, source, length, &result.front(), size, NULL, NULL))
		{
			throw EXCEPTION("System error 0x%x", GetLastError());
		}
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
		const auto handle = CreateFileW(to_wchar(file).get(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

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
#endif

	thread_local fse fs::g_tls_error = fse::ok;

	bool fs::stat(const std::string& path, stat_t& info)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		WIN32_FILE_ATTRIBUTE_DATA attrs;
		if (!GetFileAttributesExW(to_wchar(path).get(), GetFileExInfoStandard, &attrs))
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
		struct stat file_info;
		if (stat(path.c_str(), &file_info) < 0)
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
		g_tls_error = fse::ok;

#ifdef _WIN32
		return GetFileAttributesW(to_wchar(path).get()) != 0xFFFFFFFF;
#else
		struct stat buffer;
		return stat(path.c_str(), &buffer) == 0;
#endif
	}

	bool fs::is_file(const std::string& file)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		DWORD attrs;
		if ((attrs = GetFileAttributesW(to_wchar(file).get())) == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}

		return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
		struct stat file_info;
		if (stat(file.c_str(), &file_info) < 0)
		{
			return false;
		}

		return !S_ISDIR(file_info.st_mode);
#endif
	}

	bool fs::is_dir(const std::string& dir)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		DWORD attrs;
		if ((attrs = GetFileAttributesW(to_wchar(dir).get())) == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}

		return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
		struct stat file_info;
		if (stat(dir.c_str(), &file_info) < 0)
		{
			return false;
		}

		return S_ISDIR(file_info.st_mode);
#endif
	}

	bool fs::create_dir(const std::string& dir)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		if (!CreateDirectoryW(to_wchar(dir).get(), NULL))
#else
		if (mkdir(dir.c_str(), 0777))
#endif
		{
			//LOG_WARNING(GENERAL, "Error creating directory '%s': 0x%llx", dir, GET_API_ERROR);
			return false;
		}

		return true;
	}

	bool fs::create_path(const std::string& path)
	{
		g_tls_error = fse::ok;

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
		g_tls_error = fse::ok;

#ifdef _WIN32
		if (!RemoveDirectoryW(to_wchar(dir).get()))
#else
		if (rmdir(dir.c_str()))
#endif
		{
			//LOG_WARNING(GENERAL, "Error deleting directory '%s': 0x%llx", dir, GET_API_ERROR);
			return false;
		}

		return true;
	}

	bool fs::rename(const std::string& from, const std::string& to)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		if (!MoveFileW(to_wchar(from).get(), to_wchar(to).get()))
#else
		if (rename(from.c_str(), to.c_str()))
#endif
		{
			//LOG_WARNING(GENERAL, "Error renaming '%s' to '%s': 0x%llx", from, to, GET_API_ERROR);
			return false;
		}

		return true;
	}

#ifndef _WIN32
	int OSCopyFile(const char* source, const char* destination, bool overwrite)
	{
		/* Source: http://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c */

		const int input = open(source, O_RDONLY);
		if (input == -1)
		{
			return -1;
		}

		const int output = open(destination, O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL), 0666);
		if (output == -1)
		{
			close(input);
			return -1;
		}

		//Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
		//fcopyfile works on FreeBSD and OS X 10.5+ 
		const int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
		//sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
		off_t bytesCopied = 0;
		struct stat fileinfo = { 0 };
		const int result = fstat(input, &fileinfo) == -1 || sendfile(output, input, &bytesCopied, fileinfo.st_size) == -1 ? -1 : 0;
#endif
		close(input);
		close(output);

		return result;
	}
#endif

	bool fs::copy_file(const std::string& from, const std::string& to, bool overwrite)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		if (!CopyFileW(to_wchar(from).get(), to_wchar(to).get(), !overwrite))
#else
		if (OSCopyFile(from.c_str(), to.c_str(), overwrite))
#endif
		{
			//LOG_WARNING(GENERAL, "Error copying '%s' to '%s': 0x%llx", from, to, GET_API_ERROR);
			return false;
		}

		return true;
	}

	bool fs::remove_file(const std::string& file)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		if (!DeleteFileW(to_wchar(file).get()))
#else
		if (unlink(file.c_str()))
#endif
		{
			//LOG_WARNING(GENERAL, "Error deleting file '%s': 0x%llx", file, GET_API_ERROR);
			return false;
		}

		return true;
	}

	bool fs::truncate_file(const std::string& file, u64 length)
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		if (!common::truncate_file(file, length))
#else
		if (common::truncate(file.c_str(), length))
#endif
		{
			//LOG_WARNING(GENERAL, "Error resizing file '%s' to 0x%llx: 0x%llx", file, length, GET_API_ERROR);
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
			::close(m_fd);
#endif
		}
	}

	bool fs::file::open(const std::string& filename, u32 mode)
	{
		this->close();

		g_tls_error = fse::ok;

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
		default:
		{
			//LOG_ERROR(GENERAL, "fs::file::open('%s') failed: neither fom::read nor fom::write specified (0x%x)", filename, mode);
			return false;
		}
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
		}

		if (!disp || (mode & ~(fom::read | fom::write | fom::append | fom::create | fom::trunc | fom::excl)))
		{
			//LOG_ERROR(GENERAL, "fs::file::open('%s') failed: unknown mode specified (0x%x)", filename, mode);
			return false;
		}

		m_fd = (std::intptr_t)CreateFileW(to_wchar(filename).get(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
#else
		int flags = 0;

		switch (mode & (fom::read | fom::write))
		{
		case fom::read: flags |= O_RDONLY; break;
		case fom::write: flags |= O_WRONLY; break;
		case fom::read | fom::write: flags |= O_RDWR; break;
		default:
		{
			LOG_ERROR(GENERAL, "fs::file::open('%s') failed: neither fom::read nor fom::write specified (0x%x)", filename, mode);
			return false;
		}
		}

		if (mode & fom::append) flags |= O_APPEND;
		if (mode & fom::create) flags |= O_CREAT;
		if (mode & fom::trunc) flags |= O_TRUNC;
		if (mode & fom::excl) flags |= O_EXCL;

		if (((mode & fom::excl) && !(mode & fom::create)) || (mode & ~(fom::read | fom::write | fom::append | fom::create | fom::trunc | fom::excl)))
		{
			LOG_ERROR(GENERAL, "fs::file::open('%s') failed: unknown mode specified (0x%x)", filename, mode);
			return false;
		}

		m_fd = ::open(filename.c_str(), flags, 0666);
#endif

		if (m_fd == null)
		{
			//LOG_WARNING(GENERAL, "fs::file::open('%s', 0x%x) failed: error 0x%llx", filename, mode, GET_API_ERROR);
			return false;
		}

		return true;
	}

	bool fs::file::trunc(u64 size) const
	{
		g_tls_error = fse::ok;

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
		return !::ftruncate(m_fd, size);
#endif
	}

	bool fs::file::stat(stat_t& info) const
	{
		g_tls_error = fse::ok;

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
		struct stat file_info;
		if (fstat(m_fd, &file_info) < 0)
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
		g_tls_error = fse::ok;

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
		g_tls_error = fse::ok;

		const int size = count <= INT_MAX ? static_cast<int>(count) : throw EXCEPTION("Invalid count (0x%llx)", count);

#ifdef _WIN32
		DWORD nread;
		if (!ReadFile((HANDLE)m_fd, buffer, size, &nread, NULL))
		{
			return -1;
		}

		return nread;
#else
		return ::read(m_fd, buffer, size);
#endif
	}

	u64 fs::file::write(const void* buffer, u64 count) const
	{
		g_tls_error = fse::ok;

		const int size = count <= INT_MAX ? static_cast<int>(count) : throw EXCEPTION("Invalid count (0x%llx)", count);

#ifdef _WIN32
		DWORD nwritten;
		if (!WriteFile((HANDLE)m_fd, buffer, size, &nwritten, NULL))
		{
			return -1;
		}

		return nwritten;
#else
		return ::write(m_fd, buffer, size);
#endif
	}

	const fs::file& fs::file::operator <<(const std::string& str) const
	{
		if (write(str.data(), str.size()) != str.size())
		{
			throw std::runtime_error("");
		}

		return *this;
	}

	u64 fs::file::seek(s64 offset, fsm seek_mode) const
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		LARGE_INTEGER pos;
		pos.QuadPart = offset;

		const DWORD mode =
			seek_mode == fsm::begin ? FILE_BEGIN :
			seek_mode == fsm::cur ? FILE_CURRENT :
			seek_mode == fsm::end ? FILE_END :
			throw EXCEPTION("Unknown seek_mode (0x%x)", seek_mode);

		if (!SetFilePointerEx((HANDLE)m_fd, pos, &pos, mode))
		{
			return -1;
		}

		return pos.QuadPart;
#else
		const int whence =
			seek_mode == fsm::begin ? SEEK_SET :
			seek_mode == fsm::cur ? SEEK_CUR :
			seek_mode == fsm::end ? SEEK_END :
			throw EXCEPTION("Unknown seek_mode (0x%x)", seek_mode);

		return ::lseek(m_fd, offset, whence);
#endif
	}

	u64 fs::file::size() const
	{
		g_tls_error = fse::ok;

#ifdef _WIN32
		LARGE_INTEGER size;
		if (!GetFileSizeEx((HANDLE)m_fd, &size))
		{
			return -1;
		}

		return size.QuadPart;
#else
		struct stat file_info;
		if (::fstat(m_fd, &file_info) < 0)
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

	void fs::file_ptr::reset(const file& f)
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

	void fs::file_ptr::reset()
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

		g_tls_error = fse::ok;

		if (!is_dir(dirname))
		{
			return false;
		}

		m_path.reset(new char[dirname.size() + 1]);
		std::memcpy(m_path.get(), dirname.c_str(), dirname.size() + 1);

#ifdef _WIN32
		m_dd = -1;
#else
		m_dd = (std::intptr_t)::opendir(m_path.get());
#endif

		return true;
	}

	bool fs::dir::close()
	{
		g_tls_error = fse::ok;

		if (!m_path)
		{
			return false;
		}

		m_path.reset();

#ifdef _WIN32
		CHECK_ASSERTION(m_dd == -1 || FindClose((HANDLE)m_dd));
#else
		CHECK_ASSERTION(!::closedir((DIR*)m_dd));
#endif

		return true;
	}

	bool fs::dir::read(std::string& name, stat_t& info)
	{
		g_tls_error = fse::ok;

		if (!m_path)
		{
			return false;
		}

#ifdef _WIN32
		WIN32_FIND_DATAW found;

		if (m_dd == -1)
		{
			m_dd = (std::intptr_t)FindFirstFileW(to_wchar(m_path.get() + std::string("/*")).get(), &found);

			if (m_dd == -1)
			{
				return false;
			}
		}
		else if (!FindNextFileW((HANDLE)m_dd, &found))
		{
			return false;
		}

		to_utf8(name, found.cFileName);

		info.is_directory = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		info.is_writable = (found.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
		info.size = ((u64)found.nFileSizeHigh << 32) | (u64)found.nFileSizeLow;
		info.atime = to_time_t(found.ftLastAccessTime);
		info.mtime = to_time_t(found.ftLastWriteTime);
		info.ctime = to_time_t(found.ftCreationTime);
#else
		const auto found = ::readdir((DIR*)m_dd);

		struct stat file_info;
		if (!found || ::fstatat(::dirfd((DIR*)m_dd), found->d_name, &file_info, 0) < 0)
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
}