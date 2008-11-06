; Pcsx2 - Pc Ps2 Emulator
; Copyright (C) 2002-2008  Pcsx2 Team
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

;; x86-64 coroutine fucntions
extern g_pCurrentRoutine:ptr

.code

so_call proc public
        test dword ptr [rcx+88], 1
        jnz so_call_RestoreRegs
        mov [rcx+24], rbp
        mov [rcx+16], rbx
        mov [rcx+32], r12
        mov [rcx+40], r13
        mov [rcx+48], r14
        mov [rcx+56], r15
        mov [rcx+64], rsi
        mov [rcx+72], rdi
        mov dword ptr [rcx+88], 1
        jmp so_call_CallFn
so_call_RestoreRegs:
		;; have to load and save at the same time
        ;; rbp, rbx, r12
        mov rax, [rcx+24]
        mov r8, [rcx+16]
        mov rdx, [rcx+32]
        mov [rcx+24], rbp
        mov [rcx+16], rbx
        mov [rcx+32], r12
        mov rbp, rax
        mov rbx, r8
        mov r12, rdx
        ;; r13, r14, r15
        mov rax, [rcx+40]
        mov r8, [rcx+48]
        mov rdx, [rcx+56]
        mov [rcx+40], r13
        mov [rcx+48], r14
        mov [rcx+56], r15
        mov r13, rax
        mov r14, r8
        mov r15, rdx

        ;; rsi, rdi
        mov rax, [rcx+64]
        mov rdx, [rcx+72]
        mov [rcx+64], rsi
        mov [rcx+72], rdi
        mov rsi, rax
        mov rdi, rdx
        
so_call_CallFn:
        mov [g_pCurrentRoutine], rcx

		;; swap the stack
        mov rax, [rcx+8]
        mov [rcx+8], rsp
        mov rsp, rax
        mov rax, [rcx+0]
        mov rcx, [rcx+80]

        jmp rax

so_call endp

; so_resume
so_resume proc public
        ;; rbp, rbx, r12
        mov rcx, [g_pCurrentRoutine]
        mov rax, [rcx+24]
        mov r8, [rcx+16]
        mov rdx, [rcx+32]
        mov [rcx+24], rbp
        mov [rcx+16], rbx
        mov [rcx+32], r12
        mov rbp, rax
        mov rbx, r8
        mov r12, rdx
        ;; r13, r14, r15
        mov rax, [rcx+40]
        mov r8, [rcx+48]
        mov rdx, [rcx+56]
        mov [rcx+40], r13
        mov [rcx+48], r14
        mov [rcx+56], r15
        mov r13, rax
        mov r14, r8
        mov r15, rdx
        ;; rsi, rdi
        mov rax, [rcx+64]
        mov rdx, [rcx+72]
        mov [rcx+64], rsi
        mov [rcx+72], rdi
        mov rsi, rax
        mov rdi, rdx

		;; put the return address in pcalladdr
        mov rax, [rsp]
        mov [rcx], rax
        add rsp, 8 ;; remove the return address

		;; swap stack pointers
        mov rax, [rcx+8]
        mov [rcx+8], rsp
        mov rsp, rax

        ret

so_resume endp

so_exit proc public
        mov rcx, [g_pCurrentRoutine]
        mov rsp, [rcx+8]
        mov rbp, [rcx+24]
        mov rbx, [rcx+16]
        mov r12, [rcx+32]
        mov r13, [rcx+40]
        mov r14, [rcx+48]
        mov r15, [rcx+56]
        mov rsi, [rcx+64]
        mov rdi, [rcx+72]
        ret
so_exit endp

end