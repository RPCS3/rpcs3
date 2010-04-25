// stdafx.cpp : source file that includes just the standard includes
// xpad.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

string format(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	int result = -1, length = 256;

	char* buffer = NULL;

	while(result == -1)
	{
		if(buffer) delete [] buffer;

		buffer = new char[length + 1];

		memset(buffer, 0, length + 1);

		result = _vsnprintf(buffer, length, fmt, args);

		length *= 2;
	}

	va_end(args);

	string s(buffer);

	delete [] buffer;

	return s;
}
