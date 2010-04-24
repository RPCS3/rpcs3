/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * Copyright (C) 2004 adresd (adresd_ps2dev@yahoo.com)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

// Fu*k knows why net_fio & net_fsys are separated..

#include <types.h>
#include <ioman.h>
#include <sysclib.h>
#include <stdio.h>
#include <thbase.h>

#include <io_common.h>

#include "net_fio.h"
#include "hostlink.h"

#define PACKET_MAXSIZE 1024
static int send_packet[PACKET_MAXSIZE] __attribute__((aligned(16)));

#ifdef DEBUG
#define dbgprintf(args...) printf(args)
#else
#define dbgprintf(args...) do { } while(0)
#endif

//---------------------------------------------------------------------- 
//
#define PCSX2_FIO_BASE ((volatile unsigned int*)0x1d000800)
#define PCSX2_FIO_RDID (PCSX2_FIO_BASE + 0) // [f0] on read: returns the FIO magic number 'E2SP'
#define PCSX2_FIO_WCMD (PCSX2_FIO_BASE + 0) // [f0] on write: executes a command (usually "call function")
#define PCSX2_FIO_DLEN (PCSX2_FIO_BASE + 1) // [f4] write only: length of params data
#define PCSX2_FIO_DPTR (PCSX2_FIO_BASE + 2) // [f8] write only: mem address of params data
#define PCSX2_FIO_WFID (PCSX2_FIO_BASE + 3) // [fc] on write: function ID
#define PCSX2_FIO_RRET (PCSX2_FIO_BASE + 3) // [fc] on read: return value of the function

#define PCSX2_FIO_CMD_CALL 1

int pcsx2fio_device_exists()
{
	return (*PCSX2_FIO_RDID == 'E2SP'); // PS2E
}

int pcsx2fio_set_call(unsigned int func_id, void* params, int params_length)
{
	*PCSX2_FIO_DLEN = params_length;
	*PCSX2_FIO_DPTR = (int)params;
	*PCSX2_FIO_WFID = func_id;
	*PCSX2_FIO_WCMD = PCSX2_FIO_CMD_CALL;
	return *PCSX2_FIO_RRET;
}

char *strcpy(char *dest, const char *src)
{
	while(*src)
	{
		*(dest++) = *(src++);
	}
	*(dest) = 0;
	return dest;
}

//---------------------------------------------------------------------- 
//
void
pcsx2fio_close_fsys(void)
{
}

//----------------------------------------------------------------------
//
int pcsx2fio_open_file(char *path, int flags)
{
	send_packet[0] = flags;
	strcpy((char*)(send_packet+1),path);

	int length = strlen(path) + 4;

    int ret = pcsx2fio_set_call(PS2E_FIO_OPEN_CMD, send_packet, length);

    return ret;
}


//----------------------------------------------------------------------
//
int pcsx2fio_close_file(int fd)
{
	send_packet[0] = fd;

	int length = 4;

	int ret = pcsx2fio_set_call(PS2E_FIO_CLOSE_CMD, send_packet, length);

	return ret;
}

//----------------------------------------------------------------------
//
int pcsx2fio_lseek_file(int fd, unsigned int offset, int whence)
{
	send_packet[0] = fd;
	send_packet[1] = offset;
	send_packet[2] = whence;
	send_packet[3] = 0;

	int length = 12;

	int ret = pcsx2fio_set_call(PS2E_FIO_LSEEK_CMD, send_packet, length);

	return ret;
}


//----------------------------------------------------------------------
//
int pcsx2fio_write_file(int fd, char *buf, int _length)
{
	send_packet[0] = fd;
	send_packet[1] = (int)buf;
	send_packet[2] = _length;
	send_packet[3] = 0;

	int length = 12;

	int ret = pcsx2fio_set_call(PS2E_FIO_WRITE_CMD, send_packet, length);

	return ret;
}

//----------------------------------------------------------------------
//
int pcsx2fio_read_file(int fd, char *buf, int _length)
{
	send_packet[0] = fd;
	send_packet[1] = (int)buf;
	send_packet[2] = _length;
	send_packet[3] = 0;

	int length = 12;

	int ret = pcsx2fio_set_call(PS2E_FIO_READ_CMD, send_packet, length);

	return ret;
}

//----------------------------------------------------------------------
//
int pcsx2fio_remove(char *name)
{
	strcpy((char*)(send_packet+0),name);

	int length = strlen(name);

	int ret = pcsx2fio_set_call(PS2E_FIO_REMOVE_CMD, send_packet, length);

	return ret;
}

//----------------------------------------------------------------------
//
int pcsx2fio_mkdir(char *name, int mode)
{
	send_packet[0] = mode;
	strcpy((char*)(send_packet+1),name);

	int length = strlen(name) + 4;

	int ret = pcsx2fio_set_call(PS2E_FIO_MKDIR_CMD, send_packet, length);

	return ret;
}

//----------------------------------------------------------------------
//
int pcsx2fio_rmdir(char *name)
{
	strcpy((char*)(send_packet+0),name);

	int length = strlen(name);

	int ret = pcsx2fio_set_call(PS2E_FIO_RMDIR_CMD, send_packet, length);

	return ret;
}

//----------------------------------------------------------------------
//
int pcsx2fio_open_dir(char *path)
{
	strcpy((char*)(send_packet+0),path);

	int length = strlen(path);

	int ret = pcsx2fio_set_call(PS2E_FIO_OPENDIR_CMD, send_packet, length);

	return ret;
}

//----------------------------------------------------------------------
//
int pcsx2fio_read_dir(int fd, void *buf)
{
	send_packet[0] = fd;
	send_packet[1] = (int)buf;

	int length = 8;

	int ret = pcsx2fio_set_call(PS2E_FIO_READDIR_CMD, send_packet, length);

	return ret;
}


//----------------------------------------------------------------------
//
int pcsx2fio_close_dir(int fd)
{
	send_packet[0] = fd;

	int length = 4;

	int ret = pcsx2fio_set_call(PS2E_FIO_CLOSEDIR_CMD, send_packet, length);

	return ret;
}
