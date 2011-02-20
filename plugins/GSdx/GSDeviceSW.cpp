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

	return true;
}

void GSDeviceSW::Flip()
{
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
	Clear(t, c.rgba32());
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
	// TODO

	return NULL;
}

void GSDeviceSW::CopyRect(GSTexture* st, GSTexture* dt, const GSVector4i& r)
{
	// TODO
}

void GSDeviceSW::StretchRect(GSTexture* st, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	// TODO
}

void GSDeviceSW::StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	// TODO
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
	// TODO
}

void GSDeviceSW::DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset)
{
	// TODO
}

void GSDeviceSW::Clear(GSTexture* t, uint32 c)
{
	int h = t->GetHeight();
	int w = t->GetWidth();

	GSTexture::GSMap m;

	if(t->Map(m, NULL))
	{
		GSVector4i v((int)c);

		uint8* p = m.bits;

		for(int j = 0; j < h; j++, p += m.pitch)
		{
			for(int i = 0; i < w; i += 4)
			{
				*(GSVector4i*)&p[i] = v;
			}
		}

		t->Unmap();
	}
}
