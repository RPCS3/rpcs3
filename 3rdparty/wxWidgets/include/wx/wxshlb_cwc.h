/////////////////////////////////////////////////////////////////////////////
// Name:        wx_cw.h
// Purpose:     wxWidgets definitions for CodeWarrior builds
// Author:      Stefan Csomor
// Modified by:
// Created:     12/10/98
// RCS-ID:      $Id: wxshlb_cwc.h 33744 2005-04-19 10:06:30Z SC $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CW__
#define _WX_CW__

#if __MWERKS__ >= 0x2400
#pragma old_argmatch on
#endif

#if __option(profile)
#ifdef __cplusplus
    #if __POWERPC__
        #include "wxshlb_Carbon++_prof.mch"
    #endif
#else
    #if __POWERPC__
        #include "wxshlb_Carbon_prof.mch"
    #endif
#endif
#else
#ifdef __cplusplus
    #if __MACH__
        #include "wxshlb_Mach++.mch"
    #elif __POWERPC__
        #include "wxshlb_Carbon++.mch"
    #endif
#else
    #if __MACH__
        #include "wxshlb_Mach.mch"
    #elif __POWERPC__
        #include "wxshlb_Carbon.mch"
    #endif
#endif
#endif
#endif
    // _WX_CW__
