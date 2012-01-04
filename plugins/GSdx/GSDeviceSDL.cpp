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
#include "GSDeviceSDL.h"

#ifdef ENABLE_SDL_DEV
GSDeviceSDL::GSDeviceSDL()
	: m_free_window(false)
	, m_window(NULL)
	, m_renderer(NULL)
	, m_texture(NULL)
{
}

GSDeviceSDL::~GSDeviceSDL()
{
	if(m_texture != NULL)
	{
		SDL_DestroyTexture(m_texture);
	}

	if(m_renderer != NULL)
	{
		SDL_DestroyRenderer(m_renderer);
	}

	if(m_window != NULL && m_free_window)
	{
		SDL_DestroyWindow(m_window);
	}
}

bool GSDeviceSDL::Create(GSWnd* wnd)
{
	if (m_window == NULL) {
		m_window = SDL_CreateWindowFrom(wnd->GetHandle());
	 	m_free_window = true;
	}
#ifdef __LINUX__
	// In GSopen2, sdl failed to received any resize event. GSWnd::GetClientRect need to manually
	// set the window size... So we send the m_window to the wnd object to allow some manipulation on it.
	wnd->SetWindow(m_window);
#endif

	return GSDeviceSW::Create(wnd);
}

bool GSDeviceSDL::Reset(int w, int h)
{
	if(!GSDeviceSW::Reset(w, h))
	{
		return false;
	}

	delete m_backbuffer;

	m_backbuffer = new GSDummyTexture(w, h);

	if(m_texture != NULL)
	{
		SDL_DestroyTexture(m_texture);

		m_texture = NULL;
	}

	if(m_renderer != NULL)
	{
		SDL_DestroyRenderer(m_renderer);

		m_renderer = NULL;
	}

	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED); // SDL_RENDERER_PRESENTVSYNC

	if(m_renderer == NULL)
	{
		return false;
	}

	SDL_RenderClear(m_renderer);
	SDL_RenderPresent(m_renderer);

	m_format = SDL_PIXELFORMAT_ARGB8888;

	SDL_RendererInfo info;

	memset(&info, 0, sizeof(info));

	SDL_GetRendererInfo(m_renderer, &info);

	for(uint32 i = 0; i < info.num_texture_formats; i++)
	{
		if(info.texture_formats[i] == SDL_PIXELFORMAT_ABGR8888)
		{
			m_format = SDL_PIXELFORMAT_ABGR8888;
		}
	}

	return true;
}

void GSDeviceSDL::Present(GSTexture* st, GSTexture* dt, const GSVector4& dr, int shader)
{
	ASSERT(dt == m_backbuffer); // ignore m_backbuffer

	GSVector2i size = st->GetSize();

	if(m_texture != NULL)
	{
		Uint32 format;
		int access;
		int w, h;

		if(SDL_QueryTexture(m_texture, &format, &access, &w, &h) < 0)
		{
			return;
		}

		if(w != size.x || h != size.y)
		{
			SDL_DestroyTexture(m_texture);

			m_texture = NULL;
		}
	}

	if(m_texture == NULL)
	{
		m_texture = SDL_CreateTexture(m_renderer, m_format, SDL_TEXTUREACCESS_STREAMING, size.x, size.y);
	}

	if(m_texture == NULL)
	{
		return;
	}

	GSTexture::GSMap sm, dm;

	if(SDL_LockTexture(m_texture, NULL, (void**)&dm.bits, &dm.pitch) == 0)
	{
		if(st->Map(sm, NULL))
		{
			GSVector2i s = st->GetSize();

			if(m_format == SDL_PIXELFORMAT_ARGB8888)
			{
				if(((int)dm.bits & 15) == 0 && (dm.pitch & 15) == 0)
				{
					for(int j = s.y; j > 0; j--, sm.bits += sm.pitch, dm.bits += dm.pitch)
					{
						GSVector4i* RESTRICT src = (GSVector4i*)sm.bits;
						GSVector4i* RESTRICT dst = (GSVector4i*)dm.bits;

						for(int i = s.x >> 2; i > 0; i--, dst++, src++)
						{
							*dst = ((*src & 0x00ff0000) >> 16) | ((*src & 0x000000ff) << 16) | (*src & 0x0000ff00);
						}

						uint32* RESTRICT src2 = (uint32*)src;
						uint32* RESTRICT dst2 = (uint32*)dst;

						for(int i = s.x & 3; i > 0; i--, dst2++, src2++)
						{
							*dst2 = ((*src2 & 0x00ff0000) >> 16) | ((*src2 & 0x000000ff) << 16) | (*src2 & 0x0000ff00);
						}
					}
				}
				else
				{
					// VirtualBox/Ubuntu does not return an aligned pointer

					for(int j = s.y; j > 0; j--, sm.bits += sm.pitch, dm.bits += dm.pitch)
					{
						uint32* RESTRICT src = (uint32*)sm.bits;
						uint32* RESTRICT dst = (uint32*)dm.bits;

						for(int i = s.x; i > 0; i--, dst++, src++)
						{
							*dst = ((*src & 0x00ff0000) >> 16) | ((*src & 0x000000ff) << 16) | (*src & 0x0000ff00);
						}
					}
				}
			}
			else
			{
				for(int j = s.y; j > 0; j--, sm.bits += sm.pitch, dm.bits += dm.pitch)
				{
					memcpy(dm.bits, sm.bits, s.x * 4);
				}
			}

			st->Unmap();
		}

		SDL_UnlockTexture(m_texture);
	}

	GSVector4i dri(dr);

	SDL_Rect r;

	r.x = dri.left;
	r.y = dri.top;
	r.w = dri.width();
	r.h = dri.height();

	SDL_RenderClear(m_renderer);
	SDL_RenderCopy(m_renderer, m_texture, NULL, &r);
}

void GSDeviceSDL::Flip()
{
	SDL_RenderPresent(m_renderer);
}
#endif
