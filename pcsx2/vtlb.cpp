/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

/*
	EE physical map :
	[0000 0000,1000 0000) -> Ram (mirrored ?)
	[1000 0000,1400 0000) -> Registers
	[1400 0000,1fc0 0000) -> Reserved (ingored writes, 'random' reads)
	[1fc0 0000,2000 0000) -> Boot ROM

	[2000 0000,4000 0000) -> Unmapped (BUS ERROR)
	[4000 0000,8000 0000) -> "Extended memory", probably unmapped (BUS ERROR) on retail ps2's :)
	[8000 0000,FFFF FFFF] -> Unmapped (BUS ERROR)

	vtlb/phy only supports the [0000 0000,2000 0000) region, with 4k pages.
	vtlb/vmap supports mapping to either of these locations, or some other (externaly) specified address.
*/

#include "PrecompiledHeader.h"

#include "Common.h"
#include "vtlb.h"
#include "COP0.h"
#include "R5900Exceptions.h"

#include "Utilities/MemsetFast.inl"

using namespace R5900;
using namespace vtlb_private;

#define verify pxAssume

namespace vtlb_private
{
	__aligned(64) MapData vtlbdata;
}

static vtlbHandler vtlbHandlerCount = 0;

static vtlbHandler DefaultPhyHandler;
static vtlbHandler UnmappedVirtHandler0;
static vtlbHandler UnmappedVirtHandler1;
static vtlbHandler UnmappedPhyHandler0;
static vtlbHandler UnmappedPhyHandler1;


// --------------------------------------------------------------------------------------
// Interpreter Implementations of VTLB Memory Operations.
// --------------------------------------------------------------------------------------
// See recVTLB.cpp for the dynarec versions.

template< typename DataType >
DataType __fastcall vtlb_memRead(u32 addr)
{
	static const uint DataSize = sizeof(DataType) * 8;
	u32 vmv=vtlbdata.vmap[addr>>VTLB_PAGE_BITS];
	s32 ppf=addr+vmv;

	if (!(ppf<0))
		return *reinterpret_cast<DataType*>(ppf);

	//has to: translate, find function, call function
	u32 hand=(u8)vmv;
	u32 paddr=ppf-hand+0x80000000;
	//Console.WriteLn("Translated 0x%08X to 0x%08X", addr,paddr);
	//return reinterpret_cast<TemplateHelper<DataSize,false>::HandlerType*>(vtlbdata.RWFT[TemplateHelper<DataSize,false>::sidx][0][hand])(paddr,data);

	switch( DataSize )
	{
		case 8: return ((vtlbMemR8FP*)vtlbdata.RWFT[0][0][hand])(paddr);
		case 16: return ((vtlbMemR16FP*)vtlbdata.RWFT[1][0][hand])(paddr);
		case 32: return ((vtlbMemR32FP*)vtlbdata.RWFT[2][0][hand])(paddr);

		jNO_DEFAULT;
	}

	return 0;		// technically unreachable, but suppresses warnings.
}

void __fastcall vtlb_memRead64(u32 mem, mem64_t *out)
{
	u32 vmv=vtlbdata.vmap[mem>>VTLB_PAGE_BITS];
	s32 ppf=mem+vmv;

	if (!(ppf<0))
	{
		*out = *(mem64_t*)ppf;
	}
	else
	{
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//Console.WriteLn("Translated 0x%08X to 0x%08X", addr,paddr);

		((vtlbMemR64FP*)vtlbdata.RWFT[3][0][hand])(paddr, out);
	}
}
void __fastcall vtlb_memRead128(u32 mem, mem128_t *out)
{
	u32 vmv=vtlbdata.vmap[mem>>VTLB_PAGE_BITS];
	s32 ppf=mem+vmv;

	if (!(ppf<0))
	{
		CopyQWC(out,(void*)ppf);
	}
	else
	{
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//Console.WriteLn("Translated 0x%08X to 0x%08X", addr,paddr);

		((vtlbMemR128FP*)vtlbdata.RWFT[4][0][hand])(paddr, out);
	}
}

template< typename DataType >
void __fastcall vtlb_memWrite(u32 addr, DataType data)
{
	static const uint DataSize = sizeof(DataType) * 8;

	u32 vmv=vtlbdata.vmap[addr>>VTLB_PAGE_BITS];
	s32 ppf=addr+vmv;
	if (!(ppf<0))
	{
		*reinterpret_cast<DataType*>(ppf)=data;
	}
	else
	{
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//Console.WriteLn("Translated 0x%08X to 0x%08X", addr,paddr);

		switch( DataSize )
		{
			case 8: return ((vtlbMemW8FP*)vtlbdata.RWFT[0][1][hand])(paddr, (u8)data);
			case 16: return ((vtlbMemW16FP*)vtlbdata.RWFT[1][1][hand])(paddr, (u16)data);
			case 32: return ((vtlbMemW32FP*)vtlbdata.RWFT[2][1][hand])(paddr, (u32)data);

			jNO_DEFAULT;
		}
	}
}

void __fastcall vtlb_memWrite64(u32 mem, const mem64_t* value)
{
	u32 vmv=vtlbdata.vmap[mem>>VTLB_PAGE_BITS];
	s32 ppf=mem+vmv;
	if (!(ppf<0))
	{
		*(mem64_t*)ppf = *value;
	}
	else
	{
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//Console.WriteLn("Translated 0x%08X to 0x%08X", addr,paddr);

		((vtlbMemW64FP*)vtlbdata.RWFT[3][1][hand])(paddr, value);
	}
}

void __fastcall vtlb_memWrite128(u32 mem, const mem128_t *value)
{
	u32 vmv=vtlbdata.vmap[mem>>VTLB_PAGE_BITS];
	s32 ppf=mem+vmv;
	if (!(ppf<0))
	{
		CopyQWC((void*)ppf, value);
	}
	else
	{
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//Console.WriteLn("Translated 0x%08X to 0x%08X", addr,paddr);

		((vtlbMemW128FP*)vtlbdata.RWFT[4][1][hand])(paddr, value);
	}
}

template mem8_t vtlb_memRead<mem8_t>(u32 mem);
template mem16_t vtlb_memRead<mem16_t>(u32 mem);
template mem32_t vtlb_memRead<mem32_t>(u32 mem);
template void vtlb_memWrite<mem8_t>(u32 mem, mem8_t data);
template void vtlb_memWrite<mem16_t>(u32 mem, mem16_t data);
template void vtlb_memWrite<mem32_t>(u32 mem, mem32_t data);

// --------------------------------------------------------------------------------------
//  TLB Miss / BusError Handlers
// --------------------------------------------------------------------------------------
// These are valid VM memory errors that should typically be handled by the VM itself via
// its own cpu exception system.
//
// [TODO]  Add first-chance debugging hooks to these exceptions!
//
// Important recompiler note: Mid-block Exception handling isn't reliable *yet* because
// memory ops don't flush the PC prior to invoking the indirect handlers.

// Generates a tlbMiss Exception
static __ri void vtlb_Miss(u32 addr,u32 mode)
{
	if( IsDevBuild )
		Cpu->ThrowCpuException( R5900Exception::TLBMiss( addr, !!mode ) );
	else
		Console.Error( R5900Exception::TLBMiss( addr, !!mode ).FormatMessage() );
}

// BusError exception: more serious than a TLB miss.  If properly emulated the PS2 kernel
// itself would invoke a diagnostic/assertion screen that displays the cpu state at the
// time of the exception.
static __ri void vtlb_BusError(u32 addr,u32 mode)
{
	if( IsDevBuild )
		Cpu->ThrowCpuException( R5900Exception::BusError( addr, !!mode ) );
	else
		Console.Error( R5900Exception::TLBMiss( addr, !!mode ).FormatMessage() );
}

#define _tmpl(ret) template<typename OperandType, u32 saddr> ret __fastcall

_tmpl(OperandType) vtlbUnmappedVReadSm(u32 addr)					{ vtlb_Miss(addr|saddr,0); return 0; }
_tmpl(void) vtlbUnmappedVReadLg(u32 addr,OperandType* data)			{ vtlb_Miss(addr|saddr,0); }
_tmpl(void) vtlbUnmappedVWriteSm(u32 addr,OperandType data)			{ vtlb_Miss(addr|saddr,1); }
_tmpl(void) vtlbUnmappedVWriteLg(u32 addr,const OperandType* data)	{ vtlb_Miss(addr|saddr,1); }

_tmpl(OperandType) vtlbUnmappedPReadSm(u32 addr)					{ vtlb_BusError(addr|saddr,0); return 0; }
_tmpl(void) vtlbUnmappedPReadLg(u32 addr,OperandType* data)			{ vtlb_BusError(addr|saddr,0); }
_tmpl(void) vtlbUnmappedPWriteSm(u32 addr,OperandType data)			{ vtlb_BusError(addr|saddr,1); }
_tmpl(void) vtlbUnmappedPWriteLg(u32 addr,const OperandType* data)	{ vtlb_BusError(addr|saddr,1); }

#undef _tmpl

// --------------------------------------------------------------------------------------
//  VTLB mapping errors
// --------------------------------------------------------------------------------------
// These errors are assertion/logic errors that should never occur if PCSX2 has been initialized
// properly.  All addressable physical memory should be configured as TLBMiss or Bus Error.
//

static mem8_t __fastcall vtlbDefaultPhyRead8(u32 addr)
{
	pxFailDev(pxsFmt("(VTLB) Attempted read8 from unmapped physical address @ 0x%08X.", addr));
	return 0;
}

static mem16_t __fastcall vtlbDefaultPhyRead16(u32 addr)
{
	pxFailDev(pxsFmt("(VTLB) Attempted read16 from unmapped physical address @ 0x%08X.", addr));
	return 0;
}

static mem32_t __fastcall vtlbDefaultPhyRead32(u32 addr)
{
	pxFailDev(pxsFmt("(VTLB) Attempted read32 from unmapped physical address @ 0x%08X.", addr));
	return 0;
}

static void __fastcall vtlbDefaultPhyRead64(u32 addr, mem64_t* dest)
{
	pxFailDev(pxsFmt("(VTLB) Attempted read64 from unmapped physical address @ 0x%08X.", addr));
}

static void __fastcall vtlbDefaultPhyRead128(u32 addr, mem128_t* dest)
{
	pxFailDev(pxsFmt("(VTLB) Attempted read128 from unmapped physical address @ 0x%08X.", addr));
}

static void __fastcall vtlbDefaultPhyWrite8(u32 addr, mem8_t data)
{
	pxFailDev(pxsFmt("(VTLB) Attempted write8 to unmapped physical address @ 0x%08X.", addr));
}

static void __fastcall vtlbDefaultPhyWrite16(u32 addr, mem16_t data)
{
	pxFailDev(pxsFmt("(VTLB) Attempted write16 to unmapped physical address @ 0x%08X.", addr));
}

static void __fastcall vtlbDefaultPhyWrite32(u32 addr, mem32_t data)
{
	pxFailDev(pxsFmt("(VTLB) Attempted write32 to unmapped physical address @ 0x%08X.", addr));
}

static void __fastcall vtlbDefaultPhyWrite64(u32 addr,const mem64_t* data)
{
	pxFailDev(pxsFmt("(VTLB) Attempted write64 to unmapped physical address @ 0x%08X.", addr));
}

static void __fastcall vtlbDefaultPhyWrite128(u32 addr,const mem128_t* data)
{
	pxFailDev(pxsFmt("(VTLB) Attempted write128 to unmapped physical address @ 0x%08X.", addr));
}
#undef _tmpl

// ===========================================================================================
//  VTLB Public API -- Init/Term/RegisterHandler stuff 
// ===========================================================================================
//

// Assigns or re-assigns the callbacks for a VTLB memory handler.  The handler defines specific behavior
// for how memory pages bound to the handler are read from / written to.  If any of the handler pointers
// are NULL, the memory operations will be mapped to the BusError handler (thus generating BusError
// exceptions if the emulated app attempts to access them).
//
// Note: All handlers persist across calls to vtlb_Reset(), but are wiped/invalidated by calls to vtlb_Init()
//
__ri void vtlb_ReassignHandler( vtlbHandler rv,
							   vtlbMemR8FP* r8,vtlbMemR16FP* r16,vtlbMemR32FP* r32,vtlbMemR64FP* r64,vtlbMemR128FP* r128,
							   vtlbMemW8FP* w8,vtlbMemW16FP* w16,vtlbMemW32FP* w32,vtlbMemW64FP* w64,vtlbMemW128FP* w128 )
{
	pxAssume(rv < VTLB_HANDLER_ITEMS);

	vtlbdata.RWFT[0][0][rv] = (void*)((r8!=0)   ? r8	: vtlbDefaultPhyRead8);
	vtlbdata.RWFT[1][0][rv] = (void*)((r16!=0)  ? r16	: vtlbDefaultPhyRead16);
	vtlbdata.RWFT[2][0][rv] = (void*)((r32!=0)  ? r32	: vtlbDefaultPhyRead32);
	vtlbdata.RWFT[3][0][rv] = (void*)((r64!=0)  ? r64	: vtlbDefaultPhyRead64);
	vtlbdata.RWFT[4][0][rv] = (void*)((r128!=0) ? r128	: vtlbDefaultPhyRead128);

	vtlbdata.RWFT[0][1][rv] = (void*)((w8!=0)   ? w8	: vtlbDefaultPhyWrite8);
	vtlbdata.RWFT[1][1][rv] = (void*)((w16!=0)  ? w16	: vtlbDefaultPhyWrite16);
	vtlbdata.RWFT[2][1][rv] = (void*)((w32!=0)  ? w32	: vtlbDefaultPhyWrite32);
	vtlbdata.RWFT[3][1][rv] = (void*)((w64!=0)  ? w64	: vtlbDefaultPhyWrite64);
	vtlbdata.RWFT[4][1][rv] = (void*)((w128!=0) ? w128	: vtlbDefaultPhyWrite128);
}

vtlbHandler vtlb_NewHandler()
{
	pxAssertDev( vtlbHandlerCount < VTLB_HANDLER_ITEMS, "VTLB handler count overflow!" );
	return vtlbHandlerCount++;
}

// Registers a handler into the VTLB's internal handler array.  The handler defines specific behavior
// for how memory pages bound to the handler are read from / written to.  If any of the handler pointers
// are NULL, the memory operations will be mapped to the BusError handler (thus generating BusError
// exceptions if the emulated app attempts to access them).
//
// Note: All handlers persist across calls to vtlb_Reset(), but are wiped/invalidated by calls to vtlb_Init()
//
// Returns a handle for the newly created handler  See vtlb_MapHandler for use of the return value.
//
__ri vtlbHandler vtlb_RegisterHandler(	vtlbMemR8FP* r8,vtlbMemR16FP* r16,vtlbMemR32FP* r32,vtlbMemR64FP* r64,vtlbMemR128FP* r128,
										vtlbMemW8FP* w8,vtlbMemW16FP* w16,vtlbMemW32FP* w32,vtlbMemW64FP* w64,vtlbMemW128FP* w128)
{
	vtlbHandler rv = vtlb_NewHandler();
	vtlb_ReassignHandler( rv, r8, r16, r32, r64, r128, w8, w16, w32, w64, w128 );
	return rv;
}


// Maps the given hander (created with vtlb_RegisterHandler) to the specified memory region.
// New mappings always assume priority over previous mappings, so place "generic" mappings for
// large areas of memory first, and then specialize specific small regions of memory afterward.
// A single handler can be mapped to many different regions by using multiple calls to this
// function.
//
// The memory region start and size parameters must be pagesize aligned.
void vtlb_MapHandler(vtlbHandler handler, u32 start, u32 size)
{
	verify(0==(start&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);

	s32 value = handler | 0x80000000;
	u32 end = start + (size - VTLB_PAGE_SIZE);
	pxAssume( (end>>VTLB_PAGE_BITS) < ArraySize(vtlbdata.pmap) );

	while (start <= end)
	{
		vtlbdata.pmap[start>>VTLB_PAGE_BITS] = value;
		start += VTLB_PAGE_SIZE;
	}
}

void vtlb_MapBlock(void* base, u32 start, u32 size, u32 blocksize)
{
	verify(0==(start&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);
	if (!blocksize)
		blocksize = size;
	verify(0==(blocksize&VTLB_PAGE_MASK) && blocksize>0);
	verify(0==(size%blocksize));

	s32 baseint = (s32)base;
	u32 end = start + (size - VTLB_PAGE_SIZE);
	pxAssume( (end>>VTLB_PAGE_BITS) < ArraySize(vtlbdata.pmap) );

	while (start <= end)
	{
		u32 loopsz = blocksize;
		s32 ptr = baseint;

		while (loopsz > 0)
		{
			vtlbdata.pmap[start>>VTLB_PAGE_BITS] = ptr;

			start	+= VTLB_PAGE_SIZE;
			ptr		+= VTLB_PAGE_SIZE;
			loopsz	-= VTLB_PAGE_SIZE;
		}
	}
}

void vtlb_Mirror(u32 new_region,u32 start,u32 size)
{
	verify(0==(new_region&VTLB_PAGE_MASK));
	verify(0==(start&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);

	u32 end = start + (size-VTLB_PAGE_SIZE);
	pxAssume( (end>>VTLB_PAGE_BITS) < ArraySize(vtlbdata.pmap) );

	while(start <= end)
	{
		vtlbdata.pmap[start>>VTLB_PAGE_BITS] = vtlbdata.pmap[new_region>>VTLB_PAGE_BITS];

		start		+= VTLB_PAGE_SIZE;
		new_region	+= VTLB_PAGE_SIZE;
	}
}

__fi void* vtlb_GetPhyPtr(u32 paddr)
{
	if (paddr>=VTLB_PMAP_SZ || vtlbdata.pmap[paddr>>VTLB_PAGE_BITS]<0)
		return NULL;
	else
		return reinterpret_cast<void*>(vtlbdata.pmap[paddr>>VTLB_PAGE_BITS]+(paddr&VTLB_PAGE_MASK));
}

//virtual mappings
//TODO: Add invalid paddr checks
void vtlb_VMap(u32 vaddr,u32 paddr,u32 size)
{
	verify(0==(vaddr&VTLB_PAGE_MASK));
	verify(0==(paddr&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);

	while (size > 0)
	{
		s32 pme;
		if (paddr >= VTLB_PMAP_SZ)
		{
			pme = UnmappedPhyHandler0;
			if (paddr & 0x80000000)
				pme = UnmappedPhyHandler1;
			pme |= 0x80000000;
			pme |= paddr;// top bit is set anyway ...
		}
		else
		{
			pme = vtlbdata.pmap[paddr>>VTLB_PAGE_BITS];
			if (pme<0)
				pme |= paddr;// top bit is set anyway ...
		}

		vtlbdata.vmap[vaddr>>VTLB_PAGE_BITS] = pme-vaddr;
		vaddr += VTLB_PAGE_SIZE;
		paddr += VTLB_PAGE_SIZE;
		size -= VTLB_PAGE_SIZE;
	}
}

void vtlb_VMapBuffer(u32 vaddr,void* buffer,u32 size)
{
	verify(0==(vaddr&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);

	u32 bu8 = (u32)buffer;
	while (size > 0)
	{
		vtlbdata.vmap[vaddr>>VTLB_PAGE_BITS] = bu8-vaddr;
		vaddr += VTLB_PAGE_SIZE;
		bu8 += VTLB_PAGE_SIZE;
		size -= VTLB_PAGE_SIZE;
	}
}
void vtlb_VMapUnmap(u32 vaddr,u32 size)
{
	verify(0==(vaddr&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);

	while (size > 0)
	{
		u32 handl = UnmappedVirtHandler0;
		if (vaddr & 0x80000000)
		{
			handl = UnmappedVirtHandler1;
		}

		handl |= vaddr; // top bit is set anyway ...
		handl |= 0x80000000;

		vtlbdata.vmap[vaddr>>VTLB_PAGE_BITS] = handl-vaddr;
		vaddr += VTLB_PAGE_SIZE;
		size -= VTLB_PAGE_SIZE;
	}
}

// vtlb_Init -- Clears vtlb handlers and memory mappings.
void vtlb_Init()
{
	vtlbHandlerCount=0;
	memzero(vtlbdata.RWFT);

#define VTLB_BuildUnmappedHandler(baseName, highBit) \
	baseName##ReadSm<mem8_t,0>,		baseName##ReadSm<mem16_t,0>,	baseName##ReadSm<mem32_t,0>, \
	baseName##ReadLg<mem64_t,0>,	baseName##ReadLg<mem128_t,0>, \
	baseName##WriteSm<mem8_t,0>,	baseName##WriteSm<mem16_t,0>,	baseName##WriteSm<mem32_t,0>, \
	baseName##WriteLg<mem64_t,0>,	baseName##WriteLg<mem128_t,0>

	//Register default handlers
	//Unmapped Virt handlers _MUST_ be registered first.
	//On address translation the top bit cannot be preserved.This is not normaly a problem since
	//the physical address space can be 'compressed' to just 29 bits.However, to properly handle exceptions
	//there must be a way to get the full address back.Thats why i use these 2 functions and encode the hi bit directly into em :)

	UnmappedVirtHandler0 = vtlb_RegisterHandler( VTLB_BuildUnmappedHandler(vtlbUnmappedV, 0) );
	UnmappedVirtHandler1 = vtlb_RegisterHandler( VTLB_BuildUnmappedHandler(vtlbUnmappedV, 0x80000000) );

	UnmappedPhyHandler0 = vtlb_RegisterHandler( VTLB_BuildUnmappedHandler(vtlbUnmappedP, 0) );
	UnmappedPhyHandler1 = vtlb_RegisterHandler( VTLB_BuildUnmappedHandler(vtlbUnmappedP, 0x80000000) );

	DefaultPhyHandler = vtlb_RegisterHandler(0,0,0,0,0,0,0,0,0,0);

	//done !

	//Setup the initial mappings
	vtlb_MapHandler(DefaultPhyHandler,0,VTLB_PMAP_SZ);

	//Set the V space as unmapped
	vtlb_VMapUnmap(0,(VTLB_VMAP_ITEMS-1)*VTLB_PAGE_SIZE);
	//yeah i know, its stupid .. but this code has to be here for now ;p
	vtlb_VMapUnmap((VTLB_VMAP_ITEMS-1)*VTLB_PAGE_SIZE,VTLB_PAGE_SIZE);

	extern void vtlb_dynarec_init();
	vtlb_dynarec_init();
}

// vtlb_Reset -- Performs a COP0-level reset of the PS2's TLB.
// This function should probably be part of the COP0 rather than here in VTLB.
void vtlb_Reset()
{
	for(int i=0; i<48; i++) UnmapTLB(i);
}

void vtlb_Term()
{
	//nothing to do for now
}

// Reserves the vtlb core allocation used by various emulation components!
// [TODO] basemem - request allocating memory at the specified virtual location, which can allow
//    for easier debugging and/or 3rd party cheat programs.  If 0, the operating system
//    default is used.
void vtlb_Core_Alloc()
{
	if (!vtlbdata.vmap)
	{
		vtlbdata.vmap = (s32*)_aligned_malloc( VTLB_VMAP_ITEMS * sizeof(*vtlbdata.vmap), 16 );
		if (!vtlbdata.vmap)
			throw Exception::OutOfMemory( L"VTLB Virtual Address Translation LUT" )
				.SetDiagMsg(pxsFmt("(%u megs)", VTLB_VMAP_ITEMS * sizeof(*vtlbdata.vmap) / _1mb)
			);
	}
}

void vtlb_Core_Free()
{
	safe_aligned_free( vtlbdata.vmap );
}

static wxString GetHostVmErrorMsg()
{
	return pxE(".Error:HostVmReserve",
		L"Your system is too low on virtual resources for PCSX2 to run.  This can be "
		L"caused by having a small or disabled swapfile, or by other programs that are "
		L"hogging resources."
	);
}
// --------------------------------------------------------------------------------------
//  VtlbMemoryReserve  (implementations)
// --------------------------------------------------------------------------------------
VtlbMemoryReserve::VtlbMemoryReserve( const wxString& name, size_t size )
	: m_reserve( name, size )
{
	m_reserve.SetPageAccessOnCommit( PageAccess_ReadWrite() );
}

void VtlbMemoryReserve::SetBaseAddr( uptr newaddr )
{
	m_reserve.SetBaseAddr( newaddr );
}

void VtlbMemoryReserve::Reserve( sptr hostptr )
{
	if (!m_reserve.ReserveAt( hostptr ))
	{
		throw Exception::OutOfMemory( m_reserve.GetName() )
			.SetDiagMsg(L"Vtlb memory could not be reserved.")
			.SetUserMsg(GetHostVmErrorMsg());
	}
}

void VtlbMemoryReserve::Commit()
{
	if (IsCommitted()) return;
	if (!m_reserve.Commit())
	{
		throw Exception::OutOfMemory( m_reserve.GetName() )
			.SetDiagMsg(L"Vtlb memory could not be committed.")
			.SetUserMsg(GetHostVmErrorMsg());
	}
}

void VtlbMemoryReserve::Reset()
{
	Commit();
	memzero_sse_a(m_reserve.GetPtr(), m_reserve.GetCommittedBytes());
}

void VtlbMemoryReserve::Decommit()
{
	m_reserve.Reset();
}

void VtlbMemoryReserve::Release()
{
	m_reserve.Release();
}

bool VtlbMemoryReserve::IsCommitted() const
{
	return !!m_reserve.GetCommittedPageCount();
}