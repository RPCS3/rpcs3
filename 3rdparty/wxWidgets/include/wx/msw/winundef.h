/////////////////////////////////////////////////////////////////////////////
// Name:        winundef.h
// Purpose:     undefine the common symbols #define'd by <windows.h>
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.05.99
// RCS-ID:      $Id: winundef.h 36044 2005-10-31 19:35:41Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/* THIS SHOULD NOT BE USED since you might include it once e.g. in window.h,
 * then again _AFTER_ you've included windows.h, in which case it won't work
 * a 2nd time -- JACS
#ifndef _WX_WINUNDEF_H_
#define _WX_WINUNDEF_H_
 */

// ----------------------------------------------------------------------------
// windows.h #defines the following identifiers which are also used in wxWin so
// we replace these symbols with the corresponding inline functions and
// undefine the macro.
//
// This looks quite ugly here but allows us to write clear (and correct!) code
// elsewhere because the functions, unlike the macros, respect the scope.
// ----------------------------------------------------------------------------

// CreateDialog

#if defined(CreateDialog)
    #undef CreateDialog

    inline HWND CreateDialog(HINSTANCE hInstance,
                             LPCTSTR pTemplate,
                             HWND hwndParent,
                             DLGPROC pDlgProc)
    {
        #ifdef _UNICODE
            return CreateDialogW(hInstance, pTemplate, hwndParent, pDlgProc);
        #else
            return CreateDialogA(hInstance, pTemplate, hwndParent, pDlgProc);
        #endif
    }
#endif

// CreateFont

#ifdef CreateFont
    #undef CreateFont

    inline HFONT CreateFont(int height,
                            int width,
                            int escapement,
                            int orientation,
                            int weight,
                            DWORD italic,
                            DWORD underline,
                            DWORD strikeout,
                            DWORD charset,
                            DWORD outprecision,
                            DWORD clipprecision,
                            DWORD quality,
                            DWORD family,
                            LPCTSTR facename)
    {
        #ifdef _UNICODE
            return CreateFontW(height, width, escapement, orientation,
                               weight, italic, underline, strikeout, charset,
                               outprecision, clipprecision, quality,
                               family, facename);
        #else
            return CreateFontA(height, width, escapement, orientation,
                               weight, italic, underline, strikeout, charset,
                               outprecision, clipprecision, quality,
                               family, facename);
        #endif
    }
#endif // CreateFont

// CreateWindow

#if defined(CreateWindow)
    #undef CreateWindow

    inline HWND CreateWindow(LPCTSTR lpClassName,
                             LPCTSTR lpWndClass,
                             DWORD dwStyle,
                             int x, int y, int w, int h,
                             HWND hWndParent,
                             HMENU hMenu,
                             HINSTANCE hInstance,
                             LPVOID lpParam)
    {
        #ifdef _UNICODE
            return CreateWindowW(lpClassName, lpWndClass, dwStyle, x, y, w, h,
                                 hWndParent, hMenu, hInstance, lpParam);
        #else
            return CreateWindowA(lpClassName, lpWndClass, dwStyle, x, y, w, h,
                                 hWndParent, hMenu, hInstance, lpParam);
        #endif
    }
#endif

// LoadMenu

#ifdef LoadMenu
    #undef LoadMenu

    inline HMENU LoadMenu(HINSTANCE instance, LPCTSTR name)
    {
        #ifdef _UNICODE
            return LoadMenuW(instance, name);
        #else
            return LoadMenuA(instance, name);
        #endif
    }
#endif

// FindText

#ifdef FindText
    #undef FindText

    inline HWND APIENTRY FindText(LPFINDREPLACE lpfindreplace)
    {
        #ifdef UNICODE
            return FindTextW(lpfindreplace);
        #else
            return FindTextA(lpfindreplace);
        #endif // !UNICODE
    }
#endif

// GetCharWidth

#ifdef GetCharWidth
   #undef GetCharWidth
   inline BOOL  GetCharWidth(HDC dc, UINT first, UINT last, LPINT buffer)
   {
   #ifdef _UNICODE
      return GetCharWidthW(dc, first, last, buffer);
   #else
      return GetCharWidthA(dc, first, last, buffer);
   #endif
   }
#endif

// FindWindow

#ifdef FindWindow
   #undef FindWindow
   #ifdef _UNICODE
   inline HWND FindWindow(LPCWSTR classname, LPCWSTR windowname)
   {
      return FindWindowW(classname, windowname);
   }
   #else
   inline HWND FindWindow(LPCSTR classname, LPCSTR windowname)
   {
      return FindWindowA(classname, windowname);
   }
   #endif
#endif

// PlaySound

#ifdef PlaySound
   #undef PlaySound
   #ifdef _UNICODE
   inline BOOL PlaySound(LPCWSTR pszSound, HMODULE hMod, DWORD fdwSound)
   {
      return PlaySoundW(pszSound, hMod, fdwSound);
   }
   #else
   inline BOOL PlaySound(LPCSTR pszSound, HMODULE hMod, DWORD fdwSound)
   {
      return PlaySoundA(pszSound, hMod, fdwSound);
   }
   #endif
#endif

// GetClassName

#ifdef GetClassName
   #undef GetClassName
   #ifdef _UNICODE
   inline int GetClassName(HWND h, LPWSTR classname, int maxcount)
   {
      return GetClassNameW(h, classname, maxcount);
   }
   #else
   inline int GetClassName(HWND h, LPSTR classname, int maxcount)
   {
      return GetClassNameA(h, classname, maxcount);
   }
   #endif
#endif

// GetClassInfo

#ifdef GetClassInfo
   #undef GetClassInfo
   #ifdef _UNICODE
   inline BOOL GetClassInfo(HINSTANCE h, LPCWSTR name, LPWNDCLASSW winclass)
   {
      return GetClassInfoW(h, name, winclass);
   }
   #else
   inline BOOL GetClassInfo(HINSTANCE h, LPCSTR name, LPWNDCLASSA winclass)
   {
      return GetClassInfoA(h, name, winclass);
   }
   #endif
#endif

// LoadAccelerators

#ifdef LoadAccelerators
   #undef LoadAccelerators
   #ifdef _UNICODE
   inline HACCEL LoadAccelerators(HINSTANCE h, LPCWSTR name)
   {
      return LoadAcceleratorsW(h, name);
   }
   #else
   inline HACCEL LoadAccelerators(HINSTANCE h, LPCSTR name)
   {
      return LoadAcceleratorsA(h, name);
   }
   #endif
#endif

// DrawText

#ifdef DrawText
   #undef DrawText
   #ifdef _UNICODE
   inline int DrawText(HDC h, LPCWSTR str, int count, LPRECT rect, UINT format)
   {
      return DrawTextW(h, str, count, rect, format);
   }
   #else
   inline int DrawText(HDC h, LPCSTR str, int count, LPRECT rect, UINT format)
   {
      return DrawTextA(h, str, count, rect, format);
   }
   #endif
#endif


/*
  When this file is included, sometimes the wxCHECK_W32API_VERSION macro
  is undefined. With for example CodeWarrior this gives problems with
  the following code:
  #if 0 && wxCHECK_W32API_VERSION( 0, 5 )
  Because CodeWarrior does macro expansion before test evaluation.
  We define wxCHECK_W32API_VERSION here if it's undefined.
*/
#if !defined(__GNUG__) && !defined(wxCHECK_W32API_VERSION)
    #define wxCHECK_W32API_VERSION(maj, min) (0)
#endif

// StartDoc

#ifdef StartDoc
   #undef StartDoc
   #if defined( __GNUG__ ) && !wxCHECK_W32API_VERSION( 0, 5 )
      #define DOCINFOW DOCINFO
      #define DOCINFOA DOCINFO
   #endif
   #ifdef _UNICODE
   inline int StartDoc(HDC h, CONST DOCINFOW* info)
   {
      return StartDocW(h, (DOCINFOW*) info);
   }
   #else
   inline int StartDoc(HDC h, CONST DOCINFOA* info)
   {
      return StartDocA(h, (DOCINFOA*) info);
   }
   #endif
#endif

// GetObject

#ifdef GetObject
   #undef GetObject
   inline int GetObject(HGDIOBJ h, int i, LPVOID buffer)
   {
   #ifdef _UNICODE
      return GetObjectW(h, i, buffer);
   #else
      return GetObjectA(h, i, buffer);
   #endif
   }
#endif

// GetMessage

#ifdef GetMessage
   #undef GetMessage
   inline int GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
   {
   #ifdef _UNICODE
      return GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
   #else
      return GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
   #endif
   }
#endif

// LoadIcon
#ifdef LoadIcon
    #undef LoadIcon
    inline HICON LoadIcon(HINSTANCE hInstance, LPCTSTR lpIconName)
    {
        #ifdef _UNICODE
            return LoadIconW(hInstance, lpIconName);
        #else // ANSI
            return LoadIconA(hInstance, lpIconName);
        #endif // Unicode/ANSI
    }
#endif // LoadIcon

// LoadBitmap
#ifdef LoadBitmap
    #undef LoadBitmap
    inline HBITMAP LoadBitmap(HINSTANCE hInstance, LPCTSTR lpBitmapName)
    {
        #ifdef _UNICODE
            return LoadBitmapW(hInstance, lpBitmapName);
        #else // ANSI
            return LoadBitmapA(hInstance, lpBitmapName);
        #endif // Unicode/ANSI
    }
#endif // LoadBitmap

// LoadLibrary

#ifdef LoadLibrary
    #undef LoadLibrary
    #ifdef _UNICODE
    inline HINSTANCE LoadLibrary(LPCWSTR lpLibFileName)
    {
        return LoadLibraryW(lpLibFileName);
    }
    #else
    inline HINSTANCE LoadLibrary(LPCSTR lpLibFileName)
    {
        return LoadLibraryA(lpLibFileName);
    }
    #endif
#endif

// FindResource
#ifdef FindResource
    #undef FindResource
    #ifdef _UNICODE
    inline HRSRC FindResource(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType)
    {
        return FindResourceW(hModule, lpName, lpType);
    }
    #else
    inline HRSRC FindResource(HMODULE hModule, LPCSTR lpName, LPCSTR lpType)
    {
        return FindResourceA(hModule, lpName, lpType);
    }
    #endif
#endif

// IsMaximized

#ifdef IsMaximized
    #undef IsMaximized
    inline BOOL IsMaximized(HWND WXUNUSED_IN_WINCE(hwnd))
    {
#ifdef __WXWINCE__
        return FALSE;
#else
        return IsZoomed(hwnd);
#endif
    }
#endif

// GetFirstChild

#ifdef GetFirstChild
    #undef GetFirstChild
    inline HWND GetFirstChild(HWND WXUNUSED_IN_WINCE(hwnd))
    {
#ifdef __WXWINCE__
        return 0;
#else
        return GetTopWindow(hwnd);
#endif
    }
#endif

// GetFirstSibling

#ifdef GetFirstSibling
    #undef GetFirstSibling
    inline HWND GetFirstSibling(HWND hwnd)
    {
        return GetWindow(hwnd,GW_HWNDFIRST);
    }
#endif

// GetLastSibling

#ifdef GetLastSibling
    #undef GetLastSibling
    inline HWND GetLastSibling(HWND hwnd)
    {
        return GetWindow(hwnd,GW_HWNDLAST);
    }
#endif

// GetPrevSibling

#ifdef GetPrevSibling
    #undef GetPrevSibling
    inline HWND GetPrevSibling(HWND hwnd)
    {
        return GetWindow(hwnd,GW_HWNDPREV);
    }
#endif

// GetNextSibling

#ifdef GetNextSibling
    #undef GetNextSibling
    inline HWND GetNextSibling(HWND hwnd)
    {
        return GetWindow(hwnd,GW_HWNDNEXT);
    }
#endif

// For WINE

#if defined(GetWindowStyle)
  #undef GetWindowStyle
#endif

// For ming and cygwin

// GetFirstChild
#ifdef GetFirstChild
   #undef GetFirstChild
   inline HWND GetFirstChild(HWND h)
   {
      return GetTopWindow(h);
   }
#endif


// GetNextSibling
#ifdef GetNextSibling
   #undef GetNextSibling
   inline HWND GetNextSibling(HWND h)
   {
     return GetWindow(h, GW_HWNDNEXT);
   }
#endif


#ifdef Yield
    #undef Yield
#endif


#if defined(__WXWINCE__) && defined(DrawIcon) //#ifdef DrawIcon
    #undef DrawIcon
    inline BOOL DrawIcon(HDC hdc, int x, int y, HICON hicon)
    {
        return DrawIconEx(hdc,x,y,hicon,0,0,0,NULL, DI_NORMAL) ;
    }
#endif


// GetWindowProc
//ifdef GetWindowProc
//   #undef GetWindowProc
//endif
//ifdef GetNextChild
//    #undef GetNextChild
//endif

// #endif // _WX_WINUNDEF_H_

