/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/dcps.h
// Purpose:     wxPostScriptDC class
// Author:      Julian Smart and others
// Modified by:
// RCS-ID:      $Id: dcpsg.h 41751 2006-10-08 21:56:55Z VZ $
// Copyright:   (c) Julian Smart and Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DCPSG_H_
#define _WX_DCPSG_H_

#include "wx/defs.h"

#if wxUSE_PRINTING_ARCHITECTURE

#if wxUSE_POSTSCRIPT

#include "wx/dc.h"
#include "wx/dialog.h"
#include "wx/module.h"
#include "wx/cmndata.h"

extern WXDLLIMPEXP_DATA_CORE(int) wxPageNumber;

//-----------------------------------------------------------------------------
// classes
//-----------------------------------------------------------------------------

class wxPostScriptDC;

//-----------------------------------------------------------------------------
// wxPostScriptDC
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxPostScriptDC: public wxDC
{
public:
    wxPostScriptDC();

    // Recommended constructor
    wxPostScriptDC(const wxPrintData& printData);

    // Recommended destructor :-)
    virtual ~wxPostScriptDC();

  virtual bool Ok() const { return IsOk(); }
  virtual bool IsOk() const;

  bool CanDrawBitmap() const { return true; }

  void Clear();
  void SetFont( const wxFont& font );
  void SetPen( const wxPen& pen );
  void SetBrush( const wxBrush& brush );
  void SetLogicalFunction( int function );
  void SetBackground( const wxBrush& brush );

  void DestroyClippingRegion();

  bool StartDoc(const wxString& message);
  void EndDoc();
  void StartPage();
  void EndPage();

  wxCoord GetCharHeight() const;
  wxCoord GetCharWidth() const;
  bool CanGetTextExtent() const { return true; }

  // Resolution in pixels per logical inch
  wxSize GetPPI() const;

  void SetAxisOrientation( bool xLeftRight, bool yBottomUp );
  void SetDeviceOrigin( wxCoord x, wxCoord y );

  void SetBackgroundMode(int WXUNUSED(mode)) { }
  void SetPalette(const wxPalette& WXUNUSED(palette)) { }

  wxPrintData& GetPrintData() { return m_printData; }
  void SetPrintData(const wxPrintData& data) { m_printData = data; }

  virtual int GetDepth() const { return 24; }

  static void SetResolution(int ppi);
  static int GetResolution();

  void PsPrintf( const wxChar* fmt, ... );
  void PsPrint( const char* psdata );
  void PsPrint( int ch );

#if wxUSE_UNICODE
  void PsPrint( const wxChar* psdata ) { PsPrint( wxConvUTF8.cWX2MB( psdata ) ); }
#endif

private:
    static float ms_PSScaleFactor;

protected:
    bool DoFloodFill(wxCoord x1, wxCoord y1, const wxColour &col, int style = wxFLOOD_SURFACE);
    bool DoGetPixel(wxCoord x1, wxCoord y1, wxColour *col) const;
    void DoDrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2);
    void DoCrossHair(wxCoord x, wxCoord y) ;
    void DoDrawArc(wxCoord x1,wxCoord y1,wxCoord x2,wxCoord y2,wxCoord xc,wxCoord yc);
    void DoDrawEllipticArc(wxCoord x,wxCoord y,wxCoord w,wxCoord h,double sa,double ea);
    void DoDrawPoint(wxCoord x, wxCoord y);
    void DoDrawLines(int n, wxPoint points[], wxCoord xoffset = 0, wxCoord yoffset = 0);
    void DoDrawPolygon(int n, wxPoint points[], wxCoord xoffset = 0, wxCoord yoffset = 0, int fillStyle = wxODDEVEN_RULE);
    void DoDrawPolyPolygon(int n, int count[], wxPoint points[], wxCoord xoffset = 0, wxCoord yoffset = 0, int fillStyle = wxODDEVEN_RULE);
    void DoDrawRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height);
    void DoDrawRoundedRectangle(wxCoord x, wxCoord y, wxCoord width, wxCoord height, double radius = 20);
    void DoDrawEllipse(wxCoord x, wxCoord y, wxCoord width, wxCoord height);
#if wxUSE_SPLINES
    void DoDrawSpline(wxList *points);
#endif // wxUSE_SPLINES
    bool DoBlit(wxCoord xdest, wxCoord ydest, wxCoord width, wxCoord height,
                wxDC *source, wxCoord xsrc, wxCoord ysrc, int rop = wxCOPY, bool useMask = false,
                wxCoord xsrcMask = wxDefaultCoord, wxCoord ysrcMask = wxDefaultCoord);
    void DoDrawIcon(const wxIcon& icon, wxCoord x, wxCoord y);
    void DoDrawBitmap(const wxBitmap& bitmap, wxCoord x, wxCoord y, bool useMask = false);
    void DoDrawText(const wxString& text, wxCoord x, wxCoord y);
    void DoDrawRotatedText(const wxString& text, wxCoord x, wxCoord y, double angle);
    void DoSetClippingRegion(wxCoord x, wxCoord y, wxCoord width, wxCoord height);
    void DoSetClippingRegionAsRegion( const wxRegion &WXUNUSED(clip)) { }
    void DoGetTextExtent(const wxString& string, wxCoord *x, wxCoord *y,
                         wxCoord *descent = NULL,
                         wxCoord *externalLeading = NULL,
                         wxFont *theFont = NULL) const;
    void DoGetSize(int* width, int* height) const;
    void DoGetSizeMM(int *width, int *height) const;

    FILE*             m_pstream;    // PostScript output stream
    wxString          m_title;
    unsigned char     m_currentRed;
    unsigned char     m_currentGreen;
    unsigned char     m_currentBlue;
    int               m_pageNumber;
    bool              m_clipping;
    double            m_underlinePosition;
    double            m_underlineThickness;
    wxPrintData       m_printData;

private:
    DECLARE_DYNAMIC_CLASS(wxPostScriptDC)
};

#endif
    // wxUSE_POSTSCRIPT

#endif
    // wxUSE_PRINTING_ARCHITECTURE

#endif
        // _WX_DCPSG_H_
