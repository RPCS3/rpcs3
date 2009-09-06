/////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/wx_cwcocoa_d.h
// Purpose:     Metrowerks Prefix Header File (wxCocoa Debug)
// Author:      Tommy Tian (tommy.tian@webex.com)
// Modified by: David Elliott
// Created:     10/04/2004
// RCS-ID:      $Id: wx_cwcocoa_d.h 30235 2004-11-02 06:22:11Z DE $
// Copyright:   (c) Tommy Tian
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#ifndef _WX_CW_COCOA__
#define _WX_CW_COCOA__

#if __MWERKS__ >= 0x2400 && __MWERKS__ <= 0x3200
#pragma old_argmatch on
#endif

#if __option(profile)
#error "profiling is not supported in debug versions"
#else
#ifdef __cplusplus
    #ifdef __OBJC__
        #if __mwlinker__
            #include "wx_cocoaMacOSXmm_d.mch"
        #else
            #include "wx_cocoaMach-Omm_d.mch"
        #endif
    #else
        #if __mwlinker__
            #include "wx_cocoaMacOSX++_d.mch"
        #else
            #include "wx_cocoaMach-O++_d.mch"
        #endif
    #endif
#else
    #if __mwlinker__
        #include "wx_cocoaMacOSX_d.mch"
    #else
        #include "wx_cocoaMach-O_d.mch"
    #endif
#endif
#endif

#endif
    // _WX_CW_COCOA__
