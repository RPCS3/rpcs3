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

#include "stdafx.h"
#include "GPUState.h"

GPUState::GPUState()
	: s_n(0)
{
	memset(m_status, 0, sizeof(m_status));

	for(int i = 0; i < countof(m_fpGPUStatusCommandHandlers); i++)
	{
		m_fpGPUStatusCommandHandlers[i] = &GPUState::SCH_Null;
	}

	m_fpGPUStatusCommandHandlers[0x00] = &GPUState::SCH_ResetGPU;
	m_fpGPUStatusCommandHandlers[0x01] = &GPUState::SCH_ResetCommandBuffer;
	m_fpGPUStatusCommandHandlers[0x02] = &GPUState::SCH_ResetIRQ;
	m_fpGPUStatusCommandHandlers[0x03] = &GPUState::SCH_DisplayEnable;
	m_fpGPUStatusCommandHandlers[0x04] = &GPUState::SCH_DMASetup;
	m_fpGPUStatusCommandHandlers[0x05] = &GPUState::SCH_StartOfDisplayArea;
	m_fpGPUStatusCommandHandlers[0x06] = &GPUState::SCH_HorizontalDisplayRange;	
	m_fpGPUStatusCommandHandlers[0x07] = &GPUState::SCH_VerticalDisplayRange;
	m_fpGPUStatusCommandHandlers[0x08] = &GPUState::SCH_DisplayMode;
	m_fpGPUStatusCommandHandlers[0x10] = &GPUState::SCH_GPUInfo;

	m_fpGPUPacketHandler[0] = &GPUState::PH_Command;
	m_fpGPUPacketHandler[1] = &GPUState::PH_Polygon;
	m_fpGPUPacketHandler[2] = &GPUState::PH_Line;
	m_fpGPUPacketHandler[3] = &GPUState::PH_Sprite;
	m_fpGPUPacketHandler[4] = &GPUState::PH_Move;
	m_fpGPUPacketHandler[5] = &GPUState::PH_Write;
	m_fpGPUPacketHandler[6] = &GPUState::PH_Read;
	m_fpGPUPacketHandler[7] = &GPUState::PH_Environment;

	Reset();
}

GPUState::~GPUState()
{
}

void GPUState::Reset()
{
	m_env.Reset();

	m_mem.Invalidate(GSVector4i(0, 0, 1024, 512));

	memset(&m_v, 0, sizeof(m_v));
}

void GPUState::Flush()
{
	FlushPrim();
}

void GPUState::SetPrim(GPUReg* r)
{
	if(m_env.PRIM.TYPE != r->PRIM.TYPE)
	{
		ResetPrim();
	}

	GPURegPRIM PRIM = r->PRIM;
	
	PRIM.VTX = 0;

	switch(r->PRIM.TYPE)
	{
	case GPU_POLYGON: 
		PRIM.u32 = (r->PRIM.u32 & 0xF7000000) | 3; // TYPE IIP TME ABE TGE
		break;
	case GPU_LINE: 
		PRIM.u32 = (r->PRIM.u32 & 0xF2000000) | 2; // TYPE IIP ABE
		PRIM.TGE = 1; // ?
		break; 
	case GPU_SPRITE: 
		PRIM.u32 = (r->PRIM.u32 & 0xE7000000) | 2; // TYPE TME ABE TGE
		break; 
	}

	if(m_env.PRIM.u32 != PRIM.u32)
	{
		Flush();

		m_env.PRIM = PRIM;
	}
}

void GPUState::SetCLUT(GPUReg* r)
{
	uint32 mask = 0xFFFF0000; // X Y

	uint32 value = (m_env.CLUT.u32 & ~mask) | (r->u32 & mask);

	if(m_env.CLUT.u32 != value)
	{
		Flush();

		m_env.CLUT.u32 = value;
	}
}

void GPUState::SetTPAGE(GPUReg* r)
{
	uint32 mask = 0x000001FF; // TP ABR TY TX

	uint32 value = (m_env.STATUS.u32 & ~mask) | ((r->u32 >> 16) & mask);

	if(m_env.STATUS.u32 != value)
	{
		Flush();

		m_env.STATUS.u32 = value;
	}
}

void GPUState::Invalidate(const GSVector4i& r)
{
	m_mem.Invalidate(r);
}

void GPUState::WriteData(const uint8* mem, uint32 size)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	size <<= 2;

	m_write.Append(mem, size);

	int i = 0;

	while(i < m_write.bytes)
	{
		GPUReg* r = (GPUReg*)&m_write.buff[i];

		int ret = (this->*m_fpGPUPacketHandler[r->PACKET.TYPE])(r, (m_write.bytes - i) >> 2);

		if(ret == 0) return; // need more data

		i += ret << 2;
	}

	m_write.Remove(i);
}

void GPUState::ReadData(uint8* mem, uint32 size)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	int remaining = m_read.bytes - m_read.cur;

	int bytes = (int)size << 2;

	if(bytes > remaining)
	{
		// ASSERT(0);

		TRACE(_T("WARNING: ReadData\n"));

		// memset(&mem[remaining], 0, bytes - remaining);

		bytes = remaining;
	}

	memcpy(mem, &m_read.buff[m_read.cur], bytes);

	m_read.cur += bytes;

	if(m_read.cur >= m_read.bytes)
	{
		m_env.STATUS.IMG = 0;
	}
}

void GPUState::WriteStatus(uint32 status)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	uint32 b = status >> 24;

	m_status[b] = status;

	(this->*m_fpGPUStatusCommandHandlers[b])((GPUReg*)&status);	
}

uint32 GPUState::ReadStatus()
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	m_env.STATUS.LCF = ~m_env.STATUS.LCF; // ?

	return m_env.STATUS.u32;
}

void GPUState::Freeze(GPUFreezeData* data)
{
	data->status = m_env.STATUS.u32;
	memcpy(data->control, m_status, 256 * 4);
	m_mem.ReadRect(GSVector4i(0, 0, 1024, 512), data->vram);
}

void GPUState::Defrost(const GPUFreezeData* data)
{
	m_env.STATUS.u32 = data->status;
	memcpy(m_status, data->control, 256 * 4);
	m_mem.WriteRect(GSVector4i(0, 0, 1024, 512), data->vram);

	for(int i = 0; i <= 8; i++)
	{
		WriteStatus(m_status[i]);
	}
}

void GPUState::SCH_Null(GPUReg* r)
{
	ASSERT(0);
}

void GPUState::SCH_ResetGPU(GPUReg* r)
{
	Reset();
}

void GPUState::SCH_ResetCommandBuffer(GPUReg* r)
{
	// ?
}

void GPUState::SCH_ResetIRQ(GPUReg* r)
{
	// ?
}

void GPUState::SCH_DisplayEnable(GPUReg* r)
{
	m_env.STATUS.DEN = r->DEN.DEN;
}

void GPUState::SCH_DMASetup(GPUReg* r)
{
	m_env.STATUS.DMA = r->DMA.DMA;
}

void GPUState::SCH_StartOfDisplayArea(GPUReg* r)
{
	m_env.DAREA = r->DAREA;
}

void GPUState::SCH_HorizontalDisplayRange(GPUReg* r)
{
	m_env.DHRANGE = r->DHRANGE;
}

void GPUState::SCH_VerticalDisplayRange(GPUReg* r)
{
	m_env.DVRANGE = r->DVRANGE;
}

void GPUState::SCH_DisplayMode(GPUReg* r)
{
	m_env.STATUS.WIDTH0 = r->DMODE.WIDTH0;
	m_env.STATUS.HEIGHT = r->DMODE.HEIGHT;
	m_env.STATUS.ISPAL = r->DMODE.ISPAL;
	m_env.STATUS.ISRGB24 = r->DMODE.ISRGB24;
	m_env.STATUS.ISINTER = r->DMODE.ISINTER;
	m_env.STATUS.WIDTH1 = r->DMODE.WIDTH1;
}

void GPUState::SCH_GPUInfo(GPUReg* r)
{
	uint32 value = 0;

	switch(r->GPUINFO.PARAM)
	{
	case 0x2: 
		value = m_env.TWIN.u32; 
		break;
	case 0x0:
	case 0x1:
	case 0x3: 
		value = m_env.DRAREATL.u32; 
		break;
	case 0x4: 
		value = m_env.DRAREABR.u32; 
		break;
	case 0x5: 
	case 0x6: 
		value = m_env.DROFF.u32; 
		break;
	case 0x7: 
		value = 2; 
		break;
	case 0x8: 
	case 0xf: 
		value = 0xBFC03720; // ?
		break;
	default: 
		ASSERT(0); 
		break;
	}

	m_read.RemoveAll();
	m_read.Append((uint8*)&value, 4);
	m_read.cur = 0;
}

int GPUState::PH_Command(GPUReg* r, int size)
{
	switch(r->PACKET.OPTION)
	{
	case 0: // ???

		return 1;

	case 1: // clear cache

		return 1;

	case 2: // fillrect
	
		if(size < 3) return 0;

		Flush();

		GSVector4i r2;

		r2.left = r[1].XY.X;
		r2.top = r[1].XY.Y;
		r2.right = r2.left + r[2].XY.X;
		r2.bottom = r2.top + r[2].XY.Y;

		uint16 c = (uint16)(((r[0].RGB.R >> 3) << 10) | ((r[0].RGB.R >> 3) << 5) | (r[0].RGB.R >> 3));

		m_mem.FillRect(r2, c);

		Invalidate(r2);

		Dump("f");

		return 3;
	}

	ASSERT(0);

	return 1;
}

int GPUState::PH_Polygon(GPUReg* r, int size)
{
	int required = 1;

	int vertices = r[0].POLYGON.VTX ? 4 : 3;

	required += vertices;

	if(r[0].POLYGON.TME) required += vertices;

	if(r[0].POLYGON.IIP) required += vertices - 1;	

	if(size < required) return 0;

	//

	SetPrim(r);

	if(r[0].POLYGON.TME)
	{
		SetCLUT(&r[2]);

		SetTPAGE(&r[r[0].POLYGON.IIP ? 5 : 4]);
	}

	//

	GPUVertex v[4];

	for(int i = 0, j = 0; j < vertices; j++)
	{
		v[j].RGB = r[r[0].POLYGON.IIP ? i : 0].RGB;

		if(j == 0 || r[0].POLYGON.IIP) i++;

		v[j].XY = r[i++].XY;
		
		if(r[0].POLYGON.TME)
		{
			v[j].UV.X = r[i].UV.U;
			v[j].UV.Y = r[i].UV.V;

			i++;
		}
	}

	for(int i = 0; i <= vertices - 3; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			m_v = v[i + j];

			VertexKick();
		}
	}

	//

	return required;
}

int GPUState::PH_Line(GPUReg* r, int size)
{
	int required = 1;

	int vertices = 0;

	if(r->LINE.PLL)
	{
		required++;

		for(int i = 1; i < size; i++)
		{
			if(r[i].u32 == 0x55555555)
			{
				vertices = i - 1;
			}
		}

		if(vertices < 2)
		{
			return 0;
		}
	}
	else
	{
		vertices = 2;
	}

	required += vertices;

	if(r->LINE.IIP) required += vertices - 1;

	//

	SetPrim(r);

	//

	for(int i = 0, j = 0; j < vertices; j++)
	{
		if(j >= 2) VertexKick();

		m_v.RGB = r[r[0].LINE.IIP ? i : 0].RGB;

		if(j == 0 || r[0].LINE.IIP) i++;

		m_v.XY = r[i++].XY;

		VertexKick();
	}

	//

	return required;
}

int GPUState::PH_Sprite(GPUReg* r, int size)
{
	int required = 2;

	if(r[0].SPRITE.TME) required++;
	if(r[0].SPRITE.SIZE == 0) required++;

	if(size < required) return 0;

	//

	SetPrim(r);

	if(r[0].SPRITE.TME)
	{
		SetCLUT(&r[2]);
	}

	//

	int i = 0;

	m_v.RGB = r[i++].RGB;

	m_v.XY = r[i++].XY;
	
	if(r[0].SPRITE.TME)
	{
		m_v.UV.X = r[i].UV.U;
		m_v.UV.Y = r[i].UV.V;

		i++;
	}

	VertexKick();

	int w = 0;
	int h = 0;

	switch(r[0].SPRITE.SIZE)
	{
	case 0: w = r[i].XY.X; h = r[i].XY.Y; i++; break;
	case 1: w = h = 1; break;
	case 2: w = h = 8; break;
	case 3: w = h = 16; break;
	default: __assume(0);
	}

	m_v.XY.X += w;
	m_v.XY.Y += h;

	if(r[0].SPRITE.TME)
	{
		m_v.UV.X += w;
		m_v.UV.Y += h;
	}

	VertexKick();

	//

	return required;
}

int GPUState::PH_Move(GPUReg* r, int size)
{
	if(size < 4) return 0;

	Flush();

	int sx = r[1].XY.X;
	int sy = r[1].XY.Y;

	int dx = r[2].XY.X;
	int dy = r[2].XY.Y;

	int w = r[3].XY.X;
	int h = r[3].XY.Y;

	m_mem.MoveRect(sx, sy, dx, dy, w, h);

	Invalidate(GSVector4i(dx, dy, dx + w, dy + h));

	// Dump("m");

	return 4;
}

int GPUState::PH_Write(GPUReg* r, int size)
{
	if(size < 3) return 0;

	int w = r[2].XY.X;
	int h = r[2].XY.Y;

	int required = 3 + ((w * h + 1) >> 1);

	if(size < required) return 0;

	Flush();

	GSVector4i r2;

	r2.left = r[1].XY.X;
	r2.top = r[1].XY.Y;
	r2.right = r2.left + w;
	r2.bottom = r2.top + h;

	m_mem.WriteRect(r2, (const uint16*)&r[3]);

	Invalidate(r2);

	Dump("w");

	m_perfmon.Put(GSPerfMon::Swizzle, w * h * 2);

	return required;
}

int GPUState::PH_Read(GPUReg* r, int size)
{
	if(size < 3) return 0;

	Flush();

	int w = r[2].XY.X;
	int h = r[2].XY.Y;

	GSVector4i r2;

	r2.left = r[1].XY.X;
	r2.top = r[1].XY.Y;
	r2.right = r2.left + w;
	r2.bottom = r2.top + h;

	m_read.bytes = ((w * h + 1) & ~1) * 2;
	m_read.cur = 0;
	m_read.Reserve(m_read.bytes);

	m_mem.ReadRect(r2, (uint16*)m_read.buff);

	Dump("r");

	m_env.STATUS.IMG = 1;

	return 3;
}

int GPUState::PH_Environment(GPUReg* r, int size)
{
	Flush(); // TODO: only call when something really changes

	switch(r->PACKET.OPTION)
	{
	case 1: // draw mode setting

		m_env.STATUS.TX = r->MODE.TX;
		m_env.STATUS.TY = r->MODE.TY;
		m_env.STATUS.ABR = r->MODE.ABR;
		m_env.STATUS.TP = r->MODE.TP;
		m_env.STATUS.DTD = r->MODE.DTD;
		m_env.STATUS.DFE = r->MODE.DFE;

		return 1;

	case 2: // texture window setting 

		m_env.TWIN = r->TWIN;

		return 1;

	case 3: // set drawing area top left

		m_env.DRAREATL = r->DRAREA;

		return 1;

	case 4: // set drawing area bottom right

		m_env.DRAREABR = r->DRAREA;

		return 1;

	case 5: // drawing offset

		m_env.DROFF = r->DROFF;

		return 1;

	case 6: // mask setting

		m_env.STATUS.MD = r->MASK.MD;
		m_env.STATUS.ME = r->MASK.ME;

		return 1;
	}

	ASSERT(0);

	return 1;
}

//

GPUState::Buffer::Buffer()
{
	bytes = 0;
	maxbytes = 4096;
	buff = (uint8*)_aligned_malloc(maxbytes, 16);
	cur = 0;
}

GPUState::Buffer::~Buffer()
{
	_aligned_free(buff);
}

void GPUState::Buffer::Reserve(int size)
{
	if(size > maxbytes)
	{
		maxbytes = (maxbytes + size + 1023) & ~1023;

		buff = (uint8*)_aligned_realloc(buff, maxbytes, 16);
	}
}

void GPUState::Buffer::Append(const uint8* src, int size)
{
	Reserve(bytes + (int)size);

	memcpy(&buff[bytes], src, size);

	bytes += size;
}

void GPUState::Buffer::Remove(int size)
{
	ASSERT(size <= bytes);

	if(size < bytes)
	{
		memmove(&buff[0], &buff[size], bytes - size);

		bytes -= size;
	}
	else
	{
		bytes = 0;
	}

	#ifdef DEBUG
	memset(&buff[bytes], 0xff, maxbytes - bytes);
	#endif
}

void GPUState::Buffer::RemoveAll()
{
	bytes = 0;
}
