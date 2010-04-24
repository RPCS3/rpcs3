/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * Copyright (C) 2004 adresd (adresd_ps2dev@yahoo.com)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#include "stdio.h"
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <fileio.h>
#include <string.h>

#include "debug.h"
#include "excepHandler.h"

#include "byteorder.h"
#include "ps2regs.h"
#include "hostlink.h"

//#define scr_printf(args...) printf(args)
//#define init_scr() do { } while(0)
#ifdef DEBUG
#define dbgprintf(args...) printf(args)
#else
#define dbgprintf(args...) do { } while(0)
#endif

#if HOOK_THREADS
extern void KillActiveThreads(void);
#endif

////////////////////////////////////////////////////////////////////////
// Prototypes
static int cmdThread(void);
static int pkoStopVU(pko_pkt_stop_vu *);
static int pkoStartVU(pko_pkt_start_vu *);
static int pkoDumpMem(pko_pkt_mem_io *);
static int pkoDumpReg(pko_pkt_dump_regs *);
void pkoReset(void);
static int pkoLoadElf(char *path);
static int pkoGSExec(pko_pkt_gsexec_req *);
static int pkoWriteMem(pko_pkt_mem_io *);


// Flags for which type of boot (oh crap, make a header file dammit)
#define B_CD 1
#define B_MC 2
#define B_HOST 3
#define B_CC 4
#define B_UNKN 5

////////////////////////////////////////////////////////////////////////
// Globals
extern u32 _start;
extern int _gp;
extern int boot;
extern char elfName[];

int userThreadID = 0;
static int cmdThreadID = 0;
static char userThreadStack[16*1024]    __attribute__((aligned(16)));
static char cmdThreadStack[16*1024]     __attribute__((aligned(64)));
static char dataBuffer[16384]           __attribute__((aligned(16)));

// The 'global' argv string area
static char argvStrings[PKO_MAX_PATH];

////////////////////////////////////////////////////////////////////////
// How about that header file again?
#define MAX_ARGS 16
#define MAX_ARGLEN 256

struct argData
{
    int flag;                     // Contains thread id atm
    int argc;
    char *argv[MAX_ARGS];
} __attribute__((packed)) userArgs;

////////////////////////////////////////////////////////////////////////
int sifCmdSema;
int sif0HandlerId = 0;
// XXX: Hardcoded address atm.. Should be configurable!!
unsigned int *sifDmaDataPtr =(unsigned int*)(0x20100000-2048);
int excepscrdump = 1;

extern int pkoExecEE(pko_pkt_execee_req *cmd);

////////////////////////////////////////////////////////////////////////
// Create the argument struct to send to the user thread
int
makeArgs(int cmdargc, char *cmdargv, struct argData *arg_data)
{
    int i;
    int t;
    int argc;

    argc = 0;

    if (cmdargc > MAX_ARGS)
        cmdargc = MAX_ARGS;
    cmdargv[PKO_MAX_PATH-1] = '\0';

    memcpy(argvStrings, cmdargv, PKO_MAX_PATH);

    dbgprintf("cmd->argc %d, argv[0]: %s\n", cmdargc, cmdargv);

    i = 0;
    t = 0;
    do {
        arg_data->argv[i] = &argvStrings[t];
        dbgprintf("arg_data[%d]=%s\n", i, arg_data->argv[i]);
        dbgprintf("argvStrings[%d]=%s\n", t, &argvStrings[t]);
        t += strlen(&argvStrings[t]);
        t++;
        i++;
    } while ((i < cmdargc) && (t < PKO_MAX_PATH));

    arg_data->argc = i;

    return 0;
}

////////////////////////////////////////////////////////////////////////
// Load the actual elf, and create a thread for it
// Return the thread id
static int
pkoLoadElf(char *path)
{
    ee_thread_t th_attr;
    t_ExecData elfdata;
    int ret;
    int pid;

    ret = SifLoadElf(path, &elfdata);

    FlushCache(0);
    FlushCache(2);

    dbgprintf("EE: LoadElf returned %d\n", ret);

    dbgprintf("EE: Creating user thread (ent: %x, gp: %x, st: %x)\n", 
              elfdata.epc, elfdata.gp, elfdata.sp);

    if (elfdata.epc == 0) {
        dbgprintf("EE: Could not load file\n");
        return -1;
    }

    th_attr.func = (void *)elfdata.epc;
    th_attr.stack = userThreadStack;
    th_attr.stack_size = sizeof(userThreadStack);
    th_attr.gp_reg = (void *)elfdata.gp;
    th_attr.initial_priority = 64;

    pid = CreateThread(&th_attr);
    if (pid < 0) {
        printf("EE: Create user thread failed %d\n", pid);
        return -1;
    }

    dbgprintf("EE: Created user thread: %d\n", pid);

    return pid;
}


////////////////////////////////////////////////////////////////////////
// Load and start the requested elf
extern int pkoExecEE(pko_pkt_execee_req *cmd)
{
    char path[PKO_MAX_PATH];
    int ret;
    int pid;

    if (userThreadID) {
        return -1;
    }

    dbgprintf("EE: Executing file %s...\n", cmd->argv);
    memcpy(path, cmd->argv, PKO_MAX_PATH);

    scr_printf("Executing file %s...\n", path);

    pid = pkoLoadElf(path);
    if (pid < 0) {
        scr_printf("Could not execute file %s\n", path);
        return -1;
    }

    FlushCache(0);
    FlushCache(2);

    userThreadID = pid;

    makeArgs(ntohl(cmd->argc), path, &userArgs);

    // Hack away..
    userArgs.flag = (int)&userThreadID;

    ret = StartThread(userThreadID, &userArgs);
    if (ret < 0) {
        printf("EE: Start user thread failed %d\n", ret);
        cmdThreadID = 0;
        DeleteThread(userThreadID);
        return -1;
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////
// takes cmd->data and sends it to GIF via path1
static int
pkoGSExec(pko_pkt_gsexec_req *cmd) {
	int fd;
    int len;
	fd = fioOpen(cmd->file, O_RDONLY);
	if ( fd < 0 ) {
		return fd;
	}
	len = fioRead(fd, dataBuffer, 128);
	fioClose(fd);
    // stop/reset dma02

    // dmasend via GIF channel
    asm __volatile__(
        "move   $10, %0;"
        "move   $11, %1;"
        "lui    $8, 0x1001;"
        "sw     $11, -24544($8);"
        "sw     $10, -24560($8);"
        "ori    $9, $0, 0x101;"
        "sw     $9, -24576($8);"
	:: "r" (dataBuffer), "r" ((ntohs(cmd->size))/16) );

    // dmawait for GIF channel
    asm __volatile__(
        "lui    $8, 0x1001;"
        "dmawait:"
        "lw     $9, -24576($8);"
        "nop;"
        "andi   $9, $9, 0x100;"
        "nop;"
        "bnez   $9, dmawait;"
        "nop;"
       );
	return 0;
}

////////////////////////////////////////////////////////////////////////
// command to dump cmd->size bytes of memory from cmd->offset
static int 
pkoDumpMem(pko_pkt_mem_io *cmd) {
	int fd;
    int total, len;
	unsigned int offset;
	unsigned int size;
    char path[PKO_MAX_PATH];
    int ret = 0;
   	size = ntohl(cmd->size);
    offset = ntohl(cmd->offset);
	scr_printf("dump mem from 0x%x, size %d\n", 
		ntohl(cmd->offset), ntohl(cmd->size)
		);
    memcpy(path, cmd->argv, PKO_MAX_PATH);
	fd = fioOpen(path, O_WRONLY|O_CREAT);
    total = 0;
    len = 16*1024;
	if ( fd > 0 ) {
        while(total < size) {
            if ( size < len) {
                len = size;
            } else if ((size - total) < len) {
                len = size - total;
            }
            
            memcpy(dataBuffer, (void *)offset, len);
            
            if ((ret = fioWrite(fd, (void *)dataBuffer, len)) > 0) {
            } else {
                printf("EE: pkoDumpMem() fioWrite failed\n");
                return fd;
            }	
            offset = offset + len;	
            total += len;
        }
    }
	fioClose(fd);
	return ret;
}

static int
pkoWriteMem(pko_pkt_mem_io *cmd) {
	int fd, len, total;
	unsigned int offset;
	unsigned int size;
    char path[PKO_MAX_PATH];
    int ret = 0;
   	size = ntohl(cmd->size);
    offset = ntohl(cmd->offset);
	scr_printf("write mem to 0x%x, size %d\n", 
		ntohl(cmd->offset), ntohl(cmd->size)
		);
    memcpy(path, cmd->argv, PKO_MAX_PATH);
	fd = fioOpen(path, O_RDONLY);
    total = 0;
    len = 16*1024;
    if( fd > 0) {
        while(total < size) {
            if ( size < len) {
                len = size;
            } else if ((size - total) < len) {
                len = size - total;
            }

            if ((ret = fioRead(fd, (void *)dataBuffer, len)) > 0) {
            } else {
                printf("EE: pkoDumpMem() fioWrite failed\n");
                return fd;
            }

            memcpy((void *)offset, dataBuffer, len);
            offset = offset + len;
            total += len;
        }
    }
    fioClose(fd);
    return ret;
}

////////////////////////////////////////////////////////////////////////
// command to stop VU0/1 and VIF0/1
static int
pkoStopVU(pko_pkt_stop_vu *cmd) {
	if ( ntohs(cmd->vpu) == 0 ) {
		asm(
			"ori     $25, $0, 0x1;"
			"ctc2    $25, $28;"
			"sync.p;"
			"vnop;"
		   );
		VIF0_FBRST = 0x2;
	} else if ( ntohs(cmd->vpu) == 1 ) {
		asm(
			"ori     $25, $0, 0x100;"
			"ctc2    $25, $28;"
			"sync.p;"
			"vnop;"
		   );
		VIF1_FBRST = 0x2;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
// command to reset VU0/1 VIF0/1
static int
pkoStartVU(pko_pkt_start_vu *cmd) {
	if ( ntohs(cmd->vpu) == 0 ) {
		asm(
			"ori     $25, $0, 0x2;"
			"ctc2    $25, $28;"
			"sync.p;"
			"vnop;"
		   );
		VIF0_FBRST = 0x8;
	} else if ( ntohs(cmd->vpu) == 1 ) {
		asm(
			"ori     $25, $0, 0x200;"
			"ctc2    $25, $28;"
			"sync.p;"
			"vnop;"
		   );
		VIF1_FBRST = 0x8;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
void
pkoReset(void)
{
    char *argv[1];
    // Check if user thread is running, if so kill it

#if 1

#if HOOK_THREADS
    KillActiveThreads();
#else
    if (userThreadID) {
        TerminateThread(userThreadID);
        DeleteThread(userThreadID);
    }
#endif
#endif
    userThreadID = 0;

    RemoveDmacHandler(5, sif0HandlerId);
    sif0HandlerId = 0;

    SifInitRpc(0);
    //cdvdInit(CDVD_INIT_NOWAIT);
    //cdvdInit(CDVD_EXIT);
    SifIopReset(NULL, 0);
    SifExitRpc();
    while(SifIopSync());
#if 1
    SifInitRpc(0);
    //cdvdExit();
    SifExitRpc();
#endif
    FlushCache(0);

#ifdef USE_CACHED_CFG
    argv[0] = elfName;
	 SifLoadFileExit();
    ExecPS2(&_start, 0, 1, argv);
    return;
#endif

    if ((boot == B_MC) || (boot == B_HOST) || (boot == B_UNKN) || (boot == B_CC)) {
       argv[0] = elfName;
		 SifLoadFileExit();
       ExecPS2(&_start, 0, 1, argv);
    }
    else {
       LoadExecPS2(elfName, 0, NULL);
    }
}
