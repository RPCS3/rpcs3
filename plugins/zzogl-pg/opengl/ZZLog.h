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
 
#ifndef ZZLOG_H_INCLUDED
#define ZZLOG_H_INCLUDED

#include "Util.h"


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

//#define WRITE_GREG_LOGS
//#define WRITE_PRIM_LOGS
#if defined(_DEBUG) && !defined(ZEROGS_DEVBUILD)
#define ZEROGS_DEVBUILD
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

// Subset of zerogs, to avoid that whole huge header.
namespace ZeroGS
{
extern void AddMessage(const char* pstr, u32 ms);
extern void SetAA(int mode);
extern bool Create(int width, int height);
extern void Destroy(bool bD3D);
extern void StartCapture();
extern void StopCapture();
}

namespace ZZLog
{
extern bool IsLogging();
void SetDir(const char* dir);
extern void Open();
extern void Close();
extern void Message(const char *fmt, ...);
extern void Log(const char *fmt, ...);
void WriteToScreen(const char* pstr, u32 ms = 5000);
extern void WriteToConsole(const char *fmt, ...);
extern void Print(const char *fmt, ...);
extern void WriteLn(const char *fmt, ...);

extern void Greg_Log(const char *fmt, ...);
extern void Prim_Log(const char *fmt, ...);
extern void GS_Log(const char *fmt, ...);

extern void Debug_Log(const char *fmt, ...);
extern void Warn_Log(const char *fmt, ...);
extern void Error_Log(const char *fmt, ...);
};

#endif // ZZLOG_H_INCLUDED
