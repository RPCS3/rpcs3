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
#include "GSDeviceOGL.h"

GSTextureOGL::GSTextureOGL(GLuint texture, int type, int width, int height, int format)
	: m_texture(texture)
{
	m_size.x = width;
	m_size.y = height;

	// TODO: offscreen type should be just a memory array, also returned in Map

	glGenBuffers(1, &m_pbo); GSDeviceOGL::CheckError();

	m_type = type;

	m_format = format;
}

GSTextureOGL::~GSTextureOGL()
{
	if(m_pbo)
	{
		glDeleteBuffers(1, &m_pbo); GSDeviceOGL::CheckError();
	}

	if(m_texture)
	{
		switch(m_type)
		{
		case DepthStencil:
			glDeleteRenderbuffers(1, &m_texture); GSDeviceOGL::CheckError();
			break;
		default:
			glDeleteTextures(1, &m_texture); GSDeviceOGL::CheckError();
			break;
		}
	}
}

bool GSTextureOGL::Update(const GSVector4i& r, const void* data, int pitch)
{
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo); GSDeviceOGL::CheckError();

	int w = r.width();
	int h = r.height();
	int bpp = 32; // TODO: should be in sync with m_format
	int dstpitch = w * bpp >> 3;

	glBufferData(GL_PIXEL_UNPACK_BUFFER, h * dstpitch, NULL, GL_STREAM_DRAW); GSDeviceOGL::CheckError();

	if(uint8* dst = (uint8*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY))
	{
		uint8* src = (uint8*)data;

		for(int i = 0; i < h; i++, src += pitch, dst += dstpitch)
		{
			memcpy(dst, src, dstpitch);
		}

	    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); GSDeviceOGL::CheckError();
	}

	glBindTexture(GL_TEXTURE_2D, m_texture); GSDeviceOGL::CheckError();
	
	glTexSubImage2D(GL_TEXTURE_2D, 0, r.left, r.top, w, h, GL_RGBA, GL_UNSIGNED_BYTE, 0); GSDeviceOGL::CheckError();

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); GSDeviceOGL::CheckError();

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
