/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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


#include "PrecompiledHeader.h"

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "IopMem.h"

// The full suite of hardware APIs:
#include "IPU/IPU.h"
#include "GS.h"
#include "Gif.h"
#include "Counters.h"
#include "Vif.h"
#include "VifDma.h"
#include "SPR.h"
#include "Sif.h"

using namespace R5900;

/////////////////////////////////////////////////////////////////////////
// DMA Execution Interfaces

// dark cloud2 uses 8 bit DMAs register writes
static __forceinline void DmaExec8( void (*func)(), u32 mem, u8 value )
{
	u32 qwcRegister = (mem | 0x20) & ~0x1;  //Need to remove the lower bit else we end up clearing TADR

	//It's invalid for the hardware to write a DMA while it is active, not without Suspending the DMAC
	if ((value & 0x1) && ((psHu8(mem) & 0x1) == 0x1) && dmacRegs->ctrl.DMAE) 
	{
		DMA_LOG( "DMAExec8 Attempt to run DMA while one is already active mem = %x", mem );
	}

	// Upper 16bits of QWC should not be written since QWC is 16bits in size.
	if ((psHu32(qwcRegister) >> 16) != 0)
	{
		DMA_LOG("DMA QWC (%x) upper 16bits set to %x\n", qwcRegister, psHu32(qwcRegister) >> 16); 
		psHu32(qwcRegister) = 0; 
	}

	psHu8(mem) = (u8)value;
	if ((psHu8(mem) & 0x1) && dmacRegs->ctrl.DMAE)
	{
		/*Console.WriteLn("Running DMA 8 %x", psHu32(mem & ~0x1));*/
		func();
	}
}

static __forceinline void DmaExec16( void (*func)(), u32 mem, u16 value )
{
	u32 qwcRegister = mem | 0x20;

	//It's invalid for the hardware to write a DMA while it is active, not without Suspending the DMAC
	if ((value & 0x100) && ((psHu32(mem) & 0x100) == 0x100) && dmacRegs->ctrl.DMAE) 
	{
		DMA_LOG( "DMAExec16 Attempt to run DMA while one is already active mem = %x", mem);
	}

	// Upper 16bits of QWC should not be written since QWC is 16bits in size.
	if ((psHu32(qwcRegister) >> 16) != 0)
	{
		DMA_LOG("DMA QWC (%x) upper 16bits set to %x\n", 
			qwcRegister,
			psHu32(qwcRegister) >> 16); 
		psHu32(qwcRegister) = 0; 
	} 

	psHu16(mem) = (u16)value;
	if ((psHu16(mem) & 0x100) && dmacRegs->ctrl.DMAE)
	{
		//Console.WriteLn("16bit DMA Start");
		func();
	}
}

static void DmaExec( void (*func)(), u32 mem, u32 value )
{
	u32 qwcRegister = mem | 0x20;

	//It's invalid for the hardware to write a DMA while it is active, not without Suspending the DMAC
	if ((value & 0x100) && ((psHu32(mem) & 0x100) == 0x100) && dmacRegs->ctrl.DMAE) 
	{
		DMA_LOG( "DMAExec32 Attempt to run DMA while one is already active mem = %x", mem );
		
		// Returning here breaks every single Gust game written. :(
		// Not returning here breaks Fatal Frame. Gamefix time.
		if (CHECK_DMAEXECHACK) return;
	}

	// Upper 16bits of QWC should not be written since QWC is 16bits in size.
	if ((psHu32(qwcRegister) >> 16) != 0)
	{
		DMA_LOG("DMA QWC (%x) upper 16bits set to %x\n", 
			qwcRegister,
			psHu32(qwcRegister) >> 16); 
		psHu32(qwcRegister) = 0; 
	}

	/* Keep the old tag if in chain mode and hw doesn't set it*/
	if (((value & 0xc) == 0x4) && ((value & 0xffff0000) == 0))
		psHu32(mem) = (psHu32(mem) & 0xffff0000) | (u16)value;
	else /* Else (including Normal mode etc) write whatever the hardware sends*/
		psHu32(mem) = (u32)value;

	if ((psHu32(mem) & 0x100) && dmacRegs->ctrl.DMAE)
		func();
}


/////////////////////////////////////////////////////////////////////////
// Hardware WRITE 8 bit

char sio_buffer[1024];
int sio_count;
u16 QueuedDMA = 0;

void hwWrite8(u32 mem, u8 value) 
{
	if ((mem >= VIF0_STAT) && (mem < VIF0_FIFO))
	{
		u32 bytemod = mem & 0x3;
		u32 bitpos = 8 * bytemod;
		u32 newval = psHu8(mem) & (255UL << bitpos);
		
		if (mem < VIF1_STAT)
			vif0Write32( mem & ~0x3, newval | (value<<bitpos));
		else
			vif1Write32( mem & ~0x3, newval | (value<<bitpos));
			
		return;
	}

	if( mem >= IPU_CMD && mem < D0_CHCR )
		DevCon.Warning( "hwWrite8 to 0x%x = 0x%x", mem, value );

	switch (mem) {
		case RCNT0_COUNT: rcntWcount(0, value); break;
		case RCNT0_MODE: rcntWmode(0, (counters[0].modeval & 0xff00) | value); break;
		case RCNT0_MODE + 1: rcntWmode(0, (counters[0].modeval & 0xff) | value << 8); break;
		case RCNT0_TARGET: rcntWtarget(0, value); break;
		case RCNT0_HOLD: rcntWhold(0, value); break;

		case RCNT1_COUNT: rcntWcount(1, value); break;
		case RCNT1_MODE: rcntWmode(1, (counters[1].modeval & 0xff00) | value); break;
		case RCNT1_MODE + 1: rcntWmode(1, (counters[1].modeval & 0xff) | value << 8); break;
		case RCNT1_TARGET: rcntWtarget(1, value); break;
		case RCNT1_HOLD: rcntWhold(1, value); break;

		case RCNT2_COUNT: rcntWcount(2, value); break;
		case RCNT2_MODE: rcntWmode(2, (counters[2].modeval & 0xff00) | value); break;
		case RCNT2_MODE + 1: rcntWmode(2, (counters[2].modeval & 0xff) | value << 8); break;
		case RCNT2_TARGET: rcntWtarget(2, value); break;

		case RCNT3_COUNT: rcntWcount(3, value); break;
		case RCNT3_MODE: rcntWmode(3, (counters[3].modeval & 0xff00) | value); break;
		case RCNT3_MODE + 1: rcntWmode(3, (counters[3].modeval & 0xff) | value << 8); break;
		case RCNT3_TARGET: rcntWtarget(3, value); break;

		case SIO_TXFIFO:
		{
			// Terminate lines on CR or full buffers, and ignore \n's if the string contents
			// are empty (otherwise terminate on \n too!)
			if (( value == '\r' ) || ( sio_count == 1023 ) ||
			     ( value == '\n' && sio_count != 0 ))
			{
				// Use "%s" below even though it feels indirect -- it's necessary to avoid
				// errors if/when games use printf formatting control chars.

				sio_buffer[sio_count] = 0;
				Console.WriteLn( ConColor_EE, L"%s", ShiftJIS_ConvertString(sio_buffer).c_str() );
				sio_count = 0;
			}
			else if( value != '\n' )
			{
				sio_buffer[sio_count++] = value;
			}
		}
		break;
		
		//case 0x10003c02: //Tony Hawks Project 8 uses this
		//	vif1Write32(mem & ~0x2, value << 16);
		//	break;
		case D0_CHCR + 1: // dma0 - vif0
			DMA_LOG("VIF0dma EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit VIF0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x1;
			}
			DmaExec8(dmaVIF0, mem, value);
			break;

		case D1_CHCR + 1: // dma1 - vif1
			DMA_LOG("VIF1dma EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit VIF1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x2;
			}
			if(value & 0x1) vif1.done = false;  //This must be done here! some games (ala Crash of the Titans) pause the dma to start MFIFO
			DmaExec8(dmaVIF1, mem, value);
			break;

		case D2_CHCR + 1: // dma2 - gif
			DMA_LOG("GSdma EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit GIF DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x4;
			}
			DmaExec8(dmaGIF, mem, value);
			break;

		case D3_CHCR + 1: // dma3 - fromIPU
			DMA_LOG("IPU0dma EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit IPU0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x8;
			}
			DmaExec8(dmaIPU0, mem, value);
			break;

		case D4_CHCR + 1: // dma4 - toIPU
			DMA_LOG("IPU1dma EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit IPU1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x10;
			}
			DmaExec8(dmaIPU1, mem, value);
			break;

		case D5_CHCR + 1: // dma5 - sif0
			DMA_LOG("SIF0dma EXECUTE, value=0x%x", value);
//			if (value == 0) psxSu32(0x30) = 0x40000;
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit SIF0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x20;
			}
			DmaExec8(dmaSIF0, mem, value);
			break;

		case D6_CHCR + 1: // dma6 - sif1
			DMA_LOG("SIF1dma EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit SIF1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x40;
			}
			DmaExec8(dmaSIF1, mem, value);
			break;

		case D7_CHCR + 1: // dma7 - sif2
			DMA_LOG("SIF2dma EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit SIF2 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x80;
			}
			DmaExec8(dmaSIF2, mem, value);
			break;

		case D8_CHCR + 1: // dma8 - fromSPR
			DMA_LOG("fromSPRdma8 EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit SPR0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x100;
			}
			DmaExec8(dmaSPR0, mem, value);
			break;

		case SPR1_CHCR + 1: // dma9 - toSPR
			DMA_LOG("toSPRdma8 EXECUTE, value=0x%x", value);
			if ((value & 0x1) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("8 bit SPR1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x200;
			}
			DmaExec8(dmaSPR1, mem, value);
			break;

		case DMAC_ENABLEW + 2:
			psHu8(DMAC_ENABLEW + 2) = value;
			psHu8(DMAC_ENABLER + 2) = value;
			break;

		case SBUS_F200: // SIF(?)
			psHu8(mem) = value;
			break;

		case SBUS_F210:
			psHu8(mem) = value;
			break;
		
		case SBUS_F220:
			psHu8(mem) = value;
			break;
		
		case SBUS_F230:
			psHu8(mem) = value;
			break;
			
		case SBUS_F240:
			if (!(value & 0x100)) psHu32(mem) &= ~0x100;
			break;
		
		case SBUS_F250:
			psHu8(mem) = value;
			break;
		
		case SBUS_F260:
			psHu8(mem) = value;
			break;
		
		default:
			pxAssert( (mem & 0xff0f) != 0xf200 );

			switch(mem&~3) {
				case SIO_ISR:
				case 0x1000f410:
				case MCH_RICM:
					break;
				
				default:
					psHu8(mem) = value;
			}
			HW_LOG("Unknown Hardware write 8 at %x with value %x", mem, value);
			break;
	}
}

__forceinline void hwWrite16(u32 mem, u16 value)
{
	if( mem >= IPU_CMD && mem < D0_CHCR )
		Console.Warning( "hwWrite16 to %x", mem );

	switch(mem)
	{
		case RCNT0_COUNT: rcntWcount(0, value); break;
		case RCNT0_MODE: rcntWmode(0, value); break;
		case RCNT0_TARGET: rcntWtarget(0, value); break;
		case RCNT0_HOLD: rcntWhold(0, value); break;

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

		case D0_CHCR: // dma0 - vif0
			DMA_LOG("VIF0dma %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit VIF0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x1;
			}
			DmaExec16(dmaVIF0, mem, value);
			break;

		case D1_CHCR: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma CHCR %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit VIF1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x2;
			}
			if(value & 0x100) vif1.done = false;  //This must be done here! some games (ala Crash of the Titans) pause the dma to start MFIFO
			DmaExec16(dmaVIF1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case D1_MADR: // dma1 - vif1 - madr
			HW_LOG("VIF1dma Madr %lx", value);
			psHu16(mem) = value;//dma1 madr
			break;
		
		case D1_QWC: // dma1 - vif1 - qwc
			HW_LOG("VIF1dma QWC %lx", value);
			psHu16(mem) = value;//dma1 qwc
			break;
		
		case D1_TADR: // dma1 - vif1 - tadr
			HW_LOG("VIF1dma TADR %lx", value);
			psHu16(mem) = value;//dma1 tadr
			break;
		
		case D1_ASR0: // dma1 - vif1 - asr0
			HW_LOG("VIF1dma ASR0 %lx", value);
			psHu16(mem) = value;//dma1 asr0
			break;
		
		case D1_ASR1: // dma1 - vif1 - asr1
			HW_LOG("VIF1dma ASR1 %lx", value);
			psHu16(mem) = value;//dma1 asr1
			break;
		
		case D1_SADR: // dma1 - vif1 - sadr
			HW_LOG("VIF1dma SADR %lx", value);
			psHu16(mem) = value;//dma1 sadr
			break;
#endif
// ---------------------------------------------------

		case D2_CHCR: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite32: GSdma %lx", cpuRegs.cycle, value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit GIF DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x4;
			}
			DmaExec16(dmaGIF, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case D2_MADR:
		    psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write DMA2_MADR 32bit at %x with value %x",mem,value);
		    break;
		
		case D2_QWC:
			psHu16(mem) = value;//dma2 qwc
			HW_LOG("Hardware write DMA2_QWC 32bit at %x with value %x",mem,value);
			break;
		
		case D2_TADR:
			psHu16(mem) = value;//dma2 taddr
			HW_LOG("Hardware write DMA2_TADDR 32bit at %x with value %x",mem,value);
			break;
		
		case D2_ASR0:
			psHu16(mem) = value;//dma2 asr0
			HW_LOG("Hardware write DMA2_ASR0 32bit at %x with value %x",mem,value);
			break;
		
		case D2_ASR1:
			psHu16(mem) = value;//dma2 asr1
			HW_LOG("Hardware write DMA2_ASR1 32bit at %x with value %x",mem,value);
			break;
		
		case D2_SADR:
			psHu16(mem) = value;//dma2 saddr
			HW_LOG("Hardware write DMA2_SADDR 32bit at %x with value %x",mem,value);
			break;
#endif

		case D3_CHCR: // dma3 - fromIPU
			DMA_LOG("IPU0dma %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit IPU0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x8;
			}
			DmaExec16(dmaIPU0, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case D3_MADR:
	   		psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_MADR 32bit at %x with value %x",mem,value);
			break;
		
		case D3_QWC:
			psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU0DMA_QWC 32bit at %x with value %x",mem,value);
			break;
		
		case D3_TADR:
			psHu16(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU0DMA_TADR 32bit at %x with value %x",mem,value);
			break;
		
		case D3_SADR:
			psHu16(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU0DMA_SADDR 32bit at %x with value %x",mem,value);
			break;
#endif

		case D4_CHCR: // dma4 - toIPU
			DMA_LOG("IPU1dma %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit IPU1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x10;
			}
			DmaExec16(dmaIPU1, mem, value);
			break;

#ifdef PCSX2_DEVBUILD
		case D4_MADR:
			psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_MADR 32bit at %x with value %x",mem,value);
       		break;
		
		case D4_QWC:
			psHu16(mem) = value;//dma2 madr
			HW_LOG("Hardware write IPU1DMA_QWC 32bit at %x with value %x",mem,value);
       		break;
		
		case D4_TADR:
			psHu16(mem) = value;//dma2 tadr
			HW_LOG("Hardware write IPU1DMA_TADR 32bit at %x with value %x",mem,value);
			break;
		
		case D4_SADR:
			psHu16(mem) = value;//dma2 saddr
			HW_LOG("Hardware write IPU1DMA_SADDR 32bit at %x with value %x",mem,value);
			break;
#endif
		case D5_CHCR: // dma5 - sif0
			DMA_LOG("SIF0dma %lx", value);
//			if (value == 0) psxSu32(0x30) = 0x40000;
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit SIF0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x20;
			}
			DmaExec16(dmaSIF0, mem, value);
			break;

		case D5_CHCR + 2:
			//?
			break;
		
		case D6_CHCR: // dma6 - sif1
			DMA_LOG("SIF1dma %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit SIF1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x40;
			}
			DmaExec16(dmaSIF1, mem, value);
			break;
			
		// Given the other values here, perhaps something like this is in order?
		/*case 0x1000C402: // D6_CHCR + 2
			//?
			break;*/
		
#ifdef PCSX2_DEVBUILD
		case D6_MADR: // dma6 - sif1 - madr
			HW_LOG("SIF1dma MADR = %lx", value);
			psHu16(mem) = value;
			break;
		
		case D6_QWC: // dma6 - sif1 - qwc
			HW_LOG("SIF1dma QWC = %lx", value);
			psHu16(mem) = value;
			break;

		case D6_TADR: // dma6 - sif1 - tadr
			HW_LOG("SIF1dma TADR = %lx", value);
			psHu16(mem) = value;
			break;
#endif

		case D7_CHCR: // dma7 - sif2
			DMA_LOG("SIF2dma %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit SIF2 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x80;
			}
			DmaExec16(dmaSIF2, mem, value);
			break;
			
		case D7_CHCR + 2:
			//?
			break;
		
		case D8_CHCR: // dma8 - fromSPR
			DMA_LOG("fromSPRdma %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit SPR0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x100;
			}
			DmaExec16(dmaSPR0, mem, value);
			break;

		case SPR1_CHCR: // dma9 - toSPR
			DMA_LOG("toSPRdma %lx", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("16 bit SPR1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x200;
			}
			DmaExec16(dmaSPR1, mem, value);
			break;
			
		case DMAC_ENABLEW + 2:
			psHu16(DMAC_ENABLEW + 2) = value;
			psHu16(DMAC_ENABLER + 2) = value;
			break;
		
		case SIO_ISR:
		case SIO_ISR + 2:
		case 0x1000f410:
		case 0x1000f410 + 2:
		case MCH_RICM:
		case MCH_RICM + 2:
			break;

		case SBUS_F200:
			psHu16(mem) = value;
			break;
		
		case SBUS_F210:
			psHu16(mem) = value;
			break;

		case SBUS_F220:
			psHu16(mem) |= value;
			break;
		
		case SBUS_F230:
			psHu16(mem) &= ~value;
			break;
		
		case SBUS_F240:
			if (!(value & 0x100))
				psHu16(mem) &= ~0x100;
			else
				psHu16(mem) |= 0x100;
			break;
			
		case SBUS_F250:
			psHu16(mem) = value;
			break;
			
		case SBUS_F260:
			psHu16(mem) = 0;
			break;

		default:
			psHu16(mem) = value;
			UnknownHW_LOG("Unknown Hardware write 16 at %x with value %x",mem,value);
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
	if (mem >= VIF0_STAT)
	{
		if(mem < VIF1_STAT)
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
			else if ( value & 8 )
				gifRegs->stat.PSE = 1;
			else
				gifRegs->stat.PSE = 0; 
		break;

		case GIF_MODE:
		{
			// need to set GIF_MODE (hamster ball)
			gifRegs->mode._u32 = value;

			// set/clear bits 0 and 2 as per the GIF_MODE value.
            const u32 bitmask = GIF_MODE_M3R | GIF_MODE_IMT;
			psHu32(GIF_STAT) &= ~bitmask;
			psHu32(GIF_STAT) |= (u32)value & bitmask;

		}
		break;

		case GIF_STAT: // stat is readonly
			DevCon.Warning("*PCSX2* GIFSTAT write value = 0x%x (readonly, ignored)", value);
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
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit IPU0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x8;
			}
			DmaExec(dmaIPU0, mem, value);
			return;

		case D3_MADR: regName = "IPU0DMA_MADR"; break;
		case D3_QWC: regName = "IPU0DMA_QWC"; break;
		case D3_TADR: regName = "IPU0DMA_TADR"; break;
		case D3_SADR: regName = "IPU0DMA_SADDR"; break;

		//------------------------------------------------------------------

		case D4_CHCR: // dma4 - toIPU
			DMA_LOG("IPU1dma EXECUTE, value=0x%x\n", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit IPU1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x10;
			}
			DmaExec(dmaIPU1, mem, value);
			return;

		case D4_MADR: regName = "IPU1DMA_MADR"; break;
		case D4_QWC: regName = "IPU1DMA_QWC"; break;
		case D4_TADR: regName = "IPU1DMA_TADR"; break;
		case D4_SADR: regName = "IPU1DMA_SADDR"; break;
	}

	HW_LOG( "Hardware Write32 at 0x%x (%s), value=0x%x", mem, regName, value );
	psHu32(mem) = value;
}

void __fastcall StartQueuedDMA()
{
	if(QueuedDMA & 0x001) { QueuedDMA &= ~0x001; dmaVIF0(); }
	if(QueuedDMA & 0x002) { QueuedDMA &= ~0x002; dmaVIF1(); }
	if(QueuedDMA & 0x004) { QueuedDMA &= ~0x004; dmaGIF(); }
	if(QueuedDMA & 0x008) { QueuedDMA &= ~0x008; dmaIPU0(); }
	if(QueuedDMA & 0x010) { QueuedDMA &= ~0x010; dmaIPU1(); }
	if(QueuedDMA & 0x020) { QueuedDMA &= ~0x020; dmaSIF0(); }
	if(QueuedDMA & 0x040) { QueuedDMA &= ~0x040; dmaSIF1(); }
	if(QueuedDMA & 0x080) { QueuedDMA &= ~0x080; dmaSIF2(); }
	if(QueuedDMA & 0x100) { QueuedDMA &= ~0x100; dmaSPR0(); }
	if(QueuedDMA & 0x200) { QueuedDMA &= ~0x200; dmaSPR1(); }
}

void __fastcall hwWrite32_page_0E( u32 mem, u32 value )
{
	switch (mem)
	{
		case DMAC_CTRL:
		{
			u32 oldvalue = psHu32(mem);
			
			HW_LOG("DMAC_CTRL Write 32bit %x", value);
			
			psHu32(mem) = value;
			//Check for DMAS that were started while the DMAC was disabled
			if (((oldvalue & 0x1) == 0) && ((value & 0x1) == 1))
			{
				if (QueuedDMA != 0) StartQueuedDMA();
			}
			break;
		}
		
		case DMAC_STAT:
			HW_LOG("DMAC_STAT Write 32bit %x", value);

			// lower 16 bits: clear on 1
			// upper 16 bits: reverse on 1

			psHu16(0xe010) &= ~(value & 0xffff);
			psHu16(0xe012) ^= (u16)(value >> 16);

			cpuTestDMACInts();
			break;
		
		default:
			psHu32(mem) = value;
			break;
	}
}

void __fastcall hwWrite32_page_0F( u32 mem, u32 value )
{
	// Shift the middle 8 bits (bits 4-12) into the lower 8 bits.
	// This helps the compiler optimize the switch statement into a lookup table. :)

#define HELPSWITCH(m) (((m)>>4) & 0xff)

	switch( HELPSWITCH(mem) )
	{
		case HELPSWITCH(INTC_STAT):
			HW_LOG("INTC_STAT Write 32bit %x", value);
			psHu32(INTC_STAT) &= ~value;	
			//cpuTestINTCInts();
			break;

		case HELPSWITCH(INTC_MASK):
			HW_LOG("INTC_MASK Write 32bit %x", value);
			psHu32(INTC_MASK) ^= (u16)value;
			cpuTestINTCInts();
			break;

		//------------------------------------------------------------------			
		case HELPSWITCH(MCH_RICM)://MCH_RICM: x:4|SA:12|x:5|SDEV:1|SOP:4|SBC:1|SDEV:5
			if ((((value >> 16) & 0xFFF) == 0x21) && (((value >> 6) & 0xF) == 1) && (((psHu32(0xf440) >> 7) & 1) == 0))//INIT & SRP=0
				rdram_sdevid = 0;	// if SIO repeater is cleared, reset sdevid
			psHu32(mem) = value & ~0x80000000;	//kill the busy bit
			break;

		case HELPSWITCH(SBUS_F200):
			psHu32(mem) = value;
			break;
		
		case HELPSWITCH(SBUS_F220):
			psHu32(mem) |= value;
			break;
		
		case HELPSWITCH(SBUS_F230):
			psHu32(mem) &= ~value;
			break;
		
		case HELPSWITCH(SBUS_F240):
			if(!(value & 0x100))
				psHu32(mem) &= ~0x100;
			else
				psHu32(mem) |= 0x100;
			break;
			
		case HELPSWITCH(SBUS_F260):
			psHu32(mem) = 0;
			break;

		case HELPSWITCH(MCH_DRD)://MCH_DRD:
			psHu32(mem) = value;
			break;

		case HELPSWITCH(DMAC_ENABLEW):
			HW_LOG("DMAC_ENABLEW Write 32bit %lx", value);
			psHu32(DMAC_ENABLEW) = value;
			psHu32(DMAC_ENABLER) = value;
			break;

		//------------------------------------------------------------------
		case HELPSWITCH(SIO_ISR):
		case HELPSWITCH(0x1000f410):
			UnknownHW_LOG("Unknown Hardware write 32 at %x with value %x (%x)", mem, value, cpuRegs.CP0.n.Status.val);
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
			DMA_LOG("VIF0dma EXECUTE, value=0x%x", value);
		
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit VIF0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x1;
			}
			
			DmaExec(dmaVIF0, mem, value);
			return;

//------------------------------------------------------------------
		case D1_CHCR: // dma1 - vif1 - chcr
			DMA_LOG("VIF1dma EXECUTE, value=0x%x", value);
		
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit VIF1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x2;
			}
			
			if (value & 0x100) 
			{
				vif1.done = false;  //This must be done here! some games (ala Crash of the Titans) pause the dma to start MFIFO
			} else cpuRegs.interrupt &= ~(1<<10) | ~(1<<1); //Tekken tag seems to stop vif and start it again in normal, so we will cancel the mfifo loop
			
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
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit GIF DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x4;
			}
			DmaExec(dmaGIF, mem, value);
			return;

		case D2_MADR: regName = "GIFdma MADR"; break;
		case D2_QWC: regName = "GIFdma QWC"; break;
		case D2_TADR: regName = "GIFdma TADDR"; break;
		case D2_ASR0: regName = "GIFdma ASR0"; break;
		case D2_ASR1: regName = "GIFdma ASR1"; break;
		case D2_SADR: regName = "GIFdma SADDR"; break;

//------------------------------------------------------------------
		case D5_CHCR: // dma5 - sif0
			DMA_LOG("SIF0dma EXECUTE, value=0x%x", value);
			//if (value == 0) psxSu32(0x30) = 0x40000;
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit SIF0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x20;
			}
			DmaExec(dmaSIF0, mem, value);
			return;
//------------------------------------------------------------------
		case D6_CHCR: // dma6 - sif1
			DMA_LOG("SIF1dma EXECUTE, value=0x%x", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit SIF1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x40;
			}
			DmaExec(dmaSIF1, mem, value);
			return;

		case D6_MADR: regName = "SIF1dma MADR"; break;
		case D6_QWC: regName = "SIF1dma QWC"; break;
		case D6_TADR: regName = "SIF1dma TADR"; break;

//------------------------------------------------------------------
		case D7_CHCR: // dma7 - sif2
			DMA_LOG("SIF2dma EXECUTE, value=0x%x", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit SIF2 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x80;
			}
			DmaExec(dmaSIF2, mem, value);
			return;
//------------------------------------------------------------------
		case D8_CHCR: // dma8 - fromSPR
			DMA_LOG("SPR0dma EXECUTE (fromSPR), value=0x%x", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit SPR0 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x100;
			}
			DmaExec(dmaSPR0, mem, value);
			return;
//------------------------------------------------------------------
		case SPR1_CHCR: // dma9 - toSPR
			DMA_LOG("SPR1dma EXECUTE (toSPR), value=0x%x", value);
			if ((value & 0x100) && !dmacRegs->ctrl.DMAE) 
			{
				DevCon.Warning("32 bit SPR1 DMA Start while DMAC Disabled\n");
				QueuedDMA |= 0x200;
			}
			DmaExec(dmaSPR1, mem, value);
			return;
	}
	HW_LOG( "Hardware Write32 at 0x%x (%s), value=0x%x", mem, regName, value );
	psHu32(mem) = value;
}

/////////////////////////////////////////////////////////////////////////
// HW Write 64 bit

// Page 0 of HW memory houses registers for Counters 0 and 1
void __fastcall hwWrite64_page_00( u32 mem, const mem64_t* srcval )
{
	hwWrite32_page_00( mem, (u32)*srcval );		// just ignore upper 32 bits.
	*((u64*)&PS2MEM_HW[mem]) = *srcval;
}

// Page 1 of HW memory houses registers for Counters 2 and 3
void __fastcall hwWrite64_page_01( u32 mem, const mem64_t* srcval )
{
	hwWrite32_page_01( mem, (u32)*srcval );		// just ignore upper 32 bits.
	*((u64*)&PS2MEM_HW[mem]) = *srcval;
}

void __fastcall hwWrite64_page_02( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval );  return;
	ipuWrite64( mem, *srcval );
}

void __fastcall hwWrite64_page_03( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;
	const u64 value = *srcval;

	if (mem >= VIF0_STAT)
	{
		if (mem < VIF1_STAT)
			vif0Write32(mem, value); 
		else
			vif1Write32(mem, value); 
		return;
	}

	switch (mem)
	{
		case GIF_CTRL:
			DevCon.WriteLn("GIF_CTRL write 64", value);
			psHu32(mem) = value & 0x8;
			if(value & 0x1)
				gsGIFReset();
			else
			{
				if( value & 8 )
					gifRegs->stat.PSE = 1;
				else
					gifRegs->stat.PSE = 0;
			}
			break;

		case GIF_MODE:
		{
			// set/clear bits 0 and 2 as per the GIF_MODE value.
			const u32 bitmask = GIF_MODE_M3R | GIF_MODE_IMT;
			
			Console.WriteLn("GIFMODE64 %x", value);

			psHu64(GIF_MODE) = value;
			psHu32(GIF_STAT) &= ~bitmask;
			psHu32(GIF_STAT) |= (u32)value & bitmask;
			break;
		}

		case GIF_STAT: // stat is readonly
			break;
	}
}

void __fastcall hwWrite64_page_0E( u32 mem, const mem64_t* srcval )
{
	//hwWrite64( mem, *srcval ); return;

	const u64 value = *srcval;

	switch (mem)
	{
		case DMAC_CTRL:
		{
			u32 oldvalue = psHu32(mem);
			psHu64(mem) = value;
			
			HW_LOG("DMAC_CTRL Write 64bit %x", value);
			
			if (((oldvalue & 0x1) == 0) && ((value & 0x1) == 1))
			{
				if (QueuedDMA != 0) StartQueuedDMA();
			}
			break;
		}
			
		case DMAC_STAT:
			HW_LOG("DMAC_STAT Write 64bit %x", value);

			// lower 16 bits: clear on 1
			// upper 16 bits: reverse on 1

			psHu16(0xe010) &= ~(value & 0xffff);
			psHu16(0xe012) ^= (u16)(value >> 16);

			cpuTestDMACInts();
			break;
		
		default:
			psHu64(mem) = value;
			break;
	}
}

void __fastcall hwWrite64_generic( u32 mem, const mem64_t* srcval )
{
	const u64 value = *srcval;

	switch (mem)
	{
		case D2_CHCR: // dma2 - gif
			DMA_LOG("0x%8.8x hwWrite64: GSdma %x", cpuRegs.cycle, value);
			DmaExec(dmaGIF, mem, value);
			break;

		case INTC_STAT:
			HW_LOG("INTC_STAT Write 64bit %x", (u32)value);
			psHu32(INTC_STAT) &= ~value;	
			//cpuTestINTCInts();
			break;

		case INTC_MASK:
			HW_LOG("INTC_MASK Write 64bit %x", (u32)value);
			psHu32(INTC_MASK) ^= (u16)value;
			cpuTestINTCInts();
			break;

		case SIO_ISR:
		case 0x1000f410:
		case MCH_RICM:
			break;

		case DMAC_ENABLEW: // DMAC_ENABLEW
			psHu32(DMAC_ENABLEW) = value;
			psHu32(DMAC_ENABLER) = value;
		break;

		default:
			psHu64(mem) = value;
			UnknownHW_LOG("Unknown Hardware write 64 at %x with value %x (status=%x)",mem,value, cpuRegs.CP0.n.Status.val);
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
			HW_LOG("INTC_STAT Write 64bit %x", (u32)srcval[0]);
			psHu32(INTC_STAT) &= ~srcval[0];	
			//cpuTestINTCInts();
		break;
		
		case INTC_MASK:
			HW_LOG("INTC_MASK Write 64bit %x", (u32)srcval[0]);
			psHu32(INTC_MASK) ^= (u16)srcval[0];
			cpuTestINTCInts();
		break;

		case DMAC_ENABLEW: // DMAC_ENABLEW
			psHu32(DMAC_ENABLEW) = srcval[0];
			psHu32(DMAC_ENABLER) = srcval[0];
		break;

		case SIO_ISR:
		case 0x1000f410:
		case MCH_RICM:
			break;

		default:
			psHu64(mem  ) = srcval[0];
			psHu64(mem+8) = srcval[1];

			UnknownHW_LOG("Unknown Hardware write 128 at %x with value %x_%x (status=%x)", mem, srcval[1], srcval[0], cpuRegs.CP0.n.Status.val);
		break;
	}
}
