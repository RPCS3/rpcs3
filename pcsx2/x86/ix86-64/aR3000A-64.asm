extern psxRegs:abs
extern psxRecLUT:abs
extern psxRecRecompile:near
extern EEsCycle:abs

.code

R3000AInterceptor proc public
        sub rsp, 48
        jmp rdx
R3000AInterceptor endp

R3000AExecute proc public

		;;while (EEsCycle > 0) {
		push rbx
		push rbp
		push rsi
		push rdi
		push r12
		push r13
		push r14
		push r15

Execute_CheckCycles:
		cmp dword ptr [EEsCycle], 0
		jle Execute_Exit

		;;if ( !pblock->pFnptr || (pblock->startpc&PSX_MEMMASK) != (psxRegs.pc&PSX_MEMMASK) )
		;;	psxRecRecompile(psxRegs.pc);

		mov eax, dword ptr [psxRegs + 0208h]
        mov ecx, eax
        mov r12d, eax
        shl rax, 32
        shr rax, 48
        and r12, 0fffch
        shl rax, 3
        add rax, [psxRecLUT]
        shl r12, 1
        add r12, [rax]

        mov r8d, [r12+4]
		mov r9d, ecx
		and r8d, 05fffffffh
		and r9d, 05fffffffh
		cmp r8d, r9d
		jne Execute_Recompile
        mov edx, [r12]
        and rdx, 0fffffffh ;; pFnptr
        jnz Execute_Function

Execute_Recompile:
		call psxRecRecompile
		mov edx, [r12]
		and rdx, 0fffffffh ;; pFnptr
        
Execute_Function:
		call R3000AInterceptor

		jmp Execute_CheckCycles

Execute_Exit:
		pop r15
		pop r14
		pop r13
		pop r12
		pop rdi
		pop rsi
		pop rbp
		pop rbx
		ret
		
R3000AExecute endp

psxDispatcher proc public

        mov [rsp+40], rdx

        mov eax, dword ptr [psxRegs + 0208h]
        mov ecx, eax
        mov r12d, eax
        shl rax, 32
        shr rax, 48
        and r12, 0fffch
        shl rax, 3
        add rax, [psxRecLUT]
        shl r12, 1
        add r12, [rax]


        mov eax, ecx
        mov edx, [r12+4]
        and eax, 5fffffffh
        and edx, 5fffffffh
        cmp eax, edx
        je psxDispatcher_CheckPtr

        call psxRecRecompile
psxDispatcher_CheckPtr:
		mov r12d, dword ptr [r12]

		and r12, 00fffffffh
        mov rdx, r12
        mov rcx, [rsp+40]
        sub rdx, rcx
        sub rdx, 4
        mov [rcx], edx

        jmp r12
psxDispatcher endp

psxDispatcherClear proc public

        mov dword ptr [psxRegs + 0208h], edx
        mov eax, edx



        mov r12d, edx
        shl rax, 32
        shr rax, 48
        and r12, 0fffch
        shl rax, 3
        add rax, [psxRecLUT]
        shl r12, 1
        add r12, [rax]


        mov ecx, edx
        mov eax, ecx
        mov edx, [r12+4]
        and eax, 5fffffffh
        and edx, 5fffffffh
        cmp eax, edx
        jne psxDispatcherClear_Recompile

        mov eax, dword ptr [r12]

        and rax, 00fffffffh
        jmp rax

psxDispatcherClear_Recompile:
        call psxRecRecompile
        mov eax, dword ptr [r12]


        and rax, 00fffffffh
        mov byte ptr [r15], 0e9h
        mov rdx, rax
        sub rdx, r15
        sub rdx, 5
        mov [r15+1], edx

        jmp rax
psxDispatcherClear endp


psxDispatcherReg proc public

        mov eax, dword ptr [psxRegs + 0208h]
        mov ecx, eax
        mov r12d, eax
        shl rax, 32
        shr rax, 48
        and r12, 0fffch
        shl rax, 3
        add rax, [psxRecLUT]
        shl r12, 1
        add r12, [rax]


		cmp ecx, dword ptr [r12+4]
		jne psxDispatcherReg_recomp

		mov r12d, dword ptr [r12]

		and r12, 00fffffffh
        jmp r12

psxDispatcherReg_recomp:
		call psxRecRecompile

        mov eax, dword ptr [r12]
        and rax, 00fffffffh
        jmp rax

psxDispatcherReg endp

end
