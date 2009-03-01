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

RAM
---
0x00100000-0x01ffffff this is the physical address for the ram.its cached there
0x20100000-0x21ffffff uncached 
0x30100000-0x31ffffff uncached & acceleretade 
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

#pragma warning(disable:4799) // No EMMS at end of function

#include <vector>

#include "Common.h"
#include "iR5900.h"

#include "PsxCommon.h"
#include "VUmicro.h"
#include "GS.h"
#include "vtlb.h"
#include "IPU/IPU.h"


#ifdef ENABLECACHE
#include "Cache.h"
#endif

#ifdef __LINUX__
#include <sys/mman.h>
#endif

//#define FULLTLB
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
	//MEM_LOG("ba00000 Memory read16 address %x\n", mem);

	if (mem == 0x1a000006) {
		static int ba6;
		ba6++;
		if (ba6 == 3) ba6 = 0;
		return ba6;
	}
	return 0;
}


// Attempts to load a BIOS rom file, by trying multiple combinations of base filename
// and extension.  The bios specified in Config.Bios is used as the base.
void loadBiosRom( const char *ext, u8 *dest, long maxSize )
{
	string Bios1;
	string Bios;
	long filesize;

	Path::Combine( Bios, Config.BiosDir, Config.Bios );

	// Try first a basic extension concatenation (normally results in something like name.bin.rom1)
	ssprintf(Bios1, "%hs.%s", &Bios, ext);
	if( (filesize=Path::getFileSize( Bios1 ) ) <= 0 )
	{
		// Try the name properly extensioned next (name.rom1)
		Path::ReplaceExtension( Bios1, Bios, ext );
		if( (filesize=Path::getFileSize( Bios1 ) ) <= 0 )
		{
			// Try for the old-style method (rom1.bin)
			Path::Combine( Bios1, Config.BiosDir, ext );
			Bios1 += ".bin";
			if( (filesize=Path::getFileSize( Bios1 ) ) <= 0 )
			{
				Console::Error( "\n\n\n"
					"**************\n"
					"%s NOT FOUND\n"
					"**************\n\n\n", params ext
					);
				return;
			}
		}
	}

	// if we made it this far, we have a successful file found:

	FILE *fp = fopen(Bios1.c_str(), "rb");
	fread(dest, 1, std::min( maxSize, filesize ), fp);
	fclose(fp);
}

u32 psMPWC[(Ps2MemSize::Base/32)>>12];
std::vector<u32> psMPWVA[Ps2MemSize::Base>>12];

u8  *psM = NULL; //32mb Main Ram
u8  *psR = NULL; //4mb rom area
u8  *psR1 = NULL; //256kb rom1 area (actually 196kb, but can't mask this)
u8  *psR2 = NULL; // 0x00080000
u8  *psER = NULL; // 0x001C0000
u8  *psS = NULL; //0.015 mb, scratch pad

#define CHECK_MEM(mem) //MyMemCheck(mem)

void MyMemCheck(u32 mem)
{
    if( mem == 0x1c02f2a0 )
        SysPrintf("yo\n");
}

/////////////////////////////
// REGULAR MEM START 
/////////////////////////////
vtlbHandler tlb_fallback_0;
vtlbHandler tlb_fallback_1;
vtlbHandler tlb_fallback_2;
vtlbHandler tlb_fallback_3;
vtlbHandler tlb_fallback_4;
vtlbHandler tlb_fallback_5;
vtlbHandler tlb_fallback_6;
vtlbHandler tlb_fallback_7;
vtlbHandler tlb_fallback_8;

vtlbHandler vu0_micro_mem[2];		// 0 - dynarec, 1 - interpreter
vtlbHandler vu1_micro_mem[2];		// 0 - dynarec, 1 - interpreter

vtlbHandler hw_by_page[0x10];
vtlbHandler gs_page_0;
vtlbHandler gs_page_1;


// Used to remap the VUmicro memory according to the VU0/VU1 dynarec setting.
// (the VU memory operations are different for recs vs. interpreters)
void memMapVUmicro()
{
	vtlb_MapHandler(vu0_micro_mem[CHECK_VU0REC ? 0 : 1],0x11000000,0x00004000);
	vtlb_MapHandler(vu1_micro_mem[CHECK_VU1REC ? 0 : 1],0x11008000,0x00004000);

	vtlb_MapBlock(VU0.Mem,0x11004000,0x00004000,0x1000);
	vtlb_MapBlock(VU1.Mem,0x1100c000,0x00004000);
}

void memMapPhy()
{
	//Main mem
	vtlb_MapBlock(psM,0x00000000,Ps2MemSize::Base);//mirrored on first 256 mb ?
	
	//Rom
	vtlb_MapBlock(psR,0x1fc00000,Ps2MemSize::Rom);//Writable ?
	//Rom 1
	vtlb_MapBlock(psR1,0x1e000000,Ps2MemSize::Rom1);//Writable ?
	//Rom 2 ?
	vtlb_MapBlock(psR2,0x1e400000,Ps2MemSize::Rom2);//Writable ?
	//EEProm ?
	vtlb_MapBlock(psER,0x1e040000,Ps2MemSize::ERom);//Writable ?

	//IOP mem
	vtlb_MapBlock(psxM,0x1c000000,0x00800000);

	//These fallback to mem* stuff ...
	vtlb_MapHandler(tlb_fallback_1,0x10000000,0x10000);
	vtlb_MapHandler(tlb_fallback_7,0x14000000,0x10000);
	vtlb_MapHandler(tlb_fallback_4,0x18000000,0x10000);
	vtlb_MapHandler(tlb_fallback_5,0x1a000000,0x10000);
	vtlb_MapHandler(tlb_fallback_6,0x12000000,0x10000);
	vtlb_MapHandler(tlb_fallback_8,0x1f000000,0x10000);
	vtlb_MapHandler(tlb_fallback_3,0x1f400000,0x10000);
	vtlb_MapHandler(tlb_fallback_2,0x1f800000,0x10000);
	vtlb_MapHandler(tlb_fallback_8,0x1f900000,0x10000);

#ifdef PCSX2_DEVBUILD
	// Bind fallback handlers used for logging purposes only.
	// In release mode the Vtlb will map these addresses directly instead of using
	// the read/write handlers (which just issue logs and do normal memOps)
#endif

	// map specific optimized page handlers for HW accesses
	vtlb_MapHandler(hw_by_page[0x0], 0x10000000, 0x01000);
	vtlb_MapHandler(hw_by_page[0x1], 0x10001000, 0x01000);
	vtlb_MapHandler(hw_by_page[0x2], 0x10002000, 0x01000);
	vtlb_MapHandler(hw_by_page[0x3], 0x10003000, 0x01000);
	vtlb_MapHandler(hw_by_page[0x4], 0x10004000, 0x01000);
	vtlb_MapHandler(hw_by_page[0x5], 0x10005000, 0x01000);
	vtlb_MapHandler(hw_by_page[0x6], 0x10006000, 0x01000);
	vtlb_MapHandler(hw_by_page[0x7], 0x10007000, 0x01000);
	vtlb_MapHandler(hw_by_page[0xb], 0x1000b000, 0x01000);
	vtlb_MapHandler(hw_by_page[0xe], 0x1000e000, 0x01000);
	vtlb_MapHandler(hw_by_page[0xf], 0x1000f000, 0x01000);

	vtlb_MapHandler(gs_page_0, 0x12000000, 0x01000);
	vtlb_MapHandler(gs_page_1, 0x12001000, 0x01000);
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

template<int p>
mem8_t __fastcall _ext_memRead8 (u32 mem) 
{
	switch (p) 
	{
		case 1: // hwm
			return hwRead8(mem);
		case 2: // psh
			return psxHwRead8(mem);
		case 3: // psh4
			return psxHw4Read8(mem);
		case 6: // gsm
			return gsRead8(mem);
		case 7: // dev9
		{
			mem8_t retval = DEV9read8(mem & ~0xa4000000);
			SysPrintf("DEV9 read8 %8.8lx: %2.2lx\n", mem & ~0xa4000000, retval);
			return retval;
		}
	}

	MEM_LOG("Unknown Memory Read8   from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return 0;
}

template<int p>
mem16_t __fastcall _ext_memRead16(u32 mem)
{
	switch (p)
	{
		case 1: // hwm
			return hwRead16(mem);
		case 2: // psh
			return psxHwRead16(mem);
		case 4: // b80
			MEM_LOG("b800000 Memory read16 address %x\n", mem);
			return 0;
		case 5: // ba0
			return ba0R16(mem);
		case 6: // gsm
			return gsRead16(mem);

		case 7: // dev9
		{
			mem16_t retval = DEV9read16(mem & ~0xa4000000);
			SysPrintf("DEV9 read16 %8.8lx: %4.4lx\n", mem & ~0xa4000000, retval);
			return retval;
		}

		case 8: // spu2
			return SPU2read(mem);
	}
	MEM_LOG("Unknown Memory read16  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
	return 0;
}

template<int p>
mem32_t __fastcall _ext_memRead32(u32 mem) 
{
	switch (p)
	{
		case 2: // psh
			return psxHwRead32(mem);
		case 6: // gsm
			return gsRead32(mem);
		case 7: // dev9
		{
			mem32_t retval = DEV9read32(mem & ~0xa4000000);
			SysPrintf("DEV9 read32 %8.8lx: %8.8lx\n", mem & ~0xa4000000, retval);
			return retval;
		}
	}

	MEM_LOG("Unknown Memory read32  from address %8.8x (Status=%8.8x)\n", mem, cpuRegs.CP0.n.Status.val);
	cpuTlbMissR(mem, cpuRegs.branch);
	return 0;
}

template<int p>
void __fastcall _ext_memRead64(u32 mem, mem64_t *out)
{
	switch (p)
	{
		case 6: // gsm
			*out = gsRead64(mem); return;
	}

	MEM_LOG("Unknown Memory read64  from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
}

template<int p>
void __fastcall _ext_memRead128(u32 mem, mem128_t *out)
{
	switch (p)
	{
		//case 1: // hwm
		//	hwRead128(mem & ~0xa0000000, out); return;
		case 6: // gsm
			out[0] = gsRead64(mem  );
			out[1] = gsRead64(mem+8); return;
	}

	MEM_LOG("Unknown Memory read128 from address %8.8x\n", mem);
	cpuTlbMissR(mem, cpuRegs.branch);
}

template<int p>
void __fastcall _ext_memWrite8 (u32 mem, u8  value)
{
	switch (p) {
		case 1: // hwm
			hwWrite8(mem, value);
			return;
		case 2: // psh
			psxHwWrite8(mem, value); return;
		case 3: // psh4
			psxHw4Write8(mem, value); return;
		case 6: // gsm
			gsWrite8(mem, value); return;
		case 7: // dev9
			DEV9write8(mem & ~0xa4000000, value);
			SysPrintf("DEV9 write8 %8.8lx: %2.2lx\n", mem & ~0xa4000000, value);
			return;
	}

	MEM_LOG("Unknown Memory write8   to  address %x with data %2.2x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}
template<int p>
void __fastcall _ext_memWrite16(u32 mem, u16 value)
{
	switch (p) {
		case 1: // hwm
			hwWrite16(mem, value);
			return;
		case 2: // psh
			psxHwWrite16(mem, value); return;
		case 5: // ba0
			MEM_LOG("ba00000 Memory write16 to  address %x with data %x\n", mem, value);
			return;
		case 6: // gsm
			gsWrite16(mem, value); return;
		case 7: // dev9
			DEV9write16(mem & ~0xa4000000, value);
			SysPrintf("DEV9 write16 %8.8lx: %4.4lx\n", mem & ~0xa4000000, value);
			return;
		case 8: // spu2
			SPU2write(mem, value); return;
	}
	MEM_LOG("Unknown Memory write16  to  address %x with data %4.4x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

template<int p>
void __fastcall _ext_memWrite32(u32 mem, u32 value)
{
	switch (p) {
		case 2: // psh
			psxHwWrite32(mem, value); return;
		case 6: // gsm
			gsWrite32(mem, value); return;
		case 7: // dev9
			DEV9write32(mem & ~0xa4000000, value);
			SysPrintf("DEV9 write32 %8.8lx: %8.8lx\n", mem & ~0xa4000000, value);
			return;
	}
	MEM_LOG("Unknown Memory write32  to  address %x with data %8.8x\n", mem, value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

template<int p>
void __fastcall _ext_memWrite64(u32 mem, const u64* value)
{

	/*switch (p) {
		//case 1: // hwm
		//	hwWrite64(mem & ~0xa0000000, *value);
		//	return;
		//case 6: // gsm
		//	gsWrite64(mem & ~0xa0000000, *value); return;
	}*/

	MEM_LOG("Unknown Memory write64  to  address %x with data %8.8x_%8.8x\n", mem, (u32)(*value>>32), (u32)*value);
	cpuTlbMissW(mem, cpuRegs.branch);
}

template<int p>
void __fastcall _ext_memWrite128(u32 mem, const u64 *value)
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

	MEM_LOG("Unknown Memory write128 to  address %x with data %8.8x_%8.8x_%8.8x_%8.8x\n", mem, ((u32*)value)[3], ((u32*)value)[2], ((u32*)value)[1], ((u32*)value)[0]);
	cpuTlbMissW(mem, cpuRegs.branch);
}

#define vtlb_RegisterHandlerTempl1(nam,t) vtlb_RegisterHandler(nam##Read8<t>,nam##Read16<t>,nam##Read32<t>,nam##Read64<t>,nam##Read128<t>, \
															   nam##Write8<t>,nam##Write16<t>,nam##Write32<t>,nam##Write64<t>,nam##Write128<t>)

#define vtlb_RegisterHandlerTempl2(nam,t,rec) vtlb_RegisterHandler(nam##Read8<t>,nam##Read16<t>,nam##Read32<t>,nam##Read64<t>,nam##Read128<t>, \
																   nam##Write8<t,rec>,nam##Write16<t,rec>,nam##Write32<t,rec>,nam##Write64<t,rec>,nam##Write128<t,rec>)

typedef void __fastcall ClearFunc_t( u32 addr, u32 qwc );

template<int vunum, bool dynarec>
static __forceinline void ClearVuFunc( u32 addr, u32 size )
{
	if( dynarec )
	{
		if( vunum==0 )
			VU0micro::recClear(addr,size);
		else
			VU1micro::recClear(addr,size);
	}
	else
	{
		if( vunum==0 )
			VU0micro::intClear(addr,size);
		else
			VU1micro::intClear(addr,size);
	}
}

template<int vunum>
mem8_t __fastcall vuMicroRead8(u32 addr)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	return vu->Micro[addr];
}

template<int vunum>
mem16_t __fastcall vuMicroRead16(u32 addr)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	return *(u16*)&vu->Micro[addr];
}

template<int vunum>
mem32_t __fastcall vuMicroRead32(u32 addr)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	return *(u32*)&vu->Micro[addr];
}

template<int vunum>
void __fastcall vuMicroRead64(u32 addr,mem64_t* data)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	*data=*(u64*)&vu->Micro[addr];
}

template<int vunum>
void __fastcall vuMicroRead128(u32 addr,mem128_t* data)
{
	addr&=(vunum==0)?0xfff:0x3fff;
	VURegs* vu=(vunum==0)?&VU0:&VU1;

	data[0]=*(u64*)&vu->Micro[addr];
	data[1]=*(u64*)&vu->Micro[addr+8];
}

// [TODO] : Profile this code and see how often the VUs get written, and how
// often it changes the values being written (invoking a cpuClear).

template<int vunum, bool dynrec>
void __fastcall vuMicroWrite8(u32 addr,mem8_t data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (vu.Micro[addr]!=data)
	{
		ClearVuFunc<vunum, dynrec>(addr&(~7), 8); // Clear before writing new data (clearing 8 bytes because an instruction is 8 bytes) (cottonvibes)
		vu.Micro[addr]=data;
	}
}

template<int vunum, bool dynrec>
void __fastcall vuMicroWrite16(u32 addr,mem16_t data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (*(u16*)&vu.Micro[addr]!=data)
	{
		ClearVuFunc<vunum, dynrec>(addr&(~7), 8);
		*(u16*)&vu.Micro[addr]=data;
	}
}

template<int vunum, bool dynrec>
void __fastcall vuMicroWrite32(u32 addr,mem32_t data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (*(u32*)&vu.Micro[addr]!=data)
	{
		ClearVuFunc<vunum, dynrec>(addr&(~7), 8);
		*(u32*)&vu.Micro[addr]=data;
	}
}

template<int vunum, bool dynrec>
void __fastcall vuMicroWrite64(u32 addr,const mem64_t* data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (*(u64*)&vu.Micro[addr]!=data[0])
	{
		ClearVuFunc<vunum, dynrec>(addr&(~7), 8);
		*(u64*)&vu.Micro[addr]=data[0];
	}
}

template<int vunum, bool dynrec>
void __fastcall vuMicroWrite128(u32 addr,const mem128_t* data)
{
	addr &= (vunum==0) ? 0xfff : 0x3fff;
	VURegs& vu = (vunum==0) ? VU0 : VU1;

	if (*(u64*)&vu.Micro[addr]!=data[0] || *(u64*)&vu.Micro[addr+8]!=data[1])
	{
		ClearVuFunc<vunum, dynrec>(addr&(~7), 16);
		*(u64*)&vu.Micro[addr]=data[0];
		*(u64*)&vu.Micro[addr+8]=data[1];
	}
}

void memSetPageAddr(u32 vaddr, u32 paddr) 
{
	//SysPrintf("memSetPageAddr: %8.8x -> %8.8x\n", vaddr, paddr);

	vtlb_VMap(vaddr,paddr,0x1000);

}

void memClearPageAddr(u32 vaddr) 
{
	//SysPrintf("memClearPageAddr: %8.8x\n", vaddr);

	vtlb_VMapUnmap(vaddr,0x1000); // -> whut ?

#ifdef FULLTLB
	memLUTRK[vaddr >> 12] = 0;
	memLUTWK[vaddr >> 12] = 0;
#endif
}

///////////////////////////////////////////////////////////////////////////
// PS2 Memory Init / Reset / Shutdown

static const uint m_allMemSize = 
		Ps2MemSize::Rom + Ps2MemSize::Rom1 + Ps2MemSize::Rom2 + Ps2MemSize::ERom +
		Ps2MemSize::Base + Ps2MemSize::Hardware + Ps2MemSize::Scratch;

static u8* m_psAllMem = NULL;

void memAlloc() 
{
	if( m_psAllMem == NULL )
		m_psAllMem = vtlb_malloc( m_allMemSize, 4096, 0x2400000 );

	if( m_psAllMem == NULL)
		throw Exception::OutOfMemory( "memAlloc > failed to allocate PS2's base ram/rom/scratchpad." );

	u8* curpos = m_psAllMem;
	psM = curpos; curpos += Ps2MemSize::Base;
	psR = curpos; curpos += Ps2MemSize::Rom; 
	psR1 = curpos; curpos += Ps2MemSize::Rom1;
	psR2 = curpos; curpos += Ps2MemSize::Rom2;
	psER = curpos; curpos += Ps2MemSize::ERom;
	psH = curpos; curpos += Ps2MemSize::Hardware;
	psS = curpos; //curpos += Ps2MemSize::Scratch;
}

void memShutdown() 
{
	vtlb_free( m_psAllMem, m_allMemSize );
	m_psAllMem = NULL;
	psM = psR = psR1 = psR2 = psER = psS = psH = NULL;
	vtlb_Term();
}

// Resets memory mappings, unmaps TLBs, reloads bios roms, etc.
void memReset()
{
	// VTLB Protection Preparations.

	SysMemProtect( m_psAllMem, m_allMemSize, Protect_ReadWrite );

	// Note!!  Ideally the vtlb should only be initialized once, and then subsequent
	// resets of the system hardware would only clear vtlb mappings, but since the
	// rest of the emu is not really set up to support a "soft" reset of that sort
	// we opt for the hard/safe version.

	memzero_ptr<m_allMemSize>( m_psAllMem );
#ifdef ENABLECACHE
	memset(pCache,0,sizeof(_cacheS)*64);
#endif

	vtlb_Init();

	tlb_fallback_0=vtlb_RegisterHandlerTempl1(_ext_mem,0);
	tlb_fallback_2=vtlb_RegisterHandlerTempl1(_ext_mem,2);
	tlb_fallback_3=vtlb_RegisterHandlerTempl1(_ext_mem,3);
	tlb_fallback_4=vtlb_RegisterHandlerTempl1(_ext_mem,4);
	tlb_fallback_5=vtlb_RegisterHandlerTempl1(_ext_mem,5);
	//tlb_fallback_6=vtlb_RegisterHandlerTempl1(_ext_mem,6);
	tlb_fallback_7=vtlb_RegisterHandlerTempl1(_ext_mem,7);
	tlb_fallback_8=vtlb_RegisterHandlerTempl1(_ext_mem,8);

	// Dynarec versions of VUs
	vu0_micro_mem[0] = vtlb_RegisterHandlerTempl2(vuMicro,0,true);
	vu1_micro_mem[0] = vtlb_RegisterHandlerTempl2(vuMicro,1,true);

	// Interpreter versions of VUs
	vu0_micro_mem[1] = vtlb_RegisterHandlerTempl2(vuMicro,0,false);
	vu1_micro_mem[1] = vtlb_RegisterHandlerTempl2(vuMicro,1,false);

	//////////////////////////////////////////////////////////////////////
	// psHw Optimized Mappings
	// The HW Registers have been split into pages to improve optimization.
	// Anything not explicitly mapped into one of the hw_by_page handlers will be handled
	// by the default/generic tlb_fallback_1 handler.

	tlb_fallback_1 = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, hwRead128_generic,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_generic, hwWrite64_generic, hwWrite128_generic
	);

	hw_by_page[0x0] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_page_00, hwRead64_page_00, hwRead128_page_00,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_page_00, hwWrite64_generic, hwWrite128_generic
	);

	hw_by_page[0x1] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_page_01, hwRead64_page_01, hwRead128_page_01,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_page_01, hwWrite64_generic, hwWrite128_generic
	);

	hw_by_page[0x2] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_page_02, hwRead64_page_02, hwRead128_page_02,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_page_02, hwWrite64_page_02, hwWrite128_generic
	);

	hw_by_page[0x3] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, hwRead128_generic,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_page_03, hwWrite64_page_03, hwWrite128_generic
	);

	hw_by_page[0x4] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, ReadFIFO_page_4,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_generic, hwWrite64_generic, WriteFIFO_page_4
	);

	hw_by_page[0x5] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, ReadFIFO_page_5,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_generic, hwWrite64_generic, WriteFIFO_page_5
	);

	hw_by_page[0x6] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, ReadFIFO_page_6,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_generic, hwWrite64_generic, WriteFIFO_page_6
	);

	hw_by_page[0x7] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, ReadFIFO_page_7,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_generic, hwWrite64_generic, WriteFIFO_page_7
	);

	hw_by_page[0xb] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, hwRead128_generic,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_page_0B, hwWrite64_generic, hwWrite128_generic
	);

	hw_by_page[0xe] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, hwRead32_generic, hwRead64_generic, hwRead128_generic,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_page_0E, hwWrite64_page_0E, hwWrite128_generic
	);

	vtlbMemR32FP* page0F32( CHECK_INTC_STAT_HACK ? hwRead32_page_0F_INTC_HACK : hwRead32_page_0F );
	vtlbMemR64FP* page0F64( CHECK_INTC_STAT_HACK ? hwRead64_generic_INTC_HACK : hwRead64_generic );

	hw_by_page[0xf] = vtlb_RegisterHandler(
		_ext_memRead8<1>, _ext_memRead16<1>, page0F32, page0F64, hwRead128_generic,
		_ext_memWrite8<1>, _ext_memWrite16<1>, hwWrite32_page_0F, hwWrite64_generic, hwWrite128_generic
	);

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
}

int mmap_GetRamPageInfo(void* ptr)
{
	u32 offset=((u8*)ptr-psM);
	if (offset>=Ps2MemSize::Base)
		return -1; //not in ram, no tracking done ...
	offset>>=12;
	return (psMPWC[(offset/32)]&(1<<(offset&31)))?1:0;
}

void mmap_MarkCountedRamPage(void* ptr,u32 vaddr)
{
#ifdef _WIN32
	DWORD old;
	VirtualProtect(ptr,1,PAGE_READONLY,&old);
#else
	// fixed?  mprotect needs input and size to be aligned to 4096 bytes pagesize.
	// 'ptr' should be aligned properly, but a size of 1 was invalid. (air)
	mprotect(ptr, getpagesize(), PROT_READ);
#endif
	
	u32 offset=((u8*)ptr-psM);
	offset>>=12;

	for (u32 i=0;i<psMPWVA[offset].size();i++)
	{
		if (psMPWVA[offset][i]==vaddr)
			return;
	}
	psMPWVA[offset].push_back(vaddr);
}

void mmap_ResetBlockTracking()
{
	DevCon::WriteLn("vtlb/mmap: Block Tracking reset...");
	memzero_obj(psMPWC);
	for(u32 i=0;i<(Ps2MemSize::Base>>12);i++)
	{
		psMPWVA[i].clear();
	}
#ifdef _WIN32
	DWORD old;
	VirtualProtect(psM,Ps2MemSize::Base,PAGE_READWRITE,&old);
#else
	mprotect(psM,Ps2MemSize::Base, PROT_READ|PROT_WRITE);
#endif
}

#ifdef _WIN32
int SysPageFaultExceptionFilter(EXCEPTION_POINTERS* eps)
{
	const _EXCEPTION_RECORD& ExceptionRecord = *eps->ExceptionRecord;
	//const _CONTEXT& ContextRecord = *eps->ContextRecord;
	
	if (ExceptionRecord.ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// get bad virtual address
	u32 offset = (u8*)ExceptionRecord.ExceptionInformation[1]-psM;

	if (offset>=Ps2MemSize::Base)
		return EXCEPTION_CONTINUE_SEARCH;

	DWORD old;
	VirtualProtect(&psM[offset],1,PAGE_READWRITE,&old);

	offset>>=12;
	psMPWC[(offset/32)]|=(1<<(offset&31));

	for (u32 i=0;i<psMPWVA[offset].size();i++)
	{
		Cpu->Clear(psMPWVA[offset][i],0x1000);
	}
	psMPWVA[offset].clear();

	return EXCEPTION_CONTINUE_EXECUTION;
}

#else
#include "errno.h"

void InstallLinuxExceptionHandler()
{
	struct sigaction sa;
	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &SysPageFaultExceptionFilter;
	sigaction(SIGSEGV, &sa, NULL); 
}

void ReleaseLinuxExceptionHandler()
{
	// Code this later.
}
// Linux implementation of SIGSEGV handler.  Bind it using sigaction().
// This is my shot in the dark.  Probably needs some work.  Good luck! (air)
void SysPageFaultExceptionFilter( int signal, siginfo_t *info, void * )
{
	int err;
	u32 pagesize = getpagesize();

	//DevCon::Error("SysPageFaultExceptionFilter!");
	// get bad virtual address
	u32 offset = (u8*)info->si_addr - psM;
	uptr pageoffset = ( offset / pagesize ) * pagesize;
	
	DevCon::Status( "Protected memory cleanup. Offset 0x%x", params offset );

	if (offset>=Ps2MemSize::Base)
	{
		// Bad mojo!  Completly invalid address.
		// Instigate a crash or abort emulation or something.
		assert( false );
	}

	err = mprotect( &psM[pageoffset], pagesize, PROT_READ | PROT_WRITE );
	if (err) DevCon::Error("SysPageFaultExceptionFilter: %s", params strerror(errno));
	
	offset>>=12;
	psMPWC[(offset/32)]|=(1<<(offset&31));

	for (u32 i=0;i<psMPWVA[offset].size();i++)
	{
		Cpu->Clear(psMPWVA[offset][i],0x1000);
	}
	psMPWVA[offset].clear();
}
#endif
