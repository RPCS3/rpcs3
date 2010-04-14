#pragma once

#include <stdio.h>
#include <string>

typedef unsigned char byte;

class FileStream
{
	int  internalRead(byte* b, int off, int len);

	FILE* handle;
	s64 fileSize;

	FileStream(FileStream&);
public:

	FileStream(const char* fileName);

	virtual void seek(s64 offset);
	virtual void seek(s64 offset, int ref_position);
	virtual void reset();

	virtual s64 skip(s64 n);
	virtual s64 getFilePointer();

	virtual bool eof();
	virtual int  read();
	virtual int  read(byte* b, int len);

	// Tool to read a specific value type, including structs.
	template<class T>
	T read()
	{
		if(sizeof(T)==1)
			return (T)read();
		else
		{
			T t;
			read((byte*)&t,sizeof(t));
			return t;
		}
	}

	virtual std::string readLine();   // Assume null-termination
	virtual std::wstring readLineW(); // (this one too)

	virtual s64 getLength();

	virtual ~FileStream(void);
};
