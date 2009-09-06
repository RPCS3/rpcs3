///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/stackwalk.h
// Purpose:     wxStackWalker for MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-08
// RCS-ID:      $Id: stackwalk.h 43346 2006-11-12 14:33:03Z RR $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_STACKWALK_H_
#define _WX_MSW_STACKWALK_H_

#include "wx/arrstr.h"

// these structs are declared in windows headers
struct _CONTEXT;
struct _EXCEPTION_POINTERS;

// and these in dbghelp.h
struct _SYMBOL_INFO;

// ----------------------------------------------------------------------------
// wxStackFrame
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStackFrame : public wxStackFrameBase
{
private:
    wxStackFrame *ConstCast() const
        { return wx_const_cast(wxStackFrame *, this); }

    size_t DoGetParamCount() const { return m_paramTypes.GetCount(); }

public:
    wxStackFrame(size_t level, void *address, size_t addrFrame)
        : wxStackFrameBase(level, address)
    {
        m_hasName =
        m_hasLocation = false;

        m_addrFrame = addrFrame;
    }

    virtual size_t GetParamCount() const
    {
        ConstCast()->OnGetParam();
        return DoGetParamCount();
    }

    virtual bool
    GetParam(size_t n, wxString *type, wxString *name, wxString *value) const;

    // callback used by OnGetParam(), don't call directly
    void OnParam(_SYMBOL_INFO *pSymInfo);

protected:
    virtual void OnGetName();
    virtual void OnGetLocation();

    void OnGetParam();


    // helper for debug API: it wants to have addresses as DWORDs
    size_t GetSymAddr() const
    {
        return wx_reinterpret_cast(size_t, m_address);
    }

private:
    bool m_hasName,
         m_hasLocation;

    size_t m_addrFrame;

    wxArrayString m_paramTypes,
                  m_paramNames,
                  m_paramValues;
};

// ----------------------------------------------------------------------------
// wxStackWalker
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStackWalker : public wxStackWalkerBase
{
public:
    // we don't use ctor argument, it is for compatibility with Unix version
    // only
    wxStackWalker(const char * WXUNUSED(argv0) = NULL) { }

    virtual void Walk(size_t skip = 1, size_t maxDepth = 200);
    virtual void WalkFromException();


    // enumerate stack frames from the given context
    void WalkFrom(const _CONTEXT *ctx, size_t skip = 1);
    void WalkFrom(const _EXCEPTION_POINTERS *ep, size_t skip = 1);
};

#endif // _WX_MSW_STACKWALK_H_

