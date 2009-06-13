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

#include "GSDevice.h"
#include "GSTextureOGL.h"

class GSDeviceOGL : public GSDevice
{
	#ifdef _WINDOWS

	HDC m_hDC;
	HGLRC m_hGLRC;
	
	#endif

	GLuint m_vbo;
	GLuint m_fbo;

	struct
	{
		size_t stride, start, count, limit;
	} m_vertices;

	int m_topology;

	//

	static void OnStaticCgError(CGcontext ctx, CGerror err, void* p) {((GSDeviceOGL*)p)->OnCgError(ctx, err);}
	void OnCgError(CGcontext ctx, CGerror err);

	//

	GSTexture* Create(int type, int w, int h, int format);

	void DoMerge(GSTexture* st[2], GSVector4* sr, GSVector4* dr, GSTexture* dt, bool slbg, bool mmod, const GSVector4& c);
	void DoInterlace(GSTexture* st, GSTexture* dt, int shader, bool linear, float yoffset = 0);

public:
	GSDeviceOGL();
	virtual ~GSDeviceOGL();

	bool Create(GSWnd* wnd, bool vsync);
	bool Reset(int w, int h, bool fs);

	void Flip();

	void BeginScene();
	void DrawPrimitive();
	void EndScene();

	void ClearRenderTarget(GSTexture* t, const GSVector4& c);
	void ClearRenderTarget(GSTexture* t, uint32 c);
	void ClearDepth(GSTexture* t, float c);
	void ClearStencil(GSTexture* t, uint8 c);

	GSTexture* CreateRenderTarget(int w, int h, int format = 0);
	GSTexture* CreateDepthStencil(int w, int h, int format = 0);
	GSTexture* CreateTexture(int w, int h, int format = 0);
	GSTexture* CreateOffscreen(int w, int h, int format = 0);

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sr, int w, int h, int format = 0);

	void StretchRect(GSTexture* st, const GSVector4& sr, GSTexture* dt, const GSVector4& dr, int shader = 0, bool linear = true);

	void IASetVertexBuffer(const void* vertices, size_t stride, size_t count);
	void IASetInputLayout(); // TODO
	void IASetPrimitiveTopology(int topology);
};
