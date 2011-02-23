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

GSDeviceSDL::GSDeviceSDL()
	: m_init(false)
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

	if(m_window != NULL)
	{
		SDL_DestroyWindow(m_window);
	}

	if(m_init)
	{
		SDL_Quit();
	}
}

bool GSDeviceSDL::Create(GSWnd* wnd)
{
	if(!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if(SDL_Init(SDL_INIT_VIDEO) < 0) // ok here?
		{
			return false;
		}

		m_init = true;
	}

	#if 1 //def _WINDOWS

	m_window = SDL_CreateWindowFrom(wnd->GetHandle());

	#else

	// TODO: linux sould use wnd->GetHandle() too

	m_window = SDL_CreateWindow("GSdx", 0, 0, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	#endif

	if(m_window == NULL)
	{
		return false;
	}

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

	return m_renderer != NULL;
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
		m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, size.x, size.y);
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

			for(int j = s.y; j > 0; j--, sm.bits += sm.pitch, dm.bits += dm.pitch)
			{
				memcpy(dm.bits, sm.bits, s.x * 4);
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
