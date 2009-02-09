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

// Virtual memory model for Pcsx2.
// This module is left in primarily as a reference for the implementation of constant
// propagation.

#include "PrecompiledHeader.h"
#include "Common.h"
#include "iR5900.h"

#include "PsxCommon.h"
#include "VUmicro.h"
#include "GS.h"
#include "vtlb.h"
#include "IPU/IPU.h"

#ifdef PCSX2_VIRTUAL_MEM
#include "iR3000A.h"		// VM handles both Iop and EE memory from here. >_<
#include "Counters.h"
#endif

#pragma warning(disable:4799) // No EMMS at end of function

#ifdef ENABLECACHE
#include "Cache.h"
#endif

#ifdef __LINUX__
#include <sys/mman.h>
#endif

/////////////////////////////
// VIRTUAL MEM START 
/////////////////////////////
#ifdef PCSX2_VIRTUAL_MEM

class vm_alloc_failed_exception : public std::runtime_error
{
public:
	void* requested_addr;
	int requested_size;
	void* returned_addr;

	explicit vm_alloc_failed_exception( void* reqadr, uint reqsize, void* retadr ) :
	std::runtime_error( "virtual memory allocation failure." )
		, requested_addr( reqadr )
		, requested_size( reqsize )
		, returned_addr( retadr )
	{}
};

PSMEMORYBLOCK s_psM = {0}, s_psHw = {0}, s_psS = {0}, s_psxM = {0}, s_psVuMem = {0};

static void PHYSICAL_ALLOC( void* ptr, uint size, PSMEMORYBLOCK& block)
{
	if(SysPhysicalAlloc(size, &block) == -1 )
		throw vm_alloc_failed_exception( ptr, size, NULL );
	if(SysVirtualPhyAlloc(ptr, size, &block) == -1)
		throw vm_alloc_failed_exception( ptr, size, NULL );
}

static void PHYSICAL_FREE( void* ptr, uint size, PSMEMORYBLOCK& block)
{
	SysVirtualFree(ptr, size);
	SysPhysicalFree(&block);
}


#ifdef _WIN32 // windows implementation of vm

static PSMEMORYMAP initMemoryMap(uptr* aPFNs, uptr* aVFNs)
{
	PSMEMORYMAP m;
	m.aPFNs = aPFNs;
	m.aVFNs = aVFNs;
	return m;
}

// only do vm hack for release
#ifndef PCSX2_DEVBUILD
#define VM_HACK
#endif

// virtual memory blocks
PSMEMORYMAP *memLUT = NULL;

static void VIRTUAL_ALLOC( void* base, uint size, uint Protection)
{
	LPVOID lpMemReserved = VirtualAlloc( base, size, MEM_RESERVE|MEM_COMMIT, Protection );
	if( base != lpMemReserved )
		throw vm_alloc_failed_exception( base, size, lpMemReserved );
}

static void ReserveExtraMem( void* base, uint size )
{
	void* pExtraMem = VirtualAlloc(base, size, MEM_RESERVE|MEM_PHYSICAL, PAGE_READWRITE);
	if( pExtraMem != base )
		throw vm_alloc_failed_exception( base, size, pExtraMem);
}

void memAlloc()
{
	LPVOID pExtraMem = NULL;	

	// release the previous reserved mem
	VirtualFree(PS2MEM_BASE, 0, MEM_RELEASE);

	try
	{
		// allocate all virtual memory
		PHYSICAL_ALLOC(PS2MEM_BASE, Ps2MemSize::Base, s_psM);
		VIRTUAL_ALLOC(PS2MEM_ROM, Ps2MemSize::Rom, PAGE_READONLY);
		VIRTUAL_ALLOC(PS2MEM_ROM1, Ps2MemSize::Rom1, PAGE_READONLY);
		VIRTUAL_ALLOC(PS2MEM_ROM2, Ps2MemSize::Rom2, PAGE_READONLY);
		VIRTUAL_ALLOC(PS2MEM_EROM, Ps2MemSize::ERom, PAGE_READONLY);
		PHYSICAL_ALLOC(PS2MEM_SCRATCH, Ps2MemSize::Scratch, s_psS);
		PHYSICAL_ALLOC(PS2MEM_HW, Ps2MemSize::Hardware, s_psHw);
		PHYSICAL_ALLOC(PS2MEM_PSX, Ps2MemSize::IopRam, s_psxM);
		PHYSICAL_ALLOC(PS2MEM_VU0MICRO, 0x00010000, s_psVuMem);

		VIRTUAL_ALLOC(PS2MEM_PSXHW, Ps2MemSize::IopHardware, PAGE_READWRITE);
		//VIRTUAL_ALLOC(PS2MEM_PSXHW2, 0x00010000, PAGE_READWRITE);
		VIRTUAL_ALLOC(PS2MEM_PSXHW4, 0x00010000, PAGE_NOACCESS);
		VIRTUAL_ALLOC(PS2MEM_GS, 0x00002000, PAGE_READWRITE);
		VIRTUAL_ALLOC(PS2MEM_DEV9, 0x00010000, PAGE_NOACCESS);
		VIRTUAL_ALLOC(PS2MEM_SPU2, 0x00010000, PAGE_NOACCESS);
		VIRTUAL_ALLOC(PS2MEM_SPU2_, 0x00010000, PAGE_NOACCESS);

		VIRTUAL_ALLOC(PS2MEM_B80, 0x00010000, PAGE_READWRITE);
		VIRTUAL_ALLOC(PS2MEM_BA0, 0x00010000, PAGE_READWRITE);

		// reserve the left over 224Mb, don't map
		ReserveExtraMem( PS2MEM_BASE+Ps2MemSize::Base, 0x0e000000 );

		// reserve left over psx mem
		ReserveExtraMem( PS2MEM_PSX+Ps2MemSize::IopRam, 0x00600000 );

		// reserve gs mem
		ReserveExtraMem( PS2MEM_BASE+0x20000000, 0x10000000 );

		// special addrs mmap
		VIRTUAL_ALLOC(PS2MEM_BASE+0x5fff0000, 0x10000, PAGE_READWRITE);

		// alloc virtual mappings
		if( memLUT == NULL )
			memLUT = (PSMEMORYMAP*)_aligned_malloc(0x100000 * sizeof(PSMEMORYMAP), 16);
		if( memLUT == NULL )
			throw Exception::OutOfMemory( "memAlloc VM > failed to allocated memory for LUT." );
	}
	catch( vm_alloc_failed_exception& ex )
	{
		Console::Error( "Virtual Memory Error > Cannot reserve %dk memory block at 0x%8.8x", params
			ex.requested_size / 1024, ex.requested_addr );

		Console::Error( "\tError code: %d  \tReturned address: 0x%8.8x", params
			GetLastError(), ex.returned_addr);

		memShutdown();
	}
	catch( std::exception& )
	{
		memShutdown();
	}
}

void memShutdown()
{
	// Free up the "extra mem" reservations
	VirtualFree(PS2MEM_BASE+Ps2MemSize::Base, 0, MEM_RELEASE);
	VirtualFree(PS2MEM_PSX+Ps2MemSize::IopRam, 0, MEM_RELEASE);
	VirtualFree(PS2MEM_BASE+0x20000000, 0, MEM_RELEASE);		// GS reservation

	PHYSICAL_FREE(PS2MEM_BASE, Ps2MemSize::Base, s_psM);
	SysMunmap(PS2MEM_ROM, Ps2MemSize::Rom);
	SysMunmap(PS2MEM_ROM1, Ps2MemSize::Rom1);
	SysMunmap(PS2MEM_ROM2, Ps2MemSize::Rom2);
	SysMunmap(PS2MEM_EROM, Ps2MemSize::ERom);
	PHYSICAL_FREE(PS2MEM_SCRATCH, Ps2MemSize::Scratch, s_psS);
	PHYSICAL_FREE(PS2MEM_HW, Ps2MemSize::Hardware, s_psHw);
	PHYSICAL_FREE(PS2MEM_PSX, Ps2MemSize::IopRam, s_psxM);
	PHYSICAL_FREE(PS2MEM_VU0MICRO, 0x00010000, s_psVuMem);

	SysMunmap(PS2MEM_VU0MICRO, 0x00010000); // allocate for all VUs

	SysMunmap(PS2MEM_PSXHW, Ps2MemSize::IopHardware);
	//SysMunmap(PS2MEM_PSXHW2, 0x00010000);
	SysMunmap(PS2MEM_PSXHW4, 0x00010000);
	SysMunmap(PS2MEM_GS, 0x00002000);
	SysMunmap(PS2MEM_DEV9, 0x00010000);
	SysMunmap(PS2MEM_SPU2, 0x00010000);
	SysMunmap(PS2MEM_SPU2_, 0x00010000);

	SysMunmap(PS2MEM_B80, 0x00010000);
	SysMunmap(PS2MEM_BA0, 0x00010000);

	// Special Addrs.. ?
	SysMunmap(PS2MEM_BASE+0x5fff0000, 0x10000);

	VirtualFree(PS2MEM_VU0MICRO, 0, MEM_RELEASE);

	safe_aligned_free( memLUT );

	// reserve mem
	VirtualAlloc(PS2MEM_BASE, 0x40000000, MEM_RESERVE, PAGE_NOACCESS);
}

//NOTE: A lot of the code reading depends on the registers being less than 8
// MOV8 88/8A
// MOV16 6689
// MOV32 89/8B
// SSEMtoR64 120f
// SSERtoM64 130f
// SSEMtoR128 280f
// SSERtoM128 290f

#define SKIP_WRITE() { \
	switch(code&0xff) { \
		case 0x88: \
		if( !(code&0x8000) ) goto DefaultHandler; \
		ContextRecord->Eip += 6; \
		break; \
		case 0x66: \
		assert( code&0x800000 ); \
		assert( (code&0xffff) == 0x8966 ); \
		ContextRecord->Eip += 7; \
		break; \
		case 0x89: \
		assert( code&0x8000 ); \
		ContextRecord->Eip += 6; \
		break; \
		case 0x0f: /* 130f, 230f*/ \
		assert( (code&0xffff) == 0x290f || (code&0xffff) == 0x130f ); \
		assert( code&0x800000 ); \
		ContextRecord->Eip += 7; \
		break; \
		default: \
		goto DefaultHandler; \
} \
} \

#define SKIP_READ() { \
	switch(code&0xff) { \
		case 0x8A: \
		if( !(code&0x8000) ) goto DefaultHandler; \
		ContextRecord->Eip += 6; \
		rd = (code>>(8+3))&7; \
		break; \
		case 0x66: \
		if( (code&0x07000000) == 0x05000000 ) ContextRecord->Eip += 8; /* 8 for mem reads*/ \
			else ContextRecord->Eip += 4 + ((code&0x1f000000) == 0x0c000000) + !!(code&0x40000000); \
			rd = (code>>(24+3))&7; \
			break; \
		case 0x8B: \
		if( !(code&0x8000) ) goto DefaultHandler; \
		ContextRecord->Eip += 6; \
		rd = (code>>(8+3))&7; \
		break; \
		case 0x0f: { \
		assert( (code&0xffff)==0x120f || (code&0xffff)==0x280f || (code&0xffff) == 0xb60f || (code&0xffff) == 0xb70f ); \
		if( !(code&0x800000) ) goto DefaultHandler; \
		ContextRecord->Eip += 7; \
		rd = (code>>(16+3))&7; \
		break; } \
		default: \
		goto DefaultHandler; \
} \
} \

int SysPageFaultExceptionFilter(EXCEPTION_POINTERS* eps)
{
	struct _EXCEPTION_RECORD* ExceptionRecord = eps->ExceptionRecord;
	struct _CONTEXT* ContextRecord = eps->ContextRecord;

	u32 addr;

	C_ASSERT(sizeof(ContextRecord->Eax) == 4);

	// If the exception is not a page fault, exit.
	if (ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// get bad virtual address
	addr = (u32)ExceptionRecord->ExceptionInformation[1];

	if( (unsigned)(addr-(u32)PS2MEM_BASE) < 0x60000000) {
		PSMEMORYMAP* pmap;

		pmap = &memLUT[(addr-(u32)PS2MEM_BASE)>>12];

		if( !pmap->aPFNs ) {
			// NOTE: this is a hack because the address is truncated and there's no way
			// to tell what it's upper bits are (due to OS limitations).
			pmap += 0x80000;
			if( !pmap->aPFNs ) {
				pmap += 0x20000;
				if( !pmap->aPFNs ) goto OtherException;
			}
			//else addr += 0x20000000;
		}

		{
			//LPVOID pnewaddr; not used
			uptr curvaddr = pmap->aVFNs[0];

			if( curvaddr ) {
				// delete the current mapping
				SysMapUserPhysicalPages((void*)curvaddr, 1, NULL, 0);
			}

			assert( pmap->aPFNs[0] != 0 );

			pmap->aVFNs[0] = curvaddr = addr&~0xfff;
			if( SysMapUserPhysicalPages((void*)curvaddr, 1, pmap->aPFNs, 0) )
				return EXCEPTION_CONTINUE_EXECUTION;

			// try allocing the virtual mem
			//pnewaddr = <- not used
			/* use here the size of allocation granularity and force rounding down,
			because in reserve mode the address is rounded up/down to the nearest
			multiple of this granularity; if you did it not this way, in some cases
			the same address would be used twice, so the api fails */
			VirtualAlloc((void*)(curvaddr&~0xffff), 0x10000, MEM_RESERVE|MEM_PHYSICAL, PAGE_READWRITE);

			if( SysMapUserPhysicalPages((void*)curvaddr, 1, pmap->aPFNs, 0) )
				return EXCEPTION_CONTINUE_EXECUTION;

			Console::Error("Virtual Memory Error > page 0x%x cannot be found %d (p:%x,v:%x)", params
				addr-(u32)PS2MEM_BASE, GetLastError(), pmap->aPFNs[0], curvaddr);
		}
	}
	// check if vumem
	else if( (addr&0xffff4000) == 0x11000000 ) {
		// vu0mem
		SysMapUserPhysicalPages((void*)s_psVuMem.aVFNs[1], 1, NULL, 0);

		s_psVuMem.aVFNs[1] = addr&~0xfff;
		SysMapUserPhysicalPages((void*)addr, 1, s_psVuMem.aPFNs, 1);

		//SysPrintf("Exception: vumem\n");
		return EXCEPTION_CONTINUE_EXECUTION;
	}
OtherException:

#ifdef VM_HACK
	{
		u32 code = *(u32*)ExceptionRecord->ExceptionAddress;
		u32 rd = 0;

		if( ExceptionRecord->ExceptionInformation[0] ) {
			//SKIP_WRITE();
			// shouldn't be writing
		}
		else {
			SysPrintf("vmhack ");
			SKIP_READ();
			//((u32*)&ContextRecord->Eax)[rd] = 0;
			return EXCEPTION_CONTINUE_EXECUTION; // TODO: verify this!!!
		}
	}
DefaultHandler:
#endif

	return EXCEPTION_CONTINUE_SEARCH;
}

#else // linux implementation

#define VIRTUAL_ALLOC(base, size, Protection) { \
	void* lpMemReserved = mmap( base, size, Protection, MAP_PRIVATE|MAP_ANONYMOUS ); \
	if( lpMemReserved == NULL || base != lpMemReserved ) \
{ \
	SysPrintf("Cannot reserve memory at 0x%8.8x(%x).\n", base, lpMemReserved); \
	perror("err"); \
	goto eCleanupAndExit; \
} \
} \

#define VIRTUAL_FREE(ptr, size) munmap(ptr, size)

uptr *memLUT = NULL;

void memAlloc()
{
	int i;
	LPVOID pExtraMem = NULL;	

	// release the previous reserved mem
	munmap(PS2MEM_BASE, 0x40000000);

	// allocate all virtual memory
	PHYSICAL_ALLOC(PS2MEM_BASE, Ps2MemSize::Base, s_psM);
	VIRTUAL_ALLOC(PS2MEM_ROM, Ps2MemSize::Rom, PROT_READ);
	VIRTUAL_ALLOC(PS2MEM_ROM1, Ps2MemSize::Rom1, PROT_READ);
	VIRTUAL_ALLOC(PS2MEM_ROM2, Ps2MemSize::Rom2, PROT_READ);
	VIRTUAL_ALLOC(PS2MEM_EROM, Ps2MemSize::ERom, PROT_READ);
	PHYSICAL_ALLOC(PS2MEM_SCRATCH, 0x00010000, s_psS);
	PHYSICAL_ALLOC(PS2MEM_HW, Ps2MemSize::Hardware, s_psHw);
	PHYSICAL_ALLOC(PS2MEM_PSX, Ps2MemSize::IopRam, s_psxM);
	PHYSICAL_ALLOC(PS2MEM_VU0MICRO, 0x00010000, s_psVuMem);

	VIRTUAL_ALLOC(PS2MEM_PSXHW, Ps2MemSize::IopHardware, PROT_READ|PROT_WRITE);
	VIRTUAL_ALLOC(PS2MEM_PSXHW4, 0x00010000, PROT_NONE);
	VIRTUAL_ALLOC(PS2MEM_GS, 0x00002000, PROT_READ|PROT_WRITE);
	VIRTUAL_ALLOC(PS2MEM_DEV9, 0x00010000, PROT_NONE);
	VIRTUAL_ALLOC(PS2MEM_SPU2, 0x00010000, PROT_NONE);
	VIRTUAL_ALLOC(PS2MEM_SPU2_, 0x00010000, PROT_NONE);

	VIRTUAL_ALLOC(PS2MEM_B80, 0x00010000, PROT_READ|PROT_WRITE);
	VIRTUAL_ALLOC(PS2MEM_BA0, 0x00010000, PROT_READ|PROT_WRITE);

	// special addrs mmap
	VIRTUAL_ALLOC(PS2MEM_BASE+0x5fff0000, 0x10000, PROT_READ|PROT_WRITE);

eCleanupAndExit:
	memShutdown();
	return -1;
}

void memShutdown()
{
	VIRTUAL_FREE(PS2MEM_BASE, 0x40000000);
	VIRTUAL_FREE(PS2MEM_PSX, 0x00800000);

	PHYSICAL_FREE(PS2MEM_BASE, Ps2MemSize::Base, s_psM);
	VIRTUAL_FREE(PS2MEM_ROM, Ps2MemSize::Rom);
	VIRTUAL_FREE(PS2MEM_ROM1, Ps2MemSize::Rom1);
	VIRTUAL_FREE(PS2MEM_ROM2, Ps2MemSize::Rom2);
	VIRTUAL_FREE(PS2MEM_EROM, Ps2MemSize::ERom);
	PHYSICAL_FREE(PS2MEM_SCRATCH, 0x00010000, s_psS);
	PHYSICAL_FREE(PS2MEM_HW, Ps2MemSize::Hardware, s_psHw);
	PHYSICAL_FREE(PS2MEM_PSX, Ps2MemSize::IopRam, s_psxM);
	PHYSICAL_FREE(PS2MEM_VU0MICRO, 0x00010000, s_psVuMem);

	VIRTUAL_FREE(PS2MEM_VU0MICRO, 0x00010000); // allocate for all VUs

	VIRTUAL_FREE(PS2MEM_PSXHW, Ps2MemSize::IopHardware);
	VIRTUAL_FREE(PS2MEM_PSXHW4, 0x00010000);
	VIRTUAL_FREE(PS2MEM_GS, 0x00002000);
	VIRTUAL_FREE(PS2MEM_DEV9, 0x00010000);
	VIRTUAL_FREE(PS2MEM_SPU2, 0x00010000);
	VIRTUAL_FREE(PS2MEM_SPU2_, 0x00010000);

	VIRTUAL_FREE(PS2MEM_B80, 0x00010000);
	VIRTUAL_FREE(PS2MEM_BA0, 0x00010000);

	VirtualFree(PS2MEM_VU0MICRO, 0, MEM_RELEASE);

	safe_aligned_free(memLUT); 

	// reserve mem
	if( mmap(PS2MEM_BASE, 0x40000000, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0) != PS2MEM_BASE ) {
		SysPrintf("failed to reserve mem\n");
	}
}

#endif // _WIN32

void vm_Reset()
{
	jASSUME( memLUT != NULL );

	memzero_ptr<sizeof(PSMEMORYMAP)*0x100000>(memLUT);
	for (int i=0; i<0x02000; i++) memLUT[i + 0x00000] = initMemoryMap(&s_psM.aPFNs[i], &s_psM.aVFNs[i]);
	for (int i=2; i<0x00010; i++) memLUT[i + 0x10000] = initMemoryMap(&s_psHw.aPFNs[i], &s_psHw.aVFNs[i]);
	for (int i=0; i<0x00800; i++) memLUT[i + 0x1c000] = initMemoryMap(&s_psxM.aPFNs[(i & 0x1ff)], &s_psxM.aVFNs[(i & 0x1ff)]);
	for (int i=0; i<0x00004; i++)
	{
		memLUT[i + 0x11000] = initMemoryMap(&s_psVuMem.aPFNs[0], &s_psVuMem.aVFNs[0]);
		memLUT[i + 0x11004] = initMemoryMap(&s_psVuMem.aPFNs[1], &s_psVuMem.aVFNs[1]);
		memLUT[i + 0x11008] = initMemoryMap(&s_psVuMem.aPFNs[4+i], &s_psVuMem.aVFNs[4+i]);
		memLUT[i + 0x1100c] = initMemoryMap(&s_psVuMem.aPFNs[8+i], &s_psVuMem.aVFNs[8+i]);

		// Yay! Scratchpad mapping!  We love the scratchpad.
		memLUT[i + 0x50000] = initMemoryMap(&s_psS.aPFNs[i], &s_psS.aVFNs[i]);
	}

	// map to other modes
	memcpy(memLUT+0x80000, memLUT, 0x20000*sizeof(PSMEMORYMAP));
	memcpy(memLUT+0xa0000, memLUT, 0x20000*sizeof(PSMEMORYMAP));
}

// Some games read/write between different addrs but same physical memory
// this causes major slowdowns because it goes into the exception handler, so use this (zerofrog)
u32 VM_RETRANSLATE(u32 mem)
{
	u8* p, *pbase;
	if( (mem&0xffff0000) == 0x50000000 ) // reserved scratch pad mem
		return PS2MEM_BASE_+mem;

	p = (u8*)dmaGetAddrBase(mem);

#ifdef _WIN32	
	// do manual LUT since IPU/SPR seems to use addrs 0x3000xxxx quite often
	if( memLUT[ (p-PS2MEM_BASE)>>12 ].aPFNs == NULL ) {
		return PS2MEM_BASE_+mem;
	}

	pbase = (u8*)memLUT[ (p-PS2MEM_BASE)>>12 ].aVFNs[0];
	if( pbase != NULL )
		p = pbase + ((u32)p&0xfff);
#endif

	return (u32)p;
}

void memSetPageAddr(u32 vaddr, u32 paddr) {

	PSMEMORYMAP* pmap;

	if( vaddr == paddr )
		return;

	if( (vaddr>>28) != 1 && (vaddr>>28) != 9 && (vaddr>>28) != 11 ) {
#ifdef _WIN32
		pmap = &memLUT[vaddr >> 12];

		if( pmap->aPFNs != NULL && (pmap->aPFNs != memLUT[paddr>>12].aPFNs ||
			pmap->aVFNs[0] != TRANSFORM_ADDR(vaddr)+(u32)PS2MEM_BASE) ) {

				SysMapUserPhysicalPages((void*)pmap->aVFNs[0], 1, NULL, 0);
				pmap->aVFNs[0] = 0;
		}

		*pmap = memLUT[paddr >> 12];
#else
		memLUT[vaddr>>12] = memLUT[paddr>>12];
#endif
	}
}

void memClearPageAddr(u32 vaddr) {
	//	SysPrintf("memClearPageAddr: %8.8x\n", vaddr);

	if ((vaddr & 0xffffc000) == 0x70000000) return;

#ifdef _WIN32
	//	if( vaddr >= 0x20000000 && vaddr < 0x80000000 ) {
	//		Cpu->Clear(vaddr&~0xfff, 0x1000/4);
	//		if( memLUT[vaddr>>12].aVFNs != NULL ) {
	//			SysMapUserPhysicalPages((void*)memLUT[vaddr>>12].aVFNs[0], 1, NULL, 0 );
	//			memLUT[vaddr>>12].aVFNs = NULL;
	//			memLUT[vaddr>>12].aPFNs = NULL;
	//		}
	//	}
#else
	if( memLUT[vaddr>>12] != NULL ) {
		SysVirtualFree(memLUT[vaddr>>12], 0x1000);
		memLUT[vaddr>>12] = 0;
	}
#endif
}

u8 recMemRead8()
{
	register u32 mem;
	__asm mov mem, ecx // already anded with ~0xa0000000

		switch( (mem&~0xffff) ) {
case 0x1f400000: return psxHw4Read8(mem);
case 0x10000000: return hwRead8(mem);
case 0x1f800000: return psxHwRead8(mem);
case 0x12000000: return *(PS2MEM_BASE+(mem&~0xc00));
case 0x14000000:
	{
		u32 ret = DEV9read8(mem & ~0x04000000);
		SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, ret);
		return ret;
	}

default:
	return *(u8*)(PS2MEM_BASE+mem);
	}
	MEM_LOG("Unknown Memory read32  from address %8.8x\n", mem);

	cpuTlbMissR(mem, cpuRegs.branch);

	return 0;
}

void _eeReadConstMem8(int mmreg, u32 mem, int sign)
{
	assert( !IS_XMMREG(mmreg));

	if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVDMtoMMX(mmreg&0xf, mem-3);
		assert(0);
	}
	else {
		if( sign ) MOVSX32M8toR(mmreg, mem);
		else MOVZX32M8toR(mmreg, mem);
	}
}

void _eeReadConstMem16(int mmreg, u32 mem, int sign)
{
	assert( !IS_XMMREG(mmreg));

	if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVDMtoMMX(mmreg&0xf, mem-2);
		assert(0);
	}
	else {
		if( sign ) MOVSX32M16toR(mmreg, mem);
		else MOVZX32M16toR(mmreg, mem);
	}
}

void _eeReadConstMem32(int mmreg, u32 mem)
{
	if( IS_XMMREG(mmreg) ) SSEX_MOVD_M32_to_XMM(mmreg&0xf, mem);
	else if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVDMtoMMX(mmreg&0xf, mem);
	}
	else MOV32MtoR(mmreg, mem);
}

void _eeReadConstMem128(int mmreg, u32 mem)
{
	if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVQMtoR((mmreg>>4)&0xf, mem+8);
		MOVQMtoR(mmreg&0xf, mem);
	}
	else SSEX_MOVDQA_M128_to_XMM( mmreg&0xf, mem);
}

void _eeWriteConstMem8(u32 mem, int mmreg)
{
	assert( !IS_XMMREG(mmreg) && !IS_MMXREG(mmreg) );
	if( IS_EECONSTREG(mmreg) ) MOV8ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	else if( IS_PSXCONSTREG(mmreg) ) MOV8ItoM(mem, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	else MOV8RtoM(mem, mmreg);
}

void _eeWriteConstMem16(u32 mem, int mmreg)
{
	assert( !IS_XMMREG(mmreg) && !IS_MMXREG(mmreg) );
	if( IS_EECONSTREG(mmreg) ) MOV16ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	else if( IS_PSXCONSTREG(mmreg) ) MOV16ItoM(mem, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	else MOV16RtoM(mem, mmreg);
}

// op - 0 for AND, 1 for OR
void _eeWriteConstMem16OP(u32 mem, int mmreg, int op)
{
	assert( !IS_XMMREG(mmreg) && !IS_MMXREG(mmreg) );
	switch(op) {
case 0: // AND operation
	if( IS_EECONSTREG(mmreg) ) AND16ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	else if( IS_PSXCONSTREG(mmreg) ) AND16ItoM(mem, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	else AND16RtoM(mem, mmreg);
	break;
case 1: // OR operation
	if( IS_EECONSTREG(mmreg) ) OR16ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	else if( IS_PSXCONSTREG(mmreg) ) OR16ItoM(mem, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	else OR16RtoM(mem, mmreg);
	break;

	jNO_DEFAULT
	}
}

void _eeWriteConstMem32(u32 mem, int mmreg)
{
	if( IS_XMMREG(mmreg) ) SSE2_MOVD_XMM_to_M32(mem, mmreg&0xf);
	else if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVDMMXtoM(mem, mmreg&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) MOV32ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	else if( IS_PSXCONSTREG(mmreg) ) MOV32ItoM(mem, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	else MOV32RtoM(mem, mmreg);
}

void _eeWriteConstMem32OP(u32 mem, int mmreg, int op)
{
	switch(op) {
case 0: // and
	if( IS_XMMREG(mmreg) ) {
		_deleteEEreg((mmreg>>16)&0x1f, 1);
		SSE2_PAND_M128_to_XMM(mmreg&0xf, mem);
		SSE2_MOVD_XMM_to_M32(mem, mmreg&0xf);
	}
	else if( IS_MMXREG(mmreg) ) {
		_deleteEEreg((mmreg>>16)&0x1f, 1);
		SetMMXstate();
		PANDMtoR(mmreg&0xf, mem);
		MOVDMMXtoM(mem, mmreg&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) {
		AND32ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	}
	else if( IS_PSXCONSTREG(mmreg) ) {
		AND32ItoM(mem, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	}
	else {
		AND32RtoM(mem, mmreg&0xf);
	}
	break;

case 1: // or
	if( IS_XMMREG(mmreg) ) {
		_deleteEEreg((mmreg>>16)&0x1f, 1);
		SSE2_POR_M128_to_XMM(mmreg&0xf, mem);
		SSE2_MOVD_XMM_to_M32(mem, mmreg&0xf);
	}
	else if( IS_MMXREG(mmreg) ) {
		_deleteEEreg((mmreg>>16)&0x1f, 1);
		SetMMXstate();
		PORMtoR(mmreg&0xf, mem);
		MOVDMMXtoM(mem, mmreg&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) {
		OR32ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	}
	else if( IS_PSXCONSTREG(mmreg) ) {
		OR32ItoM(mem, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	}
	else {
		OR32RtoM(mem, mmreg&0xf);
	}
	break;

case 2: // not and
	if( mmreg & MEM_XMMTAG ) {
		_deleteEEreg(mmreg>>16, 1);
		SSEX_PANDN_M128_to_XMM(mmreg&0xf, mem);
		SSEX_MOVD_XMM_to_M32(mem, mmreg&0xf);
	}
	else if( mmreg & MEM_MMXTAG ) {
		_deleteEEreg(mmreg>>16, 1);
		PANDNMtoR(mmreg&0xf, mem);
		MOVDMMXtoM(mem, mmreg&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) {
		AND32ItoM(mem, ~g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	}
	else if( IS_PSXCONSTREG(mmreg) ) {
		AND32ItoM(mem, ~g_psxConstRegs[((mmreg>>16)&0x1f)]);
	}
	else {
		NOT32R(mmreg&0xf);
		AND32RtoM(mem, mmreg&0xf);
	}
	break;

default: assert(0);
	}
}

void _eeWriteConstMem64(u32 mem, int mmreg)
{
	if( IS_XMMREG(mmreg) ) SSE_MOVLPS_XMM_to_M64(mem, mmreg&0xf);
	else if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVQRtoM(mem, mmreg&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) {
		MOV32ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
		MOV32ItoM(mem+4, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[1]);
	}
	else assert(0);
}

void _eeWriteConstMem128(u32 mem, int mmreg)
{
	if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVQRtoM(mem, mmreg&0xf);
		MOVQRtoM(mem+8, (mmreg>>4)&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) {
		SetMMXstate();
		MOV32ItoM(mem, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
		MOV32ItoM(mem+4, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[1]);
		MOVQRtoM(mem+8, mmreg&0xf);
	}
	else SSEX_MOVDQA_XMM_to_M128(mem, mmreg&0xf);
}

void _eeMoveMMREGtoR(x86IntRegType to, int mmreg)
{
	if( IS_XMMREG(mmreg) ) SSE2_MOVD_XMM_to_R(to, mmreg&0xf);
	else if( IS_MMXREG(mmreg) ) {
		SetMMXstate();
		MOVD32MMXtoR(to, mmreg&0xf);
	}
	else if( IS_EECONSTREG(mmreg) ) MOV32ItoR(to, g_cpuConstRegs[((mmreg>>16)&0x1f)].UL[0]);
	else if( IS_PSXCONSTREG(mmreg) ) MOV32ItoR(to, g_psxConstRegs[((mmreg>>16)&0x1f)]);
	else if( mmreg != to ) MOV32RtoR(to, mmreg);
}

int recMemConstRead8(u32 x86reg, u32 mem, u32 sign)
{
	mem = TRANSFORM_ADDR(mem);

	switch( mem>>16 ) {
case 0x1f40: return psxHw4ConstRead8(x86reg, mem, sign);
case 0x1000: return hwConstRead8(x86reg, mem, sign);
case 0x1f80: return psxHwConstRead8(x86reg, mem, sign);
case 0x1200: return gsConstRead8(x86reg, mem, sign);
case 0x1400:
	{
		iFlushCall(0);
		PUSH32I(mem & ~0x04000000);
		CALLFunc((u32)DEV9read8);
		if( sign ) MOVSX32R8toR(EAX, EAX);
		else MOVZX32R8toR(EAX, EAX);
		return 1;
	}

default:
	_eeReadConstMem8(x86reg, VM_RETRANSLATE(mem), sign);
	return 0;
	}
}

u16 recMemRead16()  {

	register u32 mem;
	__asm mov mem, ecx // already anded with ~0xa0000000

		switch( mem>>16 ) {
		case 0x1000: return hwRead16(mem);
		case 0x1f80: return psxHwRead16(mem);
		case 0x1200: return *(u16*)(PS2MEM_BASE+(mem&~0xc00));
		case 0x1800: return 0;
		case 0x1a00: return ba0R16(mem);
		case 0x1f90:
		case 0x1f00:
			return SPU2read(mem);
		case 0x1400:
			{
				u32 ret = DEV9read16(mem & ~0x04000000);
				SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, ret);
				return ret;
			}

		default:
			return *(u16*)(PS2MEM_BASE+mem);
	}
	MEM_LOG("Unknown Memory read16  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return 0;
}

int recMemConstRead16(u32 x86reg, u32 mem, u32 sign)
{
	mem = TRANSFORM_ADDR(mem);

	switch( mem>>16 ) {
case 0x1000: return hwConstRead16(x86reg, mem, sign);
case 0x1f80: return psxHwConstRead16(x86reg, mem, sign);
case 0x1200: return gsConstRead16(x86reg, mem, sign);
case 0x1800: 
	if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
	else XOR32RtoR(x86reg, x86reg);
	return 0;
case 0x1a00:
	iFlushCall(0);
	PUSH32I(mem);
	CALLFunc((u32)ba0R16);
	ADD32ItoR(ESP, 4);
	if( sign ) MOVSX32R16toR(EAX, EAX);
	else MOVZX32R16toR(EAX, EAX);
	return 1;

case 0x1f90:
case 0x1f00:
	iFlushCall(0);
	PUSH32I(mem);
	CALLFunc((u32)SPU2read);
	if( sign ) MOVSX32R16toR(EAX, EAX);
	else MOVZX32R16toR(EAX, EAX);
	return 1;

case 0x1400:
	iFlushCall(0);
	PUSH32I(mem & ~0x04000000);
	CALLFunc((u32)DEV9read16);
	if( sign ) MOVSX32R16toR(EAX, EAX);
	else MOVZX32R16toR(EAX, EAX);
	return 1;

default:
	_eeReadConstMem16(x86reg, VM_RETRANSLATE(mem), sign);
	return 0;
	}
}

__declspec(naked)
u32 recMemRead32()  {
	// ecx is address - already anded with ~0xa0000000
	__asm {

		mov edx, ecx
			shr edx, 16
			cmp dx, 0x1000
			je hwread
			cmp dx, 0x1f80
			je psxhwread
			cmp dx, 0x1200
			je gsread
			cmp dx, 0x1400
			je devread

			// default read
			mov eax, dword ptr [ecx+PS2MEM_BASE_]
		ret
	}

hwread:
		{
			__asm {
				cmp ecx, 0x10002000
					jb counterread

					cmp ecx, 0x1000f260
					je hwsifpresetread
					cmp ecx, 0x1000f240
					je hwsifsyncread
					cmp ecx, 0x1000f440
					je hwmch_drd
					cmp ecx, 0x1000f430
					je hwmch_ricm

					cmp ecx, 0x10003000
					jb hwdefread2
					mov eax, dword ptr [ecx+PS2MEM_BASE_]
				ret

					// ipu
hwdefread2:
				push ecx
					call ipuRead32
					add esp, 4	
					ret

					// sif
hwsifpresetread:
				xor eax, eax
					ret
hwsifsyncread:
				mov eax, 0x1000F240
					mov eax, dword ptr [eax+PS2MEM_BASE_]
				or eax, 0xF0000102
					ret
			}

counterread:
				{
					static u32 mem, index;

					// counters
					__asm mov mem, ecx
						index = (mem>>11)&3;

					if( (mem&0x7ff) == 0 ) {
						__asm {
							push index
								call rcntRcount
								add esp, 4
								and eax, 0xffff
								ret
						}
					}

					index = (u32)&counters[index] + ((mem>>2)&0xc);

					__asm {
						mov eax, index
							mov eax, dword ptr [eax]
						movzx eax, ax
							ret
					}
				}

hwmch_drd: // MCH_DRD
				__asm {
					mov eax, dword ptr [ecx+PS2MEM_BASE_-0x10]
					shr eax, 6
						test eax, 0xf
						jz mch_drd_2
hwmch_ricm:
					xor eax, eax
						ret

mch_drd_2:
					shr eax, 10
						and eax, 0xfff
						cmp eax, 0x21 // INIT
						je mch_drd_init
						cmp eax, 0x23 // CNFGA
						je mch_drd_cnfga
						cmp eax, 0x24 // CNFGB
						je mch_drd_cnfgb
						cmp eax, 0x40 // DEVID
						je mch_drd_devid
						xor eax, eax 
						ret

mch_drd_init:
					mov edx, rdram_devices
						xor eax, eax
						cmp edx, rdram_sdevid
						setg al
						add rdram_sdevid, eax
						imul eax, 0x1f
						ret

mch_drd_cnfga:
					mov eax, 0x0D0D
						ret

mch_drd_cnfgb:
					mov eax, 0x0090
						ret

mch_drd_devid:
					mov eax, dword ptr [ecx+PS2MEM_BASE_-0x10]
					and eax, 0x1f
						ret
				}
		}

psxhwread:
		__asm {
			push ecx
				call psxHwRead32		
				add esp, 4
				ret
		}

gsread:
		__asm {
			and ecx, 0xfffff3ff
				mov eax, dword ptr [ecx+PS2MEM_BASE_]
			ret
		}

devread:
		__asm {
			and ecx, 0xfbffffff
				push ecx
				call DEV9read32
				add esp, 4
				ret
		}
}

int recMemConstRead32(u32 x86reg, u32 mem)
{
	mem = TRANSFORM_ADDR(mem);

	switch( (mem&~0xffff) ) {
case 0x10000000: return hwConstRead32(x86reg, mem);
case 0x1f800000: return psxHwConstRead32(x86reg, mem);
case 0x12000000: return gsConstRead32(x86reg, mem);
case 0x14000000:
	iFlushCall(0);
	PUSH32I(mem & ~0x04000000);
	CALLFunc((u32)DEV9read32);
	return 1;

default:
	_eeReadConstMem32(x86reg, VM_RETRANSLATE(mem));
	return 0;
	}
}

void recMemRead64(u64 *out)
{	
	register u32 mem;
	__asm mov mem, ecx // already anded with ~0xa0000000

		switch( (mem&0xffff0000) ) {
case 0x10000000: *out = hwRead64(mem); return;
case 0x11000000: *out = *(u64*)(PS2MEM_BASE+mem); return;
case 0x12000000: *out = *(u64*)(PS2MEM_BASE+(mem&~0xc00)); return;

default:
	//assert(0);
	*out = *(u64*)(PS2MEM_BASE+mem);
	return;
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);
}

void recMemConstRead64(u32 mem, int mmreg)
{
	mem = TRANSFORM_ADDR(mem);

	switch( (mem&0xffff0000) ) {
case 0x10000000: hwConstRead64(mem, mmreg); return;
case 0x12000000: gsConstRead64(mem, mmreg); return;
default:
	if( IS_XMMREG(mmreg) ) SSE_MOVLPS_M64_to_XMM(mmreg&0xff, VM_RETRANSLATE(mem));
	else {
		MOVQMtoR(mmreg, VM_RETRANSLATE(mem));
		SetMMXstate();
	}
	return;
	}
}

void recMemRead128(u64 *out)  {

	register u32 mem;
	__asm mov mem, ecx // already anded with ~0xa0000000

		switch( (mem&0xffff0000) ) {
		case 0x10000000: 
			hwRead128(mem, out);
			return;
		case 0x12000000:
			out[0] = *(u64*)(PS2MEM_BASE+(mem&~0xc00));
			out[1] = *(u64*)(PS2MEM_BASE+(mem&~0xc00)+8);
			return;
		case 0x11000000:
			out[0] = *(u64*)(PS2MEM_BASE+mem);
			out[1] = *(u64*)(PS2MEM_BASE+mem+8);
			return;
		default:
			//assert(0);
			out[0] = *(u64*)(PS2MEM_BASE+mem);
			out[1] = *(u64*)(PS2MEM_BASE+mem+8);
			return;
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);
}

void recMemConstRead128(u32 mem, int xmmreg)
{
	mem = TRANSFORM_ADDR(mem);

	switch( (mem&0xffff0000) ) {
case 0x10000000: hwConstRead128(mem, xmmreg); return;
case 0x12000000: gsConstRead128(mem, xmmreg); return;
default:
	_eeReadConstMem128(xmmreg, VM_RETRANSLATE(mem));
	return;
	}
}

void errwrite()
{
	int i, bit, tempeax;
	__asm mov i, ecx
		__asm mov tempeax, eax
		__asm mov bit, edx
		SysPrintf("Error write%d at %x\n", bit, i);
	assert(0);
	__asm mov eax, tempeax
		__asm mov ecx, i
}

void recMemWrite8()
{
	register u32 mem;
	register u8 value;
	__asm mov mem, ecx // already anded with ~0xa0000000
		__asm mov value, al

		switch( mem>>16 ) {
case 0x1f40: psxHw4Write8(mem, value); return;
case 0x1000: hwWrite8(mem, value); return;
case 0x1f80: psxHwWrite8(mem, value); return;
case 0x1200: gsWrite8(mem, value); return;
case 0x1400:
	DEV9write8(mem & ~0x04000000, value);
	SysPrintf("DEV9 write8 %8.8lx: %2.2lx\n", mem & ~0x04000000, value);
	return;

#ifdef _DEBUG
case 0x1100: assert(0);
#endif
default:
	// vus, bad addrs, etc
	*(u8*)(PS2MEM_BASE+mem) = value;
	return;
	}
	MEM_LOG("Unknown Memory write8   to  address %x with data %2.2x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

int recMemConstWrite8(u32 mem, int mmreg)
{
	mem = TRANSFORM_ADDR(mem);

	switch( mem>>16 ) {
case 0x1f40: psxHw4ConstWrite8(mem, mmreg); return 0;
case 0x1000: hwConstWrite8(mem, mmreg); return 0;
case 0x1f80: psxHwConstWrite8(mem, mmreg); return 0;
case 0x1200: gsConstWrite8(mem, mmreg); return 0;
case 0x1400:
	_recPushReg(mmreg);
	iFlushCall(0);
	PUSH32I(mem & ~0x04000000);
	CALLFunc((u32)DEV9write8);
	return 0;

case 0x1100:
	_eeWriteConstMem8(PS2MEM_BASE_+mem, mmreg);

	if( mem < 0x11004000 ) {
		PUSH32I(1);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU0->Clear);
		ADD32ItoR(ESP, 8);
	}
	else if( mem >= 0x11008000 && mem < 0x1100c000 ) {
		PUSH32I(1);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU1->Clear);
		ADD32ItoR(ESP, 8);
	}
	return 0;

default:
	_eeWriteConstMem8(PS2MEM_BASE_+mem, mmreg);
	return 1;
	}
}

void recMemWrite16()   {

	register u32 mem;
	register u16 value;
	__asm mov mem, ecx // already anded with ~0xa0000000
		__asm mov value, ax

		switch( mem>>16 ) {
		case 0x1000: hwWrite16(mem, value); return;
		case 0x1600:
			//HACK: DEV9 VM crash fix
			return;
		case 0x1f80: psxHwWrite16(mem, value); return;
		case 0x1200: gsWrite16(mem, value); return;
		case 0x1f90:
		case 0x1f00: SPU2write(mem, value); return;
		case 0x1400:
			DEV9write16(mem & ~0x04000000, value);
			SysPrintf("DEV9 write16 %8.8lx: %4.4lx\n", mem & ~0x04000000, value);
			return;

#ifdef _DEBUG
		case 0x1100: assert(0);
#endif
		default:
			// vus, bad addrs, etc
			*(u16*)(PS2MEM_BASE+mem) = value;
			return;
	}
	MEM_LOG("Unknown Memory write16  to  address %x with data %4.4x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

int recMemConstWrite16(u32 mem, int mmreg)
{
	mem = TRANSFORM_ADDR(mem);

	switch( mem>>16 ) {
case 0x1000: hwConstWrite16(mem, mmreg); return 0;
case 0x1600:
	//HACK: DEV9 VM crash fix
	return 0;
case 0x1f80: psxHwConstWrite16(mem, mmreg); return 0;
case 0x1200: gsConstWrite16(mem, mmreg); return 0;
case 0x1f90:
case 0x1f00:
	_recPushReg(mmreg);
	iFlushCall(0);
	PUSH32I(mem);
	CALLFunc((u32)SPU2write);
	return 0;
case 0x1400:
	_recPushReg(mmreg);
	iFlushCall(0);
	PUSH32I(mem & ~0x04000000);
	CALLFunc((u32)DEV9write16);
	return 0;

case 0x1100:
	_eeWriteConstMem16(PS2MEM_BASE_+mem, mmreg);

	if( mem < 0x11004000 ) {
		PUSH32I(1);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU0->Clear);
		ADD32ItoR(ESP, 8);
	}
	else if( mem >= 0x11008000 && mem < 0x1100c000 ) {
		PUSH32I(1);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU1->Clear);
		ADD32ItoR(ESP, 8);
	}
	return 0;

default:
	_eeWriteConstMem16(PS2MEM_BASE_+mem, mmreg);
	return 1;
	}
}

C_ASSERT( sizeof(BASEBLOCK) == 8 );

__declspec(naked)
void recMemWrite32()
{
	// ecx is address - already anded with ~0xa0000000
	__asm {

		mov edx, ecx
			shr edx, 16
			cmp dx, 0x1000
			je hwwrite
			cmp dx, 0x1f80
			je psxwrite
			cmp dx, 0x1200
			je gswrite
			cmp dx, 0x1400
			je devwrite
			cmp dx, 0x1100
			je vuwrite
	}

	__asm {
		// default write
		mov dword ptr [ecx+PS2MEM_BASE_], eax
			ret

hwwrite:
		push eax
			push ecx
			call hwWrite32
			add esp, 8
			ret
psxwrite:
		push eax
			push ecx
			call psxHwWrite32
			add esp, 8
			ret
gswrite:
		push eax
			push ecx
			call gsWrite32
			add esp, 8
			ret
devwrite:
		and ecx, 0xfbffffff
			push eax
			push ecx
			call DEV9write32
			add esp, 8
			ret
vuwrite:
		// default write
		mov dword ptr [ecx+PS2MEM_BASE_], eax

			cmp ecx, 0x11004000
			jge vu1write
			and ecx, 0x3ff8
			// clear vu0mem
			mov eax, CpuVU0
			push 1
			push ecx
			call [eax]CpuVU0.Clear
			add esp, 8
			ret

vu1write:
		cmp ecx, 0x11008000
			jl vuend
			cmp ecx, 0x1100c000
			jge vuend
			// clear vu1mem
			and ecx, 0x3ff8
			mov eax, CpuVU1
			push 1
			push ecx
			call [eax]CpuVU1.Clear
			add esp, 8
vuend:
		ret
	}
}

int recMemConstWrite32(u32 mem, int mmreg)
{
	mem = TRANSFORM_ADDR(mem);

	switch( mem&0xffff0000 ) {
case 0x10000000: hwConstWrite32(mem, mmreg); return 0;
case 0x1f800000: psxHwConstWrite32(mem, mmreg); return 0;
case 0x12000000: gsConstWrite32(mem, mmreg); return 0;
case 0x1f900000:
case 0x1f000000:
	_recPushReg(mmreg);
	iFlushCall(0);
	PUSH32I(mem);
	CALLFunc((u32)SPU2write);
	return 0;
case 0x14000000:
	_recPushReg(mmreg);
	iFlushCall(0);
	PUSH32I(mem & ~0x04000000);
	CALLFunc((u32)DEV9write32);
	return 0;

case 0x11000000:
	_eeWriteConstMem32(PS2MEM_BASE_+mem, mmreg);

	if( mem < 0x11004000 ) {
		PUSH32I(1);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU0->Clear);
		ADD32ItoR(ESP, 8);
	}
	else if( mem >= 0x11008000 && mem < 0x1100c000 ) {
		PUSH32I(1);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU1->Clear);
		ADD32ItoR(ESP, 8);
	}
	return 0;

default:
	_eeWriteConstMem32(PS2MEM_BASE_+mem, mmreg);
	return 1;
	}
}

__declspec(naked) void recMemWrite64()
{
	__asm {
		mov edx, ecx
			shr edx, 16
			cmp dx, 0x1000
			je hwwrite
			cmp dx, 0x1200
			je gswrite
			cmp dx, 0x1100
			je vuwrite
	}

	__asm {
		// default write
		mov edx, 64
			call errwrite

hwwrite:
		push dword ptr [eax+4]
		push dword ptr [eax]
		push ecx
			call hwWrite64
			add esp, 12
			ret

gswrite:
		push dword ptr [eax+4]
		push dword ptr [eax]
		push ecx
			call gsWrite64
			add esp, 12
			ret

vuwrite:
		mov ebx, dword ptr [eax]
		mov edx, dword ptr [eax+4]
		mov dword ptr [ecx+PS2MEM_BASE_], ebx
			mov dword ptr [ecx+PS2MEM_BASE_+4], edx

			cmp ecx, 0x11004000
			jge vu1write
			and ecx, 0x3ff8
			// clear vu0mem
			mov eax, CpuVU0
			push 2
			push ecx
			call [eax]CpuVU0.Clear
			add esp, 8
			ret

vu1write:
		cmp ecx, 0x11008000
			jl vuend
			cmp ecx, 0x1100c000
			jge vuend
			// clear vu1mem
			and ecx, 0x3ff8
			mov eax, CpuVU0
			push 2
			push ecx
			call [eax]CpuVU1.Clear
			add esp, 8
vuend:
		ret
	}
}

int recMemConstWrite64(u32 mem, int mmreg)
{
	mem = TRANSFORM_ADDR(mem);

	switch( (mem>>16) ) {
case 0x1000: hwConstWrite64(mem, mmreg); return 0;
case 0x1200: gsConstWrite64(mem, mmreg); return 0;

case 0x1100:
	_eeWriteConstMem64(PS2MEM_BASE_+mem, mmreg);

	if( mem < 0x11004000 ) {
		PUSH32I(2);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU0->Clear);
		ADD32ItoR(ESP, 8);
	}
	else if( mem >= 0x11008000 && mem < 0x1100c000 ) {
		PUSH32I(2);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU1->Clear);
		ADD32ItoR(ESP, 8);
	}
	return 0;

default:
	_eeWriteConstMem64(PS2MEM_BASE_+mem, mmreg);
	return 1;
	}
}

__declspec(naked)
void recMemWrite128()
{
	__asm {

		mov edx, ecx
			shr edx, 16
			cmp dx, 0x1000
			je hwwrite
			cmp dx, 0x1200
			je gswrite
			cmp dx, 0x1100
			je vuwrite
	}

	__asm {
		mov edx, 128
			call errwrite

hwwrite:

		push eax
			push ecx
			call hwWrite128
			add esp, 8
			ret

vuwrite:
		mov ebx, dword ptr [eax]
		mov edx, dword ptr [eax+4]
		mov edi, dword ptr [eax+8]
		mov eax, dword ptr [eax+12]
		mov dword ptr [ecx+PS2MEM_BASE_], ebx
			mov dword ptr [ecx+PS2MEM_BASE_+4], edx
			mov dword ptr [ecx+PS2MEM_BASE_+8], edi
			mov dword ptr [ecx+PS2MEM_BASE_+12], eax

			cmp ecx, 0x11004000
			jge vu1write
			and ecx, 0x3ff8
			// clear vu0mem
			mov eax, CpuVU0
			push 4
			push ecx
			call [eax]CpuVU0.Clear
			add esp, 8
			ret

vu1write:
		cmp ecx, 0x11008000
			jl vuend
			cmp ecx, 0x1100c000
			jge vuend
			// clear vu1mem
			and ecx, 0x3ff8
			mov eax, CpuVU1
			push 4
			push ecx
			call [eax]CpuVU1.Clear
			add esp, 8
vuend:

		// default write
		//movaps xmm7, qword ptr [eax]

		// removes possible exceptions and saves on remapping memory
		// *might* be faster for certain games, no way to tell
		//		cmp ecx, 0x20000000
		//		jb Write128
		//
		//		// look for better mapping
		//		mov edx, ecx
		//		shr edx, 12
		//		shl edx, 3
		//		add edx, memLUT
		//		mov edx, dword ptr [edx + 4]
		//		cmp edx, 0
		//		je Write128
		//		mov edx, dword ptr [edx]
		//		cmp edx, 0
		//		je Write128
		//		and ecx, 0xfff
		//		movaps qword ptr [ecx+edx], xmm7
		//		jmp CheckOverwrite
		//Write128:
		//movaps qword ptr [ecx+PS2MEM_BASE_], xmm7
		ret

gswrite:
		sub esp, 8
			movlps xmm7, qword ptr [eax]
		movlps qword ptr [esp], xmm7
			push ecx
			call gsWrite64

			// call again for upper 8 bytes
			movlps xmm7, qword ptr [eax+8]
		movlps qword ptr [esp+4], xmm7
			add [esp], 8
			call gsWrite64
			add esp, 12
			ret
	}
}

int recMemConstWrite128(u32 mem, int mmreg)
{
	mem = TRANSFORM_ADDR(mem);

	switch( (mem&0xffff0000) ) {
case 0x10000000: hwConstWrite128(mem, mmreg); return 0;
case 0x12000000: gsConstWrite128(mem, mmreg); return 0;

case 0x11000000:
	_eeWriteConstMem128(PS2MEM_BASE_+mem, mmreg);

	if( mem < 0x11004000 ) {
		PUSH32I(4);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU0->Clear);
		ADD32ItoR(ESP, 8);
	}
	else if( mem >= 0x11008000 && mem < 0x1100c000 ) {
		PUSH32I(4);
		PUSH32I(mem&0x3ff8);
		CALLFunc((u32)CpuVU1->Clear);
		ADD32ItoR(ESP, 8);
	}
	return 0;

default:
	_eeWriteConstMem128(PS2MEM_BASE_+mem, mmreg);
	return 1;
	}
}

int  __fastcall memRead8 (u32 mem, u8  *out)  {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x1f400000: *out = psxHw4Read8(mem); return 0;
		case 0x10000000: *out = hwRead8(mem); return 0;
		case 0x1f800000: *out = psxHwRead8(mem); return 0;
		case 0x12000000: *out = gsRead8(mem); return 0;
		case 0x14000000:
			*out = DEV9read8(mem & ~0x04000000);
			SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
			return 0;

		default:
			*out = *(u8*)(PS2MEM_BASE+mem);
			return 0;
	}

#ifdef MEM_LOG
	MEM_LOG("Unknown Memory read32  from address %8.8x\n", mem);
#endif
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

int  __fastcall memRead8RS (u32 mem, u64 *out)
{	
	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
case 0x1f400000: *out = (s8)psxHw4Read8(mem); return 0;
case 0x10000000: *out = (s8)hwRead8(mem); return 0;
case 0x1f800000: *out = (s8)psxHwRead8(mem); return 0;
case 0x12000000: *out = (s8)gsRead8(mem); return 0;
case 0x14000000:
	*out = (s8)DEV9read8(mem & ~0x04000000);
	SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
	return 0;

default:
	*out = *(s8*)(PS2MEM_BASE+mem);
	return 0;
	}
	MEM_LOG("Unknown Memory read32  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

int __fastcall memRead8RU (u32 mem, u64 *out)
{
	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
case 0x1f400000: *out = (u8)psxHw4Read8(mem); return 0;
case 0x10000000: *out = (u8)hwRead8(mem); return 0;
case 0x1f800000: *out = (u8)psxHwRead8(mem); return 0;
case 0x12000000: *out = (u8)gsRead8(mem); return 0;
case 0x14000000:
	*out = (u8)DEV9read8(mem & ~0x04000000);
	SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
	return 0;

default:
	*out = *(u8*)(PS2MEM_BASE+mem);
	return 0;
	}
	MEM_LOG("Unknown Memory read32  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

int __fastcall memRead16(u32 mem, u16 *out)  {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: *out = hwRead16(mem); return 0;
		case 0x1f800000: *out = psxHwRead16(mem); return 0;
		case 0x12000000: *out = gsRead16(mem); return 0;
		case 0x18000000: *out = 0; return 0;
		case 0x1a000000: *out = ba0R16(mem); return 0;
		case 0x1f900000:
		case 0x1f000000:
			*out = SPU2read(mem); return 0;
			break;
		case 0x14000000:
			*out = DEV9read16(mem & ~0x04000000);
			SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
			return 0;

		default:
			*out = *(u16*)(PS2MEM_BASE+mem);
			return 0;
	}
	MEM_LOG("Unknown Memory read16  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return -1;
}

int __fastcall memRead16RS(u32 mem, u64 *out)  {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: *out = (s16)hwRead16(mem); return 0;
		case 0x1f800000: *out = (s16)psxHwRead16(mem); return 0;
		case 0x12000000: *out = (s16)gsRead16(mem); return 0;
		case 0x18000000: *out = 0; return 0;
		case 0x1a000000: *out = (s16)ba0R16(mem); return 0;
		case 0x1f900000:
		case 0x1f000000:
			*out = (s16)SPU2read(mem); return 0;
			break;
		case 0x14000000:
			*out = (s16)DEV9read16(mem & ~0x04000000);
			SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
			return 0;

		default:
			*out = *(s16*)(PS2MEM_BASE+mem);
			return 0;
	}
	MEM_LOG("Unknown Memory read16  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return -1;
}

int __fastcall memRead16RU(u32 mem, u64 *out)  {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: *out = (u16)hwRead16(mem ); return 0;
		case 0x1f800000: *out = (u16)psxHwRead16(mem ); return 0;
		case 0x12000000: *out = (u16)gsRead16(mem); return 0;
		case 0x18000000: *out = 0; return 0;
		case 0x1a000000: *out = (u16)ba0R16(mem); return 0;
		case 0x1f900000:
		case 0x1f000000:
			*out = (u16)SPU2read(mem ); return 0;
			break;
		case 0x14000000:
			*out = (u16)DEV9read16(mem & ~0x04000000);
			SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
			return 0;

		default:
			*out = *(u16*)(PS2MEM_BASE+mem);
			return 0;
	}
	MEM_LOG("Unknown Memory read16  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return -1;
}

int __fastcall memRead32(u32 mem, u32 *out)  {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: *out = hwRead32(mem); return 0;
		case 0x1f800000: *out = psxHwRead32(mem); return 0;
		case 0x12000000: *out = gsRead32(mem); return 0;
		case 0x14000000:
			*out = (u32)DEV9read32(mem & ~0x04000000);
			SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
			return 0;

		default:
			*out = *(u32*)(PS2MEM_BASE+mem);
			return 0;
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

int __fastcall memRead32RS(u32 mem, u64 *out)
{
	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
case 0x10000000: *out = (s32)hwRead32(mem); return 0;
case 0x1f800000: *out = (s32)psxHwRead32(mem); return 0;
case 0x12000000: *out = (s32)gsRead32(mem); return 0;
case 0x14000000:
	*out = (s32)DEV9read32(mem & ~0x04000000);
	SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
	return 0;

default:
	*out = *(s32*)(PS2MEM_BASE+mem);
	return 0;
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

int __fastcall memRead32RU(u32 mem, u64 *out)
{
	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
case 0x10000000: *out = (u32)hwRead32(mem); return 0;
case 0x1f800000: *out = (u32)psxHwRead32(mem); return 0;
case 0x12000000: *out = (u32)gsRead32(mem); return 0;
case 0x14000000:
	*out = (u32)DEV9read32(mem & ~0x04000000);
	SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0x04000000, *out);
	return 0;

default:
	*out = *(u32*)(PS2MEM_BASE+mem);
	return 0;
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

int __fastcall memRead64(u32 mem, u64 *out)  {
	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: *out = hwRead64(mem); return 0;
		case 0x12000000: *out = gsRead64(mem); return 0;

		default:
			*out = *(u64*)(PS2MEM_BASE+mem);
			return 0;
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

int __fastcall memRead128(u32 mem, u64 *out)  {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: 
			hwRead128(mem, out);
			return 0;
		case 0x12000000:
			out[0] = gsRead64(mem);
			out[1] = gsRead64(mem + 8);
			return 0;

		default:
			out[0] = *(u64*)(PS2MEM_BASE+mem);
			out[1] = *(u64*)(PS2MEM_BASE+mem+8);
			return 0;
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);

	return -1;
}

void __fastcall memWrite8 (u32 mem, u8  value)   {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x1f400000: psxHw4Write8(mem, value); return;
		case 0x10000000: hwWrite8(mem, value); return;
		case 0x1f800000: psxHwWrite8(mem, value); return;
		case 0x12000000: gsWrite8(mem, value); return;
		case 0x14000000:
			DEV9write8(mem & ~0x04000000, value);
			SysPrintf("DEV9 write8 %8.8lx: %2.2lx\n", mem & ~0x04000000, value);
			return;

		default:
			*(u8*)(PS2MEM_BASE+mem) = value;

			if (CHECK_EEREC) {
				REC_CLEARM(mem&~3);
			}
			return;
	}
	MEM_LOG("Unknown Memory write8   to  address %x with data %2.2x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

void __fastcall memWrite16(u32 mem, u16 value)   {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: hwWrite16(mem, value); return;
		case 0x1f800000: psxHwWrite16(mem, value); return;
		case 0x12000000: gsWrite16(mem, value); return;
		case 0x1f900000:
		case 0x1f000000: SPU2write(mem, value); return;
		case 0x14000000:
			DEV9write16(mem & ~0x04000000, value);
			SysPrintf("DEV9 write16 %8.8lx: %4.4lx\n", mem & ~0x04000000, value);
			return;

		default:
			*(u16*)(PS2MEM_BASE+mem) = value;
			if (CHECK_EEREC) {
				REC_CLEARM(mem&~3);
			}
			return;
	}
	MEM_LOG("Unknown Memory write16  to  address %x with data %4.4x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

void __fastcall memWrite32(u32 mem, u32 value)
{
	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
case 0x10000000: hwWrite32(mem, value); return;
case 0x1f800000: psxHwWrite32(mem, value); return;
case 0x12000000: gsWrite32(mem, value); return;
case 0x1f900000:
case 0x1f000000: SPU2write(mem, value); return;
case 0x14000000:
	DEV9write32(mem & ~0x4000000, value);
	SysPrintf("DEV9 write32 %8.8lx: %8.8lx\n", mem & ~0x4000000, value);
	return;

default:
	*(u32*)(PS2MEM_BASE+mem) = value;

	if (CHECK_EEREC) {
		REC_CLEARM(mem);
	}
	return;
	}
	MEM_LOG("Unknown Memory write32  to  address %x with data %8.8x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

void __fastcall memWrite64(u32 mem, const u64* value)   {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: hwWrite64(mem, *value); return;
		case 0x12000000: gsWrite64(mem, *value); return;

		default:
			*(u64*)(PS2MEM_BASE+mem) = *value;

			if (CHECK_EEREC) {
				REC_CLEARM(mem);
				REC_CLEARM(mem+4);
			}
			return;
	}
	MEM_LOG("Unknown Memory write64  to  address %x with data %8.8x_%8.8x\n", mem, (u32)((*value)>>32), (u32)value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

void __fastcall memWrite128(u32 mem, const u64 *value) {

	mem = TRANSFORM_ADDR(mem);
	switch( (mem&~0xffff) ) {
		case 0x10000000: hwWrite128(mem, value); return;
		case 0x12000000:
			gsWrite64(mem, value[0]);
			gsWrite64(mem + 8, value[1]);
			return;

		default:
			*(u64*)(PS2MEM_BASE+mem) = value[0];
			*(u64*)(PS2MEM_BASE+mem+8) = value[1];

			if (CHECK_EEREC) {
				REC_CLEARM(mem);
				REC_CLEARM(mem+4);
				REC_CLEARM(mem+8);
				REC_CLEARM(mem+12);
			}
			return;
	}
	MEM_LOG("Unknown Memory write128 to  address %x with data %8.8x_%8.8x_%8.8x_%8.8x\n", mem, ((u32*)value)[3], ((u32*)value)[2], ((u32*)value)[1], ((u32*)value)[0]);
	cpuTlbMissW(mem, cpuRegs.branch);
}

// Resets memory mappings, unmaps TLBs, reloads bios roms, etc.
void memReset()
{
#ifdef _WIN32
	DWORD OldProtect;
	// make sure can write
	VirtualProtect(PS2MEM_ROM, Ps2MemSize::Rom, PAGE_READWRITE, &OldProtect);
	VirtualProtect(PS2MEM_ROM1, Ps2MemSize::Rom1, PAGE_READWRITE, &OldProtect);
	VirtualProtect(PS2MEM_ROM2, Ps2MemSize::Rom2, PAGE_READWRITE, &OldProtect);
	VirtualProtect(PS2MEM_EROM, Ps2MemSize::ERom, PAGE_READWRITE, &OldProtect);
#else
	mprotect(PS2EMEM_ROM, Ps2MemSize::Rom, PROT_READ|PROT_WRITE);
	mprotect(PS2EMEM_ROM1, Ps2MemSize::Rom1, PROT_READ|PROT_WRITE);
	mprotect(PS2EMEM_ROM2, Ps2MemSize::Rom2, PROT_READ|PROT_WRITE);
	mprotect(PS2EMEM_EROM, Ps2MemSize::ERom, PROT_READ|PROT_WRITE);
#endif

	memzero_ptr<Ps2MemSize::Base>(PS2MEM_BASE);
	memzero_ptr<Ps2MemSize::Scratch>(PS2MEM_SCRATCH);
	vm_Reset();

	string Bios;
	FILE *fp;

	Path::Combine( Bios, Config.BiosDir, Config.Bios );

	long filesize;
	if( ( filesize = Path::getFileSize( Bios ) ) <= 0 )
	{
		//Console::Error("Unable to load bios: '%s', PCSX2 can't run without that", params Bios);
		throw Exception::FileNotFound( Bios,
			"The specified Bios file was not found.  A bios is required for Pcsx2 to run.\n\nFile not found" );
	}

	fp = fopen(Bios.c_str(), "rb");
	fread(PS2MEM_ROM, 1, std::min( (long)Ps2MemSize::Rom, filesize ), fp);
	fclose(fp);

	BiosVersion = GetBiosVersion();
	Console::Status("Bios Version %d.%d", params BiosVersion >> 8, BiosVersion & 0xff);

	//injectIRX("host.irx");	//not fully tested; still buggy

	loadBiosRom("rom1", PS2MEM_ROM1, Ps2MemSize::Rom1);
	loadBiosRom("rom2", PS2MEM_ROM2, Ps2MemSize::Rom2);
	loadBiosRom("erom", PS2MEM_EROM, Ps2MemSize::ERom);

#ifdef _WIN32
	VirtualProtect(PS2MEM_ROM, Ps2MemSize::Rom, PAGE_READONLY, &OldProtect);
	VirtualProtect(PS2MEM_ROM1, Ps2MemSize::Rom1, PAGE_READONLY, &OldProtect);
	VirtualProtect(PS2MEM_ROM2, Ps2MemSize::Rom2, PAGE_READONLY, &OldProtect);
	VirtualProtect(PS2MEM_EROM, Ps2MemSize::ERom, PAGE_READONLY, &OldProtect);
#else
	mprotect(PS2EMEM_ROM, Ps2MemSize::Rom, PROT_READ);
	mprotect(PS2EMEM_ROM1, Ps2MemSize::Rom1, PROT_READ);
	mprotect(PS2EMEM_ROM2, Ps2MemSize::Rom2, PROT_READ);
	mprotect(PS2EMEM_EROM, Ps2MemSize::ERom, PROT_READ);
#endif
}

#endif
