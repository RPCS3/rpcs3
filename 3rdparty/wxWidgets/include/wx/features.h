/**
*  Name:        wx/features.h
*  Purpose:     test macros for the features which might be available in some
*               wxWidgets ports but not others
*  Author:      Vadim Zeitlin
*  Modified by: Ryan Norton (Converted to C)
*  Created:     18.03.02
*  RCS-ID:      $Id: features.h 40865 2006-08-27 09:42:42Z VS $
*  Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwidgets.org>
*  Licence:     wxWindows licence
*/

/* THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT */

#ifndef _WX_FEATURES_H_
#define _WX_FEATURES_H_

/*  radio menu items are currently not implemented in wxMotif, use this
    symbol (kept for compatibility from the time when they were not implemented
    under other platforms as well) to test for this */
#if !defined(__WXMOTIF__)
    #define wxHAS_RADIO_MENU_ITEMS
#else
    #undef wxHAS_RADIO_MENU_ITEMS
#endif

/*  the raw keyboard codes are generated under wxGTK and wxMSW only */
#if defined(__WXGTK__) || defined(__WXMSW__) || defined(__WXMAC__) \
    || defined(__WXDFB__)
    #define wxHAS_RAW_KEY_CODES
#else
    #undef wxHAS_RAW_KEY_CODES
#endif

/*  taskbar is implemented in the major ports */
#if defined(__WXMSW__) || defined(__WXCOCOA__) \
    || defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXX11__) \
    || defined(__WXMAC_OSX__) || defined(__WXCOCOA__)
    #define wxHAS_TASK_BAR_ICON
#else
    #undef wxHAS_TASK_BAR_ICON
#endif

/*  wxIconLocation appeared in the middle of 2.5.0 so it's handy to have a */
/*  separate define for it */
#define wxHAS_ICON_LOCATION

/*  same for wxCrashReport */
#ifdef __WXMSW__
    #define wxHAS_CRASH_REPORT
#else
    #undef wxHAS_CRASH_REPORT
#endif

/*  wxRE_ADVANCED is not always available, depending on regex library used
 *  (it's unavailable only if compiling via configure against system library) */
#ifndef WX_NO_REGEX_ADVANCED
    #define wxHAS_REGEX_ADVANCED
#else
    #undef wxHAS_REGEX_ADVANCED
#endif

#endif /*  _WX_FEATURES_H_ */

