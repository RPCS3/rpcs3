/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/ole/automtn.cpp
// Purpose:     OLE automation utilities
// Author:      Julian Smart
// Modified by:
// Created:     11/6/98
// RCS-ID:      $Id: automtn.cpp 66913 2011-02-16 21:40:07Z JS $
// Copyright:   (c) 1998, Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

// With Borland C++, all samples crash if this is compiled in.
#if (defined(__BORLANDC__) && (__BORLANDC__ < 0x520)) || defined(__CYGWIN10__)
    #undef wxUSE_OLE_AUTOMATION
    #define wxUSE_OLE_AUTOMATION 0
#endif

#if wxUSE_OLE_AUTOMATION

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/math.h"
#endif

#define _FORCENAMELESSUNION
#include "wx/msw/private.h"
#include "wx/msw/ole/oleutils.h"
#include "wx/msw/ole/automtn.h"

#ifdef __WXWINCE__
#include "wx/msw/wince/time.h"
#else
#include <time.h>
#endif

#include <wtypes.h>
#include <unknwn.h>

#include <ole2.h>
#define _huge

#ifndef __WXWINCE__
#include <ole2ver.h>
#endif

#include <oleauto.h>

#if wxUSE_DATETIME
#include "wx/datetime.h"
#endif // wxUSE_TIMEDATE

static void ClearVariant(VARIANTARG *pvarg) ;
static void ReleaseVariant(VARIANTARG *pvarg) ;
// static void ShowException(LPOLESTR szMember, HRESULT hr, EXCEPINFO *pexcep, unsigned int uiArgErr);

/*
 * wxAutomationObject
 */

wxAutomationObject::wxAutomationObject(WXIDISPATCH* dispatchPtr)
{
    m_dispatchPtr = dispatchPtr;
}

wxAutomationObject::~wxAutomationObject()
{
    if (m_dispatchPtr)
    {
        ((IDispatch*)m_dispatchPtr)->Release();
        m_dispatchPtr = NULL;
    }
}

#define INVOKEARG(i) (args ? args[i] : *(ptrArgs[i]))

// For Put/Get, no named arguments are allowed.
bool wxAutomationObject::Invoke(const wxString& member, int action,
        wxVariant& retValue, int noArgs, wxVariant args[], const wxVariant* ptrArgs[]) const
{
    if (!m_dispatchPtr)
        return false;

    // nonConstMember is necessary because the wxString class doesn't have enough consts...
    wxString nonConstMember(member);

    int ch = nonConstMember.Find('.');
    if (ch != -1)
    {
        // Use dot notation to get the next object
        wxString member2(nonConstMember.Left((size_t) ch));
        wxString rest(nonConstMember.Right(nonConstMember.length() - ch - 1));
        wxAutomationObject obj;
        if (!GetObject(obj, member2))
            return false;
        return obj.Invoke(rest, action, retValue, noArgs, args, ptrArgs);
    }

    VARIANTARG vReturn;
    ClearVariant(& vReturn);

    VARIANTARG* vReturnPtr = & vReturn;

    // Find number of names args
    int namedArgCount = 0;
    int i;
    for (i = 0; i < noArgs; i++)
        if (!INVOKEARG(i).GetName().IsNull())
        {
            namedArgCount ++;
        }

    int namedArgStringCount = namedArgCount + 1;
    BSTR* argNames = new BSTR[namedArgStringCount];
    argNames[0] = wxConvertStringToOle(member);

    // Note that arguments are specified in reverse order
    // (all totally logical; hey, we're dealing with OLE here.)

    int j = 0;
    for (i = 0; i < namedArgCount; i++)
    {
        if (!INVOKEARG(i).GetName().IsNull())
        {
            argNames[(namedArgCount-j)] = wxConvertStringToOle(INVOKEARG(i).GetName());
            j ++;
        }
    }

    // + 1 for the member name, + 1 again in case we're a 'put'
    DISPID* dispIds = new DISPID[namedArgCount + 2];

    HRESULT hr;
    DISPPARAMS dispparams;
    unsigned int uiArgErr;
    EXCEPINFO excep;

    // Get the IDs for the member and its arguments.  GetIDsOfNames expects the
    // member name as the first name, followed by argument names (if any).
    hr = ((IDispatch*)m_dispatchPtr)->GetIDsOfNames(IID_NULL, argNames,
                                1 + namedArgCount, LOCALE_SYSTEM_DEFAULT, dispIds);
    if (FAILED(hr))
    {
//        ShowException(szMember, hr, NULL, 0);
        delete[] argNames;
        delete[] dispIds;
        return false;
    }

    // if doing a property put(ref), we need to adjust the first argument to have a
    // named arg of DISPID_PROPERTYPUT.
    if (action & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
    {
        namedArgCount = 1;
        dispIds[1] = DISPID_PROPERTYPUT;
        vReturnPtr = (VARIANTARG*) NULL;
    }

    // Convert the wxVariants to VARIANTARGs
    VARIANTARG* oleArgs = new VARIANTARG[noArgs];
    for (i = 0; i < noArgs; i++)
    {
        // Again, reverse args
        if (!wxConvertVariantToOle(INVOKEARG((noArgs-1) - i), oleArgs[i]))
        {
            delete[] argNames;
            delete[] dispIds;
            delete[] oleArgs;
            return false;
        }
    }

    dispparams.rgdispidNamedArgs = dispIds + 1;
    dispparams.rgvarg = oleArgs;
    dispparams.cArgs = noArgs;
    dispparams.cNamedArgs = namedArgCount;

    excep.pfnDeferredFillIn = NULL;

    hr = ((IDispatch*)m_dispatchPtr)->Invoke(dispIds[0], IID_NULL, LOCALE_SYSTEM_DEFAULT,
                        (WORD)action, &dispparams, vReturnPtr, &excep, &uiArgErr);

    for (i = 0; i < namedArgStringCount; i++)
    {
        SysFreeString(argNames[i]);
    }
    delete[] argNames;
    delete[] dispIds;

    for (i = 0; i < noArgs; i++)
        ReleaseVariant(& oleArgs[i]) ;
    delete[] oleArgs;

    if (FAILED(hr))
    {
        // display the exception information if appropriate:
//        ShowException((const char*) member, hr, &excep, uiArgErr);

        // free exception structure information
        SysFreeString(excep.bstrSource);
        SysFreeString(excep.bstrDescription);
        SysFreeString(excep.bstrHelpFile);

        if (vReturnPtr)
            ReleaseVariant(vReturnPtr);
        return false;
    }
    else
    {
        if (vReturnPtr)
        {
            // Convert result to wxVariant form
            wxConvertOleToVariant(vReturn, retValue);
            // Mustn't release the dispatch pointer
            if (vReturn.vt == VT_DISPATCH)
            {
                vReturn.pdispVal = (IDispatch*) NULL;
            }
            ReleaseVariant(& vReturn);
        }
    }
    return true;
}

// Invoke a member function
wxVariant wxAutomationObject::CallMethod(const wxString& member, int noArgs, wxVariant args[])
{
    wxVariant retVariant;
    if (!Invoke(member, DISPATCH_METHOD, retVariant, noArgs, args))
    {
        retVariant.MakeNull();
    }
    return retVariant;
}

wxVariant wxAutomationObject::CallMethodArray(const wxString& member, int noArgs, const wxVariant **args)
{
    wxVariant retVariant;
    if (!Invoke(member, DISPATCH_METHOD, retVariant, noArgs, NULL, args))
    {
        retVariant.MakeNull();
    }
    return retVariant;
}

wxVariant wxAutomationObject::CallMethod(const wxString& member,
        const wxVariant& arg1, const wxVariant& arg2,
        const wxVariant& arg3, const wxVariant& arg4,
        const wxVariant& arg5, const wxVariant& arg6)
{
    const wxVariant** args = new const wxVariant*[6];
    int i = 0;
    if (!arg1.IsNull())
    {
        args[i] = & arg1;
        i ++;
    }
    if (!arg2.IsNull())
    {
        args[i] = & arg2;
        i ++;
    }
    if (!arg3.IsNull())
    {
        args[i] = & arg3;
        i ++;
    }
    if (!arg4.IsNull())
    {
        args[i] = & arg4;
        i ++;
    }
    if (!arg5.IsNull())
    {
        args[i] = & arg5;
        i ++;
    }
    if (!arg6.IsNull())
    {
        args[i] = & arg6;
        i ++;
    }
    wxVariant retVariant;
    if (!Invoke(member, DISPATCH_METHOD, retVariant, i, NULL, args))
    {
        retVariant.MakeNull();
    }
    delete[] args;
    return retVariant;
}

// Get/Set property
wxVariant wxAutomationObject::GetPropertyArray(const wxString& property, int noArgs, const wxVariant **args) const
{
    wxVariant retVariant;
    if (!Invoke(property, DISPATCH_PROPERTYGET, retVariant, noArgs, NULL, args))
    {
        retVariant.MakeNull();
    }
    return retVariant;
}
wxVariant wxAutomationObject::GetProperty(const wxString& property, int noArgs, wxVariant args[]) const
{
    wxVariant retVariant;
    if (!Invoke(property, DISPATCH_PROPERTYGET, retVariant, noArgs, args))
    {
        retVariant.MakeNull();
    }
    return retVariant;
}

wxVariant wxAutomationObject::GetProperty(const wxString& property,
        const wxVariant& arg1, const wxVariant& arg2,
        const wxVariant& arg3, const wxVariant& arg4,
        const wxVariant& arg5, const wxVariant& arg6)
{
    const wxVariant** args = new const wxVariant*[6];
    int i = 0;
    if (!arg1.IsNull())
    {
        args[i] = & arg1;
        i ++;
    }
    if (!arg2.IsNull())
    {
        args[i] = & arg2;
        i ++;
    }
    if (!arg3.IsNull())
    {
        args[i] = & arg3;
        i ++;
    }
    if (!arg4.IsNull())
    {
        args[i] = & arg4;
        i ++;
    }
    if (!arg5.IsNull())
    {
        args[i] = & arg5;
        i ++;
    }
    if (!arg6.IsNull())
    {
        args[i] = & arg6;
        i ++;
    }
    wxVariant retVariant;
    if (!Invoke(property, DISPATCH_PROPERTYGET, retVariant, i, NULL, args))
    {
        retVariant.MakeNull();
    }
    delete[] args;
    return retVariant;
}

bool wxAutomationObject::PutProperty(const wxString& property, int noArgs, wxVariant args[])
{
    wxVariant retVariant;
    if (!Invoke(property, DISPATCH_PROPERTYPUT, retVariant, noArgs, args))
    {
        return false;
    }
    return true;
}

bool wxAutomationObject::PutPropertyArray(const wxString& property, int noArgs, const wxVariant **args)
{
    wxVariant retVariant;
    if (!Invoke(property, DISPATCH_PROPERTYPUT, retVariant, noArgs, NULL, args))
    {
        return false;
    }
    return true;
}

bool wxAutomationObject::PutProperty(const wxString& property,
        const wxVariant& arg1, const wxVariant& arg2,
        const wxVariant& arg3, const wxVariant& arg4,
        const wxVariant& arg5, const wxVariant& arg6)
{
    const wxVariant** args = new const wxVariant*[6];
    int i = 0;
    if (!arg1.IsNull())
    {
        args[i] = & arg1;
        i ++;
    }
    if (!arg2.IsNull())
    {
        args[i] = & arg2;
        i ++;
    }
    if (!arg3.IsNull())
    {
        args[i] = & arg3;
        i ++;
    }
    if (!arg4.IsNull())
    {
        args[i] = & arg4;
        i ++;
    }
    if (!arg5.IsNull())
    {
        args[i] = & arg5;
        i ++;
    }
    if (!arg6.IsNull())
    {
        args[i] = & arg6;
        i ++;
    }
    wxVariant retVariant;
    bool ret = Invoke(property, DISPATCH_PROPERTYPUT, retVariant, i, NULL, args);
    delete[] args;
    return ret;
}


// Uses DISPATCH_PROPERTYGET
// and returns a dispatch pointer. The calling code should call Release
// on the pointer, though this could be implicit by constructing an wxAutomationObject
// with it and letting the destructor call Release.
WXIDISPATCH* wxAutomationObject::GetDispatchProperty(const wxString& property, int noArgs, wxVariant args[]) const
{
    wxVariant retVariant;
    if (Invoke(property, DISPATCH_PROPERTYGET, retVariant, noArgs, args))
    {
        if (retVariant.GetType() == wxT("void*"))
        {
            return (WXIDISPATCH*) retVariant.GetVoidPtr();
        }
    }

    return (WXIDISPATCH*) NULL;
}

// Uses DISPATCH_PROPERTYGET
// and returns a dispatch pointer. The calling code should call Release
// on the pointer, though this could be implicit by constructing an wxAutomationObject
// with it and letting the destructor call Release.
WXIDISPATCH* wxAutomationObject::GetDispatchProperty(const wxString& property, int noArgs, const wxVariant **args) const
{
    wxVariant retVariant;
    if (Invoke(property, DISPATCH_PROPERTYGET, retVariant, noArgs, NULL, args))
    {
        if (retVariant.GetType() == wxT("void*"))
        {
            return (WXIDISPATCH*) retVariant.GetVoidPtr();
        }
    }

    return (WXIDISPATCH*) NULL;
}


// A way of initialising another wxAutomationObject with a dispatch object
bool wxAutomationObject::GetObject(wxAutomationObject& obj, const wxString& property, int noArgs, wxVariant args[]) const
{
    WXIDISPATCH* dispatch = GetDispatchProperty(property, noArgs, args);
    if (dispatch)
    {
        obj.SetDispatchPtr(dispatch);
        return true;
    }
    else
        return false;
}

// A way of initialising another wxAutomationObject with a dispatch object
bool wxAutomationObject::GetObject(wxAutomationObject& obj, const wxString& property, int noArgs, const wxVariant **args) const
{
    WXIDISPATCH* dispatch = GetDispatchProperty(property, noArgs, args);
    if (dispatch)
    {
        obj.SetDispatchPtr(dispatch);
        return true;
    }
    else
        return false;
}

// Get a dispatch pointer from the current object associated
// with a class id
bool wxAutomationObject::GetInstance(const wxString& classId) const
{
    if (m_dispatchPtr)
        return false;

    CLSID clsId;
    IUnknown * pUnk = NULL;

    wxBasicString unicodeName(classId.mb_str());

    if (FAILED(CLSIDFromProgID((BSTR) unicodeName, &clsId)))
    {
        wxLogWarning(wxT("Cannot obtain CLSID from ProgID"));
        return false;
    }

    if (FAILED(GetActiveObject(clsId, NULL, &pUnk)))
    {
        wxLogWarning(wxT("Cannot find an active object"));
        return false;
    }

    if (pUnk->QueryInterface(IID_IDispatch, (LPVOID*) &m_dispatchPtr) != S_OK)
    {
        wxLogWarning(wxT("Cannot find IDispatch interface"));
        return false;
    }

    return true;
}

// Get a dispatch pointer from a new object associated
// with the given class id
bool wxAutomationObject::CreateInstance(const wxString& classId) const
{
    if (m_dispatchPtr)
        return false;

    CLSID clsId;

    wxBasicString unicodeName(classId.mb_str());

    if (FAILED(CLSIDFromProgID((BSTR) unicodeName, &clsId)))
    {
        wxLogWarning(wxT("Cannot obtain CLSID from ProgID"));
        return false;
    }

    // get the server IDispatch interface
    //
    // NB: using CLSCTX_INPROC_HANDLER results in failure when getting
    //     Automation interface for Microsoft Office applications so don't use
    //     CLSCTX_ALL which includes it
    if (FAILED(CoCreateInstance(clsId, NULL, CLSCTX_SERVER, IID_IDispatch,
                                (void**)&m_dispatchPtr)))
    {
        wxLogWarning(wxT("Cannot start an instance of this class."));
        return false;
    }

    return true;
}


WXDLLEXPORT bool wxConvertVariantToOle(const wxVariant& variant, VARIANTARG& oleVariant)
{
    ClearVariant(&oleVariant);
    if (variant.IsNull())
    {
        oleVariant.vt = VT_NULL;
        return true;
    }

    wxString type(variant.GetType());


    if (type == wxT("long"))
    {
        oleVariant.vt = VT_I4;
        oleVariant.lVal = variant.GetLong() ;
    }
    // cVal not always present
#ifndef __GNUWIN32__
    else if (type == wxT("char"))
    {
        oleVariant.vt=VT_I1;            // Signed Char
        oleVariant.cVal=variant.GetChar();
    }
#endif
    else if (type == wxT("double"))
    {
        oleVariant.vt = VT_R8;
        oleVariant.dblVal = variant.GetDouble();
    }
    else if (type == wxT("bool"))
    {
        oleVariant.vt = VT_BOOL;
        // 'bool' required for VC++ 4 apparently
#if (defined(__VISUALC__) && (__VISUALC__ <= 1000))
        oleVariant.bool = variant.GetBool();
#else
        oleVariant.boolVal = variant.GetBool();
#endif
    }
    else if (type == wxT("string"))
    {
        wxString str( variant.GetString() );
        oleVariant.vt = VT_BSTR;
        oleVariant.bstrVal = wxConvertStringToOle(str);
    }
#if wxUSE_DATETIME
    else if (type == wxT("datetime"))
    {
        wxDateTime date( variant.GetDateTime() );
        oleVariant.vt = VT_DATE;

        // we ought to use SystemTimeToVariantTime() here but this code is
        // untested and hence currently disabled, please let us know if it
        // works for you and we'll enable it
#if 0
        const wxDateTime::Tm tm(date.GetTm());

        SYSTEMTIME st;
        st.wYear = (WXWORD)tm.year;
        st.wMonth = (WXWORD)(tm.mon - wxDateTime::Jan + 1);
        st.wDay = tm.mday;

        st.wDayOfWeek = 0;
        st.wHour = tm.hour;
        st.wMinute = tm.min;
        st.wSecond = tm.sec;
        st.wMilliseconds = tm.msec;

        SystemTimeToVariantTime(&st, &oleVariant.date);
#else
        long dosDateTime = date.GetAsDOS();
        short dosDate = short((dosDateTime & 0xFFFF0000) >> 16);
        short dosTime = short(dosDateTime & 0xFFFF);

        DosDateTimeToVariantTime(dosDate, dosTime, & oleVariant.date);
#endif
    }
#endif
    else if (type == wxT("void*"))
    {
        oleVariant.vt = VT_DISPATCH;
        oleVariant.pdispVal = (IDispatch*) variant.GetVoidPtr();
    }
    else if (type == wxT("list") || type == wxT("stringlist"))
    {
        oleVariant.vt = VT_VARIANT | VT_ARRAY;

        SAFEARRAY *psa;
        SAFEARRAYBOUND saBound;
        VARIANTARG *pvargBase;
        VARIANTARG *pvarg;
        int i, j;

        int iCount = variant.GetCount();

        saBound.lLbound = 0;
        saBound.cElements = iCount;

        psa = SafeArrayCreate(VT_VARIANT, 1, &saBound);
        if (psa == NULL)
            return false;

        SafeArrayAccessData(psa, (void**)&pvargBase);

        pvarg = pvargBase;
        for (i = 0; i < iCount; i++)
        {
            // copy each string in the list of strings
            wxVariant eachVariant(variant[i]);
            if (!wxConvertVariantToOle(eachVariant, * pvarg))
            {
                // memory failure:  back out and free strings alloc'ed up to
                // now, and then the array itself.
                pvarg = pvargBase;
                for (j = 0; j < i; j++)
                {
                    SysFreeString(pvarg->bstrVal);
                    pvarg++;
                }
                SafeArrayDestroy(psa);
                return false;
            }
            pvarg++;
        }

        SafeArrayUnaccessData(psa);

        oleVariant.parray = psa;
    }
    else
    {
        oleVariant.vt = VT_NULL;
        return false;
    }
    return true;
}

#ifndef VT_TYPEMASK
#define VT_TYPEMASK 0xfff
#endif

WXDLLEXPORT bool wxConvertOleToVariant(const VARIANTARG& oleVariant, wxVariant& variant)
{
    switch (oleVariant.vt & VT_TYPEMASK)
    {
    case VT_BSTR:
        {
            wxString str(wxConvertStringFromOle(oleVariant.bstrVal));
            variant = str;
            break;
        }
    case VT_DATE:
        {
#if wxUSE_DATETIME
            SYSTEMTIME st;
            VariantTimeToSystemTime(oleVariant.date, &st);

            wxDateTime date;
            date.Set(st.wDay,
                     (wxDateTime::Month)(wxDateTime::Jan + st.wMonth - 1),
                     st.wYear,
                     st.wHour,
                     st.wMinute,
                     st.wSecond);
            variant = date;
#endif
            break;
        }
    case VT_I4:
        {
            variant = (long) oleVariant.lVal;
            break;
        }
    case VT_I2:
        {
            variant = (long) oleVariant.iVal;
            break;
        }

    case VT_BOOL:
        {
#if (defined(_MSC_VER) && (_MSC_VER <= 1000) && !defined(__MWERKS__) ) //GC
#ifndef HAVE_BOOL // Can't use bool operator if no native bool type
            variant = (long) (oleVariant.bool != 0);
#else
            variant = (bool) (oleVariant.bool != 0);
#endif
#else
#ifndef HAVE_BOOL // Can't use bool operator if no native bool type
            variant = (long) (oleVariant.boolVal != 0);
#else
            variant = (bool) (oleVariant.boolVal != 0);
#endif
#endif
            break;
        }
    case VT_R8:
        {
            variant = oleVariant.dblVal;
            break;
        }
    case VT_VARIANT:
    // case VT_ARRAY: // This is masked out by VT_TYPEMASK
        {
            variant.ClearList();

            int cDims, cElements, i;
            VARIANTARG* pvdata;

            // Iterate the dimensions: number of elements is x*y*z
            for (cDims = 0, cElements = 1;
                cDims < oleVariant.parray->cDims; cDims ++)
                    cElements *= oleVariant.parray->rgsabound[cDims].cElements;

            // Get a pointer to the data
            HRESULT hr = SafeArrayAccessData(oleVariant.parray, (void HUGEP* FAR*) & pvdata);
            if (hr != NOERROR)
                return false;
            // Iterate the data.
            for (i = 0; i < cElements; i++)
            {
                VARIANTARG& oleElement = pvdata[i];
                wxVariant vElement;
                if (!wxConvertOleToVariant(oleElement, vElement))
                    return false;

                variant.Append(vElement);
            }
            SafeArrayUnaccessData(oleVariant.parray);
            break;
        }
    case VT_DISPATCH:
        {
            variant = (void*) oleVariant.pdispVal;
            break;
        }
    case VT_NULL:
        {
            variant.MakeNull();
            break;
        }
    case VT_EMPTY:
        {
            break;    // Ignore Empty Variant, used only during destruction of objects
        }
    default:
        {
            wxLogError(wxT("wxAutomationObject::ConvertOleToVariant: Unknown variant value type"));
            return false;
        }
    }
    return true;
}

/*
 *  ClearVariant
 *
 *  Zeros a variant structure without regard to current contents
 */
static void ClearVariant(VARIANTARG *pvarg)
{
    pvarg->vt = VT_EMPTY;
    pvarg->wReserved1 = 0;
    pvarg->wReserved2 = 0;
    pvarg->wReserved3 = 0;
    pvarg->lVal = 0;
}

/*
 *  ReleaseVariant
 *
 *  Clears a particular variant structure and releases any external objects
 *  or memory contained in the variant.  Supports the data types listed above.
 */
static void ReleaseVariant(VARIANTARG *pvarg)
{
    VARTYPE vt;
    VARIANTARG _huge *pvargArray;
    LONG lLBound, lUBound, l;

    vt = (VARTYPE)(pvarg->vt & 0xfff);        // mask off flags

    // check if an array.  If so, free its contents, then the array itself.
    if (V_ISARRAY(pvarg))
    {
        // variant arrays are all this routine currently knows about.  Since a
        // variant can contain anything (even other arrays), call ourselves
        // recursively.
        if (vt == VT_VARIANT)
        {
            SafeArrayGetLBound(pvarg->parray, 1, &lLBound);
            SafeArrayGetUBound(pvarg->parray, 1, &lUBound);

            if (lUBound > lLBound)
            {
                lUBound -= lLBound;

                SafeArrayAccessData(pvarg->parray, (void**)&pvargArray);

                for (l = 0; l < lUBound; l++)
                {
                    ReleaseVariant(pvargArray);
                    pvargArray++;
                }

                SafeArrayUnaccessData(pvarg->parray);
            }
        }
        else
        {
            wxLogWarning(wxT("ReleaseVariant: Array contains non-variant type"));
        }

        // Free the array itself.
        SafeArrayDestroy(pvarg->parray);
    }
    else
    {
        switch (vt)
        {
            case VT_DISPATCH:
                if (pvarg->pdispVal)
                    pvarg->pdispVal->Release();
                break;

            case VT_BSTR:
                SysFreeString(pvarg->bstrVal);
                break;

            case VT_I2:
            case VT_I4:
            case VT_BOOL:
            case VT_R8:
            case VT_ERROR:        // to avoid erroring on an error return from Excel
            case VT_EMPTY:
            case VT_DATE:
                // no work for these types
                break;

            default:
                wxLogWarning(wxT("ReleaseVariant: Unknown type"));
                break;
        }
    }

    ClearVariant(pvarg);
}

#if 0

void ShowException(LPOLESTR szMember, HRESULT hr, EXCEPINFO *pexcep, unsigned int uiArgErr)
{
    TCHAR szBuf[512];

    switch (GetScode(hr))
    {
        case DISP_E_UNKNOWNNAME:
            wsprintf(szBuf, L"%s: Unknown name or named argument.", szMember);
            break;

        case DISP_E_BADPARAMCOUNT:
            wsprintf(szBuf, L"%s: Incorrect number of arguments.", szMember);
            break;

        case DISP_E_EXCEPTION:
            wsprintf(szBuf, L"%s: Error %d: ", szMember, pexcep->wCode);
            if (pexcep->bstrDescription != NULL)
                lstrcat(szBuf, pexcep->bstrDescription);
            else
                lstrcat(szBuf, L"<<No Description>>");
            break;

        case DISP_E_MEMBERNOTFOUND:
            wsprintf(szBuf, L"%s: method or property not found.", szMember);
            break;

        case DISP_E_OVERFLOW:
            wsprintf(szBuf, L"%s: Overflow while coercing argument values.", szMember);
            break;

        case DISP_E_NONAMEDARGS:
            wsprintf(szBuf, L"%s: Object implementation does not support named arguments.",
                        szMember);
            break;

        case DISP_E_UNKNOWNLCID:
            wsprintf(szBuf, L"%s: The locale ID is unknown.", szMember);
            break;

        case DISP_E_PARAMNOTOPTIONAL:
            wsprintf(szBuf, L"%s: Missing a required parameter.", szMember);
            break;

        case DISP_E_PARAMNOTFOUND:
            wsprintf(szBuf, L"%s: Argument not found, argument %d.", szMember, uiArgErr);
            break;

        case DISP_E_TYPEMISMATCH:
            wsprintf(szBuf, L"%s: Type mismatch, argument %d.", szMember, uiArgErr);
            break;

        default:
            wsprintf(szBuf, L"%s: Unknown error occurred.", szMember);
            break;
    }

    wxLogWarning(szBuf);
}

#endif

#endif // wxUSE_OLE_AUTOMATION
