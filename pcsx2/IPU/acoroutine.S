.intel_syntax

.extern g_pCurrentRoutine

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

