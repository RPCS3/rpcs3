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

#include "GSRendererHW.h"

class GSRendererDX : public GSRendererHW
{
	GSVector2 m_pixelcenter;
	bool m_logz;
	bool m_fba;

	bool UserHacks_AlphaHack;

protected:
	virtual void DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex);
	virtual void SetupIA() = 0;
	virtual void UpdateFBA(GSTexture* rt) {}

    int UserHacks_WildHack;

public:
	GSRendererDX(GSTextureCache* tc, const GSVector2& pixelcenter = GSVector2(0, 0));
	virtual ~GSRendererDX();

};
