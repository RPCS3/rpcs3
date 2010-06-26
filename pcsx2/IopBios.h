/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#ifndef __PSXBIOS_H__
#define __PSXBIOS_H__

#define IOP_ENOENT	2
#define IOP_EIO		5
#define IOP_ENOMEM	12
#define IOP_EACCES	13
#define IOP_EISDIR	21
#define IOP_EMFILE	24
#define IOP_EROFS	30

#define IOP_O_RDONLY	0x001
#define IOP_O_WRONLY	0x002
#define IOP_O_RDWR		0x003
#define IOP_O_APPEND	0x100
#define IOP_O_CREAT		0x200
#define IOP_O_TRUNC		0x400
#define IOP_O_EXCL		0x800

#define IOP_SEEK_SET 0
#define IOP_SEEK_CUR 1
#define IOP_SEEK_END 2

class IOManFile {
public:
	// int open(IOManFile **file, char *name, s32 flags, u16 mode);

	virtual void close() = 0;

	virtual int lseek(s32 offset, s32 whence) { return -IOP_EIO; }
	virtual int read(void *buf, u32 count) { return -IOP_EIO; }
	virtual int write(void *buf, u32 count) { return -IOP_EIO; }
};

class IOManDir {
	// Don't think about it until we know the loaded ioman version.
	// The dirent structure changed between versions.
public:
	virtual void close();
};

typedef int (*irxHLE)(); // return 1 if handled, otherwise 0
typedef void (*irxDEBUG)();

namespace R3000A
{
	const char* irxImportLibname(u32 entrypc);
	const char* irxImportFuncname(const char libname[8], u16 index);
	irxHLE irxImportHLE(const char libname[8], u16 index);
	irxDEBUG irxImportDebug(const char libname[8], u16 index);
	void __fastcall irxImportLog(const char libname[8], u16 index, const char *funcname);
	int __fastcall irxImportExec(const char libname[8], u16 index);

	namespace ioman
	{
		void reset();
	}
}

extern void Hle_SetElfPath(const char* elfFileName);

#endif /* __PSXBIOS_H__ */
