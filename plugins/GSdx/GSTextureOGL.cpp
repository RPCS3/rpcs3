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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <limits.h>
#include "GSTextureOGL.h"
#include "GLState.h"

// FIXME OGL4: investigate, only 1 unpack buffer always bound
namespace PboPool {
	
	GLuint m_pool[PBO_POOL_SIZE];
	uint32 m_offset[PBO_POOL_SIZE];
	char*  m_map[PBO_POOL_SIZE];
	uint32 m_current_pbo = 0;
	uint32 m_size;
	const uint32 m_pbo_size = (640*480*16) << 2;

	void Init() {
		gl_GenBuffers(countof(m_pool), m_pool);

		for (size_t i = 0; i < countof(m_pool); i++) {
			BindPbo();

			if (GLLoader::found_GL_ARB_buffer_storage) {
				gl_BufferStorage(GL_PIXEL_UNPACK_BUFFER, m_pbo_size, NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_CLIENT_STORAGE_BIT);
			} else {
				gl_BufferData(GL_PIXEL_UNPACK_BUFFER, m_pbo_size, NULL, GL_STREAM_COPY);
			}

			m_offset[m_current_pbo] = 0;
			m_map[m_current_pbo] = NULL;

			NextPbo();
		}
		UnbindPbo();
	}

	void MapAll() {
		if (m_map[m_current_pbo] != NULL) return;

		// FIXME I'm not sure it is allowed to map another buffer after we get a pointer
		GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_PERSISTENT_BIT;
		for (size_t i = 0; i < countof(m_pool); i++) {
			BindPbo();
			m_map[m_current_pbo] = (char*)gl_MapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, m_pbo_size, flags);
			NextPbo();
		}
		UnbindPbo();
	}

	char* Map(uint32 size) {
		m_size = size;

		if (m_size >= m_pbo_size) {
			fprintf(stderr, "BUG: PBO too small %d but need %d\n", m_pbo_size, m_size);
		}

		if (!GLLoader::found_GL_ARB_buffer_storage) {
			GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_RANGE_BIT;

			if (m_offset[m_current_pbo] + m_size >= m_pbo_size) {
				NextPbo();

				// Mark current pbo free
				m_offset[m_current_pbo] = 0;

				flags &= ~GL_MAP_INVALIDATE_RANGE_BIT;
				flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
			}

			// Pbo ready let's get a pointer
			BindPbo();

			return (char*)gl_MapBufferRange(GL_PIXEL_UNPACK_BUFFER, m_offset[m_current_pbo], m_size, flags);
		} else {
			MapAll();

			if (m_offset[m_current_pbo] + m_size >= m_pbo_size) {
				NextPbo();

				// Mark current pbo free
				m_offset[m_current_pbo] = 0;
			}

			// Note: it still need it because texsubimage will access currently bound buffer
			// Pbo ready let's get a pointer
			BindPbo();

			return m_map[m_current_pbo] + m_offset[m_current_pbo];
		}
	}

	// FIXME: unmap buffer when the context is dettached (not sure it is required actually)
	void UnmapAll() {
		if (m_map[m_current_pbo] == NULL) return;

		for (size_t i = 0; i < countof(m_pool); i++) {
			BindPbo();
			gl_UnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			m_map[m_current_pbo] = NULL;
			NextPbo();
		}
		UnbindPbo();
	}

	void Unmap() {
		if (GLLoader::found_GL_ARB_buffer_storage) {
			gl_MemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		} else {
			gl_UnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		}
	}

	uint32 Offset() {
		return m_offset[m_current_pbo];
	}

	void Destroy() {
		gl_DeleteBuffers(countof(m_pool), m_pool);
	}

	void BindPbo() {
		gl_BindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pool[m_current_pbo]);
	}

	void NextPbo() {
		m_current_pbo = (m_current_pbo + 1) & (countof(m_pool)-1);
	}

	void UnbindPbo() {
		gl_BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	void EndTransfer() {
		// Note: keep offset aligned for SSE/AVX
		m_offset[m_current_pbo] = (m_offset[m_current_pbo] + m_size + 31) & ~0x1F;
	}
}

// FIXME: check if it possible to always use those setup by default
// glPixelStorei(GL_PACK_ALIGNMENT, 1);
// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

GSTextureOGL::GSTextureOGL(int type, int w, int h, int format, GLuint fbo_read)
	: m_pbo_id(0),
	m_pbo_size(0)
{
	// *************************************************************
	// Opengl world

	// Render work in parallal with framebuffer object (FBO) http://www.opengl.org/wiki/Framebuffer_Objects
	// render are attached to FBO through : gl_FramebufferRenderbuffer. You can query the number of colorattachement with GL_MAX_COLOR_ATTACHMENTS
	// FBO   :  constructor -> gl_GenFramebuffers, gl_DeleteFramebuffers
	//			binding     -> gl_BindFramebuffer (target can be read/write/both)
	//			blit		-> gl_BlitFramebuffer (read FB to draw FB)
	//			info		-> gl_IsFramebuffer, glGetFramebufferAttachmentParameter, gl_CheckFramebufferStatus
	//
	// There are two types of framebuffer-attachable images; texture images and renderbuffer images.
	// If an image of a texture object is attached to a framebuffer, OpenGL performs "render to texture".
	// And if an image of a renderbuffer object is attached to a framebuffer, then OpenGL performs "offscreen rendering".
	// Blitting:
	// gl_DrawBuffers
	// glReadBuffer
	// gl_BlitFramebuffer

	// *************************************************************
	// m_size.x = w;
	// m_size.y = h;
	// FIXME
	m_size.x = max(1,w);
	m_size.y = max(1,h);
	m_format = format;
	m_type   = type;
	m_fbo_read = fbo_read;
	m_texture_id = 0;
	memset(&m_handles, 0, countof(m_handles) * sizeof(m_handles[0]) );

	// Bunch of constant parameter
	switch (m_format) {
		case GL_R32I:
			m_int_format    = GL_RED_INTEGER;
			m_int_type      = GL_INT;
			m_int_alignment = 4;
			m_int_shift     = 2;
			break;
		case GL_R16UI:
			m_int_format    = GL_RED_INTEGER;
			m_int_type      = GL_UNSIGNED_SHORT;
			m_int_alignment = 2;
			m_int_shift     = 1;
			break;
		case GL_RGBA8:
			m_int_format    = GL_RGBA;
			m_int_type      = GL_UNSIGNED_BYTE;
			m_int_alignment = 4;
			m_int_shift     = 2;
			break;
		case GL_R8:
			m_int_format    = GL_RED;
			m_int_type      = GL_UNSIGNED_BYTE;
			m_int_alignment = 1;
			m_int_shift     = 0;
			break;
		case 0:
		case GL_DEPTH32F_STENCIL8:
			// Backbuffer & dss aren't important 
			m_int_format    = 0;
			m_int_type      = 0;
			m_int_alignment = 0;
			m_int_shift     = 0;
			break;
		default:
			ASSERT(0);
	}

	// Generate the buffer
	switch (m_type) {
		case GSTexture::Offscreen:
			//FIXME I not sure we need a pixel buffer object. It seems more a texture
			// gl_GenBuffers(1, &m_texture_id);
			// ASSERT(0);
			// Note there is also a buffer texture!!!
			// http://www.opengl.org/wiki/Buffer_Texture
			// Note: in this case it must use in GLSL
			// gvec texelFetch(gsampler sampler, ivec texCoord, int lod[, int sample]);
			// corollary we can maybe use it for multisample stuff
		case GSTexture::Texture:
		case GSTexture::RenderTarget:
		case GSTexture::DepthStencil:
			glGenTextures(1, &m_texture_id);
			break;
		case GSTexture::Backbuffer:
			break;
		default:
			break;
	}

	// Allocate the buffer
	switch (m_type) {
		case GSTexture::Offscreen:
			// Extra buffer to handle various pixel transfer
			gl_GenBuffers(1, &m_pbo_id);

			// Allocate a pbo with the texture
			m_pbo_size = (m_size.x * m_size.y) << m_int_shift;

			gl_BindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo_id);
			gl_BufferData(GL_PIXEL_PACK_BUFFER, m_pbo_size, NULL, GL_STREAM_READ);
			gl_BindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		case GSTexture::DepthStencil:
		case GSTexture::RenderTarget:
		case GSTexture::Texture:
			EnableUnit();
			gl_TexStorage2D(GL_TEXTURE_2D, 1,  m_format, m_size.x, m_size.y);
			break;
		default: break;
	}
}

GSTextureOGL::~GSTextureOGL()
{
	/* Unbind the texture from our local state */
	
	if (m_texture_id == GLState::rt)
		GLState::rt = 0;
	if (m_texture_id == GLState::ds)
		GLState::ds = 0;
	if (m_texture_id == GLState::tex)
		GLState::tex = 0;
	if (m_texture_id == GLState::tex_unit[0])
		GLState::tex_unit[0] = 0;
	if (m_texture_id == GLState::tex_unit[1])
		GLState::tex_unit[1] = 0;

	gl_DeleteBuffers(1, &m_pbo_id);
	glDeleteTextures(1, &m_texture_id);
}

void GSTextureOGL::Clear(const void *data)
{
	EnableUnit();
	gl_ClearTexImage(m_texture_id, 0,  m_int_format, m_int_type, data);
}

bool GSTextureOGL::Update(const GSVector4i& r, const void* data, int pitch)
{
	ASSERT(m_type != GSTexture::DepthStencil && m_type != GSTexture::Offscreen);

	EnableUnit();

	// Note: FGLRX crashes with the default path. It is happy with PBO. However not sure PBO are big enough for
	// big upscale
	// Note: with latest improvement, Pbo could be faster
#if 1
	glPixelStorei(GL_UNPACK_ALIGNMENT, m_int_alignment);

	uint32 line_size = r.width() << m_int_shift;
	char* src = (char*)data;
	char* map = PboPool::Map(r.height() * line_size);

	for (uint32 h = r.height(); h > 0; h--) {
		// avoid a crash if map is not aligned
		if ((uint32)map & 0x1F)
			memcpy(map, src, line_size);
		else
			GSVector4i::storent(map, src, line_size);
		src += pitch;
		map += line_size;
	}

	PboPool::Unmap();

	glTexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, r.width(), r.height(), m_int_format, m_int_type, (const void*)PboPool::Offset());

	// FIXME OGL4: investigate, only 1 unpack buffer always bound
	//if (!GLLoader::found_GL_ARB_buffer_storage)
	PboPool::UnbindPbo();

	PboPool::EndTransfer();

	return true;

#else

	// pitch is in byte wherease GL_UNPACK_ROW_LENGTH is in pixel
	glPixelStorei(GL_UNPACK_ALIGNMENT, m_int_alignment);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch >> m_int_shift);

	glTexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, r.width(), r.height(), m_int_format, m_int_type, data);

	// FIXME useful?
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Restore default behavior

	return true;
#endif
}

GLuint64 GSTextureOGL::GetHandle(GLuint sampler_id)
{
	ASSERT(sampler_id < 12);
	if (!m_handles[sampler_id]) {
		m_handles[sampler_id] = gl_GetTextureSamplerHandleARB(m_texture_id, sampler_id);
		gl_MakeTextureHandleResidentARB(m_handles[sampler_id]);
	}

	return m_handles[sampler_id];
}

void GSTextureOGL::EnableUnit()
{
	/* Not a real texture */
	ASSERT(!IsBackbuffer());

	if (GLState::tex != m_texture_id) {
		GLState::tex = m_texture_id;
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
	}
}

bool GSTextureOGL::Map(GSMap& m, const GSVector4i* r)
{
	if (m_type != GSTexture::Offscreen) return false;

	// The function allow to modify the texture from the CPU
	// Set m.bits <- pointer to the data
	// Set m.pitch <- size of a row
	// I think in opengl we need to copy back the data to the RAM: glReadPixels — read a block of pixels from the frame buffer
	//
	// gl_MapBuffer — map a buffer object's data store
	// Can be used on GL_PIXEL_UNPACK_BUFFER or GL_TEXTURE_BUFFER

	// Bind the texture to the read framebuffer to avoid any disturbance
	EnableUnit();
	gl_BindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read);
	gl_FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	// FIXME It might be possible to only read a subrange of the texture based on r object
	// Load the PBO with the data
	gl_BindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo_id);
	glPixelStorei(GL_PACK_ALIGNMENT, m_int_alignment);
	glReadPixels(0, 0, m_size.x, m_size.y, m_int_format, m_int_type, 0);
	m.pitch = m_size.x << m_int_shift;
	gl_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	// Give access from the CPU
	m.bits = (uint8*) gl_MapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_pbo_size, GL_MAP_READ_BIT);

	if ( m.bits ) {
		return true;
	} else {
		fprintf(stderr, "bad mapping of the pbo\n");
		gl_BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		return false;
	}

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
		gl_UnmapBuffer(GL_PIXEL_PACK_BUFFER);
		gl_BindBuffer(GL_PIXEL_PACK_BUFFER, 0);

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
		if (false && IsDss()) {
			// Only get the depth and convert it to an integer
			uint8* better_data = data;
			for (int w = m_size.x; w > 0; w--, better_data += 8) {
				float* input = (float*)better_data;
				// FIXME how to dump 32 bits value into 8bits component color
				GLuint depth_integer = (GLuint)(*input * (float)UINT_MAX);
				uint8 r = (depth_integer >>  0) & 0xFF;
				uint8 g = (depth_integer >>  8) & 0xFF;
				uint8 b = (depth_integer >> 16) & 0xFF;
				uint8 a = (depth_integer >> 24) & 0xFF;

				fwrite(&r, 1, 1, fp);
				fwrite(&g, 1, 1, fp);
				fwrite(&b, 1, 1, fp);
				fwrite(&a, 1, 1, fp);
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

void GSTextureOGL::SaveRaw(const string& fn, const void* image, uint32 pitch)
{
	// Build a raw CSV file
	FILE* fp = fopen(fn.c_str(), "w");

	uint32* data = (uint32*)image;

	for(int h = m_size.y; h > 0; h--) {
		for (int w = m_size.x; w > 0; w--, data += 1) {
			if (*data > 0xffffff)
				;
			else {
				fprintf(fp, "%x", *data);
			}
			if ( w > 1)
				fprintf(fp, ",");
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
}


bool GSTextureOGL::Save(const string& fn, bool dds)
{
	// Collect the texture data
	uint32 pitch = 4 * m_size.x;
	char* image = (char*)malloc(pitch * m_size.y);
	bool status = true;

	// FIXME instead of swapping manually B and R maybe you can request the driver to do it
	// for us
	if (IsBackbuffer()) {
		//glReadBuffer(GL_BACK);
		//gl_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glReadPixels(0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image);
	} else if(IsDss()) {
		gl_BindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read);

		gl_FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture_id, 0);
		glReadPixels(0, 0, m_size.x, m_size.y, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, image);

		gl_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	} else if(m_format == GL_R32I) {
		gl_ActiveTexture(GL_TEXTURE0 + 6);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, image);
		SaveRaw(fn, image, pitch);

		// Not supported in Save function
		status = false;

	} else {
		gl_BindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read);

		gl_ActiveTexture(GL_TEXTURE0 + 6);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);

		gl_FramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0);

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		if (m_format == GL_RGBA8)
			glReadPixels(0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else if (m_format == GL_R16UI)
		{
			glReadPixels(0, 0, m_size.x, m_size.y, GL_RED_INTEGER, GL_UNSIGNED_SHORT, image);
			// Not supported in Save function
			status = false;
		}
		else if (m_format == GL_R8)
		{
			glReadPixels(0, 0, m_size.x, m_size.y, GL_RED, GL_UNSIGNED_BYTE, image);
			// Not supported in Save function
			status = false;
		}

		gl_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	if (status) Save(fn, image, pitch);
	free(image);

	// Restore state
	gl_ActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, GLState::tex);

	return status;
}

