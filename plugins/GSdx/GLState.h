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

#pragma once

#include "GSdx.h"
#include "GSVector.h"

namespace GLState {
	extern GLuint fbo; // frame buffer object
	extern GLenum draw; // Drawing buffer
	extern GSVector2i viewport;
	extern GSVector4i scissor;

	extern bool blend;
	extern GLenum eq_RGB;
	extern GLenum eq_A;
	extern GLenum f_sRGB;
	extern GLenum f_dRGB;
	extern GLenum f_sA;
	extern GLenum f_dA;
	extern bool r_msk;
	extern bool g_msk;
	extern bool b_msk;
	extern bool a_msk;
	extern float bf;

	extern bool depth;
	extern GLenum depth_func;
	extern bool depth_mask;

	extern bool stencil;
	extern GLenum stencil_func;
	extern GLenum stencil_pass;

	extern GLuint ubo; // uniform buffer object

	extern GLuint ps_ss; // sampler

	extern GLuint rt; // render target
	extern GLuint ds; // Depth-Stencil
	extern GLuint tex_unit[2]; // shader input texture
	extern GLuint tex; // Generic texture (for tex operation)

	extern GLuint ps;
	extern GLuint gs;
	extern GLuint vs;
	extern GLuint program; // monolith program (when sso isn't supported)

	extern void Clear();
}
