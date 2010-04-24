/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * Copyright (C) 2004 adresd (adresd_ps2dev@yahoo.com)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#ifndef _NETFIO_H_
#define _NETFIO_H_

int pcsx2fio_device_exists();

int pcsx2fio_set_call(unsigned int func_id, void* params, int params_length);

int pcsx2fio_file_serv(void *arg);
int pcsx2fio_recv_bytes(int fd, char *buf, int bytes);
int pcsx2fio_accept_pkt(int fd, char *buf, int len, int pkt_type);
int pcsx2fio_open_file(char *path, int flags);
int pcsx2fio_close_file(int fd);
int pcsx2fio_read_file(int fd, char *buf, int length);
int pcsx2fio_write_file(int fd, char *buf, int length);
int pcsx2fio_lseek_file(int fd, unsigned int offset, int whence);
void pcsx2fio_close_socket(void);
void pcsx2fio_close_fsys(void);
int pcsx2fio_remove(char *name);
int pcsx2fio_mkdir(char *name, int mode);
int pcsx2fio_rmdir(char *name);
int pcsx2fio_open_dir(char *path);
int pcsx2fio_read_dir(int fd, void *buf);
int pcsx2fio_close_dir(int fd);

#endif
