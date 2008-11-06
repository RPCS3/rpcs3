/*
 * QEMU System Emulator header
 * 
 * Copyright (c) 2003 Fabrice Bellard
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef VL_H
#define VL_H

/* we put basic includes here to avoid repeating them in device drivers */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
//#include <inttypes.h>
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

//typedef signed long int intptr_t;
//typedef unsigned long int uintptr_t;

typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef signed long long int intmax_t;
typedef unsigned long long int uintmax_t;


#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
//#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef ENOMEDIUM
#define ENOMEDIUM ENODEV
#endif

#ifdef _WIN32
#include <windows.h>
#define fsync _commit
#define lseek _lseeki64
#define ENOTSUP 4096
extern int qemu_ftruncate64(int, int64_t);
#define ftruncate qemu_ftruncate64


static inline char *realpath(const char *path, char *resolved_path)
{
    _fullpath(resolved_path, path, _MAX_PATH);
    return resolved_path;
}

#define PRId64 "I64d"
#define PRIx64 "I64x"
#define PRIu64 "I64u"
#define PRIo64 "I64o"
#endif

#ifdef QEMU_TOOL

/* we use QEMU_TOOL in the command line tools which do not depend on
   the target CPU type */
#include "config-host.h"
#include <setjmp.h>
#include "osdep.h"
#include "bswap.h"

#else

//#include "audio/audio.h"
//#include "cpu.h"

#endif /* !defined(QEMU_TOOL) */

#ifndef glue
#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)	tostring(s)
#define tostring(s)	#s
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* cutils.c */
void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);
int strstart(const char *str, const char *val, const char **ptr);
int stristart(const char *str, const char *val, const char **ptr);

/* vl.c */
uint64_t muldiv64(uint64_t a, uint32_t b, uint32_t c);

void hw_error(const char *fmt, ...);

extern const char *bios_dir;

extern int vm_running;

typedef struct vm_change_state_entry VMChangeStateEntry;
typedef void VMChangeStateHandler(void *opaque, int running);
typedef void VMStopHandler(void *opaque, int reason);

VMChangeStateEntry *qemu_add_vm_change_state_handler(VMChangeStateHandler *cb,
                                                     void *opaque);
void qemu_del_vm_change_state_handler(VMChangeStateEntry *e);

int qemu_add_vm_stop_handler(VMStopHandler *cb, void *opaque);
void qemu_del_vm_stop_handler(VMStopHandler *cb, void *opaque);

void vm_start(void);
void vm_stop(int reason);

typedef void QEMUResetHandler(void *opaque);

void qemu_register_reset(QEMUResetHandler *func, void *opaque);
void qemu_system_reset_request(void);
void qemu_system_shutdown_request(void);
void qemu_system_powerdown_request(void);
#if !defined(TARGET_SPARC)
// Please implement a power failure function to signal the OS
#define qemu_system_powerdown() do{}while(0)
#else
void qemu_system_powerdown(void);
#endif

void main_loop_wait(int timeout);

extern int ram_size;
extern int bios_size;
extern int rtc_utc;
extern int cirrus_vga_enabled;
extern int graphic_width;
extern int graphic_height;
extern int graphic_depth;
extern const char *keyboard_layout;
extern int kqemu_allowed;
extern int win2k_install_hack;
extern int usb_enabled;
extern int smp_cpus;
extern int no_quit;
extern int semihosting_enabled;
extern int autostart;

#define MAX_OPTION_ROMS 16
extern const char *option_rom[MAX_OPTION_ROMS];
extern int nb_option_roms;

/* XXX: make it dynamic */
#if defined (TARGET_PPC) || defined (TARGET_SPARC64)
#define BIOS_SIZE ((512 + 32) * 1024)
#elif defined(TARGET_MIPS)
#define BIOS_SIZE (4 * 1024 * 1024)
#else
#define BIOS_SIZE ((256 + 64) * 1024)
#endif

/* keyboard/mouse support */

#define MOUSE_EVENT_LBUTTON 0x01
#define MOUSE_EVENT_RBUTTON 0x02
#define MOUSE_EVENT_MBUTTON 0x04

typedef void QEMUPutKBDEvent(void *opaque, int keycode);
typedef void QEMUPutMouseEvent(void *opaque, int dx, int dy, int dz, int buttons_state);

typedef struct QEMUPutMouseEntry {
    QEMUPutMouseEvent *qemu_put_mouse_event;
    void *qemu_put_mouse_event_opaque;
    int qemu_put_mouse_event_absolute;
    char *qemu_put_mouse_event_name;

    /* used internally by qemu for handling mice */
    struct QEMUPutMouseEntry *next;
} QEMUPutMouseEntry;

void qemu_add_kbd_event_handler(QEMUPutKBDEvent *func, void *opaque);
QEMUPutMouseEntry *qemu_add_mouse_event_handler(QEMUPutMouseEvent *func,
                                                void *opaque, int absolute,
                                                const char *name);
void qemu_remove_mouse_event_handler(QEMUPutMouseEntry *entry);

void kbd_put_keycode(int keycode);
void kbd_mouse_event(int dx, int dy, int dz, int buttons_state);
int kbd_mouse_is_absolute(void);

void do_info_mice(void);
void do_mouse_set(int index);

/* keysym is a unicode code except for special keys (see QEMU_KEY_xxx
   constants) */
#define QEMU_KEY_ESC1(c) ((c) | 0xe100)
#define QEMU_KEY_BACKSPACE  0x007f
#define QEMU_KEY_UP         QEMU_KEY_ESC1('A')
#define QEMU_KEY_DOWN       QEMU_KEY_ESC1('B')
#define QEMU_KEY_RIGHT      QEMU_KEY_ESC1('C')
#define QEMU_KEY_LEFT       QEMU_KEY_ESC1('D')
#define QEMU_KEY_HOME       QEMU_KEY_ESC1(1)
#define QEMU_KEY_END        QEMU_KEY_ESC1(4)
#define QEMU_KEY_PAGEUP     QEMU_KEY_ESC1(5)
#define QEMU_KEY_PAGEDOWN   QEMU_KEY_ESC1(6)
#define QEMU_KEY_DELETE     QEMU_KEY_ESC1(3)

#define QEMU_KEY_CTRL_UP         0xe400
#define QEMU_KEY_CTRL_DOWN       0xe401
#define QEMU_KEY_CTRL_LEFT       0xe402
#define QEMU_KEY_CTRL_RIGHT      0xe403
#define QEMU_KEY_CTRL_HOME       0xe404
#define QEMU_KEY_CTRL_END        0xe405
#define QEMU_KEY_CTRL_PAGEUP     0xe406
#define QEMU_KEY_CTRL_PAGEDOWN   0xe407

void kbd_put_keysym(int keysym);

/* async I/O support */

typedef void IOReadHandler(void *opaque, const uint8_t *buf, int size);
typedef int IOCanRWHandler(void *opaque);
typedef void IOHandler(void *opaque);

int qemu_set_fd_handler2(int fd, 
                         IOCanRWHandler *fd_read_poll, 
                         IOHandler *fd_read, 
                         IOHandler *fd_write, 
                         void *opaque);
int qemu_set_fd_handler(int fd,
                        IOHandler *fd_read, 
                        IOHandler *fd_write,
                        void *opaque);

/* Polling handling */

/* return TRUE if no sleep should be done afterwards */
typedef int PollingFunc(void *opaque);

int qemu_add_polling_cb(PollingFunc *func, void *opaque);
void qemu_del_polling_cb(PollingFunc *func, void *opaque);

#ifdef _WIN32
/* Wait objects handling */
typedef void WaitObjectFunc(void *opaque);

int qemu_add_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque);
void qemu_del_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque);
#endif

typedef struct QEMUBH QEMUBH;

/* character device */

#define CHR_EVENT_BREAK 0 /* serial break char */
#define CHR_EVENT_FOCUS 1 /* focus to this terminal (modal input needed) */
#define CHR_EVENT_RESET 2 /* new connection established */


#define CHR_IOCTL_SERIAL_SET_PARAMS   1
typedef struct {
    int speed;
    int parity;
    int data_bits;
    int stop_bits;
} QEMUSerialSetParams;

#define CHR_IOCTL_SERIAL_SET_BREAK    2

#define CHR_IOCTL_PP_READ_DATA        3
#define CHR_IOCTL_PP_WRITE_DATA       4
#define CHR_IOCTL_PP_READ_CONTROL     5
#define CHR_IOCTL_PP_WRITE_CONTROL    6
#define CHR_IOCTL_PP_READ_STATUS      7

typedef void IOEventHandler(void *opaque, int event);

typedef struct CharDriverState {
    int (*chr_write)(struct CharDriverState *s, const uint8_t *buf, int len);
    void (*chr_update_read_handler)(struct CharDriverState *s);
    int (*chr_ioctl)(struct CharDriverState *s, int cmd, void *arg);
    IOEventHandler *chr_event;
    IOCanRWHandler *chr_can_read;
    IOReadHandler *chr_read;
    void *handler_opaque;
    void (*chr_send_event)(struct CharDriverState *chr, int event);
    void (*chr_close)(struct CharDriverState *chr);
    void *opaque;
    QEMUBH *bh;
} CharDriverState;

CharDriverState *qemu_chr_open(const char *filename);
void qemu_chr_printf(CharDriverState *s, const char *fmt, ...);
int qemu_chr_write(CharDriverState *s, const uint8_t *buf, int len);
void qemu_chr_send_event(CharDriverState *s, int event);
void qemu_chr_add_handlers(CharDriverState *s, 
                           IOCanRWHandler *fd_can_read, 
                           IOReadHandler *fd_read,
                           IOEventHandler *fd_event,
                           void *opaque);
int qemu_chr_ioctl(CharDriverState *s, int cmd, void *arg);
void qemu_chr_reset(CharDriverState *s);
int qemu_chr_can_read(CharDriverState *s);
void qemu_chr_read(CharDriverState *s, uint8_t *buf, int len);

/* consoles */

typedef struct DisplayState DisplayState;
typedef struct TextConsole TextConsole;

typedef void (*vga_hw_update_ptr)(void *);
typedef void (*vga_hw_invalidate_ptr)(void *);
typedef void (*vga_hw_screen_dump_ptr)(void *, const char *);

TextConsole *graphic_console_init(DisplayState *ds, vga_hw_update_ptr update,
                                  vga_hw_invalidate_ptr invalidate,
                                  vga_hw_screen_dump_ptr screen_dump,
                                  void *opaque);
void vga_hw_update(void);
void vga_hw_invalidate(void);
void vga_hw_screen_dump(const char *filename);

int is_graphic_console(void);
CharDriverState *text_console_init(DisplayState *ds);
void console_select(unsigned int index);

/* serial ports */

#define MAX_SERIAL_PORTS 4

extern CharDriverState *serial_hds[MAX_SERIAL_PORTS];

/* parallel ports */

#define MAX_PARALLEL_PORTS 3

extern CharDriverState *parallel_hds[MAX_PARALLEL_PORTS];

/* VLANs support */

typedef struct VLANClientState VLANClientState;

struct VLANClientState {
    IOReadHandler *fd_read;
    /* Packets may still be sent if this returns zero.  It's used to
       rate-limit the slirp code.  */
    IOCanRWHandler *fd_can_read;
    void *opaque;
    struct VLANClientState *next;
    struct VLANState *vlan;
    char info_str[256];
};

typedef struct VLANState {
    int id;
    VLANClientState *first_client;
    struct VLANState *next;
} VLANState;

VLANState *qemu_find_vlan(int id);
VLANClientState *qemu_new_vlan_client(VLANState *vlan,
                                      IOReadHandler *fd_read,
                                      IOCanRWHandler *fd_can_read,
                                      void *opaque);
int qemu_can_send_packet(VLANClientState *vc);
void qemu_send_packet(VLANClientState *vc, const uint8_t *buf, int size);
void qemu_handler_true(void *opaque);

void do_info_network(void);

/* TAP win32 */
int tap_win32_init(VLANState *vlan, const char *ifname);

/* NIC info */

#define MAX_NICS 8

typedef struct NICInfo {
    uint8_t macaddr[6];
    const char *model;
    VLANState *vlan;
} NICInfo;

extern int nb_nics;
extern NICInfo nd_table[MAX_NICS];

/* timers */

typedef struct QEMUClock QEMUClock;
typedef struct QEMUTimer QEMUTimer;
typedef void QEMUTimerCB(void *opaque);

/* The real time clock should be used only for stuff which does not
   change the virtual machine state, as it is run even if the virtual
   machine is stopped. The real time clock has a frequency of 1000
   Hz. */
extern QEMUClock *rt_clock;

/* The virtual clock is only run during the emulation. It is stopped
   when the virtual machine is stopped. Virtual timers use a high
   precision clock, usually cpu cycles (use ticks_per_sec). */
extern QEMUClock *vm_clock;

int64_t qemu_get_clock(QEMUClock *clock);

QEMUTimer *qemu_new_timer(QEMUClock *clock, QEMUTimerCB *cb, void *opaque);
void qemu_free_timer(QEMUTimer *ts);
void qemu_del_timer(QEMUTimer *ts);
void qemu_mod_timer(QEMUTimer *ts, int64_t expire_time);
int qemu_timer_pending(QEMUTimer *ts);

extern int64_t ticks_per_sec;
extern int pit_min_timer_count;

int64_t cpu_get_ticks(void);
void cpu_enable_ticks(void);
void cpu_disable_ticks(void);

/* VM Load/Save */

typedef struct QEMUFile QEMUFile;

QEMUFile *qemu_fopen(const char *filename, const char *mode);
void qemu_fflush(QEMUFile *f);
void qemu_fclose(QEMUFile *f);
void qemu_put_buffer(QEMUFile *f, const uint8_t *buf, int size);
void qemu_put_byte(QEMUFile *f, int v);
void qemu_put_be16(QEMUFile *f, unsigned int v);
void qemu_put_be32(QEMUFile *f, unsigned int v);
void qemu_put_be64(QEMUFile *f, uint64_t v);
int qemu_get_buffer(QEMUFile *f, uint8_t *buf, int size);
int qemu_get_byte(QEMUFile *f);
unsigned int qemu_get_be16(QEMUFile *f);
unsigned int qemu_get_be32(QEMUFile *f);
uint64_t qemu_get_be64(QEMUFile *f);

static inline void qemu_put_be64s(QEMUFile *f, const uint64_t *pv)
{
    qemu_put_be64(f, *pv);
}

static inline void qemu_put_be32s(QEMUFile *f, const uint32_t *pv)
{
    qemu_put_be32(f, *pv);
}

static inline void qemu_put_be16s(QEMUFile *f, const uint16_t *pv)
{
    qemu_put_be16(f, *pv);
}

static inline void qemu_put_8s(QEMUFile *f, const uint8_t *pv)
{
    qemu_put_byte(f, *pv);
}

static inline void qemu_get_be64s(QEMUFile *f, uint64_t *pv)
{
    *pv = qemu_get_be64(f);
}

static inline void qemu_get_be32s(QEMUFile *f, uint32_t *pv)
{
    *pv = qemu_get_be32(f);
}

static inline void qemu_get_be16s(QEMUFile *f, uint16_t *pv)
{
    *pv = qemu_get_be16(f);
}

static inline void qemu_get_8s(QEMUFile *f, uint8_t *pv)
{
    *pv = qemu_get_byte(f);
}

#if TARGET_LONG_BITS == 64
#define qemu_put_betl qemu_put_be64
#define qemu_get_betl qemu_get_be64
#define qemu_put_betls qemu_put_be64s
#define qemu_get_betls qemu_get_be64s
#else
#define qemu_put_betl qemu_put_be32
#define qemu_get_betl qemu_get_be32
#define qemu_put_betls qemu_put_be32s
#define qemu_get_betls qemu_get_be32s
#endif

int64_t qemu_ftell(QEMUFile *f);
int64_t qemu_fseek(QEMUFile *f, int64_t pos, int whence);

typedef void SaveStateHandler(QEMUFile *f, void *opaque);
typedef int LoadStateHandler(QEMUFile *f, void *opaque, int version_id);

int register_savevm(const char *idstr, 
                    int instance_id, 
                    int version_id,
                    SaveStateHandler *save_state,
                    LoadStateHandler *load_state,
                    void *opaque);
void qemu_get_timer(QEMUFile *f, QEMUTimer *ts);
void qemu_put_timer(QEMUFile *f, QEMUTimer *ts);

void cpu_save(QEMUFile *f, void *opaque);
int cpu_load(QEMUFile *f, void *opaque, int version_id);

void do_savevm(const char *name);
void do_loadvm(const char *name);
void do_delvm(const char *name);
void do_info_snapshots(void);

/* bottom halves */
typedef void QEMUBHFunc(void *opaque);

QEMUBH *qemu_bh_new(QEMUBHFunc *cb, void *opaque);
void qemu_bh_schedule(QEMUBH *bh);
void qemu_bh_cancel(QEMUBH *bh);
void qemu_bh_delete(QEMUBH *bh);
int qemu_bh_poll(void);

/* block.c */
typedef struct BlockDriverState BlockDriverState;
typedef struct BlockDriver BlockDriver;

extern BlockDriver bdrv_raw;
extern BlockDriver bdrv_host_device;
extern BlockDriver bdrv_cow;
extern BlockDriver bdrv_qcow;
extern BlockDriver bdrv_vmdk;
extern BlockDriver bdrv_cloop;
extern BlockDriver bdrv_dmg;
extern BlockDriver bdrv_bochs;
extern BlockDriver bdrv_vpc;
extern BlockDriver bdrv_vvfat;
extern BlockDriver bdrv_qcow2;

typedef struct BlockDriverInfo {
    /* in bytes, 0 if irrelevant */
    int cluster_size; 
    /* offset at which the VM state can be saved (0 if not possible) */
    int64_t vm_state_offset; 
} BlockDriverInfo;

typedef struct QEMUSnapshotInfo {
    char id_str[128]; /* unique snapshot id */
    /* the following fields are informative. They are not needed for
       the consistency of the snapshot */
    char name[256]; /* user choosen name */
    uint32_t vm_state_size; /* VM state info size */
    uint32_t date_sec; /* UTC date of the snapshot */
    uint32_t date_nsec;
    uint64_t vm_clock_nsec; /* VM clock relative to boot */
} QEMUSnapshotInfo;

#define BDRV_O_RDONLY      0x0000
#define BDRV_O_RDWR        0x0002
#define BDRV_O_ACCESS      0x0003
#define BDRV_O_CREAT       0x0004 /* create an empty file */
#define BDRV_O_SNAPSHOT    0x0008 /* open the file read only and save writes in a snapshot */
#define BDRV_O_FILE        0x0010 /* open as a raw file (do not try to
                                     use a disk image format on top of
                                     it (default for
                                     bdrv_file_open()) */

void bdrv_init(void);
BlockDriver *bdrv_find_format(const char *format_name);
int bdrv_create(BlockDriver *drv, 
                const char *filename, int64_t size_in_sectors,
                const char *backing_file, int flags);
BlockDriverState *bdrv_new(const char *device_name);
void bdrv_delete(BlockDriverState *bs);
int bdrv_file_open(BlockDriverState **pbs, const char *filename, int flags);
int bdrv_open(BlockDriverState *bs, const char *filename, int flags);
int bdrv_open2(BlockDriverState *bs, const char *filename, int flags,
               BlockDriver *drv);
void bdrv_close(BlockDriverState *bs);
int bdrv_read(BlockDriverState *bs, int64_t sector_num, 
              uint8_t *buf, int nb_sectors);
int bdrv_write(BlockDriverState *bs, int64_t sector_num, 
               const uint8_t *buf, int nb_sectors);
int bdrv_pread(BlockDriverState *bs, int64_t offset, 
               void *buf, int count);
int bdrv_pwrite(BlockDriverState *bs, int64_t offset, 
                const void *buf, int count);
int bdrv_truncate(BlockDriverState *bs, int64_t offset);
int64_t bdrv_getlength(BlockDriverState *bs);
void bdrv_get_geometry(BlockDriverState *bs, int64_t *nb_sectors_ptr);
int bdrv_commit(BlockDriverState *bs);
void bdrv_set_boot_sector(BlockDriverState *bs, const uint8_t *data, int size);
/* async block I/O */
typedef struct BlockDriverAIOCB BlockDriverAIOCB;
typedef void BlockDriverCompletionFunc(void *opaque, int ret);

BlockDriverAIOCB *bdrv_aio_read(BlockDriverState *bs, int64_t sector_num,
                                uint8_t *buf, int nb_sectors,
                                BlockDriverCompletionFunc *cb, void *opaque);
BlockDriverAIOCB *bdrv_aio_write(BlockDriverState *bs, int64_t sector_num,
                                 const uint8_t *buf, int nb_sectors,
                                 BlockDriverCompletionFunc *cb, void *opaque);
void bdrv_aio_cancel(BlockDriverAIOCB *acb);

void qemu_aio_init(void);
void qemu_aio_poll(void);
void qemu_aio_flush(void);
void qemu_aio_wait_start(void);
void qemu_aio_wait(void);
void qemu_aio_wait_end(void);

/* Ensure contents are flushed to disk.  */
void bdrv_flush(BlockDriverState *bs);

#define BDRV_TYPE_HD     0
#define BDRV_TYPE_CDROM  1
#define BDRV_TYPE_FLOPPY 2
#define BIOS_ATA_TRANSLATION_AUTO   0
#define BIOS_ATA_TRANSLATION_NONE   1
#define BIOS_ATA_TRANSLATION_LBA    2
#define BIOS_ATA_TRANSLATION_LARGE  3
#define BIOS_ATA_TRANSLATION_RECHS  4

void bdrv_set_geometry_hint(BlockDriverState *bs, 
                            int cyls, int heads, int secs);
void bdrv_set_type_hint(BlockDriverState *bs, int type);
void bdrv_set_translation_hint(BlockDriverState *bs, int translation);
void bdrv_get_geometry_hint(BlockDriverState *bs, 
                            int *pcyls, int *pheads, int *psecs);
int bdrv_get_type_hint(BlockDriverState *bs);
int bdrv_get_translation_hint(BlockDriverState *bs);
int bdrv_is_removable(BlockDriverState *bs);
int bdrv_is_read_only(BlockDriverState *bs);
int bdrv_is_inserted(BlockDriverState *bs);
int bdrv_media_changed(BlockDriverState *bs);
int bdrv_is_locked(BlockDriverState *bs);
void bdrv_set_locked(BlockDriverState *bs, int locked);
void bdrv_eject(BlockDriverState *bs, int eject_flag);
void bdrv_set_change_cb(BlockDriverState *bs, 
                        void (*change_cb)(void *opaque), void *opaque);
void bdrv_get_format(BlockDriverState *bs, char *buf, int buf_size);
void bdrv_info(void);
BlockDriverState *bdrv_find(const char *name);
void bdrv_iterate(void (*it)(void *opaque, const char *name), void *opaque);
int bdrv_is_encrypted(BlockDriverState *bs);
int bdrv_set_key(BlockDriverState *bs, const char *key);
void bdrv_iterate_format(void (*it)(void *opaque, const char *name), 
                         void *opaque);
const char *bdrv_get_device_name(BlockDriverState *bs);
int bdrv_write_compressed(BlockDriverState *bs, int64_t sector_num, 
                          const uint8_t *buf, int nb_sectors);
int bdrv_get_info(BlockDriverState *bs, BlockDriverInfo *bdi);

void bdrv_get_backing_filename(BlockDriverState *bs, 
                               char *filename, int filename_size);
int bdrv_snapshot_create(BlockDriverState *bs, 
                         QEMUSnapshotInfo *sn_info);
int bdrv_snapshot_goto(BlockDriverState *bs, 
                       const char *snapshot_id);
int bdrv_snapshot_delete(BlockDriverState *bs, const char *snapshot_id);
int bdrv_snapshot_list(BlockDriverState *bs, 
                       QEMUSnapshotInfo **psn_info);
char *bdrv_snapshot_dump(char *buf, int buf_size, QEMUSnapshotInfo *sn);

char *get_human_readable_size(char *buf, int buf_size, int64_t size);
int path_is_absolute(const char *path);
void path_combine(char *dest, int dest_size,
                  const char *base_path,
                  const char *filename);

#ifndef QEMU_TOOL

typedef void QEMUMachineInitFunc(int ram_size, int vga_ram_size, 
                                 int boot_device,
             DisplayState *ds, const char **fd_filename, int snapshot,
             const char *kernel_filename, const char *kernel_cmdline,
             const char *initrd_filename);

typedef struct QEMUMachine {
    const char *name;
    const char *desc;
    QEMUMachineInitFunc *init;
    struct QEMUMachine *next;
} QEMUMachine;

int qemu_register_machine(QEMUMachine *m);

typedef void SetIRQFunc(void *opaque, int irq_num, int level);
typedef void IRQRequestFunc(void *opaque, int level);

#ifdef ASDDasdasda0011
/* ISA bus */

extern target_phys_addr_t isa_mem_base;

typedef void (IOPortWriteFunc)(void *opaque, uint32_t address, uint32_t data);
typedef uint32_t (IOPortReadFunc)(void *opaque, uint32_t address);

int register_ioport_read(int start, int length, int size, 
                         IOPortReadFunc *func, void *opaque);
int register_ioport_write(int start, int length, int size, 
                          IOPortWriteFunc *func, void *opaque);
void isa_unassign_ioport(int start, int length);

void isa_mmio_init(target_phys_addr_t base, target_phys_addr_t size);

/* PCI bus */

extern target_phys_addr_t pci_mem_base;

typedef struct PCIBus PCIBus;
typedef struct PCIDevice PCIDevice;

typedef void PCIConfigWriteFunc(PCIDevice *pci_dev, 
                                uint32_t address, uint32_t data, int len);
typedef uint32_t PCIConfigReadFunc(PCIDevice *pci_dev, 
                                   uint32_t address, int len);
typedef void PCIMapIORegionFunc(PCIDevice *pci_dev, int region_num, 
                                uint32_t addr, uint32_t size, int type);

#define PCI_ADDRESS_SPACE_MEM		0x00
#define PCI_ADDRESS_SPACE_IO		0x01
#define PCI_ADDRESS_SPACE_MEM_PREFETCH	0x08

typedef struct PCIIORegion {
    uint32_t addr; /* current PCI mapping address. -1 means not mapped */
    uint32_t size;
    uint8_t type;
    PCIMapIORegionFunc *map_func;
} PCIIORegion;

#define PCI_ROM_SLOT 6
#define PCI_NUM_REGIONS 7

#define PCI_DEVICES_MAX 64

#define PCI_VENDOR_ID		0x00	/* 16 bits */
#define PCI_DEVICE_ID		0x02	/* 16 bits */
#define PCI_COMMAND		0x04	/* 16 bits */
#define  PCI_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY	0x2	/* Enable response in Memory space */
#define PCI_CLASS_DEVICE        0x0a    /* Device class */
#define PCI_INTERRUPT_LINE	0x3c	/* 8 bits */
#define PCI_INTERRUPT_PIN	0x3d	/* 8 bits */
#define PCI_MIN_GNT		0x3e	/* 8 bits */
#define PCI_MAX_LAT		0x3f	/* 8 bits */

struct PCIDevice {
    /* PCI config space */
    uint8_t config[256];

    /* the following fields are read only */
    PCIBus *bus;
    int devfn;
    char name[64];
    PCIIORegion io_regions[PCI_NUM_REGIONS];
    
    /* do not access the following fields */
    PCIConfigReadFunc *config_read;
    PCIConfigWriteFunc *config_write;
    /* ??? This is a PC-specific hack, and should be removed.  */
    int irq_index;

    /* Current IRQ levels.  Used internally by the generic PCI code.  */
    int irq_state[4];
};

PCIDevice *pci_register_device(PCIBus *bus, const char *name,
                               int instance_size, int devfn,
                               PCIConfigReadFunc *config_read, 
                               PCIConfigWriteFunc *config_write);

void pci_register_io_region(PCIDevice *pci_dev, int region_num, 
                            uint32_t size, int type, 
                            PCIMapIORegionFunc *map_func);

void pci_set_irq(PCIDevice *pci_dev, int irq_num, int level);

uint32_t pci_default_read_config(PCIDevice *d, 
                                 uint32_t address, int len);
void pci_default_write_config(PCIDevice *d, 
                              uint32_t address, uint32_t val, int len);
void pci_device_save(PCIDevice *s, QEMUFile *f);
int pci_device_load(PCIDevice *s, QEMUFile *f);

typedef void (*pci_set_irq_fn)(void *pic, int irq_num, int level);
typedef int (*pci_map_irq_fn)(PCIDevice *pci_dev, int irq_num);
PCIBus *pci_register_bus(pci_set_irq_fn set_irq, pci_map_irq_fn map_irq,
                         void *pic, int devfn_min, int nirq);

void pci_nic_init(PCIBus *bus, NICInfo *nd, int devfn);
void pci_data_write(void *opaque, uint32_t addr, uint32_t val, int len);
uint32_t pci_data_read(void *opaque, uint32_t addr, int len);
int pci_bus_num(PCIBus *s);
void pci_for_each_device(int bus_num, void (*fn)(PCIDevice *d));

void pci_info(void);
PCIBus *pci_bridge_init(PCIBus *bus, int devfn, uint32_t id,
                        pci_map_irq_fn map_irq, const char *name);

/* prep_pci.c */
PCIBus *pci_prep_init(void);

/* grackle_pci.c */
PCIBus *pci_grackle_init(uint32_t base, void *pic);

/* unin_pci.c */
PCIBus *pci_pmac_init(void *pic);

/* apb_pci.c */
PCIBus *pci_apb_init(target_ulong special_base, target_ulong mem_base,
                     void *pic);

PCIBus *pci_vpb_init(void *pic, int irq, int realview);

/* piix_pci.c */
PCIBus *i440fx_init(PCIDevice **pi440fx_state);
void i440fx_set_smm(PCIDevice *d, int val);
int piix3_init(PCIBus *bus, int devfn);
void i440fx_init_memory_mappings(PCIDevice *d);

int piix4_init(PCIBus *bus, int devfn);

/* openpic.c */
typedef struct openpic_t openpic_t;
void openpic_set_irq(void *opaque, int n_IRQ, int level);
openpic_t *openpic_init (PCIBus *bus, int *pmem_index, int nb_cpus,
                         CPUState **envp);

/* heathrow_pic.c */
typedef struct HeathrowPICS HeathrowPICS;
void heathrow_pic_set_irq(void *opaque, int num, int level);
HeathrowPICS *heathrow_pic_init(int *pmem_index);

/* gt64xxx.c */
PCIBus *pci_gt64120_init(void *pic);

#ifdef HAS_AUDIO
struct soundhw {
    const char *name;
    const char *descr;
    int enabled;
    int isa;
    union {
        int (*init_isa) (AudioState *s);
        int (*init_pci) (PCIBus *bus, AudioState *s);
    } init;
};

extern struct soundhw soundhw[];
#endif

/* vga.c */

#define VGA_RAM_SIZE (8192 * 1024)

struct DisplayState {
    uint8_t *data;
    int linesize;
    int depth;
    int bgr; /* BGR color order instead of RGB. Only valid for depth == 32 */
    int width;
    int height;
    void *opaque;

    void (*dpy_update)(struct DisplayState *s, int x, int y, int w, int h);
    void (*dpy_resize)(struct DisplayState *s, int w, int h);
    void (*dpy_refresh)(struct DisplayState *s);
    void (*dpy_copy)(struct DisplayState *s, int src_x, int src_y, int dst_x, int dst_y, int w, int h);
};

static inline void dpy_update(DisplayState *s, int x, int y, int w, int h)
{
    s->dpy_update(s, x, y, w, h);
}

static inline void dpy_resize(DisplayState *s, int w, int h)
{
    s->dpy_resize(s, w, h);
}

int isa_vga_init(DisplayState *ds, uint8_t *vga_ram_base, 
                 unsigned long vga_ram_offset, int vga_ram_size);
int pci_vga_init(PCIBus *bus, DisplayState *ds, uint8_t *vga_ram_base, 
                 unsigned long vga_ram_offset, int vga_ram_size,
                 unsigned long vga_bios_offset, int vga_bios_size);

/* cirrus_vga.c */
void pci_cirrus_vga_init(PCIBus *bus, DisplayState *ds, uint8_t *vga_ram_base, 
                         unsigned long vga_ram_offset, int vga_ram_size);
void isa_cirrus_vga_init(DisplayState *ds, uint8_t *vga_ram_base, 
                         unsigned long vga_ram_offset, int vga_ram_size);

/* sdl.c */
void sdl_display_init(DisplayState *ds, int full_screen);

/* cocoa.m */
void cocoa_display_init(DisplayState *ds, int full_screen);

/* vnc.c */
void vnc_display_init(DisplayState *ds, const char *display);
void do_info_vnc(void);

/* x_keymap.c */
extern uint8_t _translate_keycode(const int key);

/* ide.c */
#define MAX_DISKS 4

extern BlockDriverState *bs_table[MAX_DISKS + 1];

void isa_ide_init(int iobase, int iobase2, int irq,
                  BlockDriverState *hd0, BlockDriverState *hd1);
void pci_cmd646_ide_init(PCIBus *bus, BlockDriverState **hd_table,
                         int secondary_ide_enabled);
void pci_piix3_ide_init(PCIBus *bus, BlockDriverState **hd_table, int devfn);
int pmac_ide_init (BlockDriverState **hd_table,
                   SetIRQFunc *set_irq, void *irq_opaque, int irq);

/* cdrom.c */
int cdrom_read_toc(int nb_sectors, uint8_t *buf, int msf, int start_track);
int cdrom_read_toc_raw(int nb_sectors, uint8_t *buf, int msf, int session_num);

/* es1370.c */
int es1370_init (PCIBus *bus, AudioState *s);

/* sb16.c */
int SB16_init (AudioState *s);

/* adlib.c */
int Adlib_init (AudioState *s);

/* gus.c */
int GUS_init (AudioState *s);

/* dma.c */
typedef int (*DMA_transfer_handler) (void *opaque, int nchan, int pos, int size);
int DMA_get_channel_mode (int nchan);
int DMA_read_memory (int nchan, void *buf, int pos, int size);
int DMA_write_memory (int nchan, void *buf, int pos, int size);
void DMA_hold_DREQ (int nchan);
void DMA_release_DREQ (int nchan);
void DMA_schedule(int nchan);
void DMA_run (void);
void DMA_init (int high_page_enable);
void DMA_register_channel (int nchan,
                           DMA_transfer_handler transfer_handler,
                           void *opaque);
/* fdc.c */
#define MAX_FD 2
extern BlockDriverState *fd_table[MAX_FD];

typedef struct fdctrl_t fdctrl_t;

fdctrl_t *fdctrl_init (int irq_lvl, int dma_chann, int mem_mapped, 
                       uint32_t io_base,
                       BlockDriverState **fds);
int fdctrl_get_drive_type(fdctrl_t *fdctrl, int drive_num);

/* ne2000.c */

void isa_ne2000_init(int base, int irq, NICInfo *nd);
void pci_ne2000_init(PCIBus *bus, NICInfo *nd, int devfn);

/* rtl8139.c */

void pci_rtl8139_init(PCIBus *bus, NICInfo *nd, int devfn);

/* pcnet.c */

void pci_pcnet_init(PCIBus *bus, NICInfo *nd, int devfn);
void pcnet_h_reset(void *opaque);
void *lance_init(NICInfo *nd, uint32_t leaddr, void *dma_opaque);


/* pckbd.c */

void kbd_init(void);

/* mc146818rtc.c */

typedef struct RTCState RTCState;

RTCState *rtc_init(int base, int irq);
void rtc_set_memory(RTCState *s, int addr, int val);
void rtc_set_date(RTCState *s, const struct tm *tm);

/* serial.c */

typedef struct SerialState SerialState;
SerialState *serial_init(SetIRQFunc *set_irq, void *opaque,
                         int base, int irq, CharDriverState *chr);
SerialState *serial_mm_init (SetIRQFunc *set_irq, void *opaque,
                             target_ulong base, int it_shift,
                             int irq, CharDriverState *chr);

/* parallel.c */

typedef struct ParallelState ParallelState;
ParallelState *parallel_init(int base, int irq, CharDriverState *chr);

/* i8259.c */

typedef struct PicState2 PicState2;
extern PicState2 *isa_pic;
void pic_set_irq(int irq, int level);
void pic_set_irq_new(void *opaque, int irq, int level);
PicState2 *pic_init(IRQRequestFunc *irq_request, void *irq_request_opaque);
void pic_set_alt_irq_func(PicState2 *s, SetIRQFunc *alt_irq_func,
                          void *alt_irq_opaque);
int pic_read_irq(PicState2 *s);
void pic_update_irq(PicState2 *s);
uint32_t pic_intack_read(PicState2 *s);
void pic_info(void);
void irq_info(void);

/* APIC */
typedef struct IOAPICState IOAPICState;

int apic_init(CPUState *env);
int apic_get_interrupt(CPUState *env);
IOAPICState *ioapic_init(void);
void ioapic_set_irq(void *opaque, int vector, int level);

/* i8254.c */

#define PIT_FREQ 1193182

typedef struct PITState PITState;

PITState *pit_init(int base, int irq);
void pit_set_gate(PITState *pit, int channel, int val);
int pit_get_gate(PITState *pit, int channel);
int pit_get_initial_count(PITState *pit, int channel);
int pit_get_mode(PITState *pit, int channel);
int pit_get_out(PITState *pit, int channel, int64_t current_time);

/* pcspk.c */
void pcspk_init(PITState *);
int pcspk_audio_init(AudioState *);

#include "hw/smbus.h"

/* acpi.c */
extern int acpi_enabled;
void piix4_pm_init(PCIBus *bus, int devfn);
void piix4_smbus_register_device(SMBusDevice *dev, uint8_t addr);
void acpi_bios_init(void);

/* smbus_eeprom.c */
SMBusDevice *smbus_eeprom_device_init(uint8_t addr, uint8_t *buf);

/* pc.c */
extern QEMUMachine pc_machine;
extern QEMUMachine isapc_machine;
extern int fd_bootchk;

void ioport_set_a20(int enable);
int ioport_get_a20(void);

/* ppc.c */
extern QEMUMachine prep_machine;
extern QEMUMachine core99_machine;
extern QEMUMachine heathrow_machine;

/* mips_r4k.c */
extern QEMUMachine mips_machine;

/* mips_malta.c */
extern QEMUMachine mips_malta_machine;

/* mips_int */
extern void cpu_mips_irq_request(void *opaque, int irq, int level);

/* mips_timer.c */
extern void cpu_mips_clock_init(CPUState *);
extern void cpu_mips_irqctrl_init (void);

/* shix.c */
extern QEMUMachine shix_machine;

#ifdef TARGET_PPC
ppc_tb_t *cpu_ppc_tb_init (CPUState *env, uint32_t freq);
#endif
void PREP_debug_write (void *opaque, uint32_t addr, uint32_t val);

extern CPUWriteMemoryFunc *PPC_io_write[];
extern CPUReadMemoryFunc *PPC_io_read[];
void PPC_debug_write (void *opaque, uint32_t addr, uint32_t val);

/* sun4m.c */
extern QEMUMachine sun4m_machine;
void pic_set_irq_cpu(int irq, int level, unsigned int cpu);

/* iommu.c */
void *iommu_init(uint32_t addr);
void sparc_iommu_memory_rw(void *opaque, target_phys_addr_t addr,
                                 uint8_t *buf, int len, int is_write);
static inline void sparc_iommu_memory_read(void *opaque,
                                           target_phys_addr_t addr,
                                           uint8_t *buf, int len)
{
    sparc_iommu_memory_rw(opaque, addr, buf, len, 0);
}

static inline void sparc_iommu_memory_write(void *opaque,
                                            target_phys_addr_t addr,
                                            uint8_t *buf, int len)
{
    sparc_iommu_memory_rw(opaque, addr, buf, len, 1);
}

/* tcx.c */
void tcx_init(DisplayState *ds, uint32_t addr, uint8_t *vram_base,
	       unsigned long vram_offset, int vram_size, int width, int height);

/* slavio_intctl.c */
void *slavio_intctl_init();
void slavio_intctl_set_cpu(void *opaque, unsigned int cpu, CPUState *env);
void slavio_pic_info(void *opaque);
void slavio_irq_info(void *opaque);
void slavio_pic_set_irq(void *opaque, int irq, int level);
void slavio_pic_set_irq_cpu(void *opaque, int irq, int level, unsigned int cpu);

/* loader.c */
int get_image_size(const char *filename);
int load_image(const char *filename, uint8_t *addr);
int load_elf(const char *filename, int64_t virt_to_phys_addend, uint64_t *pentry);
int load_aout(const char *filename, uint8_t *addr);

/* slavio_timer.c */
void slavio_timer_init(uint32_t addr, int irq, int mode, unsigned int cpu);

/* slavio_serial.c */
SerialState *slavio_serial_init(int base, int irq, CharDriverState *chr1, CharDriverState *chr2);
void slavio_serial_ms_kbd_init(int base, int irq);

/* slavio_misc.c */
void *slavio_misc_init(uint32_t base, int irq);
void slavio_set_power_fail(void *opaque, int power_failing);

/* esp.c */
void esp_scsi_attach(void *opaque, BlockDriverState *bd, int id);
void *esp_init(BlockDriverState **bd, uint32_t espaddr, void *dma_opaque);
void esp_reset(void *opaque);

/* sparc32_dma.c */
void *sparc32_dma_init(uint32_t daddr, int espirq, int leirq, void *iommu,
                       void *intctl);
void ledma_set_irq(void *opaque, int isr);
void ledma_memory_read(void *opaque, target_phys_addr_t addr, 
                       uint8_t *buf, int len, int do_bswap);
void ledma_memory_write(void *opaque, target_phys_addr_t addr, 
                        uint8_t *buf, int len, int do_bswap);
void espdma_raise_irq(void *opaque);
void espdma_clear_irq(void *opaque);
void espdma_memory_read(void *opaque, uint8_t *buf, int len);
void espdma_memory_write(void *opaque, uint8_t *buf, int len);
void sparc32_dma_set_reset_data(void *opaque, void *esp_opaque,
                                void *lance_opaque);

/* cs4231.c */
void cs_init(target_phys_addr_t base, int irq, void *intctl);

/* sun4u.c */
extern QEMUMachine sun4u_machine;

/* NVRAM helpers */
#include "hw/m48t59.h"

void NVRAM_set_byte (m48t59_t *nvram, uint32_t addr, uint8_t value);
uint8_t NVRAM_get_byte (m48t59_t *nvram, uint32_t addr);
void NVRAM_set_word (m48t59_t *nvram, uint32_t addr, uint16_t value);
uint16_t NVRAM_get_word (m48t59_t *nvram, uint32_t addr);
void NVRAM_set_lword (m48t59_t *nvram, uint32_t addr, uint32_t value);
uint32_t NVRAM_get_lword (m48t59_t *nvram, uint32_t addr);
void NVRAM_set_string (m48t59_t *nvram, uint32_t addr,
                       const unsigned char *str, uint32_t max);
int NVRAM_get_string (m48t59_t *nvram, uint8_t *dst, uint16_t addr, int max);
void NVRAM_set_crc (m48t59_t *nvram, uint32_t addr,
                    uint32_t start, uint32_t count);
int PPC_NVRAM_set_params (m48t59_t *nvram, uint16_t NVRAM_size,
                          const unsigned char *arch,
                          uint32_t RAM_size, int boot_device,
                          uint32_t kernel_image, uint32_t kernel_size,
                          const char *cmdline,
                          uint32_t initrd_image, uint32_t initrd_size,
                          uint32_t NVRAM_image,
                          int width, int height, int depth);

/* adb.c */

#define MAX_ADB_DEVICES 16

#define ADB_MAX_OUT_LEN 16

typedef struct ADBDevice ADBDevice;

/* buf = NULL means polling */
typedef int ADBDeviceRequest(ADBDevice *d, uint8_t *buf_out,
                              const uint8_t *buf, int len);
typedef int ADBDeviceReset(ADBDevice *d);

struct ADBDevice {
    struct ADBBusState *bus;
    int devaddr;
    int handler;
    ADBDeviceRequest *devreq;
    ADBDeviceReset *devreset;
    void *opaque;
};

typedef struct ADBBusState {
    ADBDevice devices[MAX_ADB_DEVICES];
    int nb_devices;
    int poll_index;
} ADBBusState;

int adb_request(ADBBusState *s, uint8_t *buf_out,
                const uint8_t *buf, int len);
int adb_poll(ADBBusState *s, uint8_t *buf_out);

ADBDevice *adb_register_device(ADBBusState *s, int devaddr, 
                               ADBDeviceRequest *devreq, 
                               ADBDeviceReset *devreset, 
                               void *opaque);
void adb_kbd_init(ADBBusState *bus);
void adb_mouse_init(ADBBusState *bus);

/* cuda.c */

extern ADBBusState adb_bus;
int cuda_init(SetIRQFunc *set_irq, void *irq_opaque, int irq);

#include "hw/usb.h"

/* usb ports of the VM */

void qemu_register_usb_port(USBPort *port, void *opaque, int index,
                            usb_attachfn attach);

#define VM_USB_HUB_SIZE 8

void do_usb_add(const char *devname);
void do_usb_del(const char *devname);
void usb_info(void);

/* scsi-disk.c */
enum scsi_reason {
    SCSI_REASON_DONE, /* Command complete.  */
    SCSI_REASON_DATA  /* Transfer complete, more data required.  */
};

typedef struct SCSIDevice SCSIDevice;
typedef void (*scsi_completionfn)(void *opaque, int reason, uint32_t tag,
                                  uint32_t arg);

SCSIDevice *scsi_disk_init(BlockDriverState *bdrv,
                           int tcq,
                           scsi_completionfn completion,
                           void *opaque);
void scsi_disk_destroy(SCSIDevice *s);

int32_t scsi_send_command(SCSIDevice *s, uint32_t tag, uint8_t *buf, int lun);
/* SCSI data transfers are asynchrnonous.  However, unlike the block IO
   layer the completion routine may be called directly by
   scsi_{read,write}_data.  */
void scsi_read_data(SCSIDevice *s, uint32_t tag);
int scsi_write_data(SCSIDevice *s, uint32_t tag);
void scsi_cancel_io(SCSIDevice *s, uint32_t tag);
uint8_t *scsi_get_buf(SCSIDevice *s, uint32_t tag);

/* lsi53c895a.c */
void lsi_scsi_attach(void *opaque, BlockDriverState *bd, int id);
void *lsi_scsi_init(PCIBus *bus, int devfn);

/* integratorcp.c */
extern QEMUMachine integratorcp926_machine;
extern QEMUMachine integratorcp1026_machine;

/* versatilepb.c */
extern QEMUMachine versatilepb_machine;
extern QEMUMachine versatileab_machine;

/* realview.c */
extern QEMUMachine realview_machine;

/* ps2.c */
void *ps2_kbd_init(void (*update_irq)(void *, int), void *update_arg);
void *ps2_mouse_init(void (*update_irq)(void *, int), void *update_arg);
void ps2_write_mouse(void *, int val);
void ps2_write_keyboard(void *, int val);
uint32_t ps2_read_data(void *);
void ps2_queue(void *, int b);
void ps2_keyboard_set_translation(void *opaque, int mode);

/* smc91c111.c */
void smc91c111_init(NICInfo *, uint32_t, void *, int);

/* pl110.c */
void *pl110_init(DisplayState *ds, uint32_t base, void *pic, int irq, int);

/* pl011.c */
void pl011_init(uint32_t base, void *pic, int irq, CharDriverState *chr);

/* pl050.c */
void pl050_init(uint32_t base, void *pic, int irq, int is_mouse);

/* pl080.c */
void *pl080_init(uint32_t base, void *pic, int irq, int nchannels);

/* pl190.c */
void *pl190_init(uint32_t base, void *parent, int irq, int fiq);

/* arm-timer.c */
void sp804_init(uint32_t base, void *pic, int irq);
void icp_pit_init(uint32_t base, void *pic, int irq);

/* arm_sysctl.c */
void arm_sysctl_init(uint32_t base, uint32_t sys_id);

/* arm_gic.c */
void *arm_gic_init(uint32_t base, void *parent, int parent_irq);

/* arm_boot.c */

void arm_load_kernel(CPUState *env, int ram_size, const char *kernel_filename,
                     const char *kernel_cmdline, const char *initrd_filename,
                     int board_id);

/* sh7750.c */
struct SH7750State;

struct SH7750State *sh7750_init(CPUState * cpu);

typedef struct {
    /* The callback will be triggered if any of the designated lines change */
    uint16_t portamask_trigger;
    uint16_t portbmask_trigger;
    /* Return 0 if no action was taken */
    int (*port_change_cb) (uint16_t porta, uint16_t portb,
			   uint16_t * periph_pdtra,
			   uint16_t * periph_portdira,
			   uint16_t * periph_pdtrb,
			   uint16_t * periph_portdirb);
} sh7750_io_device;

int sh7750_register_io_device(struct SH7750State *s,
			      sh7750_io_device * device);
/* tc58128.c */
int tc58128_init(struct SH7750State *s, char *zone1, char *zone2);

/* NOR flash devices */
typedef struct pflash_t pflash_t;

pflash_t *pflash_register (target_ulong base, ram_addr_t off,
                           BlockDriverState *bs,
                           target_ulong sector_len, int nb_blocs, int width,
                           uint16_t id0, uint16_t id1, 
                           uint16_t id2, uint16_t id3);

#include "gdbstub.h"

#endif /* defined(QEMU_TOOL) */

/* monitor.c */
void monitor_init(CharDriverState *hd, int show_banner);
void term_puts(const char *str);
void term_vprintf(const char *fmt, va_list ap);
void term_printf(const char *fmt, ...);
void term_print_filename(const char *filename);
void term_flush(void);
void term_print_help(void);
void monitor_readline(const char *prompt, int is_password,
                      char *buf, int buf_size);

/* readline.c */
typedef void ReadLineFunc(void *opaque, const char *str);

extern int completion_index;
void add_completion(const char *str);
void readline_handle_byte(int ch);
void readline_find_completion(const char *cmdline);
const char *readline_get_history(unsigned int index);
void readline_start(const char *prompt, int is_password,
                    ReadLineFunc *readline_func, void *opaque);

void kqemu_record_dump(void);
#endif

#endif /* VL_H */
