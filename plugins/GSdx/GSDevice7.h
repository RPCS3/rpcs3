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

#include "GSDevice.h"
#include "GSTexture7.h"

class GSDevice7 : public GSDevice
{
private:
	CComPtr<IDirectDraw7> m_dd;
	CComPtr<IDirectDrawSurface7> m_primary;
	bool m_lost;

	GSTexture* Create(int type, int w, int h, bool msaa, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);

public:
	GSDevice7();
	virtual ~GSDevice7();

	bool Create(GSWnd* wnd, bool vsync);
	bool Reset(int w, int h);
	bool IsLost(bool update) {return m_lost;}
	void Present(const GSVector4i& r, int shader, bool limit);
};
