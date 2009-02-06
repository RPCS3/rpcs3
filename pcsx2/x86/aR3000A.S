// iR3000A.c assembly routines
// zerofrog(@gmail.com)
.intel_syntax

.extern psxRegs
.extern psxRecLUT
.extern psxRecRecompile
.extern b440
.extern b440table

#define PS2MEM_BASE_ 0x18000000 // has to match Memory.h
        
#define BLOCKTYPE_STARTPC	4		// startpc offset
#define BLOCKTYPE_DELAYSLOT	1		// if bit set, delay slot
        
#define BASEBLOCK_SIZE 2 // in dwords
#define PCOFFSET 0x208

#define PSX_MEMMASK 0x5fffffff

#ifdef __x86_64__
                
#define REG_PC %edi
#define REG_BLOCK %r12
#define REG_BLOCKd %r12d

.globl R3000AExecute
R3000AExecute:  

        push %rbx
        push %rbp
        push %r12
        push %r13
        push %r14
        push %r15

        // calc PSX_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov %eax, dword ptr [psxRegs + PCOFFSET]
        mov REG_PC, %eax
        mov REG_BLOCKd, %eax
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [psxRecLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]
        
        mov %r8d, [REG_BLOCK+4]
        mov %r9d, REG_PC
		and %r8d, 0x5fffffff
		and %r9d, 0x5fffffff
		cmp %r8d, %r9d
		jne Execute_Recompile
        mov %edx, [REG_BLOCK]
        and %rdx, 0xfffffff // pFnptr
        jnz Execute_Function
        
Execute_Recompile:
		call psxRecRecompile
		mov %edx, [REG_BLOCK]
		and %rdx, 0xfffffff // pFnptr
        
Execute_Function:
		call %rdx

		pop %r15
		pop %r14
		pop %r13
		pop %r12
		pop %rbp
		pop %rbx
		ret
		
        
// jumped to when invalid psxpc address
.globl psxDispatcher
psxDispatcher:
        // EDX contains the current psxpc to jump to, stack contains the jump addr to modify
        push %rdx

        // calc PSX_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov %eax, dword ptr [psxRegs + PCOFFSET]
        mov REG_PC, %eax
        mov REG_BLOCKd, %eax
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [psxRecLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]

        // check if startpc&PSX_MEMMASK == psxRegs.pc&PSX_MEMMASK
        mov %eax, REG_PC
        mov %edx, [REG_BLOCK+BLOCKTYPE_STARTPC]
        and %eax, PSX_MEMMASK // remove higher bits
        and %edx, PSX_MEMMASK
        cmp %eax, %edx
        je psxDispatcher_CheckPtr

        // recompile
        call psxRecRecompile
psxDispatcher_CheckPtr:
		mov REG_BLOCKd, dword ptr [REG_BLOCK]

#ifdef _DEBUG
        test REG_BLOCKd, REG_BLOCKd
        jnz psxDispatcher_CallFn
        // throw an exception
        int 10
	
psxDispatcher_CallFn:
#endif
        
		//and REG_BLOCK, 0x0fffffff
		shl REG_BLOCK, 4
        mov %rdx, REG_BLOCK
        pop %rcx // x86Ptr to mod
        sub %rdx, %rcx
        sub %rdx, 4
        mov [%rcx], %edx

        jmp REG_BLOCK

.globl psxDispatcherClear
psxDispatcherClear:
        // %EDX contains the current psxpc
        mov dword ptr [psxRegs + PCOFFSET], %edx
        mov %eax, %edx
                
        // calc PSX_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov REG_BLOCKd, %edx
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [psxRecLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]
        
        // check if startpc&PSX_MEMMASK == psxRegs.pc&PSX_MEMMASK
        mov REG_PC, %edx
        mov %eax, REG_PC
        mov %edx, [REG_BLOCK+BLOCKTYPE_STARTPC]
        and %eax, PSX_MEMMASK // remove higher bits
        and %edx, PSX_MEMMASK
        cmp %eax, %edx
        jne psxDispatcherClear_Recompile

        mov %eax, dword ptr [REG_BLOCK]
        
#ifdef _DEBUG
        test %eax, %eax
        jnz psxDispatcherClear_CallFn
        // throw an exception
        int 10
	
psxDispatcherClear_CallFn:
#endif

        //and %rax, 0x0fffffff
	shl %rax, 4
        jmp %rax
                
psxDispatcherClear_Recompile:
        call psxRecRecompile
        mov %eax, dword ptr [REG_BLOCK]

        // r15 holds the prev x86 pointer
        //and %rax, 0x0fffffff
	shl %rax, 4
        mov byte ptr [%r15], 0xe9 // jmp32
        mov %rdx, %rax
        sub %rdx, %r15
        sub %rdx, 5
        mov [%r15+1], %edx

        jmp %rax

// called when jumping to variable psxpc address
.globl psxDispatcherReg
psxDispatcherReg:

        //s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);
        mov %eax, dword ptr [psxRegs + PCOFFSET]
        mov REG_PC, %eax
        mov REG_BLOCKd, %eax
        shl %rax, 32
        shr %rax, 48
        and REG_BLOCK, 0xfffc
        shl %rax, 3
        add %rax, [psxRecLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, [%rax]
		
		// check if startpc == psxRegs.pc
		cmp REG_PC, dword ptr [REG_BLOCK+BLOCKTYPE_STARTPC]
		jne psxDispatcherReg_recomp

		mov REG_BLOCKd, dword ptr [REG_BLOCK]

#ifdef _DEBUG
        test %eax, %eax
        jnz psxDispatcherReg_CallFn2
        // throw an exception
        int 10
psxDispatcherReg_CallFn2:
#endif
		and REG_BLOCK, 0x0fffffff
        jmp REG_BLOCK // fnptr

psxDispatcherReg_recomp:
		call psxRecRecompile
	
        mov %eax, dword ptr [REG_BLOCK]
        //and %rax, 0x0fffffff
	shl, REG_BLOCK, 4
        jmp %rax // fnprt

#else // not x86-64
                
#define REG_PC %ecx
#define REG_BLOCK %esi

// jumped to when invalid psxpc address
.globl psxDispatcher
psxDispatcher:
        // EDX contains the current psxpc to jump to, stack contains the jump addr to modify
        push %edx

        // calc PSX_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov %eax, dword ptr [psxRegs + PCOFFSET]
        mov REG_BLOCK, %eax
        mov REG_PC, %eax
        shr %eax, 16
        and REG_BLOCK, 0xffff
        shl %eax, 2
        add %eax, [psxRecLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, dword ptr [%eax]

        // check if startpc&PSX_MEMMASK == psxRegs.pc&PSX_MEMMASK
        mov %eax, REG_PC
        mov %edx, [REG_BLOCK+BLOCKTYPE_STARTPC]
        and %eax, PSX_MEMMASK // remove higher bits
        and %edx, PSX_MEMMASK
        cmp %eax, %edx
        je psxDispatcher_CheckPtr

        // recompile
        push REG_BLOCK
        push REG_PC // psxpc
        call psxRecRecompile
        add %esp, 4 // pop old param
        pop REG_BLOCK
psxDispatcher_CheckPtr:
		mov REG_BLOCK, dword ptr [REG_BLOCK]

#ifdef _DEBUG
        test REG_BLOCK, REG_BLOCK
        jnz psxDispatcher_CallFn
        // throw an exception
        int 10
	
psxDispatcher_CallFn:
#endif
        
		//and REG_BLOCK, 0x0fffffff
		shl REG_BLOCK, 4
		mov %edx, REG_BLOCK
		pop %ecx // x86Ptr to mod
		sub %edx, %ecx
		sub %edx, 4
		mov dword ptr [%ecx], %edx

		jmp REG_BLOCK

.globl psxDispatcherClear
psxDispatcherClear:
        // %EDX contains the current psxpc
        mov dword ptr [psxRegs + PCOFFSET], %edx

        // calc PSX_GETBLOCK
        // ((BASEBLOCK*)(recLUT[((u32)(x)) >> 16] + (sizeof(BASEBLOCK)/4)*((x) & 0xffff)))
        mov %eax, %edx
        mov REG_BLOCK, %edx
        shr %eax, 16
        and REG_BLOCK, 0xffff
        shl %eax, 2
        add %eax, [psxRecLUT]
        shl REG_BLOCK, 1
        add REG_BLOCK, dword ptr [%eax];
        
        // check if startpc&PSX_MEMMASK == psxRegs.pc&PSX_MEMMASK
        mov %eax, %edx
        mov REG_PC, %edx
        mov %edx, [REG_BLOCK+BLOCKTYPE_STARTPC]
        and %eax, PSX_MEMMASK // remove higher bits
        and %edx, PSX_MEMMASK
        cmp %eax, %edx
        jne psxDispatcherClear_Recompile

        add %esp, 4 // ignore stack
        mov %eax, dword ptr [REG_BLOCK]
        
#ifdef _DEBUG
        test %eax, %eax
        jnz psxDispatcherClear_CallFn
        // throw an exception
        int 10
	
psxDispatcherClear_CallFn:
#endif

        //and %eax, 0x0fffffff
        shl %eax, 4
        jmp %eax
                
psxDispatcherClear_Recompile:
        push REG_BLOCK
        push REG_PC
		call psxRecRecompile
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

// called when jumping to variable psxpc address
.globl psxDispatcherReg
psxDispatcherReg:

        //s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);
		mov %edx, dword ptr [psxRegs+PCOFFSET]
		mov %ecx, %edx

		shr %edx, 14
		and %edx, 0xfffffffc
		add %edx, [psxRecLUT]
		mov %edx, dword ptr [%edx]

		mov %eax, %ecx
		and %eax, 0xfffc
		// %edx += 2*%eax
		shl %eax, 1
		add %edx, %eax
		
		// check if startpc == psxRegs.pc
		mov %eax, %ecx
		cmp %eax, dword ptr [%edx+BLOCKTYPE_STARTPC]
		jne psxDispatcherReg_recomp

		mov %eax, dword ptr [%edx]

#ifdef _DEBUG
        test %eax, %eax
        jnz psxDispatcherReg_CallFn2
        // throw an exception
        int 10
psxDispatcherReg_CallFn2:
#endif
		//and %eax, 0x0fffffff
		shl %eax, 4
		jmp %eax // fnptr

psxDispatcherReg_recomp:
		sub %esp, 8
		mov dword ptr [%esp+4], %edx
		mov dword ptr [%esp], %ecx
		call psxRecRecompile
		mov %edx, dword ptr [%esp+4]
		add %esp, 8
		
		mov %eax, dword ptr [%edx]
		//and %eax, 0x0fffffff
		shl %eax, 4
		jmp %eax // fnptr

#endif
