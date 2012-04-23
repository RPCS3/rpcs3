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

//#define ENABLE_VTUNE

#define ENABLE_JIT_RASTERIZER

//#define ENABLE_DYNAMIC_CRC_HACK

#define ENABLE_UPSCALE_HACKS // Hacks intended to fix upscaling / rendering glitches in HW renderers

//#define DISABLE_HW_TEXTURE_CACHE // Slow but fixes a lot of bugs

//#define DISABLE_CRC_HACKS // Disable all game specific hacks

#if defined(DISABLE_HW_TEXTURE_CACHE) && !defined(DISABLE_CRC_HACKS)
	#define DISABLE_CRC_HACKS
#endif

//#define DISABLE_BITMASKING

//#define DISABLE_COLCLAMP

//#define DISABLE_DATE
