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


#include <string.h>
#include <malloc.h>

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "PsxMem.h"
#include "IPU/IPU.h"
#include "GS.h"

#include <assert.h>

#ifndef PCSX2_VIRTUAL_MEM
u8  *psH; // hw mem
u16 *psHW;
u32 *psHL;
u64 *psHD;
#endif

int rdram_devices = 2;	// put 8 for TOOL and 2 for PS2 and PSX
int rdram_sdevid = 0;

int hwInit() {

#ifndef PCSX2_VIRTUAL_MEM
	psH = (u8*)_aligned_malloc(0x00010000, 16);
	if (psH == NULL) {
		SysMessage(_("Error allocating memory")); return -1;
	}
#endif

	gsInit();
	vif0Init();
	vif1Init();
	vifDmaInit();
	sifInit();
	sprInit();
	ipuInit();

	return 0;
}

void hwShutdown() {
#ifndef PCSX2_VIRTUAL_MEM
	if (psH == NULL) return;
	_aligned_free(psH); psH = NULL;
#endif
	ipuShutdown();
}

void hwReset()
{
	memset(PS2MEM_HW+0x2000, 0, 0x0000e000);

	psHu32(0xf520) = 0x1201;
	psHu32(0xf260) = 0x1D000060;
	// i guess this is kinda a version, it's used by some bioses
	psHu32(0xf590) = 0x1201;
	
	gsReset();
	ipuReset();
}

u8  hwRead8(u32 mem)
{
	u8 ret;

#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10000000 && mem < 0x10008000 )
		SysPrintf("hwRead8 to %x\n", mem);
#endif

#ifdef SPR_LOG
		SPR_LOG("Hardware read 8bit at %lx, ret %lx\n", mem, psHu8(mem));
#endif

	switch (mem) {
		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				if(mem == 0x1000f260) ret = 0;
				else if(mem == 0x1000F240) {
					ret = psHu32(mem);
					//psHu32(mem) &= ~0x4000;
				}
				else ret = psHu32(mem);
				return (u8)ret;
			}
	
			if (mem < 0x10010000)
			{
				ret = psHu8(mem);
			}
			else ret = 0;
#ifdef HW_LOG
			HW_LOG("Unknown Hardware Read 8 at %x\n",mem);
#endif
			break;
	}

	return ret;
}

u16 hwRead16(u32 mem)
{
	u16 ret;

#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10002000 && mem < 0x10008000 )
		SysPrintf("hwRead16 to %x\n", mem);
#endif

#ifdef SPR_LOG
		SPR_LOG("Hardware read 16bit at %lx, ret %lx\n", mem, psHu16(mem));
#endif
	switch (mem) {
		case 0x10000000: ret = (u16)rcntRcount(0); break;
		case 0x10000010: ret = (u16)counters[0].mode; break;
		case 0x10000020: ret = (u16)counters[0].target; break;
		case 0x10000030: ret = (u16)counters[0].hold; break;

		case 0x10000800: ret = (u16)rcntRcount(1); break;
		case 0x10000810: ret = (u16)counters[1].mode; break;
		case 0x10000820: ret = (u16)counters[1].target; break;
		case 0x10000830: ret = (u16)counters[1].hold; break;

		case 0x10001000: ret = (u16)rcntRcount(2); break;
		case 0x10001010: ret = (u16)counters[2].mode; break;
		case 0x10001020: ret = (u16)counters[2].target; break;

		case 0x10001800: ret = (u16)rcntRcount(3); break;
		case 0x10001810: ret = (u16)counters[3].mode; break;
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
			if (mem < 0x10010000) {
				ret = psHu16(mem);
			}
			else ret = 0;
#ifdef HW_LOG
			HW_LOG("Unknown Hardware Read 16 at %x\n",mem);
#endif
			break;
	}

	return ret;
}

u32 hwRead32(u32 mem) {
	u32 ret;
	
	//IPU regs
	if ((mem>=0x10002000) && (mem<0x10003000)) {
		return ipuRead32(mem);
	}

	// gauntlen uses 0x1001xxxx
	switch (mem) {
		case 0x10000000: return (u16)rcntRcount(0);
		case 0x10000010: return (u16)counters[0].mode;
		case 0x10000020: return (u16)counters[0].target;
		case 0x10000030: return (u16)counters[0].hold;

		case 0x10000800: return (u16)rcntRcount(1);
		case 0x10000810: return (u16)counters[1].mode;
		case 0x10000820: return (u16)counters[1].target;
		case 0x10000830: return (u16)counters[1].hold;

		case 0x10001000: return (u16)rcntRcount(2);
		case 0x10001010: return (u16)counters[2].mode;
		case 0x10001020: return (u16)counters[2].target;

		case 0x10001800: return (u16)rcntRcount(3);
		case 0x10001810: return (u16)counters[3].mode;
		case 0x10001820: return (u16)counters[3].target;

#ifdef PCSX2_DEVBUILD
		case 0x1000A000:
			ret = psHu32(mem);//dma2 chcr
			HW_LOG("Hardware read DMA2_CHCR 32bit at %lx, ret %lx\n", mem, ret);
			break;
		case 0x1000A010:
			ret = psHu32(mem);//dma2 madr
			HW_LOG("Hardware read DMA2_MADR 32bit at %lx, ret %lx\n", mem, ret);
			break;
		case 0x1000A020:
			ret = psHu32(mem);//dma2 qwc
			HW_LOG("Hardware readDMA2_QWC 32bit at %lx, ret %lx\n", mem, ret);
			break;
		case 0x1000A030:
			ret = psHu32(mem);//dma2 taddr
			HW_LOG("Hardware read DMA2_TADDR 32bit at %lx, ret %lx\n", mem, ret);
			break;
		case 0x1000A040:
			ret = psHu32(mem);//dma2 asr0
			HW_LOG("Hardware read DMA2_ASR0 32bit at %lx, ret %lx\n", mem, ret);
			break;
		case 0x1000A050:
			ret = psHu32(mem);//dma2 asr1
			HW_LOG("Hardware read DMA2_ASR1 32bit at %lx, ret %lx\n", mem, ret);
			break;
		case 0x1000A080:
			ret = psHu32(mem);//dma2 saddr
			HW_LOG("Hardware read DMA2_SADDR 32 at %lx, ret %lx\n", mem, ret);
			break;
		case 0x1000B400: // dma4 chcr
			ret = ((DMACh *)&PS2MEM_HW[0xb400])->chcr;
			SPR_LOG("Hardware read IPU1_CHCR 32 at %lx, ret %x\n", mem, ret);
			break;

		case 0x1000e010: // DMAC_STAT
			HW_LOG("DMAC_STAT Read  32bit %x\n", psHu32(0xe010));
			return psHu32(0xe010);
		case 0x1000f000: // INTC_STAT
//			HW_LOG("INTC_STAT Read  32bit %x\n", psHu32(0xf000));
			return psHu32(0xf000);
		case 0x1000f010: // INTC_MASK
			HW_LOG("INTC_MASK Read  32bit %x\n", psHu32(0xf010));
			return psHu32(0xf010);
#endif

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430://MCH_RICM
			ret = 0;
			break;

		case 0x1000f440://MCH_DRD
			
			if ((psHu32(0xf430) >> 6) & 0xF)
				ret = 0;
			else
				switch ((psHu32(0xf430)>>16) & 0xFFF){//MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
				case 0x21://INIT
					ret = 0x1F * (rdram_sdevid < rdram_devices);
					rdram_sdevid += (rdram_sdevid < rdram_devices);
					break;
				case 0x23://CNFGA
					ret = 0x0D0D;	//PVER=3 | MVER=16 | DBL=1 | REFBIT=5
					break;
				case 0x24://CNFGB
					//0x0110 for PSX  SVER=0 | CORG=8(5x9x7) | SPT=1 | DEVTYP=0 | BYTE=0
					ret = 0x0090;	//SVER=0 | CORG=4(5x9x6) | SPT=1 | DEVTYP=0 | BYTE=0
					break;
				case 0x40://DEVID
					ret = psHu32(0xf430) & 0x1F;	// =SDEV
					break;
				default:
					ret = 0;
					break;
			}
			break;

		case 0x1000f520: // DMAC_ENABLER
#ifdef HW_LOG
			HW_LOG("DMAC_ENABLER Read 32bit %lx\n", psHu32(0xf590));
#endif
			return psHu32(0xf590);

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				// SIF Control Registers
				/*1D000020 (word) - EE -> IOP status flag ( set to 0x10000 always ready )
				1D000030 (word) - IOP -> EE status flag
				1D000040 (word) - See psxMem.c ( Initially set to 0xF00042 and reset to
				   to this value if 0x20 is written )
				1D000060 (word) - used to detect whether the SIF interface exists
                   read must be 0x1D000060, or the top 20 bits must be zero 
				*/
				// note, any changes you make in here, also make on recMemRead32
				if(mem ==0x1000f260) ret = 0;
				else if(mem == 0x1000F240) {
					ret = psHu32(mem) | 0xF0000102;
					//psHu32(mem) &= ~0x4000;
				}
				else ret = psHu32(mem);
//#ifdef HW_LOG
				//__Log("%x: sif %x(%x) Read 32bit %x\n", cpuRegs.pc, mem, 0xbd000000 | (mem & 0xf0),ret);
//#endif	}
				break;
			
			}
			else if (mem < 0x10010000) {
				ret = psHu32(mem);
			}
			else {
				SysPrintf("32bit HW read of address 0x%x\n", mem);
				ret = 0;
			}
#ifdef HW_LOG
			HW_LOG("Unknown Hardware Read 32 at %lx, ret %lx\n", mem, ret);
#endif
			break;
	}	

	return ret;
}

u64 hwRead64(u32 mem) {
	u64 ret;

	if ((mem>=0x10002000) && (mem<0x10003000)) {
		return ipuRead64(mem);
	}

	switch (mem) {
		default:
		if (mem < 0x10010000) {
			ret = psHu64(mem);
		}
		else ret = 0;
#ifdef HW_LOG
		HW_LOG("Unknown Hardware Read 64 at %x\n",mem);
#endif
		break;
	}

	return ret;
}

void hwRead128(u32 mem, u64 *out) {
	if (mem >= 0x10004000 && mem < 0x10008000) {
		ReadFIFO(mem, out); return;
	}

	if (mem < 0x10010000) {
		out[0] = psHu64(mem);
		out[1] = psHu64(mem+8);
	}

#ifdef HW_LOG
			HW_LOG("Unknown Hardware Read 128 at %x\n",mem);
#endif
}

// dark cloud2 uses it
#define DmaExec8(name, num) { \
	psHu8(mem) = (u8)value;	\
	if ((psHu8(mem) & 0x1) && (psHu32(DMAC_CTRL) & 0x1)) { \
		/*SysPrintf("Running DMA 8 %x\n", psHu32(mem & ~0x1));*/ \
		dma##name(); \
	} \
}

char sio_buffer[1024];
int sio_count;

void hwWrite8(u32 mem, u8 value) {

#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10000000 && mem < 0x10008000 )
		SysPrintf("hwWrite8 to %x\n", mem);
#endif

	switch (mem) {
		case 0x10000000: rcntWcount(0, value); break;
		case 0x10000010: rcntWmode(0, (counters[0].mode & 0xff00) | value); break;
		case 0x10000011: rcntWmode(0, (counters[0].mode & 0xff) | value << 8); break;
		case 0x10000020: rcntWtarget(0, value); break;
		case 0x10000030: rcntWhold(0, value); break;

		case 0x10000800: rcntWcount(1, value); break;
		case 0x10000810: rcntWmode(1, (counters[1].mode & 0xff00) | value); break;
		case 0x10000811: rcntWmode(1, (counters[1].mode & 0xff) | value << 8); break;
		case 0x10000820: rcntWtarget(1, value); break;
		case 0x10000830: rcntWhold(1, value); break;

		case 0x10001000: rcntWcount(2, value); break;
		case 0x10001010: rcntWmode(2, (counters[2].mode & 0xff00) | value); break;
		case 0x10001011: rcntWmode(2, (counters[2].mode & 0xff) | value << 8); break;
		case 0x10001020: rcntWtarget(2, value); break;

		case 0x10001800: rcntWcount(3, value); break;
		case 0x10001810: rcntWmode(3, (counters[3].mode & 0xff00) | value); break;
		case 0x10001811: rcntWmode(3, (counters[3].mode & 0xff) | value << 8); break;
		case 0x10001820: rcntWtarget(3, value); break;

		case 0x1000f180:
			if (value == '\n') {
				sio_buffer[sio_count] = 0;
				SysPrintf(COLOR_GREEN "%s\n" COLOR_RESET, sio_buffer);
				sio_count = 0;
			} else {
				if (sio_count < 1023) {
					sio_buffer[sio_count++] = value;
				}
			}
//			SysPrintf("%c", value);
			break;
		
		case 0x10003c02: //Tony Hawks Project 8 uses this
			vif1Write32(mem & ~0x2, value << 16);
			break;
		case 0x10008001: // dma0 - vif0
#ifdef DMA_LOG
			DMA_LOG("VIF0dma %lx\n", value);
#endif
			DmaExec8(VIF0, 0);
			break;

		case 0x10009001: // dma1 - vif1
#ifdef DMA_LOG
			DMA_LOG("VIF1dma %lx\n", value);
#endif
			DmaExec8(VIF1, 1);
			break;

		case 0x1000a001: // dma2 - gif
#ifdef DMA_LOG
			DMA_LOG("0x%8.8x hwWrite8: GSdma %lx 0x%lx\n", cpuRegs.cycle, value);
#endif
			DmaExec8(GIF, 2);
			break;

		case 0x1000b001: // dma3 - fromIPU
#ifdef DMA_LOG
			DMA_LOG("IPU0dma %lx\n", value);
#endif
			DmaExec8(IPU0, 3);
			break;

		case 0x1000b401: // dma4 - toIPU
#ifdef DMA_LOG
			DMA_LOG("IPU1dma %lx\n", value);
#endif
			DmaExec8(IPU1, 4);
			break;

		case 0x1000c001: // dma5 - sif0
#ifdef DMA_LOG
			DMA_LOG("SIF0dma %lx\n", value);
#endif
//			if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec8(SIF0, 5);
			break;

		case 0x1000c401: // dma6 - sif1
#ifdef DMA_LOG
			DMA_LOG("SIF1dma %lx\n", value);
#endif
			DmaExec8(SIF1, 6);
			break;

		case 0x1000c801: // dma7 - sif2
#ifdef DMA_LOG
			DMA_LOG("SIF2dma %lx\n", value);
#endif
			DmaExec8(SIF2, 7);
			break;

		case 0x1000d001: // dma8 - fromSPR
#ifdef DMA_LOG
			DMA_LOG("fromSPRdma8 %lx\n", value);
#endif
			DmaExec8(SPR0, 8);
			break;

		case 0x1000d401: // dma9 - toSPR
#ifdef DMA_LOG
			DMA_LOG("toSPRdma8 %lx\n", value);
#endif
			DmaExec8(SPR1, 9);
			break;

		case 0x1000f592: // DMAC_ENABLEW
			psHu8(0xf592) = value;
			psHu8(0xf522) = value;
			break;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					psHu8(mem) = value;
					break;
				case 0x40:
					if(!(value & 0x100)) psHu32(mem) &= ~0x100;
					break;
				}
				return;
			}
			assert( (mem&0xff0f) != 0xf200 );

			switch(mem&~3) {
				case 0x1000f130:
				case 0x1000f410:
				case 0x1000f430:
					break;
				default:
					psHu8(mem) = value;
			}
#ifdef HW_LOG
			HW_LOG("Unknown Hardware write 8 at %x with value %x\n", mem, value);
#endif
			break;
	}
}

#define DmaExec16(name, num) { \
	psHu16(mem) = (u16)value;	\
	if ((psHu16(mem) & 0x100) && (psHu32(DMAC_CTRL) & 0x1)) { \
		SysPrintf("16bit DMA Start\n"); \
		dma##name(); \
	} \
}

void hwWrite16(u32 mem, u16 value)
{
#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10000000 && mem < 0x10008000 )
		SysPrintf("hwWrite16 to %x\n", mem);
#endif
	switch(mem) {
		case 0x10008000: // dma0 - vif0
#ifdef DMA_LOG
			DMA_LOG("VIF0dma %lx\n", value);
#endif
			DmaExec16(VIF0, 0);
			break;

// Latest Fix for Florin by asadr (VIF1)
		case 0x10009000: // dma1 - vif1 - chcr
#ifdef DMA_LOG
			DMA_LOG("VIF1dma CHCR %lx\n", value);
#endif
			DmaExec16(VIF1, 1);
			break;

#ifdef HW_LOG
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
// ---------------------------------------------------

		case 0x1000a000: // dma2 - gif
#ifdef DMA_LOG
			DMA_LOG("0x%8.8x hwWrite32: GSdma %lx\n", cpuRegs.cycle, value);
#endif
			DmaExec16(GIF, 2);
			break;
#ifdef HW_LOG
		
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
		case 0x1000b000: // dma3 - fromIPU
#ifdef DMA_LOG
			DMA_LOG("IPU0dma %lx\n", value);
#endif
			DmaExec16(IPU0, 3);
			break;
		
#ifdef HW_LOG
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
		case 0x1000b400: // dma4 - toIPU
#ifdef DMA_LOG
			DMA_LOG("IPU1dma %lx\n", value);
#endif
			DmaExec16(IPU1, 4);
			break;
#ifdef HW_LOG
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

		case 0x1000c000: // dma5 - sif0
#ifdef DMA_LOG
			DMA_LOG("SIF0dma %lx\n", value);
#endif
//			if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec16(SIF0, 5);
			break;

		case 0x1000c002:
			//?
			break;
		case 0x1000c400: // dma6 - sif1
#ifdef DMA_LOG
			DMA_LOG("SIF1dma %lx\n", value);
#endif
			DmaExec16(SIF1, 6);
			break;

#ifdef HW_LOG
		case 0x1000c420: // dma6 - sif1 - qwc
			HW_LOG("SIF1dma QWC = %lx\n", value);
			psHu32(mem) = value;
			break;
#endif

#ifdef HW_LOG
		case 0x1000c430: // dma6 - sif1 - tadr
			HW_LOG("SIF1dma TADR = %lx\n", value);
			psHu32(mem) = value;
			break;
#endif

		case 0x1000c800: // dma7 - sif2
#ifdef DMA_LOG
			DMA_LOG("SIF2dma %lx\n", value);
#endif
			DmaExec16(SIF2, 7);
			break;
		case 0x1000c802:
			//?
			break;
		case 0x1000d000: // dma8 - fromSPR
#ifdef DMA_LOG
			DMA_LOG("fromSPRdma %lx\n", value);
#endif
			DmaExec16(SPR0, 8);
			break;

		case 0x1000d400: // dma9 - toSPR
#ifdef DMA_LOG
			DMA_LOG("toSPRdma %lx\n", value);
#endif
			DmaExec16(SPR1, 9);
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
		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					psHu16(mem) = value;
					break;
				case 0x20:
					psHu16(mem) |= value;
					break;
				case 0x30:
					psHu16(mem) &= ~value;
					break;
				case 0x40:
					assert( (mem&2)==0);
					if(!(value & 0x100)) psHu16(mem) &= ~0x100;
					else psHu16(mem) |= 0x100;
					break;
				case 0x60:
					psHu16(mem) = 0;
					break;
				}
				return;
			}
			assert( (mem&0xff0f) != 0xf200 );

#ifndef PCSX2_VIRTUAL_MEM
			if (mem < 0x10010000)
#endif
			{
				psHu16(mem) = value;
			}
	}

#ifdef HW_LOG
	HW_LOG("Unknown Hardware write 16 at %x with value %x\n",mem,value);
#endif
}

#define DmaExec(name, num) { \
	/* Keep the old tag if in chain mode and hw doesnt set it*/ \
	if( (value & 0xc) == 0x4 && (value & 0xffff0000) == 0) \
		psHu32(mem) = (psHu32(mem) & 0xFFFF0000) | (u16)value; \
	else /* Else (including Normal mode etc) write whatever the hardware sends*/ \
		 psHu32(mem) = (u32)value;	\
    \
	if ((psHu32(mem) & 0x100) && (psHu32(DMAC_CTRL) & 0x1)) { \
		dma##name(); \
	} \
}

void hwWrite32(u32 mem, u32 value) {
	int i;

	//IPU regs
	if ((mem>=0x10002000) && (mem<0x10003000)) {
    	//psHu32(mem) = value;
		ipuWrite32(mem,value);
		return;
	}

	if ((mem>=0x10003800) && (mem<0x10003c00)) {
		vif0Write32(mem, value); return;
	}
	if ((mem>=0x10003c00) && (mem<0x10004000)) {
		vif1Write32(mem, value); return;
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
			// need to set GIF_MODE (hamster ball)
#ifdef GSPATH3FIX
			//SysPrintf("GIFMODE %x\n", value);
#endif
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
#ifdef DMA_LOG
			DMA_LOG("VIF0dma %lx\n", value);
#endif
			DmaExec(VIF0, 0);
			break;

// Latest Fix for Florin by asadr (VIF1)
		case 0x10009000: // dma1 - vif1 - chcr
#ifdef DMA_LOG
			DMA_LOG("VIF1dma CHCR %lx\n", value);
#endif
			DmaExec(VIF1, 1);
			break;
#ifdef HW_LOG
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
// ---------------------------------------------------

		case 0x1000a000: // dma2 - gif
#ifdef DMA_LOG
			DMA_LOG("0x%8.8x hwWrite32: GSdma %lx\n", cpuRegs.cycle, value);
#endif
			DmaExec(GIF, 2);
			break;
#ifdef HW_LOG
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
		case 0x1000b000: // dma3 - fromIPU
#ifdef DMA_LOG
			DMA_LOG("IPU0dma %lx\n", value);
#endif
			DmaExec(IPU0, 3);
			break;
#ifdef HW_LOG
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
		case 0x1000b400: // dma4 - toIPU
#ifdef DMA_LOG
			DMA_LOG("IPU1dma %lx\n", value);
#endif
			DmaExec(IPU1, 4);
			break;
#ifdef HW_LOG
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

		case 0x1000c000: // dma5 - sif0
#ifdef DMA_LOG
			DMA_LOG("SIF0dma %lx\n", value);
#endif
//			if (value == 0) psxSu32(0x30) = 0x40000;
			DmaExec(SIF0, 5);
			break;

		case 0x1000c400: // dma6 - sif1
#ifdef DMA_LOG
			DMA_LOG("SIF1dma %lx\n", value);
#endif
			DmaExec(SIF1, 6);
			break;

#ifdef HW_LOG
		case 0x1000c420: // dma6 - sif1 - qwc
			HW_LOG("SIF1dma QWC = %lx\n", value);
			psHu32(mem) = value;
			break;
#endif

#ifdef HW_LOG
		case 0x1000c430: // dma6 - sif1 - tadr
			HW_LOG("SIF1dma TADR = %lx\n", value);
			psHu32(mem) = value;
			break;
#endif

		case 0x1000c800: // dma7 - sif2
#ifdef DMA_LOG
			DMA_LOG("SIF2dma %lx\n", value);
#endif
			DmaExec(SIF2, 7);
			break;

		case 0x1000d000: // dma8 - fromSPR
#ifdef DMA_LOG
			DMA_LOG("fromSPRdma %lx\n", value);
#endif
			DmaExec(SPR0, 8);
			break;

		case 0x1000d400: // dma9 - toSPR
#ifdef DMA_LOG
			DMA_LOG("toSPRdma %lx\n", value);
#endif
			DmaExec(SPR1, 9);
			break;

#ifdef HW_LOG
		case 0x1000e000: // DMAC_CTRL
			HW_LOG("DMAC_CTRL Write 32bit %x\n", value);
			psHu32(mem) = value;
			break;
#endif

		case 0x1000e010: // DMAC_STAT
#ifdef HW_LOG
			HW_LOG("DMAC_STAT Write 32bit %x\n", value);
#endif
			psHu16(0xe010)&= ~(value & 0xffff); // clear on 1
			value = value >> 16;
			for (i=0; i<16; i++) { // reverse on 1
				if (value & (1<<i)) {
					if (psHu16(0xe012) & (1<<i)) psHu16(0xe012)&= ~(1<<i);
					else psHu16(0xe012)|= 1<<i;
				}
			}
			if ((cpuRegs.CP0.n.Status.val & 0x10807) == 0x10801)
                cpuTestDMACInts();
			break;

		case 0x1000f000: // INTC_STAT
#ifdef HW_LOG
			HW_LOG("INTC_STAT Write 32bit %x\n", value);
#endif
			psHu32(0xf000)&=~value;	
			if ((cpuRegs.CP0.n.Status.val & 0x10407) == 0x10401)
                cpuTestINTCInts();
			break;

		case 0x1000f010: // INTC_MASK
#ifdef HW_LOG
			HW_LOG("INTC_MASK Write 32bit %x\n", value);
#endif
/*
			for (i=0; i<16; i++) { // reverse on 1
				if (value & (1<<i)) {
					if (psHu32(0xf010) & (1<<i)) psHu32(0xf010)&= ~(1<<i);
					else psHu32(0xf010)|= 1<<i;
				}
			}
*/
			// omg, just xor it
			psHu32(0xf010) ^= value;

			if ((cpuRegs.CP0.n.Status.val & 0x10407) == 0x10401)
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

		case 0x1000f590: // DMAC_ENABLEW
			HW_LOG("DMAC_ENABLEW Write 32bit %lx\n", value);
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
			return;

		case 0x1000f130:
		case 0x1000f410:

#ifdef PCSX2_DEVBUILD
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)\n", mem, value, cpuRegs.CP0.n.Status);
#endif
			break;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					psHu32(mem) = value;
					break;
				case 0x20:
					psHu32(mem) |= value;
					break;
				case 0x30:
					psHu32(mem) &= ~value;
					break;
				case 0x40:
					if(!(value & 0x100)) psHu32(mem) &= ~0x100;
					else psHu32(mem) |= 0x100;
					break;
				case 0x60:
					psHu32(mem) = 0;
					break;
				}
//#ifdef HW_LOG
//				SysPrintf("sif %x Write 32bit %x \n", mem, value);
//#endif
				// already written in psxMemWrite32
#ifdef PCSX2_DEVBUILD
				HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)\n", mem, value, cpuRegs.CP0.n.Status);
#endif
				return;
			}

#ifndef PCSX2_VIRTUAL_MEM
		if (mem < 0x10010000)
#endif
		{
			psHu32(mem) = value;
		}
#ifdef PCSX2_DEVBUILD
			HW_LOG("Unknown Hardware write 32 at %x with value %x (%x)\n", mem, value, cpuRegs.CP0.n.Status);
#endif
			break;
	}
}

void hwWrite64(u32 mem, u64 value) {
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
#ifdef PCSX2_DEVBUILD
			SysPrintf("GIF_CTRL write 64\n", value);
#endif
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
			SysPrintf("GIFMODE64 %x\n", value);
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
#ifdef DMA_LOG
			DMA_LOG("0x%8.8x hwWrite64: GSdma %lx\n", cpuRegs.cycle, value);
#endif
			DmaExec(GIF, 2);
			break;

#ifdef HW_LOG
		case 0x1000e000: // DMAC_CTRL
			HW_LOG("DMAC_CTRL Write 64bit %x\n", value);
			psHu64(mem) = value;
			break;
#endif

		case 0x1000e010: // DMAC_STAT
#ifdef HW_LOG
			HW_LOG("DMAC_STAT Write 64bit %x\n", value);
#endif
			val32 = (u32)value;
			psHu16(0xe010)&= ~(val32 & 0xffff); // clear on 1
			val32 = val32 >> 16;
			for (i=0; i<16; i++) { // reverse on 1
				if (val32 & (1<<i)) {
					if (psHu16(0xe012) & (1<<i)) psHu16(0xe012)&= ~(1<<i);
					else psHu16(0xe012)|= 1<<i;
				}
			}
			if ((cpuRegs.CP0.n.Status.val & 0x10807) == 0x10801)
                cpuTestDMACInts();
			break;

		case 0x1000f590: // DMAC_ENABLEW
			psHu32(0xf590) = value;
			psHu32(0xf520) = value;
			break;

		case 0x1000f000: // INTC_STAT
#ifdef HW_LOG
			HW_LOG("INTC_STAT Write 64bit %x\n", value);
#endif
			psHu32(0xf000)&=~value;	
			if ((cpuRegs.CP0.n.Status.val & 0x10407) == 0x10401)
                cpuTestINTCInts();
			break;

		case 0x1000f010: // INTC_MASK
#ifdef HW_LOG
			HW_LOG("INTC_MASK Write 32bit %x\n", value);
#endif
			for (i=0; i<16; i++) { // reverse on 1
                int s = (1<<i);
				if (value & s) {
					if (psHu32(0xf010) & (1<<i)) psHu32(0xf010)&= ~(1<<i);
					else psHu32(0xf010)|= 1<<i;
				}
			}
			if ((cpuRegs.CP0.n.Status.val & 0x10407) == 0x10401)
                cpuTestINTCInts();
			break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;
		default:
			psHu64(mem) = value;
			
#ifdef PCSX2_DEVBUILD
		    HW_LOG("Unknown Hardware write 64 at %x with value %x (status=%x)\n",mem,value, cpuRegs.CP0.n.Status);
#endif
			break;
	}
}

void hwWrite128(u32 mem, u64 *value) {
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

#ifdef PCSX2_DEVBUILD
		    HW_LOG("Unknown Hardware write 128 at %x with value %x_%x (status=%x)\n", mem, value[1], value[0], cpuRegs.CP0.n.Status);
#endif
			break;
	}
}

void  intcInterrupt() {
    cpuRegs.interrupt &= ~(1 << 30);
	if ((cpuRegs.CP0.n.Status.val & 0x400) != 0x400) return;

	if ((psHu32(INTC_STAT)) == 0) {
		SysPrintf("*PCSX2*: intcInterrupt already cleared\n");
        return;
	}
	if ((psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0) return;

#ifdef HW_LOG
	HW_LOG("intcInterrupt %x\n", psHu32(INTC_STAT) & psHu32(INTC_MASK));
#endif
	if(psHu32(INTC_STAT) & 0x2){
		counters[0].hold = rcntRcount(0);
		counters[1].hold = rcntRcount(1);
	}

	cpuException(0x400, cpuRegs.branch);
}

void  dmacTestInterrupt() {
    cpuRegs.interrupt &= ~(1 << 31);
	if ((cpuRegs.CP0.n.Status.val & 0x800) != 0x800) return;

	if ((psHu16(0xe012) & psHu16(0xe010) || 
		 psHu16(0xe010) & 0x8000) == 0) return;

	if((psHu32(DMAC_CTRL) & 0x1) == 0) return;
}



void  dmacInterrupt()
{
    cpuRegs.interrupt &= ~(1 << 31);
    if ((cpuRegs.CP0.n.Status.val & 0x10807) != 0x10801) return;

	if ((psHu16(0xe012) & psHu16(0xe010) || 
		 psHu16(0xe010) & 0x8000) == 0) return;

	if((psHu32(DMAC_CTRL) & 0x1) == 0) return;

#ifdef HW_LOG
	HW_LOG("dmacInterrupt %x\n", (psHu16(0xe012) & psHu16(0xe010) || 
								  psHu16(0xe010) & 0x8000));
#endif

	cpuException(0x800, cpuRegs.branch);
}

void hwIntcIrq(int n) {
	//if( psHu32(INTC_MASK) & (1<<n) ) {
		psHu32(INTC_STAT)|= 1<<n;
		cpuTestINTCInts();
	//}
}

void hwDmacIrq(int n) {
	psHu32(DMAC_STAT)|= 1<<n;
	cpuTestDMACInts();	
}

/* Write 'size' bytes to memory address 'addr' from 'data'. */
int hwMFIFOWrite(u32 addr, u8 *data, int size) {
	int msize = psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)+16;
	u8 *dst;


	addr = psHu32(DMAC_RBOR) + (addr & psHu32(DMAC_RBSR));
	/* Check if the transfer should wrap around the ring buffer */
	if ((addr+size) >= msize) {
		int s1 = msize - addr;
		int s2 = size - s1;

		/* it does, so first copy 's1' bytes from 'data' to 'addr' */
		dst = PSM(addr);
		if (dst == NULL) return -1;
		Cpu->Clear(addr, s1/4);
		memcpy_fast(dst, data, s1);

		/* and second copy 's2' bytes from '&data[s1]' to 'maddr' */
		dst = PSM(psHu32(DMAC_RBOR));
		if (dst == NULL) return -1;
		Cpu->Clear(psHu32(DMAC_RBOR), s2/4);
		memcpy_fast(dst, &data[s1], s2);
	} else {
		//u32 * tempptr, * tempptr2;

		/* it doesn't, so just copy 'size' bytes from 'data' to 'addr' */
		dst = PSM(addr);
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
