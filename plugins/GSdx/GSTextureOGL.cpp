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

GSTextureOGL::GSTextureOGL(GLuint texture, int type, int width, int height, int format)
	: m_texture(texture)
	, m_type(type)
	, m_width(width)
	, m_height(height)
	, m_format(format)
{
}

GSTextureOGL::~GSTextureOGL()
{
	if(m_texture)
	{
		switch(m_type)
		{
		case DepthStencil:
			glDeleteRenderbuffersEXT(1, &m_texture);
			break;
		default:
			glDeleteTextures(1, &m_texture);
			break;
		}
	}
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
	// TODO: glTexSubImage2D looks like UpdateSubresource but does not take a pitch

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
