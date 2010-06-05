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

#include "GSdx.h"
#include "GSWnd.h"
#include "GSState.h"
#include "GSVertexTrace.h"
#include "GSVertexList.h"
#include "GSSettingsDlg.h"
#include "GSCapture.h"

class GSRenderer : public GSState
{
	GSCapture m_capture;
	string m_snapshot;
	int m_shader;

	bool Merge(int field);

protected:
	int m_interlace;
	int m_aspectratio;
	int m_filter;
	int m_upscale_multiplier;
	bool m_vsync;
	bool m_nativeres;
	bool m_aa1;
	bool m_framelimit;

	uint8* m_tex_buff;

	virtual GSTexture* GetOutput(int i) = 0;

	GSVertexTrace m_vt;

	// following functions need m_vt to be initialized

	void GetTextureMinMax(GSVector4i& r, bool linear);
	void GetAlphaMinMax();
	bool TryAlphaTest(uint32& fm, uint32& zm);
	bool IsLinear();
	bool IsOpaque();

public:
	GSWnd m_wnd;
	GSDevice* m_dev;

	int s_n;
	bool s_dump;
	bool s_save;
	bool s_savez;
	int s_saven;

public:
	GSRenderer();
	virtual ~GSRenderer();

	virtual bool CreateWnd(const string& title, int w, int h);
	virtual bool CreateDevice(GSDevice* dev);
	virtual void ResetDevice()
	{
		InvalidateTextureCache();
		ResetPrim();
		if( m_dev ) m_dev->Reset(1, 1);
	}
	virtual void VSync(int field);
	virtual bool MakeSnapshot(const string& path);
	virtual void KeyEvent(GSKeyEventData* e, int param = 0);
	virtual bool CanUpscale()
	{
		return !m_nativeres && m_regs->PMODE.EN != 0; // upscale ratio depends on the display size, with no output it may not be set correctly (ps2 logo to game transition)
	}
	virtual int upscale_Multiplier()
	{
		return m_upscale_multiplier;
	}

	void SetAspectRatio(int aspect)  { m_aspectratio = aspect; }
	void SetVsync(bool enabled);
	void SetFrameLimit(bool limit);
	virtual void SetExclusive(bool isExcl) {}

	// TODO : Implement proper locking here *if needed*  (not sure yet if it is) --air
	uint8* GetTextureBufferLock() { return m_tex_buff; }
	void ReleaseTextureBufferLock() { }
};

template<class Vertex> class GSRendererT : public GSRenderer
{
protected:
	Vertex* m_vertices;
	int m_count;
	int m_maxcount;
	GSVertexList<Vertex> m_vl;

	void Reset()
	{
		m_count = 0;
		m_vl.RemoveAll();

		__super::Reset();
	}

	void ResetPrim()
	{
		m_vl.RemoveAll();
	}

	void FlushPrim()
	{
		if(m_count == 0) return;

		if(GSLocalMemory::m_psm[m_context->FRAME.PSM].fmt < 3 && GSLocalMemory::m_psm[m_context->ZBUF.PSM].fmt < 3)
		{
			// FIXME: berserk fpsm = 27 (8H)

			if(!m_dev->IsLost())
			{
				m_vt.Update(m_vertices, m_count, GSUtil::GetPrimClass(PRIM->PRIM));

				Draw();
			}

			m_perfmon.Put(GSPerfMon::Draw, 1);
		}

		m_count = 0;
	}

	void GrowVertexBuffer()
	{
		m_maxcount = max(10000, m_maxcount * 3/2);
		m_vertices = (Vertex*)_aligned_realloc(m_vertices, sizeof(Vertex) * m_maxcount, 16);
		m_maxcount -= 100;
	}

	template<uint32 prim> __forceinline Vertex* DrawingKick(bool skip, int& count)
	{
		switch(prim)
		{
		case GS_POINTLIST: count = 1; break;
		case GS_LINELIST: count = 2; break;
		case GS_LINESTRIP: count = 2; break;
		case GS_TRIANGLELIST: count = 3; break;
		case GS_TRIANGLESTRIP: count = 3; break;
		case GS_TRIANGLEFAN: count = 3; break;
		case GS_SPRITE: count = 2; break;
		case GS_INVALID: count = 1; break;
		default: __assume(0);
		}

		if(m_vl.GetCount() < count)
		{
			return NULL;
		}

		if(m_count >= m_maxcount)
		{
			GrowVertexBuffer();
		}

		Vertex* v = &m_vertices[m_count];

		switch(prim)
		{
		case GS_POINTLIST:
			m_vl.GetAt(0, v[0]);
			m_vl.RemoveAll();
			break;
		case GS_LINELIST:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			break;
		case GS_LINESTRIP:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAt(0, 1);
			break;
		case GS_TRIANGLELIST:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAll();
			break;
		case GS_TRIANGLESTRIP:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAt(0, 2);
			break;
		case GS_TRIANGLEFAN:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.GetAt(2, v[2]);
			m_vl.RemoveAt(1, 1);
			break;
		case GS_SPRITE:
			m_vl.GetAt(0, v[0]);
			m_vl.GetAt(1, v[1]);
			m_vl.RemoveAll();
			break;
		case GS_INVALID:
			ASSERT(0);
			m_vl.RemoveAll();
			return NULL;
		default:
			__assume(0);
		}

		return !skip ? v : NULL;
	}

	virtual void Draw() = 0;

public:
	GSRendererT()
		: GSRenderer()
		, m_vertices(NULL)
		, m_count(0)
		, m_maxcount(0)
	{
	}

	virtual ~GSRendererT()
	{
		if(m_vertices) _aligned_free(m_vertices);
	}
};
