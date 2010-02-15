/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

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

#define PS2E_FIO_MAX_PATH   256
#define PKO_MAX_PATH   256

// old stuff

typedef struct
{
    unsigned int cmd;
    unsigned short len;
} __attribute__((packed)) pko_pkt_hdr;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    unsigned int retval;
} __attribute__((packed)) pko_pkt_file_rly;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int flags;
    char path[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_open_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
} __attribute__((packed)) pko_pkt_close_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
    int nbytes;
} __attribute__((packed)) pko_pkt_read_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int retval;
    int nbytes;
} __attribute__((packed)) pko_pkt_read_rly;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
    int nbytes;
} __attribute__((packed)) pko_pkt_write_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
    int offset;
    int whence;
} __attribute__((packed)) pko_pkt_lseek_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    char name[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_remove_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int mode;
    char name[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_mkdir_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    char name[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_rmdir_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int fd;
} __attribute__((packed)) pko_pkt_dread_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int retval;
/* from io_common.h (fio_dirent_t) in ps2lib */
    unsigned int mode;
    unsigned int attr;
    unsigned int size;
    unsigned char ctime[8];
    unsigned char atime[8];
    unsigned char mtime[8];
    unsigned int hisize;
    char name[256];
} __attribute__((packed)) pko_pkt_dread_rly;

////

typedef struct
{
    unsigned int cmd;
    unsigned short len;
} __attribute__((packed)) pko_pkt_reset_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int  argc;
    char argv[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_execee_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int  argc;
    char argv[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_execiop_req;

typedef struct
{
	unsigned int cmd;
	unsigned short len;
	unsigned short size;
	unsigned char file[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_gsexec_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
} __attribute__((packed)) pko_pkt_poweroff_req;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int vpu;
} __attribute__((packed)) pko_pkt_start_vu;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    int vpu;
} __attribute__((packed)) pko_pkt_stop_vu;

typedef struct
{
    unsigned int cmd;
    unsigned short len;
    unsigned int offset;
    unsigned int size;
    char argv[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_mem_io;

typedef struct {
    unsigned int cmd;
    unsigned short len;
    int regs;
    char argv[PKO_MAX_PATH];
} __attribute__((packed)) pko_pkt_dump_regs;

typedef struct {
	unsigned int cmd;
	unsigned short len;
	unsigned int regs[79];
} __attribute__((packed)) pko_pkt_send_regs;

typedef struct {
	unsigned int cmd;
	unsigned short len;
	unsigned int base;
	unsigned int width;
	unsigned int height;
	unsigned short psm;
} __attribute__((packed)) pko_pkt_screenshot;

#define PKO_MAX_WRITE_SEGMENT (1460 - sizeof(pko_pkt_write_req))
#define PKO_MAX_READ_SEGMENT  (1460 - sizeof(pko_pkt_read_rly))
