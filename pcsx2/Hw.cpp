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

__forceinline void  intcInterrupt()
{
	if ((cpuRegs.CP0.n.Status.val & 0x400) != 0x400) return;

	if ((psHu32(INTC_STAT)) == 0) {
		DevCon::Notice("*PCSX2*: intcInterrupt already cleared");
        return;
	}
	if ((psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0) return;

	HW_LOG("intcInterrupt %x", psHu32(INTC_STAT) & psHu32(INTC_MASK));
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
	
	HW_LOG("dmacInterrupt %x", (psHu16(0xe012) & psHu16(0xe010) || 
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
		memcpy_fast(dst, data, s1);

		/* and second copy 's2' bytes from '&data[s1]' to 'maddr' */
		dst = (u8*)PSM(psHu32(DMAC_RBOR));
		if (dst == NULL) return -1;
		memcpy_fast(dst, &data[s1], s2);
	} 
	else {
		/* it doesn't, so just copy 'size' bytes from 'data' to 'addr' */
		dst = (u8*)PSM(addr);
		if (dst == NULL) return -1;
		memcpy_fast(dst, data, size);
	}

	return 0;
}


bool hwDmacSrcChainWithStack(DMACh *dma, int id) {
	u32 temp;

	switch (id) {
		case 0: // Refe - Transfer Packet According to ADDR field
			return true;										//End Transfer

		case 1: // CNT - Transfer QWC following the tag.
			dma->madr = dma->tadr + 16;						//Set MADR to QW after Tag            
			dma->tadr = dma->madr + (dma->qwc << 4);			//Set TADR to QW following the data
			return false;

		case 2: // Next - Transfer QWC following tag. TADR = ADDR
			temp = dma->madr;								//Temporarily Store ADDR
			dma->madr = dma->tadr + 16; 					  //Set MADR to QW following the tag
			dma->tadr = temp;								//Copy temporarily stored ADDR to Tag
			return false;

		case 3: // Ref - Transfer QWC from ADDR field
		case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
			dma->tadr += 16;									//Set TADR to next tag
			return false;

		case 5: // Call - Transfer QWC following the tag, save succeeding tag
			temp = dma->madr;								//Temporarily Store ADDR
															
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			
			if ((dma->chcr & 0x30) == 0x0) {						//Check if ASR0 is empty
				dma->asr0 = dma->madr + (dma->qwc << 4);			//If yes store Succeeding tag
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x10; //1 Address in call stack
			}
			else if((dma->chcr & 0x30) == 0x10){
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x20; //2 Addresses in call stack
				dma->asr1 = dma->madr + (dma->qwc << 4);	//If no store Succeeding tag in ASR1
			}else {
				Console::Notice("Call Stack Overflow (report if it fixes/breaks anything)");
				return true;										//Return done
			}
			dma->tadr = temp;								//Set TADR to temporarily stored ADDR
											
			return false;

		case 6: // Ret - Transfer QWC following the tag, load next tag
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag

			if ((dma->chcr & 0x30) == 0x20) {							//If ASR1 is NOT equal to 0 (Contains address)
				dma->chcr = (dma->chcr & 0xffffffcf) | 0x10; //1 Address left in call stack
				dma->tadr = dma->asr1;						//Read ASR1 as next tag
				dma->asr1 = 0;								//Clear ASR1
			} 
			else {										//If ASR1 is empty (No address held)
				if((dma->chcr & 0x30) == 0x10) {						   //Check if ASR0 is NOT equal to 0 (Contains address)
					dma->chcr = (dma->chcr & 0xffffffcf);  //No addresses left in call stack
					dma->tadr = dma->asr0;					//Read ASR0 as next tag
					dma->asr0 = 0;							//Clear ASR0
				} else {									//Else if ASR1 and ASR0 are empty
					//dma->tadr += 16;						   //Clear tag address - Kills Klonoa 2
					return 1;								//End Transfer
				}
			}
			return false;

		case 7: // End - Transfer QWC following the tag 
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			//Dont Increment tadr, breaks Soul Calibur II and III
			return true;										//End Transfer
	}

	return false;
}

bool hwDmacSrcChain(DMACh *dma, int id) {
	u32 temp;

	switch (id) {
		case 0: // Refe - Transfer Packet According to ADDR field
			return true;										//End Transfer

		case 1: // CNT - Transfer QWC following the tag.
			dma->madr = dma->tadr + 16;						//Set MADR to QW after Tag            
			dma->tadr = dma->madr + (dma->qwc << 4);			//Set TADR to QW following the data
			return false;

		case 2: // Next - Transfer QWC following tag. TADR = ADDR
			temp = dma->madr;								//Temporarily Store ADDR
			dma->madr = dma->tadr + 16; 					  //Set MADR to QW following the tag
			dma->tadr = temp;								//Copy temporarily stored ADDR to Tag
			return false;

		case 3: // Ref - Transfer QWC from ADDR field
		case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
			dma->tadr += 16;									//Set TADR to next tag
			return false;

		case 7: // End - Transfer QWC following the tag
			dma->madr = dma->tadr + 16;						//Set MADR to data following the tag
			//Dont Increment tadr, breaks Soul Calibur II and III
			return true;										//End Transfer
	}

	return false;
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
		// 0x10000000 - 0x10000030
		case RCNT0_COUNT: return (u16)rcntRcount(0);
		case RCNT0_MODE: return (u16)counters[0].modeval;
		case RCNT0_TARGET: return (u16)counters[0].target;
		case RCNT0_HOLD: return (u16)counters[0].hold;

		// 0x10000800 - 0x10000830
		case RCNT1_COUNT: return (u16)rcntRcount(1);
		case RCNT1_MODE: return (u16)counters[1].modeval;
		case RCNT1_TARGET: return (u16)counters[1].target;
		case RCNT1_HOLD: return (u16)counters[1].hold;

		// 0x10001000 - 0x10001020
		case RCNT2_COUNT: return (u16)rcntRcount(2);
		case RCNT2_MODE: return (u16)counters[2].modeval;
		case RCNT2_TARGET: return (u16)counters[2].target;

		// 0x10001800 - 0x10001820
		case RCNT3_COUNT: return (u16)rcntRcount(3);
		case RCNT3_MODE: return (u16)counters[3].modeval;
		case RCNT3_TARGET: return (u16)counters[3].target;
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

			switch (mem)
			{
				case D2_CHCR: regName = "DMA2_CHCR"; break;
				case D2_MADR: regName = "DMA2_MADR"; break;
				case D2_QWC: regName = "DMA2_QWC"; break;
				case D2_TADR: regName = "DMA2_TADDR"; break;
				case D2_ASR0: regName = "DMA2_ASR0"; break;
				case D2_ASR1: regName = "DMA2_ASR1"; break;
				case D2_SADR: regName = "DMA2_SADDR"; break;
			}

			HW_LOG( "Hardware Read32 at 0x%x (%s), value=0x%x", mem, regName, psHu32(mem) );
		}
		break;

		case 0x0b:
			if( mem == D4_CHCR )
				HW_LOG("Hardware Read32 at 0x%x (IPU1:DMA4_CHCR), value=0x%x", mem, psHu32(mem));
		break;

		case 0x0c:
		case 0x0d:
		case 0x0e:
			if( mem == DMAC_STAT )
				HW_LOG("DMAC_STAT Read32, value=0x%x", psHu32(DMAC_STAT));
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
	// Would ((mem >= IPU_CMD) && (mem <= IPU_TOP)) be better? -arcum42
	if ((mem >= IPU_CMD) && (mem < GIF_CTRL)) { //IPU regs
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
		case RCNT0_COUNT: rcntWcount(0, value); break;
		case RCNT0_MODE: rcntWmode(0, value); break;
		case RCNT0_TARGET: rcntWtarget(0, value); break;
		case RCNT0_TARGET: rcntWhold(0, value); break;

		case RCNT1_COUNT: rcntWcount(1, value); break;
		case RCNT1_MODE: rcntWmode(1, value); break;
		case RCNT1_TARGET: rcntWtarget(1, value); break;
		case RCNT1_HOLD: rcntWhold(1, value); break;

		case RCNT2_COUNT: rcntWcount(2, value); break;
		case RCNT2_MODE: rcntWmode(2, value); break;
		case RCNT2_TARGET: rcntWtarget(2, value); break;

		case RCNT3_COUNT: rcntWcount(3, value); break;
		case RCNT3_MODE: rcntWmode(3, value); break;
		case RCNT3_TARGET: rcntWtarget(3, value); break;

		case GIF_CTRL:
			//Console::WriteLn("GIF_CTRL write %x", params value);
			psHu32(mem) = value & 0x8;
		
			if (value & 0x1) 
				gsGIFReset();
			else if( value & 8 ) 
				psHu32(GIF_STAT) |= 8;
			else 
				psHu32(GIF_STAT) &= ~8;
			
			return;

		case GIF_MODE:
			// need to set GIF_MODE (hamster ball)
			psHu32(GIF_MODE) = value;
		
			if (value & 0x1) 
				psHu32(GIF_STAT)|= 0x1;
			else 
				psHu32(GIF_STAT)&= ~0x1;
			
			if (value & 0x4) 
				psHu32(GIF_STAT)|= 0x4;
			else 
				psHu32(GIF_STAT)&= ~0x4;
		
			break;

		case GIF_STAT: // stat is readonly
			Console::WriteLn("Gifstat write value = %x", params value);
			return;

		case D0_CHCR: // dma0 - vif0
			DMA_LOG("VIF0dma %lx", value);
			DmaExec(dmaVIF0, mem, value);
			break;
		
		case D1_CHCR: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma CHCR %lx", value);
			DmaExec(dmaVIF1, mem, value);
			break;
		
#ifdef PCSX2_DEVBUILD
		case D1_MADR: // dma1 - vif1 - madr
			HW_LOG("VIF1dma Madr %lx", value);
			psHu32(mem) = value;//dma1 madr
			break;
		
		case D1_QWC: // dma1 - vif1 - qwc
			HW_LOG("VIF1dma QWC %lx", value);
			psHu32(mem) = value;//dma1 qwc
			break;
		
		case D1_TADR: // dma1 - vif1 - tadr
			HW_LOG("VIF1dma TADR %lx", value);
			psHu32(mem) = value;//dma1 tadr
			break;
		
		case D1_ASR0: // dma1 - vif1 - asr0
			HW_LOG("VIF1dma ASR0 %lx", value);
			psHu32(mem) = value;//dma1 asr0
			break;
		
		case D1_ASR1: // dma1 - vif1 - asr1
			HW_LOG("VIF1dma ASR1 %lx", value);
			psHu32(mem) = value;//dma1 asr1
			break;
		
		case D1_SADR: // dma1 - vif1 - sadr
			HW_LOG("VIF1dma SADR %lx", value);
			psHu32(mem) = value;//dma1 sadr
			break;
#endif
		
		case D2_CHCR: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite32: GSdma %lx", cpuRegs.cycle, value);
			DmaExec(dmaGIF, mem, value);
			break;
		
#ifdef PCSX2_DEVBUILD
		case D2_MADR:
			psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write DMA2_MADR 32bit at %x with value %x",mem,value);
			break;
	    
		case D2_QWC:
			psHu32(mem) = value;//dma2 qwc
			HW_LOG("Hardware write DMA2_QWC 32bit at %x with value %x",mem,value);
			break;
	    
		case D2_TADR:
			psHu32(mem) = value;//dma2 taddr
			HW_LOG("Hardware write DMA2_TADDR 32bit at %x with value %x",mem,value);
			break;
		
		case D2_ASR0:
			psHu32(mem) = value;//dma2 asr0
			HW_LOG("Hardware write DMA2_ASR0 32bit at %x with value %x",mem,value);
			break;
		
		case D2_ASR1:
			psHu32(mem) = value;//dma2 asr1
			HW_LOG("Hardware write DMA2_ASR1 32bit at %x with value %x",mem,value);
			break;
		
		case D2_SADR:
			psHu32(mem) = value;//dma2 saddr
			HW_LOG("Hardware write DMA2_SADDR 32bit at %x with value %x",mem,value);
			break;
#endif
		
		case D3_CHCR: // dma3 - fromIPU
			DMA_LOG("IPU0dma %lx", value);
			DmaExec(dmaIPU0, mem, value);
			break;
		
#ifdef PCSX2_DEVBUILD
		case D3_MADR:
	   		psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_MADR 32bit at %x with value %x",mem,value);
			break;
		
		case D3_QWC:
			psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_QWC 32bit at %x with value %x",mem,value);
			break;
		
		case D3_TADR:
			psHu32(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU0DMA_TADR 32bit at %x with value %x",mem,value);
			break;
		
		case D3_SADR:
			psHu32(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU0DMA_SADDR 32bit at %x with value %x",mem,value);
			break;
#endif
		
		case D4_CHCR: // dma4 - toIPU
			DMA_LOG("IPU1dma %lx", value);
			DmaExec(dmaIPU1, mem, value);
			break;
		
#ifdef PCSX2_DEVBUILD
		case D4_MADR:
			psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_MADR 32bit at %x with value %x",mem,value);
			break;
		
		case D4_QWC:
			psHu32(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_QWC 32bit at %x with value %x",mem,value);
			break;
		
		case D4_TADR:
			psHu32(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU1DMA_TADR 32bit at %x with value %x",mem,value);
			break;
		
		case D4_SADR:
			psHu32(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU1DMA_SADDR 32bit at %x with value %x",mem,value);
			break;
#endif
		case D5_CHCR: // dma5 - sif0
			DMA_LOG("SIF0dma %lx", value);
			DmaExec(dmaSIF0, mem, value);
			break;
		
		case D6_CHCR: // dma6 - sif1
			DMA_LOG("SIF1dma %lx", value);
			DmaExec(dmaSIF1, mem, value);
			break;
		
#ifdef PCSX2_DEVBUILD
		case D6_QWC: // dma6 - sif1 - qwc
			HW_LOG("SIF1dma QWC = %lx", value);
			psHu32(mem) = value;
			break;
		
		case 0x1000c430: // dma6 - sif1 - tadr
			HW_LOG("SIF1dma TADR = %lx", value);
			psHu32(mem) = value;
			break;
#endif
		case D7_CHCR: // dma7 - sif2
			DMA_LOG("SIF2dma %lx", value);
			DmaExec(dmaSIF2, mem, value);
			break;
		
		case D8_CHCR: // dma8 - fromSPR
			DMA_LOG("fromSPRdma %lx", value);
			DmaExec(dmaSPR0, mem, value);
			break;
		
		case 0x1000d400: // dma9 - toSPR
			DMA_LOG("toSPRdma %lx", value);
			DmaExec(dmaSPR1, mem, value);
			break;
		
		case DMAC_CTRL: // DMAC_CTRL
			HW_LOG("DMAC_CTRL Write 32bit %x", value);
			psHu32(0xe000) = value;
			break;

		case DMAC_STAT: // DMAC_STAT
			HW_LOG("DMAC_STAT Write 32bit %x", value);
			psHu16(0xe010)&= ~(value & 0xffff); // clear on 1
			psHu16(0xe012) ^= (u16)(value >> 16);

			cpuTestDMACInts();
			break;
		
		case INTC_STAT: // INTC_STAT
			HW_LOG("INTC_STAT Write 32bit %x", value);
			psHu32(0xf000)&=~value;	
			break;

		case INTC_MASK: // INTC_MASK
			HW_LOG("INTC_MASK Write 32bit %x", value);
			psHu32(0xf010) ^= (u16)value;
			cpuTestINTCInts();
			break;
		
		case 0x1000f430://MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
			if ((((value >> 16) & 0xFFF) == 0x21) && (((value >> 6) & 0xF) == 1) && (((psHu32(0xf440) >> 7) & 1) == 0))//INIT & SRP=0
				rdram_sdevid = 0;	// if SIO repeater is cleared, reset sdevid
			psHu32(mem) = value & ~0x80000000;	//kill the busy bit
			break;

		case 0x1000f440://MCH_DRD:
			psHu32(mem) = value;
			break;
		
		case DMAC_ENABLEW: // DMAC_ENABLEW
			HW_LOG("DMAC_ENABLEW Write 32bit %lx", value);
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
			return;
		
		case 0x1000f200:
			psHu32(mem) = value;
			break;
		
		case SBUS_F220:
			psHu32(mem) |= value;
			break;
		
		case SBUS_SMFLG:
			psHu32(mem) &= ~value;
			break;
		
		case SBUS_F240:
			if(!(value & 0x100))
				psHu32(mem) &= ~0x100;
			else
				psHu32(mem) |= 0x100;
			break;
			
		case 0x1000f260:
			psHu32(mem) = 0;
			break;
		
		case 0x1000f130:
		case 0x1000f410:
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)", mem, value, cpuRegs.CP0.n.Status.val);
			break;
		
		default:
			psHu32(mem) = value;
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)", mem, value, cpuRegs.CP0.n.Status.val);
		break;
	}
}

#endif

/*
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
			Console::Status("GIFMODE64 %x", params value);
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
			DMA_LOG("0x%8.8x hwWrite64: GSdma %lx", cpuRegs.cycle, value);
			DmaExec(dmaGIF, mem, value);
			break;

		case 0x1000e000: // DMAC_CTRL
			HW_LOG("DMAC_CTRL Write 64bit %x", value);
			psHu64(mem) = value;
			break;

		case 0x1000e010: // DMAC_STAT
			HW_LOG("DMAC_STAT Write 64bit %x", value);
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
			HW_LOG("INTC_STAT Write 64bit %x", value);
			psHu32(INTC_STAT)&=~value;	
			cpuTestINTCInts();
			break;

		case 0x1000f010: // INTC_MASK
			HW_LOG("INTC_MASK Write 32bit %x", value);
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

			HW_LOG("Unknown Hardware write 64 at %x with value %x (status=%x)",mem,value, cpuRegs.CP0.n.Status.val);
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

			HW_LOG("Unknown Hardware write 128 at %x with value %x_%x (status=%x)", mem, value[1], value[0], cpuRegs.CP0.n.Status.val);
			break;
	}
}
*/