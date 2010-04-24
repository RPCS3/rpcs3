/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#include <types.h>
#include <thbase.h>
#include <thsemap.h>
#include <sysclib.h>
#include <stdio.h>
#include <ioman.h>
#include <intrman.h>
#include <loadcore.h>

#include "net_fio.h"
#include "hostlink.h"

////////////////////////////////////////////////////////////////////////

static int tty_socket = 0;
static int tty_sema = -1;

static char ttyname[] = "tty";

////////////////////////////////////////////////////////////////////////
static int dummy()
{
    return -5;
}

////////////////////////////////////////////////////////////////////////
static int dummy0()
{
    return 0;
}

////////////////////////////////////////////////////////////////////////
static int ttyInit(iop_device_t *driver)
{
    iop_sema_t sema_info;

    sema_info.attr       = 0;
    sema_info.initial = 1;	/* Unlocked.  */
    sema_info.max  = 1;
    if ((tty_sema = CreateSema(&sema_info)) < 0)
	    return -1;


	if(!pcsx2fio_device_exists())
		return -1;

	return 1;
}

////////////////////////////////////////////////////////////////////////
static int ttyOpen( int fd, char *name, int mode)
{
    return 1;
}

////////////////////////////////////////////////////////////////////////
static int ttyClose( int fd)
{
    return 1;
}


////////////////////////////////////////////////////////////////////////
int ttyWrite(iop_file_t *file, char *buf, int size)
{
    int res;

    WaitSema(tty_sema);

	res = pcsx2fio_set_call(PS2E_FIO_PRINTF_CMD,buf,size);

    SignalSema(tty_sema);
    return res;
}

iop_device_ops_t tty_functarray = { ttyInit, dummy0, (void *)dummy,
	(void *)ttyOpen, (void *)ttyClose, (void *)dummy,
	(void *)ttyWrite, (void *)dummy, (void *)dummy,
	(void *)dummy, (void *)dummy, (void *)dummy,
	(void *)dummy, (void *)dummy, (void *)dummy,
	(void *)dummy, (void *)dummy };

iop_device_t tty_driver = { ttyname, 3, 1, "Fast TTY for pcsx2",
							&tty_functarray };

////////////////////////////////////////////////////////////////////////
// Entry point for mounting the file system
int ttyMount(void)
{
    close(0);
    close(1);
    DelDrv(ttyname);
    AddDrv(&tty_driver);
    if(open("tty00:", O_RDONLY) != 0) while(1);
    if(open("tty00:", O_WRONLY) != 1) while(1);

    return 0;
}
