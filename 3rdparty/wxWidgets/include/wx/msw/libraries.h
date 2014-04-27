/*
 * Name:        wx/msw/libraries.h
 * Purpose:     Pragmas for linking libs conditionally
 * Author:      Michael Wetherell
 * Modified by:
 * RCS-ID:      $Id: libraries.h 37045 2006-01-21 22:50:46Z MW $
 * Copyright:   (c) 2005 Michael Wetherell
 * Licence:     wxWindows licence
 */

#ifndef _WX_MSW_LIBRARIES_H_
#define _WX_MSW_LIBRARIES_H_

/*
 * Notes:
 *
 * In general the preferred place to add libs is in the bakefiles. This file
 * can be used where libs must be added conditionally, for those compilers that
 * support a way to do that.
 */

/* VC++ 5 didn't include oleacc.lib, though it came with the PSDK */
#if defined __VISUALC__ && (wxUSE_ACCESSIBILITY || __VISUALC__ >= 1200)
#pragma comment(lib, "oleacc")
#endif

#endif /* _WX_MSW_LIBRARIES_H_ */
