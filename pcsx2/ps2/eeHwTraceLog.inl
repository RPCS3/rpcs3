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

#pragma once

#define eeAddrInRange(name, addr) \
	(addr >= EEMemoryMap::name##_Start && addr < EEMemoryMap::name##_End)

static __ri bool _eelog_enabled( u32 addr )
{
	// Selective enable/disable ability for specific register maps
	if (eeAddrInRange(RCNT0, addr)) return false;
	if (eeAddrInRange(RCNT1, addr)) return true;
	if (eeAddrInRange(RCNT2, addr)) return true;
	if (eeAddrInRange(RCNT3, addr)) return true;

	if (eeAddrInRange(SBUS, addr)) return false;

	// INTC!
	if (addr == INTC_STAT || addr == INTC_MASK) return true;

	return true;
}

template< typename T>
static __ri const char* _eelog_GetHwName( u32 addr, T val )
{
#define EasyCase(label) case label: return #label

	switch( addr )
	{
		//  Counters!
		EasyCase(RCNT0_COUNT);
		EasyCase(RCNT0_MODE);
		EasyCase(RCNT0_TARGET);
		EasyCase(RCNT0_HOLD);

		EasyCase(RCNT1_COUNT);
		EasyCase(RCNT1_MODE);
		EasyCase(RCNT1_TARGET);
		EasyCase(RCNT1_HOLD);

		EasyCase(RCNT2_COUNT);
		EasyCase(RCNT2_MODE);
		EasyCase(RCNT2_TARGET);

		EasyCase(RCNT3_COUNT);
		EasyCase(RCNT3_MODE);
		EasyCase(RCNT3_TARGET);

		// IPU!
		EasyCase(IPU_CMD);
		EasyCase(IPU_CTRL);
		EasyCase(IPU_BP);
		EasyCase(IPU_TOP);

		// GIF!
		EasyCase(GIF_CTRL);
		EasyCase(GIF_MODE);
		EasyCase(GIF_STAT);
		EasyCase(GIF_TAG0);
		EasyCase(GIF_TAG1);
		EasyCase(GIF_TAG2);
		EasyCase(GIF_TAG3);
		EasyCase(GIF_CNT);
		EasyCase(GIF_P3CNT);
		EasyCase(GIF_P3TAG);

		// VIF!
		EasyCase(VIF0_STAT);
		EasyCase(VIF0_FBRST);
		EasyCase(VIF0_ERR);
		EasyCase(VIF0_MARK);
		EasyCase(VIF0_CYCLE);
		EasyCase(VIF0_MODE);
		EasyCase(VIF0_NUM);
		EasyCase(VIF0_MASK);
		EasyCase(VIF0_CODE);
		EasyCase(VIF0_ITOPS);
		EasyCase(VIF0_ITOP);
		EasyCase(VIF0_TOP);
		EasyCase(VIF0_ROW0);
		EasyCase(VIF0_ROW1);
		EasyCase(VIF0_ROW2);
		EasyCase(VIF0_ROW3);
		EasyCase(VIF0_COL0);
		EasyCase(VIF0_COL1);
		EasyCase(VIF0_COL2);
		EasyCase(VIF0_COL3);

		EasyCase(VIF1_STAT);
		EasyCase(VIF1_FBRST);
		EasyCase(VIF1_ERR);
		EasyCase(VIF1_MARK);
		EasyCase(VIF1_CYCLE);
		EasyCase(VIF1_MODE);
		EasyCase(VIF1_NUM);
		EasyCase(VIF1_MASK);
		EasyCase(VIF1_CODE);
		EasyCase(VIF1_ITOPS);
		EasyCase(VIF1_BASE);
		EasyCase(VIF1_OFST);
		EasyCase(VIF1_TOPS);
		EasyCase(VIF1_ITOP);
		EasyCase(VIF1_TOP);
		EasyCase(VIF1_ROW0);
		EasyCase(VIF1_ROW1);
		EasyCase(VIF1_ROW2);
		EasyCase(VIF1_ROW3);
		EasyCase(VIF1_COL0);
		EasyCase(VIF1_COL1);
		EasyCase(VIF1_COL2);
		EasyCase(VIF1_COL3);

		// VIF DMA!
		EasyCase(VIF0_CHCR);
		EasyCase(VIF0_MADR);
		EasyCase(VIF0_QWC);
		EasyCase(VIF0_TADR);
		EasyCase(VIF0_ASR0);
		EasyCase(VIF0_ASR1);

		EasyCase(VIF1_CHCR);
		EasyCase(VIF1_QWC);
		EasyCase(VIF1_TADR);
		EasyCase(VIF1_ASR0);
		EasyCase(VIF1_ASR1);

		// GIF DMA!
		EasyCase(GIF_CHCR);
		EasyCase(GIF_MADR);
		EasyCase(GIF_QWC);
		EasyCase(GIF_TADR);
		EasyCase(GIF_ASR0);
		EasyCase(GIF_ASR1);

		// IPU DMA!
		EasyCase(fromIPU_CHCR);
		EasyCase(fromIPU_MADR);
		EasyCase(fromIPU_QWC);
		EasyCase(fromIPU_TADR);

		EasyCase(toIPU_CHCR);
		EasyCase(toIPU_MADR);
		EasyCase(toIPU_QWC);
		EasyCase(toIPU_TADR);

		// SIF DMA!
		EasyCase(SIF0_CHCR);
		EasyCase(SIF0_MADR);
		EasyCase(SIF0_QWC);

		EasyCase(SIF1_CHCR);
		EasyCase(SIF1_MADR);
		EasyCase(SIF1_QWC);

		EasyCase(SIF2_CHCR);
		EasyCase(SIF2_MADR);
		EasyCase(SIF2_QWC);

		// Scratchpad DMA!  (SPRdma)

		EasyCase(fromSPR_CHCR);
		EasyCase(fromSPR_MADR);
		EasyCase(fromSPR_QWC);
		
		EasyCase(toSPR_CHCR);
		EasyCase(toSPR_MADR);
		EasyCase(toSPR_QWC);

		// DMAC!		
		EasyCase(DMAC_CTRL);
		EasyCase(DMAC_STAT);
		EasyCase(DMAC_PCR);
		EasyCase(DMAC_SQWC);
		EasyCase(DMAC_RBSR);
		EasyCase(DMAC_RBOR);
		EasyCase(DMAC_STADR);
		EasyCase(DMAC_ENABLER);
		EasyCase(DMAC_ENABLEW);

		// INTC!
		EasyCase(INTC_STAT);
		EasyCase(INTC_MASK);

		// SIO
		EasyCase(SIO_LCR);
		EasyCase(SIO_LSR);
		EasyCase(SIO_IER);
		EasyCase(SIO_ISR);
		EasyCase(SIO_FCR);
		EasyCase(SIO_BGR);
		EasyCase(SIO_TXFIFO);
		EasyCase(SIO_RXFIFO);

		// SBUS (terribly mysterious!)
		EasyCase(SBUS_F200);
		EasyCase(SBUS_F210);
		EasyCase(SBUS_F220);
		EasyCase(SBUS_F230);
		EasyCase(SBUS_F240);
		EasyCase(SBUS_F250);
		EasyCase(SBUS_F260);

		// MCH (vaguely mysterious!)
		EasyCase(MCH_RICM);
		EasyCase(MCH_DRD);
	}

#define EasyZone(zone) \
	if ((addr >= EEMemoryMap::zone##_Start) && (addr < EEMemoryMap::zone##_End)) return #zone

#define EasyZoneR(zone) \
	EasyZone(zone) "_reserved"

	// Nothing discovered/handled : Check for "zoned" registers -- registers mirrored
	// across large sections of memory (FIFOs mainly).

	EasyZone(VIF0_FIFO);	EasyZone(VIF1_FIFO);	EasyZone(GIF_FIFO);

	if( (addr >= EEMemoryMap::IPU_FIFO_Start) && (addr < EEMemoryMap::IPU_FIFO_End) )
	{
		return (addr & 0x10) ? "IPUin_FIFO" : "IPUout_FIFO";
	}

	// Check for "reserved" regions -- registers that most likely discard writes and
	// return 0 when read.  To assist in having useful logs, we determine the general
	// "zone" of the register address and return the zone name in the unknown string.

	EasyZoneR(RCNT0);	EasyZoneR(RCNT1);
	EasyZoneR(RCNT2);	EasyZoneR(RCNT3);

	EasyZoneR(IPU);		EasyZoneR(GIF);
	EasyZoneR(VIF0);	EasyZoneR(VIF1);
	EasyZoneR(VIF0dma);	EasyZoneR(VIF1dma);
	EasyZoneR(GIFdma);	EasyZoneR(fromIPU);	EasyZoneR(toIPU);
	EasyZoneR(SIF0dma);	EasyZoneR(SIF1dma);	EasyZoneR(SIF2dma);
	EasyZoneR(fromSPR);	EasyZoneR(toSPR);

	EasyZoneR(DMAC);	EasyZoneR(INTC);
	EasyZoneR(SIO);		EasyZoneR(SBUS);
	EasyZoneR(MCH);		EasyZoneR(DMACext);

	// If we get this far it's an *unknown* register; plain and simple.

	return NULL; //"Unknown";
}

template< typename T>
static __ri void eeHwTraceLog( u32 addr, T val, bool mode )
{
	if (!IsDevBuild) return;
	if (!EmuConfig.Trace.Enabled || !EmuConfig.Trace.EE.m_EnableAll || !EmuConfig.Trace.EE.m_EnableRegisters) return;
	if (!_eelog_enabled(addr)) return;

	FastFormatAscii valStr;
	FastFormatAscii labelStr;
	labelStr.Write("Hw%s%u", mode ? "Read" : "Write", sizeof (T) * 8);

	switch( sizeof(T) )
	{
		case 1: valStr.Write("0x%02x", val); break;
		case 2: valStr.Write("0x%04x", val); break;
		case 4: valStr.Write("0x%08x", val); break;

		case 8: valStr.Write("0x%08x.%08x", ((u32*)&val)[1], ((u32*)&val)[0]); break;
		case 16: ((u128&)val).WriteTo(valStr);
	}
			
	static const char* temp = "%-12s @ 0x%08X/%-16s %s %s";

	if( const char* regname = _eelog_GetHwName<T>( addr, val ) )
		HW_LOG( temp, labelStr.c_str(), addr, regname, mode ? "->" : "<-", valStr.c_str() );
	else
		UnknownHW_LOG( temp, labelStr.c_str(), addr, "Unknown", mode ? "->" : "<-", valStr.c_str() );
}