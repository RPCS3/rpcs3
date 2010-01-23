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
#include "IopCommon.h"
#include "Utilities/Console.h"

#ifndef __LINUX__
#include <io.h>
#endif

#pragma optimize("", off)

#define PS2E_FIO_OPEN_CMD     0xcc2e0101
#define PS2E_FIO_CLOSE_CMD    0xcc2e0102
#define PS2E_FIO_READ_CMD     0xcc2e0103
#define PS2E_FIO_WRITE_CMD    0xcc2e0104
#define PS2E_FIO_LSEEK_CMD    0xcc2e0105
#define PS2E_FIO_OPENDIR_CMD  0xcc2e0106
#define PS2E_FIO_CLOSEDIR_CMD 0xcc2e0107
#define PS2E_FIO_READDIR_CMD  0xcc2e0108
#define PS2E_FIO_REMOVE_CMD   0xcc2e0109
#define PS2E_FIO_MKDIR_CMD    0xcc2e010a
#define PS2E_FIO_RMDIR_CMD    0xcc2e010b

#define PS2E_FIO_PRINTF_CMD   0xcc2e0201

#define PS2E_FIO_MAGIC 'E2SP'

u32 functionId;
u32 paramsAddress;
u32 paramsLength;
u32 returnValue;

#define IOP_RDONLY	0x0001
#define IOP_WRONLY	0x0002
#define IOP_RDWR	0x0003
#define IOP_NBLOCK	0x0010
#define IOP_APPEND	0x0100
#define IOP_CREAT	0x0200
#define IOP_TRUNC	0x0400
#define IOP_NOWAIT	0x8000

int pcsx2fio_open_file(char *path, int flags)
{
	path++;
	int mode = 0;

	switch(flags&IOP_RDWR)
	{
	case IOP_RDONLY:
		mode = O_RDONLY;
		break;
	case IOP_WRONLY:
		mode = O_WRONLY;
		break;
	case IOP_RDWR:
		mode = O_RDWR;
		break;
	}
	if(flags&IOP_CREAT)
		mode |= O_CREAT;
	if(flags&IOP_APPEND)
		mode |= O_APPEND;
	if(flags&IOP_TRUNC)
		mode |= O_TRUNC;

	return open(path,mode);
}

int pcsx2fio_close_file(int fd)
{
	return close(fd);
}

int pcsx2fio_lseek_file(int fd, unsigned int offset, int whence)
{
	return lseek(fd,offset,whence);
}

int pcsx2fio_write_file(int fd, char *buf, int length)
{
	return write(fd,buf,length);
}

int pcsx2fio_read_file(int fd, char *buf, int length)
{
	return read(fd,buf,length);
}

int pcsx2fio_remove(char *name)
{
	return unlink(name);
}

int pcsx2fio_mkdir(char *name, int mode)
{
#ifdef __LINUX__
	return mkdir(name,mode);
#else
	return mkdir(name);
#endif
}

int pcsx2fio_rmdir(char *name)
{
	return rmdir(name);
}

int pcsx2fio_open_dir(char *path)
{
	return -1;
}

int pcsx2fio_read_dir(int fd, void *buf)
{
	return -1;
}

int pcsx2fio_close_dir(int fd)
{
	return -1;
}

int pcsx2fio_write_tty(const char* text, int length)
{
	wxString s = wxString::FromUTF8(text,length);

	return printf("%s",s.ToAscii().data());
}

#define PARAM(offset,type) (*(type*)(buffer+(offset)))
#define PARAMP(offset,type) (type*)iopPhysMem(PARAM(offset,u32))
#define PARAMSTR(offset) ((char*)(buffer+(offset)))
void Pcsx2HostFsExec()
{
	u8* buffer = (u8*)iopPhysMem(paramsAddress);

	switch(functionId)
	{
	case PS2E_FIO_OPEN_CMD:		returnValue = pcsx2fio_open_file(PARAMSTR(4),PARAM(0,s32)); break;
	case PS2E_FIO_CLOSE_CMD:	returnValue = pcsx2fio_close_file(PARAM(0,s32)); break;
	case PS2E_FIO_READ_CMD:		returnValue = pcsx2fio_read_file(PARAM(0,s32),PARAMP(4,char),PARAM(8,s32)); break;
	case PS2E_FIO_WRITE_CMD:	returnValue = pcsx2fio_write_file(PARAM(0,s32),PARAMP(4,char),PARAM(8,s32)); break;
	case PS2E_FIO_LSEEK_CMD:	returnValue = pcsx2fio_lseek_file(PARAM(0,s32),PARAM(4,s32),PARAM(8,s32)); break;
	case PS2E_FIO_REMOVE_CMD:	returnValue = pcsx2fio_remove(PARAMSTR(0)); break;
	case PS2E_FIO_OPENDIR_CMD:  returnValue = pcsx2fio_open_dir(PARAMSTR(0)); break;
	case PS2E_FIO_CLOSEDIR_CMD: returnValue = pcsx2fio_close_dir(PARAM(0,s32)); break;
	case PS2E_FIO_READDIR_CMD:  returnValue = pcsx2fio_read_dir(PARAM(0,s32),PARAMP(4,void)); break;
	case PS2E_FIO_MKDIR_CMD:	returnValue = pcsx2fio_mkdir(PARAMSTR(4),PARAM(0,s32)); break;
	case PS2E_FIO_RMDIR_CMD:	returnValue = pcsx2fio_rmdir(PARAMSTR(0)); break;

	case PS2E_FIO_PRINTF_CMD:	returnValue = pcsx2fio_write_tty(PARAMSTR(0),paramsLength);	break;
	}
}

void Pcsx2HostFSwrite32(u32 addr, u32 value)
{
	switch(addr&0xF)
	{
	case 0:
		if(value==1)
			Pcsx2HostFsExec();
		break;
	case 4:
		paramsLength = value;
		break;
	case 8:
		paramsAddress = value;
		break;
	case 12:
		functionId = value;
		break;
	}
}

u32 Pcsx2HostFSread32(u32 addr)
{
	switch(addr&0xF)
	{
	case 0:
		return PS2E_FIO_MAGIC;
		break;
	case 4:
		break;
	case 8:
		break;
	case 12:
		return returnValue;
		break;
	}
	return 0;
}
