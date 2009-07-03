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

#include "GSTexture.h"

class GSTextureNull : public GSTexture
{
	struct {int type, w, h, format;} m_desc;

public:
	GSTextureNull();
	GSTextureNull(int type, int w, int h, int format);

	int GetType() const {return m_desc.type;}
	int GetWidth() const {return m_desc.w;}
	int GetHeight() const {return m_desc.h;}
	int GetFormat() const {return m_desc.format;}
	bool Update(const GSVector4i& r, const void* data, int pitch) {return true;}
	bool Map(GSMap& m, const GSVector4i* r) {return false;}
	void Unmap() {}
	bool Save(const string& fn, bool dds = false) {return false;}
};
