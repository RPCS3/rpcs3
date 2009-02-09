/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "coroutine.h"

struct coroutine {
	void* pcalladdr;
	void *pcurstack;

    uptr storeebx, storeesi, storeedi, storeebp;
	
    int restore; // if nonzero, restore the registers
    int alloc;
	//struct s_coroutine *caller;
	//struct s_coroutine *restarget;
	
};

#define CO_STK_ALIGN 256
#define CO_STK_COROSIZE ((sizeof(coroutine) + CO_STK_ALIGN - 1) & ~(CO_STK_ALIGN - 1))
#define CO_MIN_SIZE (4 * 1024)

coroutine* g_pCurrentRoutine;

coroutine_t so_create(void (*func)(void *), void *data, void *stack, int size)
{
    void* endstack;
    int alloc = 0; // r = CO_STK_COROSIZE;
	coroutine *co;

    if ((size &= ~(sizeof(long) - 1)) < CO_MIN_SIZE)
		return NULL;
	if (!stack) {
		size = (size + sizeof(coroutine) + CO_STK_ALIGN - 1) & ~(CO_STK_ALIGN - 1);
		stack = malloc(size);
		if (!stack)
			return NULL;
		alloc = size;
	}
    endstack = (char*)stack + size - 64;
	co = (coroutine*)stack;
	stack = (char *) stack + CO_STK_COROSIZE;
	*(void**)endstack = NULL;
	*(void**)((char*)endstack+sizeof(void*)) = data;
	co->alloc = alloc;
	co->pcalladdr = (void*)func;
    co->pcurstack = endstack;
    return co;
}

void so_delete(coroutine_t coro)
{
    coroutine *co = (coroutine *) coro;
    assert( co != NULL );
	if (co->alloc)
		free(co);
}

// see acoroutines.S and acoroutines.asm for other asm implementations
#if defined(_MSC_VER)

__declspec(naked) void so_call(coroutine_t coro)
{
    __asm {
        mov eax, dword ptr [esp+4]
        test dword ptr [eax+24], 1
        jnz RestoreRegs
        mov [eax+8], ebx
        mov [eax+12], esi
        mov [eax+16], edi
        mov [eax+20], ebp
        mov dword ptr [eax+24], 1
        jmp CallFn
RestoreRegs:
        // have to load and save at the same time
        mov ecx, [eax+8]
        mov edx, [eax+12]
        mov [eax+8], ebx
        mov [eax+12], esi
        mov ebx, ecx
        mov esi, edx
        mov ecx, [eax+16]
        mov edx, [eax+20]
        mov [eax+16], edi
        mov [eax+20], ebp
        mov edi, ecx
        mov ebp, edx
        
CallFn: 
        mov [g_pCurrentRoutine], eax
        mov ecx, esp
        mov esp, [eax+4]
        mov [eax+4], ecx

        jmp dword ptr [eax]
    }
}

__declspec(naked) void so_resume(void)
{
    __asm {
        mov eax, [g_pCurrentRoutine]
        mov ecx, [eax+8]
        mov edx, [eax+12]
        mov [eax+8], ebx
        mov [eax+12], esi
        mov ebx, ecx
        mov esi, edx
        mov ecx, [eax+16]
        mov edx, [eax+20]
        mov [eax+16], edi
        mov [eax+20], ebp
        mov edi, ecx
        mov ebp, edx

        // put the return address in pcalladdr
        mov ecx, [esp]
        mov [eax], ecx
        add esp, 4 // remove the return address

        // swap stack pointers
        mov ecx, [eax+4]
        mov [eax+4], esp
        mov esp, ecx
        ret
    }
}

__declspec(naked) void so_exit(void)
{
    __asm {
        mov eax, [g_pCurrentRoutine]
        mov esp, [eax+4]
        mov ebx, [eax+8]
        mov esi, [eax+12]
        mov edi, [eax+16]
        mov ebp, [eax+20]
        ret
    }
}
#endif
