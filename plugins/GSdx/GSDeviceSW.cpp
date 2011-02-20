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
#include "GSDeviceSW.h"

GSDeviceSW::GSDeviceSW()
{
}

bool GSDeviceSW::Create(GSWnd* wnd)
{
	if(!GSDevice::Create(wnd))
		return false;

	Reset(1, 1);

	return true;
}

bool GSDeviceSW::Reset(int w, int h)
{
	if(!GSDevice::Reset(w, h))
		return false;

	m_backbuffer = new GSTextureSW(GSTexture::RenderTarget, w, h);

	return true;
}

void GSDeviceSW::Flip()
{
	// TODO: derived class should present m_backbuffer here
}

GSTexture* GSDeviceSW::Create(int type, int w, int h, bool msaa, int format)
{
	if(format != 0) return false; // there is only one format

	return new GSTextureSW(type, w, h);
}

void GSDeviceSW::BeginScene()
{
	// TODO
}

void GSDeviceSW::DrawPrimitive()
{
	// TODO
}

void GSDeviceSW::EndScene()
{
	// TODO
}

void GSDeviceSW::ClearRenderTarget(GSTexture* t, const GSVector4& c)
{
	Clear(t, (c * 255 + 0.5f).rgba32());
}

void GSDeviceSW::ClearRenderTarget(GSTexture* t, uint32 c)
{
	Clear(t, c);
}

void GSDeviceSW::ClearDepth(GSTexture* t, float c) 
{
	Clear(t, *(uint32*)&c);
}

void GSDeviceSW::ClearStencil(GSTexture* t, uint8 c) 
{
	Clear(t, c);
}

GSTexture* GSDeviceSW::CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format)
{
	GSTexture* dst = CreateOffscreen(w, h, format);

	if(dst != NULL)
	{
		CopyRect(src, dst, GSVector4i(0, 0, w, h));
	}

	return dst;
}

void GSDeviceSW::CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r)
{
	GSTexture::GSMap m;

	if(st->Map(m, &r))
	{
		dt->Update(r, m.bits, m.pitch);

		st->Unmap();
	}
}

void GSDeviceSW::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	// TODO: only used to stretch m_current to m_backbuffer, no blending needed (yet) 
}

void GSDeviceSW::PSSetShaderResources(GSTexture* sr0, GSTexture* sr1)
{
	// TODO
}

void GSDeviceSW::PSSetShaderResource(int i, GSTexture* sr)
{
	// TODO
}

void GSDeviceSW::OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor)
{
	// TODO
}

//

void GSDeviceSW::DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c)
{
	ClearRenderTarget(dt, c);

	if(st[1] && !slbg)
	{
		// TODO: copy (StretchRect)
	}

	if(st[0])
	{
		// TODO: blend
		//
		// mmod 0 => alpha = min(st[0].a * 2, 1)
		// mmod 1 => alpha = c.a 
	}
}

void GSDeviceSW::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
	GSVector4 s = GSVector4(dt->GetSize());

	GSVector4 sr(0, 0, 1, 1);
	GSVector4 dr(0.0f, yoffset, s.x, s.y + yoffset);

	// TODO
	// 
	// shader 0/1 => copy even/odd lines
	// shader 2 => blend lines (1:2:1 filter)
	// shader 3 => copy all lines (StretchRect)
}

void GSDeviceSW::Clear(GSTexture* t, uint32 c)
{
	int w = t->GetWidth();
	int h = t->GetHeight();

	GSTexture::GSMap m;

	if(t->Map(m, NULL))
	{
		GSVector4i v((int)c);

		w >>= 2;

		for(int j = 0; j < h; j++, m.bits += m.pitch)
		{
			GSVector4i* RESTRICT dst = (GSVector4i*)m.bits;

			for(int i = 0; i < w; i += 2)
			{
				dst[i + 0] = v;
				dst[i + 1] = v;
			}
		}

		t->Unmap();
	}
}
