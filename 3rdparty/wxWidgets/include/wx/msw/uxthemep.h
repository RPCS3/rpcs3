/*
 * Win32 5.1 theme definitions
 *
 * Copyright (C) 2003 Kevin Koltzau
 *
 * Originally written for the Wine project, and issued under
 * the wxWindows License by kind permission of the author.
 *
 * License:     wxWindows License
 */

#ifndef __WINE_UXTHEME_H
#define __WINE_UXTHEME_H

#include "wx/msw/wrapcctl.h"

typedef HANDLE HTHEME;

HRESULT WINAPI CloseThemeData(HTHEME hTheme);
HRESULT WINAPI DrawThemeBackground(HTHEME,HDC,int,int,const RECT*,const RECT*);

#define DTBG_CLIPRECT        0x00000001
#define DTBG_DRAWSOLID       0x00000002
#define DTBG_OMITBORDER      0x00000004
#define DTBG_OMITCONTENT     0x00000008
#define DTBG_COMPUTINGREGION 0x00000010
#define DTBG_MIRRORDC        0x00000020

typedef struct _DTBGOPTS {
    DWORD dwSize;
    DWORD dwFlags;
    RECT rcClip;
} DTBGOPTS, *PDTBGOPTS;

HRESULT WINAPI DrawThemeBackgroundEx(HTHEME,HDC,int,int,const RECT*,
                                     const DTBGOPTS*);
HRESULT WINAPI DrawThemeEdge(HTHEME,HDC,int,int,const RECT*,UINT,UINT,
                             RECT*);
HRESULT WINAPI DrawThemeIcon(HTHEME,HDC,int,int,const RECT*,HIMAGELIST,int);
HRESULT WINAPI DrawThemeParentBackground(HWND,HDC,RECT*);

#define DTT_GRAYED      0x1

HRESULT WINAPI DrawThemeText(HTHEME,HDC,int,int,LPCWSTR,int,DWORD,DWORD,
                             const RECT*);

#define ETDT_DISABLE       0x00000001
#define ETDT_ENABLE        0x00000002
#define ETDT_USETABTEXTURE 0x00000004
#define ETDT_ENABLETAB     (ETDT_ENABLE|ETDT_USETABTEXTURE)

HRESULT WINAPI EnableThemeDialogTexture(HWND,DWORD);
HRESULT WINAPI EnableTheming(BOOL);
HRESULT WINAPI GetCurrentThemeName(LPWSTR,int,LPWSTR,int,LPWSTR,int);

#define STAP_ALLOW_NONCLIENT    (1<<0)
#define STAP_ALLOW_CONTROLS     (1<<1)
#define STAP_ALLOW_WEBCONTENT   (1<<2)

DWORD WINAPI GetThemeAppProperties(void);
HRESULT WINAPI GetThemeBackgroundContentRect(HTHEME,HDC,int,int,
                                             const RECT*,RECT*);
HRESULT WINAPI GetThemeBackgroundExtent(HTHEME,HDC,int,int,const RECT*,RECT*);
HRESULT WINAPI GetThemeBackgroundRegion(HTHEME,HDC,int,int,const RECT*,HRGN*);
HRESULT WINAPI GetThemeBool(HTHEME,int,int,int,BOOL*);
HRESULT WINAPI GetThemeColor(HTHEME,int,int,int,COLORREF*);

#if defined(__GNUC__)
# define SZ_THDOCPROP_DISPLAYNAME   (const WCHAR []){ 'D','i','s','p','l','a','y','N','a','m','e',0 }
# define SZ_THDOCPROP_CANONICALNAME (const WCHAR []){ 'T','h','e','m','e','N','a','m','e',0 }
# define SZ_THDOCPROP_TOOLTIP       (const WCHAR []){ 'T','o','o','l','T','i','p',0 }
# define SZ_THDOCPROP_AUTHOR        (const WCHAR []){ 'a','u','t','h','o','r',0 }
#else /* lif defined(_MSC_VER) */
# define SZ_THDOCPROP_DISPLAYNAME   L"DisplayName"
# define SZ_THDOCPROP_CANONICALNAME L"ThemeName"
# define SZ_THDOCPROP_TOOLTIP       L"ToolTip"
# define SZ_THDOCPROP_AUTHOR        L"author"
/*
#else
static const WCHAR SZ_THDOCPROP_DISPLAYNAME[] =   { 'D','i','s','p','l','a','y','N','a','m','e',0 };
static const WCHAR SZ_THDOCPROP_CANONICALNAME[] = { 'T','h','e','m','e','N','a','m','e',0 };
static const WCHAR SZ_THDOCPROP_TOOLTIP[] =       { 'T','o','o','l','T','i','p',0 };
static const WCHAR SZ_THDOCPROP_AUTHOR[] =        { 'a','u','t','h','o','r',0 };
*/
#endif

HRESULT WINAPI GetThemeDocumentationProperty(LPCWSTR,LPCWSTR,LPWSTR,int);
HRESULT WINAPI GetThemeEnumValue(HTHEME,int,int,int,int*);
HRESULT WINAPI GetThemeFilename(HTHEME,int,int,int,LPWSTR,int);
HRESULT WINAPI GetThemeFont(HTHEME,HDC,int,int,int,LOGFONTW*);
HRESULT WINAPI GetThemeInt(HTHEME,int,int,int,int*);

#define MAX_INTLIST_COUNT 10
typedef struct _INTLIST {
    int iValueCount;
    int iValues[MAX_INTLIST_COUNT];
} INTLIST, *PINTLIST;

HRESULT WINAPI GetThemeIntList(HTHEME,int,int,int,INTLIST*);

typedef struct _MARGINS {
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
} MARGINS, *PMARGINS;

HRESULT WINAPI GetThemeMargins(HTHEME,HDC,int,int,int,RECT*,MARGINS*);
HRESULT WINAPI GetThemeMetric(HTHEME,HDC,int,int,int,int*);

typedef enum {
    TS_MIN,
    TS_TRUE,
    TS_DRAW
} THEMESIZE;

HRESULT WINAPI GetThemePartSize(HTHEME,HDC,int,int,RECT*,THEMESIZE,SIZE*);
HRESULT WINAPI GetThemePosition(HTHEME,int,int,int,POINT*);

typedef enum {
    PO_STATE,
    PO_PART,
    PO_CLASS,
    PO_GLOBAL,
    PO_NOTFOUND
} PROPERTYORIGIN;

HRESULT WINAPI GetThemePropertyOrigin(HTHEME,int,int,int,PROPERTYORIGIN*);
HRESULT WINAPI GetThemeRect(HTHEME,int,int,int,RECT*);
HRESULT WINAPI GetThemeString(HTHEME,int,int,int,LPWSTR,int);
BOOL WINAPI GetThemeSysBool(HTHEME,int);
COLORREF WINAPI GetThemeSysColor(HTHEME,int);
HBRUSH WINAPI GetThemeSysColorBrush(HTHEME,int);
HRESULT WINAPI GetThemeSysFont(HTHEME,int,LOGFONTW*);
HRESULT WINAPI GetThemeSysInt(HTHEME,int,int*);
int WINAPI GetThemeSysSize(HTHEME,int);
HRESULT WINAPI GetThemeSysString(HTHEME,int,LPWSTR,int);
HRESULT WINAPI GetThemeTextExtent(HTHEME,HDC,int,int,LPCWSTR,int,DWORD,
                                  const RECT*,RECT*);
HRESULT WINAPI GetThemeTextMetrics(HTHEME,HDC,int,int,TEXTMETRICW*);
HTHEME WINAPI GetWindowTheme(HWND);

#define HTTB_BACKGROUNDSEG          0x0000
#define HTTB_FIXEDBORDER            0x0002
#define HTTB_CAPTION                0x0004
#define HTTB_RESIZINGBORDER_LEFT    0x0010
#define HTTB_RESIZINGBORDER_TOP     0x0020
#define HTTB_RESIZINGBORDER_RIGHT   0x0040
#define HTTB_RESIZINGBORDER_BOTTOM  0x0080
#define HTTB_RESIZINGBORDER \
    (HTTB_RESIZINGBORDER_LEFT|HTTB_RESIZINGBORDER_TOP|\
     HTTB_RESIZINGBORDER_RIGHT|HTTB_RESIZINGBORDER_BOTTOM)
#define HTTB_SIZINGTEMPLATE         0x0100
#define HTTB_SYSTEMSIZINGMARGINS    0x0200

HRESULT WINAPI HitTestThemeBackground(HTHEME,HDC,int,int,DWORD,const RECT*,
                                      HRGN,POINT,WORD*);
BOOL WINAPI IsAppThemed(void);
BOOL WINAPI IsThemeActive(void);
BOOL WINAPI IsThemeBackgroundPartiallyTransparent(HTHEME,int,int);
BOOL WINAPI IsThemeDialogTextureEnabled(void);
BOOL WINAPI IsThemePartDefined(HTHEME,int,int);
HTHEME WINAPI OpenThemeData(HWND,LPCWSTR);
void WINAPI SetThemeAppProperties(DWORD);
HRESULT WINAPI SetWindowTheme(HWND,LPCWSTR,LPCWSTR);


#endif

