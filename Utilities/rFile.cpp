#include "stdafx.h"
#include "Log.h"
#pragma warning(disable : 4996)
#include <wx/dir.h>
#include <wx/file.h>
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
	const auto handle = CreateFileW(ConvertUTF8ToWChar(file).get(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	LARGE_INTEGER distance;
	distance.QuadPart = length;

	if (!SetFilePointerEx(handle, distance, NULL, FILE_BEGIN))
	{
		CloseHandle(handle);
		return false;
	}

	if (!SetEndOfFile(handle))
	{
		CloseHandle(handle);
		return false;
	}

	return CloseHandle(handle);
}

#else
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
	if (truncate64(file.c_str()), length)
#endif
	{
		LOG_ERROR(GENERAL, "Error resizing file '%s' to 0x%llx: 0x%llx", file, length, GET_API_ERROR);
		return false;
	}

	return true;
}

wxFile::OpenMode convertOpenMode(rFile::OpenMode open)
{
	wxFile::OpenMode mode;
	switch (open)
	{
	case rFile::read:
		mode = wxFile::read;
		break;
	case rFile::write:
		mode = wxFile::write;
		break;
	case rFile::read_write:
		mode = wxFile::read_write;
		break;
	case rFile::write_append:
		mode = wxFile::write_append;
		break;
	case rFile::write_excl:
		mode = wxFile::write_excl;
		break;
	}
	return mode;
}

rFile::OpenMode rConvertOpenMode(wxFile::OpenMode open)
{
	rFile::OpenMode mode;
	switch (open)
	{
	case wxFile::read:
		mode = rFile::read;
		break;
	case wxFile::write:
		mode = rFile::write;
		break;
	case wxFile::read_write:
		mode = rFile::read_write;
		break;
	case wxFile::write_append:
		mode = rFile::write_append;
		break;
	case wxFile::write_excl:
		mode = rFile::write_excl;
		break;
	}
	return mode;
}

wxSeekMode convertSeekMode(rSeekMode mode)
{
	wxSeekMode ret;
	switch (mode)
	{
	case rFromStart:
		ret = wxFromStart;
		break;
	case rFromCurrent:
		ret = wxFromCurrent;
		break;
	case rFromEnd:
		ret = wxFromEnd;
		break;
	}
	return ret;
}

rSeekMode rConvertSeekMode(wxSeekMode mode)
{
	rSeekMode ret;
	switch (mode)
	{
	case wxFromStart:
		ret = rFromStart;
		break;
	case wxFromCurrent:
		ret = rFromCurrent;
		break;
	case wxFromEnd:
		ret = rFromEnd;
		break;
	}
	return ret;
}


rFile::rFile()
{
	handle = reinterpret_cast<void*>(new wxFile());
}

rFile::rFile(const std::string& filename, rFile::OpenMode open)
{
	handle = reinterpret_cast<void*>(new wxFile(fmt::FromUTF8(filename), convertOpenMode(open)));
}

rFile::rFile(int fd)
{
	handle = reinterpret_cast<void*>(new wxFile(fd));
}

rFile::~rFile()
{
	delete reinterpret_cast<wxFile*>(handle);
}

bool rFile::Access(const std::string &filename, rFile::OpenMode mode)
{
	return wxFile::Access(fmt::FromUTF8(filename), convertOpenMode(mode));
}

size_t rFile::Write(const void *buffer, size_t count)
{
	return reinterpret_cast<wxFile*>(handle)->Write(buffer,count);
}

bool rFile::Write(const std::string &text)
{
	return reinterpret_cast<wxFile*>(handle)->Write(reinterpret_cast<const void*>(text.c_str()),text.size()) != 0;
}

bool rFile::Close()
{
	return reinterpret_cast<wxFile*>(handle)->Close();
}

bool rFile::Create(const std::string &filename, bool overwrite, int access)
{
	return reinterpret_cast<wxFile*>(handle)->Create(fmt::FromUTF8(filename), overwrite, access);
}

bool rFile::Open(const std::string &filename, rFile::OpenMode mode, int access)
{
	return reinterpret_cast<wxFile*>(handle)->Open(fmt::FromUTF8(filename), convertOpenMode(mode), access);
}

bool rFile::IsOpened() const
{
	return reinterpret_cast<wxFile*>(handle)->IsOpened();
}

size_t	rFile::Length() const
{
	return reinterpret_cast<wxFile*>(handle)->Length();
}

size_t  rFile::Read(void *buffer, size_t count)
{
	return reinterpret_cast<wxFile*>(handle)->Read(buffer,count);
}

size_t 	rFile::Seek(size_t ofs, rSeekMode mode)
{
	return reinterpret_cast<wxFile*>(handle)->Seek(ofs, convertSeekMode(mode));
}

size_t rFile::Tell() const
{
	return reinterpret_cast<wxFile*>(handle)->Tell();
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
