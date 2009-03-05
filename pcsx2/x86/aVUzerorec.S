// iVUzerorec.cpp assembly routines
// zerofrog(@gmail.com)
.intel_syntax noprefix
        
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
        mov eax, [esp]
        mov dword ptr s_TotalVUCycles, 0
        add esp, 4
        mov dword ptr [s_callstack], eax
        call SuperVUGetProgram
        mov s_vu1ebp, ebp
        mov s_vu1esi, esi
        mov s_vuedi, edi
        mov s_vuebx, ebx
#ifdef _DEBUG
        mov s_vu1esp, esp
#endif
        
        ldmxcsr g_sseVUMXCSR
        mov dword ptr s_writeQ, 0xffffffff
        mov dword ptr s_writeP, 0xffffffff
        jmp eax

.globl SuperVUEndProgram
SuperVUEndProgram:
	// restore cpu state
	ldmxcsr g_sseMXCSR
		mov ebp, s_vu1ebp
		mov esi, s_vu1esi
		mov edi, s_vuedi
		mov ebx, s_vuebx
                
#ifdef _DEBUG
        sub s_vu1esp, esp
#endif
        
		call SuperVUCleanupProgram
		jmp [s_callstack] // so returns correctly


.globl svudispfn
svudispfn:
        mov [g_curdebugvu], eax
		mov s_saveecx, ecx
		mov s_saveedx, edx
		mov s_saveebx, ebx
		mov s_saveesi, esi
		mov s_saveedi, edi
		mov s_saveebp, ebp

        call svudispfntemp
        
        mov ecx, s_saveecx
		mov edx, s_saveedx
		mov ebx, s_saveebx
		mov esi, s_saveesi
		mov edi, s_saveedi
		mov ebp, s_saveebp
        ret