//------------------------------------------------------------------------------
// File: WinCtrl.h
//
// Desc: DirectShow base classes - defines classes for video control 
//       interfaces.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __WINCTRL__
#define __WINCTRL__

#define ABSOL(x) (x < 0 ? -x : x)
#define NEGAT(x) (x > 0 ? -x : x)

//  Helper
BOOL WINAPI PossiblyEatMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CBaseControlWindow : public CBaseVideoWindow, public CBaseWindow
{
protected:

    CBaseFilter *m_pFilter;            // Pointer to owning media filter
    CBasePin *m_pPin;                  // Controls media types for connection
    CCritSec *m_pInterfaceLock;        // Externally defined critical section
    COLORREF m_BorderColour;           // Current window border colour
    BOOL m_bAutoShow;                  // What happens when the state changes
    HWND m_hwndOwner;                  // Owner window that we optionally have
    HWND m_hwndDrain;                  // HWND to post any messages received
    BOOL m_bCursorHidden;              // Should we hide the window cursor

public:

    // Internal methods for other objects to get information out

    HRESULT DoSetWindowStyle(long Style,long WindowLong);
    HRESULT DoGetWindowStyle(long *pStyle,long WindowLong);
    BOOL IsAutoShowEnabled() { return m_bAutoShow; };
    COLORREF GetBorderColour() { return m_BorderColour; };
    HWND GetOwnerWindow() { return m_hwndOwner; };
    BOOL IsCursorHidden() { return m_bCursorHidden; };

    inline BOOL PossiblyEatMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return ::PossiblyEatMessage(m_hwndDrain, uMsg, wParam, lParam);
    }

    // Derived classes must call this to set the pin the filter is using
    // We don't have the pin passed in to the constructor (as we do with
    // the CBaseFilter object) because filters typically create the
    // pins dynamically when requested in CBaseFilter::GetPin. This can
    // not be called from our constructor because is is a virtual method

    void SetControlWindowPin(CBasePin *pPin) {
        m_pPin = pPin;
    }

public:

    CBaseControlWindow(CBaseFilter *pFilter,   // Owning media filter
                       CCritSec *pInterfaceLock,    // Locking object
                       TCHAR *pName,                // Object description
                       LPUNKNOWN pUnk,              // Normal COM ownership
                       HRESULT *phr);               // OLE return code

    // These are the properties we support

    STDMETHODIMP put_Caption(BSTR strCaption);
    STDMETHODIMP get_Caption(BSTR *pstrCaption);
    STDMETHODIMP put_AutoShow(long AutoShow);
    STDMETHODIMP get_AutoShow(long *AutoShow);
    STDMETHODIMP put_WindowStyle(long WindowStyle);
    STDMETHODIMP get_WindowStyle(long *pWindowStyle);
    STDMETHODIMP put_WindowStyleEx(long WindowStyleEx);
    STDMETHODIMP get_WindowStyleEx(long *pWindowStyleEx);
    STDMETHODIMP put_WindowState(long WindowState);
    STDMETHODIMP get_WindowState(long *pWindowState);
    STDMETHODIMP put_BackgroundPalette(long BackgroundPalette);
    STDMETHODIMP get_BackgroundPalette(long *pBackgroundPalette);
    STDMETHODIMP put_Visible(long Visible);
    STDMETHODIMP get_Visible(long *pVisible);
    STDMETHODIMP put_Left(long Left);
    STDMETHODIMP get_Left(long *pLeft);
    STDMETHODIMP put_Width(long Width);
    STDMETHODIMP get_Width(long *pWidth);
    STDMETHODIMP put_Top(long Top);
    STDMETHODIMP get_Top(long *pTop);
    STDMETHODIMP put_Height(long Height);
    STDMETHODIMP get_Height(long *pHeight);
    STDMETHODIMP put_Owner(OAHWND Owner);
    STDMETHODIMP get_Owner(OAHWND *Owner);
    STDMETHODIMP put_MessageDrain(OAHWND Drain);
    STDMETHODIMP get_MessageDrain(OAHWND *Drain);
    STDMETHODIMP get_BorderColor(long *Color);
    STDMETHODIMP put_BorderColor(long Color);
    STDMETHODIMP get_FullScreenMode(long *FullScreenMode);
    STDMETHODIMP put_FullScreenMode(long FullScreenMode);

    // And these are the methods

    STDMETHODIMP SetWindowForeground(long Focus);
    STDMETHODIMP NotifyOwnerMessage(OAHWND hwnd,long uMsg,LONG_PTR wParam,LONG_PTR lParam);
    STDMETHODIMP GetMinIdealImageSize(long *pWidth,long *pHeight);
    STDMETHODIMP GetMaxIdealImageSize(long *pWidth,long *pHeight);
    STDMETHODIMP SetWindowPosition(long Left,long Top,long Width,long Height);
    STDMETHODIMP GetWindowPosition(long *pLeft,long *pTop,long *pWidth,long *pHeight);
    STDMETHODIMP GetRestorePosition(long *pLeft,long *pTop,long *pWidth,long *pHeight);
	STDMETHODIMP HideCursor(long HideCursor);
    STDMETHODIMP IsCursorHidden(long *CursorHidden);
};

// This class implements the IBasicVideo interface

class CBaseControlVideo : public CBaseBasicVideo
{
protected:

    CBaseFilter *m_pFilter;   // Pointer to owning media filter
    CBasePin *m_pPin;                   // Controls media types for connection
    CCritSec *m_pInterfaceLock;         // Externally defined critical section

public:

    // Derived classes must provide these for the implementation

    virtual HRESULT IsDefaultTargetRect() PURE;
    virtual HRESULT SetDefaultTargetRect() PURE;
    virtual HRESULT SetTargetRect(RECT *pTargetRect) PURE;
    virtual HRESULT GetTargetRect(RECT *pTargetRect) PURE;
    virtual HRESULT IsDefaultSourceRect() PURE;
    virtual HRESULT SetDefaultSourceRect() PURE;
    virtual HRESULT SetSourceRect(RECT *pSourceRect) PURE;
    virtual HRESULT GetSourceRect(RECT *pSourceRect) PURE;
    virtual HRESULT GetStaticImage(long *pBufferSize,long *pDIBImage) PURE;

    // Derived classes must override this to return a VIDEOINFO representing
    // the video format. We cannot call IPin ConnectionMediaType to get this
    // format because various filters dynamically change the type when using
    // DirectDraw such that the format shows the position of the logical
    // bitmap in a frame buffer surface, so the size might be returned as
    // 1024x768 pixels instead of 320x240 which is the real video dimensions

    virtual VIDEOINFOHEADER *GetVideoFormat() PURE;

    // Helper functions for creating memory renderings of a DIB image

    HRESULT GetImageSize(VIDEOINFOHEADER *pVideoInfo,
                         LONG *pBufferSize,
                         RECT *pSourceRect);

    HRESULT CopyImage(IMediaSample *pMediaSample,
                      VIDEOINFOHEADER *pVideoInfo,
                      LONG *pBufferSize,
                      BYTE *pVideoImage,
                      RECT *pSourceRect);

    // Override this if you want notifying when the rectangles change
    virtual HRESULT OnUpdateRectangles() { return NOERROR; };
    virtual HRESULT OnVideoSizeChange();

    // Derived classes must call this to set the pin the filter is using
    // We don't have the pin passed in to the constructor (as we do with
    // the CBaseFilter object) because filters typically create the
    // pins dynamically when requested in CBaseFilter::GetPin. This can
    // not be called from our constructor because is is a virtual method

    void SetControlVideoPin(CBasePin *pPin) {
        m_pPin = pPin;
    }

    // Helper methods for checking rectangles
    virtual HRESULT CheckSourceRect(RECT *pSourceRect);
    virtual HRESULT CheckTargetRect(RECT *pTargetRect);

public:

    CBaseControlVideo(CBaseFilter *pFilter,    // Owning media filter
                      CCritSec *pInterfaceLock,     // Serialise interface
                      TCHAR *pName,                 // Object description
                      LPUNKNOWN pUnk,               // Normal COM ownership
                      HRESULT *phr);                // OLE return code

    // These are the properties we support

    STDMETHODIMP get_AvgTimePerFrame(REFTIME *pAvgTimePerFrame);
    STDMETHODIMP get_BitRate(long *pBitRate);
    STDMETHODIMP get_BitErrorRate(long *pBitErrorRate);
    STDMETHODIMP get_VideoWidth(long *pVideoWidth);
    STDMETHODIMP get_VideoHeight(long *pVideoHeight);
    STDMETHODIMP put_SourceLeft(long SourceLeft);
    STDMETHODIMP get_SourceLeft(long *pSourceLeft);
    STDMETHODIMP put_SourceWidth(long SourceWidth);
    STDMETHODIMP get_SourceWidth(long *pSourceWidth);
    STDMETHODIMP put_SourceTop(long SourceTop);
    STDMETHODIMP get_SourceTop(long *pSourceTop);
    STDMETHODIMP put_SourceHeight(long SourceHeight);
    STDMETHODIMP get_SourceHeight(long *pSourceHeight);
    STDMETHODIMP put_DestinationLeft(long DestinationLeft);
    STDMETHODIMP get_DestinationLeft(long *pDestinationLeft);
    STDMETHODIMP put_DestinationWidth(long DestinationWidth);
    STDMETHODIMP get_DestinationWidth(long *pDestinationWidth);
    STDMETHODIMP put_DestinationTop(long DestinationTop);
    STDMETHODIMP get_DestinationTop(long *pDestinationTop);
    STDMETHODIMP put_DestinationHeight(long DestinationHeight);
    STDMETHODIMP get_DestinationHeight(long *pDestinationHeight);

    // And these are the methods

    STDMETHODIMP GetVideoSize(long *pWidth,long *pHeight);
    STDMETHODIMP SetSourcePosition(long Left,long Top,long Width,long Height);
    STDMETHODIMP GetSourcePosition(long *pLeft,long *pTop,long *pWidth,long *pHeight);
    STDMETHODIMP GetVideoPaletteEntries(long StartIndex,long Entries,long *pRetrieved,long *pPalette);
    STDMETHODIMP SetDefaultSourcePosition();
    STDMETHODIMP IsUsingDefaultSource();
    STDMETHODIMP SetDestinationPosition(long Left,long Top,long Width,long Height);
    STDMETHODIMP GetDestinationPosition(long *pLeft,long *pTop,long *pWidth,long *pHeight);
    STDMETHODIMP SetDefaultDestinationPosition();
    STDMETHODIMP IsUsingDefaultDestination();
    STDMETHODIMP GetCurrentImage(long *pBufferSize,long *pVideoImage);
};

#endif // __WINCTRL__

