
// iR5900.c assembly routines

.intel_syntax noprefix

//////////////////////////////////////////////////////////////////////////
// Note that iR3000A.S and iR5900.S asm code is now identical.  Only some
// function names and the following two defines should ever differ:

#define REGINFO cpuRegs
#define RECLUT recLUT
#define PCOFFSET 0x2a8 // this must always match what Pcsx2 displays at startup

//////////////////////////////////////////////////////////////////////////
// Preprocessor Mess!

.extern REGINFO
.extern RECLUT
.extern recRecompile
.extern recEventTest

//////////////////////////////////////////////////////////////////////////
// The address for all cleared blocks.  It recompiles the current pc and then
// dispatches to the recompiled block address.
.global JITCompile
JITCompile:

	mov esi, dword ptr [REGINFO + PCOFFSET]
	push esi
	call recRecompile
	add esp, 4
	mov ebx, esi
	shr esi, 16
	mov ecx, dword ptr [RECLUT+esi*4]
	jmp dword ptr [ecx+ebx]

.global JITCompileInBlock
JITCompileInBlock:
	jmp JITCompile

//////////////////////////////////////////////////////////////////////////
// called when jumping to variable pc address.

.globl DispatcherReg
DispatcherReg:

	mov eax, dword ptr [REGINFO + PCOFFSET]
	mov ebx, eax
	shr eax, 16
	mov ecx, dword ptr [RECLUT+eax*4]
	jmp dword ptr [ecx+ebx]

.globl DispatcherEvent
DispatcherEvent:

		call recEventTest
		jmp DispatcherReg
