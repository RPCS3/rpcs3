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

#include "GSTextureOGL.h"
static int g_state_texture_unit = -1;
static int g_state_texture_id = -1;

GSTextureOGL::GSTextureOGL(int type, int w, int h, bool msaa, int format, GLuint fbo_read)
	: m_extra_buffer_id(0),
	  m_pbo_size(0)
{
	// *************************************************************
	// Opengl world

	// Render work in parallal with framebuffer object (FBO) http://www.opengl.org/wiki/Framebuffer_Objects
	// render are attached to FBO through : glFramebufferRenderbuffer. You can query the number of colorattachement with GL_MAX_COLOR_ATTACHMENTS
	// FBO   :  constructor -> glGenFramebuffers, glDeleteFramebuffers
	//			binding     -> glBindFramebuffer (target can be read/write/both)
	//			blit		-> glBlitFramebuffer (read FB to draw FB)
	//			info		-> glIsFramebuffer, glGetFramebufferAttachmentParameter, glCheckFramebufferStatus
	//
	// There are two types of framebuffer-attachable images; texture images and renderbuffer images.
	// If an image of a texture object is attached to a framebuffer, OpenGL performs "render to texture".
	// And if an image of a renderbuffer object is attached to a framebuffer, then OpenGL performs "offscreen rendering".
	// Blitting:
	// glDrawBuffers
	// glReadBuffer
	// glBlitFramebuffer

	// *************************************************************
	m_size.x = w;
	m_size.y = h;
	m_format = format;
	m_type   = type;
	m_msaa   = msaa;
	m_fbo_read = fbo_read;

	// Generate the buffer
	switch (m_type) {
		case GSTexture::Offscreen:
			//FIXME I not sure we need a pixel buffer object. It seems more a texture
			// glGenBuffers(1, &m_texture_id);
			// m_texture_target = GL_PIXEL_UNPACK_BUFFER;
			// assert(0);
			// Note there is also a buffer texture!!!
			// http://www.opengl.org/wiki/Buffer_Texture
			// Note: in this case it must use in GLSL
			// gvec texelFetch(gsampler sampler, ivec texCoord, int lod[, int sample]);
			// corollary we can maybe use it for multisample stuff
		case GSTexture::Texture:
		case GSTexture::RenderTarget:
		case GSTexture::DepthStencil:
			glGenTextures(1, &m_texture_id);
			m_texture_target = GL_TEXTURE_2D;
			break;
		case GSTexture::Backbuffer:
			m_texture_target = 0;
			m_texture_id = 0;
		default:
			break;
	}
	// Extra buffer to handle various pixel transfer
	glGenBuffers(1, &m_extra_buffer_id);

	uint msaa_level;
	if (m_msaa) {
		// FIXME  which level of MSAA
		msaa_level = 1;
	} else {
		msaa_level = 0;
	}

	// Allocate the buffer
	switch (m_type) {
		case GSTexture::DepthStencil:
			EnableUnit(2);
			glTexImage2D(m_texture_target, 0, m_format, m_size.x, m_size.y, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);
			break;
		case GSTexture::RenderTarget:
		case GSTexture::Texture:
			// FIXME
			// Howto allocate the texture unit !!!
			// In worst case the HW renderer seems to use 3 texture unit
			// For the moment SW renderer only use 1 so don't bother
			EnableUnit(2);
			if (m_format == GL_RGBA8)
				glTexImage2D(m_texture_target, 0, m_format, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			else if (m_format == GL_R16UI)
				glTexImage2D(m_texture_target, 0, m_format, m_size.x, m_size.y, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
			else if (m_format == GL_R8)
				glTexImage2D(m_texture_target, 0, m_format, m_size.x, m_size.y, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
			else {
				fprintf(stderr, "wrong texture pixel format :%x\n", m_format);
				assert(0); // TODO Later
			}
			break;
		case GSTexture::Offscreen:
			if (m_type == GL_RGBA8) m_pbo_size = m_size.x * m_size.y * 4;
			else if (m_type == GL_R16UI) m_pbo_size = m_size.x * m_size.y * 2;
			else assert(0);

			glBindBuffer(GL_PIXEL_PACK_BUFFER, m_extra_buffer_id);
			glBufferData(GL_PIXEL_PACK_BUFFER, m_pbo_size, NULL, GL_STREAM_DRAW);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			break;
		default: break;
	}
}

GSTextureOGL::~GSTextureOGL()
{
	glDeleteBuffers(1, &m_extra_buffer_id);
	glDeleteTextures(1, &m_texture_id);
}

void GSTextureOGL::Attach(GLenum attachment)
{
	//fprintf(stderr, "format %d,%x\n", m_type, m_format);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, m_texture_target, m_texture_id, 0);
	// FIXME DEBUG
	//fprintf(stderr, "FB status %x\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
}

bool GSTextureOGL::Update(const GSVector4i& r, const void* data, int pitch)
{
	if (m_type == GSTexture::DepthStencil || m_type == GSTexture::Offscreen) assert(0);

	// FIXME warning order of the y axis
	// FIXME I'm not confident with GL_UNSIGNED_BYTE type

	EnableUnit(2);

	// pitch could be different of width*element_size
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch>>2);
	// FIXME : it crash on colin mcrae rally 3 (others game too) when the size is 16
	//fprintf(stderr, "Texture %dx%d with a pitch of %d\n", m_size.x, m_size.y, pitch >>2);
	//fprintf(stderr, "Box (%d,%d)x(%d,%d)\n", r.x, r.y, r.width(), r.height());

	if (m_format == GL_RGBA8)
		glTexSubImage2D(m_texture_target, 0, r.x, r.y, r.width(), r.height(), GL_RGBA, GL_UNSIGNED_BYTE, data);
	else if (m_format == GL_R16UI)
		glTexSubImage2D(m_texture_target, 0, r.x, r.y, r.width(), r.height(), GL_RED_INTEGER, GL_R16UI, data);
	else if (m_format == GL_R8)
		glTexSubImage2D(m_texture_target, 0, r.x, r.y, r.width(), r.height(), GL_RED, GL_R8, data);
	else {
		fprintf(stderr, "wrong texture pixel format :%x\n", m_format);
		assert(0);
	}
#if 0
	//if (m_size.x != 16)
	if (r.width() > 16 && r.height() > 16)
		glTexSubImage2D(m_texture_target, 0, r.x, r.y, r.width(), r.height(), GL_RGBA, GL_UNSIGNED_BYTE, data);
	else {
		fprintf(stderr, "skip texture upload\n");
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Restore default behavior
		return false;
	}
#endif

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Restore default behavior

	return true;
#if 0
	if(m_dev && m_texture)
	{
		D3D11_BOX box = {r.left, r.top, 0, r.right, r.bottom, 1};

		m_ctx->UpdateSubresource(m_texture, 0, &box, data, pitch, 0);

		return true;
	}

	return false;
#endif
}

void GSTextureOGL::EnableUnit(uint unit)
{
	if (!IsBackbuffer()) {
		// FIXME
		// Howto allocate the texture unit !!!
		// In worst case the HW renderer seems to use 3 texture unit
		// For the moment SW renderer only use 1 so don't bother
		if (g_state_texture_unit != unit) {
			g_state_texture_unit = unit;
			glActiveTexture(GL_TEXTURE0 + unit);
			// When you change the texture unit, texture must be rebinded
			g_state_texture_id = m_texture_id;
			glBindTexture(m_texture_target, m_texture_id);
		} else if (g_state_texture_id != m_texture_id) {
			g_state_texture_id = m_texture_id;
			glBindTexture(m_texture_target, m_texture_id);
		}
	}
}

bool GSTextureOGL::Map(GSMap& m, const GSVector4i* r)
{
	// The function allow to modify the texture from the CPU
	// Set m.bits <- pointer to the data
	// Set m.pitch <- size of a row
	// I think in opengl we need to copy back the data to the RAM: glReadPixels — read a block of pixels from the frame buffer
	//
	// glMapBuffer — map a buffer object's data store
	// Can be used on GL_PIXEL_UNPACK_BUFFER or GL_TEXTURE_BUFFER
	if (m_type == GSTexture::Offscreen) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_extra_buffer_id);
		// FIXME It might be possible to only map a subrange of the texture based on r object

		// Load the PBO
		if (m_format == GL_RGBA8)
			glReadPixels(0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		else if (m_format == GL_R16UI)
			glReadPixels(0, 0, m_size.x, m_size.y, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 0);
		else if (m_format == GL_R8)
			glReadPixels(0, 0, m_size.x, m_size.y, GL_RED, GL_UNSIGNED_BYTE, 0);
		else {
			fprintf(stderr, "wrong texture pixel format :%x\n", m_format);
			assert(0);
		}

		// Give access from the CPU
		uint32 map_flags = GL_MAP_READ_BIT;
		m.bits = (uint8*) glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_pbo_size, map_flags);
		m.pitch = m_size.x;

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		return true;
	}

	return false;
#if 0
	if(m_texture && m_desc.Usage == D3D11_USAGE_STAGING)
	{
		D3D11_MAPPED_SUBRESOURCE map;

		if(SUCCEEDED(m_ctx->Map(m_texture, 0, D3D11_MAP_READ_WRITE, 0, &map)))
		{
			m.bits = (uint8*)map.pData;
			m.pitch = (int)map.RowPitch;

			return true;
		}
	}

	return false;
#endif
}

void GSTextureOGL::Unmap()
{
	if (m_type == GSTexture::Offscreen) {
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}
}

#ifndef _WINDOWS

#pragma pack(push, 1)

struct BITMAPFILEHEADER
{
	uint16 bfType;
	uint32 bfSize;
	uint16 bfReserved1;
	uint16 bfReserved2;
	uint32 bfOffBits;
};

struct BITMAPINFOHEADER
{
	uint32 biSize;
	int32 biWidth;
	int32 biHeight;
	uint16 biPlanes;
	uint16 biBitCount;
	uint32 biCompression;
	uint32 biSizeImage;
	int32 biXPelsPerMeter;
	int32 biYPelsPerMeter;
	uint32 biClrUsed;
	uint32 biClrImportant;
};

#define BI_RGB 0

#pragma pack(pop)

#endif
void GSTextureOGL::Save(const string& fn, const void* image, uint32 pitch)
{
	// Build a BMP file
	FILE* fp = fopen(fn.c_str(), "wb");

	BITMAPINFOHEADER bih;

	memset(&bih, 0, sizeof(bih));

	bih.biSize = sizeof(bih);
	bih.biWidth = m_size.x;
	bih.biHeight = m_size.y;
	bih.biPlanes = 1;
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = m_size.x * m_size.y << 2;

	BITMAPFILEHEADER bfh;

	memset(&bfh, 0, sizeof(bfh));

	uint8* bfType = (uint8*)&bfh.bfType;

	// bfh.bfType = 'MB';
	bfType[0] = 0x42;
	bfType[1] = 0x4d;
	bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
	bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;
	bfh.bfReserved1 = bfh.bfReserved2 = 0;

	fwrite(&bfh, 1, sizeof(bfh), fp);
	fwrite(&bih, 1, sizeof(bih), fp);

	uint8* data = (uint8*)image + (m_size.y - 1) * pitch;

	for(int h = m_size.y; h > 0; h--, data -= pitch)
	{
		if (IsDss()) {
			// Only get the depth and convert it to an integer
			uint8* better_data = data;
			for (int w = m_size.x; w > 0; w--, better_data += 8) {
				float* input = (float*)better_data;
				// FIXME how to dump 32 bits value into 8bits component color
				uint32 depth = (uint32)ldexpf(*input, 32);
				uint8 small_depth = depth >> 24;
				uint8 better_data[4] = {small_depth, small_depth, small_depth, 0 };
				fwrite(&better_data, 1, 4, fp);
			}
		} else {
			// swap red and blue
			uint8* better_data = data;
			for (int w = m_size.x; w > 0; w--, better_data += 4) {
				uint8 red = better_data[2];
				better_data[2] = better_data[0];
				better_data[0] = red;
				fwrite(better_data, 1, 4, fp);
			}
		}
	}

	fclose(fp);
}

bool GSTextureOGL::Save(const string& fn, bool dds)
{
	// Collect the texture data
	uint32 pitch = 4 * m_size.x;
	if (IsDss()) pitch *= 2;
	char* image = (char*)malloc(pitch * m_size.y);

	// FIXME instead of swapping manually B and R maybe you can request the driver to do it
	// for us
	if (IsBackbuffer()) {
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image);
	} else if(IsDss()) {
		EnableUnit(2);
		glGetTexImage(m_texture_target, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, image);
	} else {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read);

		EnableUnit(2);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_texture_target, m_texture_id, 0);

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	Save(fn, image, pitch);
	free(image);

	return true;
}

