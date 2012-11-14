/////////////////////////////////////////////////////////////////////////////
// Name:        wx/quantize.h
// Purpose:     wxQuantizer class
// Author:      Julian Smart
// Modified by:
// Created:     22/6/2000
// RCS-ID:      $Id: quantize.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) Julian Smart
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_QUANTIZE_H_
#define _WX_QUANTIZE_H_

#include "wx/object.h"

/*
 * From jquant2.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 */

class WXDLLIMPEXP_FWD_CORE wxImage;
class WXDLLIMPEXP_FWD_CORE wxPalette;

/*
 * wxQuantize
 * Based on the JPEG quantization code. Reduces the number of colours in a wxImage.
 */

#define wxQUANTIZE_INCLUDE_WINDOWS_COLOURS      0x01
#define wxQUANTIZE_RETURN_8BIT_DATA             0x02
#define wxQUANTIZE_FILL_DESTINATION_IMAGE       0x04

class WXDLLEXPORT wxQuantize: public wxObject
{
public:
DECLARE_DYNAMIC_CLASS(wxQuantize)

//// Constructor

    wxQuantize() {}
    virtual ~wxQuantize() {}

//// Operations

    // Reduce the colours in the source image and put the result into the
    // destination image. Both images may be the same, to overwrite the source image.
    // Specify an optional palette pointer to receive the resulting palette.
    // This palette may be passed to ConvertImageToBitmap, for example.
    // If you pass a palette pointer, you must free the palette yourself.

    static bool Quantize(const wxImage& src, wxImage& dest, wxPalette** pPalette, int desiredNoColours = 236,
        unsigned char** eightBitData = 0, int flags = wxQUANTIZE_INCLUDE_WINDOWS_COLOURS|wxQUANTIZE_FILL_DESTINATION_IMAGE|wxQUANTIZE_RETURN_8BIT_DATA);

    // This version sets a palette in the destination image so you don't
    // have to manage it yourself.

    static bool Quantize(const wxImage& src, wxImage& dest, int desiredNoColours = 236,
        unsigned char** eightBitData = 0, int flags = wxQUANTIZE_INCLUDE_WINDOWS_COLOURS|wxQUANTIZE_FILL_DESTINATION_IMAGE|wxQUANTIZE_RETURN_8BIT_DATA);

//// Helpers

    // Converts input bitmap(s) into 8bit representation with custom palette

    // in_rows and out_rows are arrays [0..h-1] of pointer to rows
    // (in_rows contains w * 3 bytes per row, out_rows w bytes per row)
    // fills out_rows with indexes into palette (which is also stored into palette variable)
    static void DoQuantize(unsigned w, unsigned h, unsigned char **in_rows, unsigned char **out_rows, unsigned char *palette, int desiredNoColours);

};

#endif
    // _WX_QUANTIZE_H_
