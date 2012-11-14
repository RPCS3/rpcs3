/////////////////////////////////////////////////////////////////////////////
// Name:        wx_cw_d.h
// Purpose:     wxWidgets definitions for CodeWarrior builds (Debug)
// Author:      Stefan Csomor
// Modified by:
// Created:     12/10/98
// RCS-ID:      $Id: wxshlba_cwc_d.h 33744 2005-04-19 10:06:30Z SC $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CW__
#define _WX_CW__

#if __MWERKS__ >= 0x2400
#pragma old_argmatch on
#endif

#if __option(profile)
#error "profiling is not supported in debug versions"
#else
#ifdef __cplusplus
    #ifdef __MACH__
        #include "wxshlba_Mach++_d.mch"
    #elif __POWERPC__
        #include "wxshlba_Carbon++_d.mch"
    #endif
#else
    #ifdef __MACH__
        #include "wxshlba_Mach_d.mch"
    #elif __POWERPC__
        #include "wxshlba_Carbon_d.mch"
    #endif
#endif
#endif

#endif
    // _WX_CW__
