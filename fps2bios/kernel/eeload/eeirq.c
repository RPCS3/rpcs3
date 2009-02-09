// EE core interrupt and exception handlers
// most functions here can only use $at, $k0, and $k1
// [made by] [RO]man, zerofrog

#include "eekernel.h"
#include "eeirq.h"


#define LOAD_KERNELSTACK \
    "lui $sp, %hi(g_kernelstackend)\n" \
    "addiu $sp, %lo(g_kernelstackend)\n"

__asm__(".org 0x0000");

__asm__(".set noreorder");
void CpuException0() {
	__asm__ (
		"lui   $26, %hi(SavedT9)\n"
		"sd    $25, %lo(SavedT9)($26)\n"

		"mfc0  $25, $13\n"
		"andi  $25, 0x7C\n"
		"lui   $26, %hi(VCRTable)\n"
		"addu  $26, $25\n"
		"lw    $26, %lo(VCRTable)($26)\n"

		"lui   $25, %hi(SavedT9)\n"
		"jr    $26\n"
		"ld    $25, %lo(SavedT9)($25)\n"
	);
}

__asm__(".org 0x0180");

__asm__(".set noreorder");
void CpuException() {
	__asm__ (
		"lui   $26, %hi(SavedT9)\n"
		"sd    $25, %lo(SavedT9)($26)\n"

		"mfc0  $25, $13\n"
		"andi  $25, 0x7C\n"
		"lui   $26, %hi(VCRTable)\n"
		"addu  $26, $25\n"
		"lw    $26, %lo(VCRTable)($26)\n"

		"lui   $25, %hi(SavedT9)\n"
		"jr    $26\n"
		"ld    $25, %lo(SavedT9)($25)\n"
	);
}

__asm__(".org 0x0200");

__asm__(".set noreorder");
__asm__(".set noat");
void CpuException2() {
	__asm__ (
		"lui   $26, %hi(SavedSP)\n"
		"sq    $sp, %lo(SavedSP)($26)\n"
		"lui   $26, %hi(SavedRA)\n"
		"sq    $31, %lo(SavedRA)($26)\n"
		"lui   $26, %hi(SavedAT)\n"
		"sq    $1,  %lo(SavedAT)($26)\n"

		"mfc0  $1,  $13\n"
		"mfc0  $26, $14\n"
		"and   $1,  $26\n"
		"srl   $1,  8\n"
		"andi  $1,  0xFF\n"

		"plzcw $26, $1\n"
		"andi  $26, 0xFF\n"
		"ori   $1,  $0, 0x1E\n"
		"subu  $1,  $26\n"
		"sll   $1,  2\n"
		"lui   $26, %hi(VIntTable)\n"
		"addu  $26, $25\n"
		"lw    $26, %lo(VIntTable)($26)\n"
		"jr    $26\n"
		"nop\n"
	);
}

extern char call_used_regs[];
extern char fixed_regs[];

__asm__(".org 0x0280");
__asm__(".set noreorder");

void SyscException()
{
    const register int code __asm__("$3"); // $v1
    
	if (code < 0) {
        __asm__(
            "addiu $sp, -0x10\n"
            "sw    $31, 0($sp)\n"
            "mfc0  $26, $14\n"
            "addiu $26, 4\n"
            "sw    $26, 4($sp)\n"
            "mtc0  $26, $14\n"
            "sync\n");

		table_SYSCALL[-code]();
        
		__asm__(
            "lw    $26, 4($sp)\n"
            "lw    $31, 0($sp)\n"
            "addiu $sp, 0x10\n"
            "mtc0  $26, $14\n"
            "sync\n"
            "eret\n"
            "nop\n");
	}

	if (code == 0x7c) {
		_Deci2Call();
        return;
	}

    __asm__(
		"lui   $26, %hi(SavedSP)\n"
		"sq    $sp, %lo(SavedSP)($26)\n"
		"lui   $26, %hi(SavedRA)\n"
		"sq    $31, %lo(SavedRA)($26)\n"
		"lui   $26, %hi(SavedAT)\n"
		"sq    $1,  %lo(SavedAT)($26)\n"

		"mfc0  $1,  $12\n"
		"addiu $26, $0, 0xFFE4\n"
		"and   $1,  $26\n"
		"mtc0  $1,  $12\n"
		"sync\n"

		"move  $26, $sp\n"
        LOAD_KERNELSTACK
		"addiu $sp, -0x10\n"
		"sw    $31, 0($sp)\n"
		"sw    $26, 4($sp)\n"
		"mfc0  $26, $14\n"
		"addiu $26, 4\n"
		"sw    $26, 8($sp)\n"
		"mtc0  $26, $14\n"
		"sync\n");

	table_SYSCALL[code]();

    __asm__(
        "lw    $26, 8($sp)\n"
		"lw    $31, 0($sp)\n"
		"lw    $sp, 4($sp)\n"
		"mtc0  $26, $14\n"
		"sync\n"

		"mfc0  $26, $12\n"
		"ori   $26, 0x13\n"
		"mtc0  $26, $12\n"
		"sync\n"
		"eret\n"
		"nop\n");
}

void _Deci2Call() {
	__puts("_Deci2Call called\n");
}

void __ThreadHandler();

void INTCException() {
	u32 code;
	u32 temp;

	code = INTC_STAT & INTC_MASK;
	if (code & 0xC0) {
		int VpuStat;

		__asm__("cfc2 %0, $29\n" : "=r"(VpuStat) : );
		if (VpuStat & 0x202) {
            __asm__(
                ".set noat\n"
                "lui   $26, %hi(SavedAT)\n"
                "lq    $1,  %lo(SavedAT)($26)\n"
                ".set at\n");
			__exception();
		}
	}
	__asm__ (
		"mfc0  $26, $14\n"
		"li    $27, 0xFFFFFFE4\n"
		"and   $26, $27\n"
		"mtc0  $26, $14\n"
		"sync\n"

        LOAD_KERNELSTACK
		"addiu $sp, -0x10\n"
		"mfc0  $26, $14\n"
		"sw    $26, 0($sp)\n"
	);
	saveContext2();

	__asm__ (
		"plzcw %0, %1"
		: "=r"(temp) : "r"(code) 
	);
	temp = 0x1e - (temp & 0xff);
	INTC_STAT = 1 << temp;
	threadStatus = 0;

	INTCTable[temp](temp);

	restoreContext2();

	__asm__ (
        ".set noat\n"
		"lw    $26, 0($sp)\n"
		"mtc0  $26, $14\n"
		"lui   $26, %hi(SavedSP)\n"
		"lq    $sp, %lo(SavedSP)($26)\n"
		"lui   $26, %hi(SavedRA)\n"
		"lq    $31, %lo(SavedRA)($26)\n"
		"lui   $26, %hi(SavedAT)\n"
		"lq    $1,  %lo(SavedAT)($26)\n"
		".set at\n"
	);

	if (!threadStatus) {
		__asm__ (
			"mfc0  $26, $12\n"
			"ori   $26, 0x13\n"
			"mtc0  $26, $12\n"
			"sync\n"
			"eret\n"
		);
	}
    __asm__(LOAD_KERNELSTACK);
	threadStatus = 0;

	__ThreadHandler();
}

////////////////////////////////////////////////////////////////////
//800004C0
////////////////////////////////////////////////////////////////////
void DMACException() {
	unsigned int code;
	unsigned int temp;

	code = (DMAC_STAT >> 16) | 0x8000;
	code&=  DMAC_STAT & 0xFFFF;
	if (code & 0x80) {
		//__printf("%s: code & 0x80\n", __FUNCTION__);
        __asm__(
                ".set noat\n"
                "lui   $26, %hi(SavedAT)\n"
                "lq    $1,  %lo(SavedAT)($26)\n"
                ".set at\n");
		__exception1();
	}
	__asm__ (
		"mfc0  $26, $14\n"
		"li    $27, 0xFFFFFFE4\n"
		"and   $26, $27\n"
		"mtc0  $26, $14\n"
		"sync\n"

        LOAD_KERNELSTACK
		"addiu $sp, -0x10\n"
		"mfc0  $26, $14\n"
		"sw    $26, 0($sp)\n"
	);
	saveContext2();

	__asm__ (
		"plzcw %0, %1"
		: "=r"(temp) : "r"(code) 
	);
	temp = 0x1e - (temp & 0xff);
	DMAC_STAT = 1 << temp;
	threadStatus = 0;

	DMACTable[temp](temp);

	restoreContext2();

	__asm__ (
		".set noat\n"
		"lw    $26, 0($sp)\n"
		"mtc0  $26, $14\n"
		"lui   $26, %hi(SavedSP)\n"
		"lq    $sp, %lo(SavedSP)($26)\n"
		"lui   $26, %hi(SavedRA)\n"
		"lq    $31, %lo(SavedRA)($26)\n"
		"lui   $26, %hi(SavedAT)\n"
		"lq    $1,  %lo(SavedAT)($26)\n"
		".set at\n"
	);

	if (!threadStatus) {
		__asm__ (
			"mfc0  $26, $12\n"
			"ori   $26, 0x13\n"
			"mtc0  $26, $12\n"
			"sync\n"
			"eret\n"
		);
	}
    __asm__(LOAD_KERNELSTACK);
	threadStatus = 0;

	__ThreadHandler();
}

////////////////////////////////////////////////////////////////////
//80000600
////////////////////////////////////////////////////////////////////
void TIMERException()
{
    
}

////////////////////////////////////////////////////////////////////
//80000700
////////////////////////////////////////////////////////////////////
void setINTCHandler(int n, void (*phandler)(int))
{
    INTCTable[n] = phandler;
}

////////////////////////////////////////////////////////////////////
//80000780
////////////////////////////////////////////////////////////////////
void setDMACHandler(int n, void (*phandler)(int))
{
    DMACTable[n] = phandler;
}
