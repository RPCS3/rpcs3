///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/ole/activex.h
// Purpose:     wxActiveXContainer class
// Author:      Ryan Norton <wxprojects@comcast.net>
// Modified by:
// Created:     8/18/05
// RCS-ID:      $Id: activex.h 41793 2006-10-09 09:32:08Z ABX $
// Copyright:   (c) Ryan Norton
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Definitions
// ============================================================================

#ifndef _WX_MSW_OLE_ACTIVEXCONTAINER_H_
#define _WX_MSW_OLE_ACTIVEXCONTAINER_H_

#if wxUSE_ACTIVEX

//---------------------------------------------------------------------------
// wx includes
//---------------------------------------------------------------------------

#include "wx/msw/ole/oleutils.h" // wxBasicString &c
#include "wx/msw/ole/uuid.h"
#include "wx/window.h"
#include "wx/variant.h"

//---------------------------------------------------------------------------
// MSW COM includes
//---------------------------------------------------------------------------
#include <oleidl.h>
#include <olectl.h>

#if !defined(__WXWINCE__) || defined(__WINCE_STANDARDSDK__)
#include <exdisp.h>
#endif

#include <docobj.h>

#ifndef STDMETHOD
    #define STDMETHOD(funcname)  virtual HRESULT wxSTDCALL funcname
#endif

//
//  These defines are from another ole header - but its not in the
//  latest sdk.  Also the ifndef DISPID_READYSTATE is here because at
//  least on my machine with the latest sdk olectl.h defines these 3
//
#ifndef DISPID_READYSTATE
    #define DISPID_READYSTATE                               (-525)
    #define DISPID_READYSTATECHANGE                         (-609)
    #define DISPID_AMBIENT_TRANSFERPRIORITY                 (-728)
#endif

#define DISPID_AMBIENT_OFFLINEIFNOTCONNECTED            (-5501)
#define DISPID_AMBIENT_SILENT                           (-5502)

#ifndef DISPID_AMBIENT_CODEPAGE
    #define DISPID_AMBIENT_CODEPAGE                         (-725)
    #define DISPID_AMBIENT_CHARSET                          (-727)
#endif


//---------------------------------------------------------------------------
//
//  wxActiveXContainer
//
//---------------------------------------------------------------------------

#define WX_DECLARE_AUTOOLE(wxAutoOleInterface, I) \
class wxAutoOleInterface \
{   \
    protected: \
    I *m_interface; \
\
    public: \
    explicit wxAutoOleInterface(I *pInterface = NULL) : m_interface(pInterface) {} \
    wxAutoOleInterface(REFIID riid, IUnknown *pUnk) : m_interface(NULL) \
    {   QueryInterface(riid, pUnk); } \
    wxAutoOleInterface(REFIID riid, IDispatch *pDispatch) : m_interface(NULL) \
    {   QueryInterface(riid, pDispatch); } \
    wxAutoOleInterface(REFCLSID clsid, REFIID riid) : m_interface(NULL)\
    {   CreateInstance(clsid, riid); }\
    wxAutoOleInterface(const wxAutoOleInterface& ti) : m_interface(NULL)\
    {   operator = (ti); }\
\
    wxAutoOleInterface& operator = (const wxAutoOleInterface& ti)\
    {\
        if (ti.m_interface)\
            ti.m_interface->AddRef();\
        Free();\
        m_interface = ti.m_interface;\
        return *this;\
    }\
\
    wxAutoOleInterface& operator = (I *&ti)\
    {\
        Free();\
        m_interface = ti;\
        return *this;\
    }\
\
    ~wxAutoOleInterface() {   Free();   }\
\
    inline void Free()\
    {\
        if (m_interface)\
            m_interface->Release();\
        m_interface = NULL;\
    }\
\
    HRESULT QueryInterface(REFIID riid, IUnknown *pUnk)\
    {\
        Free();\
        wxASSERT(pUnk != NULL);\
        return pUnk->QueryInterface(riid, (void **) &m_interface);\
    }\
\
    HRESULT CreateInstance(REFCLSID clsid, REFIID riid)\
    {\
        Free();\
        return CoCreateInstance(clsid, NULL, CLSCTX_ALL, riid, (void **) &m_interface);\
    }\
\
    inline operator I *() const {return m_interface;}\
    inline I* operator ->() {return m_interface;}\
    inline I** GetRef()    {return &m_interface;}\
    inline bool Ok() const { return IsOk(); }\
    inline bool IsOk() const    {return m_interface != NULL;}\
};

WX_DECLARE_AUTOOLE(wxAutoIDispatch, IDispatch)
WX_DECLARE_AUTOOLE(wxAutoIOleClientSite, IOleClientSite)
WX_DECLARE_AUTOOLE(wxAutoIUnknown, IUnknown)
WX_DECLARE_AUTOOLE(wxAutoIOleObject, IOleObject)
WX_DECLARE_AUTOOLE(wxAutoIOleInPlaceObject, IOleInPlaceObject)
WX_DECLARE_AUTOOLE(wxAutoIOleInPlaceActiveObject, IOleInPlaceActiveObject)
WX_DECLARE_AUTOOLE(wxAutoIOleDocumentView, IOleDocumentView)
WX_DECLARE_AUTOOLE(wxAutoIViewObject, IViewObject)

class wxActiveXContainer : public wxWindow
{
public:
    wxActiveXContainer(wxWindow * parent, REFIID iid, IUnknown* pUnk);
    virtual ~wxActiveXContainer();

    void OnSize(wxSizeEvent&);
    void OnPaint(wxPaintEvent&);
    void OnSetFocus(wxFocusEvent&);
    void OnKillFocus(wxFocusEvent&);

protected:
    friend class FrameSite;
    friend class wxActiveXEvents;

    wxAutoIDispatch            m_Dispatch;
    wxAutoIOleClientSite      m_clientSite;
    wxAutoIUnknown         m_ActiveX;
    wxAutoIOleObject            m_oleObject;
    wxAutoIOleInPlaceObject    m_oleInPlaceObject;
    wxAutoIOleInPlaceActiveObject m_oleInPlaceActiveObject;
    wxAutoIOleDocumentView    m_docView;
    wxAutoIViewObject            m_viewObject;
    HWND m_oleObjectHWND;
    bool m_bAmbientUserMode;
    DWORD m_docAdviseCookie;
    wxWindow* m_realparent;

    void CreateActiveX(REFIID, IUnknown*);
};


// Events
class wxActiveXEvent : public wxCommandEvent
{
private:
    friend class wxActiveXEvents;
    wxVariant m_params;
    DISPID m_dispid;

public:
    virtual wxEvent *Clone() const
    { return new wxActiveXEvent(*this); }

    size_t ParamCount() const
    {   return m_params.GetCount();  }

    wxString ParamType(size_t idx) const
    {
        wxASSERT(idx < m_params.GetCount());
        return m_params[idx].GetType();
    }

    wxString ParamName(size_t idx) const
    {
        wxASSERT(idx < m_params.GetCount());
        return m_params[idx].GetName();
    }

    wxVariant& operator[] (size_t idx)
    {
        wxASSERT(idx < ParamCount());
        return m_params[idx];
    }

    DISPID GetDispatchId() const
    {   return m_dispid;    }
};

#define wxACTIVEX_ID    14001
DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_MEDIA, wxEVT_ACTIVEX, wxACTIVEX_ID)
typedef void (wxEvtHandler::*wxActiveXEventFunction)(wxActiveXEvent&);
#define EVT_ACTIVEX(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_ACTIVEX, id, -1, (wxObjectEventFunction) (wxEventFunction) (wxActiveXEventFunction) & fn, (wxObject *) NULL ),
#define wxActiveXEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxActiveXEventFunction, &func)

#endif // wxUSE_ACTIVEX

#endif // _WX_MSW_OLE_ACTIVEXCONTAINER_H_
