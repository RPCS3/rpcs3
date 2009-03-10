/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/xpmdecod.cpp
// Purpose:     wxXPMDecoder
// Author:      John Cristy, Vaclav Slavik
// RCS-ID:      $Id: xpmdecod.cpp 41689 2006-10-08 08:04:49Z PC $
// Copyright:   (c) John Cristy, Vaclav Slavik
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

/*
 * Also contains some pieces from libxpm and its modification for win32 by
 * HeDu <hedu@cul-ipn.uni-kiel.de>:
 *
 * Copyright (C) 1989-95 GROUPE BULL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * GROUPE BULL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of GROUPE BULL shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from GROUPE BULL.
 */

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && wxUSE_XPM

#include "wx/xpmdecod.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/utils.h"
    #include "wx/hashmap.h"
    #include "wx/stream.h"
    #include "wx/image.h"
#endif

#include <string.h>
#include <ctype.h>

#if wxUSE_STREAMS
bool wxXPMDecoder::CanRead(wxInputStream& stream)
{
    unsigned char buf[9];

    if ( !stream.Read(buf, WXSIZEOF(buf)) )
        return false;

    stream.SeekI(-(wxFileOffset)WXSIZEOF(buf), wxFromCurrent);

    return memcmp(buf, "/* XPM */", WXSIZEOF(buf)) == 0;
}

wxImage wxXPMDecoder::ReadFile(wxInputStream& stream)
{
    size_t length = stream.GetSize();
    wxCHECK_MSG( length != 0, wxNullImage,
                 wxT("Cannot read XPM from stream of unknown size") );

    // use a smart buffer to be sure to free memory even when we return on
    // error
    wxCharBuffer buffer(length);

    char *xpm_buffer = (char *)buffer.data();
    if ( stream.Read(xpm_buffer, length).GetLastError() == wxSTREAM_READ_ERROR )
        return wxNullImage;
    xpm_buffer[length] = '\0';

    /*
     *  Remove comments from the file:
     */
    char *p, *q;
    for (p = xpm_buffer; *p != '\0'; p++)
    {
        if ( (*p == '"') || (*p == '\'') )
        {
            if (*p == '"')
            {
              for (p++; *p != '\0'; p++)
                if ( (*p == '"') && (*(p - 1) != '\\') )
                    break;
            }
            else // *p == '\''
            {
                for (p++; *p != '\0'; p++)
                    if ( (*p == '\'') && (*(p - 1) != '\\') )
                        break;
            }
            if (*p == '\0')
                break;
            continue;
        }
        if ( (*p != '/') || (*(p + 1) != '*') )
            continue;
        for (q = p + 2; *q != '\0'; q++)
        {
            if ( (*q == '*') && (*(q + 1) == '/') )
                break;
        }

        // memmove allows overlaps (unlike strcpy):
        size_t cpylen = strlen(q + 2) + 1;
        memmove(p, q + 2, cpylen);
    }

    /*
     *  Remove unquoted characters:
     */
    size_t i = 0;
    for (p = xpm_buffer; *p != '\0'; p++)
    {
        if ( *p != '"' )
            continue;
        for (q = p + 1; *q != '\0'; q++)
            if (*q == '"')
                break;
        strncpy(xpm_buffer + i, p + 1, q - p - 1);
        i += q - p - 1;
        xpm_buffer[i++] = '\n';
        p = q + 1;
    }
    xpm_buffer[i] = '\0';

    /*
     *  Create array of lines and convert \n's to \0's:
     */
    const char **xpm_lines;
    size_t lines_cnt = 0;
    size_t line;

    for (p = xpm_buffer; *p != '\0'; p++)
    {
        if ( *p == '\n' )
            lines_cnt++;
    }

    if ( !lines_cnt )
    {
        // this doesn't really look an XPM image
        return wxNullImage;
    }

    xpm_lines = new const char*[lines_cnt + 1];
    xpm_lines[0] = xpm_buffer;
    line = 1;
    for (p = xpm_buffer; (*p != '\0') && (line < lines_cnt); p++)
    {
        if ( *p == '\n' )
        {
            xpm_lines[line] = p + 1;
            *p = '\0';
            line++;
        }
    }

    xpm_lines[lines_cnt] = NULL;

    /*
     *  Read the image:
     */
    wxImage img = ReadData(xpm_lines);

    delete[] xpm_lines;

    return img;
}
#endif // wxUSE_STREAMS


/*****************************************************************************\
* rgbtab.h                                                                    *
*                                                                             *
* A hard coded rgb.txt. To keep it short I removed all colornames with        *
* trailing numbers, Blue3 etc, except the GrayXX. Sorry Grey-lovers I prefer  *
* Gray ;-). But Grey is recognized on lookups, only on save Gray will be      *
* used, maybe you want to do some substitue there too.                        *
*                                                                             *
* To save memory the RGBs are coded in one long value, as done by the RGB     *
* macro.                                                                      *
*                                                                             *
* Developed by HeDu 3/94 (hedu@cul-ipn.uni-kiel.de)                           *
\*****************************************************************************/


typedef struct
{
    const char *name;
    wxUint32 rgb;
} rgbRecord;

#define myRGB(r,g,b)   ((wxUint32)r<<16|(wxUint32)g<<8|(wxUint32)b)

static rgbRecord theRGBRecords[] =
{
    {"aliceblue", myRGB(240, 248, 255)},
    {"antiquewhite", myRGB(250, 235, 215)},
    {"aquamarine", myRGB(50, 191, 193)},
    {"azure", myRGB(240, 255, 255)},
    {"beige", myRGB(245, 245, 220)},
    {"bisque", myRGB(255, 228, 196)},
    {"black", myRGB(0, 0, 0)},
    {"blanchedalmond", myRGB(255, 235, 205)},
    {"blue", myRGB(0, 0, 255)},
    {"blueviolet", myRGB(138, 43, 226)},
    {"brown", myRGB(165, 42, 42)},
    {"burlywood", myRGB(222, 184, 135)},
    {"cadetblue", myRGB(95, 146, 158)},
    {"chartreuse", myRGB(127, 255, 0)},
    {"chocolate", myRGB(210, 105, 30)},
    {"coral", myRGB(255, 114, 86)},
    {"cornflowerblue", myRGB(34, 34, 152)},
    {"cornsilk", myRGB(255, 248, 220)},
    {"cyan", myRGB(0, 255, 255)},
    {"darkgoldenrod", myRGB(184, 134, 11)},
    {"darkgreen", myRGB(0, 86, 45)},
    {"darkkhaki", myRGB(189, 183, 107)},
    {"darkolivegreen", myRGB(85, 86, 47)},
    {"darkorange", myRGB(255, 140, 0)},
    {"darkorchid", myRGB(139, 32, 139)},
    {"darksalmon", myRGB(233, 150, 122)},
    {"darkseagreen", myRGB(143, 188, 143)},
    {"darkslateblue", myRGB(56, 75, 102)},
    {"darkslategray", myRGB(47, 79, 79)},
    {"darkturquoise", myRGB(0, 166, 166)},
    {"darkviolet", myRGB(148, 0, 211)},
    {"deeppink", myRGB(255, 20, 147)},
    {"deepskyblue", myRGB(0, 191, 255)},
    {"dimgray", myRGB(84, 84, 84)},
    {"dodgerblue", myRGB(30, 144, 255)},
    {"firebrick", myRGB(142, 35, 35)},
    {"floralwhite", myRGB(255, 250, 240)},
    {"forestgreen", myRGB(80, 159, 105)},
    {"gainsboro", myRGB(220, 220, 220)},
    {"ghostwhite", myRGB(248, 248, 255)},
    {"gold", myRGB(218, 170, 0)},
    {"goldenrod", myRGB(239, 223, 132)},
    {"gray", myRGB(126, 126, 126)},
    {"gray0", myRGB(0, 0, 0)},
    {"gray1", myRGB(3, 3, 3)},
    {"gray10", myRGB(26, 26, 26)},
    {"gray100", myRGB(255, 255, 255)},
    {"gray11", myRGB(28, 28, 28)},
    {"gray12", myRGB(31, 31, 31)},
    {"gray13", myRGB(33, 33, 33)},
    {"gray14", myRGB(36, 36, 36)},
    {"gray15", myRGB(38, 38, 38)},
    {"gray16", myRGB(41, 41, 41)},
    {"gray17", myRGB(43, 43, 43)},
    {"gray18", myRGB(46, 46, 46)},
    {"gray19", myRGB(48, 48, 48)},
    {"gray2", myRGB(5, 5, 5)},
    {"gray20", myRGB(51, 51, 51)},
    {"gray21", myRGB(54, 54, 54)},
    {"gray22", myRGB(56, 56, 56)},
    {"gray23", myRGB(59, 59, 59)},
    {"gray24", myRGB(61, 61, 61)},
    {"gray25", myRGB(64, 64, 64)},
    {"gray26", myRGB(66, 66, 66)},
    {"gray27", myRGB(69, 69, 69)},
    {"gray28", myRGB(71, 71, 71)},
    {"gray29", myRGB(74, 74, 74)},
    {"gray3", myRGB(8, 8, 8)},
    {"gray30", myRGB(77, 77, 77)},
    {"gray31", myRGB(79, 79, 79)},
    {"gray32", myRGB(82, 82, 82)},
    {"gray33", myRGB(84, 84, 84)},
    {"gray34", myRGB(87, 87, 87)},
    {"gray35", myRGB(89, 89, 89)},
    {"gray36", myRGB(92, 92, 92)},
    {"gray37", myRGB(94, 94, 94)},
    {"gray38", myRGB(97, 97, 97)},
    {"gray39", myRGB(99, 99, 99)},
    {"gray4", myRGB(10, 10, 10)},
    {"gray40", myRGB(102, 102, 102)},
    {"gray41", myRGB(105, 105, 105)},
    {"gray42", myRGB(107, 107, 107)},
    {"gray43", myRGB(110, 110, 110)},
    {"gray44", myRGB(112, 112, 112)},
    {"gray45", myRGB(115, 115, 115)},
    {"gray46", myRGB(117, 117, 117)},
    {"gray47", myRGB(120, 120, 120)},
    {"gray48", myRGB(122, 122, 122)},
    {"gray49", myRGB(125, 125, 125)},
    {"gray5", myRGB(13, 13, 13)},
    {"gray50", myRGB(127, 127, 127)},
    {"gray51", myRGB(130, 130, 130)},
    {"gray52", myRGB(133, 133, 133)},
    {"gray53", myRGB(135, 135, 135)},
    {"gray54", myRGB(138, 138, 138)},
    {"gray55", myRGB(140, 140, 140)},
    {"gray56", myRGB(143, 143, 143)},
    {"gray57", myRGB(145, 145, 145)},
    {"gray58", myRGB(148, 148, 148)},
    {"gray59", myRGB(150, 150, 150)},
    {"gray6", myRGB(15, 15, 15)},
    {"gray60", myRGB(153, 153, 153)},
    {"gray61", myRGB(156, 156, 156)},
    {"gray62", myRGB(158, 158, 158)},
    {"gray63", myRGB(161, 161, 161)},
    {"gray64", myRGB(163, 163, 163)},
    {"gray65", myRGB(166, 166, 166)},
    {"gray66", myRGB(168, 168, 168)},
    {"gray67", myRGB(171, 171, 171)},
    {"gray68", myRGB(173, 173, 173)},
    {"gray69", myRGB(176, 176, 176)},
    {"gray7", myRGB(18, 18, 18)},
    {"gray70", myRGB(179, 179, 179)},
    {"gray71", myRGB(181, 181, 181)},
    {"gray72", myRGB(184, 184, 184)},
    {"gray73", myRGB(186, 186, 186)},
    {"gray74", myRGB(189, 189, 189)},
    {"gray75", myRGB(191, 191, 191)},
    {"gray76", myRGB(194, 194, 194)},
    {"gray77", myRGB(196, 196, 196)},
    {"gray78", myRGB(199, 199, 199)},
    {"gray79", myRGB(201, 201, 201)},
    {"gray8", myRGB(20, 20, 20)},
    {"gray80", myRGB(204, 204, 204)},
    {"gray81", myRGB(207, 207, 207)},
    {"gray82", myRGB(209, 209, 209)},
    {"gray83", myRGB(212, 212, 212)},
    {"gray84", myRGB(214, 214, 214)},
    {"gray85", myRGB(217, 217, 217)},
    {"gray86", myRGB(219, 219, 219)},
    {"gray87", myRGB(222, 222, 222)},
    {"gray88", myRGB(224, 224, 224)},
    {"gray89", myRGB(227, 227, 227)},
    {"gray9", myRGB(23, 23, 23)},
    {"gray90", myRGB(229, 229, 229)},
    {"gray91", myRGB(232, 232, 232)},
    {"gray92", myRGB(235, 235, 235)},
    {"gray93", myRGB(237, 237, 237)},
    {"gray94", myRGB(240, 240, 240)},
    {"gray95", myRGB(242, 242, 242)},
    {"gray96", myRGB(245, 245, 245)},
    {"gray97", myRGB(247, 247, 247)},
    {"gray98", myRGB(250, 250, 250)},
    {"gray99", myRGB(252, 252, 252)},
    {"green", myRGB(0, 255, 0)},
    {"greenyellow", myRGB(173, 255, 47)},
    {"honeydew", myRGB(240, 255, 240)},
    {"hotpink", myRGB(255, 105, 180)},
    {"indianred", myRGB(107, 57, 57)},
    {"ivory", myRGB(255, 255, 240)},
    {"khaki", myRGB(179, 179, 126)},
    {"lavender", myRGB(230, 230, 250)},
    {"lavenderblush", myRGB(255, 240, 245)},
    {"lawngreen", myRGB(124, 252, 0)},
    {"lemonchiffon", myRGB(255, 250, 205)},
    {"lightblue", myRGB(176, 226, 255)},
    {"lightcoral", myRGB(240, 128, 128)},
    {"lightcyan", myRGB(224, 255, 255)},
    {"lightgoldenrod", myRGB(238, 221, 130)},
    {"lightgoldenrodyellow", myRGB(250, 250, 210)},
    {"lightgray", myRGB(168, 168, 168)},
    {"lightpink", myRGB(255, 182, 193)},
    {"lightsalmon", myRGB(255, 160, 122)},
    {"lightseagreen", myRGB(32, 178, 170)},
    {"lightskyblue", myRGB(135, 206, 250)},
    {"lightslateblue", myRGB(132, 112, 255)},
    {"lightslategray", myRGB(119, 136, 153)},
    {"lightsteelblue", myRGB(124, 152, 211)},
    {"lightyellow", myRGB(255, 255, 224)},
    {"limegreen", myRGB(0, 175, 20)},
    {"linen", myRGB(250, 240, 230)},
    {"magenta", myRGB(255, 0, 255)},
    {"maroon", myRGB(143, 0, 82)},
    {"mediumaquamarine", myRGB(0, 147, 143)},
    {"mediumblue", myRGB(50, 50, 204)},
    {"mediumforestgreen", myRGB(50, 129, 75)},
    {"mediumgoldenrod", myRGB(209, 193, 102)},
    {"mediumorchid", myRGB(189, 82, 189)},
    {"mediumpurple", myRGB(147, 112, 219)},
    {"mediumseagreen", myRGB(52, 119, 102)},
    {"mediumslateblue", myRGB(106, 106, 141)},
    {"mediumspringgreen", myRGB(35, 142, 35)},
    {"mediumturquoise", myRGB(0, 210, 210)},
    {"mediumvioletred", myRGB(213, 32, 121)},
    {"midnightblue", myRGB(47, 47, 100)},
    {"mintcream", myRGB(245, 255, 250)},
    {"mistyrose", myRGB(255, 228, 225)},
    {"moccasin", myRGB(255, 228, 181)},
    {"navajowhite", myRGB(255, 222, 173)},
    {"navy", myRGB(35, 35, 117)},
    {"navyblue", myRGB(35, 35, 117)},
    {"oldlace", myRGB(253, 245, 230)},
    {"olivedrab", myRGB(107, 142, 35)},
    {"orange", myRGB(255, 135, 0)},
    {"orangered", myRGB(255, 69, 0)},
    {"orchid", myRGB(239, 132, 239)},
    {"palegoldenrod", myRGB(238, 232, 170)},
    {"palegreen", myRGB(115, 222, 120)},
    {"paleturquoise", myRGB(175, 238, 238)},
    {"palevioletred", myRGB(219, 112, 147)},
    {"papayawhip", myRGB(255, 239, 213)},
    {"peachpuff", myRGB(255, 218, 185)},
    {"peru", myRGB(205, 133, 63)},
    {"pink", myRGB(255, 181, 197)},
    {"plum", myRGB(197, 72, 155)},
    {"powderblue", myRGB(176, 224, 230)},
    {"purple", myRGB(160, 32, 240)},
    {"red", myRGB(255, 0, 0)},
    {"rosybrown", myRGB(188, 143, 143)},
    {"royalblue", myRGB(65, 105, 225)},
    {"saddlebrown", myRGB(139, 69, 19)},
    {"salmon", myRGB(233, 150, 122)},
    {"sandybrown", myRGB(244, 164, 96)},
    {"seagreen", myRGB(82, 149, 132)},
    {"seashell", myRGB(255, 245, 238)},
    {"sienna", myRGB(150, 82, 45)},
    {"silver", myRGB(192, 192, 192)},
    {"skyblue", myRGB(114, 159, 255)},
    {"slateblue", myRGB(126, 136, 171)},
    {"slategray", myRGB(112, 128, 144)},
    {"snow", myRGB(255, 250, 250)},
    {"springgreen", myRGB(65, 172, 65)},
    {"steelblue", myRGB(84, 112, 170)},
    {"tan", myRGB(222, 184, 135)},
    {"thistle", myRGB(216, 191, 216)},
    {"tomato", myRGB(255, 99, 71)},
    {"transparent", myRGB(0, 0, 1)},
    {"turquoise", myRGB(25, 204, 223)},
    {"violet", myRGB(156, 62, 206)},
    {"violetred", myRGB(243, 62, 150)},
    {"wheat", myRGB(245, 222, 179)},
    {"white", myRGB(255, 255, 255)},
    {"whitesmoke", myRGB(245, 245, 245)},
    {"yellow", myRGB(255, 255, 0)},
    {"yellowgreen", myRGB(50, 216, 56)},
    {NULL, myRGB(0, 0, 0)}
};
static int numTheRGBRecords = 235;

static unsigned char ParseHexadecimal(char digit1, char digit2)
{
    unsigned char i1, i2;

    if (digit1 >= 'a')
        i1 = (unsigned char)(digit1 - 'a' + 0x0A);
    else if (digit1 >= 'A')
        i1 = (unsigned char)(digit1 - 'A' + 0x0A);
    else
        i1 = (unsigned char)(digit1 - '0');
    if (digit2 >= 'a')
        i2 = (unsigned char)(digit2 - 'a' + 0x0A);
    else if (digit2 >= 'A')
        i2 = (unsigned char)(digit2 - 'A' + 0x0A);
    else
        i2 = (unsigned char)(digit2 - '0');
    return (unsigned char)(0x10 * i1 + i2);
}

static bool GetRGBFromName(const char *inname, bool *isNone,
                           unsigned char *r, unsigned char*g, unsigned char *b)
{
    int left, right, middle;
    int cmp;
    wxUint32 rgbVal;
    char *name;
    char *grey, *p;

    // Neither #rrggbb nor #rrrrggggbbbb are in database, we parse them directly
    size_t inname_len = strlen(inname);
    if ( *inname == '#' && (inname_len == 7 || inname_len == 13))
    {
        size_t ofs = (inname_len == 7) ? 2 : 4;
        *r = ParseHexadecimal(inname[1], inname[2]);
        *g = ParseHexadecimal(inname[1*ofs+1], inname[1*ofs+2]);
        *b = ParseHexadecimal(inname[2*ofs+1], inname[2*ofs+2]);
        *isNone = false;
        return true;
    }

    name = wxStrdupA(inname);

    // theRGBRecords[] has no names with spaces, and no grey, but a
    // lot of gray...

    // so first extract ' '
    while ((p = strchr(name, ' ')) != NULL)
    {
        while (*(p))            // till eof of string
        {
            *p = *(p + 1);      // copy to the left
            p++;
        }
    }
    // fold to lower case
    p = name;
    while (*p)
    {
        *p = (char)tolower(*p);
        p++;
    }

    // substitute Grey with Gray, else rgbtab.h would have more than 100
    // 'duplicate' entries
    if ( (grey = strstr(name, "grey")) != NULL )
        grey[2] = 'a';

    // check for special 'none' colour:
    bool found;
    if ( strcmp(name, "none") == 0 )
    {
        *isNone = true;
        found = true;
    }
    else // not "None"
    {
        found = false;

        // binary search:
        left = 0;
        right = numTheRGBRecords - 1;
        do
        {
            middle = (left + right) / 2;
            cmp = strcmp(name, theRGBRecords[middle].name);
            if ( cmp == 0 )
            {
                rgbVal = theRGBRecords[middle].rgb;
                *r = (unsigned char)((rgbVal >> 16) & 0xFF);
                *g = (unsigned char)((rgbVal >> 8) & 0xFF);
                *b = (unsigned char)((rgbVal) & 0xFF);
                *isNone = false;
                found = true;
                break;
            }
            else if ( cmp < 0 )
            {
                right = middle - 1;
            }
            else // cmp > 0
            {
                left = middle + 1;
            }
        } while (left <= right);
    }

    free(name);

    return found;
}

static const char *ParseColor(const char *data)
{
    static const char *targets[] =
                        {"c ", "g ", "g4 ", "m ", "b ", "s ", NULL};

    const char *p, *r;
    const char *q;
    int i;

    for (i = 0; targets[i] != NULL; i++)
    {
        r = data;
        for (q = targets[i]; *r != '\0'; r++)
        {
            if ( *r != *q )
                continue;
            if ( !isspace((int) (*(r - 1))) )
                continue;
            p = r;
            for (;;)
            {
                if ( *q == '\0' )
                    return p;
                if ( *p++ != *q++ )
                    break;
            }
            q = targets[i];
        }
    }
    return NULL;
}

struct wxXPMColourMapData
{
    wxXPMColourMapData() { R = G = B = 0; }
    unsigned char R,G,B;
};
WX_DECLARE_STRING_HASH_MAP(wxXPMColourMapData, wxXPMColourMap);

wxImage wxXPMDecoder::ReadData(const char* const* xpm_data)
{
    wxCHECK_MSG(xpm_data, wxNullImage, wxT("NULL XPM data") );

    wxImage img;
    int count;
    unsigned width, height, colors_cnt, chars_per_pixel;
    size_t i, j, i_key;
    wxChar key[64];
    const char *clr_def;
    bool hasMask;
    wxXPMColourMap clr_tbl;
    wxXPMColourMap::iterator it;
    wxString maskKey;

    /*
     *  Read hints and initialize structures:
     */

    count = sscanf(xpm_data[0], "%u %u %u %u",
                   &width, &height, &colors_cnt, &chars_per_pixel);
    if ( count != 4 || width * height * colors_cnt == 0 )
    {
        wxLogError(_("XPM: incorrect header format!"));
        return wxNullImage;
    }

    // VS: XPM color map this large would be insane, since XPMs are encoded with
    //     92 possible values on each position, 92^64 is *way* larger space than
    //     8bit RGB...
    wxCHECK_MSG(chars_per_pixel < 64, wxNullImage, wxT("XPM colormaps this large not supported."));

    if ( !img.Create(width, height) )
        return wxNullImage;

    img.SetMask(false);
    key[chars_per_pixel] = wxT('\0');
    hasMask = false;

    /*
     *  Create colour map:
     */
    wxXPMColourMapData clr_data;
    for (i = 0; i < colors_cnt; i++)
    {
        const char *xmpColLine = xpm_data[1 + i];

        // we must have at least " x y" after the colour index, hence +5
        if ( !xmpColLine || strlen(xmpColLine) < chars_per_pixel + 5 )
        {
            wxLogError(_("XPM: incorrect colour description in line %d"),
                       (int)(1 + i));
            return wxNullImage;
        }

        for (i_key = 0; i_key < chars_per_pixel; i_key++)
            key[i_key] = (wxChar)xmpColLine[i_key];
        clr_def = ParseColor(xmpColLine + chars_per_pixel);

        if ( clr_def == NULL )
        {
            wxLogError(_("XPM: malformed colour definition '%s' at line %d!"),
                       xmpColLine, (int)(1 + i));
            return wxNullImage;
        }

        bool isNone = false;
        if ( !GetRGBFromName(clr_def, &isNone,
                             &clr_data.R, &clr_data.G, &clr_data.B) )
        {
            wxLogError(_("XPM: malformed colour definition '%s' at line %d!"),
                       xmpColLine, (int)(1 + i));
            return wxNullImage;
        }

        if ( isNone )
        {
            img.SetMask(true);
            img.SetMaskColour(255, 0, 255);
            clr_data.R =
            clr_data.B = 255;
            clr_data.G = 0;
            hasMask = true;
            maskKey = key;
        }

        clr_tbl[key] = clr_data;
    }

    /*
     *  Modify colour entries with RGB = (255,0,255) to (255,0,254) if
     *  mask colour is present (so that existing pixels with (255,0,255)
     *  magenta colour are not incorrectly made transparent):
     */
    if (hasMask)
    {
        for (it = clr_tbl.begin(); it != clr_tbl.end(); ++it)
        {
            if (it->second.R == 255 && it->second.G == 0 &&
                it->second.B == 255 &&
                it->first != maskKey)
            {
                it->second.B = 254;
            }
        }
    }

    /*
     *  Parse image data:
     */

    unsigned char *img_data = img.GetData();
    wxXPMColourMap::iterator entry;
    wxXPMColourMap::iterator end = clr_tbl.end();

    for (j = 0; j < height; j++)
    {
        for (i = 0; i < width; i++, img_data += 3)
        {
            const char *xpmImgLine = xpm_data[1 + colors_cnt + j];
            if ( !xpmImgLine || strlen(xpmImgLine) < width*chars_per_pixel )
            {
                wxLogError(_("XPM: truncated image data at line %d!"),
                           (int)(1 + colors_cnt + j));
                return wxNullImage;
            }

            for (i_key = 0; i_key < chars_per_pixel; i_key++)
            {
                key[i_key] = (wxChar)xpmImgLine[chars_per_pixel * i + i_key];
            }

            entry = clr_tbl.find(key);
            if ( entry == end )
            {
                wxLogError(_("XPM: Malformed pixel data!"));

                // better return right now as otherwise we risk to flood the
                // user with error messages as something seems to be seriously
                // wrong with the file and so we could give this message for
                // each remaining pixel if we don't bail out
                return wxNullImage;
            }
            else
            {
                img_data[0] = entry->second.R;
                img_data[1] = entry->second.G;
                img_data[2] = entry->second.B;
            }
        }
    }

    return img;
}

#endif // wxUSE_IMAGE && wxUSE_XPM
