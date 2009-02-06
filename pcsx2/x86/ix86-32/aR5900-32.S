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
#define PCOFFSET 0x2a8 // this must always match what Pcsx2 displays at startup

#define REG_BLOCK %esi

//////////////////////////////////////////////////////////////////////////
// Recompiles the next block, and links the old block directly to it.
// This is a on-shot execution for ny block which uses it.  Once the block
// has been statically linked to the new block, this function will be bypassed
//
.globl Dispatcher
Dispatcher:
	# EDX contains the jump addr to modify
	push %edx

	# calc PC_GETBLOCK
	# ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))

	mov %eax,dword ptr [cpuRegs+PCOFFSET] 
	mov %ecx,%eax		// ecx is the BLOCK address
	mov %esi,%eax		// esi is the PC address (leave unmodified!)
	shr %eax,0x10 
	and %ecx,0xFFFF
	mov %edx,dword ptr [recLUT] 
	mov %eax,dword ptr [%edx+%eax*4]
	lea %ecx,[%eax+%ecx*2] 

	// check if startpc == cpuRegs.pc
	//and %ecx, 0x5fffffff // remove higher bits
	cmp %esi, dword ptr [%ecx+BLOCKTYPE_STARTPC]
	je Dispatcher_CheckPtr

	// recompile
	push %ecx
	push %esi // pc
	call recRecompile
	add %esp, 4
	pop %ecx	// ecx is now the REG_BLOCK
Dispatcher_CheckPtr:
	mov %eax, dword ptr [%ecx]

#ifdef _DEBUG
	test %eax, %eax
	jnz Dispatcher_CallFn
	// throw an exception
	int 10
	
Dispatcher_CallFn:
#endif
	//and %eax, 0x0fffffff
	shl %eax, 4
	pop %ecx // x86Ptr to mod
	mov %edx, %eax
	sub %edx, %ecx
	sub %edx, 4
	mov dword ptr [%ecx], %edx

	jmp %eax

//////////////////////////////////////////////////////////////////////////
// edx -  baseblock->startpc
// stack - x86Ptr
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

	//and %eax, 0x0fffffff
	shl %eax, 4
	jmp %eax

DispatcherClear_Recompile:
	push REG_BLOCK
	push %edx
	call recRecompile
	add %esp, 4 // pop old param
	pop REG_BLOCK
	mov %eax, dword ptr [REG_BLOCK]

	pop %ecx // old fnptr

	//and %eax, 0x0fffffff
    shl %eax, 4
	mov byte ptr [%ecx], 0xe9 // jmp32
	mov %edx, %eax
	sub %edx, %ecx
	sub %edx, 5
	mov dword ptr [%ecx+1], %edx

	jmp %eax

//////////////////////////////////////////////////////////////////////////
// called when jumping to variable pc address
// This is basically the same as Dispatcher but without the part at the end
// that modifies the block's jmp instruction.  (ie, no static block linking)

.globl DispatcherReg
DispatcherReg:

	mov %eax,dword ptr [cpuRegs+PCOFFSET] 
	mov %ecx,%eax		// ecx will be the BLOCK 
	mov %esi,%eax		// esi is the PC address (leave unmodified!)
	shr %eax,0x10
	and %ecx,0xFFFF 
	mov %edx,dword ptr [recLUT] 
	mov %eax,dword ptr [%edx+%eax*4] 
	lea %ecx,[%eax+%ecx*2] 

	// check if startpc == cpuRegs.pc
	//and %ecx, 0x5fffffff // remove higher bits
	cmp %esi, dword ptr [%ecx+BLOCKTYPE_STARTPC]
	je DispatcherReg_CheckPtr

	// recompile
	push %ecx // block
	push %esi // pc
	call recRecompile
	add %esp, 4
	pop %ecx  // block
DispatcherReg_CheckPtr:
	mov %eax, dword ptr [%ecx]

#ifdef _DEBUG
	test %eax, %eax
	jnz DispatcherReg_CallFn
	// throw an exception
	int 10
	
DispatcherReg_CallFn:
#endif
	//and %eax, 0x0fffffff
    shl %eax, 4
	jmp %eax


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
