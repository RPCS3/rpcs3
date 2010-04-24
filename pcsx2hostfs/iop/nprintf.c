/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#include <types.h>
#include <thbase.h>
#include <sifcmd.h>
#include <sysclib.h>
#include <stdio.h>
#include <ioman.h>
#include <intrman.h>
#include <loadcore.h>

////////////////////////////////////////////////////////////////////////
#define NPM_PUTS     0x01
#define RPC_NPM_USER 0x014d704e

////////////////////////////////////////////////////////////////////////
static void *
naplinkRpcHandler(int cmd, void *buffer, int size)
{
    // Only supports npmPrintf of course
    switch(cmd) {
    case NPM_PUTS:
        printf(buffer);
        break;
    default:
        printf("unknown npm rpc call\n");
    }

    return buffer;
}

////////////////////////////////////////////////////////////////////////
static SifRpcServerData_t server __attribute((aligned(16)));
static SifRpcDataQueue_t  queue __attribute((aligned(16)));
static unsigned char rpc_buffer[512] __attribute((aligned(16)));

static void
napThread(void *arg)
{
    int pid;

    SifInitRpc(0);
    pid = GetThreadId();
    SifSetRpcQueue(&queue, pid);
    SifRegisterRpc(&server, RPC_NPM_USER, naplinkRpcHandler,
                   rpc_buffer, 0, 0, &queue);
    SifRpcLoop(&queue);  // Never exits
    ExitDeleteThread();
}
////////////////////////////////////////////////////////////////////////
int
naplinkRpcInit(void)
{
    struct _iop_thread th_attr;
    int ret;
    int pid;

    th_attr.attr = 0x02000000;
    th_attr.option = 0;
    th_attr.thread = napThread;
    th_attr.stacksize = 0x800;
    th_attr.priority = 79;

    pid = CreateThread(&th_attr);
    if (pid < 0) {
        printf("IOP: napRpc createThread failed %d\n", pid);
        return -1;
    }

    ret = StartThread(pid, 0);
    if (ret < 0) {
        printf("IOP: napRpc startThread failed %d\n", ret);
        DeleteThread(pid);
        return -1;
    }
    return 0;
}
