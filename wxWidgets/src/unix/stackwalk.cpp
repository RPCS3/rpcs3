/////////////////////////////////////////////////////////////////////////////
// Name:        msw/stackwalk.cpp
// Purpose:     wxStackWalker implementation for Unix/glibc
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-18
// RCS-ID:      $Id: stackwalk.cpp 43965 2006-12-13 13:04:44Z VZ $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
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

#if wxUSE_STACKWALKER

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/utils.h"
#endif

#include "wx/stackwalk.h"
#include "wx/stdpaths.h"

#include <execinfo.h>

#ifdef HAVE_CXA_DEMANGLE
    #include <cxxabi.h>
#endif // HAVE_CXA_DEMANGLE

// ----------------------------------------------------------------------------
// tiny helper wrapper around popen/pclose()
// ----------------------------------------------------------------------------

class wxStdioPipe
{
public:
    // ctor parameters are passed to popen()
    wxStdioPipe(const char *command, const char *type)
    {
        m_fp = popen(command, type);
    }

    // conversion to stdio FILE
    operator FILE *() const { return m_fp; }

    // dtor closes the pipe
    ~wxStdioPipe()
    {
        if ( m_fp )
            pclose(m_fp);
    }

private:
    FILE *m_fp;

    DECLARE_NO_COPY_CLASS(wxStdioPipe)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxStackFrame
// ----------------------------------------------------------------------------

void wxStackFrame::OnGetName()
{
    if ( !m_name.empty() )
        return;

    // we already tried addr2line in wxStackWalker::InitFrames: it always
    // gives us demangled names (even if __cxa_demangle is not available) when
    // the function is part of the ELF (when it's in a shared object addr2line
    // will give "??") and because it seems less error-prone.
    // when it works, backtrace_symbols() sometimes returns incorrect results

    // format is: "module(funcname+offset) [address]" but the part in
    // parentheses can be not present
    wxString syminfo = wxString::FromAscii(m_syminfo);
    const size_t posOpen = syminfo.find(_T('('));
    if ( posOpen != wxString::npos )
    {
        const size_t posPlus = syminfo.find(_T('+'), posOpen + 1);
        if ( posPlus != wxString::npos )
        {
            const size_t posClose = syminfo.find(_T(')'), posPlus + 1);
            if ( posClose != wxString::npos )
            {
                if ( m_name.empty() )
                {
                    m_name.assign(syminfo, posOpen + 1, posPlus - posOpen - 1);

#ifdef HAVE_CXA_DEMANGLE
                    int rc = -1;
                    char *cppfunc = __cxxabiv1::__cxa_demangle
                                    (
                                        m_name.mb_str(),
                                        NULL, // output buffer (none, alloc it)
                                        NULL, // [out] len of output buffer
                                        &rc
                                    );
                    if ( rc == 0 )
                        m_name = wxString::FromAscii(cppfunc);

                    free(cppfunc);
#endif // HAVE_CXA_DEMANGLE
                }

                unsigned long ofs;
                if ( wxString(syminfo, posPlus + 1, posClose - posPlus - 1).
                        ToULong(&ofs, 0) )
                    m_offset = ofs;
            }
        }

        m_module.assign(syminfo, posOpen);
    }
    else // not in "module(funcname+offset)" format
    {
        m_module = syminfo;
    }
}


// ----------------------------------------------------------------------------
// wxStackWalker
// ----------------------------------------------------------------------------

// that many frames should be enough for everyone
#define MAX_FRAMES          200

// we need a char buffer big enough to contain a call to addr2line with
// up to MAX_FRAMES addresses !
// NB: %p specifier will print the pointer in hexadecimal form
//     and thus will require 2 chars for each byte + 3 for the
//     " 0x" prefix
#define CHARS_PER_FRAME    (sizeof(void*) * 2 + 3)

// BUFSIZE will be 2250 for 32 bit machines
#define BUFSIZE            (50 + MAX_FRAMES*CHARS_PER_FRAME)

// static data
void *wxStackWalker::ms_addresses[MAX_FRAMES];
char **wxStackWalker::ms_symbols = NULL;
int wxStackWalker::m_depth = 0;
wxString wxStackWalker::ms_exepath;
static char g_buf[BUFSIZE];


void wxStackWalker::SaveStack(size_t maxDepth)
{
    // read all frames required
    maxDepth = wxMin(WXSIZEOF(ms_addresses)/sizeof(void*), maxDepth);
    m_depth = backtrace(ms_addresses, maxDepth*sizeof(void*));
    if ( !m_depth )
        return;

    ms_symbols = backtrace_symbols(ms_addresses, m_depth);
}

void wxStackWalker::ProcessFrames(size_t skip)
{
    wxStackFrame frames[MAX_FRAMES];

    if (!ms_symbols || !m_depth)
        return;

    // we have 3 more "intermediate" frames which the calling code doesn't know
    // about, account for them
    skip += 3;

    // call addr2line only once since this call may be very slow
    // (it has to load in memory the entire EXE of this app which may be quite
    //  big, especially if it contains debug info and is compiled statically!)
    int towalk = InitFrames(frames, m_depth - skip, &ms_addresses[skip], &ms_symbols[skip]);

    // now do user-defined operations on each frame
    for ( int n = 0; n < towalk - (int)skip; n++ )
        OnStackFrame(frames[n]);
}

void wxStackWalker::FreeStack()
{
    // ms_symbols has been allocated by backtrace_symbols() and it's the responsibility
    // of the caller, i.e. us, to free that pointer
    if (ms_symbols)
        free( ms_symbols );
    ms_symbols = NULL;
    m_depth = 0;
}

int wxStackWalker::InitFrames(wxStackFrame *arr, size_t n, void **addresses, char **syminfo)
{
    // we need to launch addr2line tool to get this information and we need to
    // have the program name for this
    wxString exepath = wxStackWalker::GetExePath();
    if ( exepath.empty() )
    {
        exepath = wxStandardPaths::Get().GetExecutablePath();
        if ( exepath.empty() )
        {
            wxLogDebug(wxT("Cannot parse stack frame because the executable ")
                       wxT("path could not be detected"));
            return 0;
        }
    }

    // build the (long) command line for executing addr2line in an optimized way
    // (e.g. use always chars, even in Unicode build: popen() always takes chars)
    int len = snprintf(g_buf, BUFSIZE, "addr2line -C -f -e \"%s\"", (const char*) exepath.mb_str());
    len = (len <= 0) ? strlen(g_buf) : len;     // in case snprintf() is broken
    for (size_t i=0; i<n; i++)
    {
        snprintf(&g_buf[len], BUFSIZE - len, " %p", addresses[i]);
        len = strlen(g_buf);
    }

    //wxLogDebug(wxT("piping the command '%s'"), g_buf);  // for debug only

    wxStdioPipe fp(g_buf, "r");
    if ( !fp )
        return 0;

    // parse addr2line output (should be exactly 2 lines for each address)
    // reusing the g_buf used for building the command line above
    wxString name, filename;
    unsigned long line = 0,
                  curr = 0;
    for  ( size_t i = 0; i < n; i++ )
    {
        // 1st line has function name
        if ( fgets(g_buf, WXSIZEOF(g_buf), fp) )
        {
            name = wxString::FromAscii(g_buf);
            name.RemoveLast(); // trailing newline

            if ( name == _T("??") )
                name.clear();
        }
        else
        {
            wxLogDebug(_T("cannot read addr2line output for stack frame #%lu"),
                       (unsigned long)i);
            return false;
        }

        // 2nd one -- the file/line info
        if ( fgets(g_buf, WXSIZEOF(g_buf), fp) )
        {
            filename = wxString::FromAscii(g_buf);
            filename.RemoveLast();

            const size_t posColon = filename.find(_T(':'));
            if ( posColon != wxString::npos )
            {
                // parse line number (it's ok if it fails, this will just leave
                // line at its current, invalid, 0 value)
                wxString(filename, posColon + 1, wxString::npos).ToULong(&line);

                // remove line number from 'filename'
                filename.erase(posColon);
                if ( filename == _T("??") )
                    filename.clear();
            }
            else
            {
                wxLogDebug(_T("Unexpected addr2line format: \"%s\" - ")
                           _T("the semicolon is missing"),
                           filename.c_str());
            }
        }

        // now we've got enough info to initialize curr-th stack frame
        // (at worst, only addresses[i] and syminfo[i] have been initialized,
        //  but wxStackFrame::OnGetName may still be able to get function name):
        arr[curr++].Set(name, filename, syminfo[i], i, line, addresses[i]);
    }

    return curr;
}

void wxStackWalker::Walk(size_t skip, size_t maxDepth)
{
    // read all frames required
    SaveStack(maxDepth);

    // process them
    ProcessFrames(skip);

    // cleanup
    FreeStack();
}

#endif // wxUSE_STACKWALKER
