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

#include "stdafx.h"
#include "GSdx.h"
#include "GSDevice.h"

GSDevice::GSDevice()
	: m_wnd(NULL)
	, m_vsync(false)
	, m_rbswapped(false)
	, m_backbuffer(NULL)
	, m_merge(NULL)
	, m_weavebob(NULL)
	, m_blend(NULL)
	, m_shaderfx(NULL)
	, m_fxaa(NULL)
	, m_shadeboost(NULL)
	, m_1x1(NULL)
	, m_frame(0)
{
	memset(&m_vertex, 0, sizeof(m_vertex));
	memset(&m_index, 0, sizeof(m_index));
}

GSDevice::~GSDevice()
{
	for_each(m_pool.begin(), m_pool.end(), delete_object());

	delete m_backbuffer;
	delete m_merge;
	delete m_weavebob;
	delete m_blend;
	delete m_shaderfx;
	delete m_fxaa;
	delete m_shadeboost;
	delete m_1x1;
}

bool GSDevice::Create(GSWnd* wnd)
{
	m_wnd = wnd;

	return true;
}

bool GSDevice::Reset(int w, int h)
{
	for_each(m_pool.begin(), m_pool.end(), delete_object());

	m_pool.clear();

	delete m_backbuffer;
	delete m_merge;
	delete m_weavebob;
	delete m_blend;
	delete m_shaderfx;
	delete m_fxaa;
	delete m_shadeboost;
	delete m_1x1;

	m_backbuffer = NULL;
	m_merge = NULL;
	m_weavebob = NULL;
	m_blend = NULL;
	m_shaderfx = NULL;
	m_fxaa = NULL;
	m_shadeboost = NULL;
	m_1x1 = NULL;

	m_current = NULL; // current is special, points to other textures, no need to delete

	return m_wnd != NULL;
}

void GSDevice::Present(const GSVector4i& r, int shader)
{
	GSVector4i cr = m_wnd->GetClientRect();

	int w = std::max<int>(cr.width(), 1);
	int h = std::max<int>(cr.height(), 1);

	if(!m_backbuffer || m_backbuffer->GetWidth() != w || m_backbuffer->GetHeight() != h)
	{
		if(!Reset(w, h))
		{
			return;
		}
	}

	ClearRenderTarget(m_backbuffer, 0);

	if(m_current)
	{
		static int s_shader[5] = {0, 5, 6, 8, 9}; // FIXME

		Present(m_current, m_backbuffer, GSVector4(r), s_shader[shader]);
	}

	Flip();
}

void GSDevice::Present(GSTexture* st, GSTexture* dt, const GSVector4& dr, int shader)
{
	StretchRect(st, dt, dr, shader);
}

GSTexture* GSDevice::FetchSurface(int type, int w, int h, bool msaa, int format)
{
	GSVector2i size(w, h);

	for(list<GSTexture*>::iterator i = m_pool.begin(); i != m_pool.end(); i++)
	{
		GSTexture* t = *i;

		if(t->GetType() == type && t->GetFormat() == format && t->GetSize() == size && t->IsMSAA() == msaa)
		{
			m_pool.erase(i);

			return t;
		}
	}

	return CreateSurface(type, w, h, msaa, format);
}

void GSDevice::EndScene()
{
	m_vertex.start += m_vertex.count;
	m_vertex.count = 0;
	m_index.start += m_index.count;
	m_index.count = 0;
}

void GSDevice::Recycle(GSTexture* t)
{
	if(t)
	{
		t->last_frame_used = m_frame;

		m_pool.push_front(t);

		//printf("%d\n",m_pool.size());

		while(m_pool.size() > 300)
		{
			delete m_pool.back();

			m_pool.pop_back();
		}
	}
}

void GSDevice::AgePool()
{
	m_frame++;

	while(m_pool.size() > 20 && m_frame - m_pool.back()->last_frame_used > 10)
	{
		delete m_pool.back();

		m_pool.pop_back();
	}
}

GSTexture* GSDevice::CreateRenderTarget(int w, int h, bool msaa, int format)
{
	return FetchSurface(GSTexture::RenderTarget, w, h, msaa, format);
}

GSTexture* GSDevice::CreateDepthStencil(int w, int h, bool msaa, int format)
{
	return FetchSurface(GSTexture::DepthStencil, w, h, msaa, format);
}

GSTexture* GSDevice::CreateTexture(int w, int h, int format)
{
	return FetchSurface(GSTexture::Texture, w, h, false, format);
}

GSTexture* GSDevice::CreateOffscreen(int w, int h, int format)
{
	return FetchSurface(GSTexture::Offscreen, w, h, false, format);
}

void GSDevice::StretchRect(GSTexture* st, GSTexture* dt, const GSVector4& dr, int shader, bool linear)
{
	StretchRect(st, GSVector4(0, 0, 1, 1), dt, dr, shader, linear);
}

GSTexture* GSDevice::GetCurrent()
{
	return m_current;
}

void GSDevice::Merge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, const GSVector2i& fs, bool slbg, bool mmod, const GSVector4& c)
{
	if(m_merge == NULL || m_merge->GetSize() != fs)
	{
		Recycle(m_merge);

		m_merge = CreateRenderTarget(fs.x, fs.y, false);
	}

	// TODO: m_1x1

	// KH:COM crashes at startup when booting *through the bios* due to m_merge being NULL.
	// (texture appears to be non-null, and is being re-created at a size around like 1700x340,
	// dunno if that's relevant) -- air

	if(m_merge)
	{
		GSTexture* tex[2] = {NULL, NULL};

		for(size_t i = 0; i < countof(tex); i++)
		{
			if(st[i] != NULL)
			{
				tex[i] = st[i]->IsMSAA() ? Resolve(st[i]) : st[i];
			}
		}

		DoMerge(tex, sr, m_merge, dr, slbg, mmod, c);

		for(size_t i = 0; i < countof(tex); i++)
		{
			if(tex[i] != st[i])
			{
				Recycle(tex[i]);
			}
		}
	}
	else
	{
		printf("GSdx: m_merge is NULL!\n");
	}

	m_current = m_merge;
}

void GSDevice::Interlace(const GSVector2i& ds, int field, int mode, float yoffset)
{
	if(m_weavebob == NULL || m_weavebob->GetSize() != ds)
	{
		delete m_weavebob;

		m_weavebob = CreateRenderTarget(ds.x, ds.y, false);
	}

	if(mode == 0 || mode == 2) // weave or blend
	{
		// weave first

		DoInterlace(m_merge, m_weavebob, field, false, 0);

		if(mode == 2)
		{
			// blend

			if(m_blend == NULL || m_blend->GetSize() != ds)
			{
				delete m_blend;

				m_blend = CreateRenderTarget(ds.x, ds.y, false);
			}

			DoInterlace(m_weavebob, m_blend, 2, false, 0);

			m_current = m_blend;
		}
		else
		{
			m_current = m_weavebob;
		}
	}
	else if(mode == 1) // bob
	{
		DoInterlace(m_merge, m_weavebob, 3, true, yoffset * field);

		m_current = m_weavebob;
	}
	else
	{
		m_current = m_merge;
	}
}

void GSDevice::ExternalFX()
{
	GSVector2i s = m_current->GetSize();

	if (m_shaderfx == NULL || m_shaderfx->GetSize() != s)
	{
		delete m_shaderfx;
		m_shaderfx = CreateRenderTarget(s.x, s.y, false);
	}

	if (m_shaderfx != NULL)
	{
		GSVector4 sr(0, 0, 1, 1);
		GSVector4 dr(0, 0, s.x, s.y);

		StretchRect(m_current, sr, m_shaderfx, dr, 7, false);
		DoExternalFX(m_shaderfx, m_current);
	}
}

void GSDevice::FXAA()
{
	GSVector2i s = m_current->GetSize();

	if(m_fxaa == NULL || m_fxaa->GetSize() != s)
	{
		delete m_fxaa;
		m_fxaa = CreateRenderTarget(s.x, s.y, false);
	}

	if(m_fxaa != NULL)
	{
		GSVector4 sr(0, 0, 1, 1);
		GSVector4 dr(0, 0, s.x, s.y);

		StretchRect(m_current, sr, m_fxaa, dr, 7, false);
		DoFXAA(m_fxaa, m_current);
	}
}

void GSDevice::ShadeBoost()
{
	GSVector2i s = m_current->GetSize();

	if(m_shadeboost == NULL || m_shadeboost->GetSize() != s)
	{
		delete m_shadeboost;
		m_shadeboost = CreateRenderTarget(s.x, s.y, false);
	}

	if(m_shadeboost != NULL)
	{
		GSVector4 sr(0, 0, 1, 1);
		GSVector4 dr(0, 0, s.x, s.y);

		StretchRect(m_current, sr, m_shadeboost, dr, 0, false);
		DoShadeBoost(m_shadeboost, m_current);
	}
}

bool GSDevice::ResizeTexture(GSTexture** t, int w, int h)
{
	if(t == NULL) {ASSERT(0); return false;}

	GSTexture* t2 = *t;

	if(t2 == NULL || t2->GetWidth() != w || t2->GetHeight() != h)
	{
		delete t2;

		t2 = CreateTexture(w, h);

		*t = t2;
	}

	return t2 != NULL;
}

GSAdapter::operator std::string() const
{
	char buf[sizeof "12345678:12345678:12345678:12345678"];
	sprintf(buf, "%.4X:%.4X:%.8X:%.2X", vendor, device, subsys, rev);
	return buf;
}

bool GSAdapter::operator==(const GSAdapter &desc_dxgi) const
{
	return vendor == desc_dxgi.vendor
		&& device == desc_dxgi.device
		&& subsys == desc_dxgi.subsys
		&& rev == desc_dxgi.rev;
}

#ifdef _WINDOWS
GSAdapter::GSAdapter(const DXGI_ADAPTER_DESC1 &desc_dxgi)
	: vendor(desc_dxgi.VendorId)
	, device(desc_dxgi.DeviceId)
	, subsys(desc_dxgi.SubSysId)
	, rev(desc_dxgi.Revision)
{
}

GSAdapter::GSAdapter(const D3DADAPTER_IDENTIFIER9 &desc_d3d9)
	: vendor(desc_d3d9.VendorId)
	, device(desc_d3d9.DeviceId)
	, subsys(desc_d3d9.SubSysId)
	, rev(desc_d3d9.Revision)
{
}
#endif
#ifdef _LINUX
// TODO
#endif
