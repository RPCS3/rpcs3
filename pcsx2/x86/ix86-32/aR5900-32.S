// iR5900.c assembly routines
// zerofrog(@gmail.com)
.intel_syntax

.extern cpuRegs
.extern recRecompile
//.extern recLUT
.extern lbase
.extern s_pCurBlock_ltime

#define BLOCKTYPE_STARTPC	4		// startpc offset
#define BLOCKTYPE_DELAYSLOT	1		// if bit set, delay slot

#define BASEBLOCK_SIZE 2 // in dwords
#define PCOFFSET 0x2a8

#define REG_PC %ecx
#define REG_BLOCK %esi

.globl Dispatcher
Dispatcher:
	# EDX contains the jump addr to modify
	push %edx

	# calc PC_GETBLOCK
	# ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
	mov %eax, dword ptr [cpuRegs + PCOFFSET]
	mov REG_BLOCK, %eax
	mov REG_PC, %eax
	shr %eax, 16   
	and REG_BLOCK, 0xffff
    shl %eax, 2
    add %eax, dword ptr [recLUT]
    shl REG_BLOCK, 1
	add REG_BLOCK, dword ptr [%eax]
	
	// check if startpc == cpuRegs.pc
	//and %ecx, 0x5fffffff // remove higher bits
	cmp REG_PC, dword ptr [REG_BLOCK+BLOCKTYPE_STARTPC]
	je Dispatcher_CheckPtr

	// recompile
	push REG_BLOCK
	push REG_PC // pc
	call recRecompile
	add %esp, 4 // pop old param
	pop REG_BLOCK
Dispatcher_CheckPtr:
	mov REG_BLOCK, dword ptr [REG_BLOCK]

#ifdef _DEBUG
	test REG_BLOCK, REG_BLOCK
	jnz Dispatcher_CallFn
	// throw an exception
	int 10
	
Dispatcher_CallFn:
#endif

	and REG_BLOCK, 0x0fffffff
	mov %edx, REG_BLOCK
	pop %ecx // x86Ptr to mod
	sub %edx, %ecx
	sub %edx, 4
	mov dword ptr [%ecx], %edx

	jmp REG_BLOCK

.globl DispatcherClear
DispatcherClear:
	// EDX contains the current pc
	mov dword ptr [cpuRegs + PCOFFSET], %edx

	// calc PC_GETBLOCK
	# ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
    mov %eax, %edx
	mov REG_BLOCK, %edx
	shr %eax, 16
	and REG_BLOCK, 0xffff
    shl %eax, 2
    add %eax, dword ptr [recLUT]
    shl REG_BLOCK, 1
	add REG_BLOCK, dword ptr [%eax]

	cmp %edx, dword ptr [REG_BLOCK + 4]
	jne DispatcherClear_Recompile
	
	add %esp, 4 // ignore stack
	mov %eax, dword ptr [REG_BLOCK]
	
#ifdef _DEBUG
	test %eax, %eax
	jnz DispatcherClear_CallFn
	# throw an exception
	int 10
	
DispatcherClear_CallFn:
#endif

	and %eax, 0x0fffffff
	jmp %eax

DispatcherClear_Recompile:
	push REG_BLOCK
	push %edx
	call recRecompile
	add %esp, 4 // pop old param
	pop REG_BLOCK
	mov %eax, dword ptr [REG_BLOCK]

	pop %ecx // old fnptr

	and %eax, 0x0fffffff
	mov byte ptr [%ecx], 0xe9 // jmp32
	mov %edx, %eax
	sub %edx, %ecx
	sub %edx, 5
	mov dword ptr [%ecx+1], %edx

	jmp %eax


// called when jumping to variable pc address
.globl DispatcherReg
DispatcherReg:

	//s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);
	mov %edx, dword ptr [cpuRegs+PCOFFSET]
	mov %ecx, %edx
	
	shr %edx, 14
	and %edx, 0xfffffffc
	add %edx, [recLUT]
	mov %edx, dword ptr [%edx]

	mov %eax, %ecx
	and %eax, 0xfffc
	// %edx += 2*%eax
	shl %eax, 1
	add %edx, %eax
	
	// check if startpc == cpuRegs.pc
	mov %eax, %ecx
	//and %eax, 0x5fffffff // remove higher bits
	cmp %eax, dword ptr [%edx+BLOCKTYPE_STARTPC]
	jne DispatcherReg_recomp

	mov %eax, dword ptr [%edx]

#ifdef _DEBUG
	test %eax, %eax
	jnz CallFn2
	# throw an exception
	int 10
	
CallFn2:

#endif

	and %eax, 0x0fffffff
	jmp %eax // fnptr

DispatcherReg_recomp:
	sub %esp, 8
	mov dword ptr [%esp+4], %edx
	mov dword ptr [%esp], %ecx
	call recRecompile
	mov %edx, dword ptr [%esp+4]
	add %esp, 8
	
	mov %eax, dword ptr [%edx]
	and %eax, 0x0fffffff
	jmp %eax // fnptr


.globl _StartPerfCounter
_StartPerfCounter:

	push %eax
	push %ebx
	push %ecx

	rdtsc
	mov dword ptr [lbase], %eax
	mov dword ptr [lbase + 4], %edx

	pop %ecx
	pop %ebx
	pop %eax
	ret

.globl _StopPerfCounter
_StopPerfCounter:

	push %eax
	push %ebx
	push %ecx

	rdtsc

	sub %eax, dword ptr [lbase]
	sbb %edx, dword ptr [lbase + 4]
	mov %ecx, s_pCurBlock_ltime
	add %eax, dword ptr [%ecx]
	adc %edx, dword ptr [%ecx + 4]
	mov dword ptr [%ecx], %eax
	mov dword ptr [%ecx + 4], %edx
	pop %ecx
	pop %ebx
	pop %eax
	ret
