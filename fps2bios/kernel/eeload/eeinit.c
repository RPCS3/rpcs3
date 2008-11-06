// EE initialization functions
// [made by] [RO]man, zerofrog
#include <tamtypes.h>
#include <stdio.h>

#include "eekernel.h"
#include "eeirq.h"

void InitializeGS();
void InitializeGIF();
void InitializeDMAC(int code);
void InitializeVU1();
void InitializeVIF1();
void InitializeIPU();
void InitializeVIF0();
void InitializeVU0();
void InitializeFPU();
void InitializeScratchPad();
void InitializeUserMemory(u32 base);
void InitializeINTC(int a);
void InitializeTIMER();

////////////////////////////////////////////////////////////////////
//8000AA60
////////////////////////////////////////////////////////////////////
void InitializeGS()
{
}

////////////////////////////////////////////////////////////////////
//8000AB98
////////////////////////////////////////////////////////////////////
void InitializeGIF()
{
	GIF_CTRL = 1;
	__asm__ ("sync\n");
	GIF_FIFO = 0;
}

////////////////////////////////////////////////////////////////////
//8000AD68		SYSCALL 001 ResetEE
////////////////////////////////////////////////////////////////////
int  _ResetEE(int init)
{
	if (init & 0x01) { __printf("# Initialize DMAC ...\n"); InitializeDMAC(0x31F); }
	if (init & 0x02) { __printf("# Initialize VU1 ...\n");  InitializeVU1(); }
	if (init & 0x04) { __printf("# Initialize VIF1 ...\n"); InitializeVIF1(); }
	if (init & 0x08) { __printf("# Initialize GIF ...\n");  InitializeGIF(); }
	if (init & 0x10) { __printf("# Initialize VU0 ...\n");  InitializeVU0(); }
	if (init & 0x04) { __printf("# Initialize VIF0 ...\n"); InitializeVIF0(); }
	if (init & 0x40) { __printf("# Initialize IPU ...\n");  InitializeIPU(); }

	InitializeINTC(0xC);
//	return (*(int*)0x1000F410 &= 0xFFFBFFFF); code never reached :)
}

////////////////////////////////////////////////////////////////////
//8000AE88
////////////////////////////////////////////////////////////////////
int Initialize()
{
	__printf("# Initialize Start.\n");
	__printf("# Initialize GS ...");

	InitializeGS();
	_SetGsCrt(1, 2, 1);
	__printf("\n");

	__printf("# Initialize INTC ...\n");
	InitializeINTC(0xFFFF);

	__printf("# Initialize TIMER ...\n");
	InitializeTIMER();

	ResetEE(0x7F);

	__printf("# Initialize FPU ...\n");
	InitializeFPU();

	__printf("# Initialize User Memory ...\n");
	InitializeUserMemory(0x80000);

	__printf("# Initialize Scratch Pad ...\n");
	InitializeScratchPad();

	__printf("# Initialize Done.\n");
}

////////////////////////////////////////////////////////////////////
//8000AF50
////////////////////////////////////////////////////////////////////
int  Restart()
{
	__printf("# Restart.\n");
	__printf("# Initialize GS ...");

	INTC_STAT = 4;
	while (INTC_STAT & 4) { __asm__ ("nop\nnop\nnop\n"); }
	INTC_STAT = 4;

	InitializeGS();
	_SetGsCrt(1, 2, 1);
	__printf("\n");

	__printf("# Initialize INTC ...\n");
	InitializeINTC(0xDFFD);

	__printf("# Initialize TIMER ...\n");
	InitializeTIMER();

	ResetEE(0x7F);

	__printf("# Initialize FPU ...\n");
	InitializeFPU();

	__printf("# Initialize User Memory ...\n");
	InitializeUserMemory(0x82000);

	__printf("# Initialize Scratch Pad ...\n");
	InitializeScratchPad();

	__printf("# Restart Done.\n");

	//wait for syncing IOP
	while (SBUS_SMFLG & SBFLG_IOPSYNC) { __asm__ ("nop\nnop\nnop\n"); }

	SBUS_SMFLG=SBFLG_IOPSYNC;
}

////////////////////////////////////////////////////////////////////
//8000B0A0
////////////////////////////////////////////////////////////////////
void InitializeDMAC(int code)
{
	int i;
	int *addr;

	for (i=0; i<10; i++) {
		if (!(code & (1<<i))) continue;

		addr = (int*)dmac_CHCR[i];
		addr[0x80/4] = 0;
		addr[0x00/4] = 0;
		addr[0x30/4] = 0;
		addr[0x10/4] = 0;
		addr[0x50/4] = 0;
		addr[0x40/4] = 0;
	}

	DMAC_STAT = (code & 0xFFFF) | 0xE000;
	DMAC_STAT = (DMAC_STAT & 0xFFFF0000) & ((code | 0x6000) << 16);

	if ((code & 0x7F) == 0x7F) {
		DMAC_CTRL = 0;
		DMAC_PCR  = 0;
	} else {
		DMAC_CTRL = 0;
		DMAC_PCR  = DMAC_PCR & (((~code) << 16) | (~code));
	}

	DMAC_SQWC = 0;
	DMAC_RBSR = 0;
	DMAC_RBOR = 0;
}

////////////////////////////////////////////////////////////////////
//8000B1F0
////////////////////////////////////////////////////////////////////
void InitializeVU1()
{
}

////////////////////////////////////////////////////////////////////
//8000B2D8
////////////////////////////////////////////////////////////////////
void InitializeVIF1()
{
}

////////////////////////////////////////////////////////////////////
//8000B3B0
////////////////////////////////////////////////////////////////////
void InitializeIPU()
{
}

////////////////////////////////////////////////////////////////////
//8000B6F8
////////////////////////////////////////////////////////////////////
void InitializeVIF0()
{
}

////////////////////////////////////////////////////////////////////
//8000B778
////////////////////////////////////////////////////////////////////
void InitializeVU0()
{
}

////////////////////////////////////////////////////////////////////
//8000B7A8
////////////////////////////////////////////////////////////////////
void InitializeFPU()
{
	__asm__ (
		"mtc1    $0, $0\n"
		"mtc1    $0, $1\n"
		"mtc1    $0, $2\n"
		"mtc1    $0, $3\n"
		"mtc1    $0, $4\n"
		"mtc1    $0, $5\n"
		"mtc1    $0, $6\n"
		"mtc1    $0, $7\n"
		"mtc1    $0, $8\n"
		"mtc1    $0, $9\n"
		"mtc1    $0, $10\n"
		"mtc1    $0, $11\n"
		"mtc1    $0, $12\n"
		"mtc1    $0, $13\n"
		"mtc1    $0, $14\n"
		"mtc1    $0, $15\n"
		"mtc1    $0, $16\n"
		"mtc1    $0, $17\n"
		"mtc1    $0, $18\n"
		"mtc1    $0, $19\n"
		"mtc1    $0, $20\n"
		"mtc1    $0, $21\n"
		"mtc1    $0, $22\n"
		"mtc1    $0, $23\n"
		"mtc1    $0, $24\n"
		"mtc1    $0, $25\n"
		"mtc1    $0, $26\n"
		"mtc1    $0, $27\n"
		"mtc1    $0, $28\n"
		"mtc1    $0, $29\n"
		"mtc1    $0, $30\n"
		"mtc1    $0, $31\n"
		"adda.s  $f0, $f1\n"
		"sync\n"
		"ctc1    $0, $31\n"
	);
}

////////////////////////////////////////////////////////////////////
//8000B840
////////////////////////////////////////////////////////////////////
void InitializeScratchPad()
{
	u128 *p;

	for (p=(u128*)0x70000000; (u32)p<0x70004000; p++) *p=0;
}

////////////////////////////////////////////////////////////////////
//8000B878
////////////////////////////////////////////////////////////////////
void InitializeUserMemory(u32 base)
{
	u128 *memsz;
	u128 *p = (u128*)base;
	
	for (memsz=(u128*)_GetMemorySize(); p<memsz; p++)
        *p = 0;
}

////////////////////////////////////////////////////////////////////
//8000B8D0
////////////////////////////////////////////////////////////////////
void InitializeINTC(int a)
{
	a &= 0xDFFD;
	INTC_STAT &= a;
	INTC_MASK &= a;
}

////////////////////////////////////////////////////////////////////
//8000B900
////////////////////////////////////////////////////////////////////
void InitializeTIMER()
{
	RCNT0_MODE   = 0xC00;
	RCNT0_COUNT  = 0;
	RCNT0_TARGET = 0xFFFF;
	RCNT0_HOLD   = 0;

	RCNT1_MODE   = 0xC00;
	RCNT1_COUNT  = 0;
	RCNT1_TARGET = 0xFFFF;
	RCNT1_HOLD   = 0;

	RCNT2_MODE   = 0xC80;
	RCNT2_COUNT  = 0;
	RCNT2_TARGET = 0xFFFF;

	RCNT3_MODE   = 0xC83;
	RCNT3_COUNT  = 0;
	RCNT3_TARGET = 0xFFFF;

	InitializeINTC(0x1E00);
	_InitRCNT3();
}

u32 tlb1_data[] = {
	0x00000000, 0x70000000, 0x80000007, 0x00000007,
	0x00006000, 0xFFFF8000, 0x00001E1F, 0x00001F1F,
	0x00000000, 0x10000000, 0x00400017, 0x00400053,
	0x00000000, 0x10002000, 0x00400097, 0x004000D7, 
	0x00000000, 0x10004000,	0x00400117,	0x00400157, 
	0x00000000, 0x10006000, 0x00400197, 0x004001D7,
	0x00000000, 0x10008000, 0x00400217, 0x00400257, 
	0x00000000, 0x1000A000,	0x00400297,	0x004002D7, 
	0x00000000, 0x1000C000, 0x00400313, 0x00400357,
	0x00000000, 0x1000E000, 0x00400397, 0x004003D7,
	0x0001E000, 0x11000000,	0x00440017,	0x00440415, 
	0x0001E000, 0x12000000, 0x00480017, 0x00480415,
	0x01FFE000, 0x1E000000, 0x00780017, 0x007C0017,
};

u32 tlb2_data[] = {
	0x0007E000, 0x00080000, 0x0000201F, 0x0000301F,
	0x0007E000, 0x00100000, 0x0000401F, 0x0000501F, 
	0x0007E000, 0x00180000, 0x0000601F, 0x0000701F,
	0x001FE000, 0x00200000, 0x0000801F, 0x0000C01F, 
	0x001FE000, 0x00400000, 0x0001001F, 0x0001401F,
	0x001FE000, 0x00600000, 0x0001801F, 0x0001C01F,
	0x007FE000,	0x00800000, 0x0002001F, 0x0003001F, 
	0x007FE000, 0x01000000,	0x0004001F, 0x0005001F,	
	0x007FE000, 0x01800000, 0x0006001F, 0x0007001F,
	0x0007E000, 0x20080000, 0x00002017, 0x00003017,
	0x0007E000, 0x20100000, 0x00004017, 0x00005017,
	0x0007E000, 0x20180000, 0x00006017, 0x00007017,
	0x001FE000,	0x20200000, 0x00008017, 0x0000C017,
	0x001FE000, 0x20400000, 0x00010017, 0x00014017,
	0x001FE000, 0x20600000, 0x00018017, 0x0001C017,
	0x007FE000, 0x20800000, 0x00020017, 0x00030017,
	0x007FE000,	0x21000000, 0x00040017, 0x00050017,
	0x007FE000,	0x21800000, 0x00060017, 0x00070017,
};

u32 tlb3_data[] = {
	0x0007E000, 0x30100000, 0x0000403F, 0x0000503F, 
	0x0007E000, 0x30180000, 0x0000603F, 0x0000703F, 
	0x001FE000,	0x30200000, 0x0000803F, 0x0000C03F,
	0x001FE000,	0x30400000, 0x0001003F, 0x0001403F, 
	0x001FE000,	0x30600000, 0x0001803F, 0x0001C03F, 
	0x007FE000, 0x30800000, 0x0002003F, 0x0003003F,	
	0x007FE000, 0x31000000, 0x0004003F, 0x0005003F, 
	0x007FE000, 0x31800000, 0x0006003F, 0x0007003F,
};

u32 tlb_config[] = {
	0xD, 0x12, 8, 0,
	0x80012708, 0x800127D8, 0x800128F8
};


void _TlbSet(u32 Index, u32 PageMask, u32 EntryHi, u32 EntryLo0, u32 EntryLo1) {
	__asm__ (
		"mtc0 $4, $0\n"
		"mtc0 $5, $5\n"
		"mtc0 $6, $10\n"
		"mtc0 $7, $2\n"
		"mtc0 $8, $3\n"
		"sync\n"
		"tlbwi\n"
		"sync\n"
	);
}

void TlbInit() {
	int i, last;
	u32 *ptr;

	__printf("TlbInit... ");

	__asm__ ("mtc0 $0, $6\n");

	_TlbSet(1, 0, 0xE0000000, 0, 0);
	for (i=2; i<48; i++) {
		_TlbSet(i, 0, 0xE0002000, 0, 0);
	}

	last = tlb_config[0];
	if (last > 48) {
		__printf("# TLB over flow (1)\n");
	}

	ptr = (u32*)(tlb_config[4] + 0x10);
	for (i=1; i<last; i++, ptr+= 4) {
		_TlbSet(i, ptr[0], ptr[1], ptr[2], ptr[3]);
	}

	last = tlb_config[1] + i;
	if (last > 48) {
		__printf("# TLB over flow (2)\n");
	}

	if (tlb_config[1]) {
		ptr = (u32*)(tlb_config[5]);
		for (; i<last; i++, ptr+= 4) {
			_TlbSet(i, ptr[0], ptr[1], ptr[2], ptr[3]);
		}
	}

	__asm__ (
		"mtc0 %0, $6\n"
		"sync\n"
		: : "r"(tlb_config[3])
	);
//	if (tlb_config[2] <= 0)	return;

	last = tlb_config[2] + i;
	if (last > 48) {
		__printf("# TLB over flow (3)\n");
	}

	ptr = (u32*)(tlb_config[6]);
	for (; i<last; i++, ptr+= 4) {
		_TlbSet(i, ptr[0], ptr[1], ptr[2], ptr[3]);
	}
	__printf("Ok\n");
}

void DefaultINTCHandler(int n);
void DefaultDMACHandler(int n);
void rcnt3Handler();
void sbusHandler();

////////////////////////////////////////////////////////////////////
//80002050
////////////////////////////////////////////////////////////////////
int InitPgifHandler() {
	int i;

	_HandlersCount = 0;
    
	for (i=0; i<15; i++) {
		intcs_array[i].count = 0;
        // intcs_array[-1] = ihandlers_last,first
		intcs_array[i-1].l.prev = (struct ll*)&intcs_array[i-1].l.next;
		intcs_array[i-1].l.next = (struct ll*)&intcs_array[i-1].l.next;
        setINTCHandler(i, DefaultINTCHandler);
	}

    setINTCHandler(12, rcnt3Handler);
    setINTCHandler(1,  sbusHandler);

	for (i=0; i<32; i++) {
		sbus_handlers[i] = 0;
	}
	
	__EnableIntc(INTC_SBUS);
	
	for (i=0; i<16; i++) {
		dmacs_array[i].count = 0;
		dmacs_array[i-1].l.prev = (struct ll*)&dmacs_array[i-1].l.next;
		dmacs_array[i-1].l.next = (struct ll*)&dmacs_array[i-1].l.next;
        setDMACHandler(i, DefaultDMACHandler);
	}

	handler_ll_free.next = &handler_ll_free;
	handler_ll_free.prev = &handler_ll_free;
	for (i=0; i<160; i++) {
		pgifhandlers_array[i+1].handler = 0;
		pgifhandlers_array[i+1].flag    = 3;
		LL_add(&handler_ll_free, (struct ll*)&pgifhandlers_array[i+1].next);
	}

	return 0x81;
}

////////////////////////////////////////////////////////////////////
//800021B0
// asme as InitPgifHandler except don't reset sbus
////////////////////////////////////////////////////////////////////
int InitPgifHandler2()
{
	int i;

	_HandlersCount = 0;
    
    for(i = 0; i < 15; ++i) {
        if(i != 1 ) {
            intcs_array[i].count = 0;
            // intcs_array[-1] = ihandlers_last,first
            intcs_array[i-1].l.prev = (struct ll*)&intcs_array[i-1].l.next;
            intcs_array[i-1].l.next = (struct ll*)&intcs_array[i-1].l.next;
            setINTCHandler(i, DefaultINTCHandler);
        }
	}

    setINTCHandler(12, rcnt3Handler);
	
	for (i=0; i<16; i++) {
		dmacs_array[i].count = 0;
		dmacs_array[i-1].l.prev = (struct ll*)&dmacs_array[i-1].l.next;
		dmacs_array[i-1].l.next = (struct ll*)&dmacs_array[i-1].l.next;
        setDMACHandler(i, DefaultDMACHandler);
	}

	handler_ll_free.next = &handler_ll_free;
	handler_ll_free.prev = &handler_ll_free;
	for (i=0; i<160; i++) {
		pgifhandlers_array[i+1].handler = 0;
		pgifhandlers_array[i+1].flag    = 3;
		LL_add(&handler_ll_free, (struct ll*)&pgifhandlers_array[i+1].next);
	}

	return 0x81;
}
