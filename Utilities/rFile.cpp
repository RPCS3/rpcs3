#include "stdafx.h"
#include "Log.h"
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include "rFile.h"

#ifdef _WIN32
#include <Windows.h>

// Maybe in StrFmt?
std::wstring ConvertUTF8ToWString(const std::string &source) {
	int len = (int)source.size();
	int size = (int)MultiByteToWideChar(CP_UTF8, 0, source.c_str(), len, NULL, 0);
	std::wstring str;
	str.resize(size);
	if (size > 0) {
		MultiByteToWideChar(CP_UTF8, 0, source.c_str(), len, &str[0], size);
	}
	return str;
}
#endif

bool getFileInfo(const char *path, FileInfo *fileInfo) {
	// TODO: Expand relative paths?
	fileInfo->fullName = path;

#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA attrs;
	if (!GetFileAttributesExW(ConvertUTF8ToWString(path).c_str(), GetFileExInfoStandard, &attrs)) {
		fileInfo->size = 0;
		fileInfo->isDirectory = false;
		fileInfo->exists = false;
		return false;
	}
	fileInfo->size = (uint64_t)attrs.nFileSizeLow | ((uint64_t)attrs.nFileSizeHigh << 32);
	fileInfo->isDirectory = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	fileInfo->isWritable = (attrs.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	fileInfo->exists = true;
#else
	struct stat64 file_info;
	int result = stat64(path, &file_info);

	if (result < 0) {
		LOG_NOTICE(GENERAL, "IsDirectory: stat failed on %s", path);
		fileInfo->exists = false;
		return false;
	}

	fileInfo->isDirectory = S_ISDIR(file_info.st_mode);
	fileInfo->isWritable = false;
	fileInfo->size = file_info.st_size;
	fileInfo->exists = true;
	// HACK: approximation
	if (file_info.st_mode & 0200)
		fileInfo->isWritable = true;
#endif
	return true;
}

bool rIsDir(const std::string &filename) {
	FileInfo info;
	getFileInfo(filename.c_str(), &info);
	return info.isDirectory;
}

bool rMkdir(const std::string &dir)
{
#ifdef _WIN32
	return !_mkdir(dir.c_str());
#else
	return !mkdir(dir.c_str(), 0777);
#endif
}

bool rMkpath(const std::string &path)
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
		if((ret = _mkdir(dir.c_str())) && errno != EEXIST){
#else
		if((ret = mkdir(dir.c_str(), 0777)) && errno != EEXIST){
#endif
			return !ret;
		}
		if (pos >= path.length())
			return true;
	}
	return true;
}

bool rRmdir(const std::string &dir)
{
#ifdef _WIN32
	if (!RemoveDirectory(ConvertUTF8ToWString(dir).c_str())) {
		LOG_ERROR(GENERAL, "Error deleting directory %s: %i", dir.c_str(), GetLastError());
		return false;
	}
	return true;
#else
	rmdir(dir.c_str());
#endif
}

bool rRename(const std::string &from, const std::string &to)
{
	// TODO: Deal with case-sensitivity
#ifdef _WIN32
	return (MoveFile(ConvertUTF8ToWString(from).c_str(), ConvertUTF8ToWString(to).c_str()) == TRUE);
#else
	return (0 == rename(from.c_str(), to.c_str()));
#endif
}

bool rExists(const std::string &file)
{
#ifdef _WIN32
	std::wstring wstr = ConvertUTF8ToWString(file);
	return GetFileAttributes(wstr.c_str()) != 0xFFFFFFFF;
#else
	struct stat buffer;
	return (stat (file.c_str(), &buffer) == 0);
#endif
}

bool rRemoveFile(const std::string &file)
{
#ifdef _WIN32
	if (!DeleteFile(ConvertUTF8ToWString(file).c_str())) {
		LOG_ERROR(GENERAL, "Error deleting %s: %i", file.c_str(), GetLastError());
		return false;
	}
#else
	int err = unlink(file.c_str());
	if (err) {
		LOG_ERROR(GENERAL, "Error unlinking %s: %i", file.c_str(), err);
		return false;
	}
#endif
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
	return reinterpret_cast<wxFile*>(handle)->Write(reinterpret_cast<const void*>(text.c_str()),text.size());
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

