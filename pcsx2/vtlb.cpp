/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

using namespace R5900;
using namespace vtlb_private;

#ifdef PCSX2_DEVBUILD
#define verify(x) {if (!(x)) { (*(u8*)0)=3; }}
#else
#define verify jASSUME
#endif

namespace vtlb_private
{
	s32 pmap[VTLB_PMAP_ITEMS];	//512KB
	s32 vmap[VTLB_VMAP_ITEMS];   //4MB

	// first indexer -- 8/16/32/64/128 bit tables [values 0-4]
	// second indexer -- read/write  [0 or 1]
	// third indexer -- 128 pages of memory!
	void* RWFT[5][2][128];
}

vtlbHandler vtlbHandlerCount=0;

vtlbHandler DefaultPhyHandler;
vtlbHandler UnmappedVirtHandler0;
vtlbHandler UnmappedVirtHandler1;
vtlbHandler UnmappedPhyHandler0;
vtlbHandler UnmappedPhyHandler1;


	/*
	__asm
	{
		mov eax,ecx;
		shr ecx,12;
		mov ecx,[ecx*4+vmap];	//translate
		add ecx,eax;			//transform
		
		js callfunction;		//if <0 its invalid ptr :)

		mov eax,[ecx];
		mov [edx],eax;
		xor eax,eax;
		ret;

callfunction:
		xchg eax,ecx;
		shr eax,12; //get the 'ppn'

		//ecx = original addr
		//eax = function entry + 0x800000
		//edx = data ptr
		jmp [readfunctions8-0x800000+eax];
	}*/

/////////////////////////////////////////////////////////////////////////
// Interpreter Implementations of VTLB Memory Operations.
// See recVTLB.cpp for the dynarec versions.

// Interpreterd VTLB lookup for 8, 16, and 32 bit accesses
template<int DataSize,typename DataType>
__forceinline DataType __fastcall MemOp_r0(u32 addr)
{
	u32 vmv=vmap[addr>>VTLB_PAGE_BITS];
	s32 ppf=addr+vmv;

	if (!(ppf<0))
		return *reinterpret_cast<DataType*>(ppf);

	//has to: translate, find function, call function
	u32 hand=(u8)vmv;
	u32 paddr=ppf-hand+0x80000000;
	//SysPrintf("Translated 0x%08X to 0x%08X\n",addr,paddr);
	//return reinterpret_cast<TemplateHelper<DataSize,false>::HandlerType*>(RWFT[TemplateHelper<DataSize,false>::sidx][0][hand])(paddr,data);

	switch( DataSize )
	{
		case 8: return ((vtlbMemR8FP*)RWFT[0][0][hand])(paddr);
		case 16: return ((vtlbMemR16FP*)RWFT[1][0][hand])(paddr);
		case 32: return ((vtlbMemR32FP*)RWFT[2][0][hand])(paddr);

		jNO_DEFAULT;
	}
}

// Interpreterd VTLB lookup for 64 and 128 bit accesses.
template<int DataSize,typename DataType>
__forceinline void __fastcall MemOp_r1(u32 addr, DataType* data)
{
	u32 vmv=vmap[addr>>VTLB_PAGE_BITS];
	s32 ppf=addr+vmv;

	if (!(ppf<0))
	{
		data[0]=*reinterpret_cast<DataType*>(ppf);
		if (DataSize==128)
			data[1]=*reinterpret_cast<DataType*>(ppf+8);
	}
	else
	{	
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//SysPrintf("Translated 0x%08X to 0x%08X\n",addr,paddr);
		//return reinterpret_cast<TemplateHelper<DataSize,false>::HandlerType*>(RWFT[TemplateHelper<DataSize,false>::sidx][0][hand])(paddr,data);

		switch( DataSize )
		{
			case 64: ((vtlbMemR64FP*)RWFT[3][0][hand])(paddr, data); break;
			case 128: ((vtlbMemR128FP*)RWFT[4][0][hand])(paddr, data); break;

			jNO_DEFAULT;
		}
	}
}

template<int DataSize,typename DataType>
__forceinline void __fastcall MemOp_w0(u32 addr, DataType data)
{
	u32 vmv=vmap[addr>>VTLB_PAGE_BITS];
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
		//SysPrintf("Translted 0x%08X to 0x%08X\n",addr,paddr);

		switch( DataSize )
		{
			case 8: return ((vtlbMemW8FP*)RWFT[0][1][hand])(paddr, (u8)data);
			case 16: return ((vtlbMemW16FP*)RWFT[1][1][hand])(paddr, (u16)data);
			case 32: return ((vtlbMemW32FP*)RWFT[2][1][hand])(paddr, (u32)data);

			jNO_DEFAULT;
		}
	}
}
template<int DataSize,typename DataType>
__forceinline void __fastcall MemOp_w1(u32 addr,const DataType* data)
{
	verify(DataSize==128 || DataSize==64);
	u32 vmv=vmap[addr>>VTLB_PAGE_BITS];
	s32 ppf=addr+vmv;
	if (!(ppf<0))
	{
		*reinterpret_cast<DataType*>(ppf)=*data;
		if (DataSize==128)
			*reinterpret_cast<DataType*>(ppf+8)=data[1];
	}
	else
	{	
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//SysPrintf("Translted 0x%08X to 0x%08X\n",addr,paddr);
		switch( DataSize )
		{
			case 64: return ((vtlbMemW64FP*)RWFT[3][1][hand])(paddr, data);
			case 128: return ((vtlbMemW128FP*)RWFT[4][1][hand])(paddr, data);

			jNO_DEFAULT;
		}
	}
}

mem8_t __fastcall vtlb_memRead8(u32 mem)
{
	return MemOp_r0<8,mem8_t>(mem);
}
mem16_t __fastcall vtlb_memRead16(u32 mem)
{
	return MemOp_r0<16,mem16_t>(mem);
}
mem32_t __fastcall vtlb_memRead32(u32 mem)
{
	return MemOp_r0<32,mem32_t>(mem);
}
void __fastcall vtlb_memRead64(u32 mem, u64 *out)
{
	return MemOp_r1<64,mem64_t>(mem,out);
}
void __fastcall vtlb_memRead128(u32 mem, u64 *out)
{
	return MemOp_r1<128,mem128_t>(mem,out);
}
void __fastcall vtlb_memWrite8 (u32 mem, mem8_t value)
{
	MemOp_w0<8,mem8_t>(mem,value);
}
void __fastcall vtlb_memWrite16(u32 mem, mem16_t value)
{
	MemOp_w0<16,mem16_t>(mem,value);
}
void __fastcall vtlb_memWrite32(u32 mem, mem32_t value)
{
	MemOp_w0<32,mem32_t>(mem,value);
}
void __fastcall vtlb_memWrite64(u32 mem, const mem64_t* value)
{
	MemOp_w1<64,mem64_t>(mem,value);
}
void __fastcall vtlb_memWrite128(u32 mem, const mem128_t *value)
{
	MemOp_w1<128,mem128_t>(mem,value);
}

// Some functions used by interpreters and stuff...
// These maintain a "consistent" API with 64/128 reads.
void __fastcall memRead8(u32 mem, u8  *out) { *out = vtlb_memRead8( mem ); }
void __fastcall memRead16(u32 mem, u16 *out) { *out = vtlb_memRead16( mem ); }
void __fastcall memRead32(u32 mem, u32 *out) { *out = vtlb_memRead32( mem ); }

/////////////////////////////////////////////////////////////////////////
// Error / TLB Miss Handlers
// 

// Generates a tlbMiss Exception
static __forceinline void vtlb_Miss(u32 addr,u32 mode)
{
	Console::Error("vtlb miss : addr 0x%X, mode %d", params addr,mode);
	verify(false);

	if (mode==0)
		cpuTlbMissR(addr, cpuRegs.branch);
	else
		cpuTlbMissW(addr, cpuRegs.branch);
}

// Just dies a horrible death for now.
// Eventually should generate a BusError exception.
static __forceinline void vtlb_BusError(u32 addr,u32 mode)
{
	Console::Error("vtlb bus error : addr 0x%X, mode %d\n",params addr,mode);
	verify(false);
}

///// Virtual Mapping Errors (TLB Miss)
template<u32 saddr>
mem8_t __fastcall vtlbUnmappedVRead8(u32 addr) { vtlb_Miss(addr|saddr,0); return 0; }
template<u32 saddr>
mem16_t __fastcall vtlbUnmappedVRead16(u32 addr)  { vtlb_Miss(addr|saddr,0); return 0; }
template<u32 saddr>
mem32_t __fastcall vtlbUnmappedVRead32(u32 addr) { vtlb_Miss(addr|saddr,0); return 0; }
template<u32 saddr>
void __fastcall vtlbUnmappedVRead64(u32 addr,mem64_t* data) { vtlb_Miss(addr|saddr,0); }
template<u32 saddr>
void __fastcall vtlbUnmappedVRead128(u32 addr,mem128_t* data) { vtlb_Miss(addr|saddr,0); }
template<u32 saddr>
void __fastcall vtlbUnmappedVWrite8(u32 addr,mem8_t data) { vtlb_Miss(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedVWrite16(u32 addr,mem16_t data) { vtlb_Miss(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedVWrite32(u32 addr,mem32_t data) { vtlb_Miss(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedVWrite64(u32 addr,const mem64_t* data) { vtlb_Miss(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedVWrite128(u32 addr,const mem128_t* data) { vtlb_Miss(addr|saddr,1); }

///// Physical Mapping Errors (Bus Error)
template<u32 saddr>
mem8_t __fastcall vtlbUnmappedPRead8(u32 addr) { vtlb_BusError(addr|saddr,0); return 0; }
template<u32 saddr>
mem16_t __fastcall vtlbUnmappedPRead16(u32 addr)  { vtlb_BusError(addr|saddr,0); return 0; }
template<u32 saddr>
mem32_t __fastcall vtlbUnmappedPRead32(u32 addr) { vtlb_BusError(addr|saddr,0); return 0; }
template<u32 saddr>
void __fastcall vtlbUnmappedPRead64(u32 addr,mem64_t* data) { vtlb_BusError(addr|saddr,0); }
template<u32 saddr>
void __fastcall vtlbUnmappedPRead128(u32 addr,mem128_t* data) { vtlb_BusError(addr|saddr,0); }
template<u32 saddr>
void __fastcall vtlbUnmappedPWrite8(u32 addr,mem8_t data) { vtlb_BusError(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedPWrite16(u32 addr,mem16_t data) { vtlb_BusError(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedPWrite32(u32 addr,mem32_t data) { vtlb_BusError(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedPWrite64(u32 addr,const mem64_t* data) { vtlb_BusError(addr|saddr,1); }
template<u32 saddr>
void __fastcall vtlbUnmappedPWrite128(u32 addr,const mem128_t* data) { vtlb_BusError(addr|saddr,1); }

///// VTLB mapping errors (unmapped address spaces)
mem8_t __fastcall vtlbDefaultPhyRead8(u32 addr) { Console::Error("vtlbDefaultPhyRead8: 0x%X",params addr); verify(false); return -1; }
mem16_t __fastcall vtlbDefaultPhyRead16(u32 addr)  { Console::Error("vtlbDefaultPhyRead16: 0x%X",params addr); verify(false); return -1; }
mem32_t __fastcall vtlbDefaultPhyRead32(u32 addr) { Console::Error("vtlbDefaultPhyRead32: 0x%X",params addr); verify(false); return -1; }
void __fastcall vtlbDefaultPhyRead64(u32 addr,mem64_t* data) { Console::Error("vtlbDefaultPhyRead64: 0x%X",params addr); verify(false); }
void __fastcall vtlbDefaultPhyRead128(u32 addr,mem128_t* data) { Console::Error("vtlbDefaultPhyRead128: 0x%X",params addr); verify(false); }

void __fastcall vtlbDefaultPhyWrite8(u32 addr,mem8_t data) { Console::Error("vtlbDefaultPhyWrite8: 0x%X",params addr); verify(false); }
void __fastcall vtlbDefaultPhyWrite16(u32 addr,mem16_t data) { Console::Error("vtlbDefaultPhyWrite16: 0x%X",params addr); verify(false); }
void __fastcall vtlbDefaultPhyWrite32(u32 addr,mem32_t data) { Console::Error("vtlbDefaultPhyWrite32: 0x%X",params addr); verify(false); }
void __fastcall vtlbDefaultPhyWrite64(u32 addr,const mem64_t* data) { Console::Error("vtlbDefaultPhyWrite64: 0x%X",params addr); verify(false); }
void __fastcall vtlbDefaultPhyWrite128(u32 addr,const mem128_t* data) { Console::Error("vtlbDefaultPhyWrite128: 0x%X",params addr); verify(false); }


/////////////////////////////////////////////////////////////////////////
// VTLB Public API -- Init/Term/RegisterHandler stuff
// 


// Registers a handler into the VTLB's internal handler array.  The handler defines specific behavior
// for how memory pages bound to the handler are read from / written to.  If any of the handler pointers
// are NULL, the memory operations will be mapped to the BusError handler (thus generating BusError
// exceptions if the emulated app attempts to access them).
//
// Note: All handlers persist across calls to vtlb_Reset(), but are wiped/invalidated by calls to vtlb_Init()
//
// Returns a handle for the newly created handler  See .vtlb_MapHandler for use of the return value.
vtlbHandler vtlb_RegisterHandler(	vtlbMemR8FP* r8,vtlbMemR16FP* r16,vtlbMemR32FP* r32,vtlbMemR64FP* r64,vtlbMemR128FP* r128,
									vtlbMemW8FP* w8,vtlbMemW16FP* w16,vtlbMemW32FP* w32,vtlbMemW64FP* w64,vtlbMemW128FP* w128)
{
	//write the code :p
	vtlbHandler rv=vtlbHandlerCount++;
	
	RWFT[0][0][rv] = (r8!=0)   ? r8:vtlbDefaultPhyRead8;
	RWFT[1][0][rv] = (r16!=0)  ? r16:vtlbDefaultPhyRead16;
	RWFT[2][0][rv] = (r32!=0)  ? r32:vtlbDefaultPhyRead32;
	RWFT[3][0][rv] = (r64!=0)  ? r64:vtlbDefaultPhyRead64;
	RWFT[4][0][rv] = (r128!=0) ? r128:vtlbDefaultPhyRead128;

	RWFT[0][1][rv] = (w8!=0)   ? w8:vtlbDefaultPhyWrite8;
	RWFT[1][1][rv] = (w16!=0)  ? w16:vtlbDefaultPhyWrite16;
	RWFT[2][1][rv] = (w32!=0)  ? w32:vtlbDefaultPhyWrite32;
	RWFT[3][1][rv] = (w64!=0)  ? w64:vtlbDefaultPhyWrite64;
	RWFT[4][1][rv] = (w128!=0) ? w128:vtlbDefaultPhyWrite128;

	return rv;
}

// Maps the given hander (created with vtlb_RegisterHandler) to the specified memory region.
// New mappings always assume priority over previous mappings, so place "generic" mappings for
// large areas of memory first, and then specialize specific small regions of memory afterward.
// A single handler can be mapped to many different regions by using multiple calls to this
// function.
//
// The memory region start and size parameters must be pagesize aligned.
void vtlb_MapHandler(vtlbHandler handler,u32 start,u32 size)
{
	verify(0==(start&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);
	s32 value=handler|0x80000000;

	while(size>0)
	{
		pmap[start>>VTLB_PAGE_BITS]=value;

		start+=VTLB_PAGE_SIZE;
		size-=VTLB_PAGE_SIZE;
	}	
}

void vtlb_MapBlock(void* base,u32 start,u32 size,u32 blocksize)
{
	s32 baseint=(s32)base;

	verify(0==(start&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);
	if (blocksize==0) 
		blocksize=size;
	verify(0==(blocksize&VTLB_PAGE_MASK) && blocksize>0);
	verify(0==(size%blocksize));

	while(size>0)
	{
		u32 blocksz=blocksize;
		s32 ptr=baseint;

		while(blocksz>0)
		{
			pmap[start>>VTLB_PAGE_BITS]=ptr;

			start+=VTLB_PAGE_SIZE;
			ptr+=VTLB_PAGE_SIZE;
			blocksz-=VTLB_PAGE_SIZE;
			size-=VTLB_PAGE_SIZE;
		}
	}
}

void vtlb_Mirror(u32 new_region,u32 start,u32 size)
{
	verify(0==(new_region&VTLB_PAGE_MASK));
	verify(0==(start&VTLB_PAGE_MASK));
	verify(0==(size&VTLB_PAGE_MASK) && size>0);

	while(size>0)
	{
		pmap[start>>VTLB_PAGE_BITS]=pmap[new_region>>VTLB_PAGE_BITS];

		start+=VTLB_PAGE_SIZE;
		new_region+=VTLB_PAGE_SIZE;
		size-=VTLB_PAGE_SIZE;
	}	
}

__forceinline void* vtlb_GetPhyPtr(u32 paddr)
{
	if (paddr>=VTLB_PMAP_SZ || pmap[paddr>>VTLB_PAGE_BITS]<0)
		return 0;
	else
		return reinterpret_cast<void*>(pmap[paddr>>VTLB_PAGE_BITS]+(paddr&VTLB_PAGE_MASK));

}

//virtual mappings
//TODO: Add invalid paddr checks
void vtlb_VMap(u32 vaddr,u32 paddr,u32 sz)
{
	verify(0==(vaddr&VTLB_PAGE_MASK));
	verify(0==(paddr&VTLB_PAGE_MASK));
	verify(0==(sz&VTLB_PAGE_MASK) && sz>0);

	while(sz>0)
	{
		s32 pme;
		if (paddr>=VTLB_PMAP_SZ)
		{
			pme=UnmappedPhyHandler0;
			if (paddr&0x80000000)
				pme=UnmappedPhyHandler1;
			pme|=0x80000000;
			pme|=paddr;// top bit is set anyway ...
		}
		else
		{
			pme=pmap[paddr>>VTLB_PAGE_BITS];
			if (pme<0)
				pme|=paddr;// top bit is set anyway ...
		}
		vmap[vaddr>>VTLB_PAGE_BITS]=pme-vaddr;
		vaddr+=VTLB_PAGE_SIZE;
		paddr+=VTLB_PAGE_SIZE;
		sz-=VTLB_PAGE_SIZE;
	}
}

void vtlb_VMapBuffer(u32 vaddr,void* buffer,u32 sz)
{
	verify(0==(vaddr&VTLB_PAGE_MASK));
	verify(0==(sz&VTLB_PAGE_MASK) && sz>0);
	u32 bu8=(u32)buffer;
	while(sz>0)
	{
		vmap[vaddr>>VTLB_PAGE_BITS]=bu8-vaddr;
		vaddr+=VTLB_PAGE_SIZE;
		bu8+=VTLB_PAGE_SIZE;
		sz-=VTLB_PAGE_SIZE;
	}
}
void vtlb_VMapUnmap(u32 vaddr,u32 sz)
{
	verify(0==(vaddr&VTLB_PAGE_MASK));
	verify(0==(sz&VTLB_PAGE_MASK) && sz>0);
	
	while(sz>0)
	{
		u32 handl=UnmappedVirtHandler0;
		if (vaddr&0x80000000)
		{
			handl=UnmappedVirtHandler1;
		}
		handl|=vaddr; // top bit is set anyway ...
		handl|=0x80000000;
		vmap[vaddr>>VTLB_PAGE_BITS]=handl-vaddr;
		vaddr+=VTLB_PAGE_SIZE;
		sz-=VTLB_PAGE_SIZE;
	}
}

// Clears vtlb handlers and memory mappings.
void vtlb_Init()
{
	vtlbHandlerCount=0;
	memzero_obj(RWFT);

	//Register default handlers
	//Unmapped Virt handlers _MUST_ be registered first.
	//On address translation the top bit cannot be preserved.This is not normaly a problem since
	//the physical address space can be 'compressed' to just 29 bits.However, to properly handle exceptions
	//there must be a way to get the full address back.Thats why i use these 2 functions and encode the hi bit directly into em :)

	UnmappedVirtHandler0=vtlb_RegisterHandler(vtlbUnmappedVRead8<0>,vtlbUnmappedVRead16<0>,vtlbUnmappedVRead32<0>,vtlbUnmappedVRead64<0>,vtlbUnmappedVRead128<0>,
											  vtlbUnmappedVWrite8<0>,vtlbUnmappedVWrite16<0>,vtlbUnmappedVWrite32<0>,vtlbUnmappedVWrite64<0>,vtlbUnmappedVWrite128<0>);

	UnmappedVirtHandler1=vtlb_RegisterHandler(vtlbUnmappedVRead8<0x80000000>,vtlbUnmappedVRead16<0x80000000>,vtlbUnmappedVRead32<0x80000000>,
												vtlbUnmappedVRead64<0x80000000>,vtlbUnmappedVRead128<0x80000000>,
											  vtlbUnmappedVWrite8<0x80000000>,vtlbUnmappedVWrite16<0x80000000>,vtlbUnmappedVWrite32<0x80000000>,
												vtlbUnmappedVWrite64<0x80000000>,vtlbUnmappedVWrite128<0x80000000>);

	UnmappedPhyHandler0=vtlb_RegisterHandler(vtlbUnmappedPRead8<0>,vtlbUnmappedPRead16<0>,vtlbUnmappedPRead32<0>,vtlbUnmappedPRead64<0>,vtlbUnmappedPRead128<0>,
											  vtlbUnmappedPWrite8<0>,vtlbUnmappedPWrite16<0>,vtlbUnmappedPWrite32<0>,vtlbUnmappedPWrite64<0>,vtlbUnmappedPWrite128<0>);

	UnmappedPhyHandler1=vtlb_RegisterHandler(vtlbUnmappedPRead8<0x80000000>,vtlbUnmappedPRead16<0x80000000>,vtlbUnmappedPRead32<0x80000000>,
												vtlbUnmappedPRead64<0x80000000>,vtlbUnmappedPRead128<0x80000000>,
											  vtlbUnmappedPWrite8<0x80000000>,vtlbUnmappedPWrite16<0x80000000>,vtlbUnmappedPWrite32<0x80000000>,
												vtlbUnmappedPWrite64<0x80000000>,vtlbUnmappedPWrite128<0x80000000>);
	DefaultPhyHandler=vtlb_RegisterHandler(0,0,0,0,0,0,0,0,0,0);

	//done !

	//Setup the initial mappings
	vtlb_MapHandler(DefaultPhyHandler,0,VTLB_PMAP_SZ);	
	
	//Set the V space as unmapped
	vtlb_VMapUnmap(0,(VTLB_VMAP_ITEMS-1)*VTLB_PAGE_SIZE);
	//yeah i know, its stupid .. but this code has to be here for now ;p
	vtlb_VMapUnmap((VTLB_VMAP_ITEMS-1)*VTLB_PAGE_SIZE,VTLB_PAGE_SIZE);
}

// Performs a COP0-level reset of the PS2's TLB.
// This function should probably be part of the COP0 rather than here in VTLB.
void vtlb_Reset()
{
	for(int i=0; i<48; i++) UnmapTLB(i);
}

void vtlb_Term()
{
	//nothing to do for now
}

// This function allocates memory block with are compatible with the Vtlb's requirements
// for memory locations.  The Vtlb requires the topmost bit (Sign bit) of the memory
// pointer to be cleared.  Some operating systems and/or implementations of malloc do that,
// but others do not.  So use this instead to allocate the memory correctly for your
// platform.
u8* vtlb_malloc( uint size, uint align, uptr tryBaseAddress )
{
#ifdef __LINUX__
	return SysMmapEx( tryBaseAddress, size, 0x80000000, "Vtlb" );
#else
	// Win32 just needs this, since malloc always maps below 2GB.
	return (u8*)_aligned_malloc(size, align);
#endif
}

void vtlb_free( void* pmem, uint size )
{
	if( pmem == NULL ) return;

#ifdef __LINUX__
	SafeSysMunmap( pmem, size );
#else
	// Make sure and unprotect memory first, since CrtDebug will try to write to it.
	DWORD old;
	VirtualProtect( pmem, size, PAGE_READWRITE, &old );
	safe_aligned_free( pmem );
#endif
}
