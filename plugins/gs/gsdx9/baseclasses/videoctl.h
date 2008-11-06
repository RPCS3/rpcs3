//------------------------------------------------------------------------------
// File: VideoCtl.h
//
// Desc: DirectShow base classes.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __VIDEOCTL__
#define __VIDEOCTL__

// These help with property page implementations. The first can be used to
// load any string from a resource file. The buffer to load into is passed
// as an input parameter. The same buffer is the return value if the string
// was found otherwise it returns TEXT(""). The GetDialogSize is passed the
// resource ID of a dialog box and returns the size of it in screen pixels

#define STR_MAX_LENGTH 256
TCHAR * WINAPI StringFromResource(TCHAR *pBuffer, int iResourceID);

#ifdef UNICODE
#define WideStringFromResource StringFromResource
char* WINAPI StringFromResource(char*pBuffer, int iResourceID);
#else
WCHAR * WINAPI WideStringFromResource(WCHAR *pBuffer, int iResourceID);
#endif


BOOL WINAPI GetDialogSize(int iResourceID,     // Dialog box resource identifier
                          DLGPROC pDlgProc,    // Pointer to dialog procedure
                          LPARAM lParam,       // Any user data wanted in pDlgProc
                          SIZE *pResult);      // Returns the size of dialog box

// Class that aggregates an IDirectDraw interface

class CAggDirectDraw : public IDirectDraw, public CUnknown
{
protected:

    LPDIRECTDRAW m_pDirectDraw;

public:

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // Constructor and destructor

    CAggDirectDraw(TCHAR *pName,LPUNKNOWN pUnk) :
        CUnknown(pName,pUnk),
        m_pDirectDraw(NULL) { };

    virtual CAggDirectDraw::~CAggDirectDraw() { };

    // Set the object we should be aggregating
    void SetDirectDraw(LPDIRECTDRAW pDirectDraw) {
        m_pDirectDraw = pDirectDraw;
    }

    // IDirectDraw methods

    STDMETHODIMP Compact();
    STDMETHODIMP CreateClipper(DWORD dwFlags,LPDIRECTDRAWCLIPPER *lplpDDClipper,IUnknown *pUnkOuter);
    STDMETHODIMP CreatePalette(DWORD dwFlags,LPPALETTEENTRY lpColorTable,LPDIRECTDRAWPALETTE *lplpDDPalette,IUnknown *pUnkOuter);
    STDMETHODIMP CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc,LPDIRECTDRAWSURFACE *lplpDDSurface,IUnknown *pUnkOuter);
    STDMETHODIMP DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface,LPDIRECTDRAWSURFACE *lplpDupDDSurface);
    STDMETHODIMP EnumDisplayModes(DWORD dwSurfaceDescCount,LPDDSURFACEDESC lplpDDSurfaceDescList,LPVOID lpContext,LPDDENUMMODESCALLBACK lpEnumCallback);
    STDMETHODIMP EnumSurfaces(DWORD dwFlags,LPDDSURFACEDESC lpDDSD,LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpEnumCallback);
    STDMETHODIMP FlipToGDISurface();
    STDMETHODIMP GetCaps(LPDDCAPS lpDDDriverCaps,LPDDCAPS lpDDHELCaps);
    STDMETHODIMP GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc);
    STDMETHODIMP GetFourCCCodes(LPDWORD lpNumCodes,LPDWORD lpCodes);
    STDMETHODIMP GetGDISurface(LPDIRECTDRAWSURFACE *lplpGDIDDSurface);
    STDMETHODIMP GetMonitorFrequency(LPDWORD lpdwFrequency);
    STDMETHODIMP GetScanLine(LPDWORD lpdwScanLine);
    STDMETHODIMP GetVerticalBlankStatus(LPBOOL lpblsInVB);
    STDMETHODIMP Initialize(GUID *lpGUID);
    STDMETHODIMP RestoreDisplayMode();
    STDMETHODIMP SetCooperativeLevel(HWND hWnd,DWORD dwFlags);
    STDMETHODIMP SetDisplayMode(DWORD dwWidth,DWORD dwHeight,DWORD dwBpp);
    STDMETHODIMP WaitForVerticalBlank(DWORD dwFlags,HANDLE hEvent);
};


// Class that aggregates an IDirectDrawSurface interface

class CAggDrawSurface : public IDirectDrawSurface, public CUnknown
{
protected:

    LPDIRECTDRAWSURFACE m_pDirectDrawSurface;

public:

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // Constructor and destructor

    CAggDrawSurface(TCHAR *pName,LPUNKNOWN pUnk) :
        CUnknown(pName,pUnk),
        m_pDirectDrawSurface(NULL) { };

    virtual ~CAggDrawSurface() { };

    // Set the object we should be aggregating
    void SetDirectDrawSurface(LPDIRECTDRAWSURFACE pDirectDrawSurface) {
        m_pDirectDrawSurface = pDirectDrawSurface;
    }

    // IDirectDrawSurface methods

    STDMETHODIMP AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
    STDMETHODIMP AddOverlayDirtyRect(LPRECT lpRect);
    STDMETHODIMP Blt(LPRECT lpDestRect,LPDIRECTDRAWSURFACE lpDDSrcSurface,LPRECT lpSrcRect,DWORD dwFlags,LPDDBLTFX lpDDBltFx);
    STDMETHODIMP BltBatch(LPDDBLTBATCH lpDDBltBatch,DWORD dwCount,DWORD dwFlags);
    STDMETHODIMP BltFast(DWORD dwX,DWORD dwY,LPDIRECTDRAWSURFACE lpDDSrcSurface,LPRECT lpSrcRect,DWORD dwTrans);
    STDMETHODIMP DeleteAttachedSurface(DWORD dwFlags,LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
    STDMETHODIMP EnumAttachedSurfaces(LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
    STDMETHODIMP EnumOverlayZOrders(DWORD dwFlags,LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpfnCallback);
    STDMETHODIMP Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride,DWORD dwFlags);
    STDMETHODIMP GetAttachedSurface(LPDDSCAPS lpDDSCaps,LPDIRECTDRAWSURFACE *lplpDDAttachedSurface);
    STDMETHODIMP GetBltStatus(DWORD dwFlags);
    STDMETHODIMP GetCaps(LPDDSCAPS lpDDSCaps);
    STDMETHODIMP GetClipper(LPDIRECTDRAWCLIPPER *lplpDDClipper);
    STDMETHODIMP GetColorKey(DWORD dwFlags,LPDDCOLORKEY lpDDColorKey);
    STDMETHODIMP GetDC(HDC *lphDC);
    STDMETHODIMP GetFlipStatus(DWORD dwFlags);
    STDMETHODIMP GetOverlayPosition(LPLONG lpdwX,LPLONG lpdwY);
    STDMETHODIMP GetPalette(LPDIRECTDRAWPALETTE *lplpDDPalette);
    STDMETHODIMP GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
    STDMETHODIMP GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc);
    STDMETHODIMP Initialize(LPDIRECTDRAW lpDD,LPDDSURFACEDESC lpDDSurfaceDesc);
    STDMETHODIMP IsLost();
    STDMETHODIMP Lock(LPRECT lpDestRect,LPDDSURFACEDESC lpDDSurfaceDesc,DWORD dwFlags,HANDLE hEvent);
    STDMETHODIMP ReleaseDC(HDC hDC);
    STDMETHODIMP Restore();
    STDMETHODIMP SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
    STDMETHODIMP SetColorKey(DWORD dwFlags,LPDDCOLORKEY lpDDColorKey);
    STDMETHODIMP SetOverlayPosition(LONG dwX,LONG dwY);
    STDMETHODIMP SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
    STDMETHODIMP Unlock(LPVOID lpSurfaceData);
    STDMETHODIMP UpdateOverlay(LPRECT lpSrcRect,LPDIRECTDRAWSURFACE lpDDDestSurface,LPRECT lpDestRect,DWORD dwFlags,LPDDOVERLAYFX lpDDOverlayFX);
    STDMETHODIMP UpdateOverlayDisplay(DWORD dwFlags);
    STDMETHODIMP UpdateOverlayZOrder(DWORD dwFlags,LPDIRECTDRAWSURFACE lpDDSReference);
};


// DirectShow must work on multiple platforms.  In particular, it also runs on
// Windows NT 3.51 which does not have DirectDraw capabilities. The filters
// cannot therefore link statically to the DirectDraw library. To make their
// lives that little bit easier we provide this class that manages loading
// and unloading the library and creating the initial IDirectDraw interface

typedef DWORD (WINAPI *PGETFILEVERSIONINFOSIZE)(LPTSTR,LPDWORD);
typedef BOOL (WINAPI *PGETFILEVERSIONINFO)(LPTSTR,DWORD,DWORD,LPVOID);
typedef BOOL (WINAPI *PVERQUERYVALUE)(LPVOID,LPTSTR,LPVOID,PUINT);

class CLoadDirectDraw
{
    LPDIRECTDRAW m_pDirectDraw;     // The DirectDraw driver instance
    HINSTANCE m_hDirectDraw;        // Handle to the loaded library

public:

    CLoadDirectDraw();
    ~CLoadDirectDraw();

    HRESULT LoadDirectDraw(LPSTR szDevice);
    void ReleaseDirectDraw();
    HRESULT IsDirectDrawLoaded();
    LPDIRECTDRAW GetDirectDraw();
    BOOL IsDirectDrawVersion1();
};

#endif // __VIDEOCTL__

