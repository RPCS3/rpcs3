/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

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
