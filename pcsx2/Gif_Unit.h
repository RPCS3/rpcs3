
#pragma once
#include "System/SysThreads.h"
extern void Gif_FinishIRQ();
extern bool Gif_HandlerAD(u8* pMem);

struct Gif_Tag {
	struct HW_Gif_Tag {
		u16 NLOOP	: 15;
		u16 EOP		: 1;
		u16 _dummy0	: 16;
		u32 _dummy1	: 14;
		u32 PRE		: 1;
		u32 PRIM	: 11;
		u32 FLG		: 2;
		u32 NREG	: 4;
		u32 REGS[2];
	} tag;

	u32  nLoop;		// NLOOP left to process
	u32  nRegs;		// NREG (1~16)
	u32  nRegIdx;	// Current nReg Index (packed mode processing)
	u32  len;		// Packet Length in Bytes (not including tag)
	u32  cycles;    // Time needed to process packet data in ee-cycles
	u8   regs[16];	// Regs
	bool hasAD;		// Has an A+D Write
	bool isValid;	// Tag is valid
	
	Gif_Tag() { Reset(); }
	Gif_Tag(u8* pMem, bool analyze = false) {
		setTag (pMem, analyze);
	}

	void Reset()  { memzero(*this); }
	u8   curReg() { return regs[nRegIdx&0xf]; }

	void packedStep() {
		if (nLoop > 0) {
			nRegIdx++;
			if (nRegIdx >= nRegs) {
				nRegIdx = 0;
				nLoop--;
			}
		}
	}

	void setTag(u8* pMem, bool analyze = false) {
		tag   = *(HW_Gif_Tag*)pMem;
		nLoop = tag.NLOOP;
		hasAD = false;
		nRegIdx = 0;
		isValid = 1;
		switch(tag.FLG) {
			case GIF_FLG_PACKED:
				nRegs  = ((tag.NREG-1)&0xf) + 1;
				len    = (nRegs * tag.NLOOP)* 16;
				cycles = len << 1; // Packed Mode takes 2 ee-cycles
				if (analyze) analyzeTag();
				break;
			case GIF_FLG_REGLIST:
				nRegs  = ((tag.NREG-1)&0xf) + 1;
				len    =((nRegs * tag.NLOOP + 1) >> 1) * 16;
				cycles = len << 2; // Reg-list Mode takes 4 ee-cycles
				break;
			case GIF_FLG_IMAGE:
			case GIF_FLG_IMAGE2:
				nRegs  = 0;
				len    = tag.NLOOP * 16;
				cycles = len << 2; // Image Mode takes 4 ee-cycles
				tag.FLG = GIF_FLG_IMAGE;
				break;
			jNO_DEFAULT;
		}
	}

	void analyzeTag() {
		hasAD = false;
		u32 t = tag.REGS[0];
		for(u32 i = 0; i < nRegs; i++) {
			if (i == 8) t = tag.REGS[1];
			regs[i] = t & 0xf;
			hasAD  |= (regs[i] == GIF_REG_A_D);
			t >>= 4;
		}
	}
};

struct GS_Packet {
	u32  offset; // Path buffer offset for start of packet
	u32  size;	 // Full size of GS-Packet
	s32  cycles; // EE Cycles taken to process this GS packet
	bool done;	 // 0 = Incomplete, 1 = Complete
	GS_Packet()  { Reset(); }
	void Reset() { memzero(*this); }
};

struct GS_SIGNAL {
	u32  data[2];
	bool queued;
	void Reset() { memzero(*this); }
};

static __fi void incTag(u32& offset, u32& size, u32 incAmount) {
	size   += incAmount;
	offset += incAmount;
}

struct Gif_Path {
	volatile s32 __aligned(4) readAmount; // Amount of data MTGS still needs to read
	u8* buffer;		  // Path packet buffer
	u32 buffSize;	  // Full size of buffer
	u32 buffLimit;	  // Cut off limit to wrap around
	u32 curSize;	  // Used buffer in bytes
	u32 curOffset;	  // Offset of current gifTag
	Gif_Tag   gifTag; // Current GS Primitive tag
	GS_Packet gsPack; // Current GS Packet info
	GIF_PATH  idx;	  // Gif Path Index
	GIF_PATH_STATE state; // Path State

	Gif_Path()  {}
	~Gif_Path() { _aligned_free(buffer); }

	void Init(GIF_PATH _idx, u32 _buffSize, u32 _buffSafeZone) {
		idx       = _idx;
		buffSize  = _buffSize;
		buffLimit = _buffSize - _buffSafeZone;
		buffer    = (u8*)_aligned_malloc(buffSize, 16);
		Reset();
	}

	void Reset(bool softReset = false) {
		state = GIF_PATH_IDLE;
		if (softReset) {
			GUNIT_WARN("Gif Path %d - Soft Reset", idx+1);
			//gifTag.Reset();
			//gsPack.Reset();
			//curOffset = curSize;
			return;
		}
		curSize     = 0;
		curOffset   = 0;
		readAmount  = 0;
		gifTag.Reset();
		gsPack.Reset();
	}

	bool hasDataRemaining() { return curOffset < curSize; }
	bool isDone() { return !hasDataRemaining() && state == GIF_PATH_IDLE; }

	// Waits on the MTGS to process gs packets
	void mtgsReadWait() {
		//pxAssertDev(AtomicExchangeAdd(readAmount, 0) != 0, "Gif Path Buffer Overflow!");
		DevCon.WriteLn(Color_Red, "Gif Path[%d] - MTGS Wait! [r=0x%x]", 
			           idx+1, AtomicExchangeAdd(readAmount, 0));
		extern void Gif_MTGS_Wait();
		Gif_MTGS_Wait();
	}

	// Moves packet data to start of buffer
	void RealignPacket() {
		extern void Gif_AddBlankGSPacket(u32 size, GIF_PATH path);
		GUNIT_LOG("Path Buffer: Realigning packet!");
		s32 offset    = curOffset - gsPack.size;
		s32 sizeToAdd = curSize   - offset;
		s32 intersect = sizeToAdd - offset;
		if (intersect < 0) intersect = 0;
		for(;;) {
			s32 frontFree  = offset - AtomicExchangeAdd(readAmount, 0);
			if (frontFree >= sizeToAdd - intersect) break;
			mtgsReadWait();
		}
		if (offset < (s32)buffLimit) { // Needed for correct readAmount values
			Gif_AddBlankGSPacket(buffLimit - offset, idx);
		}
		//DevCon.WriteLn("Realign Packet [%d]", curSize - offset);
		if (intersect) memmove(buffer, &buffer[offset], curSize - offset);
		else       memcpy_fast(buffer, &buffer[offset], curSize - offset);
		curSize      -= offset;
		curOffset     = gsPack.size;
		gsPack.offset = 0;
	}

	void CopyGSPacketData(u8* pMem, u32 size, bool aligned = false) {	
		if (curSize + size > buffSize) { // Move gsPack to front of buffer
			DevCon.Warning("CopyGSPacketData: Realigning packet!");
			RealignPacket();
		}
		for(;;) {
			s32 offset  = curOffset - gsPack.size;
			s32 readPos = offset    - AtomicExchangeAdd(readAmount, 0);
			if (readPos >= 0) break; // MTGS is reading in back of curOffset
			if ((s32)buffLimit + readPos > (s32)curSize + (s32)size) break; // Enough free front space
			mtgsReadWait(); // Let MTGS run to free up buffer space
		}
		pxAssertDev(curSize+size<=buffSize, "Gif Path Buffer Overflow!");
		if (aligned) memcpy_qwc (&buffer[curSize], pMem, size/16);
		else		 memcpy_fast(&buffer[curSize], pMem, size);
		curSize     += size;
	}

	// If completed a GS packet (with EOP) then returned GS_Packet.done = 1
	GS_Packet ExecuteGSPacket() {
		for(;;) {
			if (!gifTag.isValid) { // Need new Gif Tag
				// We don't have enough data for a Gif Tag
				if (curOffset + 16 > curSize) {
					GUNIT_LOG("Path Buffer: Not enough data for gif tag! [%d]", curSize-curOffset);
					return gsPack;
				}

				// Move packet to start of buffer
				if (curOffset > buffLimit) {
					RealignPacket();
				}

				gifTag.setTag(&buffer[curOffset], 1);
				state = (GIF_PATH_STATE)(gifTag.tag.FLG + 1);

				// We don't have enough data for a complete GS packet
				if (curOffset + 16 + gifTag.len > curSize) {
					gifTag.isValid = false; // So next time we test again
					return gsPack;
				}

				incTag(curOffset, gsPack.size, 16); // Tag Size
				gsPack.cycles += 2 + gifTag.cycles; // Tag + Len ee-cycles
			}

			if (gifTag.hasAD) { // Only can be true if GIF_FLG_PACKED
				bool dblSIGNAL = false;
				while(gifTag.nLoop && !dblSIGNAL) {
					if (gifTag.curReg() == GIF_REG_A_D) {
						dblSIGNAL = Gif_HandlerAD(&buffer[curOffset]);
					}
					incTag(curOffset, gsPack.size, 16); // 1 QWC
					gifTag.packedStep();
				}
				if (dblSIGNAL && !(gifTag.tag.EOP && !gifTag.nLoop)) return gsPack; // Exit Early
			}
			else incTag(curOffset, gsPack.size, gifTag.len); // Data length

			// Reload gif tag next loop
			gifTag.isValid = false;

			if (gifTag.tag.EOP) {
				GS_Packet t = gsPack;
				t.done = 1;
				state  = GIF_PATH_IDLE;
				gsPack.Reset();
				gsPack.offset = curOffset;
				return t; // Complete GS packet
			}
		}
	}
};

struct Gif_Unit {
	Gif_Path   gifPath[3];
	GS_SIGNAL  gsSIGNAL; // Stalling Signal
	tGIF_STAT& stat;
	GIF_TRANSFER_TYPE lastTranType; // Last Transfer Type

	Gif_Unit() : stat(gifRegs.stat) {
		gifPath[0].Init(GIF_PATH_1, _1mb*8, _16kb + _1kb);
		gifPath[1].Init(GIF_PATH_2, _1mb*8, _1mb  + _1kb);
		gifPath[2].Init(GIF_PATH_3, _1mb*9, _1mb  + _1kb);
	}

	// Enable softReset when resetting during game emulation
	void Reset(bool softReset = false) {
		GUNIT_WARN(Color_Red, "Gif Unit Reset!!! [soft=%d]", softReset);
		ResetRegs();
		gsSIGNAL.Reset();
		gifPath[0].Reset(softReset);
		gifPath[1].Reset(softReset);
		gifPath[2].Reset(softReset);
		if(!softReset) {
			lastTranType = GIF_TRANS_INVALID;
		}
	}

	// Resets Gif HW Regs
	void ResetRegs() {
		gifRegs.stat.reset();
		gifRegs.ctrl.reset();
		gifRegs.mode.reset();
	}

	// Adds a finished GS Packet to the MTGS ring buffer
	__fi void AddCompletedGSPacket(GS_Packet& gsPack, GIF_PATH path) {
		extern void Gif_AddCompletedGSPacket(GS_Packet& gsPack, GIF_PATH path);
		Gif_AddCompletedGSPacket(gsPack, path);
	}

	// Returns GS Packet Size in bytes
	u32 GetGSPacketSize(GIF_PATH pathIdx, u8* pMem, u32 offset = 0) {
		u32 memMask = pathIdx ? 0xffffffffu : 0x3fffu;
		u32 size    = 0;
		for(;;) {
			Gif_Tag gifTag(&pMem[offset & memMask]);
			incTag(offset, size, 16 + gifTag.len); // Tag + Data length
			if (pathIdx == GIF_PATH_1 && size >= 0x4000) {
				Console.Warning("Gif Unit - GS packet size exceeded VU memory size!");
				return 0; // Bios does this... (Fixed if you delay vu1's xgkick by 103 vu cycles)
			}
			if (gifTag.tag.EOP) {
				return size;
			}
		}
	}

	// Specify the transfer type you are initiating
	// The return value is the amount of data (in bytes) that was processed
	// If transfer cannot take place at this moment the return value is 0
	u32 TransferGSPacketData(GIF_TRANSFER_TYPE tranType, u8* pMem, u32 size, bool aligned=false) {
		
		GIF_LOG("%s - [path=%d][size=%d]", Gif_TransferStr[(tranType>>8)&0xf], (tranType&3)+1, size);
		if (size == 0)  { GUNIT_WARN("Gif Unit - Size == 0"); return 0; }
		if(!CanDoGif()) { GUNIT_WARN("Gif Unit - PSE Set or Dir = GS to EE"); }
		pxAssertDev((stat.APATH==0) || checkPaths(1,1,1), "Gif Unit - APATH wasn't cleared?");
		lastTranType = tranType; // Used for Vif FlushA hack

		if (tranType == GIF_TRANS_FIFO) {
			if(!CanDoPath3()) DevCon.Warning("Gif Unit - Path 3 FIFO transfer while !CanDoPath3()");
		}
		if (tranType == GIF_TRANS_DMA) {
			if(!CanDoPath3())   { if (!Path3Masked()) stat.P3Q = 1; return 0; } // DMA Stall
		}
		if (tranType == GIF_TRANS_XGKICK) {
			if(!CanDoPath1())   { stat.P1Q = 1; } // We always buffer path1 packets
		}
		if (tranType == GIF_TRANS_DIRECT) {
			if(!CanDoPath2())   { stat.P2Q = 1; return 0; } // Direct Stall 
		}
		if (tranType == GIF_TRANS_DIRECTHL) {
			if(!CanDoPath2HL()) { stat.P2Q = 1; return 0; } // DirectHL Stall
		}

		gifPath[tranType&3].CopyGSPacketData(pMem, size, aligned);
		Execute();
		return size;
	}

	// Checks path activity for the given paths
	// Returns an int with a bit enabled if the corresponding
	// path is not finished (needs more data/processing for an EOP)
	__fi int checkPaths(bool p1, bool p2, bool p3, bool checkQ=false) {
		int ret = 0;
		ret |= (p1 && !gifPath[GIF_PATH_1].isDone()) << 0;
		ret |= (p2 && !gifPath[GIF_PATH_2].isDone()) << 1;
		ret |= (p3 && !gifPath[GIF_PATH_3].isDone()) << 2;
		return ret | (checkQ ? checkQueued(p1,p2,p3) : 0);
	}

	__fi int checkQueued(bool p1, bool p2, bool p3) {
		int ret = 0;
		ret |= (p1 && stat.P1Q) << 0;
		ret |= (p2 && stat.P2Q) << 1;
		ret |= (p3 && stat.P3Q) << 2;
		return ret;
	}

	// Processes gif packets and performs path arbitration
	// according to path priority...
	void Execute() {
		if (!CanDoGif()) { DevCon.Error("Gif Unit - Signal or PSE Set or Dir = GS to EE"); return; }
		bool didPath3 = false;
		stat.OPH = 1;
		for(;;) {
			if (stat.APATH) { // Some Transfer is happening
				GS_Packet gsPack = gifPath[stat.APATH-1].ExecuteGSPacket();
				if(!gsPack.done) {
					if (stat.APATH == 3 && CanDoP3Slice() && !gsSIGNAL.queued) {
						if(!didPath3 && checkPaths(1,1,0)) { // Path3 slicing
							didPath3 = true;
							stat.APATH = 0;
							stat.IP3   = 1;
							GUNIT_LOG(Color_Magenta, "Gif Unit - Path 3 slicing arbitration");
							if (gsPack.size > 16) { // Packet had other tags which we already processed
								Gif_Path&  p3 = gifPath[GIF_PATH_3];
								u32 subOffset = p3.gifTag.isValid ? 16 : 0; // if isValid, image-primitive not finished
								gsPack.size  -= subOffset; // Remove the image-tag (should be last thing read)
								AddCompletedGSPacket(gsPack, GIF_PATH_3); // Consider current packet complete
								p3.gsPack.Reset(); // Reset gs packet info
								p3.curOffset     -= subOffset;    // Start the next GS packet at the image-tag
								p3.gsPack.offset  = p3.curOffset; // Set to image-tag
								p3.gifTag.isValid = false; // Reload tag next ExecuteGSPacket()
								pxAssert((s32)p3.curOffset >= 0);
								pxAssert(p3.state == GIF_PATH_IMAGE);
								GUNIT_LOG(Color_Magenta, "Gif Unit - Sending path 3 sliced gs packet!");
							}
							continue;
						}
					}
					//DevCon.WriteLn("Incomplete GS Packet for path %d", stat.APATH);
					break; // Not finished with GS packet
				}
				//DevCon.WriteLn("Adding GS Packet for path %d", stat.APATH);
				AddCompletedGSPacket(gsPack, (GIF_PATH)(stat.APATH-1));
			}
			if   (!gsSIGNAL.queued && checkPaths(1,0,0,0)) { stat.APATH = 1; stat.P1Q = 0; }
			elif (!gsSIGNAL.queued && checkPaths(0,1,0,0)) { stat.APATH = 2; stat.P2Q = 0; }
			elif (!gsSIGNAL.queued && checkPaths(0,0,1,0) && !Path3Masked())
				 { stat.APATH = 3; stat.P3Q = 0; stat.IP3 = 0; }
			else { stat.APATH = 0; stat.OPH = 0; break; }
		}
		Gif_FinishIRQ(); // hmm
		//DevCon.WriteLn("APATH = %d [%d,%d,%d]", stat.APATH, !!checkPaths(1,0,0,0),!!checkPaths(0,1,0,0),!!checkPaths(0,0,1,0));
	}

	// XGkick
	bool CanDoPath1()	{ return (stat.APATH == 0 || stat.APATH == 1
							||   (stat.APATH == 3 && CanDoP3Slice()))
							&&   (CanDoGif() == 1); }
	// Direct
	bool CanDoPath2()   { return (stat.APATH == 0 || stat.APATH == 2
							||   (stat.APATH == 3 && CanDoP3Slice()))
							&&   (CanDoGif() == 1); }
	// DirectHL
	bool CanDoPath2HL() { return (stat.APATH == 0 || stat.APATH == 2)
							&&   (CanDoGif() == 1); }
	// Gif DMA
	bool CanDoPath3()   { return((stat.APATH == 0 && !Path3Masked())
							||	  stat.APATH == 3)
							&&   (CanDoGif() == 1); }

	bool CanDoP3Slice() { return stat.IMT == 1 && gifPath[GIF_PATH_3].state == GIF_PATH_IMAGE; }
	bool CanDoGif()		{ return stat.PSE == 0 && stat.DIR == 0 && gsSIGNAL.queued == 0; }
	bool Path3Masked()  { return stat.M3R || stat.M3P; }

	void PrintInfo(bool printP1=1, bool printP2=1, bool printP3=1) {
		u32 a = checkPaths(1,1,1), b = checkQueued(1,1,1);
		GUNIT_LOG("Gif Unit - LastTransfer = %s, Paths = [%d,%d,%d], Queued = [%d,%d,%d]",
				   Gif_TransferStr[(lastTranType>>8)&0xf],
				   !!(a&1),!!(a&2),!!(a&4),!!(b&1),!!(b&2),!!(b&4));
		GUNIT_LOG("Gif Unit - [APATH = %d][Signal = %d][PSE = %d][DIR = %d]",
			       stat.APATH, gsSIGNAL.queued, stat.PSE, stat.DIR);
		GUNIT_LOG("Gif Unit - [CanDoGif = %d][CanDoPath3 = %d][CanDoP3Slice = %d]",
				   CanDoGif(), CanDoPath3(), CanDoP3Slice());
		if (printP1) PrintPathInfo(GIF_PATH_1);
		if (printP2) PrintPathInfo(GIF_PATH_2);
		if (printP3) PrintPathInfo(GIF_PATH_3);
	}

	void PrintPathInfo(GIF_PATH path) {
		GUNIT_LOG("Gif Path %d - [hasData = %d][state = %d]", path,
			       gifPath[path].hasDataRemaining(), gifPath[path].state);
	}
};

extern Gif_Unit gifUnit;
