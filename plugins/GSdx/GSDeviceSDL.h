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

#ifdef ENABLE_SDL_DEV

#include "GSDeviceSW.h"
#include "../../3rdparty/SDL-1.3.0-5387/include/SDL.h"

class GSDeviceSDL : public GSDeviceSW
{
	bool m_free_window;
	SDL_Window* m_window;
	SDL_Renderer* m_renderer;
	SDL_Texture* m_texture;
	int m_format;

	class GSDummyTexture : public GSTexture
	{
	public:
		GSDummyTexture(int w, int h) {m_size.x = w; m_size.y = h; }
		virtual ~GSDummyTexture() {}

		virtual bool Update(const GSVector4i& r, const void* data, int pitch) {return false;}
		virtual bool Map(GSMap& m, const GSVector4i* r = NULL) {return false;}
		virtual void Unmap() {}
		virtual bool Save(const string& fn, bool dds = false) {return false;}
	};

public:
	GSDeviceSDL();
	virtual ~GSDeviceSDL();

	bool Create(GSWnd* wnd);
	bool Reset(int w, int h);
	void Present(GSTexture* st, GSTexture* dt, const GSVector4& dr, int shader = 0);
	void Flip();
};
#endif
