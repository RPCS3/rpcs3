/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
 
#include "PrecompiledHeader.h"
#include "IsoFileTools.h"

#ifdef _WIN32
#	include <wx/msw/wrapwin.h>


void *_openfile(const char *filename, int flags)
{
	HANDLE handle;

//	Console.WriteLn("_openfile %s, %d", filename, flags & O_RDONLY);
	if (flags & O_WRONLY)
	{
		int _flags = CREATE_NEW;
		if (flags & O_CREAT) _flags = CREATE_ALWAYS;
		handle = CreateFile(fromUTF8(filename), GENERIC_WRITE, 0, NULL, _flags, 0, NULL);
	}
	else
	{
		handle = CreateFile(fromUTF8(filename), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	}

	return (handle == INVALID_HANDLE_VALUE) ? NULL : handle;
}

u64 _tellfile(void *handle)
{
	u64 ofs;
	PLONG _ofs = (LONG*) & ofs;
	_ofs[1] = 0;
	_ofs[0] = SetFilePointer(handle, 0, &_ofs[1], FILE_CURRENT);
	return ofs;
}

int _seekfile(void *handle, u64 offset, int whence)
{
	u64 ofs = (u64)offset;
	PLONG _ofs = (LONG*) & ofs;

//	Console.WriteLn("_seekfile %p, %d_%d", handle, _ofs[1], _ofs[0]);

	SetFilePointer(handle, _ofs[0], &_ofs[1], (whence == SEEK_SET) ? FILE_BEGIN : FILE_END);

	return 0;
}

int _readfile(void *handle, void *dst, int size)
{
	DWORD ret;

	ReadFile(handle, dst, size, &ret, NULL);
//	Console.WriteLn("_readfile(%p, %d) = %d; %d", handle, size, ret, GetLastError());
	return ret;
}

int _writefile(void *handle, const void *src, int size)
{
	DWORD ret;

//	_seekfile(handle, _tellfile(handle));
	WriteFile(handle, src, size, &ret, NULL);
//	Console.WriteLn("_readfile(%p, %d) = %d", handle, size, ret);
	return ret;
}

void _closefile(void *handle)
{
	CloseHandle(handle);
}

#else

void *_openfile(const char *filename, int flags)
{
//	Console.WriteLn("_openfile %s %x", filename, flags);

	if (flags & O_WRONLY)
		return fopen64(filename, "wb");
	else
		return fopen64(filename, "rb");
}

u64 _tellfile(void *handle)
{
	FILE* fp = (FILE*)handle;
	s64 cursize = ftell(fp);

	if (cursize == -1)
	{
		// try 64bit
		cursize = ftello64(fp);
		if (cursize < -1)
		{
			// zero top 32 bits
			cursize &= 0xffffffff;
		}
	}
	return cursize;
}

int _seekfile(void *handle, u64 offset, int whence)
{
	int seekerr = fseeko64((FILE*)handle, offset, whence);

	if (seekerr == -1) Console.Error("Failed to seek.");

	return seekerr;
}

int _readfile(void *handle, void *dst, int size)
{
	return fread(dst, 1, size, (FILE*)handle);
}

int _writefile(void *handle, const void *src, int size)
{
	return fwrite(src, 1, size, (FILE*)handle);
}

void _closefile(void *handle)
{
	fclose((FILE*)handle);
}

#endif
