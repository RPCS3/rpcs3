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
#include "GSTextureOGL.h"

GSTextureOGL::GSTextureOGL(GLuint texture)
	: m_texture(texture)
	, m_type(None)
	, m_width(0)
	, m_height(0)
	, m_format(0)
{
	// TODO: m_type, m_format, fb/ds?

	glBindTexture(GL_TEXTURE_2D, texture);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_height);
}

int GSTextureOGL::GetType() const
{
	return m_type;
}

int GSTextureOGL::GetWidth() const 
{
	return m_width;
}

int GSTextureOGL::GetHeight() const 
{
	return m_height;
}

int GSTextureOGL::GetFormat() const 
{
	return m_format;
}

bool GSTextureOGL::Update(const GSVector4i& r, const void* data, int pitch)
{
	// TODO

	return false;
}

bool GSTextureOGL::Map(GSMap& m, const GSVector4i* r)
{
	// TODO

	return false;
}

void GSTextureOGL::Unmap()
{
	// TODO
}

bool GSTextureOGL::Save(const string& fn, bool dds)
{
	// TODO

	return false;
}
