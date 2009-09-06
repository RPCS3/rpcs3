/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/dcpsg.cpp
// Purpose:     Generic wxPostScriptDC implementation
// Author:      Julian Smart, Robert Roebling, Markus Holzhem
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: dcpsg.cpp 55927 2008-09-28 09:12:16Z VS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_PRINTING_ARCHITECTURE && wxUSE_POSTSCRIPT

#include "wx/generic/dcpsg.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/utils.h"
    #include "wx/dcmemory.h"
    #include "wx/math.h"
    #include "wx/image.h"
    #include "wx/icon.h"
#endif // WX_PRECOMP

#include "wx/prntbase.h"
#include "wx/generic/prntdlgg.h"
#include "wx/paper.h"
#include "wx/filefn.h"
#include "wx/stdpaths.h"

WXDLLIMPEXP_DATA_CORE(int) wxPageNumber;

#ifdef __WXMSW__

#ifdef DrawText
#undef DrawText
#endif

#ifdef StartDoc
#undef StartDoc
#endif

#ifdef GetCharWidth
#undef GetCharWidth
#endif

#ifdef FindWindow
#undef FindWindow
#endif

#endif

//-----------------------------------------------------------------------------
// start and end of document/page
//-----------------------------------------------------------------------------

static const char *wxPostScriptHeaderConicTo = "\
/conicto {\n\
    /to_y exch def\n\
    /to_x exch def\n\
    /conic_cntrl_y exch def\n\
    /conic_cntrl_x exch def\n\
    currentpoint\n\
    /p0_y exch def\n\
    /p0_x exch def\n\
    /p1_x p0_x conic_cntrl_x p0_x sub 2 3 div mul add def\n\
    /p1_y p0_y conic_cntrl_y p0_y sub 2 3 div mul add def\n\
    /p2_x p1_x to_x p0_x sub 1 3 div mul add def\n\
    /p2_y p1_y to_y p0_y sub 1 3 div mul add def\n\
    p1_x p1_y p2_x p2_y to_x to_y curveto\n\
}  bind def\n\
";

static const char *wxPostScriptHeaderEllipse = "\
/ellipsedict 8 dict def\n\
ellipsedict /mtrx matrix put\n\
/ellipse {\n\
    ellipsedict begin\n\
    /endangle exch def\n\
    /startangle exch def\n\
    /yrad exch def\n\
    /xrad exch def\n\
    /y exch def\n\
    /x exch def\n\
    /savematrix mtrx currentmatrix def\n\
    x y translate\n\
    xrad yrad scale\n\
    0 0 1 startangle endangle arc\n\
    savematrix setmatrix\n\
    end\n\
    } def\n\
";

static const char *wxPostScriptHeaderEllipticArc= "\
/ellipticarcdict 8 dict def\n\
ellipticarcdict /mtrx matrix put\n\
/ellipticarc\n\
{ ellipticarcdict begin\n\
  /do_fill exch def\n\
  /endangle exch def\n\
  /startangle exch def\n\
  /yrad exch def\n\
  /xrad exch def \n\
  /y exch def\n\
  /x exch def\n\
  /savematrix mtrx currentmatrix def\n\
  x y translate\n\
  xrad yrad scale\n\
  do_fill { 0 0 moveto } if\n\
  0 0 1 startangle endangle arc\n\
  savematrix setmatrix\n\
  do_fill { fill }{ stroke } ifelse\n\
  end\n\
} def\n";

static const char *wxPostScriptHeaderSpline = "\
/DrawSplineSection {\n\
    /y3 exch def\n\
    /x3 exch def\n\
    /y2 exch def\n\
    /x2 exch def\n\
    /y1 exch def\n\
    /x1 exch def\n\
    /xa x1 x2 x1 sub 0.666667 mul add def\n\
    /ya y1 y2 y1 sub 0.666667 mul add def\n\
    /xb x3 x2 x3 sub 0.666667 mul add def\n\
    /yb y3 y2 y3 sub 0.666667 mul add def\n\
    x1 y1 lineto\n\
    xa ya xb yb x3 y3 curveto\n\
    } def\n\
";

static const char *wxPostScriptHeaderColourImage = "\
% define 'colorimage' if it isn't defined\n\
%   ('colortogray' and 'mergeprocs' come from xwd2ps\n\
%     via xgrab)\n\
/colorimage where   % do we know about 'colorimage'?\n\
  { pop }           % yes: pop off the 'dict' returned\n\
  {                 % no:  define one\n\
    /colortogray {  % define an RGB->I function\n\
      /rgbdata exch store    % call input 'rgbdata'\n\
      rgbdata length 3 idiv\n\
      /npixls exch store\n\
      /rgbindx 0 store\n\
      0 1 npixls 1 sub {\n\
        grays exch\n\
        rgbdata rgbindx       get 20 mul    % Red\n\
        rgbdata rgbindx 1 add get 32 mul    % Green\n\
        rgbdata rgbindx 2 add get 12 mul    % Blue\n\
        add add 64 idiv      % I = .5G + .31R + .18B\n\
        put\n\
        /rgbindx rgbindx 3 add store\n\
      } for\n\
      grays 0 npixls getinterval\n\
    } bind def\n\
\n\
    % Utility procedure for colorimage operator.\n\
    % This procedure takes two procedures off the\n\
    % stack and merges them into a single procedure.\n\
\n\
    /mergeprocs { % def\n\
      dup length\n\
      3 -1 roll\n\
      dup\n\
      length\n\
      dup\n\
      5 1 roll\n\
      3 -1 roll\n\
      add\n\
      array cvx\n\
      dup\n\
      3 -1 roll\n\
      0 exch\n\
      putinterval\n\
      dup\n\
      4 2 roll\n\
      putinterval\n\
    } bind def\n\
\n\
    /colorimage { % def\n\
      pop pop     % remove 'false 3' operands\n\
      {colortogray} mergeprocs\n\
      image\n\
    } bind def\n\
  } ifelse          % end of 'false' case\n\
";

static char wxPostScriptHeaderReencodeISO1[] =
    "\n/reencodeISO {\n"
"dup dup findfont dup length dict begin\n"
"{ 1 index /FID ne { def }{ pop pop } ifelse } forall\n"
"/Encoding ISOLatin1Encoding def\n"
"currentdict end definefont\n"
"} def\n"
"/ISOLatin1Encoding [\n"
"/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n"
"/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n"
"/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n"
"/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n"
"/space/exclam/quotedbl/numbersign/dollar/percent/ampersand/quoteright\n"
"/parenleft/parenright/asterisk/plus/comma/minus/period/slash\n"
"/zero/one/two/three/four/five/six/seven/eight/nine/colon/semicolon\n"
"/less/equal/greater/question/at/A/B/C/D/E/F/G/H/I/J/K/L/M/N\n"
"/O/P/Q/R/S/T/U/V/W/X/Y/Z/bracketleft/backslash/bracketright\n"
"/asciicircum/underscore/quoteleft/a/b/c/d/e/f/g/h/i/j/k/l/m\n"
"/n/o/p/q/r/s/t/u/v/w/x/y/z/braceleft/bar/braceright/asciitilde\n"
"/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n"
"/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n"
"/.notdef/dotlessi/grave/acute/circumflex/tilde/macron/breve\n"
"/dotaccent/dieresis/.notdef/ring/cedilla/.notdef/hungarumlaut\n";

static char wxPostScriptHeaderReencodeISO2[] =
"/ogonek/caron/space/exclamdown/cent/sterling/currency/yen/brokenbar\n"
"/section/dieresis/copyright/ordfeminine/guillemotleft/logicalnot\n"
"/hyphen/registered/macron/degree/plusminus/twosuperior/threesuperior\n"
"/acute/mu/paragraph/periodcentered/cedilla/onesuperior/ordmasculine\n"
"/guillemotright/onequarter/onehalf/threequarters/questiondown\n"
"/Agrave/Aacute/Acircumflex/Atilde/Adieresis/Aring/AE/Ccedilla\n"
"/Egrave/Eacute/Ecircumflex/Edieresis/Igrave/Iacute/Icircumflex\n"
"/Idieresis/Eth/Ntilde/Ograve/Oacute/Ocircumflex/Otilde/Odieresis\n"
"/multiply/Oslash/Ugrave/Uacute/Ucircumflex/Udieresis/Yacute\n"
"/Thorn/germandbls/agrave/aacute/acircumflex/atilde/adieresis\n"
"/aring/ae/ccedilla/egrave/eacute/ecircumflex/edieresis/igrave\n"
"/iacute/icircumflex/idieresis/eth/ntilde/ograve/oacute/ocircumflex\n"
"/otilde/odieresis/divide/oslash/ugrave/uacute/ucircumflex/udieresis\n"
"/yacute/thorn/ydieresis\n"
        "] def\n\n";

//-------------------------------------------------------------------------------
// wxPostScriptDC
//-------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxPostScriptDC, wxDC)

float wxPostScriptDC::ms_PSScaleFactor = 1.0;

void wxPostScriptDC::SetResolution(int ppi)
{
    ms_PSScaleFactor = (float)ppi / 72.0;
}

int wxPostScriptDC::GetResolution()
{
    return (int)(ms_PSScaleFactor * 72.0);
}

//-------------------------------------------------------------------------------

wxPostScriptDC::wxPostScriptDC ()
{
    m_pstream = (FILE*) NULL;

    m_currentRed = 0;
    m_currentGreen = 0;
    m_currentBlue = 0;

    m_pageNumber = 0;

    m_clipping = false;

    m_underlinePosition = 0.0;
    m_underlineThickness = 0.0;

    m_signX =  1;  // default x-axis left to right
    m_signY = -1;  // default y-axis bottom up -> top down
}

wxPostScriptDC::wxPostScriptDC (const wxPrintData& printData)
{
    m_pstream = (FILE*) NULL;

    m_currentRed = 0;
    m_currentGreen = 0;
    m_currentBlue = 0;

    m_pageNumber = 0;

    m_clipping = false;

    m_underlinePosition = 0.0;
    m_underlineThickness = 0.0;

    m_signX =  1;  // default x-axis left to right
    m_signY = -1;  // default y-axis bottom up -> top down

    m_printData = printData;

    m_ok = true;
}

wxPostScriptDC::~wxPostScriptDC ()
{
    if (m_pstream)
    {
        fclose( m_pstream );
        m_pstream = (FILE*) NULL;
    }
}

bool wxPostScriptDC::IsOk() const
{
  return m_ok;
}

void wxPostScriptDC::DoSetClippingRegion (wxCoord x, wxCoord y, wxCoord w, wxCoord h)
{
    wxCHECK_RET( m_ok , wxT("invalid postscript dc") );

    if (m_clipping) DestroyClippingRegion();

    wxDC::DoSetClippingRegion(x, y, w, h);

    m_clipping = true;

    PsPrintf( wxT("gsave\n newpath\n")
              wxT("%d %d moveto\n")
              wxT("%d %d lineto\n")
              wxT("%d %d lineto\n")
              wxT("%d %d lineto\n")
              wxT("closepath clip newpath\n"),
            LogicalToDeviceX(x),   LogicalToDeviceY(y),
            LogicalToDeviceX(x+w), LogicalToDeviceY(y),
            LogicalToDeviceX(x+w), LogicalToDeviceY(y+h),
            LogicalToDeviceX(x),   LogicalToDeviceY(y+h) );
}


void wxPostScriptDC::DestroyClippingRegion()
{
    wxCHECK_RET( m_ok , wxT("invalid postscript dc") );

    if (m_clipping)
    {
        m_clipping = false;
        PsPrint( "grestore\n" );
    }

    wxDC::DestroyClippingRegion();
}

void wxPostScriptDC::Clear()
{
    // This should fail silently to avoid unnecessary
    // asserts
    //    wxFAIL_MSG( wxT("wxPostScriptDC::Clear not implemented.") );
}

bool wxPostScriptDC::DoFloodFill (wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxColour &WXUNUSED(col), int WXUNUSED(style))
{
    wxFAIL_MSG( wxT("wxPostScriptDC::FloodFill not implemented.") );
    return false;
}

bool wxPostScriptDC::DoGetPixel (wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), wxColour * WXUNUSED(col)) const
{
    wxFAIL_MSG( wxT("wxPostScriptDC::GetPixel not implemented.") );
    return false;
}

void wxPostScriptDC::DoCrossHair (wxCoord WXUNUSED(x), wxCoord WXUNUSED(y))
{
    wxFAIL_MSG( wxT("wxPostScriptDC::CrossHair not implemented.") );
}

void wxPostScriptDC::DoDrawLine (wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if  (m_pen.GetStyle() == wxTRANSPARENT) return;

    SetPen( m_pen );

    PsPrintf( wxT("newpath\n")
              wxT("%d %d moveto\n")
              wxT("%d %d lineto\n")
              wxT("stroke\n"),
            LogicalToDeviceX(x1), LogicalToDeviceY(y1),
            LogicalToDeviceX(x2), LogicalToDeviceY (y2) );

    CalcBoundingBox( x1, y1 );
    CalcBoundingBox( x2, y2 );
}

#define RAD2DEG 57.29577951308

void wxPostScriptDC::DoDrawArc (wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2, wxCoord xc, wxCoord yc)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    wxCoord dx = x1 - xc;
    wxCoord dy = y1 - yc;
    wxCoord radius = (wxCoord) sqrt( (double)(dx*dx+dy*dy) );
    double alpha1, alpha2;

    if (x1 == x2 && y1 == y2)
    {
        alpha1 = 0.0;
        alpha2 = 360.0;
    }
    else if ( wxIsNullDouble(radius) )
    {
        alpha1 =
        alpha2 = 0.0;
    }
    else
    {
        alpha1 = (x1 - xc == 0) ?
            (y1 - yc < 0) ? 90.0 : -90.0 :
                -atan2(double(y1-yc), double(x1-xc)) * RAD2DEG;
        alpha2 = (x2 - xc == 0) ?
            (y2 - yc < 0) ? 90.0 : -90.0 :
                -atan2(double(y2-yc), double(x2-xc)) * RAD2DEG;
    }
    while (alpha1 <= 0)   alpha1 += 360;
    while (alpha2 <= 0)   alpha2 += 360; // adjust angles to be between
    while (alpha1 > 360)  alpha1 -= 360; // 0 and 360 degree
    while (alpha2 > 360)  alpha2 -= 360;

    if (m_brush.GetStyle() != wxTRANSPARENT)
    {
        SetBrush( m_brush );

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d %d %d %d ellipse\n")
                  wxT("%d %d lineto\n")
                  wxT("closepath\n")
                  wxT("fill\n"),
                LogicalToDeviceX(xc), LogicalToDeviceY(yc), LogicalToDeviceXRel(radius), LogicalToDeviceYRel(radius), (wxCoord)alpha1, (wxCoord) alpha2,
                LogicalToDeviceX(xc), LogicalToDeviceY(yc) );

        CalcBoundingBox( xc-radius, yc-radius );
        CalcBoundingBox( xc+radius, yc+radius );
    }

    if (m_pen.GetStyle() != wxTRANSPARENT)
    {
        SetPen( m_pen );

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d %d %d %d ellipse\n")
                  wxT("%d %d lineto\n")
                  wxT("closepath\n")
                  wxT("stroke\n"),
                LogicalToDeviceX(xc), LogicalToDeviceY(yc), LogicalToDeviceXRel(radius), LogicalToDeviceYRel(radius), (wxCoord)alpha1, (wxCoord) alpha2,
                LogicalToDeviceX(xc), LogicalToDeviceY(yc) );

        CalcBoundingBox( xc-radius, yc-radius );
        CalcBoundingBox( xc+radius, yc+radius );
    }
}

void wxPostScriptDC::DoDrawEllipticArc(wxCoord x,wxCoord y,wxCoord w,wxCoord h,double sa,double ea)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if ( sa >= 360 || sa <= -360 )
        sa -= int(sa/360)*360;
    if ( ea >= 360 || ea <=- 360 )
        ea -= int(ea/360)*360;
    if ( sa < 0 )
        sa += 360;
    if ( ea < 0 )
        ea += 360;

    if ( wxIsSameDouble(sa, ea) )
    {
        DrawEllipse(x,y,w,h);
        return;
    }

    if (m_brush.GetStyle () != wxTRANSPARENT)
    {
        SetBrush( m_brush );

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d %d %d %d true ellipticarc\n"),
                  LogicalToDeviceX(x+w/2), LogicalToDeviceY(y+h/2),
                  LogicalToDeviceXRel(w/2), LogicalToDeviceYRel(h/2),
                  (wxCoord)sa, (wxCoord)ea );

        CalcBoundingBox( x ,y );
        CalcBoundingBox( x+w, y+h );
    }

    if (m_pen.GetStyle () != wxTRANSPARENT)
    {
        SetPen( m_pen );

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d %d %d %d false ellipticarc\n"),
                LogicalToDeviceX(x+w/2), LogicalToDeviceY(y+h/2),
                LogicalToDeviceXRel(w/2), LogicalToDeviceYRel(h/2),
                (wxCoord)sa, (wxCoord)ea );

        CalcBoundingBox( x ,y );
        CalcBoundingBox( x+w, y+h );
    }
}

void wxPostScriptDC::DoDrawPoint (wxCoord x, wxCoord y)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (m_pen.GetStyle() == wxTRANSPARENT) return;

    SetPen (m_pen);

    PsPrintf( wxT("newpath\n")
              wxT("%d %d moveto\n")
              wxT("%d %d lineto\n")
              wxT("stroke\n"),
            LogicalToDeviceX(x),   LogicalToDeviceY(y),
            LogicalToDeviceX(x+1), LogicalToDeviceY(y) );

    CalcBoundingBox( x, y );
}

void wxPostScriptDC::DoDrawPolygon (int n, wxPoint points[], wxCoord xoffset, wxCoord yoffset, int fillStyle)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (n <= 0) return;

    if (m_brush.GetStyle () != wxTRANSPARENT)
    {
        SetBrush( m_brush );

        PsPrint( "newpath\n" );

        wxCoord xx = LogicalToDeviceX(points[0].x + xoffset);
        wxCoord yy = LogicalToDeviceY(points[0].y + yoffset);

        PsPrintf( wxT("%d %d moveto\n"), xx, yy );

        CalcBoundingBox( points[0].x + xoffset, points[0].y + yoffset );

        for (int i = 1; i < n; i++)
        {
            xx = LogicalToDeviceX(points[i].x + xoffset);
            yy = LogicalToDeviceY(points[i].y + yoffset);

            PsPrintf( wxT("%d %d lineto\n"), xx, yy );

            CalcBoundingBox( points[i].x + xoffset, points[i].y + yoffset);
        }

        PsPrint( (fillStyle == wxODDEVEN_RULE ? "eofill\n" : "fill\n") );
    }

    if (m_pen.GetStyle () != wxTRANSPARENT)
    {
        SetPen( m_pen );

        PsPrint( "newpath\n" );

        wxCoord xx = LogicalToDeviceX(points[0].x + xoffset);
        wxCoord yy = LogicalToDeviceY(points[0].y + yoffset);

        PsPrintf( wxT("%d %d moveto\n"), xx, yy );

        CalcBoundingBox( points[0].x + xoffset, points[0].y + yoffset );

        for (int i = 1; i < n; i++)
        {
            xx = LogicalToDeviceX(points[i].x + xoffset);
            yy = LogicalToDeviceY(points[i].y + yoffset);

            PsPrintf( wxT("%d %d lineto\n"), xx, yy );

            CalcBoundingBox( points[i].x + xoffset, points[i].y + yoffset);
        }

        PsPrint( "closepath\n" );
        PsPrint( "stroke\n" );
    }
}

void wxPostScriptDC::DoDrawPolyPolygon (int n, int count[], wxPoint points[], wxCoord xoffset, wxCoord yoffset, int fillStyle)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (n <= 0) return;

    if (m_brush.GetStyle () != wxTRANSPARENT)
    {
        SetBrush( m_brush );

        PsPrint( "newpath\n" );

        int ofs = 0;
        for (int i = 0; i < n; ofs += count[i++])
        {
            wxCoord xx = LogicalToDeviceX(points[ofs].x + xoffset);
            wxCoord yy = LogicalToDeviceY(points[ofs].y + yoffset);

            PsPrintf( wxT("%d %d moveto\n"), xx, yy );

            CalcBoundingBox( points[ofs].x + xoffset, points[ofs].y + yoffset );

            for (int j = 1; j < count[i]; j++)
            {
                xx = LogicalToDeviceX(points[ofs+j].x + xoffset);
                yy = LogicalToDeviceY(points[ofs+j].y + yoffset);

                PsPrintf( wxT("%d %d lineto\n"), xx, yy );

                CalcBoundingBox( points[ofs+j].x + xoffset, points[ofs+j].y + yoffset);
            }
        }
        PsPrint( (fillStyle == wxODDEVEN_RULE ? "eofill\n" : "fill\n") );
    }

    if (m_pen.GetStyle () != wxTRANSPARENT)
    {
        SetPen( m_pen );

        PsPrint( "newpath\n" );

        int ofs = 0;
        for (int i = 0; i < n; ofs += count[i++])
        {
            wxCoord xx = LogicalToDeviceX(points[ofs].x + xoffset);
            wxCoord yy = LogicalToDeviceY(points[ofs].y + yoffset);

            PsPrintf( wxT("%d %d moveto\n"), xx, yy );

            CalcBoundingBox( points[ofs].x + xoffset, points[ofs].y + yoffset );

            for (int j = 1; j < count[i]; j++)
            {
                xx = LogicalToDeviceX(points[ofs+j].x + xoffset);
                yy = LogicalToDeviceY(points[ofs+j].y + yoffset);

                PsPrintf( wxT("%d %d lineto\n"), xx, yy );

                CalcBoundingBox( points[ofs+j].x + xoffset, points[ofs+j].y + yoffset);
            }
        }
        PsPrint( "closepath\n" );
        PsPrint( "stroke\n" );
    }
}

void wxPostScriptDC::DoDrawLines (int n, wxPoint points[], wxCoord xoffset, wxCoord yoffset)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (m_pen.GetStyle() == wxTRANSPARENT) return;

    if (n <= 0) return;

    SetPen (m_pen);

    int i;
    for ( i =0; i<n ; i++ )
    {
        CalcBoundingBox( LogicalToDeviceX(points[i].x+xoffset), LogicalToDeviceY(points[i].y+yoffset));
    }

    PsPrintf( wxT("newpath\n")
              wxT("%d %d moveto\n"),
              LogicalToDeviceX(points[0].x+xoffset),
              LogicalToDeviceY(points[0].y+yoffset) );

    for (i = 1; i < n; i++)
    {
        PsPrintf( wxT("%d %d lineto\n"),
                  LogicalToDeviceX(points[i].x+xoffset),
                  LogicalToDeviceY(points[i].y+yoffset) );
    }

    PsPrint( "stroke\n" );
}

void wxPostScriptDC::DoDrawRectangle (wxCoord x, wxCoord y, wxCoord width, wxCoord height)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (m_brush.GetStyle () != wxTRANSPARENT)
    {
        SetBrush( m_brush );

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d moveto\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d lineto\n")
                  wxT("closepath\n")
                  wxT("fill\n"),
                LogicalToDeviceX(x),         LogicalToDeviceY(y),
                LogicalToDeviceX(x + width), LogicalToDeviceY(y),
                LogicalToDeviceX(x + width), LogicalToDeviceY(y + height),
                LogicalToDeviceX(x),         LogicalToDeviceY(y + height) );

        CalcBoundingBox( x, y );
        CalcBoundingBox( x + width, y + height );
    }

    if (m_pen.GetStyle () != wxTRANSPARENT)
    {
        SetPen (m_pen);

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d moveto\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d lineto\n")
                  wxT("closepath\n")
                  wxT("stroke\n"),
                LogicalToDeviceX(x),         LogicalToDeviceY(y),
                LogicalToDeviceX(x + width), LogicalToDeviceY(y),
                LogicalToDeviceX(x + width), LogicalToDeviceY(y + height),
                LogicalToDeviceX(x),         LogicalToDeviceY(y + height) );

        CalcBoundingBox( x, y );
        CalcBoundingBox( x + width, y + height );
    }
}

void wxPostScriptDC::DoDrawRoundedRectangle (wxCoord x, wxCoord y, wxCoord width, wxCoord height, double radius)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (radius < 0.0)
    {
        // Now, a negative radius is interpreted to mean
        // 'the proportion of the smallest X or Y dimension'
        double smallest = width < height ? width : height;
        radius =  (-radius * smallest);
    }

    wxCoord rad = (wxCoord) radius;

    if (m_brush.GetStyle () != wxTRANSPARENT)
    {
        SetBrush( m_brush );

        /* Draw rectangle anticlockwise */
        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d 90 180 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d %d 180 270 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d %d 270 0 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d %d 0 90 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("closepath\n")
                  wxT("fill\n"),
                LogicalToDeviceX(x + rad), LogicalToDeviceY(y + rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x), LogicalToDeviceY(y + height - rad),
                LogicalToDeviceX(x + rad), LogicalToDeviceY(y + height - rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x + width - rad), LogicalToDeviceY(y + height),
                LogicalToDeviceX(x + width - rad), LogicalToDeviceY(y + height - rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x + width), LogicalToDeviceY(y + rad),
                LogicalToDeviceX(x + width - rad), LogicalToDeviceY(y + rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x + rad), LogicalToDeviceY(y) );

        CalcBoundingBox( x, y );
        CalcBoundingBox( x + width, y + height );
    }

    if (m_pen.GetStyle () != wxTRANSPARENT)
    {
        SetPen (m_pen);

        /* Draw rectangle anticlockwise */
        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d 90 180 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d %d 180 270 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d %d 270 0 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("%d %d %d 0 90 arc\n")
                  wxT("%d %d lineto\n")
                  wxT("closepath\n")
                  wxT("stroke\n"),
                LogicalToDeviceX(x + rad), LogicalToDeviceY(y + rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x), LogicalToDeviceY(y + height - rad),
                LogicalToDeviceX(x + rad), LogicalToDeviceY(y + height - rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x + width - rad), LogicalToDeviceY(y + height),
                LogicalToDeviceX(x + width - rad), LogicalToDeviceY(y + height - rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x + width), LogicalToDeviceY(y + rad),
                LogicalToDeviceX(x + width - rad), LogicalToDeviceY(y + rad), LogicalToDeviceXRel(rad),
                LogicalToDeviceX(x + rad), LogicalToDeviceY(y) );

        CalcBoundingBox( x, y );
        CalcBoundingBox( x + width, y + height );
    }
}

void wxPostScriptDC::DoDrawEllipse (wxCoord x, wxCoord y, wxCoord width, wxCoord height)
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (m_brush.GetStyle () != wxTRANSPARENT)
    {
        SetBrush (m_brush);

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d %d 0 360 ellipse\n")
                  wxT("fill\n"),
                LogicalToDeviceX(x + width / 2), LogicalToDeviceY(y + height / 2),
                LogicalToDeviceXRel(width / 2), LogicalToDeviceYRel(height / 2) );

        CalcBoundingBox( x - width, y - height );
        CalcBoundingBox( x + width, y + height );
    }

    if (m_pen.GetStyle () != wxTRANSPARENT)
    {
        SetPen (m_pen);

        PsPrintf( wxT("newpath\n")
                  wxT("%d %d %d %d 0 360 ellipse\n")
                  wxT("stroke\n"),
                LogicalToDeviceX(x + width / 2), LogicalToDeviceY(y + height / 2),
                LogicalToDeviceXRel(width / 2), LogicalToDeviceYRel(height / 2) );

        CalcBoundingBox( x - width, y - height );
        CalcBoundingBox( x + width, y + height );
    }
}

void wxPostScriptDC::DoDrawIcon( const wxIcon& icon, wxCoord x, wxCoord y )
{
    DrawBitmap( icon, x, y, true );
}

/* this has to be char, not wxChar */
static char hexArray[] = "0123456789ABCDEF";

void wxPostScriptDC::DoDrawBitmap( const wxBitmap& bitmap, wxCoord x, wxCoord y, bool WXUNUSED(useMask) )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (!bitmap.Ok()) return;

    wxImage image = bitmap.ConvertToImage();

    if (!image.Ok()) return;

    wxCoord w = image.GetWidth();
    wxCoord h = image.GetHeight();

    wxCoord ww = LogicalToDeviceXRel(image.GetWidth());
    wxCoord hh = LogicalToDeviceYRel(image.GetHeight());

    wxCoord xx = LogicalToDeviceX(x);
    wxCoord yy = LogicalToDeviceY(y + bitmap.GetHeight());

    PsPrintf( wxT("/origstate save def\n")
              wxT("20 dict begin\n")
              wxT("/pix %d string def\n")
              wxT("/grays %d string def\n")
              wxT("/npixels 0 def\n")
              wxT("/rgbindx 0 def\n")
              wxT("%d %d translate\n")
              wxT("%d %d scale\n")
              wxT("%d %d 8\n")
              wxT("[%d 0 0 %d 0 %d]\n")
              wxT("{currentfile pix readhexstring pop}\n")
              wxT("false 3 colorimage\n"),
            w, w, xx, yy, ww, hh, w, h, w, -h, h );

    unsigned char* data = image.GetData();

    // size of the buffer = width*rgb(3)*hexa(2)+'\n'
    wxCharBuffer buffer(w*6 + 1);
    int firstDigit, secondDigit;

    //rows
    for (int j = 0; j < h; j++)
    {
        char* bufferindex = buffer.data();

        //cols
        for (int i = 0; i < w*3; i++)
        {
            firstDigit = (int)(*data/16.0);
            secondDigit = (int)(*data - (firstDigit*16.0));
            *(bufferindex++) = hexArray[firstDigit];
            *(bufferindex++) = hexArray[secondDigit];

            data++;
        }
        *(bufferindex++) = '\n';
        *bufferindex = 0;
        PsPrint( buffer );
    }

    PsPrint( "end\n" );
    PsPrint( "origstate restore\n" );
}

void wxPostScriptDC::SetFont( const wxFont& font )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (!font.Ok())  return;

    m_font = font;

    int Style = m_font.GetStyle();
    int Weight = m_font.GetWeight();

    const char *name;
    switch (m_font.GetFamily())
    {
        case wxTELETYPE:
        case wxMODERN:
        {
            if (Style == wxITALIC)
            {
                if (Weight == wxBOLD)
                    name = "/Courier-BoldOblique";
                else
                    name = "/Courier-Oblique";
            }
            else
            {
                if (Weight == wxBOLD)
                    name = "/Courier-Bold";
                else
                    name = "/Courier";
            }
            break;
        }
        case wxROMAN:
        {
            if (Style == wxITALIC)
            {
                if (Weight == wxBOLD)
                    name = "/Times-BoldItalic";
                else
                    name = "/Times-Italic";
            }
            else
            {
                if (Weight == wxBOLD)
                    name = "/Times-Bold";
                else
                    name = "/Times-Roman";
            }
            break;
        }
        case wxSCRIPT:
        {
            name = "/ZapfChancery-MediumItalic";
            break;
        }
        case wxSWISS:
        default:
        {
            if (Style == wxITALIC)
            {
                if (Weight == wxBOLD)
                    name = "/Helvetica-BoldOblique";
                else
                    name = "/Helvetica-Oblique";
            }
            else
            {
                if (Weight == wxBOLD)
                    name = "/Helvetica-Bold";
                else
                    name = "/Helvetica";
            }
            break;
        }
    }

    // We may legitimately call SetFont before BeginDoc
    if (!m_pstream)
        return;

    PsPrint( name );
    PsPrint( " reencodeISO def\n" );
    PsPrint( name );
    PsPrint( " findfont\n" );

    float size = float(m_font.GetPointSize());
    size = size * GetFontPointSizeAdjustment(GetResolution());

    char buffer[100];
    sprintf( buffer, "%f scalefont setfont\n", size * m_scaleX);
    // this is a hack - we must scale font size (in pts) according to m_scaleY but
    // LogicalToDeviceYRel works with wxCoord type (int or longint). Se we first convert font size
    // to 1/1000th of pt and then back.
    for (int i = 0; i < 100; i++)
        if (buffer[i] == ',') buffer[i] = '.';
    PsPrint( buffer );
}

void wxPostScriptDC::SetPen( const wxPen& pen )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (!pen.Ok()) return;

    int oldStyle = m_pen.GetStyle();

    m_pen = pen;

    char buffer[100];
    sprintf( buffer, "%f setlinewidth\n", LogicalToDeviceXRel(1000 * m_pen.GetWidth()) / 1000.0f );
    for (int i = 0; i < 100; i++)
        if (buffer[i] == ',') buffer[i] = '.';
    PsPrint( buffer );

/*
     Line style - WRONG: 2nd arg is OFFSET

     Here, I'm afraid you do not conceive meaning of parameters of 'setdash'
     operator correctly. You should look-up this in the Red Book: the 2nd parame-
     ter is not number of values in the array of the first one, but an offset
     into this description of the pattern. I mean a real *offset* not index
     into array. I.e. If the command is [3 4] 1 setdash   is used, then there
     will be first black line *2* units wxCoord, then space 4 units, then the
     pattern of *3* units black, 4 units space will be repeated.
*/

    static const char *dotted = "[2 5] 2";
    static const char *short_dashed = "[4 4] 2";
    static const char *wxCoord_dashed = "[4 8] 2";
    static const char *dotted_dashed = "[6 6 2 6] 4";

    const char *psdash;

    switch (m_pen.GetStyle())
    {
        case wxDOT:           psdash = dotted;         break;
        case wxSHORT_DASH:    psdash = short_dashed;   break;
        case wxLONG_DASH:     psdash = wxCoord_dashed; break;
        case wxDOT_DASH:      psdash = dotted_dashed;  break;
        case wxUSER_DASH:
        {
            wxDash *dashes;
            int nDashes = m_pen.GetDashes (&dashes);
            PsPrint ("[");
            for (int i = 0; i < nDashes; ++i)
            {
                sprintf( buffer, "%d ", dashes [i] );
                PsPrint( buffer );
            }
            PsPrint ("] 0 setdash\n");
            psdash = 0;
        }
        break;
        case wxSOLID:
        case wxTRANSPARENT:
        default:              psdash = "[] 0";         break;
    }

    if ( psdash && (oldStyle != m_pen.GetStyle()) )
    {
        PsPrint( psdash );
        PsPrint( " setdash\n" );
    }

    // Line colour
    unsigned char red = m_pen.GetColour().Red();
    unsigned char blue = m_pen.GetColour().Blue();
    unsigned char green = m_pen.GetColour().Green();

    if (!m_colour)
    {
        // Anything not white is black
        if (! (red == (unsigned char) 255 &&
               blue == (unsigned char) 255 &&
               green == (unsigned char) 255) )
        {
            red = (unsigned char) 0;
            green = (unsigned char) 0;
            blue = (unsigned char) 0;
        }
        // setgray here ?
    }

    if (!(red == m_currentRed && green == m_currentGreen && blue == m_currentBlue))
    {
        double redPS = (double)(red) / 255.0;
        double bluePS = (double)(blue) / 255.0;
        double greenPS = (double)(green) / 255.0;

        sprintf( buffer,
            "%.8f %.8f %.8f setrgbcolor\n",
            redPS, greenPS, bluePS );
        for (int i = 0; i < 100; i++)
            if (buffer[i] == ',') buffer[i] = '.';

        PsPrint( buffer );

        m_currentRed = red;
        m_currentBlue = blue;
        m_currentGreen = green;
    }
}

void wxPostScriptDC::SetBrush( const wxBrush& brush )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (!brush.Ok()) return;

    m_brush = brush;

    // Brush colour
    unsigned char red = m_brush.GetColour().Red();
    unsigned char blue = m_brush.GetColour().Blue();
    unsigned char green = m_brush.GetColour().Green();

    if (!m_colour)
    {
        // Anything not white is black
        if (! (red == (unsigned char) 255 &&
               blue == (unsigned char) 255 &&
               green == (unsigned char) 255) )
        {
            red = (unsigned char) 0;
            green = (unsigned char) 0;
            blue = (unsigned char) 0;
        }
        // setgray here ?
    }

    if (!(red == m_currentRed && green == m_currentGreen && blue == m_currentBlue))
    {
        double redPS = (double)(red) / 255.0;
        double bluePS = (double)(blue) / 255.0;
        double greenPS = (double)(green) / 255.0;

        char buffer[100];
        sprintf( buffer,
                "%.8f %.8f %.8f setrgbcolor\n",
                redPS, greenPS, bluePS );
        for (int i = 0; i < 100; i++)
            if (buffer[i] == ',') buffer[i] = '.';

        PsPrint( buffer );

        m_currentRed = red;
        m_currentBlue = blue;
        m_currentGreen = green;
    }
}

void wxPostScriptDC::DoDrawText( const wxString& text, wxCoord x, wxCoord y )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (m_textForegroundColour.Ok())
    {
        unsigned char red = m_textForegroundColour.Red();
        unsigned char blue = m_textForegroundColour.Blue();
        unsigned char green = m_textForegroundColour.Green();

        if (!m_colour)
        {
            // Anything not white is black
            if (! (red == (unsigned char) 255 &&
                        blue == (unsigned char) 255 &&
                        green == (unsigned char) 255))
            {
                red = (unsigned char) 0;
                green = (unsigned char) 0;
                blue = (unsigned char) 0;
            }
        }

        // maybe setgray here ?
        if (!(red == m_currentRed && green == m_currentGreen && blue == m_currentBlue))
        {
            double redPS = (double)(red) / 255.0;
            double bluePS = (double)(blue) / 255.0;
            double greenPS = (double)(green) / 255.0;

            char buffer[100];
            sprintf( buffer,
                "%.8f %.8f %.8f setrgbcolor\n",
                redPS, greenPS, bluePS );
            for (size_t i = 0; i < strlen(buffer); i++)
                if (buffer[i] == ',') buffer[i] = '.';
            PsPrint( buffer );

            m_currentRed = red;
            m_currentBlue = blue;
            m_currentGreen = green;
        }
    }

    wxCoord text_w, text_h, text_descent;

    GetTextExtent(text, &text_w, &text_h, &text_descent);

    // VZ: this seems to be unnecessary, so taking it out for now, if it
    //     doesn't create any problems, remove this comment entirely
    //SetFont( m_font );


    int size = m_font.GetPointSize();

//    wxCoord by = y + (wxCoord)floor( double(size) * 2.0 / 3.0 ); // approximate baseline
//    commented by V. Slavik and replaced by accurate version
//        - note that there is still rounding error in text_descent!
    wxCoord by = y + size - text_descent; // baseline

    PsPrintf( wxT("%d %d moveto\n"), LogicalToDeviceX(x), LogicalToDeviceY(by) );
    PsPrint( "(" );

    const wxWX2MBbuf textbuf = text.mb_str();
    size_t len = strlen(textbuf);
    size_t i;
    for (i = 0; i < len; i++)
    {
        int c = (unsigned char) textbuf[i];
        if (c == ')' || c == '(' || c == '\\')
        {
            /* Cope with special characters */
            PsPrint( "\\" );
            PsPrint(c);
        }
        else if ( c >= 128 )
        {
            /* Cope with character codes > 127 */
            PsPrintf( wxT("\\%o"), c);
        }
        else
        {
            PsPrint(c);
        }
    }

    PsPrint( ") show\n" );

    if (m_font.GetUnderlined())
    {
        wxCoord uy = (wxCoord)(y + size - m_underlinePosition);
        char buffer[100];

        sprintf( buffer,
                "gsave\n"
                "%d %d moveto\n"
                "%f setlinewidth\n"
                "%d %d lineto\n"
                "stroke\n"
                "grestore\n",
                LogicalToDeviceX(x), LogicalToDeviceY(uy),
                m_underlineThickness,
                LogicalToDeviceX(x + text_w), LogicalToDeviceY(uy) );
        for (i = 0; i < 100; i++)
            if (buffer[i] == ',') buffer[i] = '.';
        PsPrint( buffer );
    }

    CalcBoundingBox( x, y );
    CalcBoundingBox( x + size * text.length() * 2/3 , y );
}

void wxPostScriptDC::DoDrawRotatedText( const wxString& text, wxCoord x, wxCoord y, double angle )
{
    if ( wxIsNullDouble(angle) )
    {
        DoDrawText(text, x, y);
        return;
    }

    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    SetFont( m_font );

    if (m_textForegroundColour.Ok())
    {
        unsigned char red = m_textForegroundColour.Red();
        unsigned char blue = m_textForegroundColour.Blue();
        unsigned char green = m_textForegroundColour.Green();

        if (!m_colour)
        {
            // Anything not white is black
            if (! (red == (unsigned char) 255 &&
                   blue == (unsigned char) 255 &&
                   green == (unsigned char) 255))
            {
                red = (unsigned char) 0;
                green = (unsigned char) 0;
                blue = (unsigned char) 0;
            }
        }

        // maybe setgray here ?
        if (!(red == m_currentRed && green == m_currentGreen && blue == m_currentBlue))
        {
            double redPS = (double)(red) / 255.0;
            double bluePS = (double)(blue) / 255.0;
            double greenPS = (double)(green) / 255.0;

            char buffer[100];
            sprintf( buffer,
                "%.8f %.8f %.8f setrgbcolor\n",
                redPS, greenPS, bluePS );
            for (int i = 0; i < 100; i++)
                if (buffer[i] == ',') buffer[i] = '.';
            PsPrint( buffer );

            m_currentRed = red;
            m_currentBlue = blue;
            m_currentGreen = green;
        }
    }

    int size = m_font.GetPointSize();

    PsPrintf( wxT("%d %d moveto\n"),
            LogicalToDeviceX(x), LogicalToDeviceY(y));

    char buffer[100];
    sprintf(buffer, "%.8f rotate\n", angle);
    size_t i;
    for (i = 0; i < 100; i++)
    {
        if (buffer[i] == ',') buffer[i] = '.';
    }
    PsPrint( buffer);

    PsPrint( "(" );
    const wxWX2MBbuf textbuf = text.mb_str();
    size_t len = strlen(textbuf);
    for (i = 0; i < len; i++)
    {
        int c = (unsigned char) textbuf[i];
        if (c == ')' || c == '(' || c == '\\')
        {
            /* Cope with special characters */
            PsPrint( "\\" );
            PsPrint(c);
        }
        else if ( c >= 128 )
        {
            /* Cope with character codes > 127 */
            PsPrintf( wxT("\\%o"), c);
        }
        else
        {
            PsPrint(c);
        }
    }

    PsPrint( ") show\n" );

    sprintf( buffer, "%.8f rotate\n", -angle );
    for (i = 0; i < 100; i++)
    {
        if (buffer[i] == ',') buffer[i] = '.';
    }
    PsPrint( buffer );

    if (m_font.GetUnderlined())
    {
        wxCoord uy = (wxCoord)(y + size - m_underlinePosition);
        wxCoord w, h;
        GetTextExtent(text, &w, &h);

        sprintf( buffer,
                "gsave\n"
                "%d %d moveto\n"
                "%f setlinewidth\n"
                "%d %d lineto\n"
                "stroke\n"
                "grestore\n",
                LogicalToDeviceX(x), LogicalToDeviceY(uy),
                m_underlineThickness,
                LogicalToDeviceX(x + w), LogicalToDeviceY(uy) );
        for (i = 0; i < 100; i++)
        {
            if (buffer[i] == ',') buffer[i] = '.';
        }
        PsPrint( buffer );
    }

    CalcBoundingBox( x, y );
    CalcBoundingBox( x + size * text.length() * 2/3 , y );
}

void wxPostScriptDC::SetBackground (const wxBrush& brush)
{
    m_backgroundBrush = brush;
}

void wxPostScriptDC::SetLogicalFunction (int WXUNUSED(function))
{
    wxFAIL_MSG( wxT("wxPostScriptDC::SetLogicalFunction not implemented.") );
}

#if wxUSE_SPLINES
void wxPostScriptDC::DoDrawSpline( wxList *points )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    SetPen( m_pen );

    // a and b are not used
    //double a, b;
    double c, d, x1, y1, x2, y2, x3, y3;
    wxPoint *p, *q;

    wxList::compatibility_iterator node = points->GetFirst();
    p = (wxPoint *)node->GetData();
    x1 = p->x;
    y1 = p->y;

    node = node->GetNext();
    p = (wxPoint *)node->GetData();
    c = p->x;
    d = p->y;
    x3 =
         #if 0
         a =
         #endif
         (double)(x1 + c) / 2;
    y3 =
         #if 0
         b =
         #endif
         (double)(y1 + d) / 2;

    PsPrintf( wxT("newpath\n")
              wxT("%d %d moveto\n")
              wxT("%d %d lineto\n"),
            LogicalToDeviceX((wxCoord)x1), LogicalToDeviceY((wxCoord)y1),
            LogicalToDeviceX((wxCoord)x3), LogicalToDeviceY((wxCoord)y3) );

    CalcBoundingBox( (wxCoord)x1, (wxCoord)y1 );
    CalcBoundingBox( (wxCoord)x3, (wxCoord)y3 );

    node = node->GetNext();
    while (node)
    {
        q = (wxPoint *)node->GetData();

        x1 = x3;
        y1 = y3;
        x2 = c;
        y2 = d;
        c = q->x;
        d = q->y;
        x3 = (double)(x2 + c) / 2;
        y3 = (double)(y2 + d) / 2;

        PsPrintf( wxT("%d %d %d %d %d %d DrawSplineSection\n"),
            LogicalToDeviceX((wxCoord)x1), LogicalToDeviceY((wxCoord)y1),
            LogicalToDeviceX((wxCoord)x2), LogicalToDeviceY((wxCoord)y2),
            LogicalToDeviceX((wxCoord)x3), LogicalToDeviceY((wxCoord)y3) );

        CalcBoundingBox( (wxCoord)x1, (wxCoord)y1 );
        CalcBoundingBox( (wxCoord)x3, (wxCoord)y3 );

        node = node->GetNext();
    }

    /*
       At this point, (x2,y2) and (c,d) are the position of the
       next-to-last and last point respectively, in the point list
     */

    PsPrintf( wxT("%d %d lineto\n")
              wxT("stroke\n"),
            LogicalToDeviceX((wxCoord)c), LogicalToDeviceY((wxCoord)d) );
}
#endif // wxUSE_SPLINES

wxCoord wxPostScriptDC::GetCharWidth() const
{
    // Chris Breeze: reasonable approximation using wxMODERN/Courier
    return (wxCoord) (GetCharHeight() * 72.0 / 120.0);
}


void wxPostScriptDC::SetAxisOrientation( bool xLeftRight, bool yBottomUp )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    m_signX = (xLeftRight ? 1 : -1);
    m_signY = (yBottomUp  ? 1 : -1);

    ComputeScaleAndOrigin();
}

void wxPostScriptDC::SetDeviceOrigin( wxCoord x, wxCoord y )
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    int h = 0;
    int w = 0;
    GetSize( &w, &h );

    wxDC::SetDeviceOrigin( x, h-y );
}

void wxPostScriptDC::DoGetSize(int* width, int* height) const
{
    wxPaperSize id = m_printData.GetPaperId();

    wxPrintPaperType *paper = wxThePrintPaperDatabase->FindPaperType(id);

    if (!paper) paper = wxThePrintPaperDatabase->FindPaperType(wxPAPER_A4);

    int w = 595;
    int h = 842;
    if (paper)
    {
        w = paper->GetSizeDeviceUnits().x;
        h = paper->GetSizeDeviceUnits().y;
    }

    if (m_printData.GetOrientation() == wxLANDSCAPE)
    {
        int tmp = w;
        w = h;
        h = tmp;
    }

    if (width) *width = (int)(w * ms_PSScaleFactor);
    if (height) *height = (int)(h * ms_PSScaleFactor);
}

void wxPostScriptDC::DoGetSizeMM(int *width, int *height) const
{
    wxPaperSize id = m_printData.GetPaperId();

    wxPrintPaperType *paper = wxThePrintPaperDatabase->FindPaperType(id);

    if (!paper) paper = wxThePrintPaperDatabase->FindPaperType(wxPAPER_A4);

    int w = 210;
    int h = 297;
    if (paper)
    {
        w = paper->GetWidth() / 10;
        h = paper->GetHeight() / 10;
    }

    if (m_printData.GetOrientation() == wxLANDSCAPE)
    {
        int tmp = w;
        w = h;
        h = tmp;
    }

    if (width) *width = w;
    if (height) *height = h;
}

// Resolution in pixels per logical inch
wxSize wxPostScriptDC::GetPPI(void) const
{
    return wxSize((int)(72 * ms_PSScaleFactor),
                  (int)(72 * ms_PSScaleFactor));
}


bool wxPostScriptDC::StartDoc( const wxString& message )
{
    wxCHECK_MSG( m_ok, false, wxT("invalid postscript dc") );

    if (m_printData.GetPrintMode() != wxPRINT_MODE_STREAM )
    {
        if (m_printData.GetFilename() == wxEmptyString)
        {
            wxString filename = wxGetTempFileName( wxT("ps") );
            m_printData.SetFilename(filename);
        }

        m_pstream = wxFopen( m_printData.GetFilename(), wxT("w+") );

        if (!m_pstream)
        {
            wxLogError( _("Cannot open file for PostScript printing!"));
            m_ok = false;
            return false;
        }
    }

    m_ok = true;
    m_title = message;

    PsPrint( "%!PS-Adobe-2.0\n" );
    PsPrint( "%%Creator: wxWidgets PostScript renderer\n" );
    PsPrintf( wxT("%%%%CreationDate: %s\n"), wxNow().c_str() );
    if (m_printData.GetOrientation() == wxLANDSCAPE)
        PsPrint( "%%Orientation: Landscape\n" );
    else
        PsPrint( "%%Orientation: Portrait\n" );

    // PsPrintf( wxT("%%%%Pages: %d\n"), (wxPageNumber - 1) );

    const wxChar *paper;
    switch (m_printData.GetPaperId())
    {
       case wxPAPER_LETTER: paper = wxT("Letter"); break;       // Letter: paper ""; 8 1/2 by 11 inches
       case wxPAPER_LEGAL: paper = wxT("Legal"); break;         // Legal, 8 1/2 by 14 inches
       case wxPAPER_A4: paper = wxT("A4"); break;               // A4 Sheet, 210 by 297 millimeters
       case wxPAPER_TABLOID: paper = wxT("Tabloid"); break;     // Tabloid, 11 by 17 inches
       case wxPAPER_LEDGER: paper = wxT("Ledger"); break;       // Ledger, 17 by 11 inches
       case wxPAPER_STATEMENT: paper = wxT("Statement"); break; // Statement, 5 1/2 by 8 1/2 inches
       case wxPAPER_EXECUTIVE: paper = wxT("Executive"); break; // Executive, 7 1/4 by 10 1/2 inches
       case wxPAPER_A3: paper = wxT("A3"); break;               // A3 sheet, 297 by 420 millimeters
       case wxPAPER_A5: paper = wxT("A5"); break;               // A5 sheet, 148 by 210 millimeters
       case wxPAPER_B4: paper = wxT("B4"); break;               // B4 sheet, 250 by 354 millimeters
       case wxPAPER_B5: paper = wxT("B5"); break;               // B5 sheet, 182-by-257-millimeter paper
       case wxPAPER_FOLIO: paper = wxT("Folio"); break;         // Folio, 8-1/2-by-13-inch paper
       case wxPAPER_QUARTO: paper = wxT("Quaro"); break;        // Quarto, 215-by-275-millimeter paper
       case wxPAPER_10X14: paper = wxT("10x14"); break;         // 10-by-14-inch sheet
       default: paper = wxT("A4");
    }
    PsPrintf( wxT("%%%%DocumentPaperSizes: %s\n"), paper );
    PsPrint( "%%EndComments\n\n" );

    PsPrint( "%%BeginProlog\n" );
    PsPrint( wxPostScriptHeaderConicTo );
    PsPrint( wxPostScriptHeaderEllipse );
    PsPrint( wxPostScriptHeaderEllipticArc );
    PsPrint( wxPostScriptHeaderColourImage );
    PsPrint( wxPostScriptHeaderReencodeISO1 );
    PsPrint( wxPostScriptHeaderReencodeISO2 );
    if (wxPostScriptHeaderSpline)
        PsPrint( wxPostScriptHeaderSpline );
    PsPrint( "%%EndProlog\n" );

    SetBrush( *wxBLACK_BRUSH );
    SetPen( *wxBLACK_PEN );
    SetBackground( *wxWHITE_BRUSH );
    SetTextForeground( *wxBLACK );

    // set origin according to paper size
    SetDeviceOrigin( 0,0 );

    wxPageNumber = 1;
    m_pageNumber = 1;
    return true;
}

void wxPostScriptDC::EndDoc ()
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    if (m_clipping)
    {
        m_clipping = false;
        PsPrint( "grestore\n" );
    }

    if ( m_pstream ) {
        fclose( m_pstream );
        m_pstream = (FILE *) NULL;
    }

#if 0
    // THE FOLLOWING HAS BEEN CONTRIBUTED BY Andy Fyfe <andy@hyperparallel.com>
    wxCoord wx_printer_translate_x, wx_printer_translate_y;
    double wx_printer_scale_x, wx_printer_scale_y;

    wx_printer_translate_x = (wxCoord)m_printData.GetPrinterTranslateX();
    wx_printer_translate_y = (wxCoord)m_printData.GetPrinterTranslateY();

    wx_printer_scale_x = m_printData.GetPrinterScaleX();
    wx_printer_scale_y = m_printData.GetPrinterScaleY();

    // Compute the bounding box.  Note that it is in the default user
    // coordinate system, thus we have to convert the values.
    wxCoord minX = (wxCoord) LogicalToDeviceX(m_minX);
    wxCoord minY = (wxCoord) LogicalToDeviceY(m_minY);
    wxCoord maxX = (wxCoord) LogicalToDeviceX(m_maxX);
    wxCoord maxY = (wxCoord) LogicalToDeviceY(m_maxY);

    // LOG2DEV may have changed the minimum to maximum vice versa
    if ( minX > maxX ) { wxCoord tmp = minX; minX = maxX; maxX = tmp; }
    if ( minY > maxY ) { wxCoord tmp = minY; minY = maxY; maxY = tmp; }

    // account for used scaling (boundingbox is before scaling in ps-file)
    double scale_x = m_printData.GetPrinterScaleX() / ms_PSScaleFactor;
    double scale_y = m_printData.GetPrinterScaleY() / ms_PSScaleFactor;

    wxCoord llx, lly, urx, ury;
    llx = (wxCoord) ((minX+wx_printer_translate_x)*scale_x);
    lly = (wxCoord) ((minY+wx_printer_translate_y)*scale_y);
    urx = (wxCoord) ((maxX+wx_printer_translate_x)*scale_x);
    ury = (wxCoord) ((maxY+wx_printer_translate_y)*scale_y);
    // (end of bounding box computation)


    // If we're landscape, our sense of "x" and "y" is reversed.
    if (m_printData.GetOrientation() == wxLANDSCAPE)
    {
        wxCoord tmp;
        tmp = llx; llx = lly; lly = tmp;
        tmp = urx; urx = ury; ury = tmp;

        // We need either the two lines that follow, or we need to subtract
        // min_x from real_translate_y, which is commented out below.
        llx = llx - (wxCoord)(m_minX*wx_printer_scale_y);
        urx = urx - (wxCoord)(m_minX*wx_printer_scale_y);
    }

    // The Adobe specifications call for integers; we round as to make
    // the bounding larger.
    PsPrintf( wxT("%%%%BoundingBox: %d %d %d %d\n"),
            (wxCoord)floor((double)llx), (wxCoord)floor((double)lly),
            (wxCoord)ceil((double)urx), (wxCoord)ceil((double)ury) );

    // To check the correctness of the bounding box, postscript commands
    // to draw a box corresponding to the bounding box are generated below.
    // But since we typically don't want to print such a box, the postscript
    // commands are generated within comments.  These lines appear before any
    // adjustment of scale, rotation, or translation, and hence are in the
    // default user coordinates.
    PsPrint( "% newpath\n" );
    PsPrintf( wxT("%% %d %d moveto\n"), llx, lly );
    PsPrintf( wxT("%% %d %d lineto\n"), urx, lly );
    PsPrintf( wxT("%% %d %d lineto\n"), urx, ury );
    PsPrintf( wxT("%% %d %d lineto closepath stroke\n"), llx, ury );
#endif

#ifndef __WXMSW__
    wxPostScriptPrintNativeData *data =
        (wxPostScriptPrintNativeData *) m_printData.GetNativeData();

    if (m_ok && (m_printData.GetPrintMode() == wxPRINT_MODE_PRINTER))
    {
        wxString command;
        command += data->GetPrinterCommand();
        command += wxT(" ");
        command += data->GetPrinterOptions();
        command += wxT(" ");
        command += m_printData.GetFilename();

        wxExecute( command, true );
        wxRemoveFile( m_printData.GetFilename() );
    }
#endif
}

void wxPostScriptDC::StartPage()
{
    wxCHECK_RET( m_ok, wxT("invalid postscript dc") );

    PsPrintf( wxT("%%%%Page: %d\n"), wxPageNumber++ );

    //  What is this one supposed to do? RR.
//  *m_pstream << "matrix currentmatrix\n";

    // Added by Chris Breeze

    // Each page starts with an "initgraphics" which resets the
    // transformation and so we need to reset the origin
    // (and rotate the page for landscape printing)

    // Output scaling
    wxCoord translate_x, translate_y;
    double scale_x, scale_y;

    wxPostScriptPrintNativeData *data =
        (wxPostScriptPrintNativeData *) m_printData.GetNativeData();

    translate_x = (wxCoord)data->GetPrinterTranslateX();
    translate_y = (wxCoord)data->GetPrinterTranslateY();

    scale_x = data->GetPrinterScaleX();
    scale_y = data->GetPrinterScaleY();

    if (m_printData.GetOrientation() == wxLANDSCAPE)
    {
        int h;
        GetSize( (int*) NULL, &h );
        translate_y -= h;
        PsPrint( "90 rotate\n" );
        // I copied this one from a PostScript tutorial, but to no avail. RR.
        // PsPrint( "90 rotate llx neg ury nef translate\n" );
    }

    char buffer[100];
    sprintf( buffer, "%.8f %.8f scale\n", scale_x / ms_PSScaleFactor,
            scale_y / ms_PSScaleFactor);
    for (int i = 0; i < 100; i++)
        if (buffer[i] == ',') buffer[i] = '.';
    PsPrint( buffer );

    PsPrintf( wxT("%d %d translate\n"), translate_x, translate_y );
}

void wxPostScriptDC::EndPage ()
{
    wxCHECK_RET( m_ok , wxT("invalid postscript dc") );

    PsPrint( "showpage\n" );
}

bool wxPostScriptDC::DoBlit( wxCoord xdest, wxCoord ydest,
                           wxCoord fwidth, wxCoord fheight,
                           wxDC *source,
                           wxCoord xsrc, wxCoord ysrc,
                           int rop, bool WXUNUSED(useMask), wxCoord WXUNUSED(xsrcMask), wxCoord WXUNUSED(ysrcMask) )
{
    wxCHECK_MSG( m_ok, false, wxT("invalid postscript dc") );

    wxCHECK_MSG( source, false, wxT("invalid source dc") );

    /* blit into a bitmap */
    wxBitmap bitmap( (int)fwidth, (int)fheight );
    wxMemoryDC memDC;
    memDC.SelectObject(bitmap);
    memDC.Blit(0, 0, fwidth, fheight, source, xsrc, ysrc, rop); /* TODO: Blit transparently? */
    memDC.SelectObject(wxNullBitmap);

    /* draw bitmap. scaling and positioning is done there */
    DrawBitmap( bitmap, xdest, ydest );

    return true;
}

wxCoord wxPostScriptDC::GetCharHeight() const
{
    if (m_font.Ok())
        return m_font.GetPointSize();
    else
        return 12;
}

void wxPostScriptDC::DoGetTextExtent(const wxString& string,
                                     wxCoord *x, wxCoord *y,
                                     wxCoord *descent, wxCoord *externalLeading,
                                     wxFont *theFont ) const
{
    wxFont *fontToUse = theFont;

    if (!fontToUse) fontToUse = (wxFont*) &m_font;

    const float fontSize =
        fontToUse->GetPointSize() * GetFontPointSizeAdjustment(72.0);

    if (string.empty())
    {
        if (x) (*x) = 0;
        if (y) (*y) = 0;
        if (descent) (*descent) = 0;
        if (externalLeading) (*externalLeading) = 0;
        return;
    }

   // GTK 2.0

    const wxWX2MBbuf strbuf = string.mb_str();

#if !wxUSE_AFM_FOR_POSTSCRIPT
    /* Provide a VERY rough estimate (avoid using it).
     * Produces accurate results for mono-spaced font
     * such as Courier (aka wxMODERN) */

    if ( x )
        *x = strlen (strbuf) * fontSize * 72.0 / 120.0;
    if ( y )
        *y = (wxCoord) (fontSize * 1.32);    /* allow for descender */
    if (descent) *descent = 0;
    if (externalLeading) *externalLeading = 0;
#else

    /* method for calculating string widths in postscript:
    /  read in the AFM (adobe font metrics) file for the
    /  actual font, parse it and extract the character widths
    /  and also the descender. this may be improved, but for now
    /  it works well. the AFM file is only read in if the
    /  font is changed. this may be chached in the future.
    /  calls to GetTextExtent with the font unchanged are rather
    /  efficient!!!
    /
    /  for each font and style used there is an AFM file necessary.
    /  currently i have only files for the roman font family.
    /  I try to get files for the other ones!
    /
    /  CAVE: the size of the string is currently always calculated
    /        in 'points' (1/72 of an inch). this should later on be
    /        changed to depend on the mapping mode.
    /  CAVE: the path to the AFM files must be set before calling this
    /        function. this is usually done by a call like the following:
    /        wxSetAFMPath("d:\\wxw161\\afm\\");
    /
    /  example:
    /
    /    wxPostScriptDC dc(NULL, true);
    /    if (dc.Ok()){
    /      wxSetAFMPath("d:\\wxw161\\afm\\");
    /      dc.StartDoc("Test");
    /      dc.StartPage();
    /      wxCoord w,h;
    /      dc.SetFont(new wxFont(10, wxROMAN, wxNORMAL, wxNORMAL));
    /      dc.GetTextExtent("Hallo",&w,&h);
    /      dc.EndPage();
    /      dc.EndDoc();
    /    }
    /
    /  by steve (stefan.hammes@urz.uni-heidelberg.de)
    /  created: 10.09.94
    /  updated: 14.05.95 */

    /* these static vars are for storing the state between calls */
    static int lastFamily= INT_MIN;
    static int lastSize= INT_MIN;
    static int lastStyle= INT_MIN;
    static int lastWeight= INT_MIN;
    static int lastDescender = INT_MIN;
    static int lastWidths[256]; /* widths of the characters */

    double UnderlinePosition = 0.0;
    double UnderlineThickness = 0.0;

    // Get actual parameters
    int Family = fontToUse->GetFamily();
    int Size =   fontToUse->GetPointSize();
    int Style =  fontToUse->GetStyle();
    int Weight = fontToUse->GetWeight();

    // If we have another font, read the font-metrics
    if (Family!=lastFamily || Size!=lastSize || Style!=lastStyle || Weight!=lastWeight)
    {
        // Store actual values
        lastFamily = Family;
        lastSize =   Size;
        lastStyle =  Style;
        lastWeight = Weight;

        const wxChar *name;

        switch (Family)
        {
            case wxMODERN:
            case wxTELETYPE:
            {
                if ((Style == wxITALIC) && (Weight == wxBOLD)) name = wxT("CourBoO.afm");
                else if ((Style != wxITALIC) && (Weight == wxBOLD)) name = wxT("CourBo.afm");
                else if ((Style == wxITALIC) && (Weight != wxBOLD)) name = wxT("CourO.afm");
                else name = wxT("Cour.afm");
                break;
            }
            case wxROMAN:
            {
                if ((Style == wxITALIC) && (Weight == wxBOLD)) name = wxT("TimesBoO.afm");
                else if ((Style != wxITALIC) && (Weight == wxBOLD)) name = wxT("TimesBo.afm");
                else if ((Style == wxITALIC) && (Weight != wxBOLD)) name = wxT("TimesO.afm");
                else name = wxT("TimesRo.afm");
                break;
            }
            case wxSCRIPT:
            {
                name = wxT("Zapf.afm");
                break;
            }
            case wxSWISS:
            default:
            {
                if ((Style == wxITALIC) && (Weight == wxBOLD)) name = wxT("HelvBoO.afm");
                else if ((Style != wxITALIC) && (Weight == wxBOLD)) name = wxT("HelvBo.afm");
                else if ((Style == wxITALIC) && (Weight != wxBOLD)) name = wxT("HelvO.afm");
                else name = wxT("Helv.afm");
                break;
            }
        }

        FILE *afmFile = NULL;

        // Get the directory of the AFM files
        wxString afmName;

        // VZ: I don't know if the cast always works under Unix but it clearly
        //     never does under Windows where the pointer is
        //     wxWindowsPrintNativeData and so calling GetFontMetricPath() on
        //     it just crashes
#ifndef __WIN32__
        wxPostScriptPrintNativeData *data =
            wxDynamicCast(m_printData.GetNativeData(), wxPostScriptPrintNativeData);

        if (data && !data->GetFontMetricPath().empty())
        {
            afmName = data->GetFontMetricPath();
            afmName << wxFILE_SEP_PATH << name;
        }
#endif // __WIN32__

        if ( !afmName.empty() )
            afmFile = wxFopen(afmName, wxT("r"));

        if ( !afmFile )
        {
#if defined(__UNIX__) && !defined(__VMS__)
           afmName = wxGetDataDir();
#else // !__UNIX__
           afmName = wxStandardPaths::Get().GetDataDir();
#endif // __UNIX__/!__UNIX__

           afmName <<  wxFILE_SEP_PATH
#if defined(__LINUX__) || defined(__FREEBSD__)
                   << wxT("gs_afm") << wxFILE_SEP_PATH
#else
                   << wxT("afm") << wxFILE_SEP_PATH
#endif
                   << name;
           afmFile = wxFopen(afmName,wxT("r"));
        }

        /* 2. open and process the file
           /  a short explanation of the AFM format:
           /  we have for each character a line, which gives its size
           /  e.g.:
           /
           /    C 63 ; WX 444 ; N question ; B 49 -14 395 676 ;
           /
           /  that means, we have a character with ascii code 63, and width
           /  (444/1000 * fontSize) points.
           /  the other data is ignored for now!
           /
           /  when the font has changed, we read in the right AFM file and store the
           /  character widths in an array, which is processed below (see point 3.). */
        if (afmFile==NULL)
        {
            wxLogDebug( wxT("GetTextExtent: can't open AFM file '%s'"), afmName.c_str() );
            wxLogDebug( wxT("               using approximate values"));
            for (int i=0; i<256; i++) lastWidths[i] = 500; /* an approximate value */
            lastDescender = -150; /* dito. */
        }
        else
        {
            /* init the widths array */
            for(int i=0; i<256; i++) lastWidths[i] = INT_MIN;
            /* some variables for holding parts of a line */
            char cString[10], semiString[10], WXString[10];
            char descString[20];
            char upString[30], utString[30];
            char encString[50];
            char line[256];
            int ascii,cWidth;
            /* read in the file and parse it */
            while(fgets(line,sizeof(line),afmFile)!=NULL)
            {
                /* A.) check for descender definition */
                if (strncmp(line,"Descender",9)==0)
                {
                    if ((sscanf(line,"%s%d",descString,&lastDescender)!=2) ||
                            (strcmp(descString,"Descender")!=0))
                    {
                        wxLogDebug( wxT("AFM-file '%s': line '%s' has error (bad descender)"), afmName.c_str(),line );
                    }
                }
                /* JC 1.) check for UnderlinePosition */
                else if(strncmp(line,"UnderlinePosition",17)==0)
                {
                    if ((sscanf(line,"%s%lf",upString,&UnderlinePosition)!=2) ||
                            (strcmp(upString,"UnderlinePosition")!=0))
                    {
                        wxLogDebug( wxT("AFM-file '%s': line '%s' has error (bad UnderlinePosition)"), afmName.c_str(), line );
                    }
                }
                /* JC 2.) check for UnderlineThickness */
                else if(strncmp(line,"UnderlineThickness",18)==0)
                {
                    if ((sscanf(line,"%s%lf",utString,&UnderlineThickness)!=2) ||
                            (strcmp(utString,"UnderlineThickness")!=0))
                    {
                        wxLogDebug( wxT("AFM-file '%s': line '%s' has error (bad UnderlineThickness)"), afmName.c_str(), line );
                    }
                }
                /* JC 3.) check for EncodingScheme */
                else if(strncmp(line,"EncodingScheme",14)==0)
                {
                    if ((sscanf(line,"%s%s",utString,encString)!=2) ||
                            (strcmp(utString,"EncodingScheme")!=0))
                    {
                        wxLogDebug( wxT("AFM-file '%s': line '%s' has error (bad EncodingScheme)"), afmName.c_str(), line );
                    }
                    else if (strncmp(encString, "AdobeStandardEncoding", 21))
                    {
                        wxLogDebug( wxT("AFM-file '%s': line '%s' has error (unsupported EncodingScheme %s)"),
                                afmName.c_str(),line, encString);
                    }
                }
                /* B.) check for char-width */
                else if(strncmp(line,"C ",2)==0)
                {
                    if (sscanf(line,"%s%d%s%s%d",cString,&ascii,semiString,WXString,&cWidth)!=5)
                    {
                        wxLogDebug(wxT("AFM-file '%s': line '%s' has an error (bad character width)"),afmName.c_str(),line);
                    }
                    if(strcmp(cString,"C")!=0 || strcmp(semiString,";")!=0 || strcmp(WXString,"WX")!=0)
                    {
                        wxLogDebug(wxT("AFM-file '%s': line '%s' has a format error"),afmName.c_str(),line);
                    }
                    /* printf("            char '%c'=%d has width '%d'\n",ascii,ascii,cWidth); */
                    if (ascii>=0 && ascii<256)
                    {
                        lastWidths[ascii] = cWidth; /* store width */
                    }
                    else
                    {
                        /* MATTHEW: this happens a lot; don't print an error */
                        /* wxLogDebug("AFM-file '%s': ASCII value %d out of range",afmName.c_str(),ascii); */
                    }
                }
                /* C.) ignore other entries. */
            }
            fclose(afmFile);
        }
        /* hack to compute correct values for german 'Umlaute'
           /  the correct way would be to map the character names
           /  like 'adieresis' to corresp. positions of ISOEnc and read
           /  these values from AFM files, too. Maybe later ... */

        // NB: casts to int are needed to suppress gcc 3.3 warnings
        lastWidths[196] = lastWidths[(int)'A'];  // U+00C4 A Umlaute
        lastWidths[228] = lastWidths[(int)'a'];  // U+00E4 a Umlaute
        lastWidths[214] = lastWidths[(int)'O'];  // U+00D6 O Umlaute
        lastWidths[246] = lastWidths[(int)'o'];  // U+00F6 o Umlaute
        lastWidths[220] = lastWidths[(int)'U'];  // U+00DC U Umlaute
        lastWidths[252] = lastWidths[(int)'u'];  // U+00FC u Umlaute
        lastWidths[223] = lastWidths[(int)251];  // U+00DF eszett (scharfes s)

        /* JC: calculate UnderlineThickness/UnderlinePosition */

        // VS: dirty, but is there any better solution?
        double *pt;
        pt = (double*) &m_underlinePosition;
        *pt = LogicalToDeviceYRel((wxCoord)(UnderlinePosition * fontSize)) / 1000.0f;
        pt = (double*) &m_underlineThickness;
        *pt = LogicalToDeviceYRel((wxCoord)(UnderlineThickness * fontSize)) / 1000.0f;

    }


    /* 3. now the font metrics are read in, calc size this
       /  is done by adding the widths of the characters in the
       /  string. they are given in 1/1000 of the size! */

    long sum=0;
    float height=fontSize; /* by default */
    unsigned char *p;
    for(p=(unsigned char *)wxMBSTRINGCAST strbuf; *p; p++)
    {
        if(lastWidths[*p]== INT_MIN)
        {
            wxLogDebug(wxT("GetTextExtent: undefined width for character '%c' (%d)"), *p,*p);
            sum += lastWidths[(unsigned char)' ']; /* assume space */
        }
        else
        {
            sum += lastWidths[*p];
        }
    }

    double widthSum = sum;
    widthSum *= fontSize;
    widthSum /= 1000.0F;

    /* add descender to height (it is usually a negative value) */
    //if (lastDescender != INT_MIN)
    //{
    //    height += (wxCoord)(((-lastDescender)/1000.0F) * Size); /* MATTHEW: forgot scale */
    //}
    // - commented by V. Slavik - height already contains descender in it
    //   (judging from few experiments)

    /* return size values */
    if ( x )
        *x = (wxCoord)widthSum;
    if ( y )
        *y = (wxCoord)height;

    /* return other parameters */
    if (descent)
    {
        if(lastDescender!=INT_MIN)
        {
            *descent = (wxCoord)(((-lastDescender)/1000.0F) * fontSize); /* MATTHEW: forgot scale */
        }
        else
        {
            *descent = 0;
        }
    }

    /* currently no idea how to calculate this! */
    if (externalLeading) *externalLeading = 0;
#endif
    // Use AFM
}

// print postscript datas via required method (file, stream)
void wxPostScriptDC::PsPrintf( const wxChar* fmt, ... )
{
    va_list argptr;
    va_start(argptr, fmt);

    PsPrint( wxString::FormatV( fmt, argptr ).c_str() );
}

void wxPostScriptDC::PsPrint( const char* psdata )
{
    wxPostScriptPrintNativeData *data =
        (wxPostScriptPrintNativeData *) m_printData.GetNativeData();

    switch (m_printData.GetPrintMode())
    {
#if wxUSE_STREAMS
        // append to output stream
        case wxPRINT_MODE_STREAM:
            {
                wxOutputStream* outputstream = data->GetOutputStream();
                wxCHECK_RET( outputstream, wxT("invalid outputstream") );
                outputstream->Write( psdata, strlen( psdata ) );
            }
            break;
#endif // wxUSE_STREAMS

        // save data into file
        default:
            wxCHECK_RET( m_pstream, wxT("invalid postscript dc") );
            fwrite( psdata, 1, strlen( psdata ), m_pstream );
    }
}

void wxPostScriptDC::PsPrint( int ch )
{
    wxPostScriptPrintNativeData *data =
        (wxPostScriptPrintNativeData *) m_printData.GetNativeData();

    switch (m_printData.GetPrintMode())
    {
#if wxUSE_STREAMS
        // append to output stream
        case wxPRINT_MODE_STREAM:
            {
                wxOutputStream* outputstream = data->GetOutputStream();
                wxCHECK_RET( outputstream, wxT("invalid outputstream") );
                outputstream->PutC( (char)ch );
            }
            break;
#endif // wxUSE_STREAMS

        // save data into file
        default:
            wxCHECK_RET( m_pstream, wxT("invalid postscript dc") );
            fputc( ch, m_pstream );
    }
}

#endif // wxUSE_PRINTING_ARCHITECTURE && wxUSE_POSTSCRIPT

// vi:sts=4:sw=4:et
