///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/ole/access.cpp
// Purpose:     implementation of wxIAccessible and wxAccessible
// Author:      Julian Smart
// Modified by:
// Created:     2003-02-12
// RCS-ID:      $Id: access.cpp 62067 2009-09-24 10:15:06Z JS $
// Copyright:   (c) 2003 Julian Smart
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
  #pragma hdrstop
#endif

#if wxUSE_OLE && wxUSE_ACCESSIBILITY

#include "wx/access.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapwin.h"
    #include "wx/window.h"
    #include "wx/log.h"
#endif

// for some compilers, the entire ole2.h must be included, not only oleauto.h
#if wxUSE_NORLANDER_HEADERS || defined(__WATCOMC__)
    #include <ole2.h>
#endif

#include <oleauto.h>
#include <oleacc.h>

#include "wx/msw/ole/oleutils.h"

#ifndef CHILDID_SELF
#define CHILDID_SELF 0
#endif

#ifndef OBJID_CLIENT
#define OBJID_CLIENT 0xFFFFFFFC
#endif

// Convert to Windows role
int wxConvertToWindowsRole(wxAccRole wxrole);

// Convert to Windows state
long wxConvertToWindowsState(long wxstate);

// Convert to Windows selection flag
int wxConvertToWindowsSelFlag(wxAccSelectionFlags sel);

// Convert from Windows selection flag
wxAccSelectionFlags wxConvertFromWindowsSelFlag(int sel);

#if wxUSE_VARIANT
// ----------------------------------------------------------------------------
// wxIEnumVARIANT interface implementation
// ----------------------------------------------------------------------------

class wxIEnumVARIANT : public IEnumVARIANT
{
public:
    wxIEnumVARIANT(const wxVariant& variant);
    virtual ~wxIEnumVARIANT() { }

    DECLARE_IUNKNOWN_METHODS;

    // IEnumVARIANT
    STDMETHODIMP Next(ULONG celt, VARIANT *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumVARIANT **ppenum);

private:
    wxVariant m_variant;  // List of further variants
    int       m_nCurrent; // Current enum position

    DECLARE_NO_COPY_CLASS(wxIEnumVARIANT)
};

// ----------------------------------------------------------------------------
// wxIEnumVARIANT
// ----------------------------------------------------------------------------

BEGIN_IID_TABLE(wxIEnumVARIANT)
    ADD_IID(Unknown)
    ADD_IID(EnumVARIANT)
END_IID_TABLE;

IMPLEMENT_IUNKNOWN_METHODS(wxIEnumVARIANT)

// wxVariant contains a list of further variants.
wxIEnumVARIANT::wxIEnumVARIANT(const wxVariant& variant)
{
    m_variant = variant;
}

STDMETHODIMP wxIEnumVARIANT::Next(ULONG      celt,
                                    VARIANT *rgelt,
                                    ULONG     *pceltFetched)
{
    wxLogTrace(wxTRACE_OleCalls, wxT("wxIEnumVARIANT::Next"));

    if ( celt > 1 ) {
        // we only return 1 element at a time - mainly because I'm too lazy to
        // implement something which you're never asked for anyhow
        return S_FALSE;
    }

    if (m_variant.GetType() != wxT("list"))
        return S_FALSE;

    if ( m_nCurrent < (int) m_variant.GetList().GetCount() ) {
        if (!wxConvertVariantToOle(m_variant[m_nCurrent++], rgelt[0]))
        {
            return S_FALSE;
        }

        // TODO: should we AddRef if this is an object?

        * pceltFetched = 1;
        return S_OK;
    }
    else {
        // bad index
        return S_FALSE;
    }
}

STDMETHODIMP wxIEnumVARIANT::Skip(ULONG celt)
{
    wxLogTrace(wxTRACE_OleCalls, wxT("wxIEnumVARIANT::Skip"));

    if (m_variant.GetType() != wxT("list"))
        return S_FALSE;

    m_nCurrent += celt;
    if ( m_nCurrent < (int) m_variant.GetList().GetCount() )
        return S_OK;

    // no, can't skip this many elements
    m_nCurrent -= celt;

    return S_FALSE;
}

STDMETHODIMP wxIEnumVARIANT::Reset()
{
    wxLogTrace(wxTRACE_OleCalls, wxT("wxIEnumVARIANT::Reset"));

    m_nCurrent = 0;

    return S_OK;
}

STDMETHODIMP wxIEnumVARIANT::Clone(IEnumVARIANT **ppenum)
{
    wxLogTrace(wxTRACE_OleCalls, wxT("wxIEnumVARIANT::Clone"));

    wxIEnumVARIANT *pNew = new wxIEnumVARIANT(m_variant);
    pNew->AddRef();
    *ppenum = pNew;

    return S_OK;
}

#endif // wxUSE_VARIANT

// ----------------------------------------------------------------------------
// wxIAccessible implementation of IAccessible interface
// ----------------------------------------------------------------------------

class wxIAccessible : public IAccessible
{
public:
    wxIAccessible(wxAccessible *pAccessible);

    // Called to indicate object should prepare to be deleted.
    void Quiesce();

    DECLARE_IUNKNOWN_METHODS;

// IAccessible

// Navigation and Hierarchy

        // Retrieves the child element or child object at a given point on the screen.
        // All visual objects support this method; sound objects do not support it.

    STDMETHODIMP accHitTest(long xLeft, long yLeft, VARIANT* pVarID);

        // Retrieves the specified object's current screen location. All visual objects must
        // support this method; sound objects do not support it.

    STDMETHODIMP accLocation ( long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varID);

        // Traverses to another user interface element within a container and retrieves the object.
        // All visual objects must support this method.

    STDMETHODIMP accNavigate ( long navDir, VARIANT varStart, VARIANT* pVarEnd);

        // Retrieves the address of an IDispatch interface for the specified child.
        // All objects must support this property.

    STDMETHODIMP get_accChild ( VARIANT varChildID, IDispatch** ppDispChild);

        // Retrieves the number of children that belong to this object.
        // All objects must support this property.

    STDMETHODIMP get_accChildCount ( long* pCountChildren);

        // Retrieves the IDispatch interface of the object's parent.
        // All objects support this property.

    STDMETHODIMP get_accParent ( IDispatch** ppDispParent);

// Descriptive Properties and Methods

        // Performs the object's default action. Not all objects have a default
        // action.

    STDMETHODIMP accDoDefaultAction(VARIANT varID);

        // Retrieves a string that describes the object's default action.
        // Not all objects have a default action.

    STDMETHODIMP get_accDefaultAction ( VARIANT varID, BSTR* pszDefaultAction);

        // Retrieves a string that describes the visual appearance of the specified object.
        // Not all objects have a description.

    STDMETHODIMP get_accDescription ( VARIANT varID, BSTR* pszDescription);

        // Retrieves an object's Help property string.
        // Not all objects support this property.

    STDMETHODIMP get_accHelp ( VARIANT varID, BSTR* pszHelp);

        // Retrieves the full path of the WinHelp file associated with the specified
        // object and the identifier of the appropriate topic within that file.
        // Not all objects support this property.

    STDMETHODIMP get_accHelpTopic ( BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);

        // Retrieves the specified object's shortcut key or access key, also known as
        // the mnemonic. All objects that have a shortcut key or access key support
        // this property.

    STDMETHODIMP get_accKeyboardShortcut ( VARIANT varID, BSTR* pszKeyboardShortcut);

        // Retrieves the name of the specified object.
        // All objects support this property.

    STDMETHODIMP get_accName ( VARIANT varID, BSTR* pszName);

        // Retrieves information that describes the role of the specified object.
        // All objects support this property.

    STDMETHODIMP get_accRole ( VARIANT varID, VARIANT* pVarRole);

        // Retrieves the current state of the specified object.
        // All objects support this property.

    STDMETHODIMP get_accState ( VARIANT varID, VARIANT* pVarState);

        // Retrieves the value of the specified object.
        // Not all objects have a value.

    STDMETHODIMP get_accValue ( VARIANT varID, BSTR* pszValue);

// Selection and Focus

        // Modifies the selection or moves the keyboard focus of the
        // specified object. All objects that select or receive the
        // keyboard focus must support this method.

    STDMETHODIMP accSelect ( long flagsSelect, VARIANT varID );

        // Retrieves the object that has the keyboard focus. All objects
        // that receive the keyboard focus must support this property.

    STDMETHODIMP get_accFocus ( VARIANT* pVarID);

        // Retrieves the selected children of this object. All objects
        // selected must support this property.

    STDMETHODIMP get_accSelection ( VARIANT * pVarChildren);

// Obsolete

    STDMETHODIMP put_accName(VARIANT WXUNUSED(varChild), BSTR WXUNUSED(szName)) { return E_FAIL; }
    STDMETHODIMP put_accValue(VARIANT WXUNUSED(varChild), BSTR WXUNUSED(szName)) { return E_FAIL; }

// IDispatch

        // Get type info

    STDMETHODIMP GetTypeInfo(unsigned int typeInfo, LCID lcid, ITypeInfo** ppTypeInfo);

        // Get type info count

    STDMETHODIMP GetTypeInfoCount(unsigned int* typeInfoCount);

        // Get ids of names

    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** names, unsigned int cNames,
        LCID lcid, DISPID* dispId);

        // Invoke

    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                        WORD wFlags, DISPPARAMS *pDispParams,
                        VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                        unsigned int *puArgErr );

// Helpers

    // Gets the standard IAccessible interface for the given child or object.
    // Call Release if this is non-NULL.
    IAccessible* GetChildStdAccessible(int id);

    // Gets the IAccessible interface for the given child or object.
    // Call Release if this is non-NULL.
    IAccessible* GetChildAccessible(int id);

private:
    wxAccessible *m_pAccessible;      // pointer to C++ class we belong to
    bool m_bQuiescing;                // Object is to be deleted

    DECLARE_NO_COPY_CLASS(wxIAccessible)
};

// ============================================================================
// Implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxIAccessible implementation
// ----------------------------------------------------------------------------
BEGIN_IID_TABLE(wxIAccessible)
  ADD_IID(Unknown)
  ADD_IID(Accessible)
  ADD_IID(Dispatch)
END_IID_TABLE;

IMPLEMENT_IUNKNOWN_METHODS(wxIAccessible)

wxIAccessible::wxIAccessible(wxAccessible *pAccessible)
{
    wxASSERT( pAccessible != NULL );

    m_pAccessible = pAccessible;
    m_bQuiescing = false;
}

// Called to indicate object should prepare to be deleted.

void wxIAccessible::Quiesce()
{
    m_bQuiescing = true;
    m_pAccessible = NULL;
}

// Retrieves the child element or child object at a given point on the screen.
// All visual objects support this method; sound objects do not support it.

STDMETHODIMP wxIAccessible::accHitTest(long xLeft, long yLeft, VARIANT* pVarID)
{
    wxLogTrace(wxT("access"), wxT("accHitTest"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    wxAccessible* childObject = NULL;
    int childId = 0;
    VariantInit(pVarID);

    wxAccStatus status = m_pAccessible->HitTest(wxPoint(xLeft, yLeft), & childId, & childObject);

    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Use standard interface instead.
        IAccessible* stdInterface = (IAccessible*)m_pAccessible->GetIAccessibleStd();
        if (!stdInterface)
            return E_NOTIMPL;
        else
            return stdInterface->accHitTest(xLeft, yLeft, pVarID);
    }

    if (childObject)
    {
        if (childObject == m_pAccessible)
        {
            pVarID->vt = VT_I4;
            pVarID->lVal = CHILDID_SELF;
            return S_OK;
        }
        else
        {
            wxIAccessible* childIA = childObject->GetIAccessible();
            if (!childIA)
                return E_NOTIMPL;

            if (childIA->QueryInterface(IID_IDispatch, (LPVOID*) & pVarID->pdispVal) != S_OK)
                return E_FAIL;

            pVarID->vt = VT_DISPATCH;
            return S_OK;
        }
    }
    else if (childId > 0)
    {
        pVarID->vt = VT_I4;
        pVarID->lVal = childId;
        return S_OK;
    }
    else
    {
        pVarID->vt = VT_EMPTY;
        return S_FALSE;
    }

    #if 0
    // all cases above already cause some return action so below line
    // is unreachable and cause unnecessary warning
    return E_NOTIMPL;
    #endif
}

// Retrieves the specified object's current screen location. All visual objects must
// support this method; sound objects do not support it.

STDMETHODIMP wxIAccessible::accLocation ( long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varID)
{
    wxLogTrace(wxT("access"), wxT("accLocation"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    wxRect rect;

    wxAccStatus status = m_pAccessible->GetLocation(rect, varID.lVal);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varID);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varID);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varID);
    }
    else
    {
        *pxLeft = rect.x;
        *pyTop = rect.y;
        *pcxWidth = rect.width;
        *pcyHeight = rect.height;
        return S_OK;
    }

    return E_NOTIMPL;
}

// Traverses to another user interface element within a container and retrieves the object.
// All visual objects must support this method.

STDMETHODIMP wxIAccessible::accNavigate ( long navDir, VARIANT varStart, VARIANT* pVarEnd)
{
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;
    wxLogTrace(wxT("access"), wxString(wxT("accNavigate for ")) + m_pAccessible->GetWindow()->GetClassInfo()->GetClassName());

    if ((varStart.vt != VT_I4 && varStart.vt != VT_EMPTY)
                                                          #if 0
                                                          // according to MSDN and sources varStart.vt is unsigned
                                                          // so below line cause warning "Condition is always false"
                                                          || varStart.vt < 0
                                                          #endif
                                                          )
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for accNavigate"));
        return E_INVALIDARG;
    }

    wxAccessible* elementObject = NULL;
    int elementId = 0;
    VariantInit(pVarEnd);
    wxNavDir navDirWX = wxNAVDIR_FIRSTCHILD;

    wxString navStr;

    switch (navDir)
    {
    case NAVDIR_DOWN:
        navDirWX = wxNAVDIR_DOWN;
        navStr = wxT("wxNAVDIR_DOWN");
        break;

    case NAVDIR_FIRSTCHILD:
        navDirWX = wxNAVDIR_FIRSTCHILD;
        navStr = wxT("wxNAVDIR_FIRSTCHILD");
        break;

    case NAVDIR_LASTCHILD:
        navDirWX = wxNAVDIR_LASTCHILD;
        navStr = wxT("wxNAVDIR_LASTCHILD");
        break;

    case NAVDIR_LEFT:
        navDirWX = wxNAVDIR_LEFT;
        navStr = wxT("wxNAVDIR_LEFT");
        break;

    case NAVDIR_NEXT:
        navDirWX = wxNAVDIR_NEXT;
        navStr = wxT("wxNAVDIR_NEXT");
        break;

    case NAVDIR_PREVIOUS:
        navDirWX = wxNAVDIR_PREVIOUS;
        navStr = wxT("wxNAVDIR_PREVIOUS");
        break;

    case NAVDIR_RIGHT:
        navDirWX = wxNAVDIR_RIGHT;
        navStr = wxT("wxNAVDIR_RIGHT");
        break;

    case NAVDIR_UP:
        navDirWX = wxNAVDIR_UP;
        navStr = wxT("wxNAVDIR_UP");
        break;
    default:
        {
            wxLogTrace(wxT("access"), wxT("Unknown NAVDIR symbol"));
            break;
        }
    }
    wxLogTrace(wxT("access"), navStr);

    wxAccStatus status = m_pAccessible->Navigate(navDirWX, varStart.lVal, & elementId,
        & elementObject);

    if (status == wxACC_FAIL)
    {
        wxLogTrace(wxT("access"), wxT("wxAccessible::Navigate failed"));
        return E_FAIL;
    }

    if (status == wxACC_FALSE)
    {
        wxLogTrace(wxT("access"), wxT("wxAccessible::Navigate found no object in this direction"));
        return S_FALSE;
    }

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        wxLogTrace(wxT("access"), wxT("Navigate not implemented"));

        // Try to use child object directly.
        if (varStart.vt == VT_I4 && varStart.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varStart.lVal);
            if (childAccessible)
            {
                varStart.lVal = 0;
                HRESULT hResult = childAccessible->accNavigate(navDir, varStart, pVarEnd);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accNavigate(navDir, varStart, pVarEnd);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accNavigate(navDir, varStart, pVarEnd);
    }
    else
    {
        if (elementObject)
        {
            wxLogTrace(wxT("access"), wxT("Getting wxIAccessible and calling QueryInterface for Navigate"));
            wxIAccessible* objectIA = elementObject->GetIAccessible();
            if (!objectIA)
            {
                wxLogTrace(wxT("access"), wxT("No wxIAccessible"));
                return E_FAIL;
            }

            HRESULT hResult = objectIA->QueryInterface(IID_IDispatch, (LPVOID*) & pVarEnd->pdispVal);
            if (hResult != S_OK)
            {
                wxLogTrace(wxT("access"), wxT("QueryInterface failed"));
                return E_FAIL;
            }

            wxLogTrace(wxT("access"), wxT("Called QueryInterface for Navigate"));
            pVarEnd->vt = VT_DISPATCH;
            return S_OK;
        }
        else if (elementId > 0)
        {
            wxLogTrace(wxT("access"), wxT("Returning element id from Navigate"));
            pVarEnd->vt = VT_I4;
            pVarEnd->lVal = elementId;
            return S_OK;
        }
        else
        {
            wxLogTrace(wxT("access"), wxT("No object in accNavigate"));
            pVarEnd->vt = VT_EMPTY;
            return S_FALSE;
        }
    }

    wxLogTrace(wxT("access"), wxT("Failing Navigate"));
    return E_NOTIMPL;
}

// Retrieves the address of an IDispatch interface for the specified child.
// All objects must support this property.

STDMETHODIMP wxIAccessible::get_accChild ( VARIANT varChildID, IDispatch** ppDispChild)
{
    wxLogTrace(wxT("access"), wxT("get_accChild"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varChildID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accChild"));
        return E_INVALIDARG;
    }

    if (varChildID.lVal == CHILDID_SELF)
    {
        *ppDispChild = this;
        AddRef();
        return S_OK;
    }

    wxAccessible* child = NULL;

    wxAccStatus status = m_pAccessible->GetChild(varChildID.lVal, & child);
    if (status == wxACC_FAIL)
    {
        wxLogTrace(wxT("access"), wxT("GetChild failed"));
        return E_FAIL;
    }

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Use standard interface instead.
        IAccessible* stdInterface = (IAccessible*)m_pAccessible->GetIAccessibleStd();
        if (!stdInterface)
            return E_NOTIMPL;
        else
        {
            wxLogTrace(wxT("access"), wxT("Using standard interface for get_accChild"));
            return stdInterface->get_accChild (varChildID, ppDispChild);
        }
    }
    else
    {
        if (child)
        {
            wxIAccessible* objectIA = child->GetIAccessible();
            if (!objectIA)
                return E_NOTIMPL;

            if (objectIA->QueryInterface(IID_IDispatch, (LPVOID*) ppDispChild) != S_OK)
            {
                wxLogTrace(wxT("access"), wxT("QueryInterface failed in get_accChild"));
                return E_FAIL;
            }

            return S_OK;
        }
        else
        {
            wxLogTrace(wxT("access"), wxT("Not an accessible object"));
            return S_FALSE; // Indicates it's not an accessible object
        }
    }

    #if 0
    // all cases above already cause some return action so below line
    // is unreachable and cause unnecessary warning
    return E_NOTIMPL;
    #endif
}

// Retrieves the number of children that belong to this object.
// All objects must support this property.

STDMETHODIMP wxIAccessible::get_accChildCount ( long* pCountChildren)
{
    wxLogTrace(wxT("access"), wxT("get_accChildCount"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    int childCount = 0;
    wxAccStatus status = m_pAccessible->GetChildCount(& childCount);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Use standard interface instead.
        IAccessible* stdInterface = (IAccessible*)m_pAccessible->GetIAccessibleStd();
        if (!stdInterface)
            return E_NOTIMPL;
        else
        {
            wxLogTrace(wxT("access"), wxT("Using standard interface for get_accChildCount"));
            HRESULT res = stdInterface->get_accChildCount (pCountChildren);
            wxString str;
            str.Printf(wxT("Number of children was %d"), (int) (*pCountChildren));
            wxLogTrace(wxT("access"), str);
            return res;
        }
    }
    else
    {
        * pCountChildren = (long) childCount;
        return S_OK;
    }

    #if 0
    // all cases above already cause some return action so below line
    // is unreachable and cause unnecessary warning
    return E_NOTIMPL;
    #endif
}

// Retrieves the IDispatch interface of the object's parent.
// All objects support this property.

STDMETHODIMP wxIAccessible::get_accParent ( IDispatch** ppDispParent)
{
    wxLogTrace(wxT("access"), wxT("get_accParent"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    wxAccessible* parent = NULL;
    wxAccStatus status = m_pAccessible->GetParent(& parent);

    if (status == wxACC_FAIL)
        return E_FAIL;

    // It doesn't seem acceptable to return S_FALSE with a NULL
    // ppDispParent, so if we have no wxWidgets parent, we leave
    // it to the standard interface.
    if (status == wxACC_NOT_IMPLEMENTED || !parent)
    {
        wxLogTrace(wxT("access"), wxT("Using standard interface to get the parent."));
        // Use standard interface instead.
        IAccessible* stdInterface = (IAccessible*)m_pAccessible->GetIAccessibleStd();
        if (!stdInterface)
            return E_NOTIMPL;
        else
            return stdInterface->get_accParent (ppDispParent);
    }
    else
    {
        if (parent)
        {
            wxIAccessible* objectIA = parent->GetIAccessible();
            if (!objectIA)
                return E_FAIL;

            wxLogTrace(wxT("access"), wxT("About to call QueryInterface"));
            if (objectIA->QueryInterface(IID_IDispatch, (LPVOID*) ppDispParent) != S_OK)
            {
                wxLogTrace(wxT("access"), wxT("Failed QueryInterface"));
                return E_FAIL;
            }

            wxLogTrace(wxT("access"), wxT("Returning S_OK for get_accParent"));
            return S_OK;
        }
        else
        {
            // This doesn't seem to be allowed, despite the documentation,
            // so we handle it higher up by using the standard interface.
            wxLogTrace(wxT("access"), wxT("Returning NULL parent because there was none"));
            *ppDispParent = NULL;
            return S_FALSE;
        }
    }

    #if 0
    // all cases above already cause some return action so below line
    // is unreachable and cause unnecessary warning
    return E_NOTIMPL;
    #endif
}

// Performs the object's default action. Not all objects have a default
// action.

STDMETHODIMP wxIAccessible::accDoDefaultAction(VARIANT varID)
{
    wxLogTrace(wxT("access"), wxT("accDoDefaultAction"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for accDoDefaultAction"));
        return E_INVALIDARG;
    }

    wxAccStatus status = m_pAccessible->DoDefaultAction(varID.lVal);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_SUPPORTED)
        return DISP_E_MEMBERNOTFOUND;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->accDoDefaultAction(varID);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accDoDefaultAction(varID);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accDoDefaultAction(varID);
    }
    return E_FAIL;
}

// Retrieves a string that describes the object's default action.
// Not all objects have a default action.

STDMETHODIMP wxIAccessible::get_accDefaultAction ( VARIANT varID, BSTR* pszDefaultAction)
{
    wxLogTrace(wxT("access"), wxT("get_accDefaultAction"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accDefaultAction"));
        return E_INVALIDARG;
    }

    wxString defaultAction;
    wxAccStatus status = m_pAccessible->GetDefaultAction(varID.lVal, & defaultAction);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_SUPPORTED)
        return DISP_E_MEMBERNOTFOUND;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accDefaultAction(varID, pszDefaultAction);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accDefaultAction(varID, pszDefaultAction);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accDefaultAction(varID, pszDefaultAction);
    }
    else
    {
        if (defaultAction.IsEmpty())
        {
            * pszDefaultAction = NULL;
            return S_FALSE;
        }
        else
        {
            wxBasicString basicString(defaultAction);
            * pszDefaultAction = basicString.Get();
            return S_OK;
        }
    }
    return E_FAIL;
}

// Retrieves a string that describes the visual appearance of the specified object.
// Not all objects have a description.

STDMETHODIMP wxIAccessible::get_accDescription ( VARIANT varID, BSTR* pszDescription)
{
    wxLogTrace(wxT("access"), wxT("get_accDescription"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accDescription"));
        return E_INVALIDARG;
    }

    wxString description;
    wxAccStatus status = m_pAccessible->GetDescription(varID.lVal, & description);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accDescription(varID, pszDescription);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accDescription(varID, pszDescription);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accDescription(varID, pszDescription);
    }
    else
    {
        if (description.empty())
        {
            * pszDescription = NULL;
            return S_FALSE;
        }
        else
        {
            wxBasicString basicString(description);
            * pszDescription = basicString.Get();
            return S_OK;
        }
    }
    return E_NOTIMPL;
}

// Retrieves an object's Help property string.
// Not all objects support this property.

STDMETHODIMP wxIAccessible::get_accHelp ( VARIANT varID, BSTR* pszHelp)
{
    wxLogTrace(wxT("access"), wxT("get_accHelp"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accHelp"));
        return E_INVALIDARG;
    }

    wxString helpString;
    wxAccStatus status = m_pAccessible->GetHelpText(varID.lVal, & helpString);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accHelp(varID, pszHelp);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accHelp(varID, pszHelp);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accHelp (varID, pszHelp);
    }
    else
    {
        if (helpString.empty())
        {
            * pszHelp = NULL;
            return S_FALSE;
        }
        else
        {
            wxBasicString basicString(helpString);
            * pszHelp = basicString.Get();
            return S_OK;
        }
    }
    return E_NOTIMPL;
}

// Retrieves the full path of the WinHelp file associated with the specified
// object and the identifier of the appropriate topic within that file.
// Not all objects support this property.
// NOTE: not supported by wxWidgets at this time. Use
// GetHelpText instead.

STDMETHODIMP wxIAccessible::get_accHelpTopic ( BSTR* pszHelpFile, VARIANT varChild, long* pidTopic)
{
    wxLogTrace(wxT("access"), wxT("get_accHelpTopic"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varChild.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accHelpTopic"));
        return E_INVALIDARG;
    }

    wxAccStatus status = wxACC_NOT_IMPLEMENTED;
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varChild.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varChild.lVal);
            if (childAccessible)
            {
                varChild.lVal = 0;
                HRESULT hResult = childAccessible->get_accHelpTopic(pszHelpFile, varChild, pidTopic);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accHelpTopic(pszHelpFile, varChild, pidTopic);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accHelpTopic (pszHelpFile, varChild, pidTopic);
    }
    return E_NOTIMPL;
}

// Retrieves the specified object's shortcut key or access key, also known as
// the mnemonic. All objects that have a shortcut key or access key support
// this property.

STDMETHODIMP wxIAccessible::get_accKeyboardShortcut ( VARIANT varID, BSTR* pszKeyboardShortcut)
{
    wxLogTrace(wxT("access"), wxT("get_accKeyboardShortcut"));
    *pszKeyboardShortcut = NULL;

    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accKeyboardShortcut"));
        return E_INVALIDARG;
    }

    wxString keyboardShortcut;
    wxAccStatus status = m_pAccessible->GetKeyboardShortcut(varID.lVal, & keyboardShortcut);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accKeyboardShortcut(varID, pszKeyboardShortcut);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accKeyboardShortcut(varID, pszKeyboardShortcut);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accKeyboardShortcut (varID, pszKeyboardShortcut);
    }
    else
    {
        if (keyboardShortcut.empty())
        {
            * pszKeyboardShortcut = NULL;
            return S_FALSE;
        }
        else
        {
            wxBasicString basicString(keyboardShortcut);
            * pszKeyboardShortcut = basicString.Get();
            return S_OK;
        }
    }
    return E_NOTIMPL;
}

// Retrieves the name of the specified object.
// All objects support this property.

STDMETHODIMP wxIAccessible::get_accName ( VARIANT varID, BSTR* pszName)
{
    wxLogTrace(wxT("access"), wxT("get_accName"));
    *pszName = NULL;

    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accName"));
        return E_INVALIDARG;
    }

    wxString name;

    wxAccStatus status = m_pAccessible->GetName(varID.lVal, & name);

    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accName(varID, pszName);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accName(varID, pszName);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accName (varID, pszName);
    }
    else
    {
        wxBasicString basicString(name);
        *pszName = basicString.Get();
        return S_OK;
    }
    return E_NOTIMPL;
}

// Retrieves information that describes the role of the specified object.
// All objects support this property.

STDMETHODIMP wxIAccessible::get_accRole ( VARIANT varID, VARIANT* pVarRole)
{
    wxLogTrace(wxT("access"), wxT("get_accRole"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accRole"));
        return E_INVALIDARG;
    }

    VariantInit(pVarRole);

    wxAccRole role = wxROLE_NONE;

    wxAccStatus status = m_pAccessible->GetRole(varID.lVal, & role);

    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accRole(varID, pVarRole);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accRole(varID, pVarRole);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accRole (varID, pVarRole);
    }
    else
    {
        if (role == wxROLE_NONE)
        {
            pVarRole->vt = VT_EMPTY;
            return S_OK;
        }

        pVarRole->lVal = wxConvertToWindowsRole(role);
        pVarRole->vt = VT_I4;

        return S_OK;
    }
    return E_NOTIMPL;
}

// Retrieves the current state of the specified object.
// All objects support this property.

STDMETHODIMP wxIAccessible::get_accState ( VARIANT varID, VARIANT* pVarState)
{
    wxLogTrace(wxT("access"), wxT("get_accState"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4 && varID.vt != VT_EMPTY)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accState"));
        return E_INVALIDARG;
    }

    long wxstate = 0;

    wxAccStatus status = m_pAccessible->GetState(varID.lVal, & wxstate);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accState(varID, pVarState);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accState(varID, pVarState);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accState (varID, pVarState);
    }
    else
    {
        long state = wxConvertToWindowsState(wxstate);
        pVarState->lVal = state;
        pVarState->vt = VT_I4;
        return S_OK;
    }
    return E_NOTIMPL;
}

// Retrieves the value of the specified object.
// Not all objects have a value.

STDMETHODIMP wxIAccessible::get_accValue ( VARIANT varID, BSTR* pszValue)
{
    wxLogTrace(wxT("access"), wxT("get_accValue"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for get_accValue"));
        return E_INVALIDARG;
    }

    wxString strValue;

    wxAccStatus status = m_pAccessible->GetValue(varID.lVal, & strValue);

    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->get_accValue(varID, pszValue);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accValue(varID, pszValue);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->get_accValue (varID, pszValue);
    }
    else
    {
        wxBasicString basicString(strValue);
        * pszValue = basicString.Get();
        return S_OK;
    }
    return E_NOTIMPL;
}

// Modifies the selection or moves the keyboard focus of the
// specified object. All objects that select or receive the
// keyboard focus must support this method.

STDMETHODIMP wxIAccessible::accSelect ( long flagsSelect, VARIANT varID )
{
    wxLogTrace(wxT("access"), wxT("get_accSelect"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    if (varID.vt != VT_I4 && varID.vt != VT_EMPTY)
    {
        wxLogTrace(wxT("access"), wxT("Invalid arg for accSelect"));
        return E_INVALIDARG;
    }

    wxAccSelectionFlags wxsel = wxConvertFromWindowsSelFlag(flagsSelect);

    wxAccStatus status = m_pAccessible->Select(varID.lVal, wxsel);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Try to use child object directly.
        if (varID.lVal > 0 && varID.lVal > 0)
        {
            IAccessible* childAccessible = GetChildAccessible(varID.lVal);
            if (childAccessible)
            {
                varID.lVal = 0;
                HRESULT hResult = childAccessible->accSelect(flagsSelect, varID);
                childAccessible->Release();
                return hResult;
            }
            else if (m_pAccessible->GetIAccessibleStd())
                return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accSelect(flagsSelect, varID);
        }
        else if (m_pAccessible->GetIAccessibleStd())
            return ((IAccessible*) m_pAccessible->GetIAccessibleStd())->accSelect(flagsSelect, varID);
    }
    else
        return S_OK;

    return E_NOTIMPL;
}

// Retrieves the object that has the keyboard focus. All objects
// that receive the keyboard focus must support this property.

STDMETHODIMP wxIAccessible::get_accFocus ( VARIANT* pVarID)
{
    wxLogTrace(wxT("access"), wxT("get_accFocus"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    wxAccessible* childObject = NULL;
    int childId = 0;
    VariantInit(pVarID);

    wxAccStatus status = m_pAccessible->GetFocus(& childId, & childObject);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Use standard interface instead.
        IAccessible* stdInterface = (IAccessible*)m_pAccessible->GetIAccessibleStd();
        if (!stdInterface)
            return E_NOTIMPL;
        else
            return stdInterface->get_accFocus (pVarID);
    }
    if (childObject)
    {
        if (childObject == m_pAccessible)
        {
            pVarID->vt = VT_I4;
            pVarID->lVal = CHILDID_SELF;
            return S_OK;        }
        else
        {
            wxIAccessible* childIA = childObject->GetIAccessible();
            if (!childIA)
                return E_NOTIMPL;

            if (childIA->QueryInterface(IID_IDispatch, (LPVOID*) & pVarID->pdispVal) != S_OK)
                return E_FAIL;

            pVarID->vt = VT_DISPATCH;
            return S_OK;
        }
    }
    else if (childId > 0)
    {
        pVarID->vt = VT_I4;
        pVarID->lVal = childId;
        return S_OK;
    }
    else
    {
        pVarID->vt = VT_EMPTY;
        return S_FALSE;
    }

    #if 0
    // all cases above already cause some return action so below line
    // is unreachable and cause unnecessary warning
    return E_NOTIMPL;
    #endif
}

// Retrieves the selected children of this object. All objects
// selected must support this property.

STDMETHODIMP wxIAccessible::get_accSelection ( VARIANT * pVarChildren)
{
#if wxUSE_VARIANT
    wxLogTrace(wxT("access"), wxT("get_accSelection"));
    wxASSERT( ( m_pAccessible != NULL ) || ( m_bQuiescing == true ) );
    if (!m_pAccessible)
        return E_FAIL;

    VariantInit(pVarChildren);

    wxVariant selections;
    wxAccStatus status = m_pAccessible->GetSelections(& selections);
    if (status == wxACC_FAIL)
        return E_FAIL;

    if (status == wxACC_NOT_IMPLEMENTED)
    {
        // Use standard interface instead.
        IAccessible* stdInterface = (IAccessible*)m_pAccessible->GetIAccessibleStd();
        if (!stdInterface)
            return E_NOTIMPL;
        else
            return stdInterface->get_accSelection (pVarChildren);
    }
    else
    {
        if (selections.GetType() == wxT("long"))
        {
            pVarChildren->vt = VT_I4;
            pVarChildren->lVal = selections.GetLong();

            return S_OK;
        }
        else if (selections.GetType() == wxT("void*"))
        {
            wxAccessible* childObject = (wxAccessible*) selections.GetVoidPtr();
            wxIAccessible* childIA = childObject->GetIAccessible();
            if (!childIA)
                return E_NOTIMPL;

            if (childIA->QueryInterface(IID_IDispatch, (LPVOID*) & pVarChildren->pdispVal) != S_OK)
                return E_FAIL;

            pVarChildren->vt = VT_DISPATCH;

            return S_OK;
        }
        else if (selections.GetType() == wxT("list"))
        {
            // TODO: should we AddRef for every "void*" member??

            wxIEnumVARIANT* enumVariant = new wxIEnumVARIANT(selections);
            enumVariant->AddRef();

            pVarChildren->vt = VT_UNKNOWN;
            pVarChildren->punkVal = enumVariant;

            return S_OK;
        }
    }
#else
    wxUnusedVar(pVarChildren);
#endif // wxUSE_VARIANT

    return E_NOTIMPL;
}

// Get type info

STDMETHODIMP wxIAccessible::GetTypeInfo(unsigned int WXUNUSED(typeInfo), LCID WXUNUSED(lcid), ITypeInfo** ppTypeInfo)
{
    *ppTypeInfo = NULL;
    return E_NOTIMPL;
}

// Get type info count

STDMETHODIMP wxIAccessible::GetTypeInfoCount(unsigned int* typeInfoCount)
{
    *typeInfoCount = 0;
    return E_NOTIMPL;
}

// Get ids of names

STDMETHODIMP wxIAccessible::GetIDsOfNames(REFIID WXUNUSED(riid), OLECHAR** WXUNUSED(names), unsigned int WXUNUSED(cNames),
        LCID WXUNUSED(lcid), DISPID* WXUNUSED(dispId))
{
    return E_NOTIMPL;
}

// Invoke

STDMETHODIMP wxIAccessible::Invoke(DISPID WXUNUSED(dispIdMember), REFIID WXUNUSED(riid), LCID WXUNUSED(lcid),
                        WORD WXUNUSED(wFlags), DISPPARAMS *WXUNUSED(pDispParams),
                        VARIANT *WXUNUSED(pVarResult), EXCEPINFO *WXUNUSED(pExcepInfo),
                        unsigned int *WXUNUSED(puArgErr) )
{
    return E_NOTIMPL;
}

// Gets the standard IAccessible interface for the given child or object.
// Call Release if this is non-NULL.
IAccessible* wxIAccessible::GetChildStdAccessible(int id)
{
    if (id == 0)
    {
        IAccessible* obj = (IAccessible*)m_pAccessible->GetIAccessibleStd();

        obj->AddRef();
        return obj;
    }
    else
    {
        VARIANT var;
        VariantInit(& var);
        var.vt = VT_I4;
        var.lVal = id;
        IDispatch* pDispatch = NULL;
        if (S_OK == get_accChild ( var, & pDispatch))
        {
            IAccessible* childAccessible = NULL;
            if (pDispatch->QueryInterface(IID_IAccessible, (LPVOID*) & childAccessible) == S_OK)
            {
                pDispatch->Release();
                wxIAccessible* c = (wxIAccessible*) childAccessible;
                IAccessible* stdChildAccessible = (IAccessible*) c->m_pAccessible->GetIAccessibleStd();
                stdChildAccessible->AddRef();
                childAccessible->Release();
                return stdChildAccessible;
            }
            else
            {
                pDispatch->Release();
            }
        }
    }

#if 0
    {
        // Loop until we find the right id
        long nChildren = 0;
        this->get_accChildCount(& nChildren);

        int i;
        for (i = 0; i < nChildren; i++)
        {
            long obtained = 0;
            VARIANT var;
            VariantInit(& var);
            var.vt = VT_I4;
            if (S_OK == AccessibleChildren(this, i, 1, & var, &obtained))
            {
                if (var.lVal == id)
                {
                    VariantInit(& var);
                    var.vt = VT_DISPATCH;
                    if (S_OK == AccessibleChildren(this, i, 1, & var, &obtained))
                    {
                        IAccessible* childAccessible = NULL;
                        if (var.pdispVal->QueryInterface(IID_IAccessible, (LPVOID*) & childAccessible) == S_OK)
                        {
                            var.pdispVal->Release();
                            return childAccessible;
                        }
                        else
                        {
                            var.pdispVal->Release();
                        }
                    }
                }
                break;
            }
        }
    }
#endif
    return NULL;
}

// Gets the IAccessible interface for the given child or object.
// Call Release if this is non-NULL.
IAccessible* wxIAccessible::GetChildAccessible(int id)
{
    if (id == 0)
    {
        IAccessible* obj = this;

        obj->AddRef();
        return obj;
    }
    else
    {
        VARIANT var;
        VariantInit(& var);
        var.vt = VT_I4;
        var.lVal = id;
        IDispatch* pDispatch = NULL;
        if (S_OK == get_accChild ( var, & pDispatch))
        {
            IAccessible* childAccessible = NULL;
            if (pDispatch->QueryInterface(IID_IAccessible, (LPVOID*) & childAccessible) == S_OK)
            {
                pDispatch->Release();
                return childAccessible;
            }
            else
            {
                pDispatch->Release();
            }
        }
    }
    return NULL;
}

// ----------------------------------------------------------------------------
// wxAccessible implementation
// ----------------------------------------------------------------------------

// ctors

// common part of all ctors
void wxAccessible::Init()
{
    m_pIAccessibleStd = NULL;
    m_pIAccessible = new wxIAccessible(this);
    m_pIAccessible->AddRef();
}

wxAccessible::wxAccessible(wxWindow* win)
            : wxAccessibleBase(win)
{
    Init();
}

wxAccessible::~wxAccessible()
{
    m_pIAccessible->Quiesce();
    m_pIAccessible->Release();
    if (m_pIAccessibleStd)
        ((IAccessible*)m_pIAccessibleStd)->Release();
}

// Gets or creates a standard interface for this object.
void* wxAccessible::GetIAccessibleStd()
{
    if (m_pIAccessibleStd)
        return m_pIAccessibleStd;

    if (GetWindow())
    {
        HRESULT retCode = ::CreateStdAccessibleObject((HWND) GetWindow()->GetHWND(),
                OBJID_CLIENT, IID_IAccessible, (void**) & m_pIAccessibleStd);
        if (retCode == S_OK)
            return m_pIAccessibleStd;
        else
        {
            m_pIAccessibleStd = NULL;
            return NULL;
        }
    }
    return NULL;
}

// Sends an event when something changes in an accessible object.
void wxAccessible::NotifyEvent(int eventType, wxWindow* window, wxAccObject objectType,
                        int objectId)
{
    ::NotifyWinEvent((DWORD) eventType, (HWND) window->GetHWND(),
        (LONG) objectType, (LONG) objectId);
}

// Utilities

// Convert to Windows role
int wxConvertToWindowsRole(wxAccRole wxrole)
{
    switch (wxrole)
    {
    case wxROLE_NONE:
        return 0;
    case wxROLE_SYSTEM_ALERT:
        return ROLE_SYSTEM_ALERT;
    case wxROLE_SYSTEM_ANIMATION:
        return ROLE_SYSTEM_ANIMATION;
    case wxROLE_SYSTEM_APPLICATION:
        return ROLE_SYSTEM_APPLICATION;
    case wxROLE_SYSTEM_BORDER:
        return ROLE_SYSTEM_BORDER;
    case wxROLE_SYSTEM_BUTTONDROPDOWN:
        return ROLE_SYSTEM_BUTTONDROPDOWN;
    case wxROLE_SYSTEM_BUTTONDROPDOWNGRID:
        return ROLE_SYSTEM_BUTTONDROPDOWNGRID;
    case wxROLE_SYSTEM_BUTTONMENU:
        return ROLE_SYSTEM_BUTTONMENU;
    case wxROLE_SYSTEM_CARET:
        return ROLE_SYSTEM_CARET;
    case wxROLE_SYSTEM_CELL:
        return ROLE_SYSTEM_CELL;
    case wxROLE_SYSTEM_CHARACTER:
        return ROLE_SYSTEM_CHARACTER;
    case wxROLE_SYSTEM_CHART:
        return ROLE_SYSTEM_CHART;
    case wxROLE_SYSTEM_CHECKBUTTON:
        return ROLE_SYSTEM_CHECKBUTTON;
    case wxROLE_SYSTEM_CLIENT:
        return ROLE_SYSTEM_CLIENT;
    case wxROLE_SYSTEM_CLOCK:
        return ROLE_SYSTEM_CLOCK;
    case wxROLE_SYSTEM_COLUMN:
        return ROLE_SYSTEM_COLUMN;
    case wxROLE_SYSTEM_COLUMNHEADER:
        return ROLE_SYSTEM_COLUMNHEADER;
    case wxROLE_SYSTEM_COMBOBOX:
        return ROLE_SYSTEM_COMBOBOX;
    case wxROLE_SYSTEM_CURSOR:
        return ROLE_SYSTEM_CURSOR;
    case wxROLE_SYSTEM_DIAGRAM:
        return ROLE_SYSTEM_DIAGRAM;
    case wxROLE_SYSTEM_DIAL:
        return ROLE_SYSTEM_DIAL;
    case wxROLE_SYSTEM_DIALOG:
        return ROLE_SYSTEM_DIALOG;
    case wxROLE_SYSTEM_DOCUMENT:
        return ROLE_SYSTEM_DOCUMENT;
    case wxROLE_SYSTEM_DROPLIST:
        return ROLE_SYSTEM_DROPLIST;
    case wxROLE_SYSTEM_EQUATION:
        return ROLE_SYSTEM_EQUATION;
    case wxROLE_SYSTEM_GRAPHIC:
        return ROLE_SYSTEM_GRAPHIC;
    case wxROLE_SYSTEM_GRIP:
        return ROLE_SYSTEM_GRIP;
    case wxROLE_SYSTEM_GROUPING:
        return ROLE_SYSTEM_GROUPING;
    case wxROLE_SYSTEM_HELPBALLOON:
        return ROLE_SYSTEM_HELPBALLOON;
    case wxROLE_SYSTEM_HOTKEYFIELD:
        return ROLE_SYSTEM_HOTKEYFIELD;
    case wxROLE_SYSTEM_INDICATOR:
        return ROLE_SYSTEM_INDICATOR;
    case wxROLE_SYSTEM_LINK:
        return ROLE_SYSTEM_LINK;
    case wxROLE_SYSTEM_LIST:
        return ROLE_SYSTEM_LIST;
    case wxROLE_SYSTEM_LISTITEM:
        return ROLE_SYSTEM_LISTITEM;
    case wxROLE_SYSTEM_MENUBAR:
        return ROLE_SYSTEM_MENUBAR;
    case wxROLE_SYSTEM_MENUITEM:
        return ROLE_SYSTEM_MENUITEM;
    case wxROLE_SYSTEM_MENUPOPUP:
        return ROLE_SYSTEM_MENUPOPUP;
    case wxROLE_SYSTEM_OUTLINE:
        return ROLE_SYSTEM_OUTLINE;
    case wxROLE_SYSTEM_OUTLINEITEM:
        return ROLE_SYSTEM_OUTLINEITEM;
    case wxROLE_SYSTEM_PAGETAB:
        return ROLE_SYSTEM_PAGETAB;
    case wxROLE_SYSTEM_PAGETABLIST:
        return ROLE_SYSTEM_PAGETABLIST;
    case wxROLE_SYSTEM_PANE:
        return ROLE_SYSTEM_PANE;
    case wxROLE_SYSTEM_PROGRESSBAR:
        return ROLE_SYSTEM_PROGRESSBAR;
    case wxROLE_SYSTEM_PROPERTYPAGE:
        return ROLE_SYSTEM_PROPERTYPAGE;
    case wxROLE_SYSTEM_PUSHBUTTON:
        return ROLE_SYSTEM_PUSHBUTTON;
    case wxROLE_SYSTEM_RADIOBUTTON:
        return ROLE_SYSTEM_RADIOBUTTON;
    case wxROLE_SYSTEM_ROW:
        return ROLE_SYSTEM_ROW;
    case wxROLE_SYSTEM_ROWHEADER:
        return ROLE_SYSTEM_ROWHEADER;
    case wxROLE_SYSTEM_SCROLLBAR:
        return ROLE_SYSTEM_SCROLLBAR;
    case wxROLE_SYSTEM_SEPARATOR:
        return ROLE_SYSTEM_SEPARATOR;
    case wxROLE_SYSTEM_SLIDER:
        return ROLE_SYSTEM_SLIDER;
    case wxROLE_SYSTEM_SOUND:
        return ROLE_SYSTEM_SOUND;
    case wxROLE_SYSTEM_SPINBUTTON:
        return ROLE_SYSTEM_SPINBUTTON;
    case wxROLE_SYSTEM_STATICTEXT:
        return ROLE_SYSTEM_STATICTEXT;
    case wxROLE_SYSTEM_STATUSBAR:
        return ROLE_SYSTEM_STATUSBAR;
    case wxROLE_SYSTEM_TABLE:
        return ROLE_SYSTEM_TABLE;
    case wxROLE_SYSTEM_TEXT:
        return ROLE_SYSTEM_TEXT;
    case wxROLE_SYSTEM_TITLEBAR:
        return ROLE_SYSTEM_TITLEBAR;
    case wxROLE_SYSTEM_TOOLBAR:
        return ROLE_SYSTEM_TOOLBAR;
    case wxROLE_SYSTEM_TOOLTIP:
        return ROLE_SYSTEM_TOOLTIP;
    case wxROLE_SYSTEM_WHITESPACE:
        return ROLE_SYSTEM_WHITESPACE;
    case wxROLE_SYSTEM_WINDOW:
        return ROLE_SYSTEM_WINDOW;
    }
    return 0;
}

// Convert to Windows state
long wxConvertToWindowsState(long wxstate)
{
    long state = 0;
    if (wxstate & wxACC_STATE_SYSTEM_ALERT_HIGH)
        state |= STATE_SYSTEM_ALERT_HIGH;

    if (wxstate & wxACC_STATE_SYSTEM_ALERT_MEDIUM)
        state |= STATE_SYSTEM_ALERT_MEDIUM;

    if (wxstate & wxACC_STATE_SYSTEM_ALERT_LOW)
        state |= STATE_SYSTEM_ALERT_LOW;

    if (wxstate & wxACC_STATE_SYSTEM_ANIMATED)
        state |= STATE_SYSTEM_ANIMATED;

    if (wxstate & wxACC_STATE_SYSTEM_BUSY)
        state |= STATE_SYSTEM_BUSY;

    if (wxstate & wxACC_STATE_SYSTEM_CHECKED)
        state |= STATE_SYSTEM_CHECKED;

    if (wxstate & wxACC_STATE_SYSTEM_COLLAPSED)
        state |= STATE_SYSTEM_COLLAPSED;

    if (wxstate & wxACC_STATE_SYSTEM_DEFAULT)
        state |= STATE_SYSTEM_DEFAULT;

    if (wxstate & wxACC_STATE_SYSTEM_EXPANDED)
        state |= STATE_SYSTEM_EXPANDED;

    if (wxstate & wxACC_STATE_SYSTEM_EXTSELECTABLE)
        state |= STATE_SYSTEM_EXTSELECTABLE;

    if (wxstate & wxACC_STATE_SYSTEM_FLOATING)
        state |= STATE_SYSTEM_FLOATING;

    if (wxstate & wxACC_STATE_SYSTEM_FOCUSABLE)
        state |= STATE_SYSTEM_FOCUSABLE;

    if (wxstate & wxACC_STATE_SYSTEM_FOCUSED)
        state |= STATE_SYSTEM_FOCUSED;

    if (wxstate & wxACC_STATE_SYSTEM_HOTTRACKED)
        state |= STATE_SYSTEM_HOTTRACKED;

    if (wxstate & wxACC_STATE_SYSTEM_INVISIBLE)
        state |= STATE_SYSTEM_INVISIBLE;

    if (wxstate & wxACC_STATE_SYSTEM_INVISIBLE)
        state |= STATE_SYSTEM_INVISIBLE;

    if (wxstate & wxACC_STATE_SYSTEM_MIXED)
        state |= STATE_SYSTEM_MIXED;

    if (wxstate & wxACC_STATE_SYSTEM_MULTISELECTABLE)
        state |= STATE_SYSTEM_MULTISELECTABLE;

    if (wxstate & wxACC_STATE_SYSTEM_OFFSCREEN)
        state |= STATE_SYSTEM_OFFSCREEN;

    if (wxstate & wxACC_STATE_SYSTEM_PRESSED)
        state |= STATE_SYSTEM_PRESSED;

//    if (wxstate & wxACC_STATE_SYSTEM_PROTECTED)
//        state |= STATE_SYSTEM_PROTECTED;

    if (wxstate & wxACC_STATE_SYSTEM_READONLY)
        state |= STATE_SYSTEM_READONLY;

    if (wxstate & wxACC_STATE_SYSTEM_SELECTABLE)
        state |= STATE_SYSTEM_SELECTABLE;

    if (wxstate & wxACC_STATE_SYSTEM_SELECTED)
        state |= STATE_SYSTEM_SELECTED;

    if (wxstate & wxACC_STATE_SYSTEM_SELFVOICING)
        state |= STATE_SYSTEM_SELFVOICING;

    if (wxstate & wxACC_STATE_SYSTEM_UNAVAILABLE)
        state |= STATE_SYSTEM_UNAVAILABLE;

    return state;
}

// Convert to Windows selection flag
int wxConvertToWindowsSelFlag(wxAccSelectionFlags wxsel)
{
    int sel = 0;

    if (wxsel & wxACC_SEL_TAKEFOCUS)
        sel |= SELFLAG_TAKEFOCUS;
    if (wxsel & wxACC_SEL_TAKESELECTION)
        sel |= SELFLAG_TAKESELECTION;
    if (wxsel & wxACC_SEL_EXTENDSELECTION)
        sel |= SELFLAG_EXTENDSELECTION;
    if (wxsel & wxACC_SEL_ADDSELECTION)
        sel |= SELFLAG_ADDSELECTION;
    if (wxsel & wxACC_SEL_REMOVESELECTION)
        sel |= SELFLAG_REMOVESELECTION;
    return sel;
}

// Convert from Windows selection flag
wxAccSelectionFlags wxConvertFromWindowsSelFlag(int sel)
{
    int wxsel = 0;

    if (sel & SELFLAG_TAKEFOCUS)
        wxsel |= wxACC_SEL_TAKEFOCUS;
    if (sel & SELFLAG_TAKESELECTION)
        wxsel |= wxACC_SEL_TAKESELECTION;
    if (sel & SELFLAG_EXTENDSELECTION)
        wxsel |= wxACC_SEL_EXTENDSELECTION;
    if (sel & SELFLAG_ADDSELECTION)
        wxsel |= wxACC_SEL_ADDSELECTION;
    if (sel & SELFLAG_REMOVESELECTION)
        wxsel |= wxACC_SEL_REMOVESELECTION;
    return (wxAccSelectionFlags) wxsel;
}


#endif  // wxUSE_OLE && wxUSE_ACCESSIBILITY
