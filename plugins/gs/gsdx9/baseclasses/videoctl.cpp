//------------------------------------------------------------------------------
// File: VideoCtl.cpp
//
// Desc: DirectShow base classes.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#include <streams.h>
#include "ddmm.h"

// Load a string from the resource file string table. The buffer must be at
// least STR_MAX_LENGTH bytes. The easiest way to use this is to declare a
// buffer in the property page class and use it for all string loading. It
// cannot be static as multiple property pages may be active simultaneously

TCHAR *WINAPI StringFromResource(TCHAR *pBuffer, int iResourceID)
{
    if (LoadString(g_hInst,iResourceID,pBuffer,STR_MAX_LENGTH) == 0) {
        return TEXT("");
    }
    return pBuffer;
}

#ifdef UNICODE
char *WINAPI StringFromResource(char *pBuffer, int iResourceID)
{
    if (LoadStringA(g_hInst,iResourceID,pBuffer,STR_MAX_LENGTH) == 0) {
        return "";
    }
    return pBuffer;
}
#endif



// Property pages typically are called through their OLE interfaces. These
// use UNICODE strings regardless of how the binary is built. So when we
// load strings from the resource file we sometimes want to convert them
// to UNICODE. This method is passed the target UNICODE buffer and does a
// convert after loading the string (if built UNICODE this is not needed)
// On WinNT we can explicitly call LoadStringW which saves two conversions

#ifndef UNICODE

WCHAR * WINAPI WideStringFromResource(WCHAR *pBuffer, int iResourceID)
{
    *pBuffer = 0;

    if (g_amPlatform == VER_PLATFORM_WIN32_NT) {
	LoadStringW(g_hInst,iResourceID,pBuffer,STR_MAX_LENGTH);
    } else {

	CHAR szBuffer[STR_MAX_LENGTH];
	DWORD dwStringLength = LoadString(g_hInst,iResourceID,szBuffer,STR_MAX_LENGTH);
	// if we loaded a string convert it to wide characters, ensuring
	// that we also null terminate the result.
	if (dwStringLength++) {
	    MultiByteToWideChar(CP_ACP,0,szBuffer,dwStringLength,pBuffer,STR_MAX_LENGTH);
	}
    }
    return pBuffer;
}

#endif


// Helper function to calculate the size of the dialog

BOOL WINAPI GetDialogSize(int iResourceID,
                          DLGPROC pDlgProc,
                          LPARAM lParam,
                          SIZE *pResult)
{
    RECT rc;
    HWND hwnd;

    // Create a temporary property page

    hwnd = CreateDialogParam(g_hInst,
                             MAKEINTRESOURCE(iResourceID),
                             GetDesktopWindow(),
                             pDlgProc,
                             lParam);
    if (hwnd == NULL) {
        return FALSE;
    }

    GetWindowRect(hwnd, &rc);
    pResult->cx = rc.right - rc.left;
    pResult->cy = rc.bottom - rc.top;

    DestroyWindow(hwnd);
    return TRUE;
}


// Class that aggregates on the IDirectDraw interface. Although DirectDraw
// has the ability in its interfaces to be aggregated they're not currently
// implemented. This makes it difficult for various parts of Quartz that want
// to aggregate these interfaces. In particular the video renderer passes out
// media samples that expose IDirectDraw and IDirectDrawSurface. The filter
// graph manager also exposes IDirectDraw as a plug in distributor. For these
// objects we provide these aggregation classes that republish the interfaces

STDMETHODIMP CAggDirectDraw::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    ASSERT(m_pDirectDraw);

    // Do we have this interface

    if (riid == IID_IDirectDraw) {
        return GetInterface((IDirectDraw *)this,ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid,ppv);
    }
}


STDMETHODIMP CAggDirectDraw::Compact()
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->Compact();
}


STDMETHODIMP CAggDirectDraw::CreateClipper(DWORD dwFlags,LPDIRECTDRAWCLIPPER *lplpDDClipper,IUnknown *pUnkOuter)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
}


STDMETHODIMP CAggDirectDraw::CreatePalette(DWORD dwFlags,LPPALETTEENTRY lpColorTable,LPDIRECTDRAWPALETTE *lplpDDPalette,IUnknown *pUnkOuter)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->CreatePalette(dwFlags,lpColorTable,lplpDDPalette,pUnkOuter);
}


STDMETHODIMP CAggDirectDraw::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc,LPDIRECTDRAWSURFACE *lplpDDSurface,IUnknown *pUnkOuter)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->CreateSurface(lpDDSurfaceDesc,lplpDDSurface,pUnkOuter);
}


STDMETHODIMP CAggDirectDraw::DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface,LPDIRECTDRAWSURFACE *lplpDupDDSurface)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->DuplicateSurface(lpDDSurface,lplpDupDDSurface);
}


STDMETHODIMP CAggDirectDraw::EnumDisplayModes(DWORD dwSurfaceDescCount,LPDDSURFACEDESC lplpDDSurfaceDescList,LPVOID lpContext,LPDDENUMMODESCALLBACK lpEnumCallback)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->EnumDisplayModes(dwSurfaceDescCount,lplpDDSurfaceDescList,lpContext,lpEnumCallback);
}


STDMETHODIMP CAggDirectDraw::EnumSurfaces(DWORD dwFlags,LPDDSURFACEDESC lpDDSD,LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpEnumCallback)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->EnumSurfaces(dwFlags,lpDDSD,lpContext,lpEnumCallback);
}


STDMETHODIMP CAggDirectDraw::FlipToGDISurface()
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->FlipToGDISurface();
}


STDMETHODIMP CAggDirectDraw::GetCaps(LPDDCAPS lpDDDriverCaps,LPDDCAPS lpDDHELCaps)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->GetCaps(lpDDDriverCaps,lpDDHELCaps);
}


STDMETHODIMP CAggDirectDraw::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->GetDisplayMode(lpDDSurfaceDesc);
}


STDMETHODIMP CAggDirectDraw::GetFourCCCodes(LPDWORD lpNumCodes,LPDWORD lpCodes)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->GetFourCCCodes(lpNumCodes,lpCodes);
}


STDMETHODIMP CAggDirectDraw::GetGDISurface(LPDIRECTDRAWSURFACE *lplpGDIDDSurface)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->GetGDISurface(lplpGDIDDSurface);
}


STDMETHODIMP CAggDirectDraw::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->GetMonitorFrequency(lpdwFrequency);
}


STDMETHODIMP CAggDirectDraw::GetScanLine(LPDWORD lpdwScanLine)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->GetScanLine(lpdwScanLine);
}


STDMETHODIMP CAggDirectDraw::GetVerticalBlankStatus(LPBOOL lpblsInVB)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->GetVerticalBlankStatus(lpblsInVB);
}


STDMETHODIMP CAggDirectDraw::Initialize(GUID *lpGUID)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->Initialize(lpGUID);
}


STDMETHODIMP CAggDirectDraw::RestoreDisplayMode()
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->RestoreDisplayMode();
}


STDMETHODIMP CAggDirectDraw::SetCooperativeLevel(HWND hWnd,DWORD dwFlags)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->SetCooperativeLevel(hWnd,dwFlags);
}


STDMETHODIMP CAggDirectDraw::SetDisplayMode(DWORD dwWidth,DWORD dwHeight,DWORD dwBpp)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->SetDisplayMode(dwWidth,dwHeight,dwBpp);
}


STDMETHODIMP CAggDirectDraw::WaitForVerticalBlank(DWORD dwFlags,HANDLE hEvent)
{
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw->WaitForVerticalBlank(dwFlags,hEvent);
}


// Class that aggregates an IDirectDrawSurface interface. Although DirectDraw
// has the ability in its interfaces to be aggregated they're not currently
// implemented. This makes it difficult for various parts of Quartz that want
// to aggregate these interfaces. In particular the video renderer passes out
// media samples that expose IDirectDraw and IDirectDrawSurface. The filter
// graph manager also exposes IDirectDraw as a plug in distributor. For these
// objects we provide these aggregation classes that republish the interfaces

STDMETHODIMP CAggDrawSurface::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    ASSERT(m_pDirectDrawSurface);

    // Do we have this interface

    if (riid == IID_IDirectDrawSurface) {
        return GetInterface((IDirectDrawSurface *)this,ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid,ppv);
    }
}


STDMETHODIMP CAggDrawSurface::AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->AddAttachedSurface(lpDDSAttachedSurface);
}


STDMETHODIMP CAggDrawSurface::AddOverlayDirtyRect(LPRECT lpRect)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->AddOverlayDirtyRect(lpRect);
}


STDMETHODIMP CAggDrawSurface::Blt(LPRECT lpDestRect,LPDIRECTDRAWSURFACE lpDDSrcSurface,LPRECT lpSrcRect,DWORD dwFlags,LPDDBLTFX lpDDBltFx)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->Blt(lpDestRect,lpDDSrcSurface,lpSrcRect,dwFlags,lpDDBltFx);
}


STDMETHODIMP CAggDrawSurface::BltBatch(LPDDBLTBATCH lpDDBltBatch,DWORD dwCount,DWORD dwFlags)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}


STDMETHODIMP CAggDrawSurface::BltFast(DWORD dwX,DWORD dwY,LPDIRECTDRAWSURFACE lpDDSrcSurface,LPRECT lpSrcRect,DWORD dwTrans)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->BltFast(dwX,dwY,lpDDSrcSurface,lpSrcRect,dwTrans);
}


STDMETHODIMP CAggDrawSurface::DeleteAttachedSurface(DWORD dwFlags,LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->DeleteAttachedSurface(dwFlags,lpDDSAttachedSurface);
}


STDMETHODIMP CAggDrawSurface::EnumAttachedSurfaces(LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->EnumAttachedSurfaces(lpContext,lpEnumSurfacesCallback);
}


STDMETHODIMP CAggDrawSurface::EnumOverlayZOrders(DWORD dwFlags,LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpfnCallback)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->EnumOverlayZOrders(dwFlags,lpContext,lpfnCallback);
}


STDMETHODIMP CAggDrawSurface::Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride,DWORD dwFlags)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->Flip(lpDDSurfaceTargetOverride,dwFlags);
}


STDMETHODIMP CAggDrawSurface::GetAttachedSurface(LPDDSCAPS lpDDSCaps,LPDIRECTDRAWSURFACE *lplpDDAttachedSurface)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetAttachedSurface(lpDDSCaps,lplpDDAttachedSurface);
}


STDMETHODIMP CAggDrawSurface::GetBltStatus(DWORD dwFlags)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetBltStatus(dwFlags);
}


STDMETHODIMP CAggDrawSurface::GetCaps(LPDDSCAPS lpDDSCaps)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetCaps(lpDDSCaps);
}


STDMETHODIMP CAggDrawSurface::GetClipper(LPDIRECTDRAWCLIPPER *lplpDDClipper)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetClipper(lplpDDClipper);
}


STDMETHODIMP CAggDrawSurface::GetColorKey(DWORD dwFlags,LPDDCOLORKEY lpDDColorKey)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetColorKey(dwFlags,lpDDColorKey);
}


STDMETHODIMP CAggDrawSurface::GetDC(HDC *lphDC)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetDC(lphDC);
}


STDMETHODIMP CAggDrawSurface::GetFlipStatus(DWORD dwFlags)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetFlipStatus(dwFlags);
}


STDMETHODIMP CAggDrawSurface::GetOverlayPosition(LPLONG lpdwX,LPLONG lpdwY)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetOverlayPosition(lpdwX,lpdwY);
}


STDMETHODIMP CAggDrawSurface::GetPalette(LPDIRECTDRAWPALETTE *lplpDDPalette)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetPalette(lplpDDPalette);
}


STDMETHODIMP CAggDrawSurface::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->GetPixelFormat(lpDDPixelFormat);
}


// A bit of a warning here: Our media samples in DirectShow aggregate on
// IDirectDraw and IDirectDrawSurface (ie are available through IMediaSample
// by QueryInterface). Unfortunately the underlying DirectDraw code cannot
// be aggregated so we have to use these classes. The snag is that when we
// call a different surface and pass in this interface as perhaps the source
// surface the call will fail because DirectDraw dereferences the pointer to
// get at its private data structures. Therefore we supply this workaround to give
// access to the real IDirectDraw surface. A filter can call GetSurfaceDesc
// and we will fill in the lpSurface pointer with the real underlying surface

STDMETHODIMP CAggDrawSurface::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    ASSERT(m_pDirectDrawSurface);

    // First call down to the underlying DirectDraw

    HRESULT hr = m_pDirectDrawSurface->GetSurfaceDesc(lpDDSurfaceDesc);
    if (FAILED(hr)) {
        return hr;
    }

    // Store the real DirectDrawSurface interface
    lpDDSurfaceDesc->lpSurface = m_pDirectDrawSurface;
    return hr;
}


STDMETHODIMP CAggDrawSurface::Initialize(LPDIRECTDRAW lpDD,LPDDSURFACEDESC lpDDSurfaceDesc)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->Initialize(lpDD,lpDDSurfaceDesc);
}


STDMETHODIMP CAggDrawSurface::IsLost()
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->IsLost();
}


STDMETHODIMP CAggDrawSurface::Lock(LPRECT lpDestRect,LPDDSURFACEDESC lpDDSurfaceDesc,DWORD dwFlags,HANDLE hEvent)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->Lock(lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent);
}


STDMETHODIMP CAggDrawSurface::ReleaseDC(HDC hDC)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->ReleaseDC(hDC);
}


STDMETHODIMP CAggDrawSurface::Restore()
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->Restore();
}


STDMETHODIMP CAggDrawSurface::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->SetClipper(lpDDClipper);
}


STDMETHODIMP CAggDrawSurface::SetColorKey(DWORD dwFlags,LPDDCOLORKEY lpDDColorKey)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->SetColorKey(dwFlags,lpDDColorKey);
}


STDMETHODIMP CAggDrawSurface::SetOverlayPosition(LONG dwX,LONG dwY)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->SetOverlayPosition(dwX,dwY);
}


STDMETHODIMP CAggDrawSurface::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->SetPalette(lpDDPalette);
}


STDMETHODIMP CAggDrawSurface::Unlock(LPVOID lpSurfaceData)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->Unlock(lpSurfaceData);
}


STDMETHODIMP CAggDrawSurface::UpdateOverlay(LPRECT lpSrcRect,LPDIRECTDRAWSURFACE lpDDDestSurface,LPRECT lpDestRect,DWORD dwFlags,LPDDOVERLAYFX lpDDOverlayFX)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->UpdateOverlay(lpSrcRect,lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFX);
}


STDMETHODIMP CAggDrawSurface::UpdateOverlayDisplay(DWORD dwFlags)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->UpdateOverlayDisplay(dwFlags);
}


STDMETHODIMP CAggDrawSurface::UpdateOverlayZOrder(DWORD dwFlags,LPDIRECTDRAWSURFACE lpDDSReference)
{
    ASSERT(m_pDirectDrawSurface);
    return m_pDirectDrawSurface->UpdateOverlayZOrder(dwFlags,lpDDSReference);
}


// DirectShow must work on multiple platforms.  In particular, it also runs on
// Windows NT 3.51 which does not have DirectDraw capabilities. The filters
// cannot therefore link statically to the DirectDraw library. To make their
// lives that little bit easier we provide this class that manages loading
// and unloading the library and creating the initial IDirectDraw interface

CLoadDirectDraw::CLoadDirectDraw() :
    m_pDirectDraw(NULL),
    m_hDirectDraw(NULL)
{
}


// Destructor forces unload

CLoadDirectDraw::~CLoadDirectDraw()
{
    ReleaseDirectDraw();

    if (m_hDirectDraw) {
        NOTE("Unloading library");
        FreeLibrary(m_hDirectDraw);
    }
}


// We can't be sure that DirectDraw is always available so we can't statically
// link to the library. Therefore we load the library, get the function entry
// point addresses and call them to create the driver objects. We return S_OK
// if we manage to load DirectDraw correctly otherwise we return E_NOINTERFACE
// We initialise a DirectDraw instance by explicitely loading the library and
// calling GetProcAddress on the DirectDrawCreate entry point that it exports

// On a multi monitor system, we can get the DirectDraw object for any
// monitor (device) with the optional szDevice parameter

HRESULT CLoadDirectDraw::LoadDirectDraw(LPSTR szDevice)
{
    PDRAWCREATE pDrawCreate;
    PDRAWENUM pDrawEnum;
    LPDIRECTDRAWENUMERATEEXA pDrawEnumEx;
    HRESULT hr = NOERROR;

    NOTE("Entering DoLoadDirectDraw");

    // Is DirectDraw already loaded

    if (m_pDirectDraw) {
        NOTE("Already loaded");
        ASSERT(m_hDirectDraw);
        return NOERROR;
    }

    // Make sure the library is available

    if(!m_hDirectDraw)
    {
        UINT ErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
        m_hDirectDraw = LoadLibrary(TEXT("DDRAW.DLL"));
        SetErrorMode(ErrorMode);

        if (m_hDirectDraw == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Can't load DDRAW.DLL")));
            NOTE("No library");
            return E_NOINTERFACE;
        }
    }

    // Get the DLL address for the creator function

    pDrawCreate = (PDRAWCREATE)GetProcAddress(m_hDirectDraw,"DirectDrawCreate");
    // force ANSI, we assume it
    pDrawEnum = (PDRAWENUM)GetProcAddress(m_hDirectDraw,"DirectDrawEnumerateA");
    pDrawEnumEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress(m_hDirectDraw,
						"DirectDrawEnumerateExA");

    // We don't NEED DirectDrawEnumerateEx, that's just for multimon stuff
    if (pDrawCreate == NULL || pDrawEnum == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("Can't get functions: Create=%x Enum=%x"),
			pDrawCreate, pDrawEnum));
        NOTE("No entry point");
        ReleaseDirectDraw();
        return E_NOINTERFACE;
    }

    DbgLog((LOG_TRACE,3,TEXT("Creating DDraw for device %s"),
					szDevice ? szDevice : "<NULL>"));

    // Create a DirectDraw display provider for this device, using the fancy
    // multimon-aware version, if it exists
    if (pDrawEnumEx)
        m_pDirectDraw = DirectDrawCreateFromDeviceEx(szDevice, pDrawCreate,
								pDrawEnumEx);
    else
        m_pDirectDraw = DirectDrawCreateFromDevice(szDevice, pDrawCreate,
								pDrawEnum);

    if (m_pDirectDraw == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Can't create DDraw")));
            NOTE("No instance");
            ReleaseDirectDraw();
            return E_NOINTERFACE;
    }
    return NOERROR;
}


// Called to release any DirectDraw provider we previously loaded. We may be
// called at any time especially when something goes horribly wrong and when
// we need to clean up before returning so we can't guarantee that all state
// variables are consistent so free only those really allocated allocated
// This should only be called once all reference counts have been released

void CLoadDirectDraw::ReleaseDirectDraw()
{
    NOTE("Releasing DirectDraw driver");

    // Release any DirectDraw provider interface

    if (m_pDirectDraw) {
        NOTE("Releasing instance");
        m_pDirectDraw->Release();
        m_pDirectDraw = NULL;
    }

}


// Return NOERROR (S_OK) if DirectDraw has been loaded by this object

HRESULT CLoadDirectDraw::IsDirectDrawLoaded()
{
    NOTE("Entering IsDirectDrawLoaded");

    if (m_pDirectDraw == NULL) {
        NOTE("DirectDraw not loaded");
        return S_FALSE;
    }
    return NOERROR;
}


// Return the IDirectDraw interface we look after

LPDIRECTDRAW CLoadDirectDraw::GetDirectDraw()
{
    NOTE("Entering GetDirectDraw");

    if (m_pDirectDraw == NULL) {
        NOTE("No DirectDraw");
        return NULL;
    }

    NOTE("Returning DirectDraw");
    m_pDirectDraw->AddRef();
    return m_pDirectDraw;
}


// Are we running on Direct Draw version 1?  We need to find out as
// we rely on specific bug fixes in DirectDraw 2 for fullscreen playback. To
// find out, we simply see if it supports IDirectDraw2.  Only version 2 and
// higher support this.

BOOL CLoadDirectDraw::IsDirectDrawVersion1()
{

    if (m_pDirectDraw == NULL)
	return FALSE;

    IDirectDraw2 *p = NULL;
    HRESULT hr = m_pDirectDraw->QueryInterface(IID_IDirectDraw2, (void **)&p);
    if (p)
	p->Release();
    if (hr == NOERROR) {
        DbgLog((LOG_TRACE,3,TEXT("Direct Draw Version 2 or greater")));
	return FALSE;
    } else {
        DbgLog((LOG_TRACE,3,TEXT("Direct Draw Version 1")));
	return TRUE;
    }
}
