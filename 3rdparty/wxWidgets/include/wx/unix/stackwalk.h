///////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/stackwalk.h
// Purpose:     declaration of wxStackWalker for Unix
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-19
// RCS-ID:      $Id: stackwalk.h 43346 2006-11-12 14:33:03Z RR $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_STACKWALK_H_
#define _WX_UNIX_STACKWALK_H_

// ----------------------------------------------------------------------------
// wxStackFrame
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStackFrame : public wxStackFrameBase
{
    friend class wxStackWalker;

public:
    // arguments are the stack depth of this frame, its address and the return
    // value of backtrace_symbols() for it
    //
    // NB: we don't copy syminfo pointer so it should have lifetime at least as
    //     long as ours
    wxStackFrame(size_t level = 0, void *address = NULL, const char *syminfo = NULL)
        : wxStackFrameBase(level, address)
    {
        m_syminfo = syminfo;
    }

protected:
    virtual void OnGetName();

    // optimized for the 2 step initialization done by wxStackWalker
    void Set(const wxString &name, const wxString &filename, const char* syminfo,
             size_t level, size_t numLine, void *address)
    {
        m_level = level;
        m_name = name;
        m_filename = filename;
        m_syminfo = syminfo;

        m_line = numLine;
        m_address = address;
    }

private:
    const char *m_syminfo;
};

// ----------------------------------------------------------------------------
// wxStackWalker
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStackWalker : public wxStackWalkerBase
{
public:
    // we need the full path to the program executable to be able to use
    // addr2line, normally we can retrieve it from wxTheApp but if wxTheApp
    // doesn't exist or doesn't have the correct value, the path may be given
    // explicitly
    wxStackWalker(const char *argv0 = NULL)
    {
        ms_exepath = wxString::FromAscii(argv0);
    }

    ~wxStackWalker()
    {
        FreeStack();
    }

    virtual void Walk(size_t skip = 1, size_t maxDepth = 200);
    virtual void WalkFromException() { Walk(2); }

    static const wxString& GetExePath() { return ms_exepath; }


    // these two may be used to save the stack at some point (fast operation)
    // and then process it later (slow operation)
    void SaveStack(size_t maxDepth);
    void ProcessFrames(size_t skip);
    void FreeStack();

private:
    int InitFrames(wxStackFrame *arr, size_t n, void **addresses, char **syminfo);

    static wxString ms_exepath;
    static void *ms_addresses[];
    static char **ms_symbols;
    static int m_depth;
};

#endif // _WX_UNIX_STACKWALK_H_
