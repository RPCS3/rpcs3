/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GS.h"
#include "GSLocalMemory.h"
#include "GSDrawingContext.h"
#include "GSDrawingEnvironment.h"
#include "GSVertex.h"
#include "GSVertexList.h"
#include "GSUtil.h"
#include "GSDirtyRect.h"
#include "GSPerfMon.h"
#include "GSVector.h"
#include "GSDevice.h"
#include "GSCrc.h"
#include "GSAlignedClass.h"
#include "GSDump.h"

class GSState : public GSAlignedClass<32>
{
	typedef void (GSState::*GIFPackedRegHandler)(const GIFPackedReg* r);
	
	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16];

	void GIFPackedRegHandlerNull(const GIFPackedReg* r);
	void GIFPackedRegHandlerRGBA(const GIFPackedReg* r);
	void GIFPackedRegHandlerSTQ(const GIFPackedReg* r);
	void GIFPackedRegHandlerUV(const GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF2(const GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ2(const GIFPackedReg* r);
	void GIFPackedRegHandlerFOG(const GIFPackedReg* r);
	void GIFPackedRegHandlerA_D(const GIFPackedReg* r);
	void GIFPackedRegHandlerNOP(const GIFPackedReg* r);

	typedef void (GSState::*GIFRegHandler)(const GIFReg* r);

	GIFRegHandler m_fpGIFRegHandlers[256];

	void ApplyTEX0(uint i, GIFRegTEX0& TEX0);
	void ApplyPRIM(const GIFRegPRIM& PRIM);

	void GIFRegHandlerNull(const GIFReg* r);
	void GIFRegHandlerPRIM(const GIFReg* r);
	void GIFRegHandlerRGBAQ(const GIFReg* r);
	void GIFRegHandlerST(const GIFReg* r);
	void GIFRegHandlerUV(const GIFReg* r);
	void GIFRegHandlerXYZF2(const GIFReg* r);
	void GIFRegHandlerXYZ2(const GIFReg* r);
	template<int i> void GIFRegHandlerTEX0(const GIFReg* r);
	template<int i> void GIFRegHandlerCLAMP(const GIFReg* r);
	void GIFRegHandlerFOG(const GIFReg* r);
	void GIFRegHandlerXYZF3(const GIFReg* r);
	void GIFRegHandlerXYZ3(const GIFReg* r);
	void GIFRegHandlerNOP(const GIFReg* r);
	template<int i> void GIFRegHandlerTEX1(const GIFReg* r);
	template<int i> void GIFRegHandlerTEX2(const GIFReg* r);
	template<int i> void GIFRegHandlerXYOFFSET(const GIFReg* r);
	void GIFRegHandlerPRMODECONT(const GIFReg* r);
	void GIFRegHandlerPRMODE(const GIFReg* r);
	void GIFRegHandlerTEXCLUT(const GIFReg* r);
	void GIFRegHandlerSCANMSK(const GIFReg* r);
	template<int i> void GIFRegHandlerMIPTBP1(const GIFReg* r);
	template<int i> void GIFRegHandlerMIPTBP2(const GIFReg* r);
	void GIFRegHandlerTEXA(const GIFReg* r);
	void GIFRegHandlerFOGCOL(const GIFReg* r);
	void GIFRegHandlerTEXFLUSH(const GIFReg* r);
	template<int i> void GIFRegHandlerSCISSOR(const GIFReg* r);
	template<int i> void GIFRegHandlerALPHA(const GIFReg* r);
	void GIFRegHandlerDIMX(const GIFReg* r);
	void GIFRegHandlerDTHE(const GIFReg* r);
	void GIFRegHandlerCOLCLAMP(const GIFReg* r);
	template<int i> void GIFRegHandlerTEST(const GIFReg* r);
	void GIFRegHandlerPABE(const GIFReg* r);
	template<int i> void GIFRegHandlerFBA(const GIFReg* r);
	template<int i> void GIFRegHandlerFRAME(const GIFReg* r);
	template<int i> void GIFRegHandlerZBUF(const GIFReg* r);
	void GIFRegHandlerBITBLTBUF(const GIFReg* r);
	void GIFRegHandlerTRXPOS(const GIFReg* r);
	void GIFRegHandlerTRXREG(const GIFReg* r);
	void GIFRegHandlerTRXDIR(const GIFReg* r);
	void GIFRegHandlerHWREG(const GIFReg* r);
	void GIFRegHandlerSIGNAL(const GIFReg* r);
	void GIFRegHandlerFINISH(const GIFReg* r);
	void GIFRegHandlerLABEL(const GIFReg* r);

	int m_version;
	int m_sssize;

	bool m_mt;
	void (*m_irq)();
	bool m_path3hack;

	struct GSTransferBuffer
	{
		int x, y;
		int start, end, total;
		bool overflow;
		uint8* buff;

		GSTransferBuffer();
		virtual ~GSTransferBuffer();

		void Init(int tx, int ty);
		bool Update(int tw, int th, int bpp, int& len);

	} m_tr;

	void FlushWrite();

protected:
	bool IsBadFrame(int& skip, int UserHacks_SkipDraw);

	typedef void (GSState::*VertexKickPtr)(bool skip);

	VertexKickPtr m_vk[8][2][2];
	VertexKickPtr m_vkf;

	template<class T> void InitVertexKick()
	{
		m_vk[GS_POINTLIST][0][0] = (VertexKickPtr)&T::VertexKick<GS_POINTLIST, 0, 0>;
		m_vk[GS_POINTLIST][0][1] = (VertexKickPtr)&T::VertexKick<GS_POINTLIST, 0, 0>;
		m_vk[GS_POINTLIST][1][0] = (VertexKickPtr)&T::VertexKick<GS_POINTLIST, 1, 0>;
		m_vk[GS_POINTLIST][1][1] = (VertexKickPtr)&T::VertexKick<GS_POINTLIST, 1, 1>;

		m_vk[GS_LINELIST][0][0] = (VertexKickPtr)&T::VertexKick<GS_LINELIST, 0, 0>;
		m_vk[GS_LINELIST][0][1] = (VertexKickPtr)&T::VertexKick<GS_LINELIST, 0, 0>;
		m_vk[GS_LINELIST][1][0] = (VertexKickPtr)&T::VertexKick<GS_LINELIST, 1, 0>;
		m_vk[GS_LINELIST][1][1] = (VertexKickPtr)&T::VertexKick<GS_LINELIST, 1, 1>;

		m_vk[GS_LINESTRIP][0][0] = (VertexKickPtr)&T::VertexKick<GS_LINESTRIP, 0, 0>;
		m_vk[GS_LINESTRIP][0][1] = (VertexKickPtr)&T::VertexKick<GS_LINESTRIP, 0, 0>;
		m_vk[GS_LINESTRIP][1][0] = (VertexKickPtr)&T::VertexKick<GS_LINESTRIP, 1, 0>;
		m_vk[GS_LINESTRIP][1][1] = (VertexKickPtr)&T::VertexKick<GS_LINESTRIP, 1, 1>;

		m_vk[GS_TRIANGLELIST][0][0] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLELIST, 0, 0>;
		m_vk[GS_TRIANGLELIST][0][1] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLELIST, 0, 0>;
		m_vk[GS_TRIANGLELIST][1][0] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLELIST, 1, 0>;
		m_vk[GS_TRIANGLELIST][1][1] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLELIST, 1, 1>;

		m_vk[GS_TRIANGLESTRIP][0][0] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLESTRIP, 0, 0>;
		m_vk[GS_TRIANGLESTRIP][0][1] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLESTRIP, 0, 0>;
		m_vk[GS_TRIANGLESTRIP][1][0] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLESTRIP, 1, 0>;
		m_vk[GS_TRIANGLESTRIP][1][1] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLESTRIP, 1, 1>;

		m_vk[GS_TRIANGLEFAN][0][0] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLEFAN, 0, 0>;
		m_vk[GS_TRIANGLEFAN][0][1] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLEFAN, 0, 0>;
		m_vk[GS_TRIANGLEFAN][1][0] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLEFAN, 1, 0>;
		m_vk[GS_TRIANGLEFAN][1][1] = (VertexKickPtr)&T::VertexKick<GS_TRIANGLEFAN, 1, 1>;

		m_vk[GS_SPRITE][0][0] = (VertexKickPtr)&T::VertexKick<GS_SPRITE, 0, 0>;
		m_vk[GS_SPRITE][0][1] = (VertexKickPtr)&T::VertexKick<GS_SPRITE, 0, 0>;
		m_vk[GS_SPRITE][1][0] = (VertexKickPtr)&T::VertexKick<GS_SPRITE, 1, 0>;
		m_vk[GS_SPRITE][1][1] = (VertexKickPtr)&T::VertexKick<GS_SPRITE, 1, 1>;

		m_vk[GS_INVALID][0][0] = &GSState::VertexKickNull;
		m_vk[GS_INVALID][0][1] = &GSState::VertexKickNull;
		m_vk[GS_INVALID][1][0] = &GSState::VertexKickNull;
		m_vk[GS_INVALID][1][1] = &GSState::VertexKickNull;
	}

	void UpdateVertexKick()
	{
		m_vkf = m_vk[PRIM->PRIM][PRIM->TME][PRIM->FST];
	}

	void VertexKickNull(bool skip)
	{
		ASSERT(0);
	}

	void VertexKick(bool skip)
	{
		(this->*m_vkf)(skip);
	}

public:
	GIFPath m_path[4];
	GIFRegPRIM* PRIM;
	GSPrivRegSet* m_regs;
	GSLocalMemory m_mem;
	GSDrawingEnvironment m_env;
	GSDrawingContext* m_context;
	GSVertex m_v;
	float m_q;
	uint32 m_vprim;

	GSPerfMon m_perfmon;
	uint32 m_crc;
	int m_options;
	int m_frameskip;
	bool m_framelimit;
	CRC::Game m_game;
	GSDump m_dump;

public:
	GSState();
	virtual ~GSState();

	void ResetHandlers();

	GSVector4i GetDisplayRect(int i = -1);
	GSVector4i GetFrameRect(int i = -1);
	GSVector2i GetDeviceSize(int i = -1);

	bool IsEnabled(int i);

	int GetFPS();

	virtual void Reset();
	virtual void Flush();
	virtual void FlushPrim() = 0;
	virtual void ResetPrim() = 0;
	virtual void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r) {}
	virtual void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r) {}
	virtual void InvalidateTextureCache() {}

	void Move();
	void Write(const uint8* mem, int len);
	void Read(uint8* mem, int len);

	void SoftReset(uint32 mask);
	void WriteCSR(uint32 csr) {m_regs->CSR.u32[1] = csr;}
	void ReadFIFO(uint8* mem, int size);
	template<int index> void Transfer(const uint8* mem, uint32 size);
	int Freeze(GSFreezeData* fd, bool sizeonly);
	int Defrost(const GSFreezeData* fd);
	void GetLastTag(uint32* tag) {*tag = m_path3hack; m_path3hack = 0;}
	virtual void SetGameCRC(uint32 crc, int options);
	void SetFrameSkip(int skip);
	void SetRegsMem(uint8* basemem);
	void SetIrqCallback(void (*irq)());
	void SetMultithreaded(bool mt = true);
};

