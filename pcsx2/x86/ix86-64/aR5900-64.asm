extern cpuRegs:abs
extern recRecompile:near
extern recLUT:abs
extern lbase:abs
extern s_pCurBlock_ltime:abs

extern g_globalXMMData:abs
extern g_globalXMMSaved:abs
extern g_EEFreezeRegs:abs

.code

FreezeXMMRegs_ proc public
	;;assert( g_EEFreezeRegs );
	test ecx, ecx
	jz XMMRestore
	
		cmp byte ptr [g_globalXMMSaved], 0
		jne XMMSaveEnd
		
		mov byte ptr [g_globalXMMSaved], 1

		movaps xmmword ptr [g_globalXMMData + 000h], xmm0
		movaps xmmword ptr [g_globalXMMData + 010h], xmm1
		movaps xmmword ptr [g_globalXMMData + 020h], xmm2
		movaps xmmword ptr [g_globalXMMData + 030h], xmm3
		movaps xmmword ptr [g_globalXMMData + 040h], xmm4
		movaps xmmword ptr [g_globalXMMData + 050h], xmm5
		movaps xmmword ptr [g_globalXMMData + 060h], xmm6
		movaps xmmword ptr [g_globalXMMData + 070h], xmm7
		
XMMSaveEnd:
        ret
	
XMMRestore:

		cmp byte ptr [g_globalXMMSaved], 0
		je XMMSaveEnd
		
		mov byte ptr [g_globalXMMSaved], 0
		
		;; TODO: really need to backup all regs?
		movaps xmm0, xmmword ptr [g_globalXMMData + 000h]
		movaps xmm1, xmmword ptr [g_globalXMMData + 010h]
		movaps xmm2, xmmword ptr [g_globalXMMData + 020h]
		movaps xmm3, xmmword ptr [g_globalXMMData + 030h]
		movaps xmm4, xmmword ptr [g_globalXMMData + 040h]
		movaps xmm5, xmmword ptr [g_globalXMMData + 050h]
		movaps xmm6, xmmword ptr [g_globalXMMData + 060h]
		movaps xmm7, xmmword ptr [g_globalXMMData + 070h]

XMMRestoreEnd:
		ret

FreezeXMMRegs_ endp

R5900Interceptor proc public
        sub rsp, 48
        jmp rdx
R5900Interceptor endp

R5900Execute proc public

		push rbx
		push rbp
		push rsi
		push rdi
		push r12
		push r13
		push r14
		push r15

		;;BASEBLOCK* pblock = PC_GETBLOCK(cpuRegs.pc);
		mov eax, dword ptr [cpuRegs + 02a8h]
		mov ecx, eax
		mov r14d, eax
		shl rax, 32
		shr rax, 48
		and r14, 0fffch
		shl rax, 3
		add rax, [recLUT]
		shl r14, 1
		add r14, [rax]

		;; g_EEFreezeRegs = 1;
		mov dword ptr [g_EEFreezeRegs], 1
		 
		cmp ecx, [r14+4]
		jne Execute_Recompile
        mov edx, [r14]
        and rdx, 0fffffffh ;; pFnptr
        jnz Execute_Function
        
Execute_Recompile:
		call recRecompile
		mov edx, [r14]
        and rdx, 0fffffffh ;; pFnptr
        
Execute_Function:
		call R5900Interceptor

		;; g_EEFreezeRegs = 0;
		mov dword ptr [g_EEFreezeRegs], 0

		pop r15
		pop r14
		pop r13
		pop r12
		pop rdi
		pop rsi
		pop rbp
		pop rbx
		
		ret

R5900Execute endp

Dispatcher proc public

        mov [rsp+40], rdx

        mov eax, dword ptr [cpuRegs + 02a8h]
        mov ecx, eax
        mov r14d, eax
        shl rax, 32
        shr rax, 48
        and r14, 0fffch
        shl rax, 3
        add rax, [recLUT]
        shl r14, 1
        add r14, [rax]



        cmp ecx, dword ptr [r14+4]
        je Dispatcher_CheckPtr


        call recRecompile
Dispatcher_CheckPtr:
        mov r14d, dword ptr [r14]

        and r14, 0fffffffh
        mov rdx, r14
        mov rcx, [rsp+40]
        sub rdx, rcx
        sub rdx, 4
        mov [rcx], edx

        jmp r14
Dispatcher endp

DispatcherClear proc public

        mov dword ptr [cpuRegs + 02a8h], edx
        mov eax, edx



        mov r14d, edx
        shl rax, 32
        shr rax, 48
        and r14, 0fffch
        shl rax, 3
        add rax, [recLUT]
        shl r14, 1
        add r14, [rax]

        cmp edx, dword ptr [r14 + 4]
        jne DispatcherClear_Recompile

        mov eax, dword ptr [r14]

        and rax, 0fffffffh
        jmp rax

DispatcherClear_Recompile:
        mov ecx, edx
        call recRecompile
        mov eax, dword ptr [r14]


        and rax, 0fffffffh
        mov byte ptr [r15], 0e9h
        mov rdx, rax
        sub rdx, r15
        sub rdx, 5
        mov [r15+1], edx

        jmp rax
DispatcherClear endp



DispatcherReg proc public

        mov eax, dword ptr [cpuRegs + 02a8h]
        mov ecx, eax
        mov r14d, eax
        shl rax, 32
        shr rax, 48
        and r14, 0fffch
        shl rax, 3
        add rax, [recLUT]
        shl r14, 1
        add r14, [rax]



        cmp ecx, dword ptr [r14+4]
        jne DispatcherReg_recomp

        mov r14d, dword ptr [r14]

        and r14, 0fffffffh
        jmp r14

DispatcherReg_recomp:
        call recRecompile

        mov eax, dword ptr [r14]
        and rax, 0fffffffh
        jmp rax
DispatcherReg endp

_StartPerfCounter proc public
	push rax
	push rbx
	push rcx

	rdtsc
	mov dword ptr [lbase], eax
	mov dword ptr [lbase + 4], edx

	pop rcx
	pop rbx
	pop rax
	ret
_StartPerfCounter endp

_StopPerfCounter proc public
	push rax
	push rbx
	push rcx

	rdtsc

	sub eax, dword ptr [lbase]
	sbb edx, dword ptr [lbase + 4]
	mov ecx, [s_pCurBlock_ltime]
	add eax, dword ptr [ecx]
	adc edx, dword ptr [ecx + 4]
	mov dword ptr [ecx], eax
	mov dword ptr [ecx + 4], edx
	pop rcx
	pop rbx
	pop rax
	ret
_StopPerfCounter endp

end
