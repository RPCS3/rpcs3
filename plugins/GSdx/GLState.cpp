/*
 *	Copyright (C) 2011-2013 Gregory hainaut
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
#include "GLState.h"

namespace GLState {
	GLuint fbo = 0;
	GLenum draw = GL_NONE;
	GSVector2i viewport(0, 0);
	GSVector4i scissor(0, 0, 0, 0);

	bool blend = false;
	GLenum eq_RGB = 0;
	GLenum eq_A   = 0;
	GLenum f_sRGB = 0;
	GLenum f_dRGB = 0;
	GLenum f_sA = 0;
	GLenum f_dA = 0;
	bool r_msk = true;
	bool g_msk = true;
	bool b_msk = true;
	bool a_msk = true;
	float bf = 0.0;

	bool depth = false;
	GLenum depth_func = 0;
	bool depth_mask = false;

	bool stencil = false;
	GLenum stencil_func = 0;
	GLenum stencil_pass = 0;

	GLuint ubo = 0;

	GLuint ps_ss = 0;

	GLuint rt = 0;
	GLuint ds = 0;
	GLuint tex_unit[2] = {0, 0};
	GLuint tex = 0;
	GLuint64 tex_handle[2] = { 0, 0};
	bool dirty_ressources = false;

	GLuint ps = 0;
	GLuint gs = 0;
	GLuint vs = 0;
	GLuint program = 0;
	bool dirty_prog = false;
	bool dirty_subroutine_vs = false;
	bool dirty_subroutine_ps = false;
#if 0
	struct {
		GSVertexBufferStateOGL* vb;
		GSDepthStencilOGL* dss;
		GSBlendStateOGL* bs;
		float bf; // blend factor
	} m_state;
#endif

	void Clear() {
		fbo = 0;
		draw = GL_NONE;
		viewport = GSVector2i(0, 0);
		scissor = GSVector4i(0, 0, 0, 0);

		blend = false;
		eq_RGB = 0;
		eq_A   = 0;
		f_sRGB = 0;
		f_dRGB = 0;
		f_sA = 0;
		f_dA = 0;
		r_msk = true;
		g_msk = true;
		b_msk = true;
		a_msk = true;
		bf = 0.0;

		depth = false;
		depth_func = 0;
		depth_mask = false;

		stencil = false;
		stencil_func = 0;
		stencil_pass = 0;

		ubo = 0;

		ps_ss = 0;

		rt = 0;
		ds = 0;
		tex_unit[0] = 0;
		tex_unit[1] = 0;
		tex = 0;
		tex_handle[0] = 0;
		tex_handle[1] = 0;

		ps = 0;
		gs = 0;
		vs = 0;
		program = 0;
		dirty_prog = false;
		dirty_subroutine_vs = false;
		dirty_subroutine_ps = false;
		dirty_ressources = false;
	}
}
