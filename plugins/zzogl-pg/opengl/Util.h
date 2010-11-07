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

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED


#ifdef _WIN32

#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "glprocs.h"

extern HWND GShwnd;

#else // linux basic definitions

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <gtk/gtk.h>

#endif


#define GSdefs
#include "PS2Edefs.h"
#include "CRC.h"
#include "ZZLog.h"

// need C definitions -- no mangling please!
extern "C" u32   CALLBACK PS2EgetLibType(void);
extern "C" u32   CALLBACK PS2EgetLibVersion2(u32 type);
extern "C" char* CALLBACK PS2EgetLibName(void);

#include "ZZoglMath.h"
#include "Profile.h"

#include <vector>
#include <string>
#include <cstring>

extern std::string s_strIniPath; // Air's new (r2361) new constant for ini file path

#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)
#include <malloc.h>

// declare linux equivalents
static __forceinline void* pcsx2_aligned_malloc(size_t size, size_t align)
{
	assert(align < 0x10000);
	char* p = (char*)malloc(size + align);
	int off = 2 + align - ((int)(uptr)(p + 2) % align);

	p += off;
	*(u16*)(p - 2) = off;

	return p;
}

static __forceinline void pcsx2_aligned_free(void* pmem)
{
	if (pmem != NULL)
	{
		char* p = (char*)pmem;
		free(p - (int)*(u16*)(p - 2));
	}
}

#define _aligned_malloc pcsx2_aligned_malloc
#define _aligned_free pcsx2_aligned_free

#endif

#include "Utilities/MemcpyFast.h"
#define memcpy_amd memcpy_fast

#define max(a,b)			(((a) > (b)) ? (a) : (b))
#define min(a,b)			(((a) < (b)) ? (a) : (b))

typedef struct
{
	int x, y, w, h;
} Rect;

typedef struct
{
	int x, y;
} Point;

typedef struct
{
	int x0, y0;
	int x1, y1;
} Rect2;

typedef struct
{
	int x, y, c;
} PointC;

enum GSWindowDim
{
	
	GSDim_640 = 0,
	GSDim_800,
	GSDim_1024,
	GSDim_1280,
};

typedef union
{
	struct
	{
		u32 fullscreen : 1;
		u32 tga_snap : 1;
		u32 capture_avi : 1;
		u32 widescreen : 1;
		u32 wireframe : 1;
		u32 loaded : 1;
		u32 dimensions : 2;
	};
	u32 _u32;
	
	void ZZOptions(u32 value) { _u32 = value; }
} ZZOptions;

typedef struct
{
	u8 mrtdepth; // write color in render target
	u8 interlace; // intelacing mode 0, 1, 3-off
	u8 aa;	// antialiasing 0 - off, 1 - 2x, 2 - 4x, 3 - 8x, 4 - 16x
	u8 bilinear; // set to enable bilinear support. 0 - off, 1 -- on, 2 -- force (use for textures that usually need it)
	ZZOptions zz_options;
	gameHacks hacks; // game options -- different hacks.
	gameHacks def_hacks;// default game settings
	int width, height; // View target size, has no impact towards speed
	int x, y; // Lets try for a persistant window position.
	bool isWideScreen; // Widescreen support
	u32 SkipDraw;
	u32 log;
	u32 disableHacks;
	
	void incAA() { aa++; if (aa > 4) aa = 0; }
	void decAA() { aa--; if (aa > 4) aa = 4; } // u8 is unsigned, so negative value is 255.
	
	gameHacks settings() 
	{
		if (disableHacks)
		{
			return hacks;
		}
		else
		{
			gameHacks tempHack;
			tempHack._u32 = (hacks._u32 | def_hacks._u32);
			return tempHack;
		}
	}
	
	bool fullscreen() { return !!(zz_options.fullscreen); }
	bool wireframe() { return !!(zz_options.wireframe); }
	bool widescreen() { return !!(zz_options.widescreen); }
	bool captureAvi() { return !!(zz_options.capture_avi); }
	bool loaded() {  return !!(zz_options.loaded); }
	
	void setFullscreen(bool flag)
	{
		zz_options.fullscreen = (flag) ? 1 : 0;
	}
	
	void setWireframe(bool flag)
	{
		zz_options.wireframe = (flag) ? 1 : 0;
	}
	
	void setCaptureAvi(bool flag)
	{
		zz_options.capture_avi = (flag) ? 1 : 0;
	}
	
	void setLoaded(bool flag)
	{
		zz_options.loaded = (flag) ? 1 : 0;
	}
	void set_dimensions(u32 dim)
	{
		switch (dim)
		{

			case GSDim_640:
				width = 640;
				height = isWideScreen ? 360 : 480;
				break;

			case GSDim_800:
				width = 800;
				height = isWideScreen ? 450 : 600;
				break;

			case GSDim_1024:
				width = 1024;
				height = isWideScreen ? 576 : 768;
				break;

			case GSDim_1280:
				width = 1280;
				height = isWideScreen ? 720 : 960;
				break;
				
			default:
				width = 800;
				height = 600;
				break;
		}
	}
	
} GSconf;
extern GSconf conf;

// ----------------------- Defines

#define REG64(name) \
union name			\
{					\
	u64 i64;		\
	u32 ai32[2];	\
	struct {		\
 
#define REG128(name)\
union name			\
{					\
	u64 ai64[2];	\
	u32 ai32[4];	\
	struct {		\
 
#define REG64_(prefix, name) REG64(prefix##name)
#define REG128_(prefix, name) REG128(prefix##name)

#define REG_END }; };
#define REG_END2 };

#define REG64_SET(name) \
union name			\
{					\
	u64 i64;		\
	u32 ai32[2];	\
 
#define REG128_SET(name)\
union name			\
{					\
	u64 ai64[2];	\
	u32 ai32[4];	\
 
#define REG_SET_END };

#ifndef SAFE_DELETE
#	define SAFE_DELETE(x)		if( (x) != NULL ) { delete (x); (x) = NULL; }
#endif
#ifndef SAFE_DELETE_ARRAY
#	define SAFE_DELETE_ARRAY(x)	if( (x) != NULL ) { delete[] (x); (x) = NULL; }
#endif
#ifndef SAFE_RELEASE
#	define SAFE_RELEASE(x)		if( (x) != NULL ) { (x)->Release(); (x) = NULL; }
#endif

#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); ++(it))
 
#ifndef ARRAY_SIZE
#	define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

extern void LoadConfig();
extern void SaveConfig();

extern void (*GSirq)();

extern void *SysLoadLibrary(char *lib);		// Loads Library
extern void *SysLoadSym(void *lib, char *sym);	// Loads Symbol from Library
extern char *SysLibError();					// Gets previous error loading sysbols
extern void SysCloseLibrary(void *lib);		// Closes Library
extern void SysMessage(const char *fmt, ...);

#ifdef ZEROGS_DEVBUILD
extern char* EFFECT_NAME;
extern char* EFFECT_DIR;
extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern bool g_bSaveTrans, g_bUpdateEffect, g_bSaveTex, g_bSaveResolved;
#endif

extern bool g_bDisplayFPS; // should we display FPS on screen?

#endif // UTIL_H_INCLUDED
