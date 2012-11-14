/////////////////////////////////////////////////////////////////////////////
// Name:        wx/prntbase.h
// Purpose:     Base classes for printing framework
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: prntbase.h 62502 2009-10-27 16:39:01Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PRNTBASEH__
#define _WX_PRNTBASEH__

#include "wx/defs.h"

#if wxUSE_PRINTING_ARCHITECTURE

#include "wx/event.h"
#include "wx/cmndata.h"
#include "wx/panel.h"
#include "wx/scrolwin.h"
#include "wx/dialog.h"
#include "wx/frame.h"

class WXDLLIMPEXP_FWD_CORE wxDC;
class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxChoice;
class WXDLLIMPEXP_FWD_CORE wxPrintout;
class WXDLLIMPEXP_FWD_CORE wxPrinterBase;
class WXDLLIMPEXP_FWD_CORE wxPrintDialogBase;
class WXDLLIMPEXP_FWD_CORE wxPrintDialog;
class WXDLLIMPEXP_FWD_CORE wxPageSetupDialogBase;
class WXDLLIMPEXP_FWD_CORE wxPageSetupDialog;
class WXDLLIMPEXP_FWD_CORE wxPrintPreviewBase;
class WXDLLIMPEXP_FWD_CORE wxPreviewCanvas;
class WXDLLIMPEXP_FWD_CORE wxPreviewControlBar;
class WXDLLIMPEXP_FWD_CORE wxPreviewFrame;
class WXDLLIMPEXP_FWD_CORE wxPrintFactory;
class WXDLLIMPEXP_FWD_CORE wxPrintNativeDataBase;

//----------------------------------------------------------------------------
// error consts
//----------------------------------------------------------------------------

enum wxPrinterError
{
    wxPRINTER_NO_ERROR = 0,
    wxPRINTER_CANCELLED,
    wxPRINTER_ERROR
};

//----------------------------------------------------------------------------
// wxPrintFactory
//----------------------------------------------------------------------------

class WXDLLEXPORT wxPrintFactory
{
public:
    wxPrintFactory() {}
    virtual ~wxPrintFactory() {}

    virtual wxPrinterBase *CreatePrinter( wxPrintDialogData* data ) = 0;

    virtual wxPrintPreviewBase *CreatePrintPreview( wxPrintout *preview,
                                                    wxPrintout *printout = NULL,
                                                    wxPrintDialogData *data = NULL ) = 0;
    virtual wxPrintPreviewBase *CreatePrintPreview( wxPrintout *preview,
                                                    wxPrintout *printout,
                                                    wxPrintData *data ) = 0;

    virtual wxPrintDialogBase *CreatePrintDialog( wxWindow *parent,
                                                  wxPrintDialogData *data = NULL ) = 0;
    virtual wxPrintDialogBase *CreatePrintDialog( wxWindow *parent,
                                                  wxPrintData *data ) = 0;

    virtual wxPageSetupDialogBase *CreatePageSetupDialog( wxWindow *parent,
                                                          wxPageSetupDialogData * data = NULL ) = 0;

    virtual wxDC* CreatePrinterDC( const wxPrintData& data ) = 0;

    // What to do and what to show in the wxPrintDialog
    // a) Use the generic print setup dialog or a native one?
    virtual bool HasPrintSetupDialog() = 0;
    virtual wxDialog *CreatePrintSetupDialog( wxWindow *parent, wxPrintData *data ) = 0;
    // b) Provide the "print to file" option ourselves or via print setup?
    virtual bool HasOwnPrintToFile() = 0;
    // c) Show current printer
    virtual bool HasPrinterLine() = 0;
    virtual wxString CreatePrinterLine() = 0;
    // d) Show Status line for current printer?
    virtual bool HasStatusLine() = 0;
    virtual wxString CreateStatusLine() = 0;


    virtual wxPrintNativeDataBase *CreatePrintNativeData() = 0;

    static void SetPrintFactory( wxPrintFactory *factory );
    static wxPrintFactory *GetFactory();
private:
    static wxPrintFactory *m_factory;
};

class WXDLLEXPORT wxNativePrintFactory: public wxPrintFactory
{
public:
    virtual wxPrinterBase *CreatePrinter( wxPrintDialogData *data );

    virtual wxPrintPreviewBase *CreatePrintPreview( wxPrintout *preview,
                                                    wxPrintout *printout = NULL,
                                                    wxPrintDialogData *data = NULL );
    virtual wxPrintPreviewBase *CreatePrintPreview( wxPrintout *preview,
                                                    wxPrintout *printout,
                                                    wxPrintData *data );

    virtual wxPrintDialogBase *CreatePrintDialog( wxWindow *parent,
                                                  wxPrintDialogData *data = NULL );
    virtual wxPrintDialogBase *CreatePrintDialog( wxWindow *parent,
                                                  wxPrintData *data );

    virtual wxPageSetupDialogBase *CreatePageSetupDialog( wxWindow *parent,
                                                          wxPageSetupDialogData * data = NULL );

    virtual wxDC* CreatePrinterDC( const wxPrintData& data );

    virtual bool HasPrintSetupDialog();
    virtual wxDialog *CreatePrintSetupDialog( wxWindow *parent, wxPrintData *data );
    virtual bool HasOwnPrintToFile();
    virtual bool HasPrinterLine();
    virtual wxString CreatePrinterLine();
    virtual bool HasStatusLine();
    virtual wxString CreateStatusLine();

    virtual wxPrintNativeDataBase *CreatePrintNativeData();
};

//----------------------------------------------------------------------------
// wxPrintNativeDataBase
//----------------------------------------------------------------------------

class WXDLLEXPORT wxPrintNativeDataBase: public wxObject
{
public:
    wxPrintNativeDataBase();
    virtual ~wxPrintNativeDataBase() {}

    virtual bool TransferTo( wxPrintData &data ) = 0;
    virtual bool TransferFrom( const wxPrintData &data ) = 0;

    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const = 0;

    int  m_ref;

private:
    DECLARE_CLASS(wxPrintNativeDataBase)
    DECLARE_NO_COPY_CLASS(wxPrintNativeDataBase)
};

//----------------------------------------------------------------------------
// wxPrinterBase
//----------------------------------------------------------------------------

/*
 * Represents the printer: manages printing a wxPrintout object
 */

class WXDLLEXPORT wxPrinterBase: public wxObject
{
public:
    wxPrinterBase(wxPrintDialogData *data = (wxPrintDialogData *) NULL);
    virtual ~wxPrinterBase();

    virtual wxWindow *CreateAbortWindow(wxWindow *parent, wxPrintout *printout);
    virtual void ReportError(wxWindow *parent, wxPrintout *printout, const wxString& message);

    virtual wxPrintDialogData& GetPrintDialogData() const;
    bool GetAbort() const { return sm_abortIt; }

    static wxPrinterError GetLastError() { return sm_lastError; }

    ///////////////////////////////////////////////////////////////////////////
    // OVERRIDES

    virtual bool Setup(wxWindow *parent) = 0;
    virtual bool Print(wxWindow *parent, wxPrintout *printout, bool prompt = true) = 0;
    virtual wxDC* PrintDialog(wxWindow *parent) = 0;

protected:
    wxPrintDialogData     m_printDialogData;
    wxPrintout*           m_currentPrintout;

    static wxPrinterError sm_lastError;

public:
    static wxWindow*      sm_abortWindow;
    static bool           sm_abortIt;

private:
    DECLARE_CLASS(wxPrinterBase)
    DECLARE_NO_COPY_CLASS(wxPrinterBase)
};

//----------------------------------------------------------------------------
// wxPrinter
//----------------------------------------------------------------------------

class WXDLLEXPORT wxPrinter: public wxPrinterBase
{
public:
    wxPrinter(wxPrintDialogData *data = (wxPrintDialogData *) NULL);
    virtual ~wxPrinter();

    virtual wxWindow *CreateAbortWindow(wxWindow *parent, wxPrintout *printout);
    virtual void ReportError(wxWindow *parent, wxPrintout *printout, const wxString& message);

    virtual bool Setup(wxWindow *parent);
    virtual bool Print(wxWindow *parent, wxPrintout *printout, bool prompt = true);
    virtual wxDC* PrintDialog(wxWindow *parent);

    virtual wxPrintDialogData& GetPrintDialogData() const;

protected:
    wxPrinterBase    *m_pimpl;

private:
    DECLARE_CLASS(wxPrinter)
    DECLARE_NO_COPY_CLASS(wxPrinter)
};

//----------------------------------------------------------------------------
// wxPrintout
//----------------------------------------------------------------------------

/*
 * Represents an object via which a document may be printed.
 * The programmer derives from this, overrides (at least) OnPrintPage,
 * and passes it to a wxPrinter object for printing, or a wxPrintPreview
 * object for previewing.
 */

class WXDLLEXPORT wxPrintout: public wxObject
{
public:
    wxPrintout(const wxString& title = wxT("Printout"));
    virtual ~wxPrintout();

    virtual bool OnBeginDocument(int startPage, int endPage);
    virtual void OnEndDocument();
    virtual void OnBeginPrinting();
    virtual void OnEndPrinting();

    // Guaranteed to be before any other functions are called
    virtual void OnPreparePrinting() { }

    virtual bool HasPage(int page);
    virtual bool OnPrintPage(int page) = 0;
    virtual void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo);

    virtual wxString GetTitle() const { return m_printoutTitle; }

    wxDC *GetDC() const { return m_printoutDC; }
    void SetDC(wxDC *dc) { m_printoutDC = dc; }

    void FitThisSizeToPaper(const wxSize& imageSize);
    void FitThisSizeToPage(const wxSize& imageSize);
    void FitThisSizeToPageMargins(const wxSize& imageSize, const wxPageSetupDialogData& pageSetupData);
    void MapScreenSizeToPaper();
    void MapScreenSizeToPage();
    void MapScreenSizeToPageMargins(const wxPageSetupDialogData& pageSetupData);
    void MapScreenSizeToDevice();

    wxRect GetLogicalPaperRect() const;
    wxRect GetLogicalPageRect() const;
    wxRect GetLogicalPageMarginsRect(const wxPageSetupDialogData& pageSetupData) const;

    void SetLogicalOrigin(wxCoord x, wxCoord y);
    void OffsetLogicalOrigin(wxCoord xoff, wxCoord yoff);

    void SetPageSizePixels(int w, int  h) { m_pageWidthPixels = w; m_pageHeightPixels = h; }
    void GetPageSizePixels(int *w, int  *h) const { *w = m_pageWidthPixels; *h = m_pageHeightPixels; }
    void SetPageSizeMM(int w, int  h) { m_pageWidthMM = w; m_pageHeightMM = h; }
    void GetPageSizeMM(int *w, int  *h) const { *w = m_pageWidthMM; *h = m_pageHeightMM; }

    void SetPPIScreen(int x, int y) { m_PPIScreenX = x; m_PPIScreenY = y; }
    void GetPPIScreen(int *x, int *y) const { *x = m_PPIScreenX; *y = m_PPIScreenY; }
    void SetPPIPrinter(int x, int y) { m_PPIPrinterX = x; m_PPIPrinterY = y; }
    void GetPPIPrinter(int *x, int *y) const { *x = m_PPIPrinterX; *y = m_PPIPrinterY; }

    void SetPaperRectPixels(const wxRect& paperRectPixels) { m_paperRectPixels = paperRectPixels; }
    wxRect GetPaperRectPixels() const { return m_paperRectPixels; }

    virtual bool IsPreview() const { return m_isPreview; }

    virtual void SetIsPreview(bool p) { m_isPreview = p; }

private:
    wxString         m_printoutTitle;
    wxDC*            m_printoutDC;

    int              m_pageWidthPixels;
    int              m_pageHeightPixels;

    int              m_pageWidthMM;
    int              m_pageHeightMM;

    int              m_PPIScreenX;
    int              m_PPIScreenY;
    int              m_PPIPrinterX;
    int              m_PPIPrinterY;

    wxRect           m_paperRectPixels;

    bool             m_isPreview;

private:
    DECLARE_ABSTRACT_CLASS(wxPrintout)
    DECLARE_NO_COPY_CLASS(wxPrintout)
};

//----------------------------------------------------------------------------
// wxPreviewCanvas
//----------------------------------------------------------------------------

/*
 * Canvas upon which a preview is drawn.
 */

class WXDLLEXPORT wxPreviewCanvas: public wxScrolledWindow
{
public:
    wxPreviewCanvas(wxPrintPreviewBase *preview,
                    wxWindow *parent,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = 0,
                    const wxString& name = wxT("canvas"));
    virtual ~wxPreviewCanvas();

    void OnPaint(wxPaintEvent& event);
    void OnChar(wxKeyEvent &event);
    // Responds to colour changes
    void OnSysColourChanged(wxSysColourChangedEvent& event);

private:
#if wxUSE_MOUSEWHEEL
    void OnMouseWheel(wxMouseEvent& event);
#endif // wxUSE_MOUSEWHEEL

    wxPrintPreviewBase* m_printPreview;

    DECLARE_CLASS(wxPreviewCanvas)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPreviewCanvas)
};

//----------------------------------------------------------------------------
// wxPreviewFrame
//----------------------------------------------------------------------------

/*
 * Default frame for showing preview.
 */

class WXDLLEXPORT wxPreviewFrame: public wxFrame
{
public:
    wxPreviewFrame(wxPrintPreviewBase *preview,
                   wxWindow *parent,
                   const wxString& title = wxT("Print Preview"),
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT,
                   const wxString& name = wxFrameNameStr);
    virtual ~wxPreviewFrame();

    void OnCloseWindow(wxCloseEvent& event);
    virtual void Initialize();
    virtual void CreateCanvas();
    virtual void CreateControlBar();

    inline wxPreviewControlBar* GetControlBar() const { return m_controlBar; }

protected:
    wxPreviewCanvas*      m_previewCanvas;
    wxPreviewControlBar*  m_controlBar;
    wxPrintPreviewBase*   m_printPreview;
    wxWindowDisabler*     m_windowDisabler;

private:
    DECLARE_CLASS(wxPreviewFrame)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPreviewFrame)
};

//----------------------------------------------------------------------------
// wxPreviewControlBar
//----------------------------------------------------------------------------

/*
 * A panel with buttons for controlling a print preview.
 * The programmer may wish to use other means for controlling
 * the print preview.
 */

#define wxPREVIEW_PRINT        1
#define wxPREVIEW_PREVIOUS     2
#define wxPREVIEW_NEXT         4
#define wxPREVIEW_ZOOM         8
#define wxPREVIEW_FIRST       16
#define wxPREVIEW_LAST        32
#define wxPREVIEW_GOTO        64

#define wxPREVIEW_DEFAULT  (wxPREVIEW_PREVIOUS|wxPREVIEW_NEXT|wxPREVIEW_ZOOM\
                            |wxPREVIEW_FIRST|wxPREVIEW_GOTO|wxPREVIEW_LAST)

// Ids for controls
#define wxID_PREVIEW_CLOSE      1
#define wxID_PREVIEW_NEXT       2
#define wxID_PREVIEW_PREVIOUS   3
#define wxID_PREVIEW_PRINT      4
#define wxID_PREVIEW_ZOOM       5
#define wxID_PREVIEW_FIRST      6
#define wxID_PREVIEW_LAST       7
#define wxID_PREVIEW_GOTO       8

class WXDLLEXPORT wxPreviewControlBar: public wxPanel
{
    DECLARE_CLASS(wxPreviewControlBar)

public:
    wxPreviewControlBar(wxPrintPreviewBase *preview,
                        long buttons,
                        wxWindow *parent,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxTAB_TRAVERSAL,
                        const wxString& name = wxT("panel"));
    virtual ~wxPreviewControlBar();

    virtual void CreateButtons();
    virtual void SetZoomControl(int zoom);
    virtual int GetZoomControl();
    virtual wxPrintPreviewBase *GetPrintPreview() const
        { return m_printPreview; }

    void OnWindowClose(wxCommandEvent& event);
    void OnNext();
    void OnPrevious();
    void OnFirst();
    void OnLast();
    void OnGoto();
    void OnPrint();
    void OnPrintButton(wxCommandEvent& WXUNUSED(event)) { OnPrint(); }
    void OnNextButton(wxCommandEvent & WXUNUSED(event)) { OnNext(); }
    void OnPreviousButton(wxCommandEvent & WXUNUSED(event)) { OnPrevious(); }
    void OnFirstButton(wxCommandEvent & WXUNUSED(event)) { OnFirst(); }
    void OnLastButton(wxCommandEvent & WXUNUSED(event)) { OnLast(); }
    void OnGotoButton(wxCommandEvent & WXUNUSED(event)) { OnGoto(); }
    void OnZoom(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);

protected:
    wxPrintPreviewBase*   m_printPreview;
    wxButton*             m_closeButton;
    wxButton*             m_nextPageButton;
    wxButton*             m_previousPageButton;
    wxButton*             m_printButton;
    wxChoice*             m_zoomControl;
    wxButton*             m_firstPageButton;
    wxButton*             m_lastPageButton;
    wxButton*             m_gotoPageButton;
    long                  m_buttonFlags;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPreviewControlBar)
};

//----------------------------------------------------------------------------
// wxPrintPreviewBase
//----------------------------------------------------------------------------

/*
 * Programmer creates an object of this class to preview a wxPrintout.
 */

class WXDLLEXPORT wxPrintPreviewBase: public wxObject
{
public:
    wxPrintPreviewBase(wxPrintout *printout,
                       wxPrintout *printoutForPrinting = (wxPrintout *) NULL,
                       wxPrintDialogData *data = (wxPrintDialogData *) NULL);
    wxPrintPreviewBase(wxPrintout *printout,
                       wxPrintout *printoutForPrinting,
                       wxPrintData *data);
    virtual ~wxPrintPreviewBase();

    virtual bool SetCurrentPage(int pageNum);
    virtual int GetCurrentPage() const;

    virtual void SetPrintout(wxPrintout *printout);
    virtual wxPrintout *GetPrintout() const;
    virtual wxPrintout *GetPrintoutForPrinting() const;

    virtual void SetFrame(wxFrame *frame);
    virtual void SetCanvas(wxPreviewCanvas *canvas);

    virtual wxFrame *GetFrame() const;
    virtual wxPreviewCanvas *GetCanvas() const;

    // This is a helper routine, used by the next 4 routines.

    virtual void CalcRects(wxPreviewCanvas *canvas, wxRect& printableAreaRect, wxRect& paperRect);

    // The preview canvas should call this from OnPaint
    virtual bool PaintPage(wxPreviewCanvas *canvas, wxDC& dc);

    // This draws a blank page onto the preview canvas
    virtual bool DrawBlankPage(wxPreviewCanvas *canvas, wxDC& dc);

    // Adjusts the scrollbars for the current scale
    virtual void AdjustScrollbars(wxPreviewCanvas *canvas);

    // This is called by wxPrintPreview to render a page into a wxMemoryDC.
    virtual bool RenderPage(int pageNum);


    virtual void SetZoom(int percent);
    virtual int GetZoom() const;

    virtual wxPrintDialogData& GetPrintDialogData();

    virtual int GetMaxPage() const;
    virtual int GetMinPage() const;

    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const;
    virtual void SetOk(bool ok);

    ///////////////////////////////////////////////////////////////////////////
    // OVERRIDES

    // If we own a wxPrintout that can be used for printing, this
    // will invoke the actual printing procedure. Called
    // by the wxPreviewControlBar.
    virtual bool Print(bool interactive) = 0;

    // Calculate scaling that needs to be done to get roughly
    // the right scaling for the screen pretending to be
    // the currently selected printer.
    virtual void DetermineScaling() = 0;

protected:
    wxPrintDialogData m_printDialogData;
    wxPreviewCanvas*  m_previewCanvas;
    wxFrame*          m_previewFrame;
    wxBitmap*         m_previewBitmap;
    wxPrintout*       m_previewPrintout;
    wxPrintout*       m_printPrintout;
    int               m_currentPage;
    int               m_currentZoom;
    float             m_previewScaleX;
    float             m_previewScaleY;
    int               m_topMargin;
    int               m_leftMargin;
    int               m_pageWidth;
    int               m_pageHeight;
    int               m_minPage;
    int               m_maxPage;

    bool              m_isOk;
    bool              m_printingPrepared; // Called OnPreparePrinting?

private:
    void Init(wxPrintout *printout, wxPrintout *printoutForPrinting);

    // helpers for RenderPage():
    bool RenderPageIntoDC(wxDC& dc, int pageNum);
    bool RenderPageIntoBitmap(wxBitmap& bmp, int pageNum);

    DECLARE_NO_COPY_CLASS(wxPrintPreviewBase)
    DECLARE_CLASS(wxPrintPreviewBase)
};

//----------------------------------------------------------------------------
// wxPrintPreview
//----------------------------------------------------------------------------

class WXDLLEXPORT wxPrintPreview: public wxPrintPreviewBase
{
public:
    wxPrintPreview(wxPrintout *printout,
                   wxPrintout *printoutForPrinting = (wxPrintout *) NULL,
                   wxPrintDialogData *data = (wxPrintDialogData *) NULL);
    wxPrintPreview(wxPrintout *printout,
                   wxPrintout *printoutForPrinting,
                   wxPrintData *data);
    virtual ~wxPrintPreview();

    virtual bool SetCurrentPage(int pageNum);
    virtual int GetCurrentPage() const;
    virtual void SetPrintout(wxPrintout *printout);
    virtual wxPrintout *GetPrintout() const;
    virtual wxPrintout *GetPrintoutForPrinting() const;
    virtual void SetFrame(wxFrame *frame);
    virtual void SetCanvas(wxPreviewCanvas *canvas);

    virtual wxFrame *GetFrame() const;
    virtual wxPreviewCanvas *GetCanvas() const;
    virtual bool PaintPage(wxPreviewCanvas *canvas, wxDC& dc);
    virtual bool DrawBlankPage(wxPreviewCanvas *canvas, wxDC& dc);
    virtual void AdjustScrollbars(wxPreviewCanvas *canvas);
    virtual bool RenderPage(int pageNum);
    virtual void SetZoom(int percent);
    virtual int GetZoom() const;

    virtual bool Print(bool interactive);
    virtual void DetermineScaling();

    virtual wxPrintDialogData& GetPrintDialogData();

    virtual int GetMaxPage() const;
    virtual int GetMinPage() const;

    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const;
    virtual void SetOk(bool ok);

private:
    wxPrintPreviewBase *m_pimpl;

private:
    DECLARE_CLASS(wxPrintPreview)
    DECLARE_NO_COPY_CLASS(wxPrintPreview)
};

//----------------------------------------------------------------------------
// wxPrintAbortDialog
//----------------------------------------------------------------------------

class WXDLLEXPORT wxPrintAbortDialog: public wxDialog
{
public:
    wxPrintAbortDialog(wxWindow *parent,
                       const wxString& title,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = 0,
                       const wxString& name = wxT("dialog"))
        : wxDialog(parent, wxID_ANY, title, pos, size, style, name)
        {
        }

    void OnCancel(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPrintAbortDialog)
};

#endif // wxUSE_PRINTING_ARCHITECTURE

#endif
    // _WX_PRNTBASEH__
