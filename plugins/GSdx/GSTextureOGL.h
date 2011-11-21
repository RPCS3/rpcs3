/*
 *	Copyright (C) 2011-2011 Gregory hainaut
 *	Copyright (C) 2007-2009 Gabest
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

class GSTextureOGL : public GSTexture
{
	private:
		GLenum m_texture_target; // texture target: 2D, rectangle etc...
		GLuint m_texture_id;	 // the texture id
		uint m_texture_unit;		 // the texture unit offset

	public:
		explicit GSTextureOGL(int type, int w, int h, bool msaa, int format);
		virtual ~GSTextureOGL();

		bool Update(const GSVector4i& r, const void* data, int pitch);
		bool Map(GSMap& m, const GSVector4i* r = NULL);
		void Unmap();
		bool Save(const string& fn, bool dds = false);

		void EnableUnit(uint unit);
		void Attach(GLenum attachment);
};
