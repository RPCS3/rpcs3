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

class GSState : public GSAlignedClass<16>
{
	typedef void (GSState::*GIFPackedRegHandler)(GIFPackedReg* r);

	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16];

	void GIFPackedRegHandlerNull(GIFPackedReg* r);
	void GIFPackedRegHandlerPRIM(GIFPackedReg* r);
	void GIFPackedRegHandlerRGBA(GIFPackedReg* r);
	void GIFPackedRegHandlerSTQ(GIFPackedReg* r);
	void GIFPackedRegHandlerUV(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF2(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ2(GIFPackedReg* r);
	template<int i> void GIFPackedRegHandlerTEX0(GIFPackedReg* r);
	template<int i> void GIFPackedRegHandlerCLAMP(GIFPackedReg* r);
	void GIFPackedRegHandlerFOG(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF3(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ3(GIFPackedReg* r);
	void GIFPackedRegHandlerA_D(GIFPackedReg* r);
	void GIFPackedRegHandlerNOP(GIFPackedReg* r);

	typedef void (GSState::*GIFRegHandler)(GIFReg* r);

	GIFRegHandler m_fpGIFRegHandlers[256];

	void GIFRegHandlerNull(GIFReg* r);
	void GIFRegHandlerPRIM(GIFReg* r);
	void GIFRegHandlerRGBAQ(GIFReg* r);
	void GIFRegHandlerST(GIFReg* r);
	void GIFRegHandlerUV(GIFReg* r);
	void GIFRegHandlerXYZF2(GIFReg* r);
	void GIFRegHandlerXYZ2(GIFReg* r);
	template<int i> void GIFRegHandlerTEX0(GIFReg* r);
	template<int i> void GIFRegHandlerCLAMP(GIFReg* r);
	void GIFRegHandlerFOG(GIFReg* r);
	void GIFRegHandlerXYZF3(GIFReg* r);
	void GIFRegHandlerXYZ3(GIFReg* r);
	void GIFRegHandlerNOP(GIFReg* r);
	template<int i> void GIFRegHandlerTEX1(GIFReg* r);
	template<int i> void GIFRegHandlerTEX2(GIFReg* r);
	template<int i> void GIFRegHandlerXYOFFSET(GIFReg* r);
	void GIFRegHandlerPRMODECONT(GIFReg* r);
	void GIFRegHandlerPRMODE(GIFReg* r);
	void GIFRegHandlerTEXCLUT(GIFReg* r);
	void GIFRegHandlerSCANMSK(GIFReg* r);
	template<int i> void GIFRegHandlerMIPTBP1(GIFReg* r);
	template<int i> void GIFRegHandlerMIPTBP2(GIFReg* r);
	void GIFRegHandlerTEXA(GIFReg* r);
	void GIFRegHandlerFOGCOL(GIFReg* r);
	void GIFRegHandlerTEXFLUSH(GIFReg* r);
	template<int i> void GIFRegHandlerSCISSOR(GIFReg* r);
	template<int i> void GIFRegHandlerALPHA(GIFReg* r);
	void GIFRegHandlerDIMX(GIFReg* r);
	void GIFRegHandlerDTHE(GIFReg* r);
	void GIFRegHandlerCOLCLAMP(GIFReg* r);
	template<int i> void GIFRegHandlerTEST(GIFReg* r);
	void GIFRegHandlerPABE(GIFReg* r);
	template<int i> void GIFRegHandlerFBA(GIFReg* r);
	template<int i> void GIFRegHandlerFRAME(GIFReg* r);
	template<int i> void GIFRegHandlerZBUF(GIFReg* r);
	void GIFRegHandlerBITBLTBUF(GIFReg* r);
	void GIFRegHandlerTRXPOS(GIFReg* r);
	void GIFRegHandlerTRXREG(GIFReg* r);
	void GIFRegHandlerTRXDIR(GIFReg* r);
	void GIFRegHandlerHWREG(GIFReg* r);
	void GIFRegHandlerSIGNAL(GIFReg* r);
	void GIFRegHandlerFINISH(GIFReg* r);
	void GIFRegHandlerLABEL(GIFReg* r);

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
		BYTE* buff;

		GSTransferBuffer();
		virtual ~GSTransferBuffer();

		void Init(int tx, int ty);
		bool Update(int tw, int th, int bpp, int& len);

	} m_tr;

	void FlushWrite();

protected:
	bool IsBadFrame(int& skip);

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
	GIFPath m_path[3];
	GIFRegPRIM* PRIM;
	GSPrivRegSet* m_regs;
	GSLocalMemory m_mem;
	GSDrawingEnvironment m_env;
	GSDrawingContext* m_context;
	GSVertex m_v;
	float m_q;
	DWORD m_vprim;

	GSPerfMon m_perfmon;
	DWORD m_crc;
	int m_options;
	int m_frameskip;
	CRC::Game m_game;
	GSDump m_dump;

public:
	GSState(BYTE* base, bool mt, void (*irq)());
	virtual ~GSState();

	void ResetHandlers();

	CPoint GetDisplayPos(int i);
	CSize GetDisplaySize(int i);
	CRect GetDisplayRect(int i);
	CSize GetDisplayPos();
	CSize GetDisplaySize();
	CRect GetDisplayRect();
	CPoint GetFramePos(int i);
	CSize GetFrameSize(int i);
	CRect GetFrameRect(int i);
	CSize GetFramePos();
	CSize GetFrameSize();
	CRect GetFrameRect();
	CSize GetDeviceSize(int i);
	CSize GetDeviceSize();
	bool IsEnabled(int i);
	int GetFPS();

	virtual void Reset();
	virtual void Flush();
	virtual void FlushPrim() = 0;
	virtual void ResetPrim() = 0;
	virtual void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r) {}
	virtual void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r) {}
	virtual void InvalidateTextureCache() {}

	void Move();
	void Write(BYTE* mem, int len);
	void Read(BYTE* mem, int len);

	void SoftReset(BYTE mask);
	void WriteCSR(UINT32 csr) {m_regs->CSR.ai32[1] = csr;}
	void ReadFIFO(BYTE* mem, int size);
	template<int index> void Transfer(BYTE* mem, UINT32 size);
	int Freeze(GSFreezeData* fd, bool sizeonly);
	int Defrost(const GSFreezeData* fd);
	void GetLastTag(UINT32* tag) {*tag = m_path3hack; m_path3hack = 0;}
	virtual void SetGameCRC(DWORD crc, int options);
	void SetFrameSkip(int frameskip);
};

