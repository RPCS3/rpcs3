// iR3000A.c assembly routines
// zerofrog(@gmail.com)
.intel_syntax
        
.extern svudispfntemp
.extern s_TotalVUCycles
.extern s_callstack
.extern s_vu1esp
.extern s_writeQ
.extern s_writeP
.extern g_curdebugvu
.extern SuperVUGetProgram
.extern SuperVUCleanupProgram
.extern g_sseVUMXCSR
.extern g_sseMXCSR
        
// SuperVUExecuteProgram(u32 startpc, int vuindex)
.globl SuperVUExecuteProgram
SuperVUExecuteProgram:
#ifdef __x86_64__
        mov %rax, [%rsp]
        mov dword ptr [s_TotalVUCycles], 0
        add %rsp, 8
        mov [s_callstack], %rax
        call SuperVUGetProgram
        push %rbp
        push %rbx
        push %r12
        push %r13
        push %r14
        push %r15
        // function arguments
        push %rdi
        push %rsi
        
#ifdef _DEBUG
        mov s_vu1esp, %rsp
#endif
        
        ldmxcsr g_sseVUMXCSR
        mov dword ptr [s_writeQ], 0xffffffff
        mov dword ptr [s_writeP], 0xffffffff
        jmp %rax
#else
        mov %eax, [%esp]
        mov dword ptr s_TotalVUCycles, 0
        add %esp, 4
        mov dword ptr [s_callstack], %eax
        call SuperVUGetProgram
        mov s_vu1ebp, %ebp
        mov s_vu1esi, %esi
        mov s_vuedi, %edi
        mov s_vuebx, %ebx
#ifdef _DEBUG
        mov s_vu1esp, %esp
#endif
        
        ldmxcsr g_sseVUMXCSR
        mov dword ptr s_writeQ, 0xffffffff
        mov dword ptr s_writeP, 0xffffffff
        jmp %eax
#endif // __x86_64__


.globl SuperVUEndProgram
SuperVUEndProgram:
		// restore cpu state
		ldmxcsr g_sseMXCSR
        
#ifdef __x86_64__
#ifdef _DEBUG
        sub s_vu1esp, %rsp
#endif
        
        // function arguments for SuperVUCleanupProgram
        pop %rsi
        pop %rdi
        
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %rbx
        pop %rbp
#else
		mov %ebp, s_vu1ebp
		mov %esi, s_vu1esi
		mov %edi, s_vuedi
		mov %ebx, s_vuebx
                
#ifdef _DEBUG
        sub s_vu1esp, %esp
#endif
#endif
        
		call SuperVUCleanupProgram
		jmp [s_callstack] // so returns correctly


.globl svudispfn
svudispfn:
#ifdef __x86_64__
        mov [g_curdebugvu], %rax
        push %rax
		push %rcx
        push %rdx
        push %rbp
        push %rsi
        push %rdi
        push %rbx
        push %r8
        push %r9
        push %r10
        push %r11
        push %r12
        push %r13
        push %r14
        push %r15

        call svudispfntemp
        
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %r11
        pop %r10
        pop %r9
        pop %r8
        pop %rbx
        pop %rdi
        pop %rsi
        pop %rbp
        pop %rdx
        pop %rcx
        pop %rax
#else
        mov [g_curdebugvu], %eax
		mov s_saveecx, %ecx
		mov s_saveedx, %edx
		mov s_saveebx, %ebx
		mov s_saveesi, %esi
		mov s_saveedi, %edi
		mov s_saveebp, %ebp

        call svudispfntemp
        
        mov %ecx, s_saveecx
		mov %edx, s_saveedx
		mov %ebx, s_saveebx
		mov %esi, s_saveesi
		mov %edi, s_saveedi
		mov %ebp, s_saveebp
#endif
        ret