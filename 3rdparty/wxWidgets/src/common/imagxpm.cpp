/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagxpm.cpp
// Purpose:     wxXPMHandler
// Author:      Vaclav Slavik, Robert Roebling
// RCS-ID:      $Id: imagxpm.cpp 53477 2008-05-07 07:28:57Z JS $
// Copyright:   (c) 2001 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*

This file is partially based on source code of ImageMagick by John Cristy. Its
license is as follows:

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            X   X  PPPP   M   M                              %
%                             X X   P   P  MM MM                              %
%                              X    PPPP   M M M                              %
%                             X X   P      M   M                              %
%                            X   X  P      M   M                              %
%                                                                             %
%                                                                             %
%                    Read/Write ImageMagick Image Format.                     %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright (C) 2001 ImageMagick Studio, a non-profit organization dedicated %
%  to making software imaging solutions freely available.                     %
%                                                                             %
%  Permission is hereby granted, free of charge, to any person obtaining a    %
%  copy of this software and associated documentation files ("ImageMagick"),  %
%  to deal in ImageMagick without restriction, including without limitation   %
%  the rights to use, copy, modify, merge, publish, distribute, sublicense,   %
%  and/or sell copies of ImageMagick, and to permit persons to whom the       %
%  ImageMagick is furnished to do so, subject to the following conditions:    %
%                                                                             %
%  The above copyright notice and this permission notice shall be included in %
%  all copies or substantial portions of ImageMagick.                         %
%                                                                             %
%  The software is provided "as is", without warranty of any kind, express or %
%  implied, including but not limited to the warranties of merchantability,   %
%  fitness for a particular purpose and noninfringement.  In no event shall   %
%  ImageMagick Studio be liable for any claim, damages or other liability,    %
%  whether in an action of contract, tort or otherwise, arising from, out of  %
%  or in connection with ImageMagick or the use or other dealings in          %
%  ImageMagick.                                                               %
%                                                                             %
%  Except as contained in this notice, the name of the ImageMagick Studio     %
%  shall not be used in advertising or otherwise to promote the sale, use or  %
%  other dealings in ImageMagick without prior written authorization from the %
%  ImageMagick Studio.                                                        %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XPM

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/utils.h"
#endif

#include "wx/imagxpm.h"
#include "wx/wfstream.h"
#include "wx/xpmdecod.h"

IMPLEMENT_DYNAMIC_CLASS(wxXPMHandler,wxImageHandler)

//-----------------------------------------------------------------------------
// wxXPMHandler
//-----------------------------------------------------------------------------

#if wxUSE_STREAMS

bool wxXPMHandler::LoadFile(wxImage *image,
                            wxInputStream& stream,
                            bool WXUNUSED(verbose), int WXUNUSED(index))
{
    wxXPMDecoder decoder;

    wxImage img = decoder.ReadFile(stream);
    if ( !img.Ok() )
        return false;
    *image = img;
    return true;
}

bool wxXPMHandler::SaveFile(wxImage * image,
                            wxOutputStream& stream, bool WXUNUSED(verbose))
{
    // 1. count colours:
    #define MaxCixels  92
    static const char Cixel[MaxCixels+1] =
                         " .XoO+@#$%&*=-;:>,<1234567890qwertyuipasdfghjk"
                         "lzxcvbnmMNBVCZASDFGHJKLPIUYTREWQ!~^/()_`'][{}|";
    int i, j, k;

    wxImageHistogram histogram;
    int cols = int(image->ComputeHistogram(histogram));

    int chars_per_pixel = 1;
    for ( k = MaxCixels; cols > k; k *= MaxCixels)
        chars_per_pixel++;

    // 2. write the header:
    wxString sName;
    if ( image->HasOption(wxIMAGE_OPTION_FILENAME) )
    {
        wxSplitPath(image->GetOption(wxIMAGE_OPTION_FILENAME),
                    NULL, &sName, NULL);
        sName << wxT("_xpm");
    }

    if ( !sName.empty() )
        sName = wxString(wxT("/* XPM */\nstatic const char *")) + sName;
    else
        sName = wxT("/* XPM */\nstatic const char *xpm_data");
    stream.Write( (const char*) sName.ToAscii(), sName.Len() );

    char tmpbuf[200];
    // VS: 200b is safe upper bound for anything produced by sprintf below
    //     (<101 bytes the string, neither %i can expand into more than 10 chars)
    sprintf(tmpbuf,
               "[] = {\n"
               "/* columns rows colors chars-per-pixel */\n"
               "\"%i %i %i %i\",\n",
               image->GetWidth(), image->GetHeight(), cols, chars_per_pixel);
    stream.Write(tmpbuf, strlen(tmpbuf));

    // 3. create color symbols table:
    char *symbols_data = new char[cols * (chars_per_pixel+1)];
    char **symbols = new char*[cols];

    // 2a. find mask colour:
    unsigned long mask_key = 0x1000000 /*invalid RGB value*/;
    if (image->HasMask())
        mask_key = (image->GetMaskRed() << 16) |
                   (image->GetMaskGreen() << 8) | image->GetMaskBlue();

    // 2b. generate colour table:
    for (wxImageHistogram::iterator entry = histogram.begin();
         entry != histogram.end(); ++entry )
    {
        unsigned long index = entry->second.index;
        symbols[index] = symbols_data + index * (chars_per_pixel+1);
        char *sym = symbols[index];

        for (j = 0; j < chars_per_pixel; j++)
        {
            sym[j] = Cixel[index % MaxCixels];
            index /= MaxCixels;
        }
        sym[j] = '\0';

        unsigned long key = entry->first;

        if (key == 0)
            sprintf( tmpbuf, "\"%s c Black\",\n", sym);
        else if (key == mask_key)
            sprintf( tmpbuf, "\"%s c None\",\n", sym);
        else
        {
            wxByte r = wxByte(key >> 16);
            wxByte g = wxByte(key >> 8);
            wxByte b = wxByte(key);
            sprintf(tmpbuf, "\"%s c #%02X%02X%02X\",\n", sym, r, g, b);
        }
        stream.Write( tmpbuf, strlen(tmpbuf) );
    }

    stream.Write("/* pixels */\n", 13);

    unsigned char *data = image->GetData();
    for (j = 0; j < image->GetHeight(); j++)
    {
        char tmp_c;
        tmp_c = '\"'; stream.Write(&tmp_c, 1);
        for (i = 0; i < image->GetWidth(); i++, data += 3)
        {
            unsigned long key = (data[0] << 16) | (data[1] << 8) | (data[2]);
            stream.Write(symbols[histogram[key].index], chars_per_pixel);
        }
        tmp_c = '\"'; stream.Write(&tmp_c, 1);
        if ( j + 1 < image->GetHeight() )
        {
            tmp_c = ','; stream.Write(&tmp_c, 1);
        }
        tmp_c = '\n'; stream.Write(&tmp_c, 1);
    }
    stream.Write("};\n", 3 );

    // Clean up:
    delete[] symbols;
    delete[] symbols_data;

    return true;
}

bool wxXPMHandler::DoCanRead(wxInputStream& stream)
{
    wxXPMDecoder decoder;
    return decoder.CanRead(stream);
}

#endif  // wxUSE_STREAMS

#endif // wxUSE_XPM
