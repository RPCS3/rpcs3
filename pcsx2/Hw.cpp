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

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "IopMem.h"

// The full suite of hardware APIs:
#include "IPU/IPU.h"
#include "GS.h"
#include "Counters.h"
#include "Vif.h"
#include "VifDma.h"
#include "SPR.h"
#include "Sif.h"

using namespace R5900;

u8  *psH; // hw mem

int rdram_devices = 2;	// put 8 for TOOL and 2 for PS2 and PSX
int rdram_sdevid = 0;

void hwInit()
{
	gsInit();
	vif0Init();
	vif1Init();
	vifDmaInit();
	sifInit();
	sprInit();
	ipuInit();
}

void hwShutdown() {
	ipuShutdown();
}

void hwReset()
{
	hwInit();

	memzero_ptr<Ps2MemSize::Hardware>( PS2MEM_HW );
	//memset(PS2MEM_HW+0x2000, 0, 0x0000e000);

	psHu32(0xf520) = 0x1201;
	psHu32(0xf260) = 0x1D000060;
	// i guess this is kinda a version, it's used by some bioses
	psHu32(0xf590) = 0x1201;

	gsReset();
	ipuReset();
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 8 bit

__forceinline u8 hwRead8(u32 mem)
{
	u8 ret;

	if( mem >= 0x10002000 && mem < 0x10008000 )
		DevCon::Notice("hwRead8 to %x", params mem);

	SPR_LOG("Hardware read 8bit at %lx, ret %lx\n", mem, psHu8(mem));

	switch (mem)
	{
		case 0x10000000: ret = (u8)rcntRcount(0); break;
		case 0x10000010: ret = (u8)counters[0].modeval; break;
		case 0x10000020: ret = (u8)counters[0].target; break;
		case 0x10000030: ret = (u8)counters[0].hold; break;
		case 0x10000001: ret = (u8)(rcntRcount(0)>>8); break;
		case 0x10000011: ret = (u8)(counters[0].modeval>>8); break;
		case 0x10000021: ret = (u8)(counters[0].target>>8); break;
		case 0x10000031: ret = (u8)(counters[0].hold>>8); break;

		case 0x10000800: ret = (u8)rcntRcount(1); break;
		case 0x10000810: ret = (u8)counters[1].modeval; break;
		case 0x10000820: ret = (u8)counters[1].target; break;
		case 0x10000830: ret = (u8)counters[1].hold; break;
		case 0x10000801: ret = (u8)(rcntRcount(1)>>8); break;
		case 0x10000811: ret = (u8)(counters[1].modeval>>8); break;
		case 0x10000821: ret = (u8)(counters[1].target>>8); break;
		case 0x10000831: ret = (u8)(counters[1].hold>>8); break;

		case 0x10001000: ret = (u8)rcntRcount(2); break;
		case 0x10001010: ret = (u8)counters[2].modeval; break;
		case 0x10001020: ret = (u8)counters[2].target; break;
		case 0x10001001: ret = (u8)(rcntRcount(2)>>8); break;
		case 0x10001011: ret = (u8)(counters[2].modeval>>8); break;
		case 0x10001021: ret = (u8)(counters[2].target>>8); break;

		case 0x10001800: ret = (u8)rcntRcount(3); break;
		case 0x10001810: ret = (u8)counters[3].modeval; break;
		case 0x10001820: ret = (u8)counters[3].target; break;
		case 0x10001801: ret = (u8)(rcntRcount(3)>>8); break;
		case 0x10001811: ret = (u8)(counters[3].modeval>>8); break;
		case 0x10001821: ret = (u8)(counters[3].target>>8); break;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200)
			{
				if(mem == 0x1000f260) ret = 0;
				else if(mem == 0x1000F240) {
					ret = psHu32(mem);
					//psHu32(mem) &= ~0x4000;
				}
				else ret = psHu32(mem);
				return (u8)ret;
			}

			ret = psHu8(mem);
			HW_LOG("Unknown Hardware Read 8 at %x\n",mem);
			break;
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 16 bit

__forceinline u16 hwRead16(u32 mem)
{
	u16 ret;

	if( mem >= 0x10002000 && mem < 0x10008000 )
		Console::Notice("hwRead16 to %x", params mem);

	switch (mem) {
		case 0x10000000: ret = (u16)rcntRcount(0); break;
		case 0x10000010: ret = (u16)counters[0].modeval; break;
		case 0x10000020: ret = (u16)counters[0].target; break;
		case 0x10000030: ret = (u16)counters[0].hold; break;

		case 0x10000800: ret = (u16)rcntRcount(1); break;
		case 0x10000810: ret = (u16)counters[1].modeval; break;
		case 0x10000820: ret = (u16)counters[1].target; break;
		case 0x10000830: ret = (u16)counters[1].hold; break;

		case 0x10001000: ret = (u16)rcntRcount(2); break;
		case 0x10001010: ret = (u16)counters[2].modeval; break;
		case 0x10001020: ret = (u16)counters[2].target; break;

		case 0x10001800: ret = (u16)rcntRcount(3); break;
		case 0x10001810: ret = (u16)counters[3].modeval; break;
		case 0x10001820: ret = (u16)counters[3].target; break;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				if(mem == 0x1000f260) ret = 0;
				else if(mem == 0x1000F240) {
					ret = psHu16(mem) | 0x0102;
					psHu32(mem) &= ~0x4000;
				}
				else ret = psHu32(mem);
				return (u16)ret;
			}
			ret = psHu16(mem);
			HW_LOG("Hardware Read16 at 0x%x, value= 0x%x\n", ret, mem);
			break;
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 32 bit

// Reads hardware registers for page 0 (counters 0 and 1)
mem32_t __fastcall hwRead32_page_00(u32 mem)
{
	mem &= 0xffff;
	switch( mem )
	{
		case 0x00: return (u16)rcntRcount(0);
		case 0x10: return (u16)counters[0].modeval;
		case 0x20: return (u16)counters[0].target;
		case 0x30: return (u16)counters[0].hold;

		case 0x800: return (u16)rcntRcount(1);
		case 0x810: return (u16)counters[1].modeval;
		case 0x820: return (u16)counters[1].target;
		case 0x830: return (u16)counters[1].hold;
	}

	return *((u32*)&PS2MEM_HW[mem]);
}

// Reads hardware registers for page 1 (counters 2 and 3)
mem32_t __fastcall hwRead32_page_01(u32 mem)
{
	mem &= 0xffff;
	switch( mem )
	{
		case 0x1000: return (u16)rcntRcount(2);
		case 0x1010: return (u16)counters[2].modeval;
		case 0x1020: return (u16)counters[2].target;

		case 0x1800: return (u16)rcntRcount(3);
		case 0x1810: return (u16)counters[3].modeval;
		case 0x1820: return (u16)counters[3].target;
	}

	return *((u32*)&PS2MEM_HW[mem]);
}

// Reads hardware registers for page 15 (0x0F).
mem32_t __fastcall hwRead32_page_0F(u32 mem)
{
	// *Performance Warning*  This function is called -A-LOT.  Be weary when making changes.  It
	// could impact FPS significantly.

	mem &= 0xffff;

	switch( mem )
	{
		case 0xf000:
			// This one is checked alot, so leave it commented out unless you love 600 meg logfiles.
			//HW_LOG("DMAC_STAT Read  32bit %x\n", psHu32(0xe010));
		break;

		case 0xf010:
			HW_LOG("INTC_MASK Read32, value=0x%x", psHu32(INTC_MASK));
		break;

		case 0xf130:	// 0x1000f130
		case 0xf260:	// 0x1000f260 SBUS?
		case 0xf410:	// 0x1000f410
		case 0xf430:	// MCH_RICM
			return 0;

		case 0xf240:	// 0x1000f240: SBUS
			return psHu32(0xf240) | 0xF0000102;

		case 0xf440:	// 0x1000f440: MCH_DRD

			if( !((psHu32(0xf430) >> 6) & 0xF) )
			{
				switch ((psHu32(0xf430)>>16) & 0xFFF)
				{
					//MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5

					case 0x21://INIT
						if(rdram_sdevid < rdram_devices)
						{
							rdram_sdevid++;
							return 0x1F;
						}
					return 0;

					case 0x23://CNFGA
						return 0x0D0D;	//PVER=3 | MVER=16 | DBL=1 | REFBIT=5

					case 0x24://CNFGB
						//0x0110 for PSX  SVER=0 | CORG=8(5x9x7) | SPT=1 | DEVTYP=0 | BYTE=0
						return 0x0090;	//SVER=0 | CORG=4(5x9x6) | SPT=1 | DEVTYP=0 | BYTE=0

					case 0x40://DEVID
						return psHu32(0xf430) & 0x1F;	// =SDEV
				}
			}
			return 0;
	}
	return *((u32*)&PS2MEM_HW[mem]);
}

mem32_t __fastcall hwRead32_page_02(u32 mem)
{
	return ipuRead32( mem );
}

// Used for all pages not explicitly specified above.
mem32_t __fastcall hwRead32_generic(u32 mem)
{
	const u16 masked_mem = mem & 0xffff;

	switch( masked_mem>>12 )		// switch out as according to the 4k page of the access.
	{
		///////////////////////////////////////////////////////
		// Most of the following case handlers are for developer builds only (logging).
		// It'll all optimize to ziltch in public release builds.

		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		{
			const char* regName = "Unknown";

			switch( mem )
			{
				case D2_CHCR: regName = "DMA2_CHCR"; break;
				case D2_MADR: regName = "DMA2_MADR"; break;
				case D2_QWC: regName = "DMA2_QWC"; break;
				case D2_TADR: regName = "DMA2_TADDR"; break;
				case D2_ASR0: regName = "DMA2_ASR0"; break;
				case D2_ASR1: regName = "DMA2_ASR1"; break;
				case D2_SADR: regName = "DMA2_SADDR"; break;
			}

			HW_LOG( "Hardware Read32 at 0x%x (%s), value=0x%x\n", mem, regName, psHu32(mem) );
		}
		break;

		case 0x0b:
			if( mem == D4_CHCR )
				HW_LOG("Hardware Read32 at 0x%x (IPU1:DMA4_CHCR), value=0x%x\n", mem, psHu32(mem));
		break;

		case 0x0c:
		case 0x0d:
		case 0x0e:
			if( mem == DMAC_STAT )
				HW_LOG("DMAC_STAT Read32, value=0x%x\n", psHu32(DMAC_STAT));
		break;

		jNO_DEFAULT;
	}

	return *((u32*)&PS2MEM_HW[masked_mem]);
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 64 bit

void __fastcall hwRead64_page_00(u32 mem, mem64_t* result )
{
	*result = hwRead32_page_00( mem );
}

void __fastcall hwRead64_page_01(u32 mem, mem64_t* result )
{
	*result = hwRead32_page_01( mem );
}

void __fastcall hwRead64_page_02(u32 mem, mem64_t* result )
{
	*result = ipuRead64(mem);
}

void __fastcall hwRead64_generic(u32 mem, mem64_t* result )
{
	*result = psHu64(mem);
	HW_LOG("Unknown Hardware Read 64 at %x\n",mem);
}

/////////////////////////////////////////////////////////////////////////
// Hardware READ 128 bit

void __fastcall hwRead128_page_00(u32 mem, mem128_t* result )
{
	result[0] = hwRead32_page_00( mem );
	result[1] = 0;
}

void __fastcall hwRead128_page_01(u32 mem, mem128_t* result )
{
	result[0] = hwRead32_page_01( mem );
	result[1] = 0;
}

void __fastcall hwRead128_page_02(u32 mem, mem128_t* result )
{
	// IPU is currently unhandled in 128 bit mode.
	HW_LOG("Unknown Hardware Read 128 at %x (IPU)\n",mem);
}

void __fastcall hwRead128_generic(u32 mem, mem128_t* out)
{
	out[0] = psHu64(mem);
	out[1] = psHu64(mem+8);

	HW_LOG("Unknown Hardware Read 128 at %x\n",mem);
}

/////////////////////////////////////////////////////////////////////////
// DMA Execution Interfaces

// dark cloud2 uses 8 bit DMAs register writes
static __forceinline void DmaExec8( void (*func)(), u32 mem, u8 value )
{
	psHu8(mem) = (u8)value;
	if ((psHu8(mem) & 0x1) && (psHu32(DMAC_CTRL) & 0x1))
	{
		/*SysPrintf("Running DMA 8 %x\n", psHu32(mem & ~0x1));*/
		func();
	}
}

static __forceinline void DmaExec16( void (*func)(), u32 mem, u16 value )
{
	psHu16(mem) = (u16)value;
	if ((psHu16(mem) & 0x100) && (psHu32(DMAC_CTRL) & 0x1))
	{
		//SysPrintf("16bit DMA Start\n");
		func();
	}
}

static void DmaExec( void (*func)(), u32 mem, u32 value )
{
	/* Keep the old tag if in chain mode and hw doesnt set it*/
	if( (value & 0xc) == 0x4 && (value & 0xffff0000) == 0)
		psHu32(mem) = (psHu32(mem) & 0xFFFF0000) | (u16)value;
	else /* Else (including Normal mode etc) write whatever the hardware sends*/
		psHu32(mem) = (u32)value;

	if ((psHu32(mem) & 0x100) && (psHu32(DMAC_CTRL) & 0x1))
		func();
}

/////////////////////////////////////////////////////////////////////////
// Hardware WRITE 8 bit

char sio_buffer[1024];
int sio_count;

void hwWrite8(u32 mem, u8 value) {

#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10002000 && mem < 0x10008000 )
		SysPrintf("hwWrite8 to %x\n", mem);
#endif

	switch (mem) {
		case 0x10000000: rcntWcount(0, value); break;
		case 0x10000010: rcntWmode(0, (counters[0].modeval & 0xff00) | value); break;
		case 0x10000011: rcntWmode(0, (counters[0].modeval & 0xff) | value << 8); break;
		case 0x10000020: rcntWtarget(0, value); break;
		case 0x10000030: rcntWhold(0, value); break;

		case 0x10000800: rcntWcount(1, value); break;
		case 0x10000810: rcntWmode(1, (counters[1].modeval & 0xff00) | value); break;
		case 0x10000811: rcntWmode(1, (counters[1].modeval & 0xff) | value << 8); break;
		case 0x10000820: rcntWtarget(1, value); break;
		case 0x10000830: rcntWhold(1, value); break;

		case 0x10001000: rcntWcount(2, value); break;
		case 0x10001010: rcntWmode(2, (counters[2].modeval & 0xff00) | value); break;
		case 0x10001011: rcntWmode(2, (counters[2].modeval & 0xff) | value << 8); break;
		case 0x10001020: rcntWtarget(2, value); break;

		case 0x10001800: rcntWcount(3, value); break;
		case 0x10001810: rcntWmode(3, (counters[3].modeval & 0xff00) | value); break;
		case 0x10001811: rcntWmode(3, (counters[3].modeval & 0xff) | value << 8); break;
		case 0x10001820: rcntWtarget(3, value); break;

		case 0x1000f180:
			if (value == '\n') {
				sio_buffer[sio_count] = 0;
				Console::WriteLn( Color_Cyan, sio_buffer );
				sio_count = 0;
			} else {
				if (sio_count < 1023) {
					sio_buffer[sio_count++] = value;
				}
			}
			break;
		
		case 0x10003c02: //Tony Hawks Project 8 uses this
			vif1Write32(mem & ~0x2, value << 16);
			break;
		case 0x10008001: // dma0 - vif0
			DMA_LOG("VIF0dma %lx\n", value);
			DmaExec8(dmaVIF0, mem, value);
			break;

		case 0x10009001: // dma1 - vif1
			DMA_LOG("VIF1dma %lx\n", value);
			DmaExec8(dmaVIF1, mem, value);
			break;

		case 0x1000a001: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite8: GSdma %lx 0x%lx\n", cpuRegs.cycle, value);
			DmaExec8(dmaGIF, mem, value);
			break;

		case 0x1000b001: // dma3 - fromIPU
			DMA_LOG("IPU0dma %lx\n", value);
			DmaExec8(dmaIPU0, mem, value);
			break;

		case 0x1000b401: // dma4 - toIPU
			DMA_LOG("IPU1dma %lx\n", value);
			DmaExec8(dmaIPU1, mem, value);
			break;

		case 0x1000c001: // dma5 - sif0
			DMA_LOG("SIF0dma %lx\n", value);
//			if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec8(dmaSIF0, mem, value);
			break;

		case 0x1000c401: // dma6 - sif1
			DMA_LOG("SIF1dma %lx\n", value);
			DmaExec8(dmaSIF1, mem, value);
			break;

		case 0x1000c801: // dma7 - sif2
			DMA_LOG("SIF2dma %lx\n", value);
			DmaExec8(dmaSIF2, mem, value);
			break;

		case 0x1000d001: // dma8 - fromSPR
			DMA_LOG("fromSPRdma8 %lx\n", value);
			DmaExec8(dmaSPR0, mem, value);
			break;

		case 0x1000d401: // dma9 - toSPR
			DMA_LOG("toSPRdma8 %lx\n", value);
			DmaExec8(dmaSPR1, mem, value);
			break;

		case 0x1000f592: // DMAC_ENABLEW
			psHu8(0xf592) = value;
			psHu8(0xf522) = value;
			break;

		case 0x1000f200: // SIF(?)
			psHu8(mem) = value;
			break;

		case 0x1000f240:// SIF(?)
			if(!(value & 0x100))
				psHu32(mem) &= ~0x100;
			break;
		
		default:
			assert( (mem&0xff0f) != 0xf200 );

			switch(mem&~3) {
				case 0x1000f130:
				case 0x1000f410:
				case 0x1000f430:
					break;
				default:
					psHu8(mem) = value;
			}
			HW_LOG("Unknown Hardware write 8 at %x with value %x\n", mem, value);
			break;
	}
}

__forceinline void hwWrite16(u32 mem, u16 value)
{
	if( mem >= 0x10002000 && mem < 0x10008000 )
		Console::Notice( "hwWrite16 to %x", params mem );

	switch(mem)
	{
		case 0x10000000: rcntWcount(0, value); break;
		case 0x10000010: rcntWmode(0, value); break;
		case 0x10000020: rcntWtarget(0, value); break;
		case 0x10000030: rcntWhold(0, value); break;

		case 0x10000800: rcntWcount(1, value); break;
		case 0x10000810: rcntWmode(1, value); break;
		case 0x10000820: rcntWtarget(1, value); break;
		case 0x10000830: rcntWhold(1, value); break;

		case 0x10001000: rcntWcount(2, value); break;
		case 0x10001010: rcntWmode(2, value); break;
		case 0x10001020: rcntWtarget(2, value); break;

		case 0x10001800: rcntWcount(3, value); break;
		case 0x10001810: rcntWmode(3, value); break;
		case 0x10001820: rcntWtarget(3, value); break;

		case 0x10008000: // dma0 - vif0
			DMA_LOG("VIF0dma %lx\n", value);
			DmaExec16(dmaVIF0, mem, value);
			break;

		case 0x10009000: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma CHCR %lx\n", value);
			DmaExec16(dmaVIF1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x10009010: // dma1 - vif1 - madr
			HW_LOG("VIF1dma Madr %lx\n", value);
			psHu16(mem) = value;//dma1 madr
			break;
		case 0x10009020: // dma1 - vif1 - qwc
			HW_LOG("VIF1dma QWC %lx\n", value);
			psHu16(mem) = value;//dma1 qwc
			break;
		case 0x10009030: // dma1 - vif1 - tadr
			HW_LOG("VIF1dma TADR %lx\n", value);
			psHu16(mem) = value;//dma1 tadr
			break;
		case 0x10009040: // dma1 - vif1 - asr0
			HW_LOG("VIF1dma ASR0 %lx\n", value);
			psHu16(mem) = value;//dma1 asr0
			break;
		case 0x10009050: // dma1 - vif1 - asr1
			HW_LOG("VIF1dma ASR1 %lx\n", value);
			psHu16(mem) = value;//dma1 asr1
			break;
		case 0x10009080: // dma1 - vif1 - sadr
			HW_LOG("VIF1dma SADR %lx\n", value);
			psHu16(mem) = value;//dma1 sadr
			break;
#endif
// ---------------------------------------------------

		case 0x1000a000: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite32: GSdma %lx\n", cpuRegs.cycle, value);
			DmaExec16(dmaGIF, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000a010:
		    psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write DMA2_MADR 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a020:
            psHu16(mem) = value;//dma2 qwc
		    HW_LOG("Hardware write DMA2_QWC 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a030:
            psHu16(mem) = value;//dma2 taddr
		    HW_LOG("Hardware write DMA2_TADDR 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a040:
            psHu16(mem) = value;//dma2 asr0
		    HW_LOG("Hardware write DMA2_ASR0 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a050:
            psHu16(mem) = value;//dma2 asr1
		    HW_LOG("Hardware write DMA2_ASR1 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a080:
            psHu16(mem) = value;//dma2 saddr
		    HW_LOG("Hardware write DMA2_SADDR 32bit at %x with value %x\n",mem,value);
		    break;
#endif

		case 0x1000b000: // dma3 - fromIPU
			DMA_LOG("IPU0dma %lx\n", value);
			DmaExec16(dmaIPU0, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000b010:
	   		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_MADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b020:
    		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_QWC 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b030:
			psHu16(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU0DMA_TADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b080:
			psHu16(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU0DMA_SADDR 32bit at %x with value %x\n",mem,value);
			break;
#endif

		case 0x1000b400: // dma4 - toIPU
			DMA_LOG("IPU1dma %lx\n", value);
			DmaExec16(dmaIPU1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000b410:
    		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_MADR 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b420:
    		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_QWC 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b430:
			psHu16(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU1DMA_TADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b480:
			psHu16(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU1DMA_SADDR 32bit at %x with value %x\n",mem,value);
			break;
#endif
		case 0x1000c000: // dma5 - sif0
			DMA_LOG("SIF0dma %lx\n", value);
//			if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec16(dmaSIF0, mem, value);
			break;

		case 0x1000c002:
			//?
			break;
		case 0x1000c400: // dma6 - sif1
			DMA_LOG("SIF1dma %lx\n", value);
			DmaExec16(dmaSIF1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case 0x1000c420: // dma6 - sif1 - qwc
			HW_LOG("SIF1dma QWC = %lx\n", value);
			psHu16(mem) = value;
			break;

		case 0x1000c430: // dma6 - sif1 - tadr
			HW_LOG("SIF1dma TADR = %lx\n", value);
			psHu16(mem) = value;
			break;
#endif

		case 0x1000c800: // dma7 - sif2
			DMA_LOG("SIF2dma %lx\n", value);
			DmaExec16(dmaSIF2, mem, value);
			break;
		case 0x1000c802:
			//?
			break;
		case 0x1000d000: // dma8 - fromSPR
			DMA_LOG("fromSPRdma %lx\n", value);
			DmaExec16(dmaSPR0, mem, value);
			break;

		case 0x1000d400: // dma9 - toSPR
			DMA_LOG("toSPRdma %lx\n", value);
			DmaExec16(dmaSPR1, mem, value);
			break;
		case 0x1000f592: // DMAC_ENABLEW
			psHu16(0xf592) = value;
			psHu16(0xf522) = value;
			break;
		case 0x1000f130:
		case 0x1000f132:
		case 0x1000f410:
		case 0x1000f412:
		case 0x1000f430:
		case 0x1000f432:
			break;

		case 0x1000f200:
			psHu16(mem) = value;
			break;

		case 0x1000f220:
			psHu16(mem) |= value;
			break;
		case 0x1000f230:
			psHu16(mem) &= ~value;
			break;
		case 0x1000f240:
			if(!(value & 0x100))
				psHu16(mem) &= ~0x100;
			else
				psHu16(mem) |= 0x100;
			break;
		case 0x1000f260:
			psHu16(mem) = 0;
			break;

		default:
			psHu16(mem) = value;
			HW_LOG("Unknown Hardware write 16 at %x with value %x\n",mem,value);
	}
}

// Page 0 of HW memory houses registers for Counters 0 and 1
void __fastcall hwWrite32_page_00( u32 mem, u32 value )
{
	mem &= 0xffff;
	switch (mem)
	{
		case 0x000: rcntWcount(0, value); return;
		case 0x010: rcntWmode(0, value); return;
		case 0x020: rcntWtarget(0, value); return;
		case 0x030: rcntWhold(0, value); return;

		case 0x800: rcntWcount(1, value); return;
		case 0x810: rcntWmode(1, value); return;
		case 0x820: rcntWtarget(1, value); return;
		case 0x830: rcntWhold(1, value); return;
	}

	*((u32*)&PS2MEM_HW[mem]) = value;
}

// Page 1 of HW memory houses registers for Counters 2 and 3
void __fastcall hwWrite32_page_01( u32 mem, u32 value )
{
	mem &= 0xffff;
	switch (mem)
	{
		case 0x1000: rcntWcount(2, value); return;
		case 0x1010: rcntWmode(2, value); return;
		case 0x1020: rcntWtarget(2, value); return;

		case 0x1800: rcntWcount(3, value); return;
		case 0x1810: rcntWmode(3, value); return;
		case 0x1820: rcntWtarget(3, value); return;
	}

	*((u32*)&PS2MEM_HW[mem]) = value;
}

// page 2 is the IPU register space!
void __fastcall hwWrite32_page_02( u32 mem, u32 value )
{
	ipuWrite32(mem, value);
}

// Page 3 contains writes to vif0 and vif1 registers, plus some GIF stuff!
void __fastcall hwWrite32_page_03( u32 mem, u32 value )
{
	if(mem>=0x10003800)
	{
		if(mem<0x10003c00)
			vif0Write32(mem, value); 
		else
			vif1Write32(mem, value); 
		return;
	}

	switch (mem)
	{
		case GIF_CTRL:
			psHu32(mem) = value & 0x8;
			if (value & 0x1)
				gsGIFReset();
			else if( value & 8 )
				psHu32(GIF_STAT) |= 8;
			else
				psHu32(GIF_STAT) &= ~8;
		break;

		case GIF_MODE:
		{
			// need to set GIF_MODE (hamster ball)
			psHu32(GIF_MODE) = value;

			// set/clear bits 0 and 2 as per the GIF_MODE value.
			const u32 bitmask = 0x1 | 0x4;
			psHu32(GIF_STAT) &= ~bitmask;
			psHu32(GIF_STAT) |= (u32)value & bitmask;
		}
		break;

		case GIF_STAT: // stat is readonly
			DevCon::Notice("*PCSX2* GIFSTAT write value = 0x%x (readonly, ignored)", params value);
		break;

		default:
			psHu32(mem) = value;
	}
}

void __fastcall hwWrite32_page_0B( u32 mem, u32 value )
{
	// Used for developer logging -- optimized away in Public Release.
	const char* regName = "Unknown";

	switch( mem )
	{
		case D3_CHCR: // dma3 - fromIPU
			DMA_LOG("IPU0dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaIPU0, mem, value);
		return;

		case D3_MADR: regName = "IPU0DMA_MADR"; break;
		case D3_QWC: regName = "IPU0DMA_QWC"; break;
		case D3_TADR: regName = "IPU0DMA_TADR"; break;
		case D3_SADR: regName = "IPU0DMA_SADDR"; break;

		//------------------------------------------------------------------

		case D4_CHCR: // dma4 - toIPU
			DMA_LOG("IPU1dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaIPU1, mem, value);
		return;

		case D4_MADR: regName = "IPU1DMA_MADR"; break;
		case D4_QWC: regName = "IPU1DMA_QWC"; break;
		case D4_TADR: regName = "IPU1DMA_TADR"; break;
		case D4_SADR: regName = "IPU1DMA_SADDR"; break;
	}

	HW_LOG( "Hardware Write32 at 0x%x (%s), value=0x%x\n", mem, regName, value );
	psHu32(mem) = value;
}

void __fastcall hwWrite32_page_0E( u32 mem, u32 value )
{
	if( mem == DMAC_CTRL )
	{
		HW_LOG("DMAC_CTRL Write 32bit %x\n", value);
	}
	else if( mem == DMAC_STAT )
	{
		HW_LOG("DMAC_STAT Write 32bit %x\n", value);

		// lower 16 bits: clear on 1
		// upper 16 bits: reverse on 1

		psHu16(0xe010) &= ~(value & 0xffff);
		psHu16(0xe012) ^= (u16)(value >> 16);

		cpuTestDMACInts();
		return;
	}

	psHu32(mem) = value;
}

void __fastcall hwWrite32_page_0F( u32 mem, u32 value )
{
	// Shift the middle 8 bits (bits 4-12) into the lower 8 bits.
	// This helps the compiler optimize the switch statement into a lookup table. :)

#define HELPSWITCH(m) (((m)>>4) & 0xff)

	switch( HELPSWITCH(mem) )
	{
		case HELPSWITCH(INTC_STAT):
			HW_LOG("INTC_STAT Write 32bit %x\n", value);
			psHu32(INTC_STAT) &= ~value;	
			//cpuTestINTCInts();
			break;

		case HELPSWITCH(INTC_MASK):
			HW_LOG("INTC_MASK Write 32bit %x\n", value);
			psHu32(INTC_MASK) ^= (u16)value;
			cpuTestINTCInts();
			break;

		//------------------------------------------------------------------			
		case HELPSWITCH(0x1000f430)://MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
			if ((((value >> 16) & 0xFFF) == 0x21) && (((value >> 6) & 0xF) == 1) && (((psHu32(0xf440) >> 7) & 1) == 0))//INIT & SRP=0
				rdram_sdevid = 0;	// if SIO repeater is cleared, reset sdevid
			psHu32(mem) = value & ~0x80000000;	//kill the busy bit
			break;

		case HELPSWITCH(0x1000f200):
			psHu32(mem) = value;
			break;
		case HELPSWITCH(0x1000f220):
			psHu32(mem) |= value;
			break;
		case HELPSWITCH(0x1000f230):
			psHu32(mem) &= ~value;
			break;
		case HELPSWITCH(0x1000f240):
			if(!(value & 0x100))
				psHu32(mem) &= ~0x100;
			else
				psHu32(mem) |= 0x100;
			break;
		case HELPSWITCH(0x1000f260):
			psHu32(mem) = 0;
			break;

		case HELPSWITCH(0x1000f440)://MCH_DRD:
			psHu32(mem) = value;
			break;

		case HELPSWITCH(0x1000f590): // DMAC_ENABLEW
			HW_LOG("DMAC_ENABLEW Write 32bit %lx\n", value);
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
			break;

		//------------------------------------------------------------------
		case HELPSWITCH(0x1000f130):
		case HELPSWITCH(0x1000f410):
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)\n", mem, value, cpuRegs.CP0.n.Status.val);
		break;

		default:
			psHu32(mem) = value;
	}
}

void __fastcall hwWrite32_generic( u32 mem, u32 value )
{
	// Used for developer logging -- optimized away in Public Release.
	const char* regName = "Unknown";

	switch (mem)
	{
		case D0_CHCR: // dma0 - vif0
			DMA_LOG("VIF0dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaVIF0, mem, value);
			return;

//------------------------------------------------------------------
		case D1_CHCR: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaVIF1, mem, value);
			return;

		case D1_MADR: regName = "VIF1dma MADR"; break;
		case D1_QWC: regName = "VIF1dma QWC"; break;
		case D1_TADR: regName = "VIF1dma TADR"; break;
		case D1_ASR0: regName = "VIF1dma ASR0"; break;
		case D1_ASR1: regName = "VIF1dma ASR1"; break;
		case D1_SADR: regName = "VIF1dma SADR"; break;

//------------------------------------------------------------------
		case D2_CHCR: // dma2 - gif
			DMA_LOG("GIFdma EXECUTE, value=0x%x", value);
			DmaExec(dmaGIF, mem, value);
			return;

		case D2_MADR: regName = "GIFdma MADR"; break;
	    case D2_QWC: regName = "GIFdma QWC"; break;
	    case D2_TADR: regName = "GIFdma TADDR"; break;
	    case D2_ASR0: regName = "GIFdma ASR0"; break;
	    case D2_ASR1: regName = "GIFdma ASR1"; break;
	    case D2_SADR: regName = "GIFdma SADDR"; break;

//------------------------------------------------------------------
		case 0x1000c000: // dma5 - sif0
			DMA_LOG("SIF0dma EXECUTE, value=0x%x\n", value);
			//if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec(dmaSIF0, mem, value);
			return;
//------------------------------------------------------------------
		case 0x1000c400: // dma6 - sif1
			DMA_LOG("SIF1dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaSIF1, mem, value);
			return;

		case 0x1000c420: regName = "SIF1dma QWC"; break;
		case 0x1000c430: regName = "SIF1dma TADR"; break;

//------------------------------------------------------------------
		case 0x1000c800: // dma7 - sif2
			DMA_LOG("SIF2dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaSIF2, mem, value);
			return;
//------------------------------------------------------------------
		case 0x1000d000: // dma8 - fromSPR
			DMA_LOG("SPR0dma EXECUTE (fromSPR), value=0x%x\n", value);
			DmaExec(dmaSPR0, mem, value);
			return;
//------------------------------------------------------------------
		case 0x1000d400: // dma9 - toSPR
			DMA_LOG("SPR0dma EXECUTE (toSPR), value=0x%x\n", value);
			DmaExec(dmaSPR1, mem, value);
			return;
	}
	HW_LOG( "Hardware Write32 at 0x%x (%s), value=0x%x\n", mem, regName, value );
	psHu32(mem) = value;
}

/////////////////////////////////////////////////////////////////////////
// HW Write 64 bit

void __fastcall hwWrite64_page_02( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval );  return;
	ipuWrite64( mem, *srcval );
}

void __fastcall hwWrite64_page_03( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;
	const u64 value = *srcval;

	if(mem>=0x10003800)
	{
		if(mem<0x10003c00)
			vif0Write32(mem, value); 
		else
			vif1Write32(mem, value); 
		return;
	}

	switch (mem)
	{
		case GIF_CTRL:
			DevCon::Status("GIF_CTRL write 64", params value);
			psHu32(mem) = value & 0x8;
			if(value & 0x1)
				gsGIFReset();
			else
			{
				if( value & 8 )
					psHu32(GIF_STAT) |= 8;
				else
					psHu32(GIF_STAT) &= ~8;
			}
	
			return;

		case GIF_MODE:
		{
#ifdef GSPATH3FIX
			Console::Status("GIFMODE64 %x\n", params value);
#endif
			psHu64(GIF_MODE) = value;

			// set/clear bits 0 and 2 as per the GIF_MODE value.
			const u32 bitmask = 0x1 | 0x4;
			psHu32(GIF_STAT) &= ~bitmask;
			psHu32(GIF_STAT) |= (u32)value & bitmask;
		}

		case GIF_STAT: // stat is readonly
			return;
	}
}

void __fastcall hwWrite64_page_0E( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;

	const u64 value = *srcval;

	if( mem == DMAC_CTRL )
	{
		HW_LOG("DMAC_CTRL Write 64bit %x\n", value);
	}
	else if( mem == DMAC_STAT )
	{
		HW_LOG("DMAC_STAT Write 64bit %x\n", value);

		// lower 16 bits: clear on 1
		// upper 16 bits: reverse on 1

		psHu16(0xe010) &= ~(value & 0xffff);
		psHu16(0xe012) ^= (u16)(value >> 16);

		cpuTestDMACInts();
		return;
	}

	psHu64(mem) = value;
}

void __fastcall hwWrite64_generic( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;

	const u64 value = *srcval;

	switch (mem)
	{
		case 0x1000a000: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite64: GSdma %x\n", cpuRegs.cycle, value);
			DmaExec(dmaGIF, mem, value);
		break;

		case INTC_STAT:
			HW_LOG("INTC_STAT Write 64bit %x\n", (u32)value);
			psHu32(INTC_STAT) &= ~value;	
			//cpuTestINTCInts();
		break;

		case INTC_MASK:
			HW_LOG("INTC_MASK Write 64bit %x\n", (u32)value);
			psHu32(INTC_MASK) ^= (u16)value;
			cpuTestINTCInts();
		break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		case 0x1000f590: // DMAC_ENABLEW
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
		break;

		default:
			psHu64(mem) = value;
			HW_LOG("Unknown Hardware write 64 at %x with value %x (status=%x)\n",mem,value, cpuRegs.CP0.n.Status.val);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////
// HW Write 128 bit

void __fastcall hwWrite128_generic(u32 mem, const mem128_t *srcval)
{
	//hwWrite128( mem, srcval ); return;

	switch (mem)
	{
		case INTC_STAT:
			HW_LOG("INTC_STAT Write 64bit %x\n", (u32)srcval[0]);
			psHu32(INTC_STAT) &= ~srcval[0];	
			//cpuTestINTCInts();
		break;
		
		case INTC_MASK:
			HW_LOG("INTC_MASK Write 64bit %x\n", (u32)srcval[0]);
			psHu32(INTC_MASK) ^= (u16)srcval[0];
			cpuTestINTCInts();
		break;

		case 0x1000f590: // DMAC_ENABLEW
			psHu32(0xf590) = srcval[0];
			psHu32(0xf520) = srcval[0];
		break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		default:
			psHu64(mem  ) = srcval[0];
			psHu64(mem+8) = srcval[1];

			HW_LOG("Unknown Hardware write 128 at %x with value %x_%x (status=%x)\n", mem, srcval[1], srcval[0], cpuRegs.CP0.n.Status.val);
		break;
	}
}

__forceinline void  intcInterrupt()
{
	if ((cpuRegs.CP0.n.Status.val & 0x400) != 0x400) return;

	if ((psHu32(INTC_STAT)) == 0) {
		DevCon::Notice("*PCSX2*: intcInterrupt already cleared");
        return;
	}
	if ((psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0) return;

	HW_LOG("intcInterrupt %x\n", psHu32(INTC_STAT) & psHu32(INTC_MASK));
	if(psHu32(INTC_STAT) & 0x2){
		counters[0].hold = rcntRcount(0);
		counters[1].hold = rcntRcount(1);
	}

	cpuException(0x400, cpuRegs.branch);
}

__forceinline void  dmacInterrupt()
{
    if ((cpuRegs.CP0.n.Status.val & 0x10807) != 0x10801) return;

	if( ((psHu16(0xe012) & psHu16(0xe010)) == 0 ) && 
		( psHu16(0xe010) & 0x8000) == 0 ) return;

	if((psHu32(DMAC_CTRL) & 0x1) == 0) return;
	
	HW_LOG("dmacInterrupt %x\n", (psHu16(0xe012) & psHu16(0xe010) || 
								  psHu16(0xe010) & 0x8000));

	cpuException(0x800, cpuRegs.branch);
}

void hwIntcIrq(int n) {
	psHu32(INTC_STAT)|= 1<<n;
	cpuTestINTCInts();
}

void hwDmacIrq(int n) {
	psHu32(DMAC_STAT)|= 1<<n;
	cpuTestDMACInts();	
}

/* Write 'size' bytes to memory address 'addr' from 'data'. */
int hwMFIFOWrite(u32 addr, u8 *data, u32 size) {
	u32 msize = psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)+16;
	u8 *dst;


	addr = psHu32(DMAC_RBOR) + (addr & psHu32(DMAC_RBSR));
	/* Check if the transfer should wrap around the ring buffer */
	if ((addr+size) >= msize) {
		int s1 = msize - addr;
		int s2 = size - s1;

		/* it does, so first copy 's1' bytes from 'data' to 'addr' */
		dst = (u8*)PSM(addr);
		if (dst == NULL) return -1;
		Cpu->Clear(addr, s1/4);
		memcpy_fast(dst, data, s1);

		/* and second copy 's2' bytes from '&data[s1]' to 'maddr' */
		dst = (u8*)PSM(psHu32(DMAC_RBOR));
		if (dst == NULL) return -1;
		Cpu->Clear(psHu32(DMAC_RBOR), s2/4);
		memcpy_fast(dst, &data[s1], s2);
	} else {
		//u32 * tempptr, * tempptr2;

		/* it doesn't, so just copy 'size' bytes from 'data' to 'addr' */
		dst = (u8*)PSM(addr);
		if (dst == NULL) return -1;
		Cpu->Clear(addr, size/4);
		memcpy_fast(dst, data, size);
	}
	

	return 0;
}


int hwDmacSrcChainWithStack(DMACh *dma, int id) {
	u32 temp;

	switch (id) {
		case 0: // Refe - Transfer Packet According to ADDR field
			//dma->tadr += 16;
			return 1;										//End Transfer

		case 1: // CNT - Transfer QWC following the tag.
			dma->madr = dma->tadr + 16;						//Set MADR to QW after Tag            
			dma->tadr = dma->madr + (dma->qwc << 4);			//Set TADR to QW following the data
			return 0;

		case 2: // Next - Transfer QWC following tag. TADR = ADDR
			temp = dma->madr;								//Temporarily Store ADDR
			dma->madr = dma->tadr + 16; 					  //Set MADR to QW following the tag
			dma->tadr = temp;								//Copy temporarily stored ADDR to Tag
			return 0;

		case 3: // Ref - Transfer QWC from ADDR field
		case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
			dma->tadr += 16;									//Set TADR to next tag
			return 0;

		case 5: // Call - Transfer QWC following the tag, save succeeding tag
			temp = dma->madr;								//Temporarily Store ADDR
															
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			
			if ((dma->chcr & 0x30) == 0x0) {						//Check if ASR0 is empty
				dma->asr0 = dma->madr + (dma->qwc << 4);			//If yes store Succeeding tag
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x10; //1 Address in call stack
			}else if((dma->chcr & 0x30) == 0x10){
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x20; //2 Addresses in call stack
				dma->asr1 = dma->madr + (dma->qwc << 4);	//If no store Succeeding tag in ASR1
			}else {
				SysPrintf("Call Stack Overflow (report if it fixes/breaks anything)\n");
				return 1;										//Return done
			}
			dma->tadr = temp;								//Set TADR to temporarily stored ADDR
											
			return 0;

		case 6: // Ret - Transfer QWC following the tag, load next tag
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag

			if ((dma->chcr & 0x30) == 0x20) {							//If ASR1 is NOT equal to 0 (Contains address)
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x10; //1 Address left in call stack
				dma->tadr = dma->asr1;						//Read ASR1 as next tag
				dma->asr1 = 0;								//Clear ASR1
			} else {										//If ASR1 is empty (No address held)
				if((dma->chcr & 0x30) == 0x10) {						   //Check if ASR0 is NOT equal to 0 (Contains address)
					dma->chcr = (dma->chcr & 0xffffffcf);  //No addresses left in call stack
					dma->tadr = dma->asr0;					//Read ASR0 as next tag
					dma->asr0 = 0;							//Clear ASR0
				} else {									//Else if ASR1 and ASR0 are empty
					//dma->tadr += 16;						   //Clear tag address - Kills Klonoa 2
					return 1;								//End Transfer
				}
			}
			return 0;

		case 7: // End - Transfer QWC following the tag 
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			//comment out tadr fixes lemans
			//dma->tadr = dma->madr + (dma->qwc << 4);			//Dont Increment tag, breaks Soul Calibur II and III
			return 1;										//End Transfer
	}

	return -1;
}

int hwDmacSrcChain(DMACh *dma, int id) {
	u32 temp;

	switch (id) {
		case 0: // Refe - Transfer Packet According to ADDR field
			//dma->tadr += 16;
			return 1;										//End Transfer

		case 1: // CNT - Transfer QWC following the tag.
			dma->madr = dma->tadr + 16;						//Set MADR to QW after Tag            
			dma->tadr = dma->madr + (dma->qwc << 4);			//Set TADR to QW following the data
			return 0;

		case 2: // Next - Transfer QWC following tag. TADR = ADDR
			temp = dma->madr;								//Temporarily Store ADDR
			dma->madr = dma->tadr + 16; 					  //Set MADR to QW following the tag
			dma->tadr = temp;								//Copy temporarily stored ADDR to Tag
			return 0;

		case 3: // Ref - Transfer QWC from ADDR field
		case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
			dma->tadr += 16;									//Set TADR to next tag
			return 0;

		case 7: // End - Transfer QWC following the tag
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			//dma->tadr = dma->madr + (dma->qwc << 4);			//Dont Increment tag, breaks Soul Calibur II and III
			return 1;										//End Transfer
	}

	return -1;
}

// Original hwRead/Write32 functions .. left in for now, for troubleshooting purposes.
#if 0
mem32_t __fastcall hwRead32(u32 mem)
{
	// *Performance Warning*  This function is called -A-LOT.  Be weary when making changes.  It
	// could impact FPS significantly.

	// Optimization Note:
	// Shortcut for the INTC_STAT register, which is checked *very* frequently as part of the EE's
	// vsynch timers.  INTC_STAT has the disadvantage of being in the 0x1000f000 case, which has
	// a lot of additional registers in it, and combined with it's call frequency is a bad thing.

	if(mem == INTC_STAT)
	{
		// This one is checked alot, so leave it commented out unless you love 600 meg logfiles.
		//HW_LOG("DMAC_STAT Read  32bit %x\n", psHu32(0xe010));
		return psHu32(INTC_STAT);
	}

	const u16 masked_mem = mem & 0xffff;

	// We optimize the hw register reads by breaking them into manageable 4k chunks (for a total of
	// 16 cases spanning the 64k PS2 hw register memory map).  It helps also that the EE is, for
	// the most part, designed so that various classes of registers are sectioned off into these
	// 4k segments.

	// Notes: Breaks from the switch statement will return a standard hw memory read.
	// Special case handling of reads should use "return" directly.

	switch( masked_mem>>12 )		// switch out as according to the 4k page of the access.
	{
		// Counters Registers
		// This code uses some optimized trickery to produce more compact output.
		// See below for the "reference" block to get a better idea what this code does. :)

		case 0x0:		// counters 0 and 1
		case 0x1:		// counters 2 and 3
		{
			const uint cntidx = masked_mem >> 11;	// neat trick to scale the counter HW address into 0-3 range.
			switch( (masked_mem>>4) & 0xf )
			{
				case 0x0: return (u16)rcntRcount(cntidx);
				case 0x1: return (u16)counters[cntidx].modeval;
				case 0x2: return (u16)counters[cntidx].target;
				case 0x3: return (u16)counters[cntidx].hold;
			}
		}

#if 0	// Counters Reference Block (original case setup)
		case 0x10000000: return (u16)rcntRcount(0);
		case 0x10000010: return (u16)counters[0].modeval;
		case 0x10000020: return (u16)counters[0].target;
		case 0x10000030: return (u16)counters[0].hold;

		case 0x10000800: return (u16)rcntRcount(1);
		case 0x10000810: return (u16)counters[1].modeval;
		case 0x10000820: return (u16)counters[1].target;
		case 0x10000830: return (u16)counters[1].hold;

		case 0x10001000: return (u16)rcntRcount(2);
		case 0x10001010: return (u16)counters[2].modeval;
		case 0x10001020: return (u16)counters[2].target;

		case 0x10001800: return (u16)rcntRcount(3);
		case 0x10001810: return (u16)counters[3].modeval;
		case 0x10001820: return (u16)counters[3].target;
#endif

		break;

		case 0x2: return ipuRead32( mem );

		case 0xf:
			switch( (masked_mem >> 4) & 0xff )
			{
				case 0x01:
					HW_LOG("INTC_MASK Read32, value=0x%x", psHu32(INTC_MASK));
				break;

				case 0x13:	// 0x1000f130
				case 0x26:	// 0x1000f260 SBUS?
				case 0x41:	// 0x1000f410
				case 0x43:	// MCH_RICM
					return 0;

				case 0x24:	// 0x1000f240: SBUS
					return psHu32(0xf240) | 0xF0000102;

				case 0x44:	// 0x1000f440: MCH_DRD

					if( !((psHu32(0xf430) >> 6) & 0xF) )
					{
						switch ((psHu32(0xf430)>>16) & 0xFFF)
						{
							//MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5

							case 0x21://INIT
								if(rdram_sdevid < rdram_devices)
								{
									rdram_sdevid++;
									return 0x1F;
								}
							return 0;

							case 0x23://CNFGA
								return 0x0D0D;	//PVER=3 | MVER=16 | DBL=1 | REFBIT=5

							case 0x24://CNFGB
								//0x0110 for PSX  SVER=0 | CORG=8(5x9x7) | SPT=1 | DEVTYP=0 | BYTE=0
								return 0x0090;	//SVER=0 | CORG=4(5x9x6) | SPT=1 | DEVTYP=0 | BYTE=0

							case 0x40://DEVID
								return psHu32(0xf430) & 0x1F;	// =SDEV
						}
					}
					return 0;
			}
			break;

		///////////////////////////////////////////////////////
		// Most of the following case handlers are for developer builds only (logging).
		// It'll all optimize to ziltch in public release builds.

		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		{
			const char* regName = "Unknown";

			switch( mem )
			{
				case D2_CHCR: regName = "DMA2_CHCR"; break;
				case D2_MADR: regName = "DMA2_MADR"; break;
				case D2_QWC: regName = "DMA2_QWC"; break;
				case D2_TADR: regName = "DMA2_TADDR"; break;
				case D2_ASR0: regName = "DMA2_ASR0"; break;
				case D2_ASR1: regName = "DMA2_ASR1"; break;
				case D2_SADR: regName = "DMA2_SADDR"; break;
			}

			HW_LOG( "Hardware Read32 at 0x%x (%s), value=0x%x\n", mem, regName, psHu32(mem) );
		}
		break;

		case 0x0b:
			if( mem == D4_CHCR )
				HW_LOG("Hardware Read32 at 0x%x (IPU1:DMA4_CHCR), value=0x%x\n", mem, psHu32(mem));
		break;

		case 0x0c:
		case 0x0d:
		case 0x0e:
			if( mem == DMAC_STAT )
				HW_LOG("DMAC_STAT Read32, value=0x%x\n", psHu32(DMAC_STAT));
		break;

		jNO_DEFAULT;
	}

	// Optimization note: We masked 'mem' earlier, so it's safe to access PS2MEM_HW directly.
	// (checked disasm, and MSVC 2008 fails to optimize it on its own)

	//return psHu32(mem);
	return *((u32*)&PS2MEM_HW[masked_mem]);
}


__forceinline void __fastcall hwWrite32(u32 mem, u32 value)
{

	if ((mem>=0x10002000) && (mem<0x10003000)) { //IPU regs
		ipuWrite32(mem,value);
		return;
	}
	if ((mem>=0x10003800) && (mem<0x10003c00)) {
		vif0Write32(mem, value); 
		return;
	}
	if ((mem>=0x10003c00) && (mem<0x10004000)) {
		vif1Write32(mem, value); 
		return;
	}

	switch (mem) {
		case 0x10000000: rcntWcount(0, value); break;
		case 0x10000010: rcntWmode(0, value); break;
		case 0x10000020: rcntWtarget(0, value); break;
		case 0x10000030: rcntWhold(0, value); break;

		case 0x10000800: rcntWcount(1, value); break;
		case 0x10000810: rcntWmode(1, value); break;
		case 0x10000820: rcntWtarget(1, value); break;
		case 0x10000830: rcntWhold(1, value); break;

		case 0x10001000: rcntWcount(2, value); break;
		case 0x10001010: rcntWmode(2, value); break;
		case 0x10001020: rcntWtarget(2, value); break;

		case 0x10001800: rcntWcount(3, value); break;
		case 0x10001810: rcntWmode(3, value); break;
		case 0x10001820: rcntWtarget(3, value); break;

		case GIF_CTRL:
			//SysPrintf("GIF_CTRL write %x\n", value);
			psHu32(mem) = value & 0x8;
			if (value & 0x1) gsGIFReset();
			else if( value & 8 ) psHu32(GIF_STAT) |= 8;
			else psHu32(GIF_STAT) &= ~8;
			return;

		case GIF_MODE:
			// need to set GIF_MODE (hamster ball)
			psHu32(GIF_MODE) = value;
			if (value & 0x1) psHu32(GIF_STAT)|= 0x1;
			else psHu32(GIF_STAT)&= ~0x1;
			if (value & 0x4) psHu32(GIF_STAT)|= 0x4;
			else psHu32(GIF_STAT)&= ~0x4;
			break;

		case GIF_STAT: // stat is readonly
			SysPrintf("Gifstat write value = %x\n", value);
			return;

		case 0x10008000: // dma0 - vif0
			DMA_LOG("VIF0dma %lx\n", value);
			DmaExec(dmaVIF0, mem, value);
			break;
//------------------------------------------------------------------
		case 0x10009000: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma CHCR %lx\n", value);
			DmaExec(dmaVIF1, mem, value);
			break;
#ifdef PCSX2_DEVBUILD
		case 0x10009010: // dma1 - vif1 - madr
			HW_LOG("VIF1dma Madr %lx\n", value);
			psHu32(mem) = value;//dma1 madr
			break;
		case 0x10009020: // dma1 - vif1 - qwc
			HW_LOG("VIF1dma QWC %lx\n", value);
			psHu32(mem) = value;//dma1 qwc
			break;
		case 0x10009030: // dma1 - vif1 - tadr
			HW_LOG("VIF1dma TADR %lx\n", value);
			psHu32(mem) = value;//dma1 tadr
			break;
		case 0x10009040: // dma1 - vif1 - asr0
			HW_LOG("VIF1dma ASR0 %lx\n", value);
			psHu32(mem) = value;//dma1 asr0
			break;
		case 0x10009050: // dma1 - vif1 - asr1
			HW_LOG("VIF1dma ASR1 %lx\n", value);
			psHu32(mem) = value;//dma1 asr1
			break;
		case 0x10009080: // dma1 - vif1 - sadr
			HW_LOG("VIF1dma SADR %lx\n", value);
			psHu32(mem) = value;//dma1 sadr
			break;
#endif
//------------------------------------------------------------------
		case 0x1000a000: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite32: GSdma %lx\n", cpuRegs.cycle, value);
			DmaExec(dmaGIF, mem, value);
			break;
#ifdef PCSX2_DEVBUILD
	    case 0x1000a010:
		    psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write DMA2_MADR 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a020:
            psHu32(mem) = value;//dma2 qwc
		    HW_LOG("Hardware write DMA2_QWC 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a030:
            psHu32(mem) = value;//dma2 taddr
		    HW_LOG("Hardware write DMA2_TADDR 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a040:
            psHu32(mem) = value;//dma2 asr0
		    HW_LOG("Hardware write DMA2_ASR0 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a050:
            psHu32(mem) = value;//dma2 asr1
		    HW_LOG("Hardware write DMA2_ASR1 32bit at %x with value %x\n",mem,value);
		    break;
	    case 0x1000a080:
            psHu32(mem) = value;//dma2 saddr
		    HW_LOG("Hardware write DMA2_SADDR 32bit at %x with value %x\n",mem,value);
		    break;
#endif
//------------------------------------------------------------------
		case 0x1000b000: // dma3 - fromIPU
			DMA_LOG("IPU0dma %lx\n", value);
			DmaExec(dmaIPU0, mem, value);
			break;
//------------------------------------------------------------------
#ifdef PCSX2_DEVBUILD
		case 0x1000b010:
	   		psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_MADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b020:
    		psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_QWC 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b030:
			psHu32(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU0DMA_TADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b080:
			psHu32(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU0DMA_SADDR 32bit at %x with value %x\n",mem,value);
			break;
#endif
//------------------------------------------------------------------
		case 0x1000b400: // dma4 - toIPU
			DMA_LOG("IPU1dma %lx\n", value);
			DmaExec(dmaIPU1, mem, value);
			break;
//------------------------------------------------------------------
#ifdef PCSX2_DEVBUILD
		case 0x1000b410:
    		psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_MADR 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b420:
    		psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_QWC 32bit at %x with value %x\n",mem,value);
       		break;
		case 0x1000b430:
			psHu32(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU1DMA_TADR 32bit at %x with value %x\n",mem,value);
			break;
		case 0x1000b480:
			psHu32(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU1DMA_SADDR 32bit at %x with value %x\n",mem,value);
			break;
#endif
//------------------------------------------------------------------
		case 0x1000c000: // dma5 - sif0
			DMA_LOG("SIF0dma %lx\n", value);
			//if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec(dmaSIF0, mem, value);
			break;
//------------------------------------------------------------------
		case 0x1000c400: // dma6 - sif1
			DMA_LOG("SIF1dma %lx\n", value);
			DmaExec(dmaSIF1, mem, value);
			break;
#ifdef PCSX2_DEVBUILD
		case 0x1000c420: // dma6 - sif1 - qwc
			HW_LOG("SIF1dma QWC = %lx\n", value);
			psHu32(mem) = value;
			break;
		case 0x1000c430: // dma6 - sif1 - tadr
			HW_LOG("SIF1dma TADR = %lx\n", value);
			psHu32(mem) = value;
			break;
#endif
//------------------------------------------------------------------
		case 0x1000c800: // dma7 - sif2
			DMA_LOG("SIF2dma %lx\n", value);
			DmaExec(dmaSIF2, mem, value);
			break;
//------------------------------------------------------------------
		case 0x1000d000: // dma8 - fromSPR
			DMA_LOG("fromSPRdma %lx\n", value);
			DmaExec(dmaSPR0, mem, value);
			break;
//------------------------------------------------------------------
		case 0x1000d400: // dma9 - toSPR
			DMA_LOG("toSPRdma %lx\n", value);
			DmaExec(dmaSPR1, mem, value);
			break;
//------------------------------------------------------------------
		case 0x1000e000: // DMAC_CTRL
			HW_LOG("DMAC_CTRL Write 32bit %x\n", value);
			psHu32(0xe000) = value;
			break;

		case 0x1000e010: // DMAC_STAT
			HW_LOG("DMAC_STAT Write 32bit %x\n", value);
			psHu16(0xe010)&= ~(value & 0xffff); // clear on 1
			psHu16(0xe012) ^= (u16)(value >> 16);

			cpuTestDMACInts();
			break;
//------------------------------------------------------------------
		case 0x1000f000: // INTC_STAT
			HW_LOG("INTC_STAT Write 32bit %x\n", value);
			psHu32(0xf000)&=~value;	
			//cpuTestINTCInts();
			break;

		case 0x1000f010: // INTC_MASK
			HW_LOG("INTC_MASK Write 32bit %x\n", value);
			psHu32(0xf010) ^= (u16)value;
			cpuTestINTCInts();
			break;
//------------------------------------------------------------------			
		case 0x1000f430://MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
			if ((((value >> 16) & 0xFFF) == 0x21) && (((value >> 6) & 0xF) == 1) && (((psHu32(0xf440) >> 7) & 1) == 0))//INIT & SRP=0
				rdram_sdevid = 0;	// if SIO repeater is cleared, reset sdevid
			psHu32(mem) = value & ~0x80000000;	//kill the busy bit
			break;

		case 0x1000f440://MCH_DRD:
			psHu32(mem) = value;
			break;
//------------------------------------------------------------------
		case 0x1000f590: // DMAC_ENABLEW
			HW_LOG("DMAC_ENABLEW Write 32bit %lx\n", value);
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
			return;
//------------------------------------------------------------------
		case 0x1000f200:
			psHu32(mem) = value;
			break;
		case 0x1000f220:
			psHu32(mem) |= value;
			break;
		case 0x1000f230:
			psHu32(mem) &= ~value;
			break;
		case 0x1000f240:
			if(!(value & 0x100))
				psHu32(mem) &= ~0x100;
			else
				psHu32(mem) |= 0x100;
			break;
		case 0x1000f260:
			psHu32(mem) = 0;
			break;
//------------------------------------------------------------------
		case 0x1000f130:
		case 0x1000f410:
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)\n", mem, value, cpuRegs.CP0.n.Status.val);
			break;
//------------------------------------------------------------------
		default:
			psHu32(mem) = value;
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)\n", mem, value, cpuRegs.CP0.n.Status.val);
		break;
	}
}

#endif

#if 0
__forceinline void hwWrite64(u32 mem, u64 value)
{
	u32 val32;
	int i;

	if ((mem>=0x10002000) && (mem<=0x10002030)) {
		ipuWrite64(mem, value);
		return;
	}

	if ((mem>=0x10003800) && (mem<0x10003c00)) {
		vif0Write32(mem, value); return;
	}
	if ((mem>=0x10003c00) && (mem<0x10004000)) {
		vif1Write32(mem, value); return;
	}

	switch (mem) {
		case GIF_CTRL:
			DevCon::Status("GIF_CTRL write 64", params value);
			psHu32(mem) = value & 0x8;
			if(value & 0x1) {
				gsGIFReset();
				//gsReset();
			}
			else {
				if( value & 8 ) psHu32(GIF_STAT) |= 8;
				else psHu32(GIF_STAT) &= ~8;
			}
	
			return;

		case GIF_MODE:
#ifdef GSPATH3FIX
			Console::Status("GIFMODE64 %x\n", params value);
#endif
			psHu64(GIF_MODE) = value;
			if (value & 0x1) psHu32(GIF_STAT)|= 0x1;
			else psHu32(GIF_STAT)&= ~0x1;
			if (value & 0x4) psHu32(GIF_STAT)|= 0x4;
			else psHu32(GIF_STAT)&= ~0x4;
			break;

		case GIF_STAT: // stat is readonly
			return;

		case 0x1000a000: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite64: GSdma %lx\n", cpuRegs.cycle, value);
			DmaExec(dmaGIF, mem, value);
			break;

		case 0x1000e000: // DMAC_CTRL
			HW_LOG("DMAC_CTRL Write 64bit %x\n", value);
			psHu64(mem) = value;
			break;

		case 0x1000e010: // DMAC_STAT
			HW_LOG("DMAC_STAT Write 64bit %x\n", value);
			val32 = (u32)value;
			psHu16(0xe010)&= ~(val32 & 0xffff); // clear on 1
			val32 = val32 >> 16;
			for (i=0; i<16; i++) { // reverse on 1
				if (val32 & (1<<i)) {
					if (psHu16(0xe012) & (1<<i))
						psHu16(0xe012)&= ~(1<<i);
					else
						psHu16(0xe012)|= 1<<i;
				}
			}
            cpuTestDMACInts();
			break;

		case 0x1000f590: // DMAC_ENABLEW
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
			break;

		case 0x1000f000: // INTC_STAT
			HW_LOG("INTC_STAT Write 64bit %x\n", value);
			psHu32(INTC_STAT)&=~value;	
			cpuTestINTCInts();
			break;

		case 0x1000f010: // INTC_MASK
			HW_LOG("INTC_MASK Write 32bit %x\n", value);
			for (i=0; i<16; i++) { // reverse on 1
                const int s = (1<<i);
				if (value & s) {
					if (psHu32(INTC_MASK) & s)
						psHu32(INTC_MASK)&= ~s;
					else
						psHu32(INTC_MASK)|= s;
				}
			}
			cpuTestINTCInts();
			break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		default:
			psHu64(mem) = value;

			HW_LOG("Unknown Hardware write 64 at %x with value %x (status=%x)\n",mem,value, cpuRegs.CP0.n.Status.val);
			break;
	}
}

__forceinline void hwWrite128(u32 mem, const u64 *value)
{
	if (mem >= 0x10004000 && mem < 0x10008000) {
		WriteFIFO(mem, value); return;
	}

	switch (mem) {
		case 0x1000f590: // DMAC_ENABLEW
			psHu32(0xf590) = *(u32*)value;
			psHu32(0xf520) = *(u32*)value;
			break;
		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		default:

			psHu64(mem  ) = value[0];
			psHu64(mem+8) = value[1];

			HW_LOG("Unknown Hardware write 128 at %x with value %x_%x (status=%x)\n", mem, value[1], value[0], cpuRegs.CP0.n.Status.val);
			break;
	}
}
#endif