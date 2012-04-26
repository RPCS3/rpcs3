/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <stdlib.h>
#include <math.h>

#include "GS.h"
#include "Mem.h"
#include "x86.h"
#include "targets.h"
#include "ZZoglShaders.h"
#include "ZZClut.h"
#include "ZZoglVB.h"
#include "Util.h"

extern bool g_bUpdateStencil;

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode);
void SetWriteDepth();
bool IsWriteDepth();
bool IsWriteDestAlphaTest();

const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

CDepthTarget::CDepthTarget() : CRenderTarget(), pdepth(0), pstencil(0), icount(0) {}

CDepthTarget::~CDepthTarget()
{
	FUNCLOG

	Destroy();
}

bool CDepthTarget::Create(const frameInfo& frame)
{
	FUNCLOG

	if (!CRenderTarget::Create(frame)) return false;

	GL_REPORT_ERROR();

	glGenRenderbuffersEXT(1, &pdepth);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pdepth);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, RW(fbw), RH(fbh));

	if (glGetError() != GL_NO_ERROR)
	{
		// try a separate depth and stencil buffer
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pdepth);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, RW(fbw), RH(fbh));

		if (g_bUpdateStencil)
		{
			glGenRenderbuffersEXT(1, &pstencil);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pstencil);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, RW(fbw), RH(fbh));

			if (glGetError() != GL_NO_ERROR)
			{
				ZZLog::Error_Log("Failed to create depth buffer %dx%d.", RW(fbw), RH(fbh));
				return false;
			}
		}
		else
		{
			pstencil = 0;
		}
	}
	else
	{
		pstencil = pdepth;
	}

	status = TS_NeedUpdate;

	return true;
}

void CDepthTarget::Destroy()
{
	FUNCLOG

	if (status)     // In this case Framebuffer extension is off-use and lead to segfault
	{
		ResetRenderTarget(1);
		FB::Attach(GL_DEPTH_ATTACHMENT_EXT);
		FB::Attach(GL_STENCIL_ATTACHMENT_EXT);
		GL_REPORT_ERRORD();

		if (pstencil != 0)
		{
			if (pstencil != pdepth) glDeleteRenderbuffersEXT(1, &pstencil);
			pstencil = 0;
		}

		if (pdepth != 0)
		{
			glDeleteRenderbuffersEXT(1, &pdepth);
			pdepth = 0;
		}

		GL_REPORT_ERRORD();
	}

	CRenderTarget::Destroy();
}


extern int g_nDepthUsed; // > 0 if depth is used

void CDepthTarget::Resolve()
{
	FUNCLOG

	if (g_nDepthUsed > 0 && conf.mrtdepth && !(status & TS_Virtual) && IsWriteDepth() && !(conf.settings().no_depth_resolve))
		CRenderTarget::Resolve();
	else
	{
		// flush if necessary
		FlushIfNecesary(this);

		if (!(status & TS_Virtual)) status |= TS_Resolved;
	}

	if (!(status&TS_Virtual))
	{
		SetWriteDepth();
	}
}

void CDepthTarget::Resolve(int startrange, int endrange)
{
	FUNCLOG

	if (g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && IsWriteDepth())
	{
		CRenderTarget::Resolve(startrange, endrange);
	}
	else
	{
		// flush if necessary
		FlushIfNecesary(this) ;

		if (!(status & TS_Virtual))
			status |= TS_Resolved;
	}

	if (!(status&TS_Virtual))
	{
		SetWriteDepth();
	}
}

void CDepthTarget::Update(int context, CRenderTarget* prndr)
{
	FUNCLOG

	assert(!(status & TS_Virtual));

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	tex0Info texframe;
	texframe.tbp0 = fbp;
	texframe.tbw = fbw;
	texframe.tw = fbw;
	texframe.th = fbh;
	texframe.psm = psm;
    // FIXME some field are not initialized...
    // in particular the clut related one
    assert(!PSMT_ISCLUT(psm));

	DisableAllgl();

	VB& curvb = vb[context];

	if (curvb.test.zte == 0) return;

	SetShaderCaller("CDepthTarget::Update");

	glEnable(GL_DEPTH_TEST);

	glDepthMask(!curvb.zbuf.zmsk);

	static const u32 g_dwZCmp[] = { GL_NEVER, GL_ALWAYS, GL_GEQUAL, GL_GREATER };

	glDepthFunc(g_dwZCmp[curvb.test.ztst]);

	// write color and zero out stencil buf, always 0 context!
	SetTexVariablesInt(0, 0, texframe, false, &ppsBitBltDepth, 1);
	ZZshGLSetTextureParameter(ppsBitBltDepth.prog, ppsBitBltDepth.sMemory, vb[0].pmemtarg->ptex->tex, "BitBltDepth");

	float4 v = DefaultBitBltPos();

	v = DefaultBitBltTex();

	v.x = 1;
	v.y = 2;
	v.z = PSMT_IS16Z(psm) ? 1.0f : 0.0f;
	v.w = g_filog32;
	ZZshSetParameter4fv(ppsBitBltDepth.prog, ppsBitBltDepth.sOneColor, v, "g_fOneColor");

	float4 vdepth = g_vdepth;

	if (psm == PSMT24Z)
	{
		vdepth.w = 0;
	}
	else if (psm != PSMT32Z)
	{
		vdepth.z = vdepth.w = 0;
	}

	assert(ppsBitBltDepth.sBitBltZ != 0);

	ZZshSetParameter4fv(ppsBitBltDepth.prog, ppsBitBltDepth.sBitBltZ, (vdepth*(255.0f / 256.0f)), "g_fBitBltZ");

	assert(pdepth != 0);
	//GLint w1 = 0;
	//GLint h1 = 0;

	FB::Attach2D(0, ptex);
	//glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH_EXT, &w1);
	//glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_HEIGHT_EXT, &h1);
	SetDepthStencilSurface();

	FB::Attach2D(1);

	GLenum buffer = GL_COLOR_ATTACHMENT0_EXT;

	//ZZLog::Error_Log("CDepthTarget::Update: w1 = 0x%x; h1 = 0x%x", w1, h1);
	DrawBuffers(&buffer);

	SetViewport();

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);

	SET_STREAM();
	ZZshSetVertexShader(pvsBitBlt.prog);
	ZZshSetPixelShader(ppsBitBltDepth.prog);

	DrawTriangleArray();

	status = TS_Resolved;

	if (!IsWriteDepth())
	{
		ResetRenderTarget(1);
	}

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_SCISSOR_TEST);

#ifdef _DEBUG
	if (g_bSaveZUpdate)
	{
		SaveTex(&texframe, 1);
		SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh));
	}
#endif
}

void CDepthTarget::SetDepthStencilSurface()
{
	FUNCLOG
	FB::Attach(GL_DEPTH_ATTACHMENT_EXT, pdepth);

	if (pstencil)
	{
		// there's a bug with attaching stencil and depth buffers
		FB::Attach(GL_STENCIL_ATTACHMENT_EXT, pstencil);

		if (icount++ < 8)    // not going to fail if succeeded 4 times
		{
			GL_REPORT_ERRORD();

			if (FB::State() != GL_FRAMEBUFFER_COMPLETE_EXT)
			{
				FB::Attach(GL_STENCIL_ATTACHMENT_EXT);

				if (pstencil != pdepth) glDeleteRenderbuffersEXT(1, &pstencil);

				pstencil = 0;
				g_bUpdateStencil = 0;
			}
		}
	}
	else
	{
		FB::Attach(GL_STENCIL_ATTACHMENT_EXT);
	}
}

