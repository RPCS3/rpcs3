/////////////////////////////////////////////////////////////////////////////
// Name:        htmldefs.h
// Purpose:     constants for wxhtml library
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmldefs.h 40823 2006-08-25 16:52:58Z VZ $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#ifndef _WX_HTMLDEFS_H_
#define _WX_HTMLDEFS_H_

#include "wx/defs.h"

#if wxUSE_HTML

//--------------------------------------------------------------------------------
// ALIGNMENTS
//                  Describes alignment of text etc. in containers
//--------------------------------------------------------------------------------

#define wxHTML_ALIGN_LEFT            0x0000
#define wxHTML_ALIGN_RIGHT           0x0002
#define wxHTML_ALIGN_JUSTIFY         0x0010

#define wxHTML_ALIGN_TOP             0x0004
#define wxHTML_ALIGN_BOTTOM          0x0008

#define wxHTML_ALIGN_CENTER          0x0001



//--------------------------------------------------------------------------------
// COLOR MODES
//                  Used by wxHtmlColourCell to determine clr of what is changing
//--------------------------------------------------------------------------------

#define wxHTML_CLR_FOREGROUND        0x0001
#define wxHTML_CLR_BACKGROUND        0x0002



//--------------------------------------------------------------------------------
// UNITS
//                  Used to specify units
//--------------------------------------------------------------------------------

#define wxHTML_UNITS_PIXELS          0x0001
#define wxHTML_UNITS_PERCENT         0x0002



//--------------------------------------------------------------------------------
// INDENTS
//                  Used to specify indetation relatives
//--------------------------------------------------------------------------------

#define wxHTML_INDENT_LEFT           0x0010
#define wxHTML_INDENT_RIGHT          0x0020
#define wxHTML_INDENT_TOP            0x0040
#define wxHTML_INDENT_BOTTOM         0x0080

#define wxHTML_INDENT_HORIZONTAL     (wxHTML_INDENT_LEFT | wxHTML_INDENT_RIGHT)
#define wxHTML_INDENT_VERTICAL       (wxHTML_INDENT_TOP | wxHTML_INDENT_BOTTOM)
#define wxHTML_INDENT_ALL            (wxHTML_INDENT_VERTICAL | wxHTML_INDENT_HORIZONTAL)




//--------------------------------------------------------------------------------
// FIND CONDITIONS
//                  Identifiers of wxHtmlCell's Find() conditions
//--------------------------------------------------------------------------------

#define wxHTML_COND_ISANCHOR              1
        // Finds the anchor of 'param' name (pointer to wxString).

#define wxHTML_COND_ISIMAGEMAP            2
        // Finds imagemap of 'param' name (pointer to wxString).
    // (used exclusively by m_image.cpp)

#define wxHTML_COND_USER              10000
        // User-defined conditions should start from this number


//--------------------------------------------------------------------------------
// INTERNALS
//                  wxHTML internal constants
//--------------------------------------------------------------------------------

    /* size of one scroll step of wxHtmlWindow in pixels */
#define wxHTML_SCROLL_STEP               16

    /* size of temporary buffer used during parsing */
#define wxHTML_BUFLEN                  1024

    /* maximum number of pages printable via html printing */
#define wxHTML_PRINT_MAX_PAGES          999

#endif // wxUSE_HTML

#endif // _WX_HTMLDEFS_H_

