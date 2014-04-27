/////////////////////////////////////////////////////////////////////////////
// Name:        wince/crt.cpp
// Purpose:     Implementation of CRT functions missing under Windows CE
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.04.04
// RCS-ID:
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif

#include "wx/msw/wince/missing.h"

extern "C" void *
bsearch(const void *key, const void *base, size_t num, size_t size,
        int (wxCMPFUNC_CONV *cmp)(const void *, const void *))
{
    int res;
    char *mid;

    char *lo = (char *)base;
    char *hi = lo + num*size;
    while ( lo < hi )
    {
        mid = lo + (hi - lo)/2;

        res = (*cmp)(key, mid);
        if ( res < 0 )
            hi = mid;
        else if ( res > 0 )
            lo = mid + size;
        else // res == 0
            return mid;
    }

    return NULL;
}

extern "C"
void abort()
{
    wxString name;
    if ( wxTheApp )
        name = wxTheApp->GetAppName();
    if ( name.empty() )
        name = L"wxWidgets Application";

    MessageBox(NULL, L"Abnormal program termination", name, MB_ICONHAND | MB_OK);

    _exit(3);
//    ::ExitProcess(3);
}

extern "C"
char *getenv(const char * WXUNUSED(name))
{
    // no way to implement it in Unicode-only environment without using
    // wxCharBuffer and it is of no use in C code which uses this function
    // (libjpeg)
    return NULL;
}

