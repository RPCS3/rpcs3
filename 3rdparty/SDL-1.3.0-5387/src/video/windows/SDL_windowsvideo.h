/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_windowsvideo_h
#define _SDL_windowsvideo_h

#include "../SDL_sysvideo.h"

#include "../../core/windows/SDL_windows.h"

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#include <msctf.h>
#else
#include "SDL_msctf.h"
#endif

#include <imm.h>

#define MAX_CANDLIST    10
#define MAX_CANDLENGTH  256

#include "SDL_windowsclipboard.h"
#include "SDL_windowsevents.h"
#include "SDL_windowskeyboard.h"
#include "SDL_windowsmodes.h"
#include "SDL_windowsmouse.h"
#include "SDL_windowsopengl.h"
#include "SDL_windowswindow.h"
#include "SDL_events.h"
#include "SDL_loadso.h"


#if WINVER < 0x0601
/* Touch input definitions */
#define TWF_FINETOUCH	1
#define TWF_WANTPALM	2

#define TOUCHEVENTF_MOVE 0x0001
#define TOUCHEVENTF_DOWN 0x0002
#define TOUCHEVENTF_UP   0x0004

DECLARE_HANDLE(HTOUCHINPUT);

typedef struct _TOUCHINPUT {
	LONG      x;
	LONG      y;
	HANDLE    hSource;
	DWORD     dwID;
	DWORD     dwFlags;
	DWORD     dwMask;
	DWORD     dwTime;
	ULONG_PTR dwExtraInfo;
	DWORD     cxContact;
	DWORD     cyContact;
} TOUCHINPUT, *PTOUCHINPUT;

#endif /* WINVER < 0x0601 */

typedef BOOL  (*PFNSHFullScreen)(HWND, DWORD);
typedef void  (*PFCoordTransform)(SDL_Window*, POINT*);

typedef struct  
{
    void **lpVtbl;
    int refcount;
    void *data;
} TSFSink;

/* Definition from Win98DDK version of IMM.H */
typedef struct tagINPUTCONTEXT2 {
    HWND hWnd;
    BOOL fOpen;
    POINT ptStatusWndPos;
    POINT ptSoftKbdPos;
    DWORD fdwConversion;
    DWORD fdwSentence;
    union {
        LOGFONTA A;
        LOGFONTW W;
    } lfFont;
    COMPOSITIONFORM cfCompForm;
    CANDIDATEFORM cfCandForm[4];
    HIMCC hCompStr;
    HIMCC hCandInfo;
    HIMCC hGuideLine;
    HIMCC hPrivate;
    DWORD dwNumMsgBuf;
    HIMCC hMsgBuf;
    DWORD fdwInit;
    DWORD dwReserve[3];
} INPUTCONTEXT2, *PINPUTCONTEXT2, NEAR *NPINPUTCONTEXT2, FAR *LPINPUTCONTEXT2;

/* Private display data */

typedef struct SDL_VideoData
{
    int render;

#ifdef _WIN32_WCE
    void* hAygShell;
    PFNSHFullScreen SHFullScreen;
    PFCoordTransform CoordTransform;
#endif

    const SDL_Scancode *key_layout;
	DWORD clipboard_count;

	/* Touch input functions */
	void* userDLL;
	BOOL (WINAPI *CloseTouchInputHandle)( HTOUCHINPUT );
	BOOL (WINAPI *GetTouchInputInfo)( HTOUCHINPUT, UINT, PTOUCHINPUT, int );
	BOOL (WINAPI *RegisterTouchWindow)( HWND, ULONG );

    SDL_bool ime_com_initialized;
    struct ITfThreadMgr *ime_threadmgr;
    SDL_bool ime_initialized;
    SDL_bool ime_enabled;
    SDL_bool ime_available;
    HWND ime_hwnd_main;
    HWND ime_hwnd_current;
    HIMC ime_himc;

    WCHAR ime_composition[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
    WCHAR ime_readingstring[16];
    int ime_cursor;

    SDL_bool ime_candlist;
    WCHAR ime_candidates[MAX_CANDLIST][MAX_CANDLENGTH];
    DWORD ime_candcount;
    DWORD ime_candref;
    DWORD ime_candsel;
    UINT ime_candpgsize;
    int ime_candlistindexbase;
    SDL_bool ime_candvertical;

    SDL_bool ime_dirty;
    SDL_Rect ime_rect;
    SDL_Rect ime_candlistrect;
    int ime_winwidth;
    int ime_winheight;

    HKL ime_hkl;
    void* ime_himm32;
    UINT (WINAPI *GetReadingString)(HIMC himc, UINT uReadingBufLen, LPWSTR lpwReadingBuf, PINT pnErrorIndex, BOOL *pfIsVertical, PUINT puMaxReadingLen);
    BOOL (WINAPI *ShowReadingWindow)(HIMC himc, BOOL bShow);
    LPINPUTCONTEXT2 (WINAPI *ImmLockIMC)(HIMC himc);
    BOOL (WINAPI *ImmUnlockIMC)(HIMC himc);
    LPVOID (WINAPI *ImmLockIMCC)(HIMCC himcc);
    BOOL (WINAPI *ImmUnlockIMCC)(HIMCC himcc);

    SDL_bool ime_uiless;
    struct ITfThreadMgrEx *ime_threadmgrex;
    DWORD ime_uielemsinkcookie;
    DWORD ime_alpnsinkcookie;
    DWORD ime_openmodesinkcookie;
    DWORD ime_convmodesinkcookie;
    TSFSink *ime_uielemsink;
    TSFSink *ime_ippasink;
} SDL_VideoData;

#endif /* _SDL_windowsvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
