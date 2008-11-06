;; iR3000A.c assembly routines
;; zerofrog(@gmail.com)
extern svudispfntemp:near
extern QueryPerformanceCounter:near
extern s_TotalVUCycles:abs
extern s_callstack:ptr
extern s_vu1esp:abs
extern s_writeQ:abs
extern s_writeP:abs
extern g_curdebugvu:abs
extern SuperVUGetProgram:near
extern SuperVUCleanupProgram:near
extern g_sseVUMXCSR:abs
extern g_sseMXCSR:abs

extern svubase:abs

.code
   
;; SuperVUExecuteProgram(u32 startpc, int vuindex)
SuperVUExecuteProgram proc public

		;; uncomment only if SUPERVU_COUNT is defined
		;; {
		;push rcx
		;push rdx
		;mov rcx, svubase
		;sub rsp,32
		;call QueryPerformanceCounter
		;add rsp,32
		;pop rdx
		;pop rcx
		;; }
		
        mov rax, [rsp]
        mov dword ptr [s_TotalVUCycles], 0

        sub rsp, 48-8+80
        mov [rsp+48], rcx
        mov [rsp+56], rdx
        
        mov [s_callstack], rax
        
        call SuperVUGetProgram

        mov [rsp+64], rbp
        mov [rsp+72], rsi
        mov [rsp+80], rdi
        mov [rsp+88], rbx
        mov [rsp+96], r12
        mov [rsp+104], r13
        mov [rsp+112], r14
        mov [rsp+120], r15

        ;; _DEBUG
        ;;mov [s_vu1esp], rsp
        
        ldmxcsr [g_sseVUMXCSR]
        mov dword ptr [s_writeQ], 0ffffffffh
        mov dword ptr [s_writeP], 0ffffffffh
        jmp rax
SuperVUExecuteProgram endp

SuperVUEndProgram proc public
		;; restore cpu state
		ldmxcsr [g_sseMXCSR]
        
        mov rcx, [rsp+48]
        mov rdx, [rsp+56]
        mov rbp, [rsp+64]
        mov rsi, [rsp+72]
        mov rdi, [rsp+80]
        mov rbx, [rsp+88]
        mov r12, [rsp+96]
        mov r13, [rsp+104]
        mov r14, [rsp+112]
        mov r15, [rsp+120]
                
        ;; _DEBUG
        ;;sub [s_vu1esp], rsp

        add rsp, 128-32
		call SuperVUCleanupProgram
        add rsp, 32
		jmp [s_callstack] ;; so returns correctly
SuperVUEndProgram endp

svudispfn proc public
        mov [g_curdebugvu], rax
        push rcx
        push rdx
        push rbp
        push rsi
        push rdi
        push rbx
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
        
        sub rsp, 32        
		call svudispfntemp
        add rsp, 32
        
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rbx
        pop rdi
        pop rsi
        pop rbp
        pop rdx
        pop rcx
        ret
svudispfn endp

end
