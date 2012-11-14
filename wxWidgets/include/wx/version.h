/*
 *  Name:        wx/version.h
 *  Purpose:     wxWidgets version numbers
 *  Author:      Julian Smart
 *  Modified by: Ryan Norton (Converted to C)
 *  Created:     29/01/98
 *  RCS-ID:      $Id: version.h 66910 2011-02-16 20:53:53Z JS $
 *  Copyright:   (c) 1998 Julian Smart
 *  Licence:     wxWindows licence
 */

/* THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT */

#ifndef _WX_VERSION_H_
#define _WX_VERSION_H_

#include "wx/cpp.h"    /* for wxSTRINGIZE */

/*  the constants below must be changed with each new version */
/*  ---------------------------------------------------------------------------- */

/*
    Don't forget to update WX_CURRENT, WX_REVISION and WX_AGE in
    build/bakefiles/version.bkl and regenerate the makefiles when you change
    this!
 */

/*  NB: this file is parsed by automatic tools so don't change its format! */
#define wxMAJOR_VERSION      2
#define wxMINOR_VERSION      8
#define wxRELEASE_NUMBER     12
#define wxSUBRELEASE_NUMBER  0
#define wxVERSION_STRING   wxT("wxWidgets 2.8.12")

/*  nothing to update below this line when updating the version */
/*  ---------------------------------------------------------------------------- */

/* Users can pre-define wxABI_VERSION to a lower value in their
 * makefile/project settings to compile code that will be binary compatible
 * with earlier versions of the ABI within the same minor version (between
 * minor versions binary compatibility breaks anyway). The default is the
 * version of wxWidgets being used. A single number with two decimal digits
 * for each component, e.g. 20601 for 2.6.1 */
#ifndef wxABI_VERSION
#define wxABI_VERSION ( wxMAJOR_VERSION * 10000 + wxMINOR_VERSION * 100 + 99 )
#endif

/*  helpers for wxVERSION_NUM_XXX */
#define wxMAKE_VERSION_STRING(x, y, z) \
    wxSTRINGIZE(x) wxSTRINGIZE(y) wxSTRINGIZE(z)
#define wxMAKE_VERSION_DOT_STRING(x, y, z) \
    wxSTRINGIZE(x) "." wxSTRINGIZE(y) "." wxSTRINGIZE(z)

#define wxMAKE_VERSION_STRING_T(x, y, z) \
    wxSTRINGIZE_T(x) wxSTRINGIZE_T(y) wxSTRINGIZE_T(z)
#define wxMAKE_VERSION_DOT_STRING_T(x, y, z) \
    wxSTRINGIZE_T(x) wxT(".") wxSTRINGIZE_T(y) wxT(".") wxSTRINGIZE_T(z)

/*  these are used by src/msw/version.rc and should always be ASCII, not Unicode */
#define wxVERSION_NUM_STRING \
  wxMAKE_VERSION_STRING(wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER)
#define wxVERSION_NUM_DOT_STRING \
  wxMAKE_VERSION_DOT_STRING(wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER)

/* those are Unicode-friendly */
#define wxVERSION_NUM_STRING_T \
  wxMAKE_VERSION_STRING_T(wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER)
#define wxVERSION_NUM_DOT_STRING_T \
  wxMAKE_VERSION_DOT_STRING_T(wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER)

/*  some more defines, not really sure if they're [still] useful */
#define wxVERSION_NUMBER ( (wxMAJOR_VERSION * 1000) + (wxMINOR_VERSION * 100) + wxRELEASE_NUMBER )
#define wxBETA_NUMBER      0
#define wxVERSION_FLOAT ( wxMAJOR_VERSION + (wxMINOR_VERSION/10.0) + (wxRELEASE_NUMBER/100.0) + (wxBETA_NUMBER/10000.0) )

/*  check if the current version is at least major.minor.release */
#define wxCHECK_VERSION(major,minor,release) \
    (wxMAJOR_VERSION > (major) || \
    (wxMAJOR_VERSION == (major) && wxMINOR_VERSION > (minor)) || \
    (wxMAJOR_VERSION == (major) && wxMINOR_VERSION == (minor) && wxRELEASE_NUMBER >= (release)))

/* the same but check the subrelease also */
#define wxCHECK_VERSION_FULL(major,minor,release,subrel) \
    (wxCHECK_VERSION(major, minor, release) && \
        ((major) != wxMAJOR_VERSION || \
            (minor) != wxMINOR_VERSION || \
                (release) != wxRELEASE_NUMBER || \
                    (subrel) <= wxSUBRELEASE_NUMBER))

#endif /*  _WX_VERSION_H_ */

