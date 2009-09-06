///////////////////////////////////////////////////////////////////////////////
// Name:        wx/wx/stackwalk.h
// Purpose:     wxStackWalker and related classes, common part
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-07
// RCS-ID:      $Id: stackwalk.h 43346 2006-11-12 14:33:03Z RR $
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STACKWALK_H_
#define _WX_STACKWALK_H_

#include "wx/defs.h"

#if wxUSE_STACKWALKER

class WXDLLIMPEXP_BASE wxStackFrame;

// ----------------------------------------------------------------------------
// wxStackFrame: a single stack level
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStackFrameBase
{
private:
    // put this inline function here so that it is defined before use
    wxStackFrameBase *ConstCast() const
        { return wx_const_cast(wxStackFrameBase *, this); }

public:
    wxStackFrameBase(size_t level, void *address = NULL)
    {
        m_level = level;

        m_line =
        m_offset = 0;

        m_address = address;
    }

    // get the level of this frame (deepest/innermost one is 0)
    size_t GetLevel() const { return m_level; }

    // return the address of this frame
    void *GetAddress() const { return m_address; }


    // return the unmangled (if possible) name of the function containing this
    // frame
    wxString GetName() const { ConstCast()->OnGetName(); return m_name; }

    // return the instruction pointer offset from the start of the function
    size_t GetOffset() const { ConstCast()->OnGetName(); return m_offset; }

    // get the module this function belongs to (not always available)
    wxString GetModule() const { ConstCast()->OnGetName(); return m_module; }


    // return true if we have the filename and line number for this frame
    bool HasSourceLocation() const { return !GetFileName().empty(); }

    // return the name of the file containing this frame, empty if
    // unavailable (typically because debug info is missing)
    wxString GetFileName() const
        { ConstCast()->OnGetLocation(); return m_filename; }

    // return the line number of this frame, 0 if unavailable
    size_t GetLine() const { ConstCast()->OnGetLocation(); return m_line; }


    // return the number of parameters of this function (may return 0 if we
    // can't retrieve the parameters info even although the function does have
    // parameters)
    virtual size_t GetParamCount() const { return 0; }

    // get the name, type and value (in text form) of the given parameter
    //
    // any pointer may be NULL
    //
    // return true if at least some values could be retrieved
    virtual bool GetParam(size_t WXUNUSED(n),
                          wxString * WXUNUSED(type),
                          wxString * WXUNUSED(name),
                          wxString * WXUNUSED(value)) const
    {
        return false;
    }


    // although this class is not supposed to be used polymorphically, give it
    // a virtual dtor to silence compiler warnings
    virtual ~wxStackFrameBase() { }

protected:
    // hooks for derived classes to initialize some fields on demand
    virtual void OnGetName() { }
    virtual void OnGetLocation() { }


    // fields are protected, not private, so that OnGetXXX() could modify them
    // directly
    size_t m_level;

    wxString m_name,
             m_module,
             m_filename;

    size_t m_line;

    void *m_address;
    size_t m_offset;
};

// ----------------------------------------------------------------------------
// wxStackWalker: class for enumerating stack frames
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStackWalkerBase
{
public:
    // ctor does nothing, use Walk() to walk the stack
    wxStackWalkerBase() { }

    // dtor does nothing neither but should be virtual
    virtual ~wxStackWalkerBase() { }

    // enumerate stack frames from the current location, skipping the initial
    // number of them (this can be useful when Walk() is called from some known
    // location and you don't want to see the first few frames anyhow; also
    // notice that Walk() frame itself is not included if skip >= 1)
    virtual void Walk(size_t skip = 1, size_t maxDepth = 200) = 0;

    // enumerate stack frames from the location of uncaught exception
    //
    // this version can only be called from wxApp::OnFatalException()
    virtual void WalkFromException() = 0;

protected:
    // this function must be overrided to process the given frame
    virtual void OnStackFrame(const wxStackFrame& frame) = 0;
};

#ifdef __WXMSW__
    #include "wx/msw/stackwalk.h"
#elif defined(__UNIX__)
    #include "wx/unix/stackwalk.h"
#else
    #error "wxStackWalker is not supported, set wxUSE_STACKWALKER to 0"
#endif

#endif // wxUSE_STACKWALKER

#endif // _WX_STACKWALK_H_

