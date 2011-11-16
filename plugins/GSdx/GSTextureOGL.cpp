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

GSTextureOGL::GSTextureOGL(/* ID3D11Texture2D* texture */ )
{
	// *************************************************************
	// Opengl world

	// http://www.opengl.org/wiki/Renderbuffer_Object
	// render:	constructor -> glGenRenderbuffers, glDeleteRenderbuffers
	//			info		-> glGetRenderbufferParameteriv, glIsRenderbuffer
	//		    binding		-> glBindRenderbuffer (only 1 target), glFramebufferRenderbuffer (attach actually)
	//			setup param -> glRenderbufferStorageMultisample (after this call the buffer is unitialized)
	//
	// the only way to use a renderbuffer object is to attach it to a Framebuffer Object. After you bind that
	// FBO to the context, and set up the draw or read buffers appropriately, you can use pixel transfer operations
	// to read and write to it. Of course, you can also render to it. The standard glClear function will also clear the appropriate buffer.
	//
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
	// Doc
	// It might need a texture structure to replace ID3D11Texture2D.
	//
	// == The function need to set (from parameter)
	//		: m_size.x
	//		: m_size.y
	//		: m_type
	//		: m_format
	//		: m_msaa
	// == Might be useful to save
	//		: m_texture_target (like GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE etc...)
	//		: m_texture_id (return by glGen*)
	//
	// == Then generate the texture or buffer.
	// D3D11_BIND_RENDER_TARGET: Bind a texture as a render target for the output-merger stage.
	//		=> glGenRenderbuffers with GL_COLOR_ATTACHMENTi (Hum glGenTextures might work too)
	// D3D11_BIND_DEPTH_STENCIL: Bind a texture as a depth-stencil target for the output-merger stage.
	//		=> glGenRenderbuffers with GL_DEPTH_STENCIL_ATTACHMENT
	// D3D11_BIND_SHADER_RESOURCE: Bind a buffer or texture to a shader stage
	//		=> glGenTextures
	// D3D11_USAGE_STAGING: A resource that supports data transfer (copy) from the GPU to the CPU.
	//		glGenTextures
	//		glReadPixels seems to use a framebuffer. In this case you can setup the source
	//		with glReadBuffer(GL_COLOR_ATTACHMENTi)
	//		glGetTexImage: read pixels of a bound texture
	//		=> To allow map/unmap. I think we can use a pixel buffer (target GL_PIXEL_UNPACK_BUFFER)
	//		http://www.opengl.org/wiki/Pixel_Buffer_Objects
	// == Enable texture Unit
	// EnableUnit();
	// == Allocate space
	// glRenderbufferStorageMultisample or glTexStorage2D


#if 0
	m_texture->GetDevice(&m_dev);
	m_texture->GetDesc(&m_desc);

	m_dev->GetImmediateContext(&m_ctx);

	m_size.x = (int)m_desc.Width;
	m_size.y = (int)m_desc.Height;

	if(m_desc.BindFlags & D3D11_BIND_RENDER_TARGET) m_type = RenderTarget;
	else if(m_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL) m_type = DepthStencil;
	else if(m_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) m_type = Texture;
	else if(m_desc.Usage == D3D11_USAGE_STAGING) m_type = Offscreen;

	m_format = (int)m_desc.Format;

	m_msaa = m_desc.SampleDesc.Count > 1;
#endif
}

GSTextureOGL::~GSTextureOGL()
{
	// glDeleteTextures or glDeleteRenderbuffers
}

void GSTextureOGL::EnableUnit()
{
	// == For texture, the Unit must be selected
	// glActiveTexture
	// !!!!!!!!!! VERY BIG ISSUE, how to ensure that the different texture use different texture unit.
	// I think we need to create a pool on GSdevice.
	// 1/ this->m_device_ogl
	// 2/ int GSDeviceOGL::get_free_texture_unit() called from GSTextureOGL constructor
	// 3/ void GSDeviceOGL::release_texture_unit(int) called from GSDeviceOGL destructor
	// Another (better) idea, will be to create a global static pool
	//
	// == Bind the texture or buffer
	// glBindRenderbuffer or glBindTexture
	//
	// !!!!!!!!!! Maybe attach to the FBO but where to manage the FBO!!!
	// Create a separare public method for attachment ???
}

bool GSTextureOGL::Update(const GSVector4i& r, const void* data, int pitch)
{
	// To update only a part of the texture you can use:
	// glTexSubImage2D — specify a two-dimensional texture subimage
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

bool GSTextureOGL::Map(GSMap& m, const GSVector4i* r)
{
	// The function allow to modify the texture from the CPU
	// Set m.bits <- pointer to the data
	// Set m.pitch <- size of a row
	// I think in opengl we need to copy back the data to the RAM: glReadPixels — read a block of pixels from the frame buffer
	//
	// glMapBuffer — map a buffer object's data store
#if 0
	if(r != NULL)
	{
		// ASSERT(0); // not implemented

		return false;
	}

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
	// copy the texture to the GPU
	// GLboolean glUnmapBuffer(GLenum target);
#if 0
	if(m_texture)
	{
		m_ctx->Unmap(m_texture, 0);
	}
#endif
}

// If I'm correct, this function only dump some buffer. Debug only, still very useful!
// Note: check zzogl implementation
bool GSTextureOGL::Save(const string& fn, bool dds)
{
#if 0
	CComPtr<ID3D11Resource> res;

	if(m_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		HRESULT hr;

		D3D11_TEXTURE2D_DESC desc;

		memset(&desc, 0, sizeof(desc));

		m_texture->GetDesc(&desc);

		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		CComPtr<ID3D11Texture2D> src, dst;

		hr = m_dev->CreateTexture2D(&desc, NULL, &src);

		m_ctx->CopyResource(src, m_texture);

		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = m_dev->CreateTexture2D(&desc, NULL, &dst);

		D3D11_MAPPED_SUBRESOURCE sm, dm;

		hr = m_ctx->Map(src, 0, D3D11_MAP_READ, 0, &sm);
		hr = m_ctx->Map(dst, 0, D3D11_MAP_WRITE, 0, &dm);

		uint8* s = (uint8*)sm.pData;
		uint8* d = (uint8*)dm.pData;

		for(uint32 y = 0; y < desc.Height; y++, s += sm.RowPitch, d += dm.RowPitch)
		{
			for(uint32 x = 0; x < desc.Width; x++)
			{
				((uint32*)d)[x] = (uint32)(ldexpf(((float*)s)[x*2], 32));
			}
		}

		m_ctx->Unmap(src, 0);
		m_ctx->Unmap(dst, 0);

		res = dst;
	}
	else
	{
		res = m_texture;
	}

	return SUCCEEDED(D3DX11SaveTextureToFile(m_ctx, res, dds ? D3DX11_IFF_DDS : D3DX11_IFF_BMP, fn.c_str()));
#endif
}

