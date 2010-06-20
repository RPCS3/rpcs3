/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

// need C definitions -- no mangling please!
extern "C" u32   CALLBACK PS2EgetLibType(void);
extern "C" u32   CALLBACK PS2EgetLibVersion2(u32 type);
extern "C" char* CALLBACK PS2EgetLibName(void);

#include "zerogsmath.h"

#include <vector>
#include <string>

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

#include <sys/timeb.h>	// ftime(), struct timeb

inline unsigned long timeGetTime()
{
	timeb t;
	ftime(&t);

	return (unsigned long)(t.time*1000 + t.millitm);
}

struct RECT
{
	int left, top;
	int right, bottom;
};

#endif

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
		u32 texture_targs : 1;
		u32 auto_reset : 1;
		u32 interlace_2x : 1;
		u32 texa : 1; // apply texa to non textured polys
		u32 no_target_resolve : 1;
		u32 exact_color : 1;
		u32 no_color_clamp : 1;
		u32 ffx : 1;
		u32 no_alpha_fail : 1;
		u32 no_depth_update : 1;
		u32 quick_resolve_1 : 1;
		u32 no_quick_resolve : 1;
		u32 no_target_clut : 1; // full 16 bit resolution
		u32 no_stencil : 1;
		u32 vss_hack_off : 1; // vertical stripe syndrome
		u32 no_depth_resolve : 1;
		u32 full_16_bit_res : 1;
		u32 resolve_promoted : 1;
		u32 fast_update : 1;
		u32 no_alpha_test : 1;
		u32 disable_mrt_depth : 1;
		u32 args_32_bit : 1;
		u32 path3 : 1;
		u32 parallel_context : 1; // tries to parallelize both contexts so that render calls are reduced (xenosaga)
									// makes the game faster, but can be buggy
		u32 xenosaga_spec : 1; // xenosaga specularity hack (ignore any zmask=1 draws)
		u32 partial_pointers : 1; // whenver the texture or render target are small, tries to look for bigger ones to read from
		u32 partial_depth : 1; // tries to save depth targets as much as possible across height changes
		u32 reget : 1; // some sort of weirdness in ReGet() code
		u32 gust : 1; // Needed for Gustgames fast update.
		u32 no_logz : 1; // Intended for linux -- not logarithmic Z.
		u32 reserved1 :1;
		u32 reserved2 :1;
	};
	u32 _u32;
} gameHacks;

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
	u8 negaa; // negative aliasing
	u8 bilinear; // set to enable bilinear support. 0 - off, 1 -- on, 2 -- force (use for textures that usually need it)
	ZZOptions zz_options;
	gameHacks hacks; // game options -- different hacks.
	gameHacks def_hacks;// default game settings
	int width, height; // View target size, has no impact towards speed
	int x, y; // Lets try for a persistant window position.
	bool isWideScreen; // Widescreen support
	u32 log;
	
	void incAA() { aa++; if (aa > 4) aa = 4; }
	void decAA() { aa--; if (aa > 4) aa = 0; } // u8 is unsigned, so negative value is 255.
	
	gameHacks settings() 
	{
		gameHacks tempHack;
		tempHack._u32 = (hacks._u32 | def_hacks._u32 | GAME_PATH3HACK);
		 return tempHack; 
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
	
} GSconf;

//Logging for errors that are called often should have a time counter.
#ifdef __LINUX__
static u32 __attribute__((unused)) lasttime = 0;
static u32 __attribute__((unused)) BigTime = 5000;
static bool __attribute__((unused)) SPAM_PASS;
#else
static u32 lasttime = 0;
static u32 BigTime = 5000;
static bool SPAM_PASS;
#endif

#define ERROR_LOG_SPAM(text) { \
	if( timeGetTime() - lasttime > BigTime ) { \
		ZZLog::Error_Log(text); \
		lasttime = timeGetTime(); \
	} \
}
// The same macro with one-argument substitution.
#define ERROR_LOG_SPAMA(fmt, value) { \
	if( timeGetTime() - lasttime > BigTime ) { \
		ZZLog::Error_Log(fmt, value); \
		lasttime = timeGetTime(); \
	} \
}

#define ERROR_LOG_SPAM_TEST(text) {\
	if( timeGetTime() - lasttime > BigTime ) { \
		ZZLog::Error_Log(text); \
		lasttime = timeGetTime(); \
		SPAM_PASS = true; \
	} \
	else \
		SPAM_PASS = false; \
}

#if DEBUG_PROF
#define FILE_IS_IN_CHECK ((strcmp(__FILE__, "targets.cpp") == 0) || (strcmp(__FILE__, "ZZoglFlush.cpp") == 0))

#define FUNCLOG {\
	static bool Was_Here = false; \
	static unsigned long int waslasttime = 0; \
	if (!Was_Here && FILE_IS_IN_CHECK) { \
		Was_Here = true;\
		ZZLog::Error_Log("%s:%d %s", __FILE__, __LINE__, __func__); \
		waslasttime = timeGetTime(); \
	} \
	if (FILE_IS_IN_CHECK && (timeGetTime() - waslasttime > BigTime ))  { \
		Was_Here = false; \
	} \
}
#else
#define FUNCLOG
#endif

//#define WRITE_PRIM_LOGS
#if defined(_DEBUG) && !defined(ZEROGS_DEVBUILD)
#define ZEROGS_DEVBUILD
#endif

#ifdef ZEROGS_DEVBUILD
//#define DEVBUILD
#endif


// sends a message to output window if assert fails
#define BMSG(x, str)			{ if( !(x) ) { ZZLog::Log(str); ZZLog::Log(str); } }
#define BMSG_RETURN(x, str)	{ if( !(x) ) { ZZLog::Log(str); ZZLog::Log(str); return; } }
#define BMSG_RETURNX(x, str, rtype)	{ if( !(x) ) { ZZLog::Log(str); ZZLog::Log(str); return (##rtype); } }
#define B(x)				{ if( !(x) ) { ZZLog::Log(_#x"\n"); ZZLog::Log(#x"\n"); } }
#define B_RETURN(x)			{ if( !(x) ) { ZZLog::Error_Log("%s:%d: %s", __FILE__, (u32)__LINE__, #x); return; } }
#define B_RETURNX(x, rtype)			{ if( !(x) ) { ZZLog::Error_Log("%s:%d: %s", __FILE__, (u32)__LINE__, #x); return (##rtype); } }
#define B_G(x, action)			{ if( !(x) ) { ZZLog::Error_Log("%s:%d: %s", __FILE__, (u32)__LINE__, #x); action; } }

#define GL_REPORT_ERROR() \
{ \
	GLenum err = glGetError(); \
	if( err != GL_NO_ERROR ) \
	{ \
		ZZLog::Error_Log("%s:%d: gl error %s(0x%x)", __FILE__, (int)__LINE__, error_name(err), err); \
		ZeroGS::HandleGLError(); \
	} \
}

#ifdef _DEBUG
#	define GL_REPORT_ERRORD() \
{ \
	GLenum err = glGetError(); \
	if( err != GL_NO_ERROR ) \
	{ \
		ZZLog::Error_Log("%s:%d: gl error %s (0x%x)", __FILE__, (int)__LINE__, error_name(err), err); \
		ZeroGS::HandleGLError(); \
	} \
}
#else
#	define GL_REPORT_ERRORD()
#endif


inline const char *error_name(int err)
{
	switch (err)
	{
		case GL_NO_ERROR:
			return "GL_NO_ERROR";

		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";

		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";

		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";

		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";

		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";

		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";

		case GL_TABLE_TOO_LARGE:
			return "GL_TABLE_TOO_LARGE";

		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
			
		default:
			return "Unknown GL error";
	}
}

extern void __LogToConsole(const char *fmt, ...);

namespace ZZLog
{
extern void Message(const char *fmt, ...);
extern void Log(const char *fmt, ...);
extern void WriteToConsole(const char *fmt, ...);
extern void Print(const char *fmt, ...);

extern void Greg_Log(const char *fmt, ...);
extern void Prim_Log(const char *fmt, ...);
extern void GS_Log(const char *fmt, ...);

extern void Debug_Log(const char *fmt, ...);
extern void Warn_Log(const char *fmt, ...);
extern void Error_Log(const char *fmt, ...);
};

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

extern void LoadConfig();
extern void SaveConfig();

extern void (*GSirq)();

extern void *SysLoadLibrary(char *lib);		// Loads Library
extern void *SysLoadSym(void *lib, char *sym);	// Loads Symbol from Library
extern char *SysLibError();					// Gets previous error loading sysbols
extern void SysCloseLibrary(void *lib);		// Closes Library
extern void SysMessage(const char *fmt, ...);

#ifdef __LINUX__
#include "Utilities/MemcpyFast.h"
#define memcpy_amd memcpy_fast
#else
extern "C" void * memcpy_amd(void *dest, const void *src, size_t n);
extern "C" u8 memcmp_mmx(const void *dest, const void *src, int n);
#endif

#endif // UTIL_H_INCLUDED
