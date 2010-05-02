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

extern HWND GShwnd;

#else // linux basic definitions

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/xf86vmode.h>
#include <gtk/gtk.h>
#include <sys/types.h>

#endif

#include <stdio.h>
#include <malloc.h>
#include <assert.h>


#define GSdefs
#include "PS2Edefs.h"

// need C definitions -- no mangling please!
extern "C" u32   CALLBACK PS2EgetLibType(void);
extern "C" u32   CALLBACK PS2EgetLibVersion2(u32 type);
extern "C" char* CALLBACK PS2EgetLibName(void);

#include "zerogsmath.h"

#include <assert.h>

#include <vector>
#include <string>

extern u32 THR_KeyEvent; // value for passing out key events beetwen threads
extern bool THR_bShift;
extern std::string s_strIniPath; // Air's new (r2361) new constant for ini file path

#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)

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

enum GSOption
{
	GSOPTION_FULLSCREEN = 0x2,
	GSOPTION_TGASNAP = 0x4,
	GSOPTION_CAPTUREAVI = 0x8,

	GSOPTION_WINDIMS = 0x30,
	GSOPTION_WIN640 = 0x00,
	GSOPTION_WIN800 = 0x10,
	GSOPTION_WIN1024 = 0x20,
	GSOPTION_WIN1280 = 0x30,
	GSOPTION_WIDESCREEN = 0x40,

	GSOPTION_WIREFRAME = 0x100,
	GSOPTION_LOADED = 0x8000
};

//Configuration values.

typedef struct
{
	u8 mrtdepth; // write color in render target
	u8 interlace; // intelacing mode 0, 1, 3-off
	u8 aa;	// antialiasing 0 - off, 1 - 2x, 2 - 4x, 3 - 8x, 4 - 16x
	u8 negaa; // negative aliasing
	u8 bilinear; // set to enable bilinear support. 0 - off, 1 -- on, 2 -- force (use for textures that usually need it)
	u32 options; // game options -- different hacks.
	u32 gamesettings;// default game settings
	int width, height; // View target size, has no impact towards speed
	int x, y; // Lets try for a persistant window position.
	bool isWideScreen; // Widescreen support
	u32 log;
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
#ifdef _DEBUG
#define ZEROGS_DEVBUILD
#endif

#ifdef ZEROGS_DEVBUILD
//#define DEVBUILD
#endif

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
