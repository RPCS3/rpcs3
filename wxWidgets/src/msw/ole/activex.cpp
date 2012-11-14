/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/ole/activex.cpp
// Purpose:     wxActiveXContainer implementation
// Author:      Ryan Norton <wxprojects@comcast.net>, Lindsay Mathieson <???>
// Modified by:
// Created:     11/07/04
// RCS-ID:      $Id: activex.cpp 42688 2006-10-29 23:38:37Z VZ $
// Copyright:   (c) 2003 Lindsay Mathieson, (c) 2005 Ryan Norton
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_ACTIVEX

#ifndef WX_PRECOMP
    #include "wx/dcclient.h"
    #include "wx/math.h"
#endif

#include "wx/msw/ole/activex.h"
// autointerfaces that we only use here
WX_DECLARE_AUTOOLE(wxAutoIOleInPlaceSite, IOleInPlaceSite)
WX_DECLARE_AUTOOLE(wxAutoIOleDocument, IOleDocument)
WX_DECLARE_AUTOOLE(wxAutoIPersistStreamInit, IPersistStreamInit)
WX_DECLARE_AUTOOLE(wxAutoIAdviseSink, IAdviseSink)
WX_DECLARE_AUTOOLE(wxAutoIProvideClassInfo, IProvideClassInfo)
WX_DECLARE_AUTOOLE(wxAutoITypeInfo, ITypeInfo)
WX_DECLARE_AUTOOLE(wxAutoIConnectionPoint, IConnectionPoint)
WX_DECLARE_AUTOOLE(wxAutoIConnectionPointContainer, IConnectionPointContainer)

DEFINE_EVENT_TYPE(wxEVT_ACTIVEX)

// Ole class helpers (sort of MFC-like) from wxActiveX
#define DECLARE_OLE_UNKNOWN(cls)\
    private:\
    class TAutoInitInt\
    {\
        public:\
        LONG l;\
        TAutoInitInt() : l(0) {}\
    };\
    TAutoInitInt refCount, lockCount;\
    static void _GetInterface(cls *self, REFIID iid, void **_interface, const char *&desc);\
    public:\
    LONG GetRefCount();\
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);\
    ULONG STDMETHODCALLTYPE AddRef();\
    ULONG STDMETHODCALLTYPE Release();\
    ULONG STDMETHODCALLTYPE AddLock();\
    ULONG STDMETHODCALLTYPE ReleaseLock()

#define DEFINE_OLE_TABLE(cls)\
    LONG cls::GetRefCount() {return refCount.l;}\
    HRESULT STDMETHODCALLTYPE cls::QueryInterface(REFIID iid, void ** ppvObject)\
    {\
        if (! ppvObject)\
        {\
            return E_FAIL;\
        }\
        const char *desc = NULL;\
        cls::_GetInterface(this, iid, ppvObject, desc);\
        if (! *ppvObject)\
        {\
            return E_NOINTERFACE;\
        }\
        ((IUnknown * )(*ppvObject))->AddRef();\
        return S_OK;\
    }\
    ULONG STDMETHODCALLTYPE cls::AddRef()\
    {\
        InterlockedIncrement(&refCount.l);\
        return refCount.l;\
    }\
    ULONG STDMETHODCALLTYPE cls::Release()\
    {\
        if (refCount.l > 0)\
        {\
            InterlockedDecrement(&refCount.l);\
            if (refCount.l == 0)\
            {\
                delete this;\
                return 0;\
            }\
            return refCount.l;\
        }\
        else\
            return 0;\
    }\
    ULONG STDMETHODCALLTYPE cls::AddLock()\
    {\
        InterlockedIncrement(&lockCount.l);\
        return lockCount.l;\
    }\
    ULONG STDMETHODCALLTYPE cls::ReleaseLock()\
    {\
        if (lockCount.l > 0)\
        {\
            InterlockedDecrement(&lockCount.l);\
            return lockCount.l;\
        }\
        else\
            return 0;\
    }\
    DEFINE_OLE_BASE(cls)

#define DEFINE_OLE_BASE(cls)\
    void cls::_GetInterface(cls *self, REFIID iid, void **_interface, const char *&desc)\
    {\
        *_interface = NULL;\
        desc = NULL;

#define OLE_INTERFACE(_iid, _type)\
    if (IsEqualIID(iid, _iid))\
    {\
        *_interface = (IUnknown *) (_type *) self;\
        desc = # _iid;\
        return;\
    }

#define OLE_IINTERFACE(_face) OLE_INTERFACE(IID_##_face, _face)

#define OLE_INTERFACE_CUSTOM(func)\
    if (func(self, iid, _interface, desc))\
    {\
        return;\
    }

#define END_OLE_TABLE\
    }

// ============================================================================
// implementation
// ============================================================================

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PixelsToHimetric
//
// Utility to convert from pixels to the himetric values in some COM methods
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define HIMETRIC_PER_INCH   2540
#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))

static void PixelsToHimetric(SIZEL &sz)
{
    static int logX = 0;
    static int logY = 0;

    if (logY == 0)
    {
        // initaliase
        HDC dc = GetDC(NULL);
        logX = GetDeviceCaps(dc, LOGPIXELSX);
        logY = GetDeviceCaps(dc, LOGPIXELSY);
        ReleaseDC(NULL, dc);
    };

#define HIMETRIC_INCH   2540
#define CONVERT(x, logpixels)   wxMulDivInt32(HIMETRIC_INCH, (x), (logpixels))

    sz.cx = CONVERT(sz.cx, logX);
    sz.cy = CONVERT(sz.cy, logY);

#undef CONVERT
#undef HIMETRIC_INCH
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FrameSite
//
// Handles the actual wxActiveX container implementation
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class FrameSite :
    public IOleClientSite,
    public IOleInPlaceSiteEx,
    public IOleInPlaceFrame,
    public IOleItemContainer,
    public IDispatch,
    public IOleCommandTarget,
    public IOleDocumentSite,
    public IAdviseSink,
    public IOleControlSite
{
private:
    DECLARE_OLE_UNKNOWN(FrameSite);

public:
    FrameSite(wxWindow * win, wxActiveXContainer * win2)
    {
        m_window = win2;
        m_bSupportsWindowlessActivation = true;
        m_bInPlaceLocked = false;
        m_bUIActive = false;
        m_bInPlaceActive = false;
        m_bWindowless = false;

        m_nAmbientLocale = 0;
        m_clrAmbientForeColor = ::GetSysColor(COLOR_WINDOWTEXT);
        m_clrAmbientBackColor = ::GetSysColor(COLOR_WINDOW);
        m_bAmbientShowHatching = true;
        m_bAmbientShowGrabHandles = true;
        m_bAmbientAppearance = true;

        m_hDCBuffer = NULL;
        m_hWndParent = (HWND)win->GetHWND();
    }
    virtual ~FrameSite(){}
    //***************************IDispatch*****************************
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, OLECHAR ** ,
                                            unsigned int , LCID ,
                                            DISPID * )
    {   return E_NOTIMPL;   }
    STDMETHOD(GetTypeInfo)(unsigned int, LCID, ITypeInfo **)
    {   return E_NOTIMPL;   }
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int *)
    {   return E_NOTIMPL;   }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID, LCID,
                            WORD wFlags, DISPPARAMS *,
                            VARIANT * pVarResult, EXCEPINFO *,
                            unsigned int *)
    {
        if (!(wFlags & DISPATCH_PROPERTYGET))
            return S_OK;

        if (pVarResult == NULL)
            return E_INVALIDARG;

        //The most common case is boolean, use as an initial type
        V_VT(pVarResult) = VT_BOOL;

        switch (dispIdMember)
        {
            case DISPID_AMBIENT_MESSAGEREFLECT:
                V_BOOL(pVarResult)= FALSE;
                return S_OK;

            case DISPID_AMBIENT_DISPLAYASDEFAULT:
                V_BOOL(pVarResult)= TRUE;
                return S_OK;

            case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
                V_BOOL(pVarResult) = TRUE;
                return S_OK;

            case DISPID_AMBIENT_SILENT:
                V_BOOL(pVarResult)= TRUE;
                return S_OK;

            case DISPID_AMBIENT_APPEARANCE:
                pVarResult->vt = VT_BOOL;
                pVarResult->boolVal = m_bAmbientAppearance;
                break;

            case DISPID_AMBIENT_FORECOLOR:
                pVarResult->vt = VT_I4;
                pVarResult->lVal = (long) m_clrAmbientForeColor;
                break;

            case DISPID_AMBIENT_BACKCOLOR:
                pVarResult->vt = VT_I4;
                pVarResult->lVal = (long) m_clrAmbientBackColor;
                break;

            case DISPID_AMBIENT_LOCALEID:
                pVarResult->vt = VT_I4;
                pVarResult->lVal = (long) m_nAmbientLocale;
                break;

            case DISPID_AMBIENT_USERMODE:
                pVarResult->vt = VT_BOOL;
                pVarResult->boolVal = m_window->m_bAmbientUserMode;
                break;

            case DISPID_AMBIENT_SHOWGRABHANDLES:
                pVarResult->vt = VT_BOOL;
                pVarResult->boolVal = m_bAmbientShowGrabHandles;
                break;

            case DISPID_AMBIENT_SHOWHATCHING:
                pVarResult->vt = VT_BOOL;
                pVarResult->boolVal = m_bAmbientShowHatching;
                break;

            default:
                return DISP_E_MEMBERNOTFOUND;
        }

        return S_OK;
    }

    //**************************IOleWindow***************************
    HRESULT STDMETHODCALLTYPE GetWindow(HWND * phwnd)
    {
        if (phwnd == NULL)
            return E_INVALIDARG;
        (*phwnd) = m_hWndParent;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL)
    {return S_OK;}
    //**************************IOleInPlaceUIWindow*****************
    HRESULT STDMETHODCALLTYPE GetBorder(LPRECT lprectBorder)
    {
        if (lprectBorder == NULL)
            return E_INVALIDARG;
        return INPLACE_E_NOTOOLSPACE;
    }
    HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
    {
        if (pborderwidths == NULL)
            return E_INVALIDARG;
        return INPLACE_E_NOTOOLSPACE;
    }
    HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS)
    {return S_OK;}
    HRESULT STDMETHODCALLTYPE SetActiveObject(
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR)
    {
        if (pActiveObject)
            pActiveObject->AddRef();

        m_window->m_oleInPlaceActiveObject = pActiveObject;
        return S_OK;
    }

    //********************IOleInPlaceFrame************************

    STDMETHOD(InsertMenus)(HMENU, LPOLEMENUGROUPWIDTHS){return S_OK;}
    STDMETHOD(SetMenu)(HMENU, HOLEMENU, HWND){  return S_OK;}
    STDMETHOD(RemoveMenus)(HMENU){return S_OK;}
    STDMETHOD(SetStatusText)(LPCOLESTR){ return S_OK;}
    HRESULT STDMETHODCALLTYPE EnableModeless(BOOL){return S_OK;}
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg, WORD)
    {
        // TODO: send an event with this id
        if (m_window->m_oleInPlaceActiveObject.Ok())
            m_window->m_oleInPlaceActiveObject->TranslateAccelerator(lpmsg);
        return S_FALSE;
    }

    //*******************IOleInPlaceSite**************************
    HRESULT STDMETHODCALLTYPE CanInPlaceActivate(){return S_OK;}
    HRESULT STDMETHODCALLTYPE OnInPlaceActivate()
    {   m_bInPlaceActive = true;    return S_OK;    }
    HRESULT STDMETHODCALLTYPE OnUIActivate()
    {   m_bUIActive = true;         return S_OK;    }
    HRESULT STDMETHODCALLTYPE GetWindowContext(IOleInPlaceFrame **ppFrame,
                                        IOleInPlaceUIWindow **ppDoc,
                                        LPRECT lprcPosRect,
                                        LPRECT lprcClipRect,
                                        LPOLEINPLACEFRAMEINFO lpFrameInfo)
    {
        if (ppFrame == NULL || ppDoc == NULL || lprcPosRect == NULL ||
            lprcClipRect == NULL || lpFrameInfo == NULL)
        {
            if (ppFrame != NULL)
                (*ppFrame) = NULL;
            if (ppDoc != NULL)
                (*ppDoc) = NULL;
            return E_INVALIDARG;
        }

        HRESULT hr = QueryInterface(IID_IOleInPlaceFrame, (void **) ppFrame);
        if (! SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = QueryInterface(IID_IOleInPlaceUIWindow, (void **) ppDoc);
        if (! SUCCEEDED(hr))
        {
            (*ppFrame)->Release();
            *ppFrame = NULL;
            return E_UNEXPECTED;
        }

        RECT rect;
        ::GetClientRect(m_hWndParent, &rect);
        if (lprcPosRect)
        {
            lprcPosRect->left = lprcPosRect->top = 0;
            lprcPosRect->right = rect.right;
            lprcPosRect->bottom = rect.bottom;
        }
        if (lprcClipRect)
        {
            lprcClipRect->left = lprcClipRect->top = 0;
            lprcClipRect->right = rect.right;
            lprcClipRect->bottom = rect.bottom;
        }

        memset(lpFrameInfo, 0, sizeof(OLEINPLACEFRAMEINFO));
        lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
        lpFrameInfo->hwndFrame = m_hWndParent;

        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Scroll(SIZE){return S_OK;}
    HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL)
    {   m_bUIActive = false;         return S_OK;    }
    HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate()
    {   m_bInPlaceActive = false;    return S_OK;    }
    HRESULT STDMETHODCALLTYPE DiscardUndoState(){return S_OK;}
    HRESULT STDMETHODCALLTYPE DeactivateAndUndo(){return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT lprcPosRect)
    {
        if (m_window->m_oleInPlaceObject.Ok() && lprcPosRect)
        {
           //
           // Result of several hours and days of bug hunting -
           // this is called by an object when it wants to resize
           // itself to something different then our parent window -
           // don't let it :)
           //
//            m_window->m_oleInPlaceObject->SetObjectRects(
//                lprcPosRect, lprcPosRect);
           RECT rcClient;
           ::GetClientRect(m_hWndParent, &rcClient);
            m_window->m_oleInPlaceObject->SetObjectRects(
                &rcClient, &rcClient);
        }
        return S_OK;
    }
    //*************************IOleInPlaceSiteEx***********************
    HRESULT STDMETHODCALLTYPE OnInPlaceActivateEx(BOOL * pfNoRedraw, DWORD)
    {
#ifdef __WXWINCE__
        IRunnableObject* runnable = NULL;
        HRESULT hr = QueryInterface(
            IID_IRunnableObject, (void**)(& runnable));
        if (SUCCEEDED(hr))
        {
            runnable->LockRunning(TRUE, FALSE);
        }
#else
        OleLockRunning(m_window->m_ActiveX, TRUE, FALSE);
#endif
        if (pfNoRedraw)
            (*pfNoRedraw) = FALSE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnInPlaceDeactivateEx(BOOL)
    {
#ifdef __WXWINCE__
        IRunnableObject* runnable = NULL;
        HRESULT hr = QueryInterface(
            IID_IRunnableObject, (void**)(& runnable));
        if (SUCCEEDED(hr))
        {
            runnable->LockRunning(FALSE, FALSE);
        }
#else
        OleLockRunning(m_window->m_ActiveX, FALSE, FALSE);
#endif
        return S_OK;
    }
    STDMETHOD(RequestUIActivate)(){ return S_OK;}
    //*************************IOleClientSite**************************
    HRESULT STDMETHODCALLTYPE SaveObject(){return S_OK;}
    const char *OleGetMonikerToStr(DWORD dwAssign)
    {
        switch (dwAssign)
        {
        case OLEGETMONIKER_ONLYIFTHERE  : return "OLEGETMONIKER_ONLYIFTHERE";
        case OLEGETMONIKER_FORCEASSIGN  : return "OLEGETMONIKER_FORCEASSIGN";
        case OLEGETMONIKER_UNASSIGN     : return "OLEGETMONIKER_UNASSIGN";
        case OLEGETMONIKER_TEMPFORUSER  : return "OLEGETMONIKER_TEMPFORUSER";
        default                         : return "Bad Enum";
        }
    }

    const char *OleGetWhicMonikerStr(DWORD dwWhichMoniker)
    {
        switch(dwWhichMoniker)
        {
        case OLEWHICHMK_CONTAINER   : return "OLEWHICHMK_CONTAINER";
        case OLEWHICHMK_OBJREL      : return "OLEWHICHMK_OBJREL";
        case OLEWHICHMK_OBJFULL     : return "OLEWHICHMK_OBJFULL";
        default                     : return "Bad Enum";
        }
    }
    STDMETHOD(GetMoniker)(DWORD, DWORD, IMoniker **){return E_FAIL;}
    HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER * ppContainer)
    {
        if (ppContainer == NULL)
            return E_INVALIDARG;
        HRESULT hr = QueryInterface(
            IID_IOleContainer, (void**)(ppContainer));
        wxASSERT(SUCCEEDED(hr));
        return hr;
    }
    HRESULT STDMETHODCALLTYPE ShowObject()
    {
        if (m_window->m_oleObjectHWND)
            ::ShowWindow(m_window->m_oleObjectHWND, SW_SHOW);
        return S_OK;
    }
    STDMETHOD(OnShowWindow)(BOOL){return S_OK;}
    STDMETHOD(RequestNewObjectLayout)(){return E_NOTIMPL;}
    //********************IParseDisplayName***************************
    HRESULT STDMETHODCALLTYPE ParseDisplayName(
        IBindCtx *, LPOLESTR, ULONG *, IMoniker **){return E_NOTIMPL;}
    //********************IOleContainer*******************************
    STDMETHOD(EnumObjects)(DWORD, IEnumUnknown **){return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE LockContainer(BOOL){return S_OK;}
    //********************IOleItemContainer***************************
    HRESULT STDMETHODCALLTYPE
    #if 0 // defined(__WXWINCE__) && __VISUALC__ < 1400
    GetObject
    #elif defined(_UNICODE)
    GetObjectW
    #else
    GetObjectA
    #endif
    (LPOLESTR pszItem, DWORD, IBindCtx *, REFIID, void ** ppvObject)
    {
        if (pszItem == NULL || ppvObject == NULL)
            return E_INVALIDARG;
        *ppvObject = NULL;
        return MK_E_NOOBJECT;
    }
    HRESULT STDMETHODCALLTYPE GetObjectStorage(
        LPOLESTR pszItem, IBindCtx * , REFIID, void ** ppvStorage)
    {
        if (pszItem == NULL || ppvStorage == NULL)
            return E_INVALIDARG;
        *ppvStorage = NULL;
        return MK_E_NOOBJECT;
    }
    HRESULT STDMETHODCALLTYPE IsRunning(LPOLESTR pszItem)
    {
        if (pszItem == NULL)
            return E_INVALIDARG;
        return MK_E_NOOBJECT;
    }
    //***********************IOleControlSite*****************************
    HRESULT STDMETHODCALLTYPE OnControlInfoChanged()
    {return S_OK;}
    HRESULT STDMETHODCALLTYPE LockInPlaceActive(BOOL fLock)
    {
        m_bInPlaceLocked = (fLock) ? true : false;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetExtendedControl(IDispatch **)
    {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE TransformCoords(
        POINTL * pPtlHimetric, POINTF * pPtfContainer, DWORD)
    {
        if (pPtlHimetric == NULL || pPtfContainer == NULL)
            return E_INVALIDARG;
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG, DWORD)
    {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE OnFocus(BOOL){return S_OK;}
    HRESULT STDMETHODCALLTYPE ShowPropertyFrame(){return E_NOTIMPL;}
    //**************************IOleCommandTarget***********************
    HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *, ULONG cCmds,
                                OLECMD prgCmds[], OLECMDTEXT *)
    {
        if (prgCmds == NULL) return E_INVALIDARG;
        for (ULONG nCmd = 0; nCmd < cCmds; nCmd++)
        {
            // unsupported by default
            prgCmds[nCmd].cmdf = 0;
        }
        return OLECMDERR_E_UNKNOWNGROUP;
    }

    HRESULT STDMETHODCALLTYPE Exec(const GUID *, DWORD,
                            DWORD, VARIANTARG *, VARIANTARG *)
    {return OLECMDERR_E_NOTSUPPORTED;}

    //**********************IAdviseSink************************************
    void STDMETHODCALLTYPE OnDataChange(FORMATETC *, STGMEDIUM *) {}
    void STDMETHODCALLTYPE OnViewChange(DWORD, LONG) {}
    void STDMETHODCALLTYPE OnRename(IMoniker *){}
    void STDMETHODCALLTYPE OnSave(){}
    void STDMETHODCALLTYPE OnClose(){}

    //**********************IOleDocumentSite***************************
    HRESULT STDMETHODCALLTYPE ActivateMe(
        IOleDocumentView __RPC_FAR *pViewToActivate)
    {
        wxAutoIOleInPlaceSite inPlaceSite(
            IID_IOleInPlaceSite, (IDispatch *) this);
        if (!inPlaceSite.Ok())
            return E_FAIL;

        if (pViewToActivate)
        {
            m_window->m_docView = pViewToActivate;
            m_window->m_docView->SetInPlaceSite(inPlaceSite);
        }
        else
        {
            wxAutoIOleDocument oleDoc(
                IID_IOleDocument, m_window->m_oleObject);
            if (! oleDoc.Ok())
                return E_FAIL;

            HRESULT hr = oleDoc->CreateView(inPlaceSite, NULL,
                                    0, m_window->m_docView.GetRef());
            if (hr != S_OK)
                return E_FAIL;

            m_window->m_docView->SetInPlaceSite(inPlaceSite);
        }

        m_window->m_docView->UIActivate(TRUE);
        return S_OK;
    }


protected:
    wxActiveXContainer * m_window;

    HDC m_hDCBuffer;
    HWND m_hWndParent;

    bool m_bSupportsWindowlessActivation;
    bool m_bInPlaceLocked;
    bool m_bInPlaceActive;
    bool m_bUIActive;
    bool m_bWindowless;

    LCID m_nAmbientLocale;
    COLORREF m_clrAmbientForeColor;
    COLORREF m_clrAmbientBackColor;
    bool m_bAmbientShowHatching;
    bool m_bAmbientShowGrabHandles;
    bool m_bAmbientAppearance;
};

DEFINE_OLE_TABLE(FrameSite)
    OLE_INTERFACE(IID_IUnknown, IOleClientSite)
    OLE_IINTERFACE(IOleClientSite)
    OLE_INTERFACE(IID_IOleWindow, IOleInPlaceSite)
    OLE_IINTERFACE(IOleInPlaceSite)
    OLE_IINTERFACE(IOleInPlaceSiteEx)
    OLE_IINTERFACE(IOleInPlaceUIWindow)
    OLE_IINTERFACE(IOleInPlaceFrame)
    OLE_IINTERFACE(IParseDisplayName)
    OLE_IINTERFACE(IOleContainer)
    OLE_IINTERFACE(IOleItemContainer)
    OLE_IINTERFACE(IDispatch)
    OLE_IINTERFACE(IOleCommandTarget)
    OLE_IINTERFACE(IOleDocumentSite)
    OLE_IINTERFACE(IAdviseSink)
    OLE_IINTERFACE(IOleControlSite)
END_OLE_TABLE


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// wxActiveXEvents
//
// Handles and sends activex events received from the ActiveX control
// to the appropriate wxEvtHandler
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class wxActiveXEvents : public IDispatch
{
private:
    DECLARE_OLE_UNKNOWN(wxActiveXEvents);


    wxActiveXContainer *m_activeX;
    IID m_customId;
    bool m_haveCustomId;

    friend bool wxActiveXEventsInterface(wxActiveXEvents *self, REFIID iid, void **_interface, const char *&desc);

public:
    wxActiveXEvents(wxActiveXContainer *ax) : m_activeX(ax), m_haveCustomId(false) {}
    wxActiveXEvents(wxActiveXContainer *ax, REFIID iid) : m_activeX(ax), m_customId(iid), m_haveCustomId(true) {}
    virtual ~wxActiveXEvents()
    {
    }

    // IDispatch
    STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR**, unsigned int, LCID, DISPID*)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetTypeInfo(unsigned int, LCID, ITypeInfo**)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetTypeInfoCount(unsigned int*)
    {
        return E_NOTIMPL;
    }


    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID WXUNUSED(riid),
                        LCID WXUNUSED(lcid),
                          WORD wFlags, DISPPARAMS * pDispParams,
                          VARIANT * WXUNUSED(pVarResult), EXCEPINFO * WXUNUSED(pExcepInfo),
                          unsigned int * WXUNUSED(puArgErr))
    {
        if (wFlags & (DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
            return E_NOTIMPL;

        wxASSERT(m_activeX);

        // ActiveX Event

        // Dispatch Event
        wxActiveXEvent  event;
        event.SetEventType(wxEVT_ACTIVEX);
        event.m_params.NullList();
        event.m_dispid = dispIdMember;

        // arguments
        if (pDispParams)
        {
            for (DWORD i = pDispParams->cArgs; i > 0; i--)
            {
                VARIANTARG& va = pDispParams->rgvarg[i-1];
                wxVariant vx;

//                        vx.SetName(px.name);
                wxConvertOleToVariant(va, vx);
                event.m_params.Append(vx);
            }
        }

        // process the events from the activex method
           m_activeX->ProcessEvent(event);
        for (DWORD i = 0; i < pDispParams->cArgs; i++)
        {
            VARIANTARG& va = pDispParams->rgvarg[i];
            wxVariant& vx =
                event.m_params[pDispParams->cArgs - i - 1];
            wxConvertVariantToOle(vx, va);
        }

        if(event.GetSkipped())
            return DISP_E_MEMBERNOTFOUND;

        return S_OK;
    }
};

bool wxActiveXEventsInterface(wxActiveXEvents *self, REFIID iid, void **_interface, const char *&desc)
{
    if (self->m_haveCustomId && IsEqualIID(iid, self->m_customId))
    {
//        WXOLE_TRACE("Found Custom Dispatch Interface");
        *_interface = (IUnknown *) (IDispatch *) self;
        desc = "Custom Dispatch Interface";
        return true;
    }

    return false;
}

DEFINE_OLE_TABLE(wxActiveXEvents)
    OLE_IINTERFACE(IUnknown)
    OLE_INTERFACE(IID_IDispatch, IDispatch)
    OLE_INTERFACE_CUSTOM(wxActiveXEventsInterface)
END_OLE_TABLE

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// wxActiveXContainer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
// wxActiveXContainer Constructor
//
// Initializes members and creates the native ActiveX container
//---------------------------------------------------------------------------
wxActiveXContainer::wxActiveXContainer(wxWindow * parent,
                                       REFIID iid, IUnknown* pUnk)
    : m_realparent(parent)
{
    m_bAmbientUserMode = true;
    m_docAdviseCookie = 0;
    CreateActiveX(iid, pUnk);
}

//---------------------------------------------------------------------------
// wxActiveXContainer Destructor
//
// Destroys members (the FrameSite et al. are destroyed implicitly
// through COM ref counting)
//---------------------------------------------------------------------------
wxActiveXContainer::~wxActiveXContainer()
{
    // disconnect connection points
    if (m_oleInPlaceObject.Ok())
    {
        m_oleInPlaceObject->InPlaceDeactivate();
        m_oleInPlaceObject->UIDeactivate();
    }

    if (m_oleObject.Ok())
    {
        if (m_docAdviseCookie != 0)
            m_oleObject->Unadvise(m_docAdviseCookie);

        m_oleObject->DoVerb(
            OLEIVERB_HIDE, NULL, m_clientSite, 0, (HWND) GetHWND(), NULL);
        m_oleObject->Close(OLECLOSE_NOSAVE);
        m_oleObject->SetClientSite(NULL);
    }
}

//---------------------------------------------------------------------------
// wxActiveXContainer::CreateActiveX
//
// Actually creates the ActiveX container through the FrameSite
// and sets up ActiveX events
//
// TODO: Document this more
//---------------------------------------------------------------------------
void wxActiveXContainer::CreateActiveX(REFIID iid, IUnknown* pUnk)
{
    HRESULT hret;
    hret = m_ActiveX.QueryInterface(iid, pUnk);
    wxASSERT(SUCCEEDED(hret));

    // FrameSite
    FrameSite *frame = new FrameSite(m_realparent, this);
    // oleClientSite
    hret = m_clientSite.QueryInterface(
        IID_IOleClientSite, (IDispatch *) frame);
    wxASSERT(SUCCEEDED(hret));
    // adviseSink
    wxAutoIAdviseSink adviseSink(IID_IAdviseSink, (IDispatch *) frame);
    wxASSERT(adviseSink.Ok());

    // Get Dispatch interface
    hret = m_Dispatch.QueryInterface(IID_IDispatch, m_ActiveX);

    //
    // SETUP TYPEINFO AND ACTIVEX EVENTS
    //

    // get type info via class info
    wxAutoIProvideClassInfo classInfo(IID_IProvideClassInfo, m_ActiveX);
    wxASSERT(classInfo.Ok());

    // type info
    wxAutoITypeInfo typeInfo;
    hret = classInfo->GetClassInfo(typeInfo.GetRef());
    wxASSERT(typeInfo.Ok());

    // TYPEATTR
    TYPEATTR *ta = NULL;
    hret = typeInfo->GetTypeAttr(&ta);
    wxASSERT(ta);

    // this should be a TKIND_COCLASS
    wxASSERT(ta->typekind == TKIND_COCLASS);

    // iterate contained interfaces
    for (int i = 0; i < ta->cImplTypes; i++)
    {
        HREFTYPE rt = 0;

        // get dispatch type info handle
        hret = typeInfo->GetRefTypeOfImplType(i, &rt);
        if (! SUCCEEDED(hret))
            continue;

        // get dispatch type info interface
        wxAutoITypeInfo  ti;
        hret = typeInfo->GetRefTypeInfo(rt, ti.GetRef());
        if (! ti.Ok())
            continue;

        // check if default event sink
        bool defEventSink = false;
        int impTypeFlags = 0;
        typeInfo->GetImplTypeFlags(i, &impTypeFlags);

        if (impTypeFlags & IMPLTYPEFLAG_FDEFAULT)
        {
            if (impTypeFlags & IMPLTYPEFLAG_FSOURCE)
            {
                // WXOLE_TRACEOUT("Default Event Sink");
                defEventSink = true;
                if (impTypeFlags & IMPLTYPEFLAG_FDEFAULTVTABLE)
                {
                    // WXOLE_TRACEOUT("*ERROR* - Default Event Sink is via vTable");
                    defEventSink = false;
                    wxFAIL_MSG(wxT("Default event sink is in vtable!"));
                }
            }
        }


        // wxAutoOleInterface<> assumes a ref has already been added
        // TYPEATTR
        TYPEATTR *ta = NULL;
        hret = ti->GetTypeAttr(&ta);
        wxASSERT(ta);

        if (ta->typekind == TKIND_DISPATCH)
        {
            // WXOLE_TRACEOUT("GUID = " << GetIIDName(ta->guid).c_str());
            if (defEventSink)
            {
                wxAutoIConnectionPoint    cp;
                DWORD                    adviseCookie = 0;

                wxAutoIConnectionPointContainer cpContainer(IID_IConnectionPointContainer, m_ActiveX);
                wxASSERT( cpContainer.Ok());

                HRESULT hret =
                    cpContainer->FindConnectionPoint(ta->guid, cp.GetRef());
                wxASSERT ( SUCCEEDED(hret));

                IDispatch* disp;
                frame->QueryInterface(IID_IDispatch, (void**)&disp);
                hret = cp->Advise(new wxActiveXEvents(this, ta->guid),
                                  &adviseCookie);
                wxASSERT_MSG( SUCCEEDED(hret),
                    wxString::Format(wxT("Cannot connect!\nHRESULT:%X"), (unsigned int)hret)
                            );
            }
        }

        ti->ReleaseTypeAttr(ta);
    }

    // free
    typeInfo->ReleaseTypeAttr(ta);

    //
    // END
    //

    // Get IOleObject interface
    hret = m_oleObject.QueryInterface(IID_IOleObject, m_ActiveX);
    wxASSERT(SUCCEEDED(hret));

    // get IViewObject Interface
    hret = m_viewObject.QueryInterface(IID_IViewObject, m_ActiveX);
    wxASSERT(SUCCEEDED(hret));

    // document advise
    m_docAdviseCookie = 0;
    hret = m_oleObject->Advise(adviseSink, &m_docAdviseCookie);
    // TODO:Needed?
//    hret = m_viewObject->SetAdvise(DVASPECT_CONTENT, 0, adviseSink);
    m_oleObject->SetHostNames(L"wxActiveXContainer", NULL);
    OleSetContainedObject(m_oleObject, TRUE);
    OleRun(m_oleObject);


    // Get IOleInPlaceObject interface
    hret = m_oleInPlaceObject.QueryInterface(
        IID_IOleInPlaceObject, m_ActiveX);
    wxASSERT(SUCCEEDED(hret));

    // status
    DWORD dwMiscStatus;
    m_oleObject->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);
    wxASSERT(SUCCEEDED(hret));

    // set client site first ?
    if (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
        m_oleObject->SetClientSite(m_clientSite);


    // stream init
    wxAutoIPersistStreamInit
        pPersistStreamInit(IID_IPersistStreamInit, m_oleObject);

    if (pPersistStreamInit.Ok())
    {
        hret = pPersistStreamInit->InitNew();
    }

    if (! (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
        m_oleObject->SetClientSite(m_clientSite);


    RECT posRect;
    ::GetClientRect((HWND)m_realparent->GetHWND(), &posRect);

    m_oleObjectHWND = 0;

    if (m_oleInPlaceObject.Ok())
    {
        hret = m_oleInPlaceObject->GetWindow(&m_oleObjectHWND);
        if (SUCCEEDED(hret))
            ::SetActiveWindow(m_oleObjectHWND);
    }


    if (! (dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME))
    {
        if (posRect.right > 0 && posRect.bottom > 0 &&
            m_oleInPlaceObject.Ok())
                m_oleInPlaceObject->SetObjectRects(&posRect, &posRect);

        hret = m_oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL,
            m_clientSite, 0, (HWND)m_realparent->GetHWND(), &posRect);
        hret = m_oleObject->DoVerb(OLEIVERB_SHOW, 0, m_clientSite, 0,
            (HWND)m_realparent->GetHWND(), &posRect);
    }

    if (! m_oleObjectHWND && m_oleInPlaceObject.Ok())
    {
        hret = m_oleInPlaceObject->GetWindow(&m_oleObjectHWND);
    }

    if (m_oleObjectHWND)
    {
        ::SetActiveWindow(m_oleObjectHWND);
        ::ShowWindow(m_oleObjectHWND, SW_SHOW);

        this->AssociateHandle(m_oleObjectHWND);
        this->Reparent(m_realparent);

        wxWindow* pWnd = m_realparent;
        int id = m_realparent->GetId();

        pWnd->Connect(id, wxEVT_SIZE,
            wxSizeEventHandler(wxActiveXContainer::OnSize), 0, this);
//        this->Connect(GetId(), wxEVT_PAINT,
//            wxPaintEventHandler(wxActiveXContainer::OnPaint), 0, this);
        pWnd->Connect(id, wxEVT_SET_FOCUS,
            wxFocusEventHandler(wxActiveXContainer::OnSetFocus), 0, this);
        pWnd->Connect(id, wxEVT_KILL_FOCUS,
            wxFocusEventHandler(wxActiveXContainer::OnKillFocus), 0, this);
    }
}

//---------------------------------------------------------------------------
// wxActiveXContainer::OnSize
//
// Called when the parent is resized - we need to do this to actually
// move the ActiveX control to where the parent is
//---------------------------------------------------------------------------
void wxActiveXContainer::OnSize(wxSizeEvent& event)
{
    int w, h;
    GetParent()->GetClientSize(&w, &h);

    RECT posRect;
    posRect.left = 0;
    posRect.top = 0;
    posRect.right = w;
    posRect.bottom = h;

    if (w <= 0 && h <= 0)
        return;

    // extents are in HIMETRIC units
    if (m_oleObject.Ok())
    {
        m_oleObject->DoVerb(OLEIVERB_HIDE, 0, m_clientSite, 0,
            (HWND)m_realparent->GetHWND(), &posRect);

        SIZEL sz = {w, h};
        PixelsToHimetric(sz);

        SIZEL sz2;

        m_oleObject->GetExtent(DVASPECT_CONTENT, &sz2);
        if (sz2.cx !=  sz.cx || sz.cy != sz2.cy)
            m_oleObject->SetExtent(DVASPECT_CONTENT, &sz);

        m_oleObject->DoVerb(OLEIVERB_SHOW, 0, m_clientSite, 0,
            (HWND)m_realparent->GetHWND(), &posRect);
    }

    if (m_oleInPlaceObject.Ok())
        m_oleInPlaceObject->SetObjectRects(&posRect, &posRect);

    event.Skip();
}

//---------------------------------------------------------------------------
// wxActiveXContainer::OnPaint
//
// Called when the parent is resized - repaints the ActiveX control
//---------------------------------------------------------------------------
void wxActiveXContainer::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    // Draw only when control is windowless or deactivated
    if (m_viewObject)
    {
        int w, h;
        GetParent()->GetSize(&w, &h);
        RECT posRect;
        posRect.left = 0;
        posRect.top = 0;
        posRect.right = w;
        posRect.bottom = h;

#if !(defined(_WIN32_WCE) && _WIN32_WCE < 400)
        ::RedrawWindow(m_oleObjectHWND, NULL, NULL, RDW_INTERNALPAINT);
#else
        ::InvalidateRect(m_oleObjectHWND, NULL, false);
#endif
        RECTL *prcBounds = (RECTL *) &posRect;
        m_viewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL,
            (HDC)dc.GetHDC(), prcBounds, NULL, NULL, 0);
    }
}

//---------------------------------------------------------------------------
// wxActiveXContainer::OnSetFocus
//
// Called when the focus is set on the parent - activates the activex control
//---------------------------------------------------------------------------
void wxActiveXContainer::OnSetFocus(wxFocusEvent& event)
{
    if (m_oleInPlaceActiveObject.Ok())
        m_oleInPlaceActiveObject->OnFrameWindowActivate(TRUE);

    event.Skip();
}

//---------------------------------------------------------------------------
// wxActiveXContainer::OnKillFocus
//
// Called when the focus is killed on the parent -
// deactivates the activex control
//---------------------------------------------------------------------------
void wxActiveXContainer::OnKillFocus(wxFocusEvent& event)
{
    if (m_oleInPlaceActiveObject.Ok())
        m_oleInPlaceActiveObject->OnFrameWindowActivate(FALSE);

    event.Skip();
}

#endif // wxUSE_ACTIVEX
