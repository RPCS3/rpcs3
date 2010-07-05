// iVUzerorec.cpp assembly routines
// zerofrog(@gmail.com)
.intel_syntax noprefix
        
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
        mov eax, [esp]
        mov dword ptr s_TotalVUCycles, 0
        add esp, 4
        mov dword ptr [s_callstack], eax
        call SuperVUGetProgram
        mov s_vu1esi, esi
        mov s_vuedi, edi
        mov s_vuebx, ebx

        mov s_vu1esp, esp
		and esp, -16		// align stack for GCC compilance

        ldmxcsr g_sseVUMXCSR
        mov dword ptr s_writeQ, 0xffffffff
        mov dword ptr s_writeP, 0xffffffff
        jmp eax

.globl SuperVUEndProgram
SuperVUEndProgram:
	// restore cpu state
	ldmxcsr g_sseMXCSR
		mov esi, s_vu1esi
		mov edi, s_vuedi
		mov ebx, s_vuebx
                
        mov esp, s_vu1esp

		call SuperVUCleanupProgram
		jmp [s_callstack] // so returns correctly

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
