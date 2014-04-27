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

#pragma once

#include "GSTexture.h"

class GSTextureSW : public GSTexture
{
	// mem texture, always 32-bit rgba (might add 8-bit for palette if needed)

	int m_pitch;
	void* m_data;
	long m_mapped;

public:
	GSTextureSW(int type, int width, int height);
	virtual ~GSTextureSW();

	bool Update(const GSVector4i& r, const void* data, int pitch);
	bool Map(GSMap& m, const GSVector4i* r);
	void Unmap();
	bool Save(const string& fn, bool dds = false);
};
