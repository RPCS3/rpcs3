
// iR5900.c assembly routines

.intel_syntax noprefix

//////////////////////////////////////////////////////////////////////////
// Note that iR3000A.S and iR5900.S asm code is now identical.  Only some
// function names and the following two defines should ever differ:

#define REGINFO cpuRegs
#define PCOFFSET 0x2a8 // this must always match what Pcsx2 displays at startup

//////////////////////////////////////////////////////////////////////////
// Preprocessor Mess!

.extern REGINFO
.extern recRecompile

#define BLOCKTYPE_STARTPC	4		// startpc offset
#define BLOCKTYPE_DELAYSLOT	1		// if bit set, delay slot

#define BASEBLOCK_SIZE 2 // in dwords

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
	mov ecx, dword ptr [recLUT+esi*4]
	jmp dword ptr [ecx+ebx*2]


//////////////////////////////////////////////////////////////////////////
// Recompiles the next block, and links the old block directly to it.
// This is a on-shot execution for any block which uses it.  Once the block
// has been statically linked to the new block, this function will be bypassed
//
// edx - jump address to modify
.globl Dispatcher
Dispatcher:

	# calc PC_GETBLOCK
	# ((BASEBLOCK*)(reclut[((u32)(x)) >> 16] + (x)*(sizeof(BASEBLOCK)/4)))

	mov eax, dword ptr [REGINFO + PCOFFSET]
	mov ebx, eax
	shr eax, 16
	mov ecx, dword ptr [recLUT+eax*4]
	mov eax, dword ptr [ecx+ebx*2]

	cmp eax, offset JITCompile
	je Dispatch_notcompiled
	lea ebx, [eax-4]
	sub ebx, edx
	mov dword ptr [edx], ebx
	jmp eax

	.align 16
Dispatch_notcompiled:
	mov esi, edx
	lea edi, [ecx+ebx*2]
	push ebx
	call recRecompile
	add esp, 4

	mov eax, dword ptr [edi]
	lea ebx, [eax-4]
	sub ebx, esi
	mov dword ptr [esi], ebx
	jmp eax


//////////////////////////////////////////////////////////////////////////
// edx -  baseblock->startpc
// stack - x86Ptr
.globl DispatcherClear
DispatcherClear:
	mov [REGINFO + PCOFFSET], edx

	# calc PC_GETBLOCK
	# ((BASEBLOCK*)(reclut[((u32)(x)) >> 16] + (x)*(sizeof(BASEBLOCK)/4)))

	mov ebx, edx
	shr edx, 16
	mov ecx, dword ptr [recLUT+edx*4]
	mov eax, dword ptr [ecx+ebx*2]

	cmp eax, offset JITCompile
	je Clear_notcompiled
	add esp, 4
	jmp eax

	.align 16
Clear_notcompiled:
	lea edi, [ecx+ebx*2]
	push ebx
	call recRecompile
	add esp, 4
	mov eax, dword ptr [edi]

	pop ecx
	mov byte ptr [ecx], 0xe9 // jmp32
	lea ebx, [eax-5]
	sub ebx, ecx
	mov dword ptr [ecx+1], ebx

	jmp eax


//////////////////////////////////////////////////////////////////////////
// called when jumping to variable pc address.

.globl DispatcherReg
DispatcherReg:

	mov eax, dword ptr [REGINFO + PCOFFSET]
	mov ebx, eax
	shr eax, 16
	mov ecx, dword ptr [recLUT+eax*4]
	jmp dword ptr [ecx+ebx*2]
