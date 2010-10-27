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

RAM
---
0x00100000-0x01ffffff this is the physical address for the ram.its cached there
0x20100000-0x21ffffff uncached
0x30100000-0x31ffffff uncached & accelerated
0xa0000000-0xa1ffffff MIRROR might...???
0x80000000-0x81ffffff MIRROR might... ????

scratch pad
----------
0x70000000-0x70003fff scratch pad

BIOS
----
0x1FC00000 - 0x1FFFFFFF un-cached
0x9FC00000 - 0x9FFFFFFF cached
0xBFC00000 - 0xBFFFFFFF un-cached
*/

#include "PrecompiledHeader.h"
#include <wx/file.h>

#include "IopCommon.h"
#include "VUmicro.h"
#include "GS.h"

#include "ps2/HwInternal.h"
#include "ps2/BiosTools.h"

#include "Utilities/PageFaultSource.h"

#ifdef ENABLECACHE
#include "Cache.h"
#endif

int MemMode = 0;		// 0 is Kernel Mode, 1 is Supervisor Mode, 2 is User Mode

void memSetKernelMode() {
	//Do something here
	MemMode = 0;
}

void memSetSupervisorMode() {
}

void memSetUserMode() {

}

u16 ba0R16(u32 mem)
{
	//MEM_LOG("ba00000 Memory read16 address %x", mem);

	if (mem == 0x1a000006) {
		static int ba6;
		ba6++;
		if (ba6 == 3) ba6 = 0;
		return ba6;
	}
	return 0;
}

#define CHECK_MEM(mem) //MyMemCheck(mem)

void MyMemCheck(u32 mem)
{
    if( mem == 0x1c02f2a0 )
        Console.WriteLn("yo; (mem == 0x1c02f2a0) in MyMemCheck...");
}

/////////////////////////////
// REGULAR MEM START
/////////////////////////////
static vtlbHandler
	null_handler,

	tlb_fallback_0,
	tlb_fallback_1,
	tlb_fallback_2,
	tlb_fallback_3,
	tlb_fallback_4,
	tlb_fallback_5,
	tlb_fallback_6,
	tlb_fallback_7,
	tlb_fallback_8,

	vu0_micro_mem,
	vu1_micro_mem,

	hw_by_page[0x10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },

	gs_page_0,
	gs_page_1,

	iopHw_by_page_01,
	iopHw_by_page_03,
	iopHw_by_page_08;


void memMapVUmicro()
{
	// VU0/VU1 micro mem (instructions)
	// (Like IOP memory, these are generally only used by the EE Bios kernel during
	//  boot-up.  Applications/games are "supposed" to use the thread-safe VIF instead;
	//  or must ensure all VIF/GIF transfers are finished and all VUmicro execution stopped
	//  prior to accessing VU memory directly).

	// The VU0 mapping actually repeats 4 times across the mapped range, but we don't bother
	// to manually mirror it here because the indirect memory handler for it (see vuMicroRead*
	// functions below) automatically mask and wrap the address for us.

	vtlb_MapHandler(vu0_micro_mem,0x11000000,0x00004000);
	vtlb_MapHandler(vu1_micro_mem,0x11008000,0x00004000);

	// VU0/VU1 memory (data)
	// VU0 is 4k, mirrored 4 times across a 16k area.
	vtlb_MapBlock(VU0.Mem,0x11004000,0x00004000,0x1000);
	vtlb_MapBlock(VU1.Mem,0x1100c000,0x00004000);
}

void memMapPhy()
{
	// Main memory
	vtlb_MapBlock(eeMem->Main,	0x00000000,Ps2MemSize::MainRam);//mirrored on first 256 mb ?
	// High memory, uninstalled on the configuration we emulate
	vtlb_MapHandler(null_handler, Ps2MemSize::MainRam, 0x10000000 - Ps2MemSize::MainRam);

	// Various ROMs (all read-only)
	vtlb_MapBlock(eeMem->ROM,	0x1fc00000,Ps2MemSize::Rom);
	vtlb_MapBlock(eeMem->ROM1,	0x1e000000,Ps2MemSize::Rom1);
	vtlb_MapBlock(eeMem->ROM2,	0x1e400000,Ps2MemSize::Rom2);
	vtlb_MapBlock(eeMem->EROM,	0x1e040000,Ps2MemSize::ERom);

	// IOP memory
	// (used by the EE Bios Kernel during initial hardware initialization, Apps/Games
	//  are "supposed" to use the thread-safe SIF instead.)
	vtlb_MapBlock(iopMem->Main,0x1c000000,0x00800000);

	// Generic Handlers; These fallback to mem* stuff...
	vtlb_MapHandler(tlb_fallback_7,0x14000000,0x10000);
	vtlb_MapHandler(tlb_fallback_4,0x18000000,0x10000);
	vtlb_MapHandler(tlb_fallback_5,0x1a000000,0x10000);
	vtlb_MapHandler(tlb_fallback_6,0x12000000,0x10000);
	vtlb_MapHandler(tlb_fallback_8,0x1f000000,0x10000);
	vtlb_MapHandler(tlb_fallback_3,0x1f400000,0x10000);
	vtlb_MapHandler(tlb_fallback_2,0x1f800000,0x10000);
	vtlb_MapHandler(tlb_fallback_8,0x1f900000,0x10000);

	// Hardware Register Handlers : specialized/optimized per-page handling of HW register accesses
	// (note that hw_by_page handles are assigned in memReset prior to calling this function)

	for( uint i=0; i<16; ++i)
		vtlb_MapHandler(hw_by_page[i], 0x10000000 + (0x01000 * i), 0x01000);

	vtlb_MapHandler(gs_page_0, 0x12000000, 0x01000);
	vtlb_MapHandler(gs_page_1, 0x12001000, 0x01000);

	// "Secret" IOP HW mappings - Used by EE Bios Kernel during boot and generally
	// left untouched after that, as per EE/IOP thread safety rules.

	vtlb_MapHandler(iopHw_by_page_01, 0x1f801000, 0x01000);
	vtlb_MapHandler(iopHw_by_page_03, 0x1f803000, 0x01000);
	vtlb_MapHandler(iopHw_by_page_08, 0x1f808000, 0x01000);

}

//Why is this required ?
void memMapKernelMem()
{
	//lower 512 mb: direct map
	//vtlb_VMap(0x00000000,0x00000000,0x20000000);
	//0x8* mirror
	vtlb_VMap(0x80000000,0x00000000,0x20000000);
	//0xa* mirror
	vtlb_VMap(0xA0000000,0x00000000,0x20000000);
}

//what do do with these ?
void memMapSupervisorMem()
{
}

void memMapUserMem()
{
}

static mem8_t __fastcall nullRead8(u32 mem) {
	MEM_LOG("Read uninstalled memory at address %08x", mem);
	return 0;
}
static mem16_t __fastcall nullRead16(u32 mem) {
	MEM_LOG("Read uninstalled memory at address %08x", mem);
	return 0;
}
static mem32_t __fastcall nullRead32(u32 mem) {
	MEM_LOG("Read uninstalled memory at address %08x", mem);
	return 0;
}
static void __fastcall nullRead64(u32 mem, mem64_t *out) {
	MEM_LOG("Read uninstalled memory at address %08x", mem);
	*out = 0;
}
static void __fastcall nullRead128(u32 mem, mem128_t *out) {
	MEM_LOG("Read uninstalled memory at address %08x", mem);
	ZeroQWC(out);
}
static void __fastcall nullWrite8(u32 mem, mem8_t value)
{
	MEM_LOG("Write uninstalled memory at address %08x", mem);
}
static void __fastcall nullWrite16(u32 mem, mem16_t value)
{
	MEM_LOG("Write uninstalled memory at address %08x", mem);
}
static void __fastcall nullWrite32(u32 mem, mem32_t value)
{
	MEM_LOG("Write uninstalled memory at address %08x", mem);
}
static void __fastcall nullWrite64(u32 mem, const mem64_t *value)
{
	MEM_LOG("Write uninstalled memory at address %08x", mem);
}
static void __fastcall nullWrite128(u32 mem, const mem128_t *value)
{
	MEM_LOG("Write uninstalled memory at address %08x", mem);
}

template<int p>
static mem8_t __fastcall _ext_memRead8 (u32 mem)
{
	switch (p)
	{
		case 3: // psh4
			return psxHw4Read8(mem);
		case 6: // gsm
			return gsRead8(mem);
		case 7: // dev9
		{
			mem8_t retval = DEV9read8(mem & ~0xa4000000);
			Console.WriteLn("DEV9 read8 %8.8lx: %2.2lx", mem & ~0xa4000000, retval);
			return retval;
		}
	}

	MEM_LOG("Unknown Memory Read8   from address %8.8x", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return 0;
}

template<int p>
static mem16_t __fastcall _ext_memRead16(u32 mem)
{
	switch (p)
	{
		case 4: // b80
			MEM_LOG("b800000 Memory read16 address %x", mem);
			return 0;
		case 5: // ba0
			return ba0R16(mem);
		case 6: // gsm
			return gsRead16(mem);

		case 7: // dev9
		{
			mem16_t retval = DEV9read16(mem & ~0xa4000000);
			Console.WriteLn("DEV9 read16 %8.8lx: %4.4lx", mem & ~0xa4000000, retval);
			return retval;
		}

		case 8: // spu2
			return SPU2read(mem);
	}
	MEM_LOG("Unknown Memory read16  from address %8.8x", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return 0;
}

template<int p>
static mem32_t __fastcall _ext_memRead32(u32 mem)
{
	switch (p)
	{
		case 6: // gsm
			return gsRead32(mem);
		case 7: // dev9
		{
			mem32_t retval = DEV9read32(mem & ~0xa4000000);
			Console.WriteLn("DEV9 read32 %8.8lx: %8.8lx", mem & ~0xa4000000, retval);
			return retval;
		}
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);
	return 0;
}

template<int p>
static void __fastcall _ext_memRead64(u32 mem, mem64_t *out)
{
	switch (p)
	{
		case 6: // gsm
			*out = gsRead64(mem); return;
	}

	MEM_LOG("Unknown Memory read64  from address %8.8x", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
}

template<int p>
static void __fastcall _ext_memRead128(u32 mem, mem128_t *out)
{
	switch (p)
	{
		//case 1: // hwm
		//	hwRead128(mem & ~0xa0000000, out); return;
		case 6: // gsm
			CopyQWC(out,PS2GS_BASE(mem));
		return;
	}

	MEM_LOG("Unknown Memory read128 from address %8.8x", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
}

template<int p>
static void __fastcall _ext_memWrite8 (u32 mem, mem8_t  value)
{
	switch (p) {
		case 3: // psh4
			psxHw4Write8(mem, value); return;
		case 6: // gsm
			gsWrite8(mem, value); return;
		case 7: // dev9
			DEV9write8(mem & ~0xa4000000, value);
			Console.WriteLn("DEV9 write8 %8.8lx: %2.2lx", mem & ~0xa4000000, value);
			return;
	}

	MEM_LOG("Unknown Memory write8   to  address %x with data %2.2x", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

template<int p>
static void __fastcall _ext_memWrite16(u32 mem, mem16_t value)
{
	switch (p) {
		case 5: // ba0
			MEM_LOG("ba00000 Memory write16 to  address %x with data %x", mem, value);
			return;
		case 6: // gsm
			gsWrite16(mem, value); return;
		case 7: // dev9
			DEV9write16(mem & ~0xa4000000, value);
			Console.WriteLn("DEV9 write16 %8.8lx: %4.4lx", mem & ~0xa4000000, value);
			return;
		case 8: // spu2
			SPU2write(mem, value); return;
	}
	MEM_LOG("Unknown Memory write16  to  address %x with data %4.4x", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

template<int p>
static void __fastcall _ext_memWrite32(u32 mem, mem32_t value)
{
	switch (p) {
		case 6: // gsm
			gsWrite32(mem, value); return;
		case 7: // dev9
			DEV9write32(mem & ~0xa4000000, value);
			Console.WriteLn("DEV9 write32 %8.8lx: %8.8lx", mem & ~0xa4000000, value);
			return;
	}
	MEM_LOG("Unknown Memory write32  to  address %x with data %8.8x", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

template<int p>
static void __fastcall _ext_memWrite64(u32 mem, const mem64_t* value)
{

	/*switch (p) {
		//case 1: // hwm
		//	hwWrite64(mem & ~0xa0000000, *value);
		//	return;
		//case 6: // gsm
		//	gsWrite64(mem & ~0xa0000000, *value); return;
	}*/

	MEM_LOG("Unknown Memory write64  to  address %x with data %8.8x_%8.8x", mem, (u32)(*value>>32), (u32)*value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

template<int p>
static void __fastcall _ext_memWrite128(u32 mem, const mem128_t *value)
{
	/*switch (p) {
		//case 1: // hwm
		//	hwWrite128(mem & ~0xa0000000, value);
		//	return;
		//case 6: // gsm
		//	mem &= ~0xa0000000;
		//	gsWrite64(mem,   value[0]);
		//	gsWrite64(mem+8, value[1]); return;
	}*/

	MEM_LOG("Unknown Memory write128 to  address %x with data %8.8x_%8.8x_%8.8x_%8.8x", mem, ((u32*)value)[3], ((u32*)value)[2], ((u32*)value)[1], ((u32*)value)[0]);
	cpuTlbMissW(mem, cpuRegs.branch);
}

#define vtlb_RegisterHandlerTempl1(nam,t) vtlb_RegisterHandler(nam##Read8<t>,nam##Read16<t>,nam##Read32<t>,nam##Read64<t>,nam##Read128<t>, \
															   nam##Write8<t>,nam##Write16<t>,nam##Write32<t>,nam##Write64<t>,nam##Write128<t>)

typedef void __fastcall ClearFunc_t( u32 addr, u32 qwc );

template<int vunum>
static __fi void ClearVuFunc( u32 addr, u32 size )
{
	if( vunum==0 )
		CpuVU0->Clear(addr,size);
	else
		CpuVU1->Clear(addr,size);
}

template<int vunum>
static mem8_t __fastcall vuMicroRead8(u32 addr)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	return vu->Micro[addr];
}

template<int vunum>
static mem16_t __fastcall vuMicroRead16(u32 addr)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	return *(u16*)&vu->Micro[addr];
}

template<int vunum>
static mem32_t __fastcall vuMicroRead32(u32 addr)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	return *(u32*)&vu->Micro[addr];
}

template<int vunum>
static void __fastcall vuMicroRead64(u32 addr,mem64_t* data)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	*data=*(u64*)&vu->Micro[addr];
}

template<int vunum>
static void __fastcall vuMicroRead128(u32 addr,mem128_t* data)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	CopyQWC(data,&vu->Micro[addr]);
}

// Profiled VU writes: Happen very infrequently, with exception of BIOS initialization (at most twice per
//   frame in-game, and usually none at all after BIOS), so cpu clears aren't much of a big deal.

template<int vunum>
static void __fastcall vuMicroWrite8(u32 addr,mem8_t data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (vu.Micro[addr]!=data)
	{
		ClearVuFunc<vunum>(addr&(~7), 8); // Clear before writing new data (clearing 8 bytes because an instruction is 8 bytes) (cottonvibes)
		vu.Micro[addr]=data;
	}
}

template<int vunum>
static void __fastcall vuMicroWrite16(u32 addr,mem16_t data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (*(u16*)&vu.Micro[addr]!=data)
	{
		ClearVuFunc<vunum>(addr&(~7), 8);
		*(u16*)&vu.Micro[addr]=data;
	}
}

template<int vunum>
static void __fastcall vuMicroWrite32(u32 addr,mem32_t data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (*(u32*)&vu.Micro[addr]!=data)
	{
		ClearVuFunc<vunum>(addr&(~7), 8);
		*(u32*)&vu.Micro[addr]=data;
	}
}

template<int vunum>
static void __fastcall vuMicroWrite64(u32 addr,const mem64_t* data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (*(u64*)&vu.Micro[addr]!=data[0])
	{
		ClearVuFunc<vunum>(addr&(~7), 8);
		*(u64*)&vu.Micro[addr]=data[0];
	}
}

template<int vunum>
static void __fastcall vuMicroWrite128(u32 addr,const mem128_t* data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if ((u128&)vu.Micro[addr] != *data)
	{
		ClearVuFunc<vunum>(addr&(~7), 16);
		CopyQWC(&vu.Micro[addr],data);
	}
}

void memSetPageAddr(u32 vaddr, u32 paddr)
{
	//Console.WriteLn("memSetPageAddr: %8.8x -> %8.8x", vaddr, paddr);

	vtlb_VMap(vaddr,paddr,0x1000);

}

void memClearPageAddr(u32 vaddr)
{
	//Console.WriteLn("memClearPageAddr: %8.8x", vaddr);

	vtlb_VMapUnmap(vaddr,0x1000); // -> whut ?

#ifdef FULLTLB
//	memLUTRK[vaddr >> 12] = 0;
//	memLUTWK[vaddr >> 12] = 0;
#endif
}

///////////////////////////////////////////////////////////////////////////
// PS2 Memory Init / Reset / Shutdown

class mmap_PageFaultHandler : public EventListener_PageFault
{
protected:
	void OnPageFaultEvent( const PageFaultInfo& info, bool& handled );
};

static mmap_PageFaultHandler* mmap_faultHandler = NULL;

EEVM_MemoryAllocMess* eeMem = NULL;

__pagealigned u8 eeHw[Ps2MemSize::Hardware];

void memAlloc()
{
	if( eeMem == NULL )
		eeMem = (EEVM_MemoryAllocMess*)vtlb_malloc( sizeof(*eeMem), 4096 );

	if( eeMem == NULL)
		throw Exception::OutOfMemory( L"memAlloc > failed to allocate PS2's base ram/rom/scratchpad." );

	pxAssume(Source_PageFault);
	mmap_faultHandler = new mmap_PageFaultHandler();
}

void memShutdown()
{
	safe_delete(mmap_faultHandler);

	vtlb_free( eeMem, sizeof(*eeMem) );
	eeMem = NULL;
	vtlb_Term();
}

void memBindConditionalHandlers()
{
	if( hw_by_page[0xf] == -1 ) return;

	if (EmuConfig.Speedhacks.IntcStat)
	{
		vtlbMemR16FP* page0F16(hwRead16_page_0F_INTC_HACK);
		vtlbMemR32FP* page0F32(hwRead32_page_0F_INTC_HACK);
		//vtlbMemR64FP* page0F64(hwRead64_generic_INTC_HACK);

		vtlb_ReassignHandler( hw_by_page[0xf],
			hwRead8<0x0f>,	page0F16,			page0F32,			hwRead64<0x0f>,		hwRead128<0x0f>,
			hwWrite8<0x0f>,	hwWrite16<0x0f>,	hwWrite32<0x0f>,	hwWrite64<0x0f>,	hwWrite128<0x0f>
		);
	}
	else
	{
		vtlbMemR16FP* page0F16(hwRead16<0x0f>);
		vtlbMemR32FP* page0F32(hwRead32<0x0f>);
		//vtlbMemR64FP* page0F64(hwRead64<0x0f>);

		vtlb_ReassignHandler( hw_by_page[0xf],
			hwRead8<0x0f>,	page0F16,			page0F32,			hwRead64<0x0f>,		hwRead128<0x0f>,
			hwWrite8<0x0f>,	hwWrite16<0x0f>,	hwWrite32<0x0f>,	hwWrite64<0x0f>,	hwWrite128<0x0f>
		);
	}
}

// Resets memory mappings, unmaps TLBs, reloads bios roms, etc.
void memReset()
{
	// VTLB Protection Preparations.
	//HostSys::MemProtect( m_psAllMem, m_allMemSize, Protect_ReadWrite );

	// Note!!  Ideally the vtlb should only be initialized once, and then subsequent
	// resets of the system hardware would only clear vtlb mappings, but since the
	// rest of the emu is not really set up to support a "soft" reset of that sort
	// we opt for the hard/safe version.

	pxAssume( eeMem != NULL );
	memzero( *eeMem );

#ifdef ENABLECACHE
	memset(pCache,0,sizeof(_cacheS)*64);
#endif

	vtlb_Init();

	null_handler = vtlb_RegisterHandler(nullRead8, nullRead16, nullRead32, nullRead64, nullRead128,
		nullWrite8, nullWrite16, nullWrite32, nullWrite64, nullWrite128);

	tlb_fallback_0 = vtlb_RegisterHandlerTempl1(_ext_mem,0);
	tlb_fallback_3 = vtlb_RegisterHandlerTempl1(_ext_mem,3);
	tlb_fallback_4 = vtlb_RegisterHandlerTempl1(_ext_mem,4);
	tlb_fallback_5 = vtlb_RegisterHandlerTempl1(_ext_mem,5);
	tlb_fallback_7 = vtlb_RegisterHandlerTempl1(_ext_mem,7);
	tlb_fallback_8 = vtlb_RegisterHandlerTempl1(_ext_mem,8);

	// Dynarec versions of VUs
	vu0_micro_mem = vtlb_RegisterHandlerTempl1(vuMicro,0);
	vu1_micro_mem = vtlb_RegisterHandlerTempl1(vuMicro,1);

	//////////////////////////////////////////////////////////////////////////////////////////
	// IOP's "secret" Hardware Register mapping, accessible from the EE (and meant for use
	// by debugging or BIOS only).  The IOP's hw regs are divided into three main pages in
	// the 0x1f80 segment, and then another oddball page for CDVD in the 0x1f40 segment.
	//

	using namespace IopMemory;

	tlb_fallback_2 = vtlb_RegisterHandler(
		iopHwRead8_generic, iopHwRead16_generic, iopHwRead32_generic, _ext_memRead64<2>, _ext_memRead128<2>,
		iopHwWrite8_generic, iopHwWrite16_generic, iopHwWrite32_generic, _ext_memWrite64<2>, _ext_memWrite128<2>
	);

	iopHw_by_page_01 = vtlb_RegisterHandler(
		iopHwRead8_Page1, iopHwRead16_Page1, iopHwRead32_Page1, _ext_memRead64<2>, _ext_memRead128<2>,
		iopHwWrite8_Page1, iopHwWrite16_Page1, iopHwWrite32_Page1, _ext_memWrite64<2>, _ext_memWrite128<2>
	);

	iopHw_by_page_03 = vtlb_RegisterHandler(
		iopHwRead8_Page3, iopHwRead16_Page3, iopHwRead32_Page3, _ext_memRead64<2>, _ext_memRead128<2>,
		iopHwWrite8_Page3, iopHwWrite16_Page3, iopHwWrite32_Page3, _ext_memWrite64<2>, _ext_memWrite128<2>
	);

	iopHw_by_page_08 = vtlb_RegisterHandler(
		iopHwRead8_Page8, iopHwRead16_Page8, iopHwRead32_Page8, _ext_memRead64<2>, _ext_memRead128<2>,
		iopHwWrite8_Page8, iopHwWrite16_Page8, iopHwWrite32_Page8, _ext_memWrite64<2>, _ext_memWrite128<2>
	);


	// psHw Optimized Mappings
	// The HW Registers have been split into pages to improve optimization.
	
#define hwHandlerTmpl(page) \
	hwRead8<page>,	hwRead16<page>,	hwRead32<page>,	hwRead64<page>,	hwRead128<page>, \
	hwWrite8<page>,	hwWrite16<page>,hwWrite32<page>,hwWrite64<page>,hwWrite128<page>

	hw_by_page[0x0] = vtlb_RegisterHandler( hwHandlerTmpl(0x00) );
	hw_by_page[0x1] = vtlb_RegisterHandler( hwHandlerTmpl(0x01) );
	hw_by_page[0x2] = vtlb_RegisterHandler( hwHandlerTmpl(0x02) );
	hw_by_page[0x3] = vtlb_RegisterHandler( hwHandlerTmpl(0x03) );
	hw_by_page[0x4] = vtlb_RegisterHandler( hwHandlerTmpl(0x04) );
	hw_by_page[0x5] = vtlb_RegisterHandler( hwHandlerTmpl(0x05) );
	hw_by_page[0x6] = vtlb_RegisterHandler( hwHandlerTmpl(0x06) );
	hw_by_page[0x7] = vtlb_RegisterHandler( hwHandlerTmpl(0x07) );
	hw_by_page[0x8] = vtlb_RegisterHandler( hwHandlerTmpl(0x08) );
	hw_by_page[0x9] = vtlb_RegisterHandler( hwHandlerTmpl(0x09) );
	hw_by_page[0xa] = vtlb_RegisterHandler( hwHandlerTmpl(0x0a) );
	hw_by_page[0xb] = vtlb_RegisterHandler( hwHandlerTmpl(0x0b) );
	hw_by_page[0xc] = vtlb_RegisterHandler( hwHandlerTmpl(0x0c) );
	hw_by_page[0xd] = vtlb_RegisterHandler( hwHandlerTmpl(0x0d) );
	hw_by_page[0xe] = vtlb_RegisterHandler( hwHandlerTmpl(0x0e) );
	hw_by_page[0xf] = vtlb_NewHandler();		// redefined later based on speedhacking prefs
	memBindConditionalHandlers();

	//////////////////////////////////////////////////////////////////////
	// GS Optimized Mappings

	tlb_fallback_6 = vtlb_RegisterHandler(
		_ext_memRead8<6>, _ext_memRead16<6>, _ext_memRead32<6>, _ext_memRead64<6>, _ext_memRead128<6>,
		_ext_memWrite8<6>, _ext_memWrite16<6>, _ext_memWrite32<6>, gsWrite64_generic, gsWrite128_generic
	);

	gs_page_0 = vtlb_RegisterHandler(
		_ext_memRead8<6>, _ext_memRead16<6>, _ext_memRead32<6>, _ext_memRead64<6>, _ext_memRead128<6>,
		_ext_memWrite8<6>, _ext_memWrite16<6>, _ext_memWrite32<6>, gsWrite64_page_00, gsWrite128_page_00
	);

	gs_page_1 = vtlb_RegisterHandler(
		_ext_memRead8<6>, _ext_memRead16<6>, _ext_memRead32<6>, _ext_memRead64<6>, _ext_memRead128<6>,
		_ext_memWrite8<6>, _ext_memWrite16<6>, _ext_memWrite32<6>, gsWrite64_page_01, gsWrite128_page_01
	);

	//vtlb_Reset();

	// reset memLUT (?)
	//vtlb_VMap(0x00000000,0x00000000,0x20000000);
	//vtlb_VMapUnmap(0x20000000,0x60000000);

	memMapPhy();
	memMapVUmicro();
	memMapKernelMem();
	memMapSupervisorMem();
	memMapUserMem();
	memSetKernelMode();

	vtlb_VMap(0x00000000,0x00000000,0x20000000);
	vtlb_VMapUnmap(0x20000000,0x60000000);

	LoadBIOS();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Memory Protection and Block Checking, vtlb Style!
//
// For the first time code is recompiled (executed), the PS2 ram page for that code is
// protected using Virtual Memory (mprotect).  If the game modifies its own code then this
// protection causes an *exception* to be raised (signal in Linux), which is handled by
// unprotecting the page and switching the recompiled block to "manual" protection.
//
// Manual protection uses a simple brute-force memcmp of the recompiled code to the code
// currently in RAM for *each time* the block is executed.  Fool-proof, but slow, which
// is why we default to using the exception-based protection scheme described above.
//
// Why manual blocks?  Because many games contain code and data in the same 4k page, so
// we *cannot* automatically recompile and reprotect pages, lest we end up recompiling and
// reprotecting them constantly (Which would be very slow).  As a counter, the R5900 side
// of the block checking code does try to periodically re-protect blocks [going from manual
// back to protected], so that blocks which underwent a single invalidation don't need to
// incur a permanent performance penalty.
//
// Page Granularity:
// Fortunately for us MIPS and x86 use the same page granularity for TLB and memory
// protection, so we can use a 1:1 correspondence when protecting pages.  Page granularity
// is 4096 (4k), which is why you'll see a lot of 0xfff's, >><< 12's, and 0x1000's in the
// code below.
//

enum vtlb_ProtectionMode
{
	ProtMode_None = 0,		// page is 'unaccounted' -- neither protected nor unprotected
	ProtMode_Write,			// page is under write protection (exception handler)
	ProtMode_Manual			// page is under manual protection (self-checked at execution)
};

struct vtlb_PageProtectionInfo
{
	// Ram De-mapping -- used to convert fully translated/mapped offsets (which reside with
	// in the eeMem->Main block) back into their originating ps2 physical ram address.
	// Values are assigned when pages are marked for protection.  since pages are automatically
	// cleared and reset when TLB-remapped, stale values in this table (due to on-the-fly TLB
	// changes) will be re-assigned the next time the page is accessed.
	u32 ReverseRamMap;

	vtlb_ProtectionMode Mode;
};

static __aligned16 vtlb_PageProtectionInfo m_PageProtectInfo[Ps2MemSize::MainRam >> 12];


// returns:
//  -1 - unchecked block (resides in ROM, thus is integrity is constant)
//   0 - page is using Write protection
//   1 - page is using manual protection (recompiler must include execution-time
//       self-checking of block integrity)
//
int mmap_GetRamPageInfo( u32 paddr )
{
	paddr &= ~0xfff;

	uptr ptr = (uptr)PSM( paddr );
	uptr rampage = ptr - (uptr)eeMem->Main;

	if (rampage >= Ps2MemSize::MainRam)
		return -1; //not in ram, no tracking done ...

	rampage >>= 12;
	return ( m_PageProtectInfo[rampage].Mode == ProtMode_Manual ) ? 1 : 0;
}

// paddr - physically mapped PS2 address
void mmap_MarkCountedRamPage( u32 paddr )
{
	paddr &= ~0xfff;

	uptr ptr = (uptr)PSM( paddr );
	int rampage = (ptr - (uptr)eeMem->Main) >> 12;

	// Important: Update the ReverseRamMap here because TLB changes could alter the paddr
	// mapping into eeMem->Main.

	m_PageProtectInfo[rampage].ReverseRamMap = paddr;

	if( m_PageProtectInfo[rampage].Mode == ProtMode_Write )
		return;		// skip town if we're already protected.

	eeRecPerfLog.Write( (m_PageProtectInfo[rampage].Mode == ProtMode_Manual) ?
		"Re-protecting page @ 0x%05x" : "Protected page @ 0x%05x",
		paddr>>12
	);

	m_PageProtectInfo[rampage].Mode = ProtMode_Write;
	HostSys::MemProtect( &eeMem->Main[rampage<<12], __pagesize, PageAccess_ReadOnly() );
}

// offset - offset of address relative to psM.
// All recompiled blocks belonging to the page are cleared, and any new blocks recompiled
// from code residing in this page will use manual protection.
static __fi void mmap_ClearCpuBlock( uint offset )
{
	int rampage = offset >> 12;

	// Assertion: This function should never be run on a block that's already under
	// manual protection.  Indicates a logic error in the recompiler or protection code.
	pxAssertMsg( m_PageProtectInfo[rampage].Mode != ProtMode_Manual,
		"Attempted to clear a block that is already under manual protection." );

	HostSys::MemProtect( &eeMem->Main[rampage<<12], __pagesize, PageAccess_ReadWrite() );
	m_PageProtectInfo[rampage].Mode = ProtMode_Manual;
	Cpu->Clear( m_PageProtectInfo[rampage].ReverseRamMap, 0x400 );
}

void mmap_PageFaultHandler::OnPageFaultEvent( const PageFaultInfo& info, bool& handled )
{
	// get bad virtual address
	uptr offset = info.addr - (uptr)eeMem->Main;
	if( offset >= Ps2MemSize::MainRam ) return;

	mmap_ClearCpuBlock( offset );
	handled = true;
}

// Clears all block tracking statuses, manual protection flags, and write protection.
// This does not clear any recompiler blocks.  It is assumed (and necessary) for the caller
// to ensure the EErec is also reset in conjunction with calling this function.
//  (this function is called by default from the eerecReset).
void mmap_ResetBlockTracking()
{
	//DbgCon.WriteLn( "vtlb/mmap: Block Tracking reset..." );
	memzero( m_PageProtectInfo );
	HostSys::MemProtect( eeMem->Main, Ps2MemSize::MainRam, PageAccess_ReadWrite() );
}
