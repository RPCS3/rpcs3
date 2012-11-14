/*
 *  Name:        wx/cpp.h
 *  Purpose:     Various preprocessor helpers
 *  Author:      Vadim Zeitlin
 *  Created:     2006-09-30
 *  RCS-ID:      $Id: cpp.h 42993 2006-11-03 21:06:57Z VZ $
 *  Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
 *  Licence:     wxWindows licence
 */

/* THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT */

#ifndef _WX_CPP_H_
#define _WX_CPP_H_

/* wxCONCAT works like preprocessor ## operator but also works with macros */
#define wxCONCAT_HELPER(text, line) text ## line
#define wxCONCAT(text, line)        wxCONCAT_HELPER(text, line)

/* wxSTRINGIZE works as the preprocessor # operator but also works with macros */
#define wxSTRINGIZE_HELPER(x)       #x
#define wxSTRINGIZE(x)              wxSTRINGIZE_HELPER(x)

/* a Unicode-friendly version of wxSTRINGIZE_T */
#define wxSTRINGIZE_T(x)            wxAPPLY_T(wxSTRINGIZE(x))

/*
   Helper macros for wxMAKE_UNIQUE_NAME: normally this works by appending the
   current line number to the given identifier to reduce the probability of the
   conflict (it may still happen if this is used in the headers, hence you
   should avoid doing it or provide unique prefixes then) but we have to do it
   differently for VC++
  */
#if defined(__VISUALC__) && (__VISUALC__ >= 1300)
    /*
       __LINE__ handling is completely broken in VC++ when using "Edit and
       Continue" (/ZI option) and results in preprocessor errors if we use it
       inside the macros. Luckily VC7 has another standard macro which can be
       used like this and is even better than __LINE__ because it is globally
       unique.
     */
#   define wxCONCAT_LINE(text)         wxCONCAT(text, __COUNTER__)
#else /* normal compilers */
#   define wxCONCAT_LINE(text)         wxCONCAT(text, __LINE__)
#endif

/* Create a "unique" name with the given prefix */
#define wxMAKE_UNIQUE_NAME(text)    wxCONCAT_LINE(text)

/*
   This macro can be passed as argument to another macro when you don't have
   anything to pass in fact.
 */
#define wxEMPTY_PARAMETER_VALUE /* Fake macro parameter value */

#endif // _WX_CPP_H_

