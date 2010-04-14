#include <sys/stat.h>
#include "CDVD.h"
#include "FileStream.h"

using namespace std;

FileStream::FileStream(const char* fileName)
{
	struct __stat64 stat_info;
	_stat64(fileName, &stat_info);
	fileSize = stat_info.st_size;

	handle = fopen(fileName, "rb");
}

void FileStream::seek(s64 offset)
{
	seek(offset,SEEK_SET);
}

void FileStream::seek(s64 offset, int ref_position)
{
	int ret = _fseeki64(handle, offset, SEEK_SET);
#ifdef __LINUX__
	if (ret) throw "Seek offset out of bounds.";
#else
	if (ret)
		throw new exception("Seek offset out of bounds.");
#endif
}

void FileStream::reset()
{
	seek(0);
}

s64 FileStream::skip(s64 n)
{
	s64 before = getFilePointer();
	seek(n,SEEK_CUR);

	return getFilePointer() - before;
}

s64 FileStream::getFilePointer()
{
	return _ftelli64(handle);
}

bool FileStream::eof()
{
	return feof(handle)!=0;
}

int FileStream::read()
{
	return fgetc(handle);
}

int FileStream::read(byte* b, int len)
{
	if (b == NULL)
	{
#ifdef __LINUX__
		throw "NULL buffer passed.";
#else
		throw new exception("NULL buffer passed.");
#endif
	}

	if (len < 0)
	{
#ifdef __LINUX__
		throw "off<0 or len<0.";
#else
		throw new exception("off<0 or len<0.");
#endif
	}

	return fread(b,1,len,handle);
}

string FileStream::readLine()
{
	string s;
	char c;

	s.clear();
	do {
		if(eof())
			break;

		c = read();

		if((c=='\n')||(c=='\r')||(c==0))
			break;

		s.append(1,c);
	} while(true);

	return s;
}

wstring FileStream::readLineW()
{
	wstring s;
	wchar_t c;

	s.clear();
	do {
		if(eof())
			break;

		c = read<wchar_t>();

		if((c==L'\n')||(c==L'\r')||(c==0))
			break;

		s.append(1,c);
	} while(true);

	return s;
}

s64 FileStream::getLength()
{
	return fileSize;
}

FileStream::~FileStream(void)
{
	fclose(handle);
}
