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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GPU.h"
#include "GPUDrawingEnvironment.h"
#include "GPULocalMemory.h"
#include "GPUVertex.h"
#include "GSAlignedClass.h"
#include "GSUtil.h"
#include "GSPerfMon.h"

class GPUState : public GSAlignedClass<32>
{
	typedef void (GPUState::*GPUStatusCommandHandler)(GPUReg* r);

	GPUStatusCommandHandler m_fpGPUStatusCommandHandlers[256];

	void SCH_Null(GPUReg* r);
	void SCH_ResetGPU(GPUReg* r);
	void SCH_ResetCommandBuffer(GPUReg* r);
	void SCH_ResetIRQ(GPUReg* r);
	void SCH_DisplayEnable(GPUReg* r);
	void SCH_DMASetup(GPUReg* r);
	void SCH_StartOfDisplayArea(GPUReg* r);
	void SCH_HorizontalDisplayRange(GPUReg* r);
	void SCH_VerticalDisplayRange(GPUReg* r);
	void SCH_DisplayMode(GPUReg* r);
	void SCH_GPUInfo(GPUReg* r);

	typedef int (GPUState::*GPUPacketHandler)(GPUReg* r, int size);

	GPUPacketHandler m_fpGPUPacketHandler[8];

	int PH_Command(GPUReg* r, int size);
	int PH_Polygon(GPUReg* r, int size);
	int PH_Line(GPUReg* r, int size);
	int PH_Sprite(GPUReg* r, int size);
	int PH_Move(GPUReg* r, int size);
	int PH_Write(GPUReg* r, int size);
	int PH_Read(GPUReg* r, int size);
	int PH_Environment(GPUReg* r, int size);

	class Buffer
	{
	public:
		int bytes;
		int maxbytes;
		uint8* buff;
		int cur;

	public:
		Buffer();
		~Buffer();
		void Reserve(int size);
		void Append(const uint8* src, int size);
		void Remove(int size);
		void RemoveAll();
	};

	Buffer m_write;
	Buffer m_read;

	void SetPrim(GPUReg* r);
	void SetCLUT(GPUReg* r);
	void SetTPAGE(GPUReg* r);

protected:

	int s_n;

	void Dump(const string& s, uint32 TP, const GSVector4i& r, int inc = true)
	{
		//if(m_perfmon.GetFrame() < 1000)
		//if((m_env.TWIN.u32 & 0xfffff) == 0)
		//if(!m_env.STATUS.ME && !m_env.STATUS.MD)
			return;

		if(inc) s_n++;

		//if(s_n < 86) return;

		int dir = 1;
#ifdef DEBUG
		dir = 2;
#endif
        string path = format("c:\\temp%d\\%04d_%s.bmp", dir, s_n, s.c_str());

		m_mem.SaveBMP(path, r, TP, m_env.CLUT.X, m_env.CLUT.Y);
	}

	void Dump(const string& s, int inc = true)
	{
		Dump(s, 2, GSVector4i(0, 0, 1024, 512), inc);
	}

public:
	GPUDrawingEnvironment m_env;
	GPULocalMemory m_mem;
	GPUVertex m_v;
	GSPerfMon m_perfmon;
	uint32 m_status[256];

public:
	GPUState();
	virtual ~GPUState();

	virtual void Reset();
	virtual void Flush();
	virtual void FlushPrim() = 0;
	virtual void ResetPrim() = 0;
	virtual void VertexKick() = 0;
	virtual void Invalidate(const GSVector4i& r);

	void WriteData(const uint8* mem, uint32 size);
	void ReadData(uint8* mem, uint32 size);

	void WriteStatus(uint32 status);
	uint32 ReadStatus();

	void Freeze(GPUFreezeData* data);
	void Defrost(const GPUFreezeData* data);
};

