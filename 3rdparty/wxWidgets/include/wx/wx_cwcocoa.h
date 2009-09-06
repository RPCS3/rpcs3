/////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/wx_cwcocoa.h
// Purpose:     Metrowerks Prefix Header File (wxCocoa Release)
// Author:      Tommy Tian (tommy.tian@webex.com)
// Modified by: David Elliott
// Created:     10/22/2004
// RCS-ID:      $Id: wx_cwcocoa.h 30235 2004-11-02 06:22:11Z DE $
// Copyright:   (c) Tommy Tian
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CW_COCOA__
#define _WX_CW_COCOA__

#if __MWERKS__ >= 0x2400 && __MWERKS__ <= 0x3200
#pragma old_argmatch on
#endif

#if __option(profile)
#ifdef __cplusplus
    #ifdef __OBJC__
        #if __mwlinker__
            #include "wx_cocoaMacOSXmm_prof.mch"
        #else
            #include "wx_cocoaMach-Omm_prof.mch"
        #endif
    #else
        #if __mwlinker__
            #include "wx_cocoaMacOSX++_prof.mch"
        #else
            #include "wx_cocoaMach-O++_prof.mch"
        #endif
    #endif
#else
    #if __mwlinker__
        #include "wx_cocoaMacOSX_prof.mch"
    #else
        #include "wx_cocoaMach-O_prof.mch"
    #endif
#endif
#else
#ifdef __cplusplus
    #ifdef __OBJC__
        #if __mwlinker__
            #include "wx_cocoaMacOSXmm.mch"
        #else
            #include "wx_cocoaMach-Omm.mch"
        #endif
    #else
        #if __mwlinker__
            #include "wx_cocoaMacOSX++.mch"
        #else
            #include "wx_cocoaMach-O++.mch"
        #endif
    #endif
#else
    #if __mwlinker__
        #include "wx_cocoaMacOSX.mch"
    #else
        #include "wx_cocoaMach-O.mch"
    #endif
#endif
#endif

#endif
    // _WX_CW_COCOA__
