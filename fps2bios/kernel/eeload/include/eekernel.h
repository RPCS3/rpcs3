#ifndef __EEKERNEL_H__
#define __EEKERNEL_H__

#include <tamtypes.h>
#include <kernel.h>


#define RCNT0_COUNT		*(volatile int*)0xB0000000
#define RCNT0_MODE		*(volatile int*)0xB0000010
#define RCNT0_TARGET	*(volatile int*)0xB0000020
#define RCNT0_HOLD		*(volatile int*)0xB0000030

#define RCNT1_COUNT		*(volatile int*)0xB0000800
#define RCNT1_MODE		*(volatile int*)0xB0000810
#define RCNT1_TARGET	*(volatile int*)0xB0000820
#define RCNT1_HOLD		*(volatile int*)0xB0000830

#define RCNT2_COUNT		*(volatile int*)0xB0001000
#define RCNT2_MODE		*(volatile int*)0xB0001010
#define RCNT2_TARGET	*(volatile int*)0xB0001020

#define RCNT3_COUNT		*(volatile int*)0xB0001800
#define RCNT3_MODE		*(volatile int*)0xB0001810
#define RCNT3_TARGET	*(volatile int*)0xB0001820

#define GIF_CTRL		*(volatile int*)0xB0003000

#define GIF_FIFO		*(volatile u128*)0xB0006000
	
//SIF0
#define D5_CHCR			*(volatile int*)0xB000C000
#define D5_MADR			*(volatile int*)0xB000C010
#define D5_QWC			*(volatile int*)0xB000C020

//SIF1
#define D6_CHCR			*(volatile int*)0xB000C400
#define D6_MADR			*(volatile int*)0xB000C410
#define D6_QWC			*(volatile int*)0xB000C420
#define D6_TAG          *(volatile int*)0xB000C430

#define DMAC_CTRL		*(volatile int*)0xB000E000
#define DMAC_STAT		*(volatile int*)0xB000E010
#define DMAC_PCR		*(volatile int*)0xB000E020
#define DMAC_SQWC		*(volatile int*)0xB000E030
#define DMAC_RBSR		*(volatile int*)0xB000E040
#define DMAC_RBOR		*(volatile int*)0xB000E050
#define DMAC_STADR		*(volatile int*)0xB000E060

#define INTC_STAT		*(volatile int*)0xB000F000
#define INTC_MASK		*(volatile int*)0xB000F010

#define SBUS_MSFLG		*(volatile int*)0xB000F220
#define SBUS_SMFLG		*(volatile int*)0xB000F230
#define SBUS_F240		*(volatile int*)0xB000F240

#define DMAC_ENABLER	*(volatile int*)0xB000F520
#define DMAC_ENABLEW	*(volatile int*)0xB000F590

#define SBFLG_IOPALIVE	0x10000
#define SBFLG_IOPSYNC	0x40000

#define GS_PMODE		*(volatile u64*)0xB2000000
#define GS_SMODE1		*(volatile u64*)0xB2000010
#define GS_SMODE2		*(volatile u64*)0xB2000020
#define GS_SRFSH		*(volatile u64*)0xB2000030
#define GS_SYNCH1		*(volatile u64*)0xB2000040
#define GS_SYNCH2		*(volatile u64*)0xB2000050
#define GS_SYNCV		*(volatile u64*)0xB2000060
#define GS_DISPFB1		*(volatile u64*)0xB2000070
#define GS_DISPLAY1		*(volatile u64*)0xB2000080
#define GS_DISPFB2		*(volatile u64*)0xB2000090
#define GS_DISPLAY2		*(volatile u64*)0xB20000A0
#define GS_EXTBUF		*(volatile u64*)0xB20000B0
#define GS_EXTDATA		*(volatile u64*)0xB20000C0
#define GS_EXTWRITE		*(volatile u64*)0xB20000D0
#define GS_BGCOLOR		*(volatile u64*)0xB20000E0
#define GS_CSR			*(volatile u64*)0xB2001000
#define GS_IMR			*(volatile u64*)0xB2001010
#define GS_BUSDIR		*(volatile u64*)0xB2001040
#define GS_SIGLBLID		*(volatile u64*)0xB2001080

#define INTC_GS  		0
#define INTC_SBUS  		1
#define	INTC_VBLANK_S		2
#define	INTC_VBLANK_E		3
#define INTC_VIF0  		4
#define INTC_VIF1  		5
#define INTC_VU0  		6
#define INTC_VU1  		7
#define INTC_IPU  		8
#define INTC_TIM0  		9
#define INTC_TIM1  		10
#define INTC_TIM2  		11
#define INTC_TIM3  		12	//threads
//#define INTC_13		13		//not used
//#define INTC_14		14		//not used

#define DMAC_VIF0		0
#define DMAC_VIF1		1
#define DMAC_GIF		2
#define DMAC_FROM_IPU		3
#define DMAC_TO_IPU		4
#define DMAC_SIF0		5
#define DMAC_SIF1		6
#define DMAC_SIF2		7
#define DMAC_FROM_SPR		8
#define DMAC_TO_SPR		9
//#define DMAC_10		10		//not used
//#define DMAC_11		11		//not used
//#define DMAC_12		12		//not used
//#define DMAC_13		13		//not used
//#define DMAC_14		14		//not used
#define DMAC_ERROR		15

 ///////////////////////
 // DMA TAG REGISTERS //
 ///////////////////////
 #define DMA_TAG_REFE	0x00
 #define DMA_TAG_CNT	0x01
 #define DMA_TAG_NEXT	0x02
 #define DMA_TAG_REF	0x03
 #define DMA_TAG_REFS	0x04
 #define DMA_TAG_CALL	0x05
 #define DMA_TAG_RET	0x06
 #define DMA_TAG_END	0x07

// Modes for DMA transfers
#define SIF_DMA_FROM_IOP	0x0
#define SIF_DMA_TO_IOP		0x1
#define SIF_DMA_FROM_EE		0x0
#define SIF_DMA_TO_EE		0x1

#define SIF_DMA_INT_I		0x2
#define SIF_DMA_INT_O		0x4
#define SIF_DMA_SPR		0x8
#define SIF_DMA_BSN		0x10 /* ? what is this? */
#define SIF_DMA_TAG 0x20
#define SIF_DMA_ERT 0x40
#define DMA_TAG_IRQ 0x80000000
#define DMA_TAG_PCE 0x0C000000

#define KSEG1_ADDR(x) (((u32)(x))|0xA0000000)
#define KUSEG_ADDR(x) (((u32)(x))&0x1FFFFFFF)

#define MAX_SEMAS 256
#define STACK_RES 0x2A0

//?
#define SRInitVal 0x70030C13
#define ConfigInitVal 0x73003

enum {
	THS_RUN     = 0x01,
	THS_READY   = 0x02,
	THS_WAIT    = 0x04,
	THS_SUSPEND = 0x08,
	THS_DORMANT = 0x10,
};

typedef struct {
	u128 gpr[24]; // v0-t9 (skip r0,at,k0-ra)
	u128 gp;
	u128 fp;
	u128 hi;
	u128 lo;
	u32  sa;
} eeRegs;

struct ThreadParam {
	int		status;
	void	(*entry)(void*);
	void	*stack;
	int		stackSize;
	void	*gpReg;
	int		initPriority;
	int		currentPriority;
	u32		attr;
	u32		option;
	int		waitSema; // waitType?
	int		waitId;
	int		wakeupCount;
};

struct TCB { //internal struct
	struct TCB	*next; //+00
	struct TCB	*prev; //+04
	int 		status;//+08
	void		(*entry)(void*); //+0C
	void		*stack_res;		//+10 initial $sp
	void		*gpReg; //+14
	short		currentPriority; //+18
	short		initPriority; //+1A
	int			waitSema, //+1C waitType?
                semaId, //+20
                wakeupCount, //+24
                attr, //+28
                option; //+2C
	void		(*entry_)(void*); //+30
	int			argc; //+34
	char		*argstring;//+38
	void		*stack;//+3C
	int			stackSize; //+40
	int			(*root)(); //+44
	void*		heap_base; //+48
};

struct threadCtx {
	u128 gpr[25]; // at-t9, (skip r0,k0-ra)
	u128 gp; //+190
	u128 sp; //+1A0
	u128 fp; //+1B0
	u128 ra; //+1C0
	u128 hi; //+1D0
	u128 lo; //+1E0
	u32 sa;  //+1F0
	u32 cf31;//+1F4
	u32 acc; //+1F8
	u32 res; //+1FC
	u32 fpr[32];//+200
};

typedef struct tag_ARGS{
	int	  argc;
	char *argv[16];
	char  args[256];
} ARGS;

struct SemaParam {
	int count;
	int max_count;
	int init_count;
	int wait_threads;
	int attr;
	u32 option;
};

struct kSema { // internal struct
	struct kSema	*free;//+00
	int 			count;//+04
	int 			max_count;//+08
	int 			attr;//+0C
	int 			option;//+10
	int 			wait_threads;//+14
	struct TCB		*wait_next,//+18
                    *wait_prev;//+1C
};

struct ll { struct ll *next, *prev; };			//linked list

//internal struct
struct IDhandl { //intc dmac handler
	struct ll *next, //+00
        *prev; //+04
	int (*handler)(int); //+08
	u32 gp; //+0C
	void *arg; //+10
	int flag; //+14
};

//internal struct
struct HCinfo{ //handler cause info
	int		count;
	struct ll	l;
};

extern u128 SavedSP;
extern u128 SavedRA;
extern u128 SavedAT;
extern u64  SavedT9;

extern eeRegs SavedRegs;

extern u32  excepRA;
extern u32  excepSP;

extern u32 (*table_CpuConfig[6])(u32);
extern void (*table_SYSCALL[0x80])();

extern void (*VCRTable[14])();
extern void (*VIntTable[8])();
extern void (*INTCTable[16])(int);
extern void (*DMACTable[16])(int);

extern int	threadId;
extern int	threadPrio;
extern int	threadStatus;

extern u64 hvParam;

extern u32 machineType;
extern u64 gsIMR;
extern u32 memorySize;

extern u32 g_kernelstackend;

extern u32 dmac_CHCR[10];

extern int VSyncFlag0;
extern int VSyncFlag1;

extern int _HandlersCount;
extern struct ll	handler_ll_free, *ihandlers_last, *ihandlers_first;
extern struct HCinfo	intcs_array[14];
extern struct ll	*dhandlers_last, *dhandlers_first;
extern struct HCinfo	dmacs_array[15];
extern struct IDhandl	pgifhandlers_array[161];
extern void (*sbus_handlers[32])(int ca);

extern int 		rcnt3Code;
extern int 		rcnt3TargetTable[0x142];
extern u8       rcnt3TargetNum[0x40];
extern int		threads_count;
extern struct ll	thread_ll_free;
extern struct ll	thread_ll_priorities[128];
extern int		semas_count;
extern struct kSema* semas_last;

extern struct TCB	threads_array[256];
extern struct kSema	semas_array[256];

extern char		tagindex;
extern short		transferscount;
extern struct TAG{
	int id_qwc;
	int addr;
	int unk[2];
}		tadrptr[31];
extern int	extrastorage[(16/4) * 8][31];

extern int	osdConfigParam;

// syscalls
// Every syscall is officially prefixed with _ (to avoid clashing with ps2sdk)
void _SetSYSCALL(int num, int address);
int  __EnableIntc(int ch);
int  __DisableIntc(int ch);
int  __EnableDmac(int ch);
int  __DisableDmac(int ch);
void *_SetVTLBRefillHandler(int cause, void (*handler)());
void *_SetVCommonHandler(int cause, void (*handler)());
void *_SetVInterruptHandler(int cause, void (*handler)());
void _PSMode();
u32  _MachineType();
u32 _SetMemorySize(u32 size);
u32 _GetMemorySize();
u64  _GsGetIMR();
u64 _GsPutIMR(u64 val);
int  _Exit(); // 3
void _RFU___(); // 0
void _SetVSyncFlag(int flag0, int flag1);
int  _AddIntcHandler(int cause, int (*handler)(int), int next, void *arg);
int  _AddIntcHandler2(int cause, int (*handler)(int), int next, void *arg);
int  _RemoveIntcHandler(int cause, int hid);
int  _AddDmacHandler(int cause, int (*handler)(int), int next, void *arg);
int  _AddDmacHandler2(int cause, int (*handler)(int), int next, void *arg);
int  _RemoveDmacHandler(int code, int hid);
int  _AddSbusIntcHandler(int cause, void (*handler)(int ca));
int  _RemoveSbusIntcHandler(int cause);
int  _Interrupt2Iop(int cause);
void _RFU005();
int  _GetCop0(int reg);
int _ExecPS2(void *, void *, int, char **);
int  _DeleteThread(int tid);
int  _StartThread(int tid, void *arg);
int  _ExitThread();
int  _ExitDeleteThread();
int  _SleepThread();
int  _WakeupThread(int tid);
int  _WaitSema(int sid);
void  _ChangeThreadPriority(int tid, int prio);
int  _CreateThread(struct ThreadParam *param);
int  _iChangeThreadPriority(int tid, int prio);
int  _GetThreadId();
int  _ReferThreadStatus(int tid, struct ThreadParam *info);
int  _iWakeupThread(int tid);
int  _SuspendThread(int thid);
int  _iResumeThread(int tid);
int  _CancelWakeupThread(int tid);
int  _CreateEventFlag();
int  _CreateSema(struct SemaParam *sema);
int  _RFU073(int sid);
int  _iSignalSema(int sid);
int  _PollSema(int sid);
int  _ReferSemaStatus(int sid, struct SemaParam *sema);
int  _DeleteEventFlag();
void*_InitializeMainThread(u32 gp, void *stack, int stack_size, 
						char *args, int root);
void *_InitializeHeapArea(void *heap_base, int heap_size);
void *_EndOfHeap();
int _LoadPS2Exe(char *filename, int argc, char **argv);
int _ExecOSD(int argc, char **argv);
void _SifSetDChain();
void _SifStopDma();
u32 _SifSetDma(SifDmaTransfer_t *sdd, int len);
void _SetGsCrt(short arg0, short arg1, short arg2); // 2
void _GetGsHParam(int *p0, int *p1, int *p2, int *p3);
int  _GetGsVParam();
void _SetGsVParam(int VParam);
void _GetOsdConfigParam(int *result);
void _SetOsdConfigParam(int *param);
int  _ResetEE(int init); // 1
int  _TlbWriteRandom(u32 PageMask, u32 EntryHi, u32 EntryLo0, u32 EntryLo1);
int _SetAlarm(short a0, int a1, int a2);
void _ReleaseAlarm();
int _TerminateThread(int tid);
void _RotateThreadReadyQueue(int prio);
int _iTerminateThread(int tid);
int _DisableDispatchThread();
int _EnableDispatchThread();
int  _iRotateThreadReadyQueue(int prio);
void _ReleaseWaitThread(int tid);
int _iReleaseWaitThread(int tid);
int _ResumeThread(int tid);
int  _iResumeThread(int tid);
void _JoinThread();
int _DeleteSema(int sid);
int _iDeleteSema(int sid);
void _SignalSema(int sid);
void _SetGsHParam(int a0, int a1, int a2, int a3);
int _SetEventFlag(int ef, u32 bits); // bits is EF_X
int _iSetEventFlag(int ef, u32 bits);
void _EnableIntcHandler(u32 id);
void _DisableIntcHandler(u32 id);
void _EnableDmacHandler(u32 id);
void _DisableDmacHandler(u32 id);
void _KSeg0(u32 arg);
int _EnableCache(int cache);
int _DisableCache(int cache);
void _FlushCache(int op);
void _105(int op1, int op2);
u32 _CpuConfig(u32 op);
void _SetCPUTimerHandler(void (*handler)());
void _SetCPUTimer(int compval);
void _SetPgifHandler(void (*handler)(int));
void _print();
int _SifDmaStat(int id);

int _SifGetReg(int reg);
int _SifSetReg(int reg, u32 val);

////////////////////////////////////////////////////////////////////
//			KERNEL BSS				  //
////////////////////////////////////////////////////////////////////


#endif

