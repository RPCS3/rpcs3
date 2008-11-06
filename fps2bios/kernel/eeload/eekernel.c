//[module]	KERNEL
//[processor]	EE
//[type]	RAW
//[size]	80880(0x13BF0)
//[bios]	10000(0xB3200-0xC6DF0)
//[iopboot]	NA
//[loaded @]	80000000
//[name]	NA
//[version]	NA
//[memory map]	[too many, see mem.txt]
//[handlers]	
//[entry point]	ENTRYPOINT @ 0x80001000
//[made by]	[RO]man, zerofrog & others
#include <tamtypes.h>
#include <stdio.h>

#include "eekernel.h"
#include "eeinit.h"
#include "eedebug.h"
#include "romdir.h"

#define PS2_LOADADDR 0x82000

// internal functions
int InitPgifHandler();
int InitPgifHandler2();

struct ll* LL_unlink(struct ll *l);
struct ll* LL_rotate(struct ll *l);
struct ll *LL_unlinkthis(struct ll *l);
void LL_add(struct ll *l, struct ll *new);

extern int (*table_GetCop0[32])(int reg); // 124E8-12564
extern int (*table_SetCop0[32])(int reg, int val); // 12568-125E4

void DefaultCPUTimerHandler();
void _excepRet(u32 eretpc, u32 v1, u32 a, u32 a1, u32 a2);
int __ExitThread();
int __ExitDeleteThread();
int __SleepThread();
int _iWaitSema(int sid);

void saveContext();
void restoreContext();

int _SemasInit();

void _ThreadHandler(u32 epc, u32 stack);
void _ChangeThread(u32 entry, u32 stack_res, int waitSema);

int __DeleteThread(int tid);
int __StartThread(int tid, void *arg);
int __load_module(char *name, void (*entry)(void*), void *stack_res, int prio);
void* __ExecPS2(void * entry, void * gp, int argc, char ** argv);

void releaseTCB(int tid);
void unsetTCB(int tid);
void thread_2_ready(int tid);

void FlushInstructionCache();
void FlushDataCache();
void FlushSecondaryCache();

void SifDmaInit();

u32 excepRetPc=0;//124E4
int	threadId=0; // 125EC
int	threadPrio=0; // 125F0
int	threadStatus=0; // 125F4

char threadArgStrBuffer[256]; // 12608
u64 hvParam;

u32 machineType;
u64 gsIMR;
u32 memorySize;

u8 g_kernelstack[0x2000] __attribute((aligned(128)));
u32 g_kernelstackend; // hopefully this is right after g_kernelstack

int _HandlersCount;

int VSyncFlag0;
int VSyncFlag1;

struct ll	handler_ll_free, *ihandlers_last=NULL, *ihandlers_first=NULL;
struct HCinfo	intcs_array[14];
struct ll	*dhandlers_last=NULL, *dhandlers_first=NULL;
struct HCinfo	dmacs_array[15];
struct IDhandl	pgifhandlers_array[161];
void (*sbus_handlers[32])(int ca);
void (*CPUTimerHandler)() = DefaultCPUTimerHandler; //12480

u64 _alarm_unk = 0;
u64 rcnt3Valid=0;//16A68
int 		rcnt3Count = 0; //16A70
u32 dword_80016A78, dword_80016A7C, dword_80016A84, dword_80016A88;
//u32 rcnt3gp; //16A80
//u32 rcnt3Mode; //16A84
int 		rcnt3TargetTable[0x142]={0};//16A80
u8		rcnt3TargetNum[0x40];//16F78
int		threads_count=0;

u32  excepRA=0;//16FC0
u32  excepSP=0;//16FD0

struct ll	thread_ll_free;
struct ll	thread_ll_priorities[128];
int		semas_count=0;
struct kSema* semas_last=0;

struct TCB	threads_array[256];
struct kSema	semas_array[256];

char		tagindex=0;//1E104
short		transferscount=0;//1E106
struct TAG tadrptr[31] __attribute((aligned(16)));//1E140
int	extrastorage[(16/4) * 8][31] __attribute((aligned(16)));//1E340

int	osdConfigParam;//1F340

u32 sifEEbuff[32] __attribute((aligned(16)));
u32 sifRegs[32] __attribute((aligned(16)));
u32 sifIOPbuff;
u32 sif1tagdata;

void _eret()
{
        __asm__("mfc0 $26, $12\n"
                "ori $26, 0x13\n"
                "mtc0 $26, $12\n"
                "sync\n"
                "eret\n"
                "nop\n");
}

#define eret() __asm__("j _eret\nnop\n");

////////////////////////////////////////////////////////////////////
//800006C0		SYSCALL 116 RFU116_SetSYSCALL
////////////////////////////////////////////////////////////////////
void _SetSYSCALL(int num, int address)
{
	table_SYSCALL[num] = (void (*)())address;
}

////////////////////////////////////////////////////////////////////
//80000740		SYSCALL 114 SetPgifHandler
////////////////////////////////////////////////////////////////////
void _SetPgifHandler(void (*handler)(int))
{
    INTCTable[15] = handler;
}

////////////////////////////////////////////////////////////////////
//800007C0		SYSCALL 108 SetCPUTimerHandler
////////////////////////////////////////////////////////////////////
void _SetCPUTimerHandler(void (*handler)())
{
    CPUTimerHandler = handler;
}

////////////////////////////////////////////////////////////////////
//80000800		SYSCALL 109 SetCPUTimer
////////////////////////////////////////////////////////////////////
void _SetCPUTimer(int compval)
{
    if( compval < 0 ) {
        u32 status;
        __asm__("mfc0 %0, $12\n" : "=r"(status) : );
        __asm__("mtc0 %0, $12\n"
                "mtc0 $0, $9\n" : : "r"(status|0x18001));
    }
    else if( compval == 0 ) {
        u32 compare, status;
        __asm__("mfc0 %0, $12\n"
                "mfc0 %1, $11\n"
                : "=r"(status), "=r"(compare) : );
        __asm__("mtc0 %0, $12\n"
                "mtc0 $0, $9\n"
                "mtc0 %1, $11\n"
                : : "r"(status&~0x8000), "r"(compare) );
    }
    else {
        __asm__("mtc0 $0, $9\n" // count
                "mtc0 %0, $11\n" // compare
                : : "r"(compval));
    }
}

////////////////////////////////////////////////////////////////////
//800008C0		SYSCALL 020 EnableIntc
////////////////////////////////////////////////////////////////////
int __EnableIntc(int ch)
{
	int intbit;

	intbit = 0x1 << ch;
	if ((INTC_MASK & intbit) != 0) return 0;

    INTC_MASK = intbit;

	return 1;
}

////////////////////////////////////////////////////////////////////
//80000900		SYSCALL 021 DisableIntc
////////////////////////////////////////////////////////////////////
int __DisableIntc(int ch)
{
	int intbit;

	intbit = 0x1 << ch;
	if ((INTC_MASK & intbit) == 0) return 0;

    INTC_MASK = intbit;

	return 1;
}

////////////////////////////////////////////////////////////////////
//80000940		SYSCALL 022 EnableDmac
////////////////////////////////////////////////////////////////////
int __EnableDmac(int ch)
{
	int dmabit;

	dmabit = 0x10000 << ch;
	if ((DMAC_STAT & dmabit) != 0) return 0;

    DMAC_STAT = dmabit;

	return 1;
}

////////////////////////////////////////////////////////////////////
//80000980		SYSCALL 023 DisableDmac
////////////////////////////////////////////////////////////////////
int __DisableDmac(int ch)
{
	int dmabit;

	dmabit = 0x10000 << ch;
	if ((DMAC_STAT & dmabit) == 0) return 0;

    DMAC_STAT = dmabit;

	return 1;
}

////////////////////////////////////////////////////////////////////
//800009C0		SYSCALL 013 SetVTLBRefillHandler
////////////////////////////////////////////////////////////////////
void* _SetVTLBRefillHandler(int cause, void (*handler)())
{
	if ((cause-1) >= 3) return 0;

	VCRTable[cause] = handler;

	return handler;
}

////////////////////////////////////////////////////////////////////
//80000A00		SYSCALL 014 SetVCommonHandler
////////////////////////////////////////////////////////////////////
void* _SetVCommonHandler(int cause, void (*handler)())
{
	if ((cause-4) >= 10) return 0;

	VCRTable[cause] = handler;

	return handler;
}

////////////////////////////////////////////////////////////////////
//80000A40		SYSCALL 015 SetVInterruptHandler
////////////////////////////////////////////////////////////////////
void* _SetVInterruptHandler(int cause, void (*handler)())
{
	if (cause >= 8) return 0;

	VIntTable[cause] = handler;
    // no return value...
}

////////////////////////////////////////////////////////////////////
//80000A80		SYSCALL 102,106 CpuConfig
////////////////////////////////////////////////////////////////////
u32 _CpuConfig(u32 op)
{
    u32 cfg;
    __asm__("mfc0 %0, $16\n" : "=r"(cfg) : );
    return table_CpuConfig[op](cfg);
}

////////////////////////////////////////////////////////////////////
//80000AB8
////////////////////////////////////////////////////////////////////
u32 _CpuConfig_0(u32 cfg)
{
    cfg |= 0x40000;
    __asm__("mtc0 %0, $16\n"
            "sync\n": : "r"(cfg));
    return cfg;
}

////////////////////////////////////////////////////////////////////
//80000AD0
////////////////////////////////////////////////////////////////////
u32 _CpuConfig_1(u32 cfg)
{
    cfg |= 0x2000;
    __asm__("mtc0 %0, $16\n"
            "sync\n": : "r"(cfg));
    return cfg;
}

////////////////////////////////////////////////////////////////////
//80000AEC
////////////////////////////////////////////////////////////////////
u32 _CpuConfig_2(u32 cfg)
{
    cfg |= 0x1000;
    __asm__("mtc0 %0, $16\n"
            "sync\n": : "r"(cfg));
    return cfg;
}

////////////////////////////////////////////////////////////////////
//80000B04
////////////////////////////////////////////////////////////////////
u32 _CpuConfig_3(u32 cfg)
{
    cfg &= ~0x40000;
    __asm__("mtc0 %0, $16\n"
            "sync\n": : "r"(cfg));
    return cfg;
}

////////////////////////////////////////////////////////////////////
//80000B20
////////////////////////////////////////////////////////////////////
u32 _CpuConfig_4(u32 cfg)
{
    cfg &= ~0x2000;
    __asm__("mtc0 %0, $16\n"
            "sync\n": : "r"(cfg));
    return cfg;
}

////////////////////////////////////////////////////////////////////
//80000B40
////////////////////////////////////////////////////////////////////
u32 _CpuConfig_5(u32 cfg)
{
    cfg &= ~0x1000;
    __asm__("mtc0 %0, $16\n"
            "sync\n": : "r"(cfg));
    return cfg;
}

////////////////////////////////////////////////////////////////////
//80000B80		SYSCALL 125 PSMode
////////////////////////////////////////////////////////////////////
void _PSMode()
{
	machineType|= 0x8000;
}

////////////////////////////////////////////////////////////////////
//80000BC0		SYSCALL 126 MachineType
////////////////////////////////////////////////////////////////////
u32 _MachineType()
{
	return machineType;
}

////////////////////////////////////////////////////////////////////
//80000C00
////////////////////////////////////////////////////////////////////
u32 _SetMemorySize(u32 size)
{
	memorySize = size;
	return size;
}

////////////////////////////////////////////////////////////////////
//80000C40		SYSCALL 127 GetMemorySize
////////////////////////////////////////////////////////////////////
u32 _GetMemorySize()
{
	return memorySize;
}

////////////////////////////////////////////////////////////////////
//80000D00		SYSCALL 112 GsGetIMR
////////////////////////////////////////////////////////////////////
u64 _GsGetIMR()
{
    return gsIMR;
}

////////////////////////////////////////////////////////////////////
//80000D40		SYSCALL 113 GsPutIMR
////////////////////////////////////////////////////////////////////
u64 _GsPutIMR(u64 val)
{
    GS_IMR = val;
	gsIMR = val;
    return val;
}

////////////////////////////////////////////////////////////////////
//80000D80		SYSCALL 002 Exit
////////////////////////////////////////////////////////////////////
int _Exit()
{
	return _RFU004_Exit();
}

////////////////////////////////////////////////////////////////////
//80001000		ENTRYPOINT
////////////////////////////////////////////////////////////////////
void __attribute__((noreturn)) _start()
{
	__asm__("lui $sp, %hi(g_kernelstackend)\n"
            "addiu $sp, %lo(g_kernelstackend)\n");

    __puts("EEKERNEL start\n");

	memorySize = 32*1024*1024;
	machineType = 0;
    //memorySize = *(int*)0x70003FF0;
	//machineType = machine_type;

	__printf("TlbInit\n");
	TlbInit();
	__printf("InitPgifHandler\n");
	InitPgifHandler();
	__printf("Initialize\n");
	Initialize();
//	__load_module_EENULL();

	__asm__ (
		"li   $26, %0\n"
        "lw $26, 0($26)\n"
		"mtc0 $26, $12\n"
		"li   $26, %1\n"
        "lw $26, 0($26)\n"
		"mtc0 $26, $16\n"
        : : "i"(SRInitVal), "i"(ConfigInitVal)
	);

//	ee_deci2_manager_init();
	SifDmaInit();
//	__init_unk(0x81FE0);
	SetVSyncFlag(0, 0);

    // call directly instread of loading module, might need to
    // set thread0 as used since soem code assumes thread 0 is always
    // main thread
	//eeload_start();
    //launch_thread(__load_module("EELOAD", 0x82000, 0x81000, 0));

	for (;;);
}

////////////////////////////////////////////////////////////////////
//80001300
////////////////////////////////////////////////////////////////////
void saveContext2() {
	__asm__ (
        ".set noat\n"
		"lui   $26, %hi(SavedRegs)\n"
		"sq    $2,  %lo(SavedRegs+0x000)($26)\n"
		"sq    $3,  %lo(SavedRegs+0x010)($26)\n"
		"sq    $4,  %lo(SavedRegs+0x020)($26)\n"
		"sq    $5,  %lo(SavedRegs+0x030)($26)\n"
		"sq    $6,  %lo(SavedRegs+0x040)($26)\n"
		"sq    $7,  %lo(SavedRegs+0x050)($26)\n"
		"sq    $8,  %lo(SavedRegs+0x060)($26)\n"
		"sq    $9,  %lo(SavedRegs+0x070)($26)\n"
		"sq    $10, %lo(SavedRegs+0x080)($26)\n"
		"sq    $11, %lo(SavedRegs+0x090)($26)\n"
		"sq    $12, %lo(SavedRegs+0x0A0)($26)\n"
		"sq    $13, %lo(SavedRegs+0x0B0)($26)\n"
		"sq    $14, %lo(SavedRegs+0x0C0)($26)\n"
		"sq    $15, %lo(SavedRegs+0x0D0)($26)\n"
		"sq    $16, %lo(SavedRegs+0x0E0)($26)\n"
		"sq    $17, %lo(SavedRegs+0x0F0)($26)\n"
		"sq    $18, %lo(SavedRegs+0x100)($26)\n"
		"sq    $19, %lo(SavedRegs+0x110)($26)\n"
		"sq    $20, %lo(SavedRegs+0x120)($26)\n"
		"sq    $21, %lo(SavedRegs+0x130)($26)\n"
		"sq    $22, %lo(SavedRegs+0x140)($26)\n"
		"sq    $23, %lo(SavedRegs+0x150)($26)\n"
		"sq    $24, %lo(SavedRegs+0x160)($26)\n"
		"sq    $25, %lo(SavedRegs+0x170)($26)\n"
		"sq    $gp, %lo(SavedRegs+0x180)($26)\n"
		"sq    $fp, %lo(SavedRegs+0x190)($26)\n"

		"mfhi  $2\n"
		"sd    $2, %lo(SavedRegs+0x1A0)($26)\n"
		"mfhi1 $2\n"
		"sd    $2, %lo(SavedRegs+0x1A8)($26)\n"

		"mflo  $2\n"
		"sd    $2, %lo(SavedRegs+0x1B0)($26)\n"
		"mflo1 $2\n"
		"sd    $2, %lo(SavedRegs+0x1B8)($26)\n"

		"mfsa  $2\n"
		"sd    $2, %lo(SavedRegs+0x1C0)($26)\n"
        "jr    $31\n"
        "nop\n"
        ".set at\n"
	);
}

////////////////////////////////////////////////////////////////////
//800013C0
////////////////////////////////////////////////////////////////////
void restoreContext2() {
	__asm__ (
        ".set noat\n"
		"lui   $26, %hi(SavedRegs)\n"
		"lq    $2,  %lo(SavedRegs+0x000)($26)\n"
		"lq    $3,  %lo(SavedRegs+0x010)($26)\n"
		"lq    $4,  %lo(SavedRegs+0x020)($26)\n"
		"lq    $5,  %lo(SavedRegs+0x030)($26)\n"
		"lq    $6,  %lo(SavedRegs+0x040)($26)\n"
		"lq    $7,  %lo(SavedRegs+0x050)($26)\n"
		"lq    $8,  %lo(SavedRegs+0x060)($26)\n"
		"lq    $9,  %lo(SavedRegs+0x070)($26)\n"
		"lq    $10, %lo(SavedRegs+0x080)($26)\n"
		"lq    $11, %lo(SavedRegs+0x090)($26)\n"
		"lq    $12, %lo(SavedRegs+0x0A0)($26)\n"
		"lq    $13, %lo(SavedRegs+0x0B0)($26)\n"
		"lq    $14, %lo(SavedRegs+0x0C0)($26)\n"
		"lq    $15, %lo(SavedRegs+0x0D0)($26)\n"
		"lq    $16, %lo(SavedRegs+0x0E0)($26)\n"
		"lq    $17, %lo(SavedRegs+0x0F0)($26)\n"
		"lq    $18, %lo(SavedRegs+0x100)($26)\n"
		"lq    $19, %lo(SavedRegs+0x110)($26)\n"
		"lq    $20, %lo(SavedRegs+0x120)($26)\n"
		"lq    $21, %lo(SavedRegs+0x130)($26)\n"
		"lq    $22, %lo(SavedRegs+0x140)($26)\n"
		"lq    $23, %lo(SavedRegs+0x150)($26)\n"
		"lq    $24, %lo(SavedRegs+0x160)($26)\n"
		"lq    $25, %lo(SavedRegs+0x170)($26)\n"
		"lq    $gp, %lo(SavedRegs+0x180)($26)\n"
		"lq    $fp, %lo(SavedRegs+0x190)($26)\n"

		"ld    $1,  %lo(SavedRegs+0x1A0)($26)\n"
		"mthi  $1\n"
		"ld    $1,  %lo(SavedRegs+0x1A8)($26)\n"
		"mthi1 $1\n"

		"ld    $1,  %lo(SavedRegs+0x1B0)($26)\n"
		"mtlo  $1\n"
		"ld    $1,  %lo(SavedRegs+0x1B8)($26)\n"
		"mtlo1 $1\n"

		"ld    $1,  %lo(SavedRegs+0x1C0)($26)\n"
		"mtsa  $1\n"

		"jr    $31\n"
        "nop\n"
        ".set at\n"
	);
}

////////////////////////////////////////////////////////////////////
//80001460
////////////////////////////////////////////////////////////////////
void erase_cpu_regs()
{
    __asm__(
        ".set noat\n"
        "padduw $1, $0, $0\n"
        "padduw $2, $0, $0\n"
        "padduw $3, $0, $0\n"
        "padduw $4, $0, $0\n"
        "padduw $5, $0, $0\n"
        "padduw $6, $0, $0\n"
        "padduw $7, $0, $0\n"
        "padduw $8, $0, $0\n"
        "padduw $9, $0, $0\n"
        "padduw $10, $0, $0\n"
        "padduw $11, $0, $0\n"
        "padduw $12, $0, $0\n"
        "padduw $13, $0, $0\n"
        "padduw $14, $0, $0\n"
        "padduw $15, $0, $0\n"
        "padduw $16, $0, $0\n"
        "padduw $17, $0, $0\n"
        "padduw $18, $0, $0\n"
        "padduw $19, $0, $0\n"
        "padduw $20, $0, $0\n"
        "padduw $21, $0, $0\n"
        "padduw $22, $0, $0\n"
        "padduw $23, $0, $0\n"
        "padduw $24, $0, $0\n"
        "padduw $25, $0, $0\n"
        "padduw $gp, $0, $0\n"
        "jr $31\n"
        "padduw $fp, $0, $0\n"
        ".set at\n"
    );
}

////////////////////////////////////////////////////////////////////
//800014D8 dummy intc handler
////////////////////////////////////////////////////////////////////
void _DummyINTCHandler(int n)
{
    __printf("# INT: INTC (%d)\n", n);
    while(1) { __asm__ ("nop\nnop\nnop\nnop\n"); }
}

////////////////////////////////////////////////////////////////////
//80001508 dummy dmac handler
////////////////////////////////////////////////////////////////////
void _DummyDMACHandler(int n)
{
    __printf("# INT: DMAC (%d)\n", n);
    while(1) { __asm__ ("nop\nnop\nnop\nnop\n"); }
}

////////////////////////////////////////////////////////////////////
//80001508 
////////////////////////////////////////////////////////////////////
void DefaultCPUTimerHandler()
{
    __printf("# INT: CPU Timer\n");
}

////////////////////////////////////////////////////////////////////
//80001564		SYSCALL 000 RFU000_FullReset
//80001564		SYSCALL 003 RFU003 (also)
//80001564		SYSCALL 008 RFU008 (too)
////////////////////////////////////////////////////////////////////
void _RFU___()
{
	register int syscnum __asm__("$3");
    __printf("# Syscall: undefined (%d)\n", syscnum>>2);
}

////////////////////////////////////////////////////////////////////
//80001588
////////////////////////////////////////////////////////////////////
void _SetVSyncFlag(int flag0, int flag1)
{
	VSyncFlag0 = flag0;
	VSyncFlag1 = flag1;
}

////////////////////////////////////////////////////////////////////
//800015A0
////////////////////////////////////////////////////////////////////
struct ll* _AddHandler()
{
	struct ll *l;

	l = LL_unlink(&handler_ll_free);
	if (l == NULL) return NULL;

	_HandlersCount++;
	return l;
}

////////////////////////////////////////////////////////////////////
//800015E8
////////////////////////////////////////////////////////////////////
void _RemoveHandler(int n)
{
	LL_add(&handler_ll_free, (struct ll*)&pgifhandlers_array[n]);
	_HandlersCount--;
}

////////////////////////////////////////////////////////////////////
//80001630
////////////////////////////////////////////////////////////////////
void DefaultINTCHandler(int n)
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//80001798
////////////////////////////////////////////////////////////////////
void DefaultDMACHandler(int n)
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//800018B0
////////////////////////////////////////////////////////////////////
int __AddIntcHandler(int cause, int (*handler)(int), int next, void *arg, int flag)
{
	struct IDhandl *idh;

	if ((flag != 0) && (cause == INTC_SBUS)) return -1;
	if (cause >= 15) return -1;

	idh = (struct IDhandl *)_AddHandler();
	if (idh == 0) return -1;

	idh->handler = handler;
    __asm__ ("sw $gp, %0\n" : "=m"(idh->gp) : );
	idh->arg     = arg;
	idh->flag    = flag;

	if (next==-1)				//register_last
		LL_add(&ihandlers_last[cause*12], (struct ll*)idh);
	else if (next==0)			//register_first
		LL_add(&ihandlers_first[cause*12], (struct ll*)idh);
	else{
		if (next>128) return -1;
		if (pgifhandlers_array[next].flag==3)	return -1;
		LL_add((struct ll*)&pgifhandlers_array[next], (struct ll*)idh);
	}

	intcs_array[cause].count++;
	return (((u32)idh-(u32)&pgifhandlers_array) * 0xAAAAAAAB) / 8;
}

////////////////////////////////////////////////////////////////////
//80001A30		SYSCALL 016 AddIntcHandler
////////////////////////////////////////////////////////////////////
int _AddIntcHandler(int cause, int (*handler)(int), int next, void *arg)
{
	__AddIntcHandler(cause, handler, next, arg, 2);
}

////////////////////////////////////////////////////////////////////
//80001A50
////////////////////////////////////////////////////////////////////
int _AddIntcHandler2(int cause, int (*handler)(int), int next, void *arg)
{
	__AddIntcHandler(cause, handler, next, arg, 0);
}

////////////////////////////////////////////////////////////////////
//80001AC8		SYSCALL 017 RemoveIntcHandler
////////////////////////////////////////////////////////////////////
int  _RemoveIntcHandler(int cause, int hid)
{
	if (hid >= 128) return -1;

	if (pgifhandlers_array[hid].flag == 3) return -1;
	pgifhandlers_array[hid].flag    = 3;
	pgifhandlers_array[hid].handler = 0;

	LL_unlinkthis((struct ll*)&pgifhandlers_array[hid]);
	_RemoveHandler(hid);

	intcs_array[cause].count--;
}

////////////////////////////////////////////////////////////////////
//80001B10
////////////////////////////////////////////////////////////////////
int __AddDmacHandler(int cause, int (*handler)(int), int next, void *arg, int flag)
{
	struct IDhandl *idh;
	register int temp;

	if (cause >= 16) return -1;

	idh = (struct IDhandl *)_AddHandler();
	if (idh == 0) return -1;

	idh->handler = handler;
	__asm__ ("sw $gp, %0\n" : "=m"(idh->gp) : );
	idh->arg     = arg;
	idh->flag    = flag;

	if (next==-1)				//register_last
		LL_add(&dhandlers_last[cause*12], (struct ll*)idh);
	else if (next==0)			//register_first
		LL_add(&dhandlers_first[cause*12], (struct ll*)idh);
	else{
		if (next>128) return -1;
		if (pgifhandlers_array[next].flag==3)	return -1;
		LL_add((struct ll*)&pgifhandlers_array[next], (struct ll*)idh);
	}

	dmacs_array[cause].count++;
	return (((u32)idh-(u32)&pgifhandlers_array) * 0xAAAAAAAB) / 8;
}

////////////////////////////////////////////////////////////////////
//80001C78		SYSCALL 018 AddDmacHandler
////////////////////////////////////////////////////////////////////
int _AddDmacHandler(int cause, int (*handler)(int), int next, void *arg)
{
	__AddDmacHandler(cause, handler, next, arg, 2);
}

////////////////////////////////////////////////////////////////////
//80001C98
////////////////////////////////////////////////////////////////////
int _AddDmacHandler2(int cause, int (*handler)(int), int next, void *arg)
{
	__AddDmacHandler(cause, handler, next, arg, 0);
}

////////////////////////////////////////////////////////////////////
//80001CB8		SYSCALL 019 RemoveDmacHandler
////////////////////////////////////////////////////////////////////
int  _RemoveDmacHandler(int cause, int hid)
{
	if (hid >= 128) return -1;

	if (pgifhandlers_array[hid].flag == 3) return -1;
	pgifhandlers_array[hid].flag    = 3;
	pgifhandlers_array[hid].handler = 0;

	LL_unlinkthis((struct ll*)&pgifhandlers_array[hid]);
	_RemoveHandler(hid);

	dmacs_array[cause].count--;
}

////////////////////////////////////////////////////////////////////
//80001D58
////////////////////////////////////////////////////////////////////
void sbusHandler()
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//80001E38
////////////////////////////////////////////////////////////////////
int __AddSbusIntcHandler(int cause, void (*handler)(int ca))
{
	if (cause >= 32) return -1;

	if (sbus_handlers[cause] != 0) return -1;

	sbus_handlers[cause] = handler;
	return cause;
}

////////////////////////////////////////////////////////////////////
//80001E78		SYSCALL 010 AddSbusIntcHandler
////////////////////////////////////////////////////////////////////
int _AddSbusIntcHandler(int cause, void (*handler)(int ca))
{
	if (cause < 16) return __AddSbusIntcHandler(cause, handler);

	return -1;
}

////////////////////////////////////////////////////////////////////
//80001EA8
////////////////////////////////////////////////////////////////////
int __RemoveSbusIntcHandler(int cause)
{
	if (cause >= 32) return -1;

	sbus_handlers[cause] = 0;
	return cause;
}

////////////////////////////////////////////////////////////////////
//80001ED0		SYSCALL 011 RemoveSbusIntcHandler
////////////////////////////////////////////////////////////////////
int _RemoveSbusIntcHandler(int cause)
{
	if (cause < 16) return __RemoveSbusIntcHandler(cause);

	return -1;
}

////////////////////////////////////////////////////////////////////
//80001F00
////////////////////////////////////////////////////////////////////
int __Interrupt2Iop(int cause)
{
	if (cause >= 32) {
		return -1;
	}

    SBUS_MSFLG = 1 << cause;

	SBUS_F240 = 0x100;
    SBUS_F240 = 0x100;
    SBUS_F240 = 0x100;
    SBUS_F240 = 0x100;
    SBUS_F240 = 0x100;
    SBUS_F240 = 0x100;
    SBUS_F240 = 0x100;
    SBUS_F240 = 0x100; // eight times

    SBUS_F240 = 0x40100;

	return cause;
}

////////////////////////////////////////////////////////////////////
//80001F70		SYSCALL 012 Interrupt2Iop
////////////////////////////////////////////////////////////////////
int _Interrupt2Iop(int cause)
{
	if (cause < 16) {
		return _Interrupt2Iop(cause);
	} else {
		return -1;
	}
}

////////////////////////////////////////////////////////////////////
//80001FA0		SYSCALL 092 EnableIntcHandler
////////////////////////////////////////////////////////////////////
void _EnableIntcHandler(u32 id)
{
    pgifhandlers_array[id].flag = 2;
}

////////////////////////////////////////////////////////////////////
//80001FA0		SYSCALL 093 DisableIntcHandler
////////////////////////////////////////////////////////////////////
void _DisableIntcHandler(u32 id)
{
    pgifhandlers_array[id].flag = 1;
}

////////////////////////////////////////////////////////////////////
//80001FA0		SYSCALL 094 EnableDmacHandler
////////////////////////////////////////////////////////////////////
void _EnableDmacHandler(u32 id)
{
    pgifhandlers_array[id].flag = 2;
}

////////////////////////////////////////////////////////////////////
//80001FA0		SYSCALL 095 DisableDmacHandler
////////////////////////////////////////////////////////////////////
void _DisableDmacHandler(u32 id)
{
    pgifhandlers_array[id].flag = 1;
}

////////////////////////////////////////////////////////////////////
//80002040     SYSCALL 083 iSetEventFlag
////////////////////////////////////////////////////////////////////
int _iSetEventFlag(int ef, u32 bits)
{
    return _HandlersCount; //?
}

////////////////////////////////////////////////////////////////////
//800022D8      SYSCALL 24,30
////////////////////////////////////////////////////////////////////
int _SetAlarm(short a0, int a1, int a2)
{
	int mode = _alarm_unk & 0x1;
	int i;

	i = 0;
	while (mode) {
		mode = (_alarm_unk >> i++) & 0x1;
		if (i >= 64) {
			return -1;
		}
	}

	_alarm_unk|= mode << i;	

	__asm__("move %0, $gp\n" : "=r"(rcnt3TargetTable[0]) : );
	dword_80016A78 = a1;
	dword_80016A7C = a2;
    rcnt3TargetTable[1] = RCNT3_MODE;
	i = RCNT3_MODE + a0;
	if (i < -1)
		i&= 0xffff;
	dword_80016A88 = i;

	if (RCNT3_MODE < i) {
		if (rcnt3Count <= 0) {
			
		}
	}
}

////////////////////////////////////////////////////////////////////
//80002570      SYSCALL 25,31 ReleaseAlarm
////////////////////////////////////////////////////////////////////
void _ReleaseAlarm()
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//80002650
////////////////////////////////////////////////////////////////////
void rcnt3Handler()
{
	unsigned int i;
	u32* ptr;
    u32 storegp;
    u32 addr;

	if (rcnt3Count < 2) {
        RCNT3_MODE = 0x483;
	} else {
        RCNT3_MODE = 0x583;
		RCNT3_TARGET = rcnt3TargetTable[2+rcnt3TargetNum[1] * 5];
	}

	for (;;) {
        u32 id = (u32)rcnt3TargetNum[0];
		if (--rcnt3Count >= 0) {
            // shift one down
			for (i=0; i<rcnt3Count; i++) {
				rcnt3TargetNum[i] = rcnt3TargetNum[i+1];
			}
		}

        rcnt3Valid &= ~(1 << id);
		ptr = &rcnt3TargetTable[id * 5];
        addr =  (u32)*(u16*)(ptr+2);
		__asm__("move %0, $gp\n" : "=r"(storegp) : );
        __asm__("move $gp, %0\n" : : "r"(ptr[0]) );
        
        _excepRet(excepRetPc, ptr[-2], id, addr, ptr[-1]);

		__asm__("move $gp, %0\n" : : "r"(storegp) );

		if (rcnt3Count >= 0) {
			if (addr != rcnt3TargetTable[rcnt3TargetNum[0] * 5 + 2]) {
				break;
			}
		} else break;
	}
}

////////////////////////////////////////////////////////////////////
//800027F8     SYSCALL 082 SetEventFlag
////////////////////////////////////////////////////////////////////
int _SetEventFlag(int ef, u32 bits)
{
    return rcnt3Count; //?
}

////////////////////////////////////////////////////////////////////
//800027F8
////////////////////////////////////////////////////////////////////
void _InitRCNT3()
{
	int i;

	rcnt3Count = 0;
    rcnt3Valid = 0;
	_alarm_unk = 0;
	for (i=0; i<0x40; i++) rcnt3TargetNum[i] = 0;

	__EnableIntc(INTC_TIM3);
}

////////////////////////////////////////////////////////////////////
//80002840
////////////////////////////////////////////////////////////////////
void _excepRet(u32 eretpc, u32 v1, u32 a, u32 a1, u32 a2)
{
    __asm__("sw $31, %0\n"
            "sw $sp, %1\n"
            : "=m"(excepRA), "=m"(excepSP) : );
    __asm__("mtc0 $4, $14\n"
            "sync\n"
            "daddu $3, $5, $0\n"
            "daddu $4, $6, $0\n"
            "daddu $5, $7, $0\n"
            "daddu $6, $8, $0\n"
            "mfc0 $26, $12\n"
            "ori $26, 0x12\n"
            "mtc0 $26, $12\n"
            "sync\n"
            "eret\n"
            "nop\n");    
}

////////////////////////////////////////////////////////////////////
//80002880		SYSCALL 005 RFU005
////////////////////////////////////////////////////////////////////
void _RFU005()
{
    __asm__ (
		".set noat\n"

		"mfc0 $26, $12\n"
		"ori  $1,  $0, 0xFFE4\n"
		"and  $26, $1\n"
		"mtc0 $26, $12\n"
		"sync\n"

		"lw   $ra, excepRA\n"
		"lw   $sp, excepSP\n"
		"jr   $ra\n"
        "nop\n"
        
		".set at\n");
}

////////////////////////////////////////////////////////////////////
//A00028C0     SYSCALL 097 EnableCache
////////////////////////////////////////////////////////////////////
int _EnableCache(int cache)
{
    u32 cfg;
    __asm__("mfc0 %0, $16\n" : "=r"(cfg) : );
    cfg |= ((cache&3)<<16);
    __asm__("mtc0 %0, $16\n"
            "sync\n" : : "r"(cfg) );
}

////////////////////////////////////////////////////////////////////
//A0002980     SYSCALL 098 DisableCache
////////////////////////////////////////////////////////////////////
int _DisableCache(int cache)
{
        u32 cfg;
    __asm__("mfc0 %0, $16\n" : "=r"(cfg) : );
    cfg &= ~((cache&3)<<16);
    __asm__("mtc0 %0, $16\n"
            "sync\n" : : "r"(cfg) );
}

////////////////////////////////////////////////////////////////////
//A0002A40     SYSCALL 100 FlushCache
////////////////////////////////////////////////////////////////////
void _FlushCache(int op)
{
    if( op == 0 ) FlushInstructionCache();
    else if ( op == 1 ) FlushSecondaryCache();
    else if( op == 2 ) FlushDataCache();
    else {
        FlushSecondaryCache();
        FlushDataCache();
    }
}

////////////////////////////////////////////////////////////////////
//80002AC0
////////////////////////////////////////////////////////////////////
void FlushInstructionCache()
{
}

////////////////////////////////////////////////////////////////////
//80002A80
////////////////////////////////////////////////////////////////////
void FlushDataCache()
{
}

////////////////////////////////////////////////////////////////////
//80002B00
////////////////////////////////////////////////////////////////////
void FlushSecondaryCache()
{
}

////////////////////////////////////////////////////////////////////
//A0002B40     SYSCALL 101,105 _105
////////////////////////////////////////////////////////////////////
void _105(int op1, int op2)
{
    // flushing caches again
}

////////////////////////////////////////////////////////////////////
//A0002C00     SYSCALL 096 KSeg0
////////////////////////////////////////////////////////////////////
void _KSeg0(u32 arg)
{
    u32 cfg;
    __asm__("mfc0 %0, $16\n" : "=r"(cfg) : );
    cfg = (arg&3)&((cfg>>3)<<3); // yes it is 0, don't ask
    __asm__("mtc0 %0, $16\n"
            "sync\n" : : "r"(cfg) );
}

////////////////////////////////////////////////////////////////////
//80002C40		SYSCALL 099 GetCop0
////////////////////////////////////////////////////////////////////
int _GetCop0(int reg)
{ 
	return table_GetCop0[reg](reg);
}

////////////////////////////////////////////////////////////////////
//80002C58-80002D50	calls for table_GetCop0
////////////////////////////////////////////////////////////////////
int GetCop0_Index(int reg) 	{ __asm__(" mfc0 $2, $0\n"); }
int GetCop0_Random(int reg) 	{ __asm__(" mfc0 $2, $1\n"); }
int GetCop0_EntryLo0(int reg) 	{ __asm__(" mfc0 $2, $2\n"); }
int GetCop0_EntryLo1(int reg) 	{ __asm__(" mfc0 $2, $3\n"); }

int GetCop0_Context(int reg) 	{ __asm__(" mfc0 $2, $4\n"); }
int GetCop0_PageMask(int reg) 	{ __asm__(" mfc0 $2, $5\n"); }
int GetCop0_Wired(int reg) 	{ __asm__(" mfc0 $2, $6\n"); }
int GetCop0_Reg7(int reg) 	{ return; }

int GetCop0_BadVAddr(int reg) 	{ __asm__(" mfc0 $2, $8\n"); }
int GetCop0_Count(int reg) 	{ __asm__(" mfc0 $2, $9\n"); }
int GetCop0_EntryHi(int reg) 	{ __asm__(" mfc0 $2, $10\n"); }
int GetCop0_Compare(int reg) 	{ __asm__(" mfc0 $2, $11\n"); }

int GetCop0_Status(int reg) 	{ __asm__(" mfc0 $2, $12\n"); }
int GetCop0_Cause(int reg) 	{ __asm__(" mfc0 $2, $13\n"); }
int GetCop0_ExceptPC(int reg) 	{ __asm__(" mfc0 $2, $14\n"); }
int GetCop0_PRevID(int reg) 	{ __asm__(" mfc0 $2, $15\n"); }

int GetCop0_Config(int reg) 	{ __asm__(" mfc0 $2, $16\n"); }
int GetCop0_Reg17(int reg) 	{ return; }
int GetCop0_Reg18(int reg) 	{ return; }
int GetCop0_Reg19(int reg) 	{ return; }

int GetCop0_Reg20(int reg) 	{ return; }
int GetCop0_Reg21(int reg) 	{ return; }
int GetCop0_Reg22(int reg) 	{ return; }
int GetCop0_Reg23(int reg) 	{ __asm__(" mfc0 $2, $23\n"); }

int GetCop0_DebugReg24(int reg)	{ __asm__(" mfc0 $2, $24\n"); }
int GetCop0_Perf(int reg) 	{ __asm__(" mfc0 $2, $25\n"); }
int GetCop0_Reg26(int reg) 	{ return; }
int GetCop0_Reg27(int reg) 	{ return; }

int GetCop0_TagLo(int reg) 	{ __asm__(" mfc0 $2, $28\n"); }
int GetCop0_TagHi(int reg) 	{ __asm__(" mfc0 $2, $29\n"); }
int GetCop0_ErrorPC(int reg) 	{ __asm__(" mfc0 $2, $30\n"); }
int GetCop0_Reg31(int reg) 	{ return; }

int (*table_GetCop0[32])(int reg) = { // 800124E8
	GetCop0_Index,      GetCop0_Random,   GetCop0_EntryLo0, GetCop0_EntryLo1,
	GetCop0_Context,    GetCop0_PageMask, GetCop0_Wired,    GetCop0_Reg7,
	GetCop0_BadVAddr,   GetCop0_Count,    GetCop0_EntryHi,  GetCop0_Compare, 
	GetCop0_Status,     GetCop0_Cause,    GetCop0_ExceptPC, GetCop0_PRevID,
	GetCop0_Config,     GetCop0_Reg17,    GetCop0_Reg18,    GetCop0_Reg19,
	GetCop0_Reg20,      GetCop0_Reg21,    GetCop0_Reg22,    GetCop0_Reg23,
	GetCop0_DebugReg24, GetCop0_Perf,     GetCop0_Reg26,    GetCop0_Reg27, 
	GetCop0_TagLo,      GetCop0_TagHi,    GetCop0_ErrorPC,  GetCop0_Reg31
};

////////////////////////////////////////////////////////////////////
//80002D80
////////////////////////////////////////////////////////////////////
int SetCop0(int reg, int val)
{
	return table_SetCop0[reg](reg, val);
}

////////////////////////////////////////////////////////////////////
//80002D98-80002F74	calls for table_SetCop0
////////////////////////////////////////////////////////////////////
int SetCop0_Index(int reg, int val) 	{ __asm__(" mfc0 $2, $0\nmtc0 %0, $0\nsync\n" : : "r"(val)); }
int SetCop0_Random(int reg, int val) 	{ return -1; }
int SetCop0_EntryLo0(int reg, int val) 	{ __asm__(" mfc0 $2, $2\nmtc0 %0, $2\nsync\n" : : "r"(val)); }
int SetCop0_EntryLo1(int reg, int val) 	{ __asm__(" mfc0 $2, $3\nmtc0 %0, $3\nsync\n" : : "r"(val)); }

int SetCop0_Context(int reg, int val) 	{ __asm__(" mfc0 $2, $4\nmtc0 %0, $4\nsync\n" : : "r"(val)); }
int SetCop0_PageMask(int reg, int val) 	{ __asm__(" mfc0 $2, $5\nmtc0 %0, $5\nsync\n" : : "r"(val)); }
int SetCop0_Wired(int reg, int val) 	{ __asm__(" mfc0 $2, $6\nmtc0 %0, $6\nsync\n" : : "r"(val)); }
int SetCop0_Reg7(int reg, int val) 	{ return -1; }

int SetCop0_BadVAddr(int reg, int val) 	{ return -1; }
int SetCop0_Count(int reg, int val) 	{ __asm__(" mfc0 $2, $9\nmtc0 %0, $9\nsync\n" : : "r"(val)); }
int SetCop0_EntryHi(int reg, int val) 	{ __asm__(" mfc0 $2, $10\nmtc0 %0, $10\nsync\n" : : "r"(val)); }
int SetCop0_Compare(int reg, int val) 	{ __asm__(" mfc0 $2, $11\nmtc0 %0, $11\nsync\n" : : "r"(val)); }

int SetCop0_Status(int reg, int val) 	{ __asm__(" mfc0 $2, $12\nmtc0 %0, $12\nsync\n" : : "r"(val)); }
int SetCop0_Cause(int reg, int val) 	{ return -1; }
int SetCop0_ExceptPC(int reg, int val) 	{ __asm__(" mfc0 $2, $14\nmtc0 %0, $14\nsync\n" : : "r"(val)); }
int SetCop0_PRevID(int reg, int val) 	{ return -1; }

int SetCop0_Config(int reg, int val) 	{ __asm__(" mfc0 $2, $16\nmtc0 %0, $16\nsync\n" : : "r"(val)); }
int SetCop0_Reg17(int reg, int val) 	{ return -1; }
int SetCop0_Reg18(int reg, int val) 	{ return -1; }
int SetCop0_Reg19(int reg, int val) 	{ return -1; }

int SetCop0_Reg20(int reg, int val) 	{ return -1; }
int SetCop0_Reg21(int reg, int val) 	{ return -1; }
int SetCop0_Reg22(int reg, int val) 	{ return -1; }
int SetCop0_Reg23(int reg, int val) 	{ __asm__(" mfc0 $2, $23\nmtc0 %0, $23\nsync\n" : : "r"(val)); }

int SetCop0_DebugReg24(int reg, int val){ __asm__(" mfc0 $2, $24\nmtc0 %0, $24\nsync\n" : : "r"(val)); }
int SetCop0_Perf(int reg, int val)      { __asm__(" mfc0 $2, $25\nmtc0 %0, $25\nsync\n" : : "r"(val)); }
int SetCop0_Reg26(int reg, int val) 	{ return -1; }
int SetCop0_Reg27(int reg, int val) 	{ return -1; }

int SetCop0_TagLo(int reg, int val) 	{ __asm__(" mfc0 $2, $28\nmtc0 %0, $28\nsync\n" : : "r"(val)); }
int SetCop0_TagHi(int reg, int val) 	{ __asm__(" mfc0 $2, $29\nmtc0 %0, $29\nsync\n" : : "r"(val)); }
int SetCop0_ErrorPC(int reg, int val) 	{ __asm__(" mfc0 $2, $30\nmtc0 %0, $30\nsync\n" : : "r"(val)); }
int SetCop0_Reg31(int reg, int val) 	{ return -1; }

int (*table_SetCop0[32])(int reg, int val) = { // 80012568
	SetCop0_Index,      SetCop0_Random,   SetCop0_EntryLo0, SetCop0_EntryLo1,
	SetCop0_Context,    SetCop0_PageMask, SetCop0_Wired,    SetCop0_Reg7,
	SetCop0_BadVAddr,   SetCop0_Count,    SetCop0_EntryHi,  SetCop0_Compare, 
	SetCop0_Status,     SetCop0_Cause,    SetCop0_ExceptPC, SetCop0_PRevID,
	SetCop0_Config,     SetCop0_Reg17,    SetCop0_Reg18,    SetCop0_Reg19,
	SetCop0_Reg20,      SetCop0_Reg21,    SetCop0_Reg22,    SetCop0_Reg23,
	SetCop0_DebugReg24, SetCop0_Perf,     SetCop0_Reg26,    SetCop0_Reg27, 
	SetCop0_TagLo,      SetCop0_TagHi,    SetCop0_ErrorPC,  SetCop0_Reg31
};

////////////////////////////////////////////////////////////////////
//80002F80		SYSCALL 007 ExecPS2
////////////////////////////////////////////////////////////////////
int _ExecPS2(void * entry, void * gp, int argc, char ** argv)
{
	saveContext();
    __ExecPS2(entry, gp, argc, argv);
	__asm__("mtc0 $2, $14\n"
            "move $sp, %0\n"
            "sd $2, 0x20($sp)\n"
            : : "r"(SavedSP));

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80002FC0		SYSCALL 033 DeleteThread
////////////////////////////////////////////////////////////////////
int _DeleteThread(int tid)
{
    register int ret __asm__("$2");
    register u32 curepc __asm__("$4");

	saveContext();

	ret = __DeleteThread(tid);
	if (ret < 0) {
        __asm__("lw $sp, %0\n" : : "m"(SavedSP));
        // make sure the return value is also stored
        __asm__("sd $2, 0x20($sp)\n");

        restoreContext();        
        eret();
    }

        
    __asm__("mfc0 %0, $14\n" : "=r"(curepc) : ); // EPC
    _ThreadHandler(curepc, SavedSP); // returns entry in $3, stack in $4
    
    __asm__("mtc0 $2, $14\n"
            "move $sp, $3\n");

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003040		SYSCALL 034 StartThread
////////////////////////////////////////////////////////////////////
int _StartThread(int tid, void *arg)
{
	register int ret __asm__("$2");
    register u32 curepc __asm__("$4");

	saveContext();

	ret = __StartThread(tid, arg);
    
    if (ret < 0) {
        __asm__("lw $sp, %0\n" : : "m"(SavedSP));
        // make sure the return value is also stored
        __asm__("sd $2, 0x20($sp)\n");

        restoreContext();
        eret();
    }

        
    __asm__("mfc0 %0, $14\n" : "=r"(curepc) : ); // EPC
    _ThreadHandler(curepc, SavedSP); // returns entry in $3, stack in $4
    
    __asm__("mtc0 $2, $14\n"
            "move $sp, $3\n");

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//800030C0		SYSCALL 035 ExitThread
////////////////////////////////////////////////////////////////////
int _ExitThread()
{
	saveContext();

	__ExitThread();
    __asm__("mtc0 $2, $14\n"
            "sync\n"
            "move $sp, $3\n");

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003100		SYSCALL 036 ExitDeleteThread
////////////////////////////////////////////////////////////////////
int _ExitDeleteThread()
{
	saveContext();

	__ExitDeleteThread();
    __asm__("mtc0 $2, $14\n"
            "sync\n"
            "move $sp, $3\n");

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003140      SYSCALL 037 TerminateThread
////////////////////////////////////////////////////////////////////
int _TerminateThread(int tid)
{
	register int ret __asm__("$2");
    register u32 curepc __asm__("$4");

	saveContext();

	ret = _iTerminateThread(tid);

    if( ret < 0 ) {
        __asm__("lw $sp, %0\n" : : "m"(SavedSP));
        // make sure the return value is also stored
        __asm__("sd $2, 0x20($sp)\n");

        restoreContext();
        eret();
    }
    
    __asm__("mfc0 %0, $14\n" : "=r"(curepc) : ); // EPC
    _ThreadHandler(curepc, SavedSP); // returns entry in $3, stack in $4
    
    __asm__("mtc0 $2, $14\n"
            "move $sp, $3\n");

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//800031C0		SYSCALL 043 RotateThreadReadyQueue
////////////////////////////////////////////////////////////////////
void _RotateThreadReadyQueue(int pri)
{
    register int ret __asm__("$2");
    register u32 curepc __asm__("$4");

    ret = pri;
	saveContext();

	ret = _iRotateThreadReadyQueue(pri);

    if( ret < 0 ) {
        __asm__("lw $sp, %0\n" : : "m"(SavedSP));
        // make sure the return value is also stored
        __asm__("sd $2, 0x20($sp)\n");

        restoreContext();
        eret();
    }
    
    __asm__("mfc0 %0, $14\n" : "=r"(curepc) : ); // EPC
    _ThreadHandler(curepc, SavedSP); // returns entry in $3, stack in $4
    
    __asm__("mtc0 $2, $14\n"
            "move $sp, $3\n");

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003240		SYSCALL 045 ReleaseWaitThread
////////////////////////////////////////////////////////////////////
void _ReleaseWaitThread(int tid)
{
    register int ret __asm__("$2");
    register u32 curepc __asm__("$4");

    ret = tid;
	saveContext();

	ret = _iReleaseWaitThread(tid);

    if( ret < 0 ) {
        __asm__("lw $sp, %0\n" : : "m"(SavedSP));
        // make sure the return value is also stored
        __asm__("sd $2, 0x20($sp)\n");

        restoreContext();
        eret();
    }
    
    __asm__("mfc0 %0, $14\n" : "=r"(curepc) : ); // EPC
    _ThreadHandler(curepc, SavedSP); // returns entry in $3, stack in $4
    
    __asm__("mtc0 $2, $14\n"
            "move $sp, $3\n");

	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//800032C0		SYSCALL 052 SleepThread
////////////////////////////////////////////////////////////////////
int _SleepThread()
{
	register int ret __asm__("$2");

	ret = threadId;
	saveContext();

	ret = __SleepThread();
	if (ret < 0) {
		register int curepc __asm__("$4");
        __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
		_ChangeThread(curepc, SavedSP, 1);
        
        __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
		restoreContext();
        eret();
	}

    __asm__("lw $sp, %0\n"
            "sd %1, 0x20($sp)\n" : : "m"(SavedSP), "r"(ret) );
    restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003340		SYSCALL 051 WakeupThread
////////////////////////////////////////////////////////////////////
int _WakeupThread(int tid)
{
	register int ret __asm__("$2");

	ret = tid;
	saveContext();
    
	ret = iWakeupThread(tid);
    if( ret >= 0 ) {
        register int curepc __asm__("$4");
        __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
        _ThreadHandler(curepc, SavedSP);
        
        __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
		restoreContext();
        eret();
    }

    __asm__("lw $sp, %0\n"
            "sd %1, 0x20($sp)\n" : : "m"(SavedSP), "r"(ret) );
    restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//800033C0		SYSCALL 057 ResumeThread
////////////////////////////////////////////////////////////////////
int _ResumeThread(int tid)
{
	register int ret __asm__("$2");

	ret = tid;
	saveContext();
    
	ret = _iResumeThread(tid);
    if( ret >= 0 ) {
        register int curepc __asm__("$4");
        __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
        _ThreadHandler(curepc, SavedSP);
        
        __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
		restoreContext();
        eret();
    }

    __asm__("lw $sp, %0\n"
            "sd %1, 0x20($sp)\n" : : "m"(SavedSP), "r"(ret) );
    restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003440		SYSCALL 068 WaitSema
////////////////////////////////////////////////////////////////////
int _WaitSema(int sid)
{
    register int ret __asm__("$2");

	ret = sid;
	saveContext();
    
	ret = _iWaitSema(sid);
    if( ret == 0xFFFFFFFE ) {
        register int curepc __asm__("$4");
        __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
        _ChangeThread(curepc, SavedSP, 2);
        
        __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
		restoreContext();
        eret();
    }

    __asm__("lw $sp, %0\n" : : "m"(SavedSP));
    restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//800034C0		SYSCALL 066 SignalSema
////////////////////////////////////////////////////////////////////
void _SignalSema(int sid)
{
    register int ret __asm__("$2");

	ret = sid;
	saveContext();
    
	ret = _iSignalSema(sid);
    if( ret >= 0 ) {
        register int curepc __asm__("$4");
        __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
        _ThreadHandler(curepc, SavedSP);
        
        __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
		restoreContext();
        eret();
    }

    __asm__("lw $sp, %0\n"
            "sd %1, 0x20($sp)\n" : : "m"(SavedSP), "r"(ret) );
    restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003540		SYSCALL 065 DeleteSema
////////////////////////////////////////////////////////////////////
int _DeleteSema(int sid)
{
    register int ret __asm__("$2");

	ret = sid;
	saveContext();
    
	ret = _iDeleteSema(sid);
    if( ret >= 0 ) {
        register int curepc __asm__("$4");
        __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
        _ThreadHandler(curepc, SavedSP);
        
        __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
		restoreContext();
        eret();
    }

    __asm__("lw $sp, %0\n"
            "sd %1, 0x20($sp)\n" : : "m"(SavedSP), "r"(ret) );
    restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//800035C0		SYSCALL 041 ChangeThreadPriority
//return has to be void (even though it fills in v0)
////////////////////////////////////////////////////////////////////
void _ChangeThreadPriority(int tid, int prio)
{
	register int ret __asm__("$2");

	saveContext();
	ret = _iChangeThreadPriority(tid, prio);

    __asm__("lw $26, %0\n"
            "sd %1, 0x20($26)\n" : : "m"(SavedSP), "r"(ret) );

	if (ret>=0){
        register int curepc __asm__("$4");
        __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
        _ThreadHandler(curepc, SavedSP);
        
        __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
		restoreContext();
        eret();
	}
    
    // why twice?
    __asm__("lw $sp, %0\n"
            "sd %1, 0x20($sp)\n" : : "m"(SavedSP), "r"(ret) );
	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//8000363C
////////////////////////////////////////////////////////////////////
void __ThreadHandler()
{
    register int curepc __asm__("$4");

	saveContext();

    __asm__("mfc0 %0, $14\n" : "=r"(curepc) : );
	_ThreadHandler(curepc, SavedSP);
    __asm__("mtc0 $2, $14\n"
                "sync\n"
                "move $sp, $3\n");
	restoreContext();
    eret();
}

////////////////////////////////////////////////////////////////////
//80003680
////////////////////////////////////////////////////////////////////
void saveContext()
{
	__asm__ (
		"lui   $26, %hi(SavedSP)\n"
		"lq    $26, %lo(SavedSP)($26)\n"
	);
	__asm__ (
		"addiu $26, %0\n"
		: : "i"(-sizeof(struct threadCtx))
	);
	__asm__ (
		".set noat\n"
		"lui   $1,  %hi(SavedAT)\n"
		"lq    $1,  %lo(SavedAT)($1)\n"
		"sq    $1,  0x000($26)\n"
		"sq    $2,  0x010($26)\n"
		"sq    $3,  0x020($26)\n"
		"sq    $4,  0x030($26)\n"
		"sq    $5,  0x040($26)\n"
		"sq    $6,  0x050($26)\n"
		"sq    $7,  0x060($26)\n"
		"sq    $8,  0x070($26)\n"
		"sq    $9,  0x080($26)\n"
		"sq    $10, 0x090($26)\n"
		"sq    $11, 0x0A0($26)\n"
		"sq    $12, 0x0B0($26)\n"
		"sq    $13, 0x0C0($26)\n"
		"sq    $14, 0x0D0($26)\n"
		"sq    $15, 0x0E0($26)\n"
		"sq    $16, 0x0F0($26)\n"
		"sq    $17, 0x100($26)\n"
		"sq    $18, 0x110($26)\n"
		"sq    $19, 0x120($26)\n"
		"sq    $20, 0x130($26)\n"
		"sq    $21, 0x140($26)\n"
		"sq    $22, 0x150($26)\n"
		"sq    $23, 0x160($26)\n"
		"sq    $24, 0x170($26)\n"
		"sq    $25, 0x180($26)\n"
		"sq    $gp, 0x190($26)\n"
		"lui   $1,  %hi(SavedSP)\n"
		"lq    $sp, %lo(SavedSP)($1)\n"
		"sq    $sp, 0x1A0($26)\n"
		"sq    $fp, 0x1B0($26)\n"
		"lui   $1,  %hi(SavedRA)\n"
		"lq    $1, %lo(SavedRA)($1)\n"
		"sq    $1, 0x1C0($26)\n"

		"swc1  $0,  0x200($26)\n"
		"swc1  $1,  0x204($26)\n"
		"swc1  $2,  0x208($26)\n"
		"swc1  $3,  0x20C($26)\n"
		"swc1  $4,  0x210($26)\n"
		"swc1  $5,  0x214($26)\n"
		"swc1  $6,  0x218($26)\n"
		"swc1  $7,  0x21C($26)\n"
		"swc1  $8,  0x220($26)\n"
		"swc1  $9,  0x224($26)\n"
		"swc1  $10, 0x228($26)\n"
		"swc1  $11, 0x22C($26)\n"
		"swc1  $12, 0x230($26)\n"
		"swc1  $13, 0x234($26)\n"
		"swc1  $14, 0x238($26)\n"
		"swc1  $15, 0x23C($26)\n"
		"swc1  $16, 0x240($26)\n"
		"swc1  $17, 0x244($26)\n"
		"swc1  $18, 0x248($26)\n"
		"swc1  $19, 0x24C($26)\n"
		"swc1  $20, 0x250($26)\n"
		"swc1  $21, 0x254($26)\n"
		"swc1  $22, 0x258($26)\n"
		"swc1  $23, 0x25C($26)\n"
		"swc1  $24, 0x260($26)\n"
		"swc1  $25, 0x264($26)\n"
		"swc1  $26, 0x268($26)\n"
		"swc1  $27, 0x26C($26)\n"
		"swc1  $28, 0x270($26)\n"
		"swc1  $29, 0x274($26)\n"
		"swc1  $30, 0x278($26)\n"
		"swc1  $31, 0x27C($26)\n"

        "mfsa  $1\n"
		"sw    $1, 0x1F0($26)\n"

		"cfc1  $1, $31\n"
		"sw    $1, 0x1F4($26)\n"

        "lui   $1, 0x8000\n"
        "mtc1  $1, $1\n"
        "mtc1  $1, $0\n"
        "madd.s $f0, $f0, $f1\n"
        "swc1 $0, 0x1F8($26)\n"

        "mfhi  $1\n"
		"sd    $1, 0x1D0($26)\n"
		"mfhi1 $1\n"
		"sd    $1, 0x1D8($26)\n"

		"mflo  $1\n"
		"sd    $1, 0x1E0($26)\n"
		"mflo1 $1\n"
		"sd    $1, 0x1E8($26)\n"

		"lui   $1,  %hi(SavedSP)\n"
		"sw    $26, %lo(SavedSP)($1)\n"

        "jr $31\n"
        "lui $1, 0x8000\n"
		".set at\n"
	);
}

////////////////////////////////////////////////////////////////////
//80003800
////////////////////////////////////////////////////////////////////
void restoreContext()
{
	__asm__ (
		".set noat\n"

        "lui  $26,  0x8000\n"
        "mtc1 $26,  $1\n"
        "lwc1 $0,   0x1F8($sp)\n"
        "adda.s $f0, $f1\n"
        
        "lw    $26,  0x1F0($sp)\n"
		"mtsa  $26\n"

        "lw    $26, 0x1F4($sp)\n"
        "ctc1  $26, $31\n"

        "ld    $26,  0x1D0($sp)\n"
		"mthi  $26\n"
		"ld    $26,  0x1D8($sp)\n"
		"mthi1 $26\n"

		"ld    $26,  0x1E0($sp)\n"
		"mtlo  $26\n"
		"ld    $26,  0x1E8($sp)\n"
		"mtlo1 $26\n"

		"lq    $1,  0x000($sp)\n"
		"lq    $2,  0x010($sp)\n"
		"lq    $3,  0x020($sp)\n"
		"lq    $4,  0x030($sp)\n"
		"lq    $5,  0x040($sp)\n"
		"lq    $6,  0x050($sp)\n"
		"lq    $7,  0x060($sp)\n"
		"lq    $8,  0x070($sp)\n"
		"lq    $9,  0x080($sp)\n"
		"lq    $10, 0x090($sp)\n"
		"lq    $11, 0x0A0($sp)\n"
		"lq    $12, 0x0B0($sp)\n"
		"lq    $13, 0x0C0($sp)\n"
		"lq    $14, 0x0D0($sp)\n"
		"lq    $15, 0x0E0($sp)\n"
		"lq    $16, 0x0F0($sp)\n"
		"lq    $17, 0x100($sp)\n"
		"lq    $18, 0x110($sp)\n"
		"lq    $19, 0x120($sp)\n"
		"lq    $20, 0x130($sp)\n"
		"lq    $21, 0x140($sp)\n"
		"lq    $22, 0x150($sp)\n"
		"lq    $23, 0x160($sp)\n"
		"lq    $24, 0x170($sp)\n"
        "lq    $25, 0x180($sp)\n"
		"lq    $gp, 0x190($sp)\n"
		"lq    $fp, 0x1B0($sp)\n"

        "lwc1  $0,  0x200($sp)\n"
        "lwc1  $1,  0x204($sp)\n"
        "lwc1  $2,  0x208($sp)\n"
        "lwc1  $3,  0x20C($sp)\n"
        "lwc1  $4,  0x210($sp)\n"
        "lwc1  $5,  0x214($sp)\n"
        "lwc1  $6,  0x218($sp)\n"
        "lwc1  $7,  0x21C($sp)\n"
        "lwc1  $8,  0x220($sp)\n"
        "lwc1  $9,  0x224($sp)\n"
        "lwc1  $10,  0x228($sp)\n"
        "lwc1  $11,  0x22C($sp)\n"
        "lwc1  $12,  0x230($sp)\n"
        "lwc1  $13,  0x234($sp)\n"
        "lwc1  $14,  0x238($sp)\n"
        "lwc1  $15,  0x23C($sp)\n"
        "lwc1  $16,  0x240($sp)\n"
        "lwc1  $17,  0x244($sp)\n"
        "lwc1  $18,  0x248($sp)\n"
        "lwc1  $19,  0x24C($sp)\n"
        "lwc1  $20,  0x250($sp)\n"
        "lwc1  $21,  0x254($sp)\n"
        "lwc1  $22,  0x258($sp)\n"
        "lwc1  $23,  0x25C($sp)\n"
        "lwc1  $24,  0x260($sp)\n"
        "lwc1  $25,  0x264($sp)\n"
        "lwc1  $26,  0x268($sp)\n"
        "lwc1  $27,  0x26C($sp)\n"
        "lwc1  $28,  0x270($sp)\n"
        "lwc1  $29,  0x274($sp)\n"
        "lwc1  $30,  0x278($sp)\n"
        "lwc1  $31,  0x27C($sp)\n"
        "daddu $26, $31, $0\n"
        "lq    $31,  0x1C0($sp)\n"
        "jr    $26\n"
        "lq    $sp, 0x1A0($sp)\n"
		".set at\n"
	);
}

////////////////////////////////////////////////////////////////////
//80003940
////////////////////////////////////////////////////////////////////
void _ThreadHandler(u32 epc, u32 stack)
{
	register int tid;

	threads_array[threadId].entry		=(void*)epc;
	threads_array[threadId].status		=THS_READY;
	threads_array[threadId].stack_res =(void*)stack;

	for ( ; threadPrio < 129; threadPrio++)
		if ((thread_ll_priorities[threadPrio].next !=
		     &thread_ll_priorities[threadPrio]) ||
		    (thread_ll_priorities[threadPrio].prev !=
		     &thread_ll_priorities[threadPrio])){
			tid=threadId=(( (u32)thread_ll_priorities[threadPrio].prev -
                            (u32)threads_array)*0x286BCA1B)>>2;
			break;
		}

	if (threadPrio>=129){
		__printf("# <Thread> No active threads\n");
		Exit(1);
		tid=0;
	}

	threads_array[tid].status=THS_RUN;

	if (threads_array[tid].waitSema){
		threads_array[tid].waitSema=0;
		*(u32*)((u32)threads_array[tid].stack_res + 0x20) = -1;
	}

    __asm__("move $2, %0\n"
            "move $3, %1\n"
            : : "r"(threads_array[tid].entry), "r"(threads_array[tid].stack_res) );
}

////////////////////////////////////////////////////////////////////
//80003A78
////////////////////////////////////////////////////////////////////
void _ChangeThread(u32 entry, u32 stack_res, int waitSema)
{
	struct TCB *th;
	struct ll *l, *p;
	int prio;

	th = &threads_array[threadId];
	th->status = THS_WAIT;
	th->waitSema = waitSema;
	th->entry = (void (*)(void*))entry;
	th->stack_res = (void*)stack_res;

	prio = threadPrio;
	for (l = &thread_ll_priorities[prio]; ; l++, prio++) {
		if (prio >= 129) {
			__printf("# <Thread> No active threads\n");
			Exit(1); l = 0; break;
		}

		if (l->next != l) { p = l->next; break; }
		if (l->prev == l) continue;
		p = l->prev; break;
	}

	if (l) {
		threadPrio = prio;
		threadId = (((u32)p - (u32)threads_array) * 0x286BCA1B) / 4;
	}

	th = &threads_array[threadId];
	th->status = THS_RUN;
	if (th->waitSema) {
		th->waitSema = 0;
		*(s64*)((u32)th->stack_res+0x20) = -1;
	}

    __asm__("move $2, %0\n"
            "move $3, %1\n"
            : : "r"(th->entry), "r"(th->stack_res) );
}

////////////////////////////////////////////////////////////////////
//80003C50		SYSCALL 032 CreateThread
////////////////////////////////////////////////////////////////////
int  _CreateThread(struct ThreadParam *param)
{
	struct TCB *th;
	struct threadCtx *thctx;
	int index;
	int *ptr;

	th = (struct TCB *)LL_unlink((struct ll*)&thread_ll_free);
	if (th == NULL) {
		__printf("%s: failed to get free thread\n", __FUNCTION__);
		return -1;
	}

	threads_count++;
	index=(((u32)th-(u32)threads_array) * 0x286BCA1B)/4;

	th->entry           = param->entry;
	th->stack_res       = param->stack + param->stackSize - STACK_RES;
	th->status          = THS_DORMANT;
	th->gpReg           = param->gpReg;
	th->initPriority    = param->initPriority;
	th->argstring       = 0;
	th->wakeupCount     = 0;
	th->semaId          = 0;
	th->stack           = param->stack;
	th->argc            = 0;
	th->entry_          = param->entry;
	th->heap_base       = threads_array[threadId].heap_base;
	th->stackSize       = param->stackSize;
	th->currentPriority = param->initPriority;
	th->waitSema        = 0;
	th->root            = threads_array[threadId].root;

	thctx = th->stack_res;
	thctx->gp = (u32)param->gpReg;
	thctx->sp = (u32)&thctx[1];
	thctx->fp = (u32)&thctx[1];
	thctx->ra = (u32)threads_array[threadId].root;

	return index;
}

////////////////////////////////////////////////////////////////////
//80003E00      SYSCALL 37 iTerminateThread
////////////////////////////////////////////////////////////////////
int _iTerminateThread(int tid)
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//80003F00
////////////////////////////////////////////////////////////////////
int __DeleteThread(int tid)
{
	if ((tid>=256) || (tid==threadId) || (threads_array[tid].status!=THS_DORMANT))
		return -1;

	releaseTCB(tid);
	return tid;
}

////////////////////////////////////////////////////////////////////
//80003F70
////////////////////////////////////////////////////////////////////
int __StartThread(int tid, void *arg)
{
	if ((tid>=256) || (tid==threadId) || (threads_array[tid].status!=THS_DORMANT))
		return -1;

	threads_array[tid].argstring	             = arg;
	((void**)threads_array[tid].stack_res)[0x10] = arg;  //a0
	thread_2_ready(tid);
	return tid;
}

////////////////////////////////////////////////////////////////////
//80003FF8
////////////////////////////////////////////////////////////////////
int __ExitThread()
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//80004110
////////////////////////////////////////////////////////////////////
int __ExitDeleteThread()
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//80004228      SYSCALL 39 DisableDispatchThread
////////////////////////////////////////////////////////////////////
int _DisableDispatchThread()
{
    __printf("# DisableDispatchThread is not supported in this version\n");
    return threadId;
}

////////////////////////////////////////////////////////////////////
//80004258      SYSCALL 40 EnableDispatchThread
////////////////////////////////////////////////////////////////////
int _EnableDispatchThread()
{
    __printf("# EnableDispatchThread is not supported in this version\n");
    return threadId;
}

////////////////////////////////////////////////////////////////////
//80004288		SYSCALL 042 iChangeThreadPriority
////////////////////////////////////////////////////////////////////
int _iChangeThreadPriority(int tid, int prio)
{
	short oldPrio;

	if ((tid >= 256) || (prio < 0) || (prio >= 128)) return -1;

	if (tid == 0) tid = threadId;
	if (threads_array[tid].status == 0) return -1;
	if ((0 < (threads_array[tid].status ^ 0x10)) == 0) return -1;

	oldPrio = threads_array[tid].currentPriority;
	if ((tid != threadId) && (threads_array[tid].status != THS_READY)) {
		threads_array[tid].currentPriority = prio;
		return oldPrio;
	}

	if (threadPrio < prio) threadStatus = 1;

	unsetTCB(tid);
	threads_array[tid].currentPriority = prio;
	thread_2_ready(tid);

	return oldPrio;
}

////////////////////////////////////////////////////////////////////
//80004388		SYSCALL 044 iRotateThreadReadyQueue
////////////////////////////////////////////////////////////////////
int _iRotateThreadReadyQueue(int prio)
{
	if (prio >= 128) return -1;

	LL_rotate(&thread_ll_priorities[prio]);
	return prio;
}

////////////////////////////////////////////////////////////////////
//800043D0		SYSCALL 046 iReleaseWaitThread
////////////////////////////////////////////////////////////////////
int _iReleaseWaitThread(int tid)
{
    if( (u32)(tid-1) >= 255 )
        return -1;

    if( (u32)threads_array[tid].status >= 17 )
        return tid;

    switch(threads_array[tid].status) {
    case 0:
        return -1;
    case THS_WAIT: {
        //443C
        if( threads_array[tid].waitSema == 2 ) {
            LL_unlinkthis((struct ll*)&threads_array[tid]);
            semas_array[threads_array[tid].semaId].wait_threads--;
        }

        int threadPrioOld = threadPrio;
        threads_array[tid].status = THS_READY;
        thread_2_ready(tid);

        if( threadPrio < threadPrioOld ) {
            threadStatus = 1;
        }
        break;
    }
    case (THS_WAIT|THS_SUSPEND):
        threads_array[tid].status = THS_SUSPEND;
        if( threads_array[tid].waitSema != 2 )
            return tid;

        LL_unlinkthis((struct ll*)&threads_array[tid]);
        semas_array[threads_array[tid].semaId].wait_threads--;
        break;
    }

    return tid;
}

////////////////////////////////////////////////////////////////////
//80004548		SYSCALL 047 GetThreadId
////////////////////////////////////////////////////////////////////
int _GetThreadId()
{
	return threadId;
}

////////////////////////////////////////////////////////////////////
//80004558		SYSCALL 048 ReferThreadStatus
////////////////////////////////////////////////////////////////////
int _ReferThreadStatus(int tid, struct ThreadParam *info)
{
	if (tid >= 256) return -1;
	if (tid == 0) tid = threadId;

	if (info != NULL) {
		info->entry           = threads_array[tid].entry;
		info->status          = threads_array[tid].status;
		info->stack           = threads_array[tid].stack;
		info->stackSize      = threads_array[tid].stackSize;
		info->gpReg           = threads_array[tid].gpReg;
		info->initPriority    = threads_array[tid].initPriority;
		info->currentPriority = threads_array[tid].currentPriority;
		info->attr            = threads_array[tid].attr;
		info->waitSema        = threads_array[tid].waitSema;
		info->option          = threads_array[tid].option;
		info->waitId          = threads_array[tid].semaId;
		info->wakeupCount     = threads_array[tid].wakeupCount;
	}

	return threads_array[tid].status;
}

////////////////////////////////////////////////////////////////////
//80004658
////////////////////////////////////////////////////////////////////
int __SleepThread()
{
	if (threads_array[threadId].wakeupCount <= 0) {
		unsetTCB(threadId);
		return -1;
	}

	threads_array[threadId].wakeupCount--;
	return threadId;
}

////////////////////////////////////////////////////////////////////
//800046B0		SYSCALL 052 iWakeupThread
////////////////////////////////////////////////////////////////////
int _iWakeupThread(int tid)
{
	register int prio;

	if (tid>=256) return -1;

	if (tid==0) tid = threadId;

	switch (threads_array[tid].status){
	case THS_WAIT:
		if (threads_array[tid].waitSema=1){
			prio=threadPrio;
			thread_2_ready(tid);
			if (threadPrio<prio)
				threadStatus=THS_RUN;
			threads_array[tid].waitSema=0;
		}else
			threads_array[tid].wakeupCount++;
		break;
	case THS_READY:
	case THS_SUSPEND:
		threads_array[tid].wakeupCount++;
		break;
	case (THS_WAIT|THS_SUSPEND):
		if (threads_array[tid].waitSema==1){
			threads_array[tid].status=THS_SUSPEND;
			threads_array[tid].waitSema=0;
		}else
			threads_array[tid].wakeupCount++;
		break;
	default:
		return -1;
	}
	
	return tid;
}

////////////////////////////////////////////////////////////////////
//80004808		SYSCALL 055 SuspendThread
//80004808		SYSCALL 056 iSuspendThread
////////////////////////////////////////////////////////////////////
int _SuspendThread(int thid)
{
	if (thid<256){
		if ((threads_array[thid].status==THS_READY) || 
		    (threads_array[thid].status==THS_RUN)){
			unsetTCB(thid);
			threads_array[thid].status=THS_SUSPEND;
			return thid;
		}
		if (threads_array[thid].status==THS_WAIT){
			threads_array[thid].status=(THS_WAIT|THS_SUSPEND);
			return thid;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////
//800048C0		SYSCALL 058 iResumeThread
////////////////////////////////////////////////////////////////////
int _iResumeThread(int tid)
{
    int tmp;
	if ((tid<256) && (threadId!=tid)) {
		if (threads_array[tid].status==THS_SUSPEND){
			tmp=threadPrio;
			thread_2_ready(tid);
			if (threadPrio < tmp)
				threadStatus=THS_RUN;
		}
        else if (threads_array[tid].status==(THS_WAIT|THS_SUSPEND))
			threads_array[tid].status=THS_WAIT;
		return tid;
	}
	return -1;
}

////////////////////////////////////////////////////////////////////
//80004970		SYSCALL 053 CancelWakeupThread
//80004970		SYSCALL 054 iCancelWakeupThread
////////////////////////////////////////////////////////////////////
int _CancelWakeupThread(int tid)
{
	register int ret;

	if (tid>=256)	return -1;
	tid = tid ? tid : threadId;
	ret=threads_array[tid].wakeupCount;
	threads_array[tid].wakeupCount=0;
	return ret;
}

////////////////////////////////////////////////////////////////////
//800049B0		SYSCALL 080 RFU080_CreateEventFlag
////////////////////////////////////////////////////////////////////
int _CreateEventFlag()
{
	return threads_count; // CreateEventFlag, why!?!
}

////////////////////////////////////////////////////////////////////
//800049C0		SYSCALL 064 CreateSema
////////////////////////////////////////////////////////////////////
int _CreateSema(struct SemaParam *sema)
{
    register struct kSema *crt=semas_last;

	if ((crt==NULL) || (sema->init_count<0))	return -1;

	crt->wait_prev	= (struct TCB*)&crt->wait_next;
	semas_count++;
	crt->count	=sema->init_count;
	crt->wait_next	=(struct TCB*)&crt->wait_next;
	semas_last	= crt->free;
	crt->max_count	=sema->max_count;
	crt->free	=NULL;
	crt->attr	=sema->attr;
	crt->wait_threads=0;
	crt->option	=sema->option;

	return (crt-semas_array);	//sizeof(kSema)==32
}

////////////////////////////////////////////////////////////////////
//80004A48		SYSCALL 073 iDeleteSema
////////////////////////////////////////////////////////////////////
int _iDeleteSema(int sid)
{
    register thid, thprio;
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	semas_count--;
	while (semas_array[sid].wait_threads>0){
		thid=(((u32)LL_unlink((struct ll*)&semas_array[sid].wait_next)-(u32)threads_array) * 0x286BCA1B)/4;
		LL_unlinkthis((struct ll*)&threads_array[thid]);
		semas_array[sid].wait_threads--;
		if (threads_array[thid].status==THS_WAIT){
			thprio=threadPrio;
			thread_2_ready(thid);
			if (threadPrio<thprio)
				threadStatus=THS_RUN;
		}
        else if (threads_array[thid].status!=(THS_WAIT|THS_SUSPEND))
            threads_array[thid].status=THS_SUSPEND;
	}
	semas_array[sid].count	=-1;
	semas_array[sid].free	=semas_last;
	semas_last		=&semas_array[sid];
	return sid;
}

////////////////////////////////////////////////////////////////////
//80004BC8		SYSCALL 067 iSignalSema
////////////////////////////////////////////////////////////////////
int _iSignalSema(int sid)
{
    register int prio, thid;
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	if (semas_array[sid].wait_threads>0){
		thid=(((u32)LL_unlink((struct ll*)&semas_array[sid].wait_next)-(u32)threads_array)*0x286BCA1B)/4;

		LL_unlinkthis((struct ll*)&threads_array[thid]);

		semas_array[sid].wait_threads--;

		if (threads_array[thid].status==THS_WAIT){
			prio=threadPrio;
			thread_2_ready(thid);
			if (threadPrio < prio)
				threadStatus=THS_RUN;
			threads_array[thid].waitSema=0;	//just a guess:P
		}
        else if (threads_array[thid].status==(THS_WAIT|THS_SUSPEND)){
			threads_array[thid].status =THS_SUSPEND;
			threads_array[thid].waitSema=0;
		}
	}else
		semas_array[sid].count++;
	return sid;
}

////////////////////////////////////////////////////////////////////
//80004CF8
////////////////////////////////////////////////////////////////////
int _iWaitSema(int sid)
{
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	if (semas_array[sid].count>0){
		semas_array[sid].count--;
		return sid;
	}

	semas_array[sid].wait_threads++;

	unsetTCB(threadId);
	LL_add((struct ll*)&semas_array[sid].wait_next, (struct ll*)&threads_array[threadId]);
	threads_array[threadId].semaId=sid;

	return -2;
}

////////////////////////////////////////////////////////////////////
//80004DC8	SYSCALL 069 PollSema, 070 iPollSema
////////////////////////////////////////////////////////////////////
int _PollSema(int sid)
{
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<=0))	return -1;

	semas_array[sid].count--;

	return sid;
}

////////////////////////////////////////////////////////////////////
//80004E00	SYSCALL 071 ReferSemaStatus, 072 iReferSemaStatus
////////////////////////////////////////////////////////////////////
int _ReferSemaStatus(int sid, struct SemaParam *sema)
{
	if ((sid>=MAX_SEMAS) || (semas_array[sid].count<0))	return -1;

	sema->count		=semas_array[sid].count;
	sema->max_count		=semas_array[sid].max_count;
	sema->wait_threads	=semas_array[sid].wait_threads;
	sema->attr		=semas_array[sid].attr;
	sema->option		=semas_array[sid].option;

	return sid;
}

////////////////////////////////////////////////////////////////////
//80004E58		SYSCALL 081 RFU081_DeleteEventFlag
////////////////////////////////////////////////////////////////////
int _DeleteEventFlag()
{
	return semas_count; // DeleteEventFlag, why!?!
}

////////////////////////////////////////////////////////////////////
//80004E68
////////////////////////////////////////////////////////////////////
int _SemasInit()
{
	int i;

	for (i=0; i<256; i++) {
		semas_array[i].free = &semas_array[i+1];
		semas_array[i].count = -1;
		semas_array[i].wait_threads = 0;
		semas_array[i].wait_next = (struct TCB*)&semas_array[i].wait_next;
		semas_array[i].wait_prev = (struct TCB*)&semas_array[i].wait_next;
	}
	semas_array[255].free = 0;

	semas_last  = semas_array;
	semas_count = 0;

	return 256;
}

////////////////////////////////////////////////////////////////////
//80004EC8
////////////////////////////////////////////////////////////////////
void __load_module_EENULL()
{
	int i;

	thread_ll_free.prev = &thread_ll_free;
	thread_ll_free.next = &thread_ll_free;

	for (i=0; i<128; i++) {
		thread_ll_priorities[i].prev = &thread_ll_priorities[i];
		thread_ll_priorities[i].next = &thread_ll_priorities[i];
	}

	threads_count = 0;
	threadId = 0;
	threadPrio = 0;

	for (i=0; i<256; i++) {
		threads_array[i].status = 0;
		LL_add(&thread_ll_free, (struct ll*)&threads_array[i]);
	}

	_SemasInit();

	threadStatus = 0;
	__load_module("EENULL", (void (*)(void*))0x81FC0, (void*)0x81000, 0x80); 
}

////////////////////////////////////////////////////////////////////
//80004FB0
// makes the args from argc & argstring; args is in bss of program
////////////////////////////////////////////////////////////////////
void _InitArgs(char *argstring, ARGS *args, int argc)
{
	int i;
	char *p = args->args;

	args->argc = argc;
	for (i=0; i<argc; i++) {
		args->argv[i] = p;	//copy string pointer
		while (*argstring)	//copy the string itself
			*p++ = *argstring++;
		*p++ = *argstring++;	//copy the '\0'
	}
}

////////////////////////////////////////////////////////////////////
//80005198		SYSCALL 060 _InitializeMainThread
////////////////////////////////////////////////////////////////////
void *_InitializeMainThread(u32 gp, void *stack, int stack_size, char *args, int root)
{
    struct TCB *th;
	struct threadCtx *ctx;

	if ((int)stack == -1)
		stack = (void*)((_GetMemorySize() - 4*1024) - stack_size);

	ctx = (struct threadCtx*)((u32)stack + stack_size - STACK_RES/4);
	ctx->gp   = gp;			//+1C0
	ctx->ra   = root;		//+1F0
	ctx->fp   = (u32)ctx+0x280;	//+1E0 <- &280
	ctx->sp   = (u32)ctx+0x280;	//+1D0 <- &280

	th = &threads_array[threadId];
	th->gpReg	= (void*)gp;
	th->stackSize   = stack_size;
	th->stack_res   = ctx;
	th->stack       = stack;
	th->root        = (void*)root;
	_InitArgs(th->argstring, (ARGS*)args, th->argc);
	th->argstring   = args;

	return ctx;
}

////////////////////////////////////////////////////////////////////
//800052A0		SYSCALL 061 RFU061_InitializeHeapArea
////////////////////////////////////////////////////////////////////
void* _InitializeHeapArea(void *heap_base, int heap_size)
{
	void *ret;

	if (heap_size < 0) {
		ret = threads_array[threadId].stack;
	} else {
		ret = heap_base + heap_size;
	}

	threads_array[threadId].heap_base = ret;
	return ret;
}

////////////////////////////////////////////////////////////////////
//800052D8		SYSCALL 062 RFU062_EndOfHeap
////////////////////////////////////////////////////////////////////
void* _EndOfHeap()
{
	return threads_array[threadId].heap_base;
}

////////////////////////////////////////////////////////////////////
//80005390
////////////////////////////////////////////////////////////////////
int __load_module(char *name, void (*entry)(void*), void *stack_res, int prio)
{
	struct TCB *th;
	int index;
	int *ptr;
    struct rominfo ri;

	th = (struct TCB*)LL_unlink(&thread_ll_free);
	if (th) {
		threads_count++;
		index = (((u32)th-(u32)threads_array) * 0x286BCA1B)/4;
	} else {
		index = -1;
	}

	threadId = index;
	th->wakeupCount     = 0;
	th->semaId          = 0;
	th->attr            = 0;
	th->stack_res       = stack_res;
	th->option          = 0;
	th->entry           = entry;
	th->gpReg           = 0;
	th->currentPriority = prio;
	th->status          = THS_DORMANT;
	th->waitSema        = 0;
	th->entry_          = entry;
	th->argc            = 0;
	th->argstring       = 0;
	th->initPriority    = prio;

	thread_2_ready(index);

	if (romdirGetFile(name, &ri) == NULL) {
		__printf("# panic ! '%s' not found\n", name);
		_Exit(1);
	}	

	if (ri.fileSize > 0) {
		int i;
		int *src = (int*)(0xbfc00000+ri.fileOffset);
		int *dst = (int*)entry;

		for (i=0; i<ri.fileSize; i+=4) {
			*dst++ = *src++;
		}
	}

	FlushInstructionCache();
    FlushDataCache();

	return index;
}

////////////////////////////////////////////////////////////////////
//80005560 eestrcpy
////////////////////////////////////////////////////////////////////
char* eestrcpy(char* dst, const char* src)
{
    while( *src )
        *dst++ = *src++;

    *dst = 0;
    return dst+1;
}

////////////////////////////////////////////////////////////////////
//800055A0    SYSCALL 6 LoadPS2Exe
////////////////////////////////////////////////////////////////////
int _LoadPS2Exe(char *filename, int argc, char **argv)
{
    char* pbuf;
    struct rominfo ri;
    struct TCB* curtcb;
    int curthreadid;

    pbuf = eestrcpy(threadArgStrBuffer, "EELOAD");
    pbuf = eestrcpy(pbuf, filename);
    
    if( argc > 0 ) {
        int i;
        for(i = 0; i < argc; ++i)
            pbuf = strcpy(pbuf, argv[i]);
    }

    threads_array[threadId].argc = argc+2;
    threads_array[threadId].argstring = threadArgStrBuffer;
    _CancelWakeupThread(threadId);
    _iChangeThreadPriority(threadId, 0);
    
    // search for RESET
    // search for filename romdir entry
    if( romdirGetFile(filename, &ri) == NULL ) {
        __printf("# panic ! '%s' not found\n", filename);
        _Exit();
    }

    // terminate threads
    curthreadid = 1; // skip main thread?
    curtcb = &threads_array[curthreadid];
    
    while(curthreadid < 256) {
        if( curtcb->status && threadId != curthreadid ) {
            if( curtcb->status == THS_DORMANT ) {
                _DeleteThread(curthreadid);
            }
            else {
                iTerminateThread(curthreadid);
                _DeleteThread(curthreadid);
            }
        }
        ++curthreadid;
        ++curtcb;
    }

    _SemasInit();
    threadStatus = 0;
    InitPgifHandler2();
    
    Restart();

    if( ri.fileSize > 0 ) {
        // copy to PS2_LOADADDR
        int i;
        u32* psrc = (u32*)(0xbfc00000+ri.fileOffset);
        u32* pdst = (u32*)PS2_LOADADDR;
        for(i = 0; i < ri.fileSize; i += 4)
            *pdst++ = *psrc++;
    }

    FlushInstructionCache();
    FlushDataCache();
    
	__asm__("mtc0 %0, $14\n"
            "sync\n" : : "r"(PS2_LOADADDR));
    erase_cpu_regs();
    __asm__("di\n"
            "eret\n");
}

////////////////////////////////////////////////////////////////////
//800057E8
////////////////////////////////////////////////////////////////////
void* __ExecPS2(void * entry, void * gp, int argc, char ** argv)
{
    char* pbuf = threadArgStrBuffer;
    int i;

    if( argc > 0 ) {
        for(i = 0; i < argc; ++i)
            pbuf = eestrcpy(pbuf, argv[i]);
    }
    
    threads_array[threadId].entry = entry; //0C
    threads_array[threadId].wakeupCount = 0; //24
    threads_array[threadId].gpReg = gp; //14
    threads_array[threadId].semaId = 0; //20
    threads_array[threadId].argstring = threadArgStrBuffer; //38
    threads_array[threadId].argc = argc; //34
    threads_array[threadId].entry_ = entry; //30
    threads_array[threadId].currentPriority = 0; //18
    threads_array[threadId].waitSema = 0; //1C
    threads_array[threadId].initPriority = 0;
    FlushInstructionCache();
    FlushDataCache();

    return entry;
}

////////////////////////////////////////////////////////////////////
//800058E8
////////////////////////////////////////////////////////////////////
int _ExecOSD(int argc, char **argv)
{
    return _LoadPS2Exe("rom0:OSDSYS", argc, argv);
}

////////////////////////////////////////////////////////////////////
//80005900	
////////////////////////////////////////////////////////////////////
int  _RFU004_Exit()
{
    char *bb = "BootBrowser";
	return _LoadPS2Exe("rom0:OSDSYS", 1, &bb);
}

////////////////////////////////////////////////////////////////////
//80005938
////////////////////////////////////////////////////////////////////
void releaseTCB(int tid)
{
	threads_count--;
	threads_array[tid].status=0;
	LL_add(&thread_ll_free, (struct ll*)&threads_array[tid]);
}

////////////////////////////////////////////////////////////////////
//80005978
////////////////////////////////////////////////////////////////////
void unsetTCB(int tid)
{
	if ((threads_array[tid].status) <= THS_READY)
		LL_unlinkthis((struct ll*)&threads_array[tid]);
}

////////////////////////////////////////////////////////////////////
//800059B8
////////////////////////////////////////////////////////////////////
void thread_2_ready(int tid)
{
	threads_array[tid].status=THS_READY;
	if (threads_array[tid].initPriority < threadPrio)
		threadPrio=(short)threads_array[tid].initPriority;
	LL_add( &thread_ll_priorities[threads_array[tid].initPriority], (struct ll*)&threads_array[tid] );
}

////////////////////////////////////////////////////////////////////
//80005A58
////////////////////////////////////////////////////////////////////
struct ll* LL_unlink(struct ll *l)
{
	struct ll *p;

	if ((l==l->next) && (l==l->prev))
		return 0;
	p=l->prev;
	p->prev->next=p->next;
	p->next->prev=p->prev;
	return p;
}

////////////////////////////////////////////////////////////////////
//80005A98
////////////////////////////////////////////////////////////////////
struct ll* LL_rotate(struct ll *l)
{
	struct ll *p;

	if (p=LL_unlink(l)){
		p->prev=l;
		p->next=l->next;
		l->next->prev=p;
		l->next=p;
		return l->prev;
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////
//80005AE8
////////////////////////////////////////////////////////////////////
struct ll *LL_unlinkthis(struct ll *l)
{
	l->prev->next=l->next;
	l->next->prev=l->prev;
	return l->next;
}

////////////////////////////////////////////////////////////////////
//80005B08
////////////////////////////////////////////////////////////////////
void LL_add(struct ll *l, struct ll *new)
{
	new->prev=l;
	new->next=l->next;
	l->next->prev=new;
	l->next=new;
}

////////////////////////////////////////////////////////////////////
//80005B28		SYSCALL 9 (0x09) TlbWriteRandom
////////////////////////////////////////////////////////////////////
int  _TlbWriteRandom(u32 PageMask, u32 EntryHi, u32 EntryLo0, u32 EntryLo1) {
	if ((EntryHi >> 24) != 4) return -1;
	__asm__ (
		"mfc0 $2, $1\n"
		"mtc0 $2, $0\n"
		"mtc0 $4, $5\n"
		"mtc0 $5, $10\n"
		"mtc0 $6, $2\n"
		"mtc0 $7, $3\n"
		"sync\n"
		"tlbwi\n"
		"sync\n"
	);
}

int _sifGetMSFLG() {
	u32 msflg;
	for (;;) {
        msflg = SBUS_MSFLG;
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");

        if (msflg == SBUS_MSFLG) return msflg;
	}
}

////////////////////////////////////////////////////////////////////
//80005E58
////////////////////////////////////////////////////////////////////
int _sifGetSMFLG() {
	u32 smflg;
	for (;;) {
        smflg = SBUS_SMFLG;
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");

        if (smflg == SBUS_SMFLG) return smflg;
	}
}

int _ResetSif1()
{
    sif1tagdata = 0xFFFF001E;
    //*(int*)0xa0001e330 = 0x20000000;
    //*(int*)0xa0001e334 = (u32)ptag&0x0fffffff;
    D6_QWC = 0;
    D6_TAG = (u32)&sif1tagdata&0x0fffffff;
}

////////////////////////////////////////////////////////////////////
//80006198
////////////////////////////////////////////////////////////////////
void SifDmaInit()
{
    int msflg;
	memset(sifEEbuff, 0, sizeof(sifEEbuff));
	memset(sifRegs, 0, sizeof(sifRegs));

	*(u32*)0xB000F260 = 0xFF;
	D5_CHCR = 0;
	D6_CHCR = 0;

	_SifSetDChain();
	*(u32*)0xB000F200 = (u32)sifEEbuff;
	__printf("MSFLG = 0x10000\n");
	SBUS_MSFLG = 0x10000;
	msflg = SBUS_MSFLG;
    _ResetSif1();
	_SifSetReg(1, 0);

	while (!(_SifGetReg(4) & 0x10000)) { __asm__ ("nop\nnop\nnop\nnop\n"); }

	sifIOPbuff = *(u32*)0xB000F210;

	SBUS_MSFLG = 0x20000;
	SBUS_MSFLG;
}

////////////////////////////////////////////////////////////////////
//800062A0		SYSCALL 120 (0x78) SifSetDChain
////////////////////////////////////////////////////////////////////
void _SifSetDChain(){
	int var_10;

    D5_CHCR = 0;
    D5_QWC = 0;
	D5_CHCR = 0x184;	// 0001 1000 0100
	var_10	= D5_CHCR;		// read?
}

////////////////////////////////////////////////////////////////////
//800062D8		SYSCALL 107 (0x6B) SifStopDma
////////////////////////////////////////////////////////////////////
void _SifStopDma(){
	int var_10;

    D5_CHCR	= 0;
	D5_QWC	= 0;
	var_10	= D5_CHCR;	// read?
}

////////////////////////////////////////////////////////////////////
//80006370
////////////////////////////////////////////////////////////////////
void _SifDmaSend(void *src, void *dest, int size, int attr, int id)
{
    int qwc;
    struct TAG* t1;
    int *t3;
    int tocopy;

	if (((++tagindex) & 0xFF) == 31){
		tagindex=0;
		transferscount++;
	}

	qwc=(size+15)/16;	//rount up

	t1=(struct TAG*)KSEG1_ADDR(&tadrptr[tagindex]);
	if (attr & SIF_DMA_TAG) {
		t1->id_qwc = (id << 16) | qwc |
			((attr & SIF_DMA_INT_I) ? 0x80000000 : 0);	//IRQ
		t1->addr=(u32)src & 0x1FFFFFFF;
		t3=(int*)KSEG1_ADDR(src);
		qwc--;
	}
    else {
		if (qwc >= 8){	//two transfers
			tocopy=7;
			t1->id_qwc=0x30000008;	//(id)REF | (qwc)8;
			t1->addr=(u32)&extrastorage[tagindex] | 0x1FFFFFFF;
			t3=(int*)KSEG1_ADDR(&extrastorage[tagindex]);

			if (((++tagindex) & 0xff) == 31){
				tagindex=0;
				transferscount++;
			}

			t1=(struct TAG*)KSEG1_ADDR(&tadrptr[tagindex]);
			t1->id_qwc=(id << 16) | (qwc - 7) |
				((attr & SIF_DMA_INT_I) ? 0x80000000 : 0);//IRQ
			t1->addr=(u32)(src+112) | 0x1FFFFFFF;
		}
        else {
			tocopy=qwc;
			t1->id_qwc=(id << 16) | (qwc+1) |
				((attr & SIF_DMA_INT_I) ? 0x80000000 : 0);//IRQ
			t1->addr=(u32)&extrastorage[tagindex] & 0x1FFFFFFF;
			t3=(int*)KSEG1_ADDR(&extrastorage[tagindex]);
		}
		memcpy((char*)t3+16, (void*)KSEG1_ADDR(src), tocopy*16);//inline with qwords
	}
	t3[1]=qwc * 4;
	t3[0]=(u32)((u32)dest & 0x00FFFFFF) |
		((((u32)(attr & SIF_DMA_INT_O)) ? 0x40000000 : 0) |
        (((u32)(attr & SIF_DMA_ERT))	? 0x80000000 : 0));
}

////////////////////////////////////////////////////////////////////
//800065C8
////////////////////////////////////////////////////////////////////
int _SifDmaCount()
{
	register int count;

	count=((D6_TAG-(u32)tadrptr) & 0x1FFFFFFF) >> 4;

	count=count>0? count-1:30;

	if (count == tagindex)
		return (D6_QWC ? 30 : 31);

	if (count < tagindex)
		return count + 30 - tagindex;

	return count-tagindex-1;
}

////////////////////////////////////////////////////////////////////
//80006650
////////////////////////////////////////////////////////////////////
void _SifDmaPrepare(int count)
{
	register struct TAG *t0;

	if (count==31)	return;

	t0=(struct TAG*)KSEG1_ADDR(&tadrptr[tagindex]);
	if (count == 30){
		t0->id_qwc &= DMA_TAG_IRQ|DMA_TAG_PCE;	//keep PCE|REFE|IRQ
		t0->id_qwc |= DMA_TAG_REF<<28;
		t0->id_qwc |= D6_QWC;
		t0->addr    = D6_MADR;
		D6_QWC  = 0;
		D6_TAG = KUSEG_ADDR(t0);
	}else
		t0->id_qwc |= DMA_TAG_REF<<28;
}

////////////////////////////////////////////////////////////////////
//800066F8		SYSCALL 119 (0x77) SifSetDma
////////////////////////////////////////////////////////////////////
u32 _SifSetDma(SifDmaTransfer_t *sdd, int len)
{
	int var_10;
	int count, tmp;
	int i, c, _len = len;
    int nextindex;

    DMAC_ENABLEW = DMAC_ENABLER | 0x10000;	//suspend

    D6_CHCR = 0;	//kill any previous transfer?!?
	var_10	= D6_CHCR;	// read?

    DMAC_ENABLEW = DMAC_ENABLER & ~0x10000;	//enable

	count = _SifDmaCount();

lenloop:
	i=0; c=0;
	while (_len > 0) {
		if (!(sdd[i].attr & SIF_DMA_TAG)) {
			if (sdd[i].size <= 112) c++;
			else c+=2;
		} else c++;
		_len--; i++;
	}
	if (count < c) { count = 0; goto lenloop; }

    nextindex = ((tagindex+1) % 31) & 0xff;
	if (nextindex == 0)
		tmp = (transferscount + 1) & 0xFFFF;
	else
		tmp = transferscount;

	_SifDmaPrepare(count);

	while (len > 0) {
		_SifDmaSend(sdd->src, sdd->dest, sdd->size, sdd->attr, DMA_TAG_REF<<12);//REF >> 16
		sdd++; len--;
	}

	_SifDmaSend(sdd->src, sdd->dest, sdd->size, sdd->attr, DMA_TAG_REFE<<12);//REFE >> 16

    D6_CHCR|= 0x184;
	var_10	= D6_CHCR;	// read?
    return (tmp<<16)|(nextindex<<8)|c;
}

////////////////////////////////////////////////////////////////////
//800068B0		SYSCALL 118 (0x76) SifDmaStat
////////////////////////////////////////////////////////////////////
int _SifDmaStat(int id)
{
    //TODO
    return 0;
}

////////////////////////////////////////////////////////////////////
//80006B60		SYSCALL 121 (0x79) SifSetDma
////////////////////////////////////////////////////////////////////
int _SifSetReg(int reg, u32 val)
{
    __printf("%s: reg=%d; val=%x\n", __FUNCTION__, reg, val);

	if (reg == 1) {
		*(u32*)0xB000F200 = val;
		return *(u32*)0xB000F200;
	} else
	if (reg == 3) {
		SBUS_MSFLG = val;
		return _sifGetMSFLG();
	} else
	if (reg == 4) {
		SBUS_SMFLG = val;
		return _sifGetSMFLG();
	} else
	if (reg >= 0) {
		return 0; 
	}

	reg&= 0x7FFFFFFF;
	if (reg >= 32) return 0;

	sifRegs[reg] = val;
	return val;
}

////////////////////////////////////////////////////////////////////
//80006C18		SYSCALL 122 (0x7A) SifSetDma
////////////////////////////////////////////////////////////////////
int _SifGetReg(int reg)
{
    //__printf("%s: reg=%x\n", __FUNCTION__, reg);

	if (reg == 1) {
		return *(u32*)0xB000F200;
	} else
	if (reg == 2) {
		return *(u32*)0xB000F210;
	} else
	if (reg == 3) {
		return _sifGetMSFLG();
	} else
	if (reg == 4) {
		return _sifGetSMFLG();
	} else
	if (reg >= 0) {
		return 0; 
	}

	reg&= 0x7FFFFFFF;
	if (reg >= 32) return 0;

    //__printf("ret=%x\n", sifRegs[reg]);
	return sifRegs[reg];
}

////////////////////////////////////////////////////////////////////
//80007428      SYSCALL 117 print
////////////////////////////////////////////////////////////////////
void _print()
{
}

////////////////////////////////////////////////////////////////////
//800074D8
////////////////////////////////////////////////////////////////////
void _SetGsCrt2()
{
	u32 tmp;

	tmp = *(int*)0x8001F344;
	if (tmp == 0x40 || tmp == 0x60 || tmp == 0x61) {
		*(char*)0xbf803218 = 0;
		*(char*)0xbf803218 = 2;
		return;
	}

	*(short*)0xbf801470 = 0;
	*(short*)0xbf801472 = 0;
	*(short*)0xbf801472 = 1;
}

////////////////////////////////////////////////////////////////////
//800076B0
////////////////////////////////////////////////////////////////////
void _iJoinThread(int param)
{
    //TODO
}

////////////////////////////////////////////////////////////////////
//80008A60
////////////////////////////////////////////////////////////////////
void _SetGsCrt3(short arg0, short arg1, short arg2)
{
    __printf("_SetGsCrt3 unimplemented\n");
}

////////////////////////////////////////////////////////////////////
//80009CE0
////////////////////////////////////////////////////////////////////
void _SetGsCrt4(short arg0, short arg1, short arg2)
{
    __printf("_SetGsCrt4 unimplemented\n");
}

////////////////////////////////////////////////////////////////////
//8000A060		SYSCALL 002 SetGsCrt
////////////////////////////////////////////////////////////////////
void _SetGsCrt(short arg0, short arg1, short arg2)
{
	u64 val, val2;
	u64 tmp;
	int count;

	if (arg1 == 0) {
		tmp = (hvParam >> 3) & 0x7;
		tmp^= 0x2;
		if (tmp == 0) arg1 = 3;
		else arg1 = 2;
	}

	for (count=0x270f; count >= 0; count--) {
		__asm__ ("nop\nnop\nnop\nnop\nnop\nnop\n");
	}
    
	*(int*)0x8001F344 = 0;

	if (arg1 == 2) {
		if (arg0 != 0) {
			val = 0x740834504LL;
			val2 = 0x740814504LL;
            
			tmp = (hvParam & 0x1) << 25;
			val|= tmp;
			val2|= tmp;
            
			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5B61F06F040LL;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01801LL;
            
			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		} else {
			val = 0x740834504LL;
			val2 = 0x740814504LL;

			tmp = (hvParam & 0x2) << 35;
			val|= tmp;
			val2|= tmp;

			tmp = (hvParam & 0x1) << 25;
			val|= tmp;
			val2|= tmp;

			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5B61F06F040LL;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01802LL;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		}
		_SetGsCrt2();
        return;
	}

	if (arg1 == 3) {
		if (arg0 != 0) {
			val = 0x740836504LL;
			val2 = 0x740816504LL;

			tmp = (hvParam & 0x1) << 25;
			val|= tmp;
			val2|= tmp;

			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5C21FC83030LL;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101401LL;

			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		} else {
			val = 0x740836504LL;
			val2 = 0x740816504LL;

			tmp = (hvParam & 0x2) << 35;
			val|= tmp;
			val2|= tmp;

			tmp = (hvParam & 0x1) << 25;
			val|= tmp;
			val2|= tmp;

			GS_SMODE1 = val;
			GS_SYNCH1 = 0x7F5C21F683030LL;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101404LL;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val2;
		}
		_SetGsCrt2();
        return;
	}

	if (arg1 == 0x72) {
		if (arg0 != 0) {
			val = 0x740814504LL;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5B61F06F040LL;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01801LL;

			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		} else {
			val = 0x740814504LL;

			val|= (hvParam & 0x2) << 35;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5B61F06F040LL;
			GS_SYNCH2 = 0x33A4D8;
			GS_SYNCV  = 0xC7800601A01802LL;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		}
		return;
	}

	if (arg1 == 0x73) {
		if (arg0 != 0) {
			val = 0x740816504LL;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5C21FC83030LL;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101401LL;

			GS_SMODE2 = (arg2 << 1) | 1;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		} else {
			val = 0x740816504;

			val|= (hvParam & 0x2) << 35;
			val|= (hvParam & 0x1) << 25;

			GS_SYNCH1 = 0x7F5C21FC83030LL;
			GS_SYNCH2 = 0x3484BC;
			GS_SYNCV  = 0xA9000502101404LL;
			GS_SMODE2 = 0;
			GS_SRFSH  = 8;
			GS_SMODE1 = val;
		}
		return;
	}
    
	if ((u32)(arg1 - 26) >= 0x38) {
		_SetGsCrt3(arg0, arg1, arg2); return;
	}
    
	if (arg1 == 0x52) {
		_SetGsCrt3(arg0, arg1, arg2); return;
	}

	_SetGsCrt4(arg0, arg1, arg2);
}

////////////////////////////////////////////////////////////////////
//8000A768		SYSCALL 076 GetGsHParam
////////////////////////////////////////////////////////////////////
void _GetGsHParam(int *p0, int *p1, int *p2, int *p3)
{
    u32 _hvParam = (u32)hvParam;

	*p0 = _hvParam >> 12;
	*p1 = _hvParam >> 24;
	*p2 = _hvParam >> 18;
	*p3 = _hvParam >> 28;
}

////////////////////////////////////////////////////////////////////
//8000A7D0		SYSCALL 077 GetGsVParam
////////////////////////////////////////////////////////////////////
int _GetGsVParam()
{
	return hvParam & 0x3;
}

////////////////////////////////////////////////////////////////////
//8000A800		SYSCALL 059 JoinThread
////////////////////////////////////////////////////////////////////
void _JoinThread()
{
    _iJoinThread(0x87);
}

////////////////////////////////////////////////////////////////////
//8000A820		SYSCALL 078 SetGsHParam
////////////////////////////////////////////////////////////////////
void _SetGsHParam(int a0, int a1, int a2, int a3)
{
    __printf("SetGsHParam(%x,%x,%x,%x)... probably will never be supported\n", a0, a1, a2, a3);
    //write hvParam&1 to 12000010?
}

////////////////////////////////////////////////////////////////////
//8000A8F8		SYSCALL 079 SetGsVParam
////////////////////////////////////////////////////////////////////
void _SetGsVParam(int VParam)
{
	hvParam&= ~0x1;
	hvParam|= VParam & 0x1;
}

////////////////////////////////////////////////////////////////////
//8000A920		SYSCALL 075 GetOsdConfigParam
////////////////////////////////////////////////////////////////////
void _GetOsdConfigParam(int *result)
{
	*result= (*result & 0xFFFFFFFE) | (osdConfigParam & 1);
	*result= (*result & 0xFFFFFFF9) | (osdConfigParam & 6);
	*result= (*result & 0xFFFFFFF7) | (osdConfigParam & 8);
	*result= (*result & 0xFFFFFFEF) | (osdConfigParam & 0x10);
	*result=((*result & 0xFFFFE01F) | (osdConfigParam & 0x1FE0)) & 0xFFFF1FFF;
	((u16*)result)[1]=((u16*)&osdConfigParam)[1];
}

////////////////////////////////////////////////////////////////////
//8000A9C0		SYSCALL 074 SetOsdConfigParam
////////////////////////////////////////////////////////////////////
void _SetOsdConfigParam(int *param)
{
	osdConfigParam= (osdConfigParam & 0xFFFFFFFE) | (*param & 1);
	osdConfigParam= (osdConfigParam & 0xFFFFFFF9) | (*param & 6);
	osdConfigParam= (osdConfigParam & 0xFFFFFFF7) | (*param & 8);
	osdConfigParam= (osdConfigParam & 0xFFFFFFEF) | (*param & 0x10);
	osdConfigParam=((osdConfigParam & 0xFFFFE01F) | (*param & 0x1FE0)) & 0xFFFF1FFF;
	((u16*)&osdConfigParam)[1]=((u16*)param)[1];
}

////////////////////////////////////////////////////////////////////
//8000FCE8
////////////////////////////////////////////////////////////////////
void __exhandler(int a0)
{
}

////////////////////////////////////////////////////////////////////
//80010F34
////////////////////////////////////////////////////////////////////
void __disableInterrupts()
{
    __asm__("mtc0 $0, $25\n"
            "mtc0 $0, $24\n"
            "li   $3, 0xFFFFFFE0\n"
            "mfc0 $2, $12\n"
            "and  $2, $3\n"
            "mtc0 $2, $12\n"
            "sync\n");
}

////////////////////////////////////////////////////////////////////
//80010F58
////////////////////////////////////////////////////////////////////
void kSaveContext()
{
}

////////////////////////////////////////////////////////////////////
//80011030
////////////////////////////////////////////////////////////////////
void kLoadContext()
{
}

////////////////////////////////////////////////////////////////////
//800110F4
////////////////////////////////////////////////////////////////////
void kLoadDebug()
{
    kLoadContext();
    // lq $31, 0x1F0($27)
    // lq $27, 0x1B0($27)
    // j 80005020 - probably load debug services or restore prev program state
}

////////////////////////////////////////////////////////////////////
//80011108
////////////////////////////////////////////////////////////////////
int __exception()
{
	register u32 curepc __asm__("$2");
	int ret;

	__asm__("mfc0 %0, $14\n" : "=r"(curepc) : );

    // check if epc is currently in the except handlers:
    // _Deci2Handler, __exception1, and __exception
	if (curepc >= (u32)__exception && curepc < (u32)__exception+0x300) {
		__asm__("mfc0 %0, $13\n" : "=r"(ret) : );
        return (ret & 0x7C) >> 2;
	}

	kSaveContext();

	__disableInterrupts();
	__exhandler(1);
	
	kLoadContext();

	__asm__("eret\n");
}

////////////////////////////////////////////////////////////////////
//800111F0
////////////////////////////////////////////////////////////////////
void __exception1()
{
    kSaveContext();
    __disableInterrupts();
    //sub_8000CF68
    kLoadContext();
    __asm__("eret\n");
}
