/////////////////////////////////////////////////////////////////////////////
// Name:        wx_cw.h
// Purpose:     wxWidgets definitions for CodeWarrior builds
// Author:      Stefan Csomor
// Modified by:
// Created:     12/10/98
// RCS-ID:      $Id: wxshlb_cw.h 33744 2005-04-19 10:06:30Z SC $
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
        #include "wxshlb_PPC++_prof.mch"
    #elif __INTEL__
        #include "wxshlb_x86++_prof.mch"
    #elif __CFM68K__
        #include "wxshlb_cfm++_prof.mch"
    #else
        #include "wxshlb_68k++_prof.mch"
    #endif
#else
    #if __POWERPC__
        #include "wxshlb_PPC_prof.mch"
    #elif __INTEL__
        #include "wxshlb_x86_prof.mch"
    #elif __CFM68K__
        #include "wxshlb_cfm_prof.mch"
    #else
        #include "wxshlb_68k_prof.mch"
    #endif
#endif
#else
#ifdef __cplusplus
    #if __POWERPC__
        #include "wxshlb_PPC++.mch"
    #elif __INTEL__
        #include "wxshlb_x86++.mch"
    #elif __CFM68K__
        #include "wxshlb_cfm++.mch"
    #else
        #include "wxshlb_68k++.mch"
    #endif
#else
    #if __POWERPC__
        #include "wxshlb_PPC.mch"
    #elif __INTEL__
        #include "wxshlb_x86.mch"
    #elif __CFM68K__
        #include "wxshlb_cfm.mch"
    #else
        #include "wxshlb_68k.mch"
    #endif
#endif
#endif
#endif
    // _WX_CW__
