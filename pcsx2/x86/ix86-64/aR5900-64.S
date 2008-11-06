// iR5900.c assembly routines
// zerofrog(@gmail.com)
.intel_syntax

.extern cpuRegs
.extern recRecompile
.extern recLUT
.extern lbase
.extern s_pCurBlock_ltime
.extern g_EEFreezeRegs
        
#define BLOCKTYPE_STARTPC	4		// startpc offset
#define BLOCKTYPE_DELAYSLOT	1		// if bit set, delay slot

#define BASEBLOCK_SIZE 2 // in dwords
#define PCOFFSET 0x2a8

#define REG_PC %edi
#define REG_BLOCK %r14 // preserved across calls
#define REG_BLOCKd %r14d

.globl R5900Execute
R5900Execute:
		push %rbx
		push %rbp
		push %r12
		push %r13
		push %r14
		push %r15
		
        // calc PC_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov %eax, dword ptr [cpuRegs + PCOFFSET]
        mov REG_PC, %eax
        mov REG_BLOCKd, %eax
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [recLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]
		
		// g_EEFreezeRegs = 1;
		mov dword ptr [g_EEFreezeRegs], 1
		 
		cmp REG_PC, [REG_BLOCK+4]
		jne Execute_Recompile
        mov %edx, [REG_BLOCK]
        and %rdx, 0xfffffff // pFnptr
        jnz Execute_Function
        
Execute_Recompile:
		call recRecompile
		mov %edx, [REG_BLOCK]
        and %rdx, 0xfffffff // pFnptr
        
Execute_Function:
		call %rdx
		
		// g_EEFreezeRegs = 0;
		mov dword ptr [g_EEFreezeRegs], 0
		
		pop %r15
		pop %r14
		pop %r13
		pop %r12
		pop %rbp
		pop %rbx
		
		ret
        
.globl Dispatcher
Dispatcher:
        // EDX contains the jump addr to modify
        push %rdx

        // calc PC_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov %eax, dword ptr [cpuRegs + PCOFFSET]
        mov REG_PC, %eax
        mov REG_BLOCKd, %eax
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [recLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]
	
        // check if startpc == cpuRegs.pc
        //and %ecx, 0x5fffffff // remove higher bits
        cmp REG_PC, dword ptr [REG_BLOCK+BLOCKTYPE_STARTPC]
        je Dispatcher_CheckPtr

        // recompile
        call recRecompile
Dispatcher_CheckPtr:
        mov REG_BLOCKd, dword ptr [REG_BLOCK]

#ifdef _DEBUG
        test REG_BLOCKd, REG_BLOCKd
        jnz Dispatcher_CallFn
        // throw an exception
        int 10
	
Dispatcher_CallFn:
#endif

        and REG_BLOCK, 0x0fffffff
        mov %rdx, REG_BLOCK
        pop %rcx // x86Ptr to mod
        sub %rdx, %rcx
        sub %rdx, 4
        mov [%rcx], %edx

        jmp REG_BLOCK

        
.globl DispatcherClear
DispatcherClear:
        // EDX contains the current pc
        mov dword ptr [cpuRegs + PCOFFSET], %edx
        mov %eax, %edx

        // calc PC_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov REG_BLOCKd, %edx
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [recLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]

        cmp %edx, dword ptr [REG_BLOCK + BLOCKTYPE_STARTPC]
        jne DispatcherClear_Recompile
	
        mov %eax, dword ptr [REG_BLOCK]
	
#ifdef _DEBUG
        test %eax, %eax
        jnz DispatcherClear_CallFn
        // throw an exception
        int 10
	
DispatcherClear_CallFn:
#endif

        and %rax, 0x0fffffff
        jmp %rax

DispatcherClear_Recompile:
        mov REG_PC, %edx
        call recRecompile
        mov %eax, dword ptr [REG_BLOCK]

        // r15 holds the prev x86 pointer
        and %rax, 0x0fffffff
        mov byte ptr [%r15], 0xe9 // jmp32
        mov %rdx, %rax
        sub %rdx, %r15
        sub %rdx, 5
        mov [%r15+1], %edx

        jmp %rax


        
// called when jumping to variable pc address
.globl DispatcherReg
DispatcherReg:
        //s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc)
        mov %eax, dword ptr [cpuRegs + PCOFFSET]
        mov REG_PC, %eax
        mov REG_BLOCKd, %eax
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [recLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]
	
        // check if startpc == cpuRegs.pc
        //and %eax, 0x5fffffff // remove higher bits
        cmp REG_PC, dword ptr [REG_BLOCK+BLOCKTYPE_STARTPC]
        jne DispatcherReg_recomp

        mov REG_BLOCKd, dword ptr [REG_BLOCK]

#ifdef _DEBUG
        test REG_BLOCKd, REG_BLOCKd
        jnz CallFn2
        // throw an exception
        int 10
	
CallFn2:

#endif

        and REG_BLOCK, 0x0fffffff
        jmp REG_BLOCK // fnptr

DispatcherReg_recomp:
        call recRecompile
	
        mov %eax, dword ptr [REG_BLOCK]
        and %rax, 0x0fffffff
        jmp %rax // fnptr

        
.globl _StartPerfCounter
_StartPerfCounter:

	push %rax
	push %rbx
	push %rcx

	rdtsc
	mov dword ptr [lbase], %eax
	mov dword ptr [lbase + 4], %edx

	pop %rcx
	pop %rbx
	pop %rax
	ret

.globl _StopPerfCounter
_StopPerfCounter:

	push %rax
	push %rbx
	push %rcx

	rdtsc

	sub %eax, dword ptr [lbase]
	sbb %edx, dword ptr [lbase + 4]
	mov %ecx, s_pCurBlock_ltime
	add %eax, dword ptr [%ecx]
	adc %edx, dword ptr [%ecx + 4]
	mov dword ptr [%ecx], %eax
	mov dword ptr [%ecx + 4], %edx
	pop %rcx
	pop %rbx
	pop %rax
	ret
