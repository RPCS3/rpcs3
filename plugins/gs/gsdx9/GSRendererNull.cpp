/* 
 *	Copyright (C) 2003-2005 Gabest
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

#include "StdAfx.h"
#include "GSRendererNull.h"

GSRendererNull::GSRendererNull(HWND hWnd, HRESULT& hr)
	: GSRenderer<NULLVERTEX>(256, 256, hWnd, hr)
{
}

GSRendererNull::~GSRendererNull()
{
}

void GSRendererNull::VertexKick(bool fSkip)
{
	NULLVERTEX v;

	m_vl.AddTail(v);

	__super::VertexKick(fSkip);
}

int GSRendererNull::DrawingKick(bool fSkip)
{
	NULLVERTEX v;

	switch(m_PRIM)
	{
	case 3: // triangle list
		m_vl.RemoveAt(0, v);
		m_vl.RemoveAt(0, v);
		m_vl.RemoveAt(0, v);
		break;
	case 4: // triangle strip
		m_vl.RemoveAt(0, v);
		m_vl.GetAt(0, v);
		m_vl.GetAt(1, v);
		break;
	case 5: // triangle fan
		m_vl.GetAt(0, v);
		m_vl.RemoveAt(1, v);
		m_vl.GetAt(1, v);
		break;
	case 6: // sprite
		m_vl.RemoveAt(0, v);
		m_vl.RemoveAt(0, v);
		break;
	case 1: // line
		m_vl.RemoveAt(0, v);
		m_vl.RemoveAt(0, v);
		break;
	case 2: // line strip
		m_vl.RemoveAt(0, v);
		m_vl.GetAt(0, v);
		break;
	case 0: // point
		m_vl.RemoveAt(0, v);
		break;
	default:
		ASSERT(0);
		m_vl.RemoveAll();
		return 0;
	}

	if(!fSkip)
	{
		m_perfmon.IncCounter(GSPerfMon::c_prim, 1);
	}

	return 0;
}

void GSRendererNull::Flip()
{
	FlipInfo rt[2];
	FinishFlip(rt);
}

void GSRendererNull::EndFrame()
{
}
