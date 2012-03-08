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
#include "GSRendererDX11.h"
#include "GSCrc.h"
#include "resource.h"

GSRendererDX11::GSRendererDX11()
	: GSRendererDX(new GSTextureCache11(this), GSVector2(-0.5f, -0.5f))
{
}

bool GSRendererDX11::CreateDevice(GSDevice* dev)
{
	if(!__super::CreateDevice(dev))
		return false;

	return true;
}

void GSRendererDX11::SetupIA()
{
	GSDevice11* dev = (GSDevice11*)m_dev;

	void* ptr = NULL;

	if(dev->IAMapVertexBuffer(&ptr, sizeof(GSVertex), m_vertex.next))
	{
        GSVector4i::storent(ptr, m_vertex.buff, sizeof(GSVertex) * m_vertex.next);
        
        if(UserHacks_WildHack && !isPackedUV_HackFlag)
        {
            GSVertex* RESTRICT d = (GSVertex*)ptr;
        
            for(unsigned int i = 0; i < m_vertex.next; i++, d++)
                if(PRIM->TME && PRIM->FST)
                    d->UV &= UserHacks_WildHack == 1 ? 0x3FEF3FEF : 0x3FF73FF7;
        }
        
		dev->IAUnmapVertexBuffer();
	}

	dev->IASetIndexBuffer(m_index.buff, m_index.tail);

	D3D11_PRIMITIVE_TOPOLOGY t;

	switch(m_vt.m_primclass)
	{
	case GS_POINT_CLASS:
		t = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case GS_LINE_CLASS:
	case GS_SPRITE_CLASS:
		t = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	case GS_TRIANGLE_CLASS:
		t = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	default:
		__assume(0);
	}
	
	dev->IASetPrimitiveTopology(t);
}
