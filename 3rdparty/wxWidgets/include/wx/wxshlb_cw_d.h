/////////////////////////////////////////////////////////////////////////////
// Name:        wx_cw_d.h
// Purpose:     wxWidgets definitions for CodeWarrior builds (Debug)
// Author:      Stefan Csomor
// Modified by:
// Created:     12/10/98
// RCS-ID:      $Id: wxshlb_cw_d.h 33744 2005-04-19 10:06:30Z SC $
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
    #if __POWERPC__
        #include "wxshlb_PPC++_d.mch"
    #elif __INTEL__
        #include "wxshlb_x86++_d.mch"
    #elif __CFM68K__
        #include "wxshlb_cfm++_d.mch"
    #else
        #include "wxshlb_68k++_d.mch"
    #endif
#else
    #if __POWERPC__
        #include "wxshlb_PPC_d.mch"
    #elif __INTEL__
        #include "wxshlb_x86_d.mch"
    #elif __CFM68K__
        #include "wxshlb_cfm_d.mch"
    #else
        #include "wxshlb_68k_d.mch"
    #endif
#endif
#endif

#endif
    // _WX_CW__
