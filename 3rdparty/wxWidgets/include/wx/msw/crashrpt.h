///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/crashrpt.h
// Purpose:     helpers for the structured exception handling (SEH) under Win32
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.07.2003
// RCS-ID:      $Id: crashrpt.h 34436 2005-05-31 09:20:43Z JS $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_CRASHRPT_H_
#define _WX_MSW_CRASHRPT_H_

#include "wx/defs.h"

#if wxUSE_CRASHREPORT

struct _EXCEPTION_POINTERS;

// ----------------------------------------------------------------------------
// crash report generation flags
// ----------------------------------------------------------------------------

enum
{
    // we always report where the crash occurred
    wxCRASH_REPORT_LOCATION = 0,

    // if this flag is given, the call stack is dumped
    //
    // this results in dump/crash report as small as possible, this is the
    // default flag
    wxCRASH_REPORT_STACK = 1,

    // if this flag is given, the values of the local variables are dumped
    //
    // note that with the current implementation it requires dumping the full
    // process address space and so this will result in huge dump file and will
    // take some time to generate
    //
    // it's probably not a good idea to use this by default, start with default
    // mini dump and ask your users to set WX_CRASH_FLAGS environment variable
    // to 2 or 4 if you need more information in the dump
    wxCRASH_REPORT_LOCALS = 2,

    // if this flag is given, the values of all global variables are dumped
    //
    // this creates a much larger mini dump than just wxCRASH_REPORT_STACK but
    // still much smaller than wxCRASH_REPORT_LOCALS one
    wxCRASH_REPORT_GLOBALS = 4,

    // default is to create the smallest possible crash report
    wxCRASH_REPORT_DEFAULT = wxCRASH_REPORT_LOCATION | wxCRASH_REPORT_STACK
};

// ----------------------------------------------------------------------------
// wxCrashContext: information about the crash context
// ----------------------------------------------------------------------------

struct WXDLLIMPEXP_BASE wxCrashContext
{
    // initialize this object with the given information or from the current
    // global exception info which is only valid inside wxApp::OnFatalException
    wxCrashContext(_EXCEPTION_POINTERS *ep = NULL);

    // get the name for this exception code
    wxString GetExceptionString() const;


    // exception code
    size_t code;

    // exception address
    void *addr;

    // machine-specific registers vaues
    struct
    {
#ifdef __INTEL__
        wxInt32 eax, ebx, ecx, edx, esi, edi,
                ebp, esp, eip,
                cs, ds, es, fs, gs, ss,
                flags;
#endif // __INTEL__
    } regs;
};

// ----------------------------------------------------------------------------
// wxCrashReport: this class is used to create crash reports
// ----------------------------------------------------------------------------

struct WXDLLIMPEXP_BASE wxCrashReport
{
    // set the name of the file to which the report is written, it is
    // constructed from the .exe name by default
    static void SetFileName(const wxChar *filename);

    // return the current file name
    static const wxChar *GetFileName();

    // write the exception report to the file, return true if it could be done
    // or false otherwise
    //
    // if ep pointer is NULL, the global exception info which is valid only
    // inside wxApp::OnFatalException() is used
    static bool Generate(int flags = wxCRASH_REPORT_DEFAULT,
                         _EXCEPTION_POINTERS *ep = NULL);


    // generate a crash report from outside of wxApp::OnFatalException(), this
    // can be used to take "snapshots" of the program in wxApp::OnAssert() for
    // example
    static bool GenerateNow(int flags = wxCRASH_REPORT_DEFAULT);
};

#endif // wxUSE_CRASHREPORT

#endif // _WX_MSW_CRASHRPT_H_

