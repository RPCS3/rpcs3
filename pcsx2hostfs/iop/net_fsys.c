/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * Copyright (C) 2004 adresd (adresd_ps2dev@yahoo.com)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#include <types.h>
#include <thbase.h>
#include <ioman.h>
#include <sysclib.h>
#include <stdio.h>
#include <intrman.h>
#include <loadcore.h>
#include <thsemap.h>

#include "net_fio.h"

#ifdef DEBUG
#define dbgprintf(args...) printf(args)
#else
#define dbgprintf(args...) do { } while(0)
#endif

static char fsname[] = "host";

////////////////////////////////////////////////////////////////////////
//static iop_device_t fsys_driver;

/* File desc struct is probably larger than this, but we only
 * need to access the word @ offset 0x0C (in which we put our identifier)
 */
struct filedesc_info
{
    int unkn0;
    int unkn4;
    int device_id;   // the X in hostX
    int own_fd;
};

////////////////////////////////////////////////////////////////////////
/* We need(?) to protect the net access, so the server doesn't get
 * confused if two processes calls a fsys func at the same time
 */
static int fsys_sema;
static int fsys_pid = 0;
//static iop_device_t fsys_driver;

////////////////////////////////////////////////////////////////////////
static int dummy5()
{
    printf("dummy function called\n");
    return -5;
}

////////////////////////////////////////////////////////////////////////
static void fsysInit(iop_device_t *driver)
{

}

////////////////////////////////////////////////////////////////////////
static int fsysDestroy(void)
{
    WaitSema(fsys_sema);
    pcsx2fio_close_fsys();
    //    ExitDeleteThread(fsys_pid);
    SignalSema(fsys_sema);
    DeleteSema(fsys_sema);
    return 0;
}


////////////////////////////////////////////////////////////////////////
static int fsysOpen( int fd, char *name, int mode)
{
    struct filedesc_info *fd_info;
    int fsys_fd;
    
    dbgprintf("fsysOpen..\n");
    dbgprintf("  fd: %x, name: %s, mode: %d\n\n", fd, name, mode);

    fd_info = (struct filedesc_info *)fd;

    WaitSema(fsys_sema);
    fsys_fd = pcsx2fio_open_file(name, mode);
    SignalSema(fsys_sema);
    fd_info->own_fd = fsys_fd;

    return fsys_fd;
}



////////////////////////////////////////////////////////////////////////
static int fsysClose( int fd)
{
    struct filedesc_info *fd_info;
    int ret;
    
    dbgprintf("fsys_close..\n"
           "  fd: %x\n\n", fd);
    
    fd_info = (struct filedesc_info *)fd;
    WaitSema(fsys_sema);
    ret = pcsx2fio_close_file(fd_info->own_fd);    
    SignalSema(fsys_sema);

    return ret;
}



////////////////////////////////////////////////////////////////////////
static int fsysRead( int fd, char *buf, int size)
{
    struct filedesc_info *fd_info;
    int ret;

    fd_info = (struct filedesc_info *)fd;

    dbgprintf("fsysRead..."
           "  fd: %x\n"
           "  bf: %x\n"
           "  sz: %d\n"
           "  ow: %d\n\n", fd, (int)buf, size, fd_info->own_fd);

    WaitSema(fsys_sema);
    ret = pcsx2fio_read_file(fd_info->own_fd, buf, size);
    SignalSema(fsys_sema);

    return ret;
}




////////////////////////////////////////////////////////////////////////
static int fsysWrite( int fd, char *buf, int size)
{
    struct filedesc_info *fd_info;
    int ret;
    
    dbgprintf("fsysWrite..."
           "  fd: %x\n", fd);

    fd_info = (struct filedesc_info *)fd;
    WaitSema(fsys_sema);
    ret = pcsx2fio_write_file(fd_info->own_fd, buf, size);
    SignalSema(fsys_sema);
    return ret;
}



////////////////////////////////////////////////////////////////////////
static int fsysLseek( int fd, unsigned int offset, int whence)
{
    struct filedesc_info *fd_info;
    int ret;

    dbgprintf("fsysLseek..\n"
           "  fd: %x\n"
           "  of: %x\n"
           "  wh: %x\n\n", fd, offset, whence);

    fd_info = (struct filedesc_info *)fd;
    WaitSema(fsys_sema);
    ret = pcsx2fio_lseek_file(fd_info->own_fd, offset, whence);
    SignalSema(fsys_sema);
    return ret;
}

////////////////////////////////////////////////////////////////////////
static int fsysRemove(iop_file_t* file, char *name)
{
    int ret;
    dbgprintf("fsysRemove..\n");
    dbgprintf("  name: %s\n\n", name);

    WaitSema(fsys_sema);
    ret = pcsx2fio_remove(name);
    SignalSema(fsys_sema);

    return ret;
}

////////////////////////////////////////////////////////////////////////
static int fsysMkdir(iop_file_t* file, char *name, int mode)
{
    int ret;
    dbgprintf("fsysMkdir..\n");
    dbgprintf("  name: '%s'\n\n", name);

    WaitSema(fsys_sema);
    ret = pcsx2fio_mkdir(name, mode);
    SignalSema(fsys_sema);

    return ret;
}

////////////////////////////////////////////////////////////////////////
static int fsysRmdir(iop_file_t* file, char *name)
{
    int ret;
    dbgprintf("fsysRmdir..\n");
    dbgprintf("  name: %s\n\n", name);

    WaitSema(fsys_sema);
    ret = pcsx2fio_rmdir(name);
    SignalSema(fsys_sema);

    return ret;
}


////////////////////////////////////////////////////////////////////////
static int fsysDopen(int fd, char *name)
{
    struct filedesc_info *fd_info;
    int fsys_fd;
    
    dbgprintf("fsysDopen..\n");
    dbgprintf("  fd: %x, name: %s\n\n", fd, name);

    fd_info = (struct filedesc_info *)fd;

    WaitSema(fsys_sema);
    fsys_fd = pcsx2fio_open_dir(name);
    SignalSema(fsys_sema);
    fd_info->own_fd = fsys_fd;

    return fsys_fd;
}

////////////////////////////////////////////////////////////////////////
static int fsysDread(int fd, void *buf)
{
    struct filedesc_info *fd_info;
    int ret;

    fd_info = (struct filedesc_info *)fd;

    dbgprintf("fsysDread..."
           "  fd: %x\n"
           "  bf: %x\n"
           "  ow: %d\n\n", fd, (int)buf, fd_info->own_fd);

    WaitSema(fsys_sema);
    ret = pcsx2fio_read_dir(fd_info->own_fd, buf);
    SignalSema(fsys_sema);

    return ret;

}

////////////////////////////////////////////////////////////////////////
static int fsysDclose(int fd)
{
    struct filedesc_info *fd_info;
    int ret;
    
    dbgprintf("fsys_dclose..\n"
           "  fd: %x\n\n", fd);
    
    fd_info = (struct filedesc_info *)fd;
    WaitSema(fsys_sema);
    ret = pcsx2fio_close_dir(fd_info->own_fd);    
    SignalSema(fsys_sema);

    return ret;
}

iop_device_ops_t fsys_functarray = { (void *)fsysInit, (void *)fsysDestroy, (void *)dummy5, 
	(void *)fsysOpen, (void *)fsysClose, (void *)fsysRead, 
	(void *)fsysWrite, (void *)fsysLseek, (void *)dummy5,
	(void *)fsysRemove, (void *)fsysMkdir, (void *)fsysRmdir,
	(void *)fsysDopen, (void *)fsysDclose, (void *)fsysDread,
	(void *)dummy5,  (void *)dummy5 };

iop_device_t fsys_driver = { fsname, 16, 1, "fsys driver", 
							&fsys_functarray };
////////////////////////////////////////////////////////////////////////
// Entry point for mounting the file system
int fsysMount(void)
{
    iop_sema_t sema_info;

    sema_info.attr = 1;
    sema_info.option = 0;
    sema_info.initial = 1;
    sema_info.max = 1;
    fsys_sema = CreateSema(&sema_info);

    DelDrv(fsname);
    AddDrv(&fsys_driver);

    return 0;
}

int fsysUnmount(void)
{
    DelDrv(fsname);
    return 0;
}
