.intel_syntax

.extern g_pCurrentRoutine

#ifdef __x86_64__

#define FUNC_OFFSET 0
#define STACK_OFFSET 8
#define RBX_OFFSET 16
#define RBP_OFFSET 24
#define R12_OFFSET 32
#define R13_OFFSET 40
#define R14_OFFSET 48
#define R15_OFFSET 56
#define DATA_OFFSET 64
#define RESTORE_OFFSET 72

.globl so_call
so_call:
        test dword ptr [%rdi+RESTORE_OFFSET], 1
        jnz so_call_RestoreRegs
        mov [%rdi+RBP_OFFSET], %rbp
        mov [%rdi+RBX_OFFSET], %rbx
        mov [%rdi+R12_OFFSET], %r12
        mov [%rdi+R13_OFFSET], %r13
        mov [%rdi+R14_OFFSET], %r14
        mov [%rdi+R15_OFFSET], %r15        
        mov dword ptr [%rdi+RESTORE_OFFSET], 1
        jmp so_call_CallFn
so_call_RestoreRegs:
        // have to load and save at the same time
        mov %rax, [%rdi+RBP_OFFSET]
        mov %rcx, [%rdi+RBX_OFFSET]
        mov %rdx, [%rdi+R12_OFFSET]
        mov [%rdi+RBP_OFFSET], %rbp
        mov [%rdi+RBX_OFFSET], %rbx
        mov [%rdi+R12_OFFSET], %r12
        mov %rbp, %rax
        mov %rbx, %rcx
        mov %r12, %rdx
        mov %rax, [%rdi+R13_OFFSET]
        mov %rcx, [%rdi+R14_OFFSET]
        mov %rdx, [%rdi+R15_OFFSET]
        mov [%rdi+R13_OFFSET], %r13
        mov [%rdi+R14_OFFSET], %r14
        mov [%rdi+R15_OFFSET], %r15
        mov %r13, %rax
        mov %r14, %rcx
        mov %r15, %rdx        

so_call_CallFn:
        mov [g_pCurrentRoutine], %rdi

		// swap the stack
        mov %rax, [%rdi+STACK_OFFSET]
        mov [%rdi+STACK_OFFSET], %rsp
        mov %rsp, %rax
        mov %rax, [%rdi+FUNC_OFFSET]
        mov %rdi, [%rdi+DATA_OFFSET]

        jmp %rax

.globl so_resume
so_resume:
        mov %rdi, [g_pCurrentRoutine]
        mov %rax, [%rdi+RBP_OFFSET]
        mov %rcx, [%rdi+RBX_OFFSET]
        mov %rdx, [%rdi+R12_OFFSET]
        mov [%rdi+RBP_OFFSET], %rbp
        mov [%rdi+RBX_OFFSET], %rbx
        mov [%rdi+R12_OFFSET], %r12
        mov %rbp, %rax
        mov %rbx, %rcx
        mov %r12, %rdx
        mov %rax, [%rdi+R13_OFFSET]
        mov %rcx, [%rdi+R14_OFFSET]
        mov %rdx, [%rdi+R15_OFFSET]
        mov [%rdi+R13_OFFSET], %r13
        mov [%rdi+R14_OFFSET], %r14
        mov [%rdi+R15_OFFSET], %r15
        mov %r13, %rax
        mov %r14, %rcx
        mov %r15, %rdx

		// put the return address in pcalladdr
        mov %rsi, [%rsp]
        mov [%rdi], %rsi
        add %rsp, 8 // remove the return address

		// swap stack pointers
        mov %rax, [%rdi+STACK_OFFSET]
        mov [%rdi+STACK_OFFSET], %rsp
        mov %rsp, %rax
        
        ret

.globl so_exit
so_exit:
        mov %rdi, [g_pCurrentRoutine]
        mov %rsp, [%rdi+STACK_OFFSET]
        mov %rbp, [%rdi+RBP_OFFSET]
        mov %rbx, [%rdi+RBX_OFFSET]
        mov %r12, [%rdi+R12_OFFSET]
        mov %r13, [%rdi+R13_OFFSET]
        mov %r14, [%rdi+R14_OFFSET]
        mov %r15, [%rdi+R15_OFFSET]
        ret
#else

.globl so_call
so_call:
		mov %eax, dword ptr [%esp+4]
        test dword ptr [%eax+24], 1
        jnz RestoreRegs
        mov [%eax+8], %ebx
        mov [%eax+12], %esi
        mov [%eax+16], %edi
        mov [%eax+20], %ebp
        mov dword ptr [%eax+24], 1
        jmp CallFn
RestoreRegs:
        // have to load and save at the same time
        mov %ecx, [%eax+8]
        mov %edx, [%eax+12]
        mov [%eax+8], %ebx
        mov [%eax+12], %esi
        mov %ebx, %ecx
        mov %esi, %edx
        mov %ecx, [%eax+16]
        mov %edx, [%eax+20]
        mov [%eax+16], %edi
        mov [%eax+20], %ebp
        mov %edi, %ecx
        mov %ebp, %edx

CallFn:
        mov [g_pCurrentRoutine], %eax
        mov %ecx, %esp
        mov %esp, [%eax+4]
        mov [%eax+4], %ecx

        jmp dword ptr [%eax]

.globl so_resume
so_resume:
		mov %eax, [g_pCurrentRoutine]
        mov %ecx, [%eax+8]
        mov %edx, [%eax+12]
        mov [%eax+8], %ebx
        mov [%eax+12], %esi
        mov %ebx, %ecx
        mov %esi, %edx
        mov %ecx, [%eax+16]
        mov %edx, [%eax+20]
        mov [%eax+16], %edi
        mov [%eax+20], %ebp
        mov %edi, %ecx
        mov %ebp, %edx

        // put the return address in pcalladdr
        mov %ecx, [%esp]
        mov [%eax], %ecx
        add %esp, 4 // remove the return address

        // swap stack pointers
        mov %ecx, [%eax+4]
        mov [%eax+4], %esp
        mov %esp, %ecx
        ret

.globl so_exit
so_exit:
		mov %eax, [g_pCurrentRoutine]
        mov %esp, [%eax+4]
        mov %ebx, [%eax+8]
        mov %esi, [%eax+12]
        mov %edi, [%eax+16]
        mov %ebp, [%eax+20]
        ret

#endif
