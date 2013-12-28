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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

//#define ENABLE_VTUNE

#define ENABLE_JIT_RASTERIZER

#define EXTERNAL_SHADER_LOADING 1

//#define ENABLE_DYNAMIC_CRC_HACK
#define DYNA_DLL_PATH "c:/dev/pcsx2/trunk/tools/dynacrchack/DynaCrcHack.dll"

//#define DISABLE_HW_TEXTURE_CACHE // Slow but fixes a lot of bugs

//#define DISABLE_BITMASKING

//#define DISABLE_COLCLAMP

//#define DISABLE_DATE

#define ENABLE_OGL_MT_HACK // OpenGL doesn't allow to access the same bound context from multiple threads. This hack changes context binding for GSreadFIFO* access
#ifdef _DEBUG
#define ENABLE_OGL_DEBUG   // Create a debug context and check opengl command status. Allow also to dump various textures/states.
#endif

// Output stencil to a color buffer
//#define ENABLE_OGL_STENCIL_DEBUG
