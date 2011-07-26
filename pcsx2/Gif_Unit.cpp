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

#include "PrecompiledHeader.h"
#include "Common.h"

#include "GS.h"
#include "Gif_Unit.h"
#include "Vif_Dma.h"

Gif_Unit gifUnit;

// Returns true on stalling SIGNAL
bool Gif_HandlerAD(u8* pMem) {
	u32  reg  = pMem[8];
	u32* data = (u32*)pMem;
	if   (reg == 0x50) vif1.BITBLTBUF._u64	= *(u64*)pMem;
	elif (reg == 0x52) vif1.TRXREG._u64		= *(u64*)pMem;
	elif (reg == 0x53) { // TRXDIR
		if ((pMem[0] & 3) == 1) { // local -> host
			u8 bpp = 32; // Onimusha does TRXDIR without BLTDIVIDE first, assume 32bit
			switch(vif1.BITBLTBUF.SPSM & 7) {
				case 0: bpp = 32; break;
				case 1: bpp = 24; break;
				case 2: bpp = 16; break;
				case 3: bpp = 8;  break;
				default: // 4 is 4 bit but this is forbidden
					Console.Error("Illegal format for GS upload: SPSM=0%02o", vif1.BITBLTBUF.SPSM);
					break;
			}
			// qwords, rounded down; any extra bits are lost
			// games must take care to ensure transfer rectangles are exact multiples of a qword
			vif1.GSLastDownloadSize = vif1.TRXREG.RRW * vif1.TRXREG.RRH * bpp >> 7;
		}
	}
	elif (reg == 0x60) { // SIGNAL
		if (CSRreg.SIGNAL) { // Time to ignore all subsequent drawing operations.
			GUNIT_WARN(Color_Orange, "GIF Handler - Stalling SIGNAL");
			if(!gifUnit.gsSIGNAL.queued) {
				gifUnit.gsSIGNAL.queued  = true;
				gifUnit.gsSIGNAL.data[0] = data[0];
				gifUnit.gsSIGNAL.data[1] = data[1];
				return true; // Stalling SIGNAL
			}
		}
		else {
			GUNIT_WARN("GIF Handler - SIGNAL");
			GSSIGLBLID.SIGID = (GSSIGLBLID.SIGID&~data[1])|(data[0]&data[1]);
			if (!(GSIMR&0x100)) gsIrq();
			CSRreg.SIGNAL = true;
		}
	}
	elif (reg == 0x61) { // FINISH
		GUNIT_WARN("GIF Handler - FINISH");
		CSRreg.FINISH = true;
	}
	elif (reg == 0x62) { // LABEL
		GUNIT_WARN("GIF Handler - LABEL");
		GSSIGLBLID.LBLID = (GSSIGLBLID.LBLID&~data[1])|(data[0]&data[1]);
	}
	elif (reg >= 0x63 && reg != 0x7f) {
		DevCon.Warning("GIF Handler - Write to unknown register! [reg=%x]", reg);
	}
	return false;
}

void Gif_FinishIRQ() {
	if (CSRreg.FINISH && !(GSIMR&0x200)) {
		gsIrq();
	}
}

void Gif_AddCompletedGSPacket(GS_Packet& gsPack, GIF_PATH path) {
	//DevCon.WriteLn("Adding Completed Gif Packet [size=%x]", gsPack.size);
	if (COPY_GS_PACKET_TO_MTGS) {
		GetMTGS().PrepDataPacket(path, gsPack.size/16);
		MemCopy_WrappedDest((u128*)&gifUnit.gifPath[path].buffer[gsPack.offset], RingBuffer.m_Ring, 
							GetMTGS().m_packet_writepos, RingBufferSize, gsPack.size/16);
		GetMTGS().SendDataPacket();
	}
	else {
		AtomicExchangeAdd(gifUnit.gifPath[path].readAmount, gsPack.size);
		GetMTGS().SendSimpleGSPacket(GS_RINGTYPE_GSPACKET,  gsPack.offset, gsPack.size, path);
	}
}

void Gif_AddBlankGSPacket(u32 size, GIF_PATH path) {
	//DevCon.WriteLn("Adding Blank Gif Packet [size=%x]", size);
	AtomicExchangeAdd(gifUnit.gifPath[path].readAmount, size);
	GetMTGS().SendSimpleGSPacket(GS_RINGTYPE_GSPACKET, ~0u, size, path);
}

void Gif_MTGS_Wait() {
	GetMTGS().WaitGS();
}

void Gif_Execute() {
	gifUnit.Execute();
}

void SaveStateBase::gifPathFreeze(u32 path) {

	Gif_Path& gifPath = gifUnit.gifPath[path];
	pxAssertDev(gifPath.readAmount==0, "Gif Path readAmount should be 0!");
	if (IsSaving()) { // Move all the buffered data to the start of buffer
		gifPath.RealignPacket(); // May add readAmount which we need to clear on load
	}
	u8* bufferPtr = gifPath.buffer; // Backup current buffer ptr
	Freeze(gifPath);
	FreezeMem(bufferPtr, gifPath.curSize);
	gifPath.buffer = bufferPtr;
	if (!IsSaving()) gifPath.readAmount = 0;
}

void SaveStateBase::gifFreeze() {
	Gif_MTGS_Wait();
	FreezeTag("Gif Unit");
	Freeze(gifUnit.stat);
	Freeze(gifUnit.gsSIGNAL);
	Freeze(gifUnit.lastTranType);
	gifPathFreeze(GIF_PATH_1);
	gifPathFreeze(GIF_PATH_2);
	gifPathFreeze(GIF_PATH_3);
}
