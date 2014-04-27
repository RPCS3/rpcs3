/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/prntbase.cpp
// Purpose:     Printing framework base class implementation
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: prntbase.cpp 58209 2009-01-18 21:31:36Z RD $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_PRINTING_ARCHITECTURE

// change this to 1 to use experimental high-quality printing on Windows
// (NB: this can't be in msw/printwin.cpp because of binary compatibility)
#define wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW 0

#if wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW
    #if !defined(__WXMSW__) || !wxUSE_IMAGE || !wxUSE_WXDIB
        #undef wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW
        #define wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW 0
    #endif
#endif

#include "wx/dcprint.h"

#ifndef WX_PRECOMP
    #if defined(__WXMSW__)
        #include "wx/msw/wrapcdlg.h"
    #endif // MSW
    #include "wx/utils.h"
    #include "wx/dc.h"
    #include "wx/app.h"
    #include "wx/math.h"
    #include "wx/msgdlg.h"
    #include "wx/layout.h"
    #include "wx/choice.h"
    #include "wx/button.h"
    #include "wx/settings.h"
    #include "wx/dcmemory.h"
    #include "wx/stattext.h"
    #include "wx/intl.h"
    #include "wx/textdlg.h"
    #include "wx/sizer.h"
    #include "wx/module.h"
#endif // !WX_PRECOMP

#include "wx/prntbase.h"
#include "wx/printdlg.h"
#include "wx/print.h"
#include "wx/dcprint.h"

#include <stdlib.h>
#include <string.h>

#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
#include "wx/msw/printdlg.h"
#elif defined(__WXMAC__)
#include "wx/mac/printdlg.h"
#include "wx/mac/private/print.h"
#else
#include "wx/generic/prntdlgg.h"
#include "wx/dcps.h"
#endif

#ifdef __WXMSW__
    #ifndef __WIN32__
        #include <print.h>
    #endif
#endif // __WXMSW__

#if wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW

#include "wx/msw/dib.h"
#include "wx/image.h"

typedef bool (wxPrintPreviewBase::*RenderPageIntoDCFunc)(wxDC&, int);
static bool RenderPageIntoBitmapHQ(wxPrintPreviewBase *preview,
                                   RenderPageIntoDCFunc RenderPageIntoDC,
                                   wxPrinterDC& printerDC,
                                   wxBitmap& bmp, int pageNum,
                                   int pageWidth, int pageHeight);
#endif // wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW

//----------------------------------------------------------------------------
// wxPrintFactory
//----------------------------------------------------------------------------

wxPrintFactory *wxPrintFactory::m_factory = NULL;

void wxPrintFactory::SetPrintFactory( wxPrintFactory *factory )
{
    if (wxPrintFactory::m_factory)
        delete wxPrintFactory::m_factory;

    wxPrintFactory::m_factory = factory;
}

wxPrintFactory *wxPrintFactory::GetFactory()
{
    if (!wxPrintFactory::m_factory)
        wxPrintFactory::m_factory = new wxNativePrintFactory;

    return wxPrintFactory::m_factory;
}

//----------------------------------------------------------------------------
// wxNativePrintFactory
//----------------------------------------------------------------------------

wxPrinterBase *wxNativePrintFactory::CreatePrinter( wxPrintDialogData *data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxWindowsPrinter( data );
#elif defined(__WXMAC__)
    return new wxMacPrinter( data );
#elif defined(__WXPM__)
    return new wxOS2Printer( data );
#else
    return new wxPostScriptPrinter( data );
#endif
}

wxPrintPreviewBase *wxNativePrintFactory::CreatePrintPreview( wxPrintout *preview,
    wxPrintout *printout, wxPrintDialogData *data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxWindowsPrintPreview( preview, printout, data );
#elif defined(__WXMAC__)
    return new wxMacPrintPreview( preview, printout, data );
#elif defined(__WXPM__)
    return new wxOS2PrintPreview( preview, printout, data );
#else
    return new wxPostScriptPrintPreview( preview, printout, data );
#endif
}

wxPrintPreviewBase *wxNativePrintFactory::CreatePrintPreview( wxPrintout *preview,
    wxPrintout *printout, wxPrintData *data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxWindowsPrintPreview( preview, printout, data );
#elif defined(__WXMAC__)
    return new wxMacPrintPreview( preview, printout, data );
#elif defined(__WXPM__)
    return new wxOS2PrintPreview( preview, printout, data );
#else
    return new wxPostScriptPrintPreview( preview, printout, data );
#endif
}

wxPrintDialogBase *wxNativePrintFactory::CreatePrintDialog( wxWindow *parent,
                                                  wxPrintDialogData *data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxWindowsPrintDialog( parent, data );
#elif defined(__WXMAC__)
    return new wxMacPrintDialog( parent, data );
#else
    return new wxGenericPrintDialog( parent, data );
#endif
}

wxPrintDialogBase *wxNativePrintFactory::CreatePrintDialog( wxWindow *parent,
                                                  wxPrintData *data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxWindowsPrintDialog( parent, data );
#elif defined(__WXMAC__)
    return new wxMacPrintDialog( parent, data );
#else
    return new wxGenericPrintDialog( parent, data );
#endif
}

wxPageSetupDialogBase *wxNativePrintFactory::CreatePageSetupDialog( wxWindow *parent,
                                                  wxPageSetupDialogData *data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxWindowsPageSetupDialog( parent, data );
#elif defined(__WXMAC__)
    return new wxMacPageSetupDialog( parent, data );
#else
    return new wxGenericPageSetupDialog( parent, data );
#endif
}

bool wxNativePrintFactory::HasPrintSetupDialog()
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return false;
#elif defined(__WXMAC__)
    return false;
#else
    // Only here do we need to provide the print setup
    // dialog ourselves, the other platforms either have
    // none, don't make it accessible or let you configure
    // the printer from the wxPrintDialog anyway.
    return true;
#endif

}

wxDialog *wxNativePrintFactory::CreatePrintSetupDialog( wxWindow *parent,
                                                        wxPrintData *data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    wxUnusedVar(parent);
    wxUnusedVar(data);
    return NULL;
#elif defined(__WXMAC__)
    wxUnusedVar(parent);
    wxUnusedVar(data);
    return NULL;
#else
    // Only here do we need to provide the print setup
    // dialog ourselves, the other platforms either have
    // none, don't make it accessible or let you configure
    // the printer from the wxPrintDialog anyway.
    return new wxGenericPrintSetupDialog( parent, data );
#endif
}

wxDC* wxNativePrintFactory::CreatePrinterDC( const wxPrintData& data )
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxPrinterDC(data);
#elif defined(__WXMAC__)
    return new wxPrinterDC(data);
#else
    return new wxPostScriptDC(data);
#endif
}

bool wxNativePrintFactory::HasOwnPrintToFile()
{
    // Only relevant for PostScript and here the
    // setup dialog provides no "print to file"
    // option. In the GNOME setup dialog, the
    // setup dialog has its own print to file.
    return false;
}

bool wxNativePrintFactory::HasPrinterLine()
{
    // Only relevant for PostScript for now
    return true;
}

wxString wxNativePrintFactory::CreatePrinterLine()
{
    // Only relevant for PostScript for now

    // We should query "lpstat -d" here
    return _("Generic PostScript");
}

bool wxNativePrintFactory::HasStatusLine()
{
    // Only relevant for PostScript for now
    return true;
}

wxString wxNativePrintFactory::CreateStatusLine()
{
    // Only relevant for PostScript for now

    // We should query "lpstat -r" or "lpstat -p" here
    return _("Ready");
}

wxPrintNativeDataBase *wxNativePrintFactory::CreatePrintNativeData()
{
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    return new wxWindowsPrintNativeData;
#elif defined(__WXMAC__)
    return new wxMacCarbonPrintData;
#else
    return new wxPostScriptPrintNativeData;
#endif
}

//----------------------------------------------------------------------------
// wxPrintNativeDataBase
//----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxPrintNativeDataBase, wxObject)

wxPrintNativeDataBase::wxPrintNativeDataBase()
{
    m_ref = 1;
}

//----------------------------------------------------------------------------
// wxPrintFactoryModule
//----------------------------------------------------------------------------

class wxPrintFactoryModule: public wxModule
{
public:
    wxPrintFactoryModule() {}
    bool OnInit() { return true; }
    void OnExit() { wxPrintFactory::SetPrintFactory( NULL ); }

private:
    DECLARE_DYNAMIC_CLASS(wxPrintFactoryModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxPrintFactoryModule, wxModule)

//----------------------------------------------------------------------------
// wxPrinterBase
//----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxPrinterBase, wxObject)

wxPrinterBase::wxPrinterBase(wxPrintDialogData *data)
{
    m_currentPrintout = (wxPrintout *) NULL;
    sm_abortWindow = (wxWindow *) NULL;
    sm_abortIt = false;
    if (data)
        m_printDialogData = (*data);
    sm_lastError = wxPRINTER_NO_ERROR;
}

wxWindow *wxPrinterBase::sm_abortWindow = (wxWindow *) NULL;
bool wxPrinterBase::sm_abortIt = false;
wxPrinterError wxPrinterBase::sm_lastError = wxPRINTER_NO_ERROR;

wxPrinterBase::~wxPrinterBase()
{
}

wxWindow *wxPrinterBase::CreateAbortWindow(wxWindow *parent, wxPrintout * printout)
{
    wxPrintAbortDialog *dialog = new wxPrintAbortDialog(parent, _("Printing ") , wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);

    wxBoxSizer *button_sizer = new wxBoxSizer( wxVERTICAL );
    button_sizer->Add( new wxStaticText(dialog, wxID_ANY, _("Please wait while printing\n") + printout->GetTitle() ), 0, wxALL, 10 );
    button_sizer->Add( new wxButton( dialog, wxID_CANCEL, wxT("Cancel") ), 0, wxALL | wxALIGN_CENTER, 10 );

    dialog->SetAutoLayout( true );
    dialog->SetSizer( button_sizer );

    button_sizer->Fit(dialog);
    button_sizer->SetSizeHints (dialog) ;

    return dialog;
}

void wxPrinterBase::ReportError(wxWindow *parent, wxPrintout *WXUNUSED(printout), const wxString& message)
{
    wxMessageBox(message, _("Printing Error"), wxOK, parent);
}

wxPrintDialogData& wxPrinterBase::GetPrintDialogData() const
{
    return (wxPrintDialogData&) m_printDialogData;
}

//----------------------------------------------------------------------------
// wxPrinter
//----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxPrinter, wxPrinterBase)

wxPrinter::wxPrinter(wxPrintDialogData *data)
{
    m_pimpl = wxPrintFactory::GetFactory()->CreatePrinter( data );
}

wxPrinter::~wxPrinter()
{
    delete m_pimpl;
}

wxWindow *wxPrinter::CreateAbortWindow(wxWindow *parent, wxPrintout *printout)
{
    return m_pimpl->CreateAbortWindow( parent, printout );
}

void wxPrinter::ReportError(wxWindow *parent, wxPrintout *printout, const wxString& message)
{
    m_pimpl->ReportError( parent, printout, message );
}

bool wxPrinter::Setup(wxWindow *parent)
{
    return m_pimpl->Setup( parent );
}

bool wxPrinter::Print(wxWindow *parent, wxPrintout *printout, bool prompt)
{
    return m_pimpl->Print( parent, printout, prompt );
}

wxDC* wxPrinter::PrintDialog(wxWindow *parent)
{
    return m_pimpl->PrintDialog( parent );
}

wxPrintDialogData& wxPrinter::GetPrintDialogData() const
{
    return m_pimpl->GetPrintDialogData();
}

// ---------------------------------------------------------------------------
// wxPrintDialogBase: the dialog for printing.
// ---------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxPrintDialogBase, wxDialog)

wxPrintDialogBase::wxPrintDialogBase(wxWindow *parent,
                                     wxWindowID id,
                                     const wxString &title,
                                     const wxPoint &pos,
                                     const wxSize &size,
                                     long style)
    : wxDialog( parent, id, title.empty() ? wxString(_("Print")) : title,
                pos, size, style )
{
}

// ---------------------------------------------------------------------------
// wxPrintDialog: the dialog for printing
// ---------------------------------------------------------------------------

IMPLEMENT_CLASS(wxPrintDialog, wxObject)

wxPrintDialog::wxPrintDialog(wxWindow *parent, wxPrintDialogData* data)
{
    m_pimpl = wxPrintFactory::GetFactory()->CreatePrintDialog( parent, data );
}

wxPrintDialog::wxPrintDialog(wxWindow *parent, wxPrintData* data)
{
    m_pimpl = wxPrintFactory::GetFactory()->CreatePrintDialog( parent, data );
}

wxPrintDialog::~wxPrintDialog()
{
    delete m_pimpl;
}

int wxPrintDialog::ShowModal()
{
    return m_pimpl->ShowModal();
}

wxPrintDialogData& wxPrintDialog::GetPrintDialogData()
{
    return m_pimpl->GetPrintDialogData();
}

wxPrintData& wxPrintDialog::GetPrintData()
{
    return m_pimpl->GetPrintData();
}

wxDC *wxPrintDialog::GetPrintDC()
{
    return m_pimpl->GetPrintDC();
}

// ---------------------------------------------------------------------------
// wxPageSetupDialogBase: the page setup dialog
// ---------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxPageSetupDialogBase, wxDialog)

wxPageSetupDialogBase::wxPageSetupDialogBase(wxWindow *parent,
                                     wxWindowID id,
                                     const wxString &title,
                                     const wxPoint &pos,
                                     const wxSize &size,
                                     long style)
    : wxDialog( parent, id, title.empty() ? wxString(_("Page setup")) : title,
                pos, size, style )
{
}

// ---------------------------------------------------------------------------
// wxPageSetupDialog: the page setup dialog
// ---------------------------------------------------------------------------

IMPLEMENT_CLASS(wxPageSetupDialog, wxObject)

wxPageSetupDialog::wxPageSetupDialog(wxWindow *parent, wxPageSetupDialogData *data )
{
    m_pimpl = wxPrintFactory::GetFactory()->CreatePageSetupDialog( parent, data );
}

wxPageSetupDialog::~wxPageSetupDialog()
{
    delete m_pimpl;
}

int wxPageSetupDialog::ShowModal()
{
    return m_pimpl->ShowModal();
}

wxPageSetupDialogData& wxPageSetupDialog::GetPageSetupDialogData()
{
    return m_pimpl->GetPageSetupDialogData();
}

// old name
wxPageSetupDialogData& wxPageSetupDialog::GetPageSetupData()
{
    return m_pimpl->GetPageSetupDialogData();
}

//----------------------------------------------------------------------------
// wxPrintAbortDialog
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxPrintAbortDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL, wxPrintAbortDialog::OnCancel)
END_EVENT_TABLE()

void wxPrintAbortDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    wxPrinterBase::sm_abortIt = true;
    wxPrinterBase::sm_abortWindow->Show(false);
    wxPrinterBase::sm_abortWindow->Close(true);
    wxPrinterBase::sm_abortWindow->Destroy();
    wxPrinterBase::sm_abortWindow = (wxWindow *) NULL;
}

//----------------------------------------------------------------------------
// wxPrintout
//----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxPrintout, wxObject)

wxPrintout::wxPrintout(const wxString& title)
{
    m_printoutTitle = title ;
    m_printoutDC = (wxDC *) NULL;
    m_pageWidthMM = 0;
    m_pageHeightMM = 0;
    m_pageWidthPixels = 0;
    m_pageHeightPixels = 0;
    m_PPIScreenX = 0;
    m_PPIScreenY = 0;
    m_PPIPrinterX = 0;
    m_PPIPrinterY = 0;
    m_isPreview = false;
}

wxPrintout::~wxPrintout()
{
}

bool wxPrintout::OnBeginDocument(int WXUNUSED(startPage), int WXUNUSED(endPage))
{
    return GetDC()->StartDoc(_("Printing ") + m_printoutTitle);
}

void wxPrintout::OnEndDocument()
{
    GetDC()->EndDoc();
}

void wxPrintout::OnBeginPrinting()
{
}

void wxPrintout::OnEndPrinting()
{
}

bool wxPrintout::HasPage(int page)
{
    return (page == 1);
}

void wxPrintout::GetPageInfo(int *minPage, int *maxPage, int *fromPage, int *toPage)
{
    *minPage = 1;
    *maxPage = 32000;
    *fromPage = 1;
    *toPage = 1;
}

void wxPrintout::FitThisSizeToPaper(const wxSize& imageSize)
{
    // Set the DC scale and origin so that the given image size fits within the
    // entire page and the origin is at the top left corner of the page. Note
    // that with most printers, portions of the page will be non-printable. Use
    // this if you're managing your own page margins.
    if (!m_printoutDC) return;
    wxRect paperRect = GetPaperRectPixels();
    wxCoord pw, ph;
    GetPageSizePixels(&pw, &ph);
    wxCoord w, h;
    m_printoutDC->GetSize(&w, &h);
    float scaleX = ((float(paperRect.width) * w) / (float(pw) * imageSize.x));
    float scaleY = ((float(paperRect.height) * h) / (float(ph) * imageSize.y));
    float actualScale = wxMin(scaleX, scaleY);
    m_printoutDC->SetUserScale(actualScale, actualScale);
    m_printoutDC->SetDeviceOrigin(0, 0);
    wxRect logicalPaperRect = GetLogicalPaperRect();
    SetLogicalOrigin(logicalPaperRect.x, logicalPaperRect.y);
}

void wxPrintout::FitThisSizeToPage(const wxSize& imageSize)
{
    // Set the DC scale and origin so that the given image size fits within the
    // printable area of the page and the origin is at the top left corner of
    // the printable area.
    if (!m_printoutDC) return;
    int w, h;
    m_printoutDC->GetSize(&w, &h);
    float scaleX = float(w) / imageSize.x;
    float scaleY = float(h) / imageSize.y;
    float actualScale = wxMin(scaleX, scaleY);
    m_printoutDC->SetUserScale(actualScale, actualScale);
    m_printoutDC->SetDeviceOrigin(0, 0);
}

void wxPrintout::FitThisSizeToPageMargins(const wxSize& imageSize, const wxPageSetupDialogData& pageSetupData)
{
    // Set the DC scale and origin so that the given image size fits within the
    // page margins defined in the given wxPageSetupDialogData object and the
    // origin is at the top left corner of the page margins.
    if (!m_printoutDC) return;
    wxRect paperRect = GetPaperRectPixels();
    wxCoord pw, ph;
    GetPageSizePixels(&pw, &ph);
    wxPoint topLeft = pageSetupData.GetMarginTopLeft();
    wxPoint bottomRight = pageSetupData.GetMarginBottomRight();
    wxCoord mw, mh;
    GetPageSizeMM(&mw, &mh);
    float mmToDeviceX = float(pw) / mw;
    float mmToDeviceY = float(ph) / mh;
    wxRect pageMarginsRect(paperRect.x + wxRound(mmToDeviceX * topLeft.x),
        paperRect.y + wxRound(mmToDeviceY * topLeft.y),
        paperRect.width - wxRound(mmToDeviceX * (topLeft.x + bottomRight.x)),
        paperRect.height - wxRound(mmToDeviceY * (topLeft.y + bottomRight.y)));
    wxCoord w, h;
    m_printoutDC->GetSize(&w, &h);
    float scaleX = (float(pageMarginsRect.width) * w) / (float(pw) * imageSize.x);
    float scaleY = (float(pageMarginsRect.height) * h) / (float(ph) * imageSize.y);
    float actualScale = wxMin(scaleX, scaleY);
    m_printoutDC->SetUserScale(actualScale, actualScale);
    m_printoutDC->SetDeviceOrigin(0, 0);
    wxRect logicalPageMarginsRect = GetLogicalPageMarginsRect(pageSetupData);
    SetLogicalOrigin(logicalPageMarginsRect.x, logicalPageMarginsRect.y);
}

void wxPrintout::MapScreenSizeToPaper()
{
    // Set the DC scale so that an image on the screen is the same size on the
    // paper and the origin is at the top left of the paper. Note that with most
    // printers, portions of the page will be cut off. Use this if you're
    // managing your own page margins.
    if (!m_printoutDC) return;
    MapScreenSizeToPage();
    wxRect logicalPaperRect = GetLogicalPaperRect();
    SetLogicalOrigin(logicalPaperRect.x, logicalPaperRect.y);
}

void wxPrintout::MapScreenSizeToPage()
{
    // Set the DC scale and origin so that an image on the screen is the same
    // size on the paper and the origin is at the top left of the printable area.
    if (!m_printoutDC) return;
    int ppiScreenX, ppiScreenY;
    GetPPIScreen(&ppiScreenX, &ppiScreenY);
    int ppiPrinterX, ppiPrinterY;
    GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
    int w, h;
    m_printoutDC->GetSize(&w, &h);
    int pageSizePixelsX, pageSizePixelsY;
    GetPageSizePixels(&pageSizePixelsX, &pageSizePixelsY);
    float userScaleX = (float(ppiPrinterX) * w) / (float(ppiScreenX) * pageSizePixelsX);
    float userScaleY = (float(ppiPrinterY) * h) / (float(ppiScreenY) * pageSizePixelsY);
    m_printoutDC->SetUserScale(userScaleX, userScaleY);
    m_printoutDC->SetDeviceOrigin(0, 0);
}

void wxPrintout::MapScreenSizeToPageMargins(const wxPageSetupDialogData& pageSetupData)
{
    // Set the DC scale so that an image on the screen is the same size on the
    // paper and the origin is at the top left of the page margins defined by
    // the given wxPageSetupDialogData object.
    if (!m_printoutDC) return;
    MapScreenSizeToPage();
    wxRect logicalPageMarginsRect = GetLogicalPageMarginsRect(pageSetupData);
    SetLogicalOrigin(logicalPageMarginsRect.x, logicalPageMarginsRect.y);
}

void wxPrintout::MapScreenSizeToDevice()
{
    // Set the DC scale so that a screen pixel is the same size as a device
    // pixel and the origin is at the top left of the printable area.
    if (!m_printoutDC) return;
    int w, h;
    m_printoutDC->GetSize(&w, &h);
    int pageSizePixelsX, pageSizePixelsY;
    GetPageSizePixels(&pageSizePixelsX, &pageSizePixelsY);
    float userScaleX = float(w) / pageSizePixelsX;
    float userScaleY = float(h) / pageSizePixelsY;
    m_printoutDC->SetUserScale(userScaleX, userScaleY);
    m_printoutDC->SetDeviceOrigin(0, 0);
}

wxRect wxPrintout::GetLogicalPaperRect() const
{
    // Return the rectangle in logical units that corresponds to the paper
    // rectangle.
    wxRect paperRect = GetPaperRectPixels();
    wxCoord pw, ph;
    GetPageSizePixels(&pw, &ph);
    wxCoord w, h;
    m_printoutDC->GetSize(&w, &h);
    if (w == pw && h == ph) {
        // this DC matches the printed page, so no scaling
        return wxRect(m_printoutDC->DeviceToLogicalX(paperRect.x),
            m_printoutDC->DeviceToLogicalY(paperRect.y),
            m_printoutDC->DeviceToLogicalXRel(paperRect.width),
            m_printoutDC->DeviceToLogicalYRel(paperRect.height));
    }
    // This DC doesn't match the printed page, so we have to scale.
    float scaleX = float(w) / pw;
    float scaleY = float(h) / ph;
    return wxRect(m_printoutDC->DeviceToLogicalX(wxRound(paperRect.x * scaleX)),
        m_printoutDC->DeviceToLogicalY(wxRound(paperRect.y * scaleY)),
        m_printoutDC->DeviceToLogicalXRel(wxRound(paperRect.width * scaleX)),
        m_printoutDC->DeviceToLogicalYRel(wxRound(paperRect.height * scaleY)));
}

wxRect wxPrintout::GetLogicalPageRect() const
{
    // Return the rectangle in logical units that corresponds to the printable
    // area.
    int w, h;
    m_printoutDC->GetSize(&w, &h);
    return wxRect(m_printoutDC->DeviceToLogicalX(0),
        m_printoutDC->DeviceToLogicalY(0),
        m_printoutDC->DeviceToLogicalXRel(w),
        m_printoutDC->DeviceToLogicalYRel(h));
}

wxRect wxPrintout::GetLogicalPageMarginsRect(const wxPageSetupDialogData& pageSetupData) const
{
    // Return the rectangle in logical units that corresponds to the region
    // within the page margins as specified by the given wxPageSetupDialogData
    // object.
    wxRect paperRect = GetPaperRectPixels();
    wxCoord pw, ph;
    GetPageSizePixels(&pw, &ph);
    wxPoint topLeft = pageSetupData.GetMarginTopLeft();
    wxPoint bottomRight = pageSetupData.GetMarginBottomRight();
    wxCoord mw, mh;
    GetPageSizeMM(&mw, &mh);
    float mmToDeviceX = float(pw) / mw;
    float mmToDeviceY = float(ph) / mh;
    wxRect pageMarginsRect(paperRect.x + wxRound(mmToDeviceX * topLeft.x),
        paperRect.y + wxRound(mmToDeviceY * topLeft.y),
        paperRect.width - wxRound(mmToDeviceX * (topLeft.x + bottomRight.x)),
        paperRect.height - wxRound(mmToDeviceY * (topLeft.y + bottomRight.y)));
    wxCoord w, h;
    m_printoutDC->GetSize(&w, &h);
    if (w == pw && h == ph) {
        // this DC matches the printed page, so no scaling
        return wxRect(m_printoutDC->DeviceToLogicalX(pageMarginsRect.x),
            m_printoutDC->DeviceToLogicalY(pageMarginsRect.y),
            m_printoutDC->DeviceToLogicalXRel(pageMarginsRect.width),
            m_printoutDC->DeviceToLogicalYRel(pageMarginsRect.height));
    }
    // This DC doesn't match the printed page, so we have to scale.
    float scaleX = float(w) / pw;
    float scaleY = float(h) / ph;
    return wxRect(m_printoutDC->DeviceToLogicalX(wxRound(pageMarginsRect.x * scaleX)),
        m_printoutDC->DeviceToLogicalY(wxRound(pageMarginsRect.y * scaleY)),
        m_printoutDC->DeviceToLogicalXRel(wxRound(pageMarginsRect.width * scaleX)),
        m_printoutDC->DeviceToLogicalYRel(wxRound(pageMarginsRect.height * scaleY)));
}

void wxPrintout::SetLogicalOrigin(wxCoord x, wxCoord y)
{
    // Set the device origin by specifying a point in logical coordinates.
    m_printoutDC->SetDeviceOrigin(m_printoutDC->LogicalToDeviceX(x),
        m_printoutDC->LogicalToDeviceY(y));
}

void wxPrintout::OffsetLogicalOrigin(wxCoord xoff, wxCoord yoff)
{
    // Offset the device origin by a specified distance in device coordinates.
    wxCoord x = m_printoutDC->LogicalToDeviceX(0);
    wxCoord y = m_printoutDC->LogicalToDeviceY(0);
    m_printoutDC->SetDeviceOrigin(x + m_printoutDC->LogicalToDeviceXRel(xoff),
        y + m_printoutDC->LogicalToDeviceYRel(yoff));
}


//----------------------------------------------------------------------------
// wxPreviewCanvas
//----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxPreviewCanvas, wxWindow)

BEGIN_EVENT_TABLE(wxPreviewCanvas, wxScrolledWindow)
    EVT_PAINT(wxPreviewCanvas::OnPaint)
    EVT_CHAR(wxPreviewCanvas::OnChar)
    EVT_SYS_COLOUR_CHANGED(wxPreviewCanvas::OnSysColourChanged)
#if wxUSE_MOUSEWHEEL
    EVT_MOUSEWHEEL(wxPreviewCanvas::OnMouseWheel)
#endif
END_EVENT_TABLE()

// VZ: the current code doesn't refresh properly without
//     wxFULL_REPAINT_ON_RESIZE, this must be fixed as otherwise we have
//     really horrible flicker when resizing the preview frame, but without
//     this style it simply doesn't work correctly at all...
wxPreviewCanvas::wxPreviewCanvas(wxPrintPreviewBase *preview, wxWindow *parent,
                                 const wxPoint& pos, const wxSize& size, long style, const wxString& name):
wxScrolledWindow(parent, wxID_ANY, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name)
{
    m_printPreview = preview;
#ifdef __WXMAC__
    // The app workspace colour is always white, but we should have
    // a contrast with the page.
    wxSystemColour colourIndex = wxSYS_COLOUR_3DDKSHADOW;
#elif defined(__WXGTK__)
    wxSystemColour colourIndex = wxSYS_COLOUR_BTNFACE;
#else
    wxSystemColour colourIndex = wxSYS_COLOUR_APPWORKSPACE;
#endif
    SetBackgroundColour(wxSystemSettings::GetColour(colourIndex));

    SetScrollbars(10, 10, 100, 100);
}

wxPreviewCanvas::~wxPreviewCanvas()
{
}

void wxPreviewCanvas::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    PrepareDC( dc );

/*
#ifdef __WXGTK__
    if (!GetUpdateRegion().IsEmpty())
        dc.SetClippingRegion( GetUpdateRegion() );
#endif
*/

    if (m_printPreview)
    {
        m_printPreview->PaintPage(this, dc);
    }
}

// Responds to colour changes, and passes event on to children.
void wxPreviewCanvas::OnSysColourChanged(wxSysColourChangedEvent& event)
{
#ifdef __WXMAC__
    // The app workspace colour is always white, but we should have
    // a contrast with the page.
    wxSystemColour colourIndex = wxSYS_COLOUR_3DDKSHADOW;
#elif defined(__WXGTK__)
    wxSystemColour colourIndex = wxSYS_COLOUR_BTNFACE;
#else
    wxSystemColour colourIndex = wxSYS_COLOUR_APPWORKSPACE;
#endif
    SetBackgroundColour(wxSystemSettings::GetColour(colourIndex));
    Refresh();

    // Propagate the event to the non-top-level children
    wxWindow::OnSysColourChanged(event);
}

void wxPreviewCanvas::OnChar(wxKeyEvent &event)
{
    wxPreviewControlBar* controlBar = ((wxPreviewFrame*) GetParent())->GetControlBar();
    if (event.GetKeyCode() == WXK_ESCAPE)
    {
        ((wxPreviewFrame*) GetParent())->Close(true);
        return;
    }
    else if (event.GetKeyCode() == WXK_TAB)
    {
        controlBar->OnGoto();
        return;
    }
    else if (event.GetKeyCode() == WXK_RETURN)
    {
        controlBar->OnPrint();
        return;
    }

    if (!event.ControlDown())
    {
        event.Skip();
        return;
    }

    switch(event.GetKeyCode())
    {
        case WXK_PAGEDOWN:
            controlBar->OnNext(); break;
        case WXK_PAGEUP:
            controlBar->OnPrevious(); break;
        case WXK_HOME:
            controlBar->OnFirst(); break;
        case WXK_END:
            controlBar->OnLast(); break;
        default:
            event.Skip();
    }
}

#if wxUSE_MOUSEWHEEL

void wxPreviewCanvas::OnMouseWheel(wxMouseEvent& event)
{
    wxPreviewControlBar *
        controlBar = wxStaticCast(GetParent(), wxPreviewFrame)->GetControlBar();

    if ( controlBar )
    {
        if ( event.ControlDown() && event.GetWheelRotation() != 0 )
        {
            int currentZoom = controlBar->GetZoomControl();

            int delta;
            if ( currentZoom < 100 )
                delta = 5;
            else if ( currentZoom <= 120 )
                delta = 10;
            else
                delta = 50;

            if ( event.GetWheelRotation() > 0 )
                delta = -delta;

            int newZoom = currentZoom + delta;
            if ( newZoom < 10 )
                newZoom = 10;
            if ( newZoom > 200 )
                newZoom = 200;
            if ( newZoom != currentZoom )
            {
                controlBar->SetZoomControl(newZoom);
                m_printPreview->SetZoom(newZoom);
                Refresh();
            }
            return;
        }
    }

    event.Skip();
}

#endif // wxUSE_MOUSEWHEEL

//----------------------------------------------------------------------------
// wxPreviewControlBar
//----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxPreviewControlBar, wxWindow)

BEGIN_EVENT_TABLE(wxPreviewControlBar, wxPanel)
    EVT_BUTTON(wxID_PREVIEW_CLOSE,    wxPreviewControlBar::OnWindowClose)
    EVT_BUTTON(wxID_PREVIEW_PRINT,    wxPreviewControlBar::OnPrintButton)
    EVT_BUTTON(wxID_PREVIEW_PREVIOUS, wxPreviewControlBar::OnPreviousButton)
    EVT_BUTTON(wxID_PREVIEW_NEXT,     wxPreviewControlBar::OnNextButton)
    EVT_BUTTON(wxID_PREVIEW_FIRST,    wxPreviewControlBar::OnFirstButton)
    EVT_BUTTON(wxID_PREVIEW_LAST,     wxPreviewControlBar::OnLastButton)
    EVT_BUTTON(wxID_PREVIEW_GOTO,     wxPreviewControlBar::OnGotoButton)
    EVT_CHOICE(wxID_PREVIEW_ZOOM,     wxPreviewControlBar::OnZoom)
    EVT_PAINT(wxPreviewControlBar::OnPaint)
END_EVENT_TABLE()

wxPreviewControlBar::wxPreviewControlBar(wxPrintPreviewBase *preview, long buttons,
                                         wxWindow *parent, const wxPoint& pos, const wxSize& size,
                                         long style, const wxString& name):
wxPanel(parent, wxID_ANY, pos, size, style, name)
{
    m_printPreview = preview;
    m_closeButton = (wxButton *) NULL;
    m_nextPageButton = (wxButton *) NULL;
    m_previousPageButton = (wxButton *) NULL;
    m_printButton = (wxButton *) NULL;
    m_zoomControl = (wxChoice *) NULL;
    m_buttonFlags = buttons;
}

wxPreviewControlBar::~wxPreviewControlBar()
{
}

void wxPreviewControlBar::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    int w, h;
    GetSize(&w, &h);
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawLine( 0, h-1, w, h-1 );
}

void wxPreviewControlBar::OnWindowClose(wxCommandEvent& WXUNUSED(event))
{
    wxPreviewFrame *frame = (wxPreviewFrame *)GetParent();
    frame->Close(true);
}

void wxPreviewControlBar::OnPrint(void)
{
    wxPrintPreviewBase *preview = GetPrintPreview();
    preview->Print(true);
}

void wxPreviewControlBar::OnNext(void)
{
    wxPrintPreviewBase *preview = GetPrintPreview();
    if (preview)
    {
        int currentPage = preview->GetCurrentPage();
        if ((preview->GetMaxPage() > 0) &&
            (currentPage < preview->GetMaxPage()) &&
            preview->GetPrintout()->HasPage(currentPage + 1))
        {
            preview->SetCurrentPage(currentPage + 1);
        }
    }
}

void wxPreviewControlBar::OnPrevious(void)
{
    wxPrintPreviewBase *preview = GetPrintPreview();
    if (preview)
    {
        int currentPage = preview->GetCurrentPage();
        if ((preview->GetMinPage() > 0) &&
            (currentPage > preview->GetMinPage()) &&
            preview->GetPrintout()->HasPage(currentPage - 1))
        {
            preview->SetCurrentPage(currentPage - 1);
        }
    }
}

void wxPreviewControlBar::OnFirst(void)
{
    wxPrintPreviewBase *preview = GetPrintPreview();
    if (preview)
    {
        int currentPage = preview->GetMinPage();
        if (preview->GetPrintout()->HasPage(currentPage))
        {
            preview->SetCurrentPage(currentPage);
        }
    }
}

void wxPreviewControlBar::OnLast(void)
{
    wxPrintPreviewBase *preview = GetPrintPreview();
    if (preview)
    {
        int currentPage = preview->GetMaxPage();
        if (preview->GetPrintout()->HasPage(currentPage))
        {
            preview->SetCurrentPage(currentPage);
        }
    }
}

void wxPreviewControlBar::OnGoto(void)
{
    wxPrintPreviewBase *preview = GetPrintPreview();
    if (preview)
    {
        long currentPage;

        if (preview->GetMinPage() > 0)
        {
            wxString strPrompt;
            wxString strPage;

            strPrompt.Printf( _("Enter a page number between %d and %d:"),
                preview->GetMinPage(), preview->GetMaxPage());
            strPage.Printf( wxT("%d"), preview->GetCurrentPage() );

            strPage =
                wxGetTextFromUser( strPrompt, _("Goto Page"), strPage, GetParent());

            if ( strPage.ToLong( &currentPage ) )
                if (preview->GetPrintout()->HasPage(currentPage))
                {
                    preview->SetCurrentPage(currentPage);
                }
        }
    }
}

void wxPreviewControlBar::OnZoom(wxCommandEvent& WXUNUSED(event))
{
    int zoom = GetZoomControl();
    if (GetPrintPreview())
        GetPrintPreview()->SetZoom(zoom);
}

void wxPreviewControlBar::CreateButtons()
{
    SetSize(0, 0, 400, 40);

    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    m_closeButton = new wxButton( this, wxID_PREVIEW_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( m_closeButton, 0, wxALIGN_CENTRE|wxALL, 5 );

    if (m_buttonFlags & wxPREVIEW_PRINT)
    {
        m_printButton = new wxButton( this, wxID_PREVIEW_PRINT, _("&Print..."), wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( m_printButton, 0, wxALIGN_CENTRE|wxALL, 5 );
    }

    // Exact-fit buttons are too tiny on wxUniversal
    int navButtonStyle;
    wxSize navButtonSize;
#ifdef __WXUNIVERSAL__
    navButtonStyle = 0;
    navButtonSize = wxSize(40, m_closeButton->GetSize().y);
#else
    navButtonStyle = wxBU_EXACTFIT;
    navButtonSize = wxDefaultSize;
#endif

    if (m_buttonFlags & wxPREVIEW_FIRST)
    {
        m_firstPageButton = new wxButton( this, wxID_PREVIEW_FIRST, _("|<<"), wxDefaultPosition, navButtonSize, navButtonStyle );
        item0->Add( m_firstPageButton, 0, wxALIGN_CENTRE|wxALL, 5 );
    }

    if (m_buttonFlags & wxPREVIEW_PREVIOUS)
    {
        m_previousPageButton = new wxButton( this, wxID_PREVIEW_PREVIOUS, _("<<"), wxDefaultPosition, navButtonSize, navButtonStyle );
        item0->Add( m_previousPageButton, 0, wxALIGN_CENTRE|wxRIGHT|wxTOP|wxBOTTOM, 5 );
    }

    if (m_buttonFlags & wxPREVIEW_NEXT)
    {
        m_nextPageButton = new wxButton( this, wxID_PREVIEW_NEXT, _(">>"), wxDefaultPosition, navButtonSize, navButtonStyle );
        item0->Add( m_nextPageButton, 0, wxALIGN_CENTRE|wxRIGHT|wxTOP|wxBOTTOM, 5 );
    }

    if (m_buttonFlags & wxPREVIEW_LAST)
    {
        m_lastPageButton = new wxButton( this, wxID_PREVIEW_LAST, _(">>|"), wxDefaultPosition, navButtonSize, navButtonStyle );
        item0->Add( m_lastPageButton, 0, wxALIGN_CENTRE|wxRIGHT|wxTOP|wxBOTTOM, 5 );
    }

    if (m_buttonFlags & wxPREVIEW_GOTO)
    {
        m_gotoPageButton = new wxButton( this, wxID_PREVIEW_GOTO, _("&Goto..."), wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( m_gotoPageButton, 0, wxALIGN_CENTRE|wxALL, 5 );
    }

    if (m_buttonFlags & wxPREVIEW_ZOOM)
    {
        wxString choices[] =
        {
            wxT("10%"), wxT("15%"), wxT("20%"), wxT("25%"), wxT("30%"), wxT("35%"), wxT("40%"), wxT("45%"), wxT("50%"), wxT("55%"),
                wxT("60%"), wxT("65%"), wxT("70%"), wxT("75%"), wxT("80%"), wxT("85%"), wxT("90%"), wxT("95%"), wxT("100%"), wxT("110%"),
                wxT("120%"), wxT("150%"), wxT("200%")
        };
        int n = WXSIZEOF(choices);

        m_zoomControl = new wxChoice( this, wxID_PREVIEW_ZOOM, wxDefaultPosition, wxSize(70,wxDefaultCoord), n, choices, 0 );
        item0->Add( m_zoomControl, 0, wxALIGN_CENTRE|wxALL, 5 );
        SetZoomControl(m_printPreview->GetZoom());
    }

    SetSizer(item0);
    item0->Fit(this);
}

void wxPreviewControlBar::SetZoomControl(int zoom)
{
    if (m_zoomControl)
    {
        int n, count = m_zoomControl->GetCount();
        long val;
        for (n=0; n<count; n++)
        {
            if (m_zoomControl->GetString(n).BeforeFirst(wxT('%')).ToLong(&val) &&
                (val >= long(zoom)))
            {
                m_zoomControl->SetSelection(n);
                return;
            }
        }

        m_zoomControl->SetSelection(count-1);
    }
}

int wxPreviewControlBar::GetZoomControl()
{
    if (m_zoomControl && (m_zoomControl->GetStringSelection() != wxEmptyString))
    {
        long val;
        if (m_zoomControl->GetStringSelection().BeforeFirst(wxT('%')).ToLong(&val))
            return int(val);
    }

    return 0;
}


/*
* Preview frame
*/

IMPLEMENT_CLASS(wxPreviewFrame, wxFrame)

BEGIN_EVENT_TABLE(wxPreviewFrame, wxFrame)
    EVT_CLOSE(wxPreviewFrame::OnCloseWindow)
END_EVENT_TABLE()

wxPreviewFrame::wxPreviewFrame(wxPrintPreviewBase *preview, wxWindow *parent, const wxString& title,
                               const wxPoint& pos, const wxSize& size, long style, const wxString& name):
wxFrame(parent, wxID_ANY, title, pos, size, style, name)
{
    m_printPreview = preview;
    m_controlBar = NULL;
    m_previewCanvas = NULL;
    m_windowDisabler = NULL;

    // Give the application icon
#ifdef __WXMSW__
    wxFrame* topFrame = wxDynamicCast(wxTheApp->GetTopWindow(), wxFrame);
    if (topFrame)
        SetIcon(topFrame->GetIcon());
#endif
}

wxPreviewFrame::~wxPreviewFrame()
{
}

void wxPreviewFrame::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
    if (m_windowDisabler)
        delete m_windowDisabler;

    // Need to delete the printout and the print preview
    wxPrintout *printout = m_printPreview->GetPrintout();
    if (printout)
    {
        delete printout;
        m_printPreview->SetPrintout(NULL);
        m_printPreview->SetCanvas(NULL);
        m_printPreview->SetFrame(NULL);
    }
    delete m_printPreview;

    Destroy();
}

void wxPreviewFrame::Initialize()
{
#if wxUSE_STATUSBAR
    CreateStatusBar();
#endif
    CreateCanvas();
    CreateControlBar();

    m_printPreview->SetCanvas(m_previewCanvas);
    m_printPreview->SetFrame(this);

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    item0->Add( m_controlBar, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );
    item0->Add( m_previewCanvas, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    SetAutoLayout( true );
    SetSizer( item0 );

    m_windowDisabler = new wxWindowDisabler(this);

    Layout();

    m_printPreview->AdjustScrollbars(m_previewCanvas);
    m_previewCanvas->SetFocus();
    m_controlBar->SetFocus();
}

void wxPreviewFrame::CreateCanvas()
{
    m_previewCanvas = new wxPreviewCanvas(m_printPreview, this);
}

void wxPreviewFrame::CreateControlBar()
{
    long buttons = wxPREVIEW_DEFAULT;
    if (m_printPreview->GetPrintoutForPrinting())
        buttons |= wxPREVIEW_PRINT;

    m_controlBar = new wxPreviewControlBar(m_printPreview, buttons, this, wxPoint(0,0), wxSize(400, 40));
    m_controlBar->CreateButtons();
}

/*
* Print preview
*/

IMPLEMENT_CLASS(wxPrintPreviewBase, wxObject)

wxPrintPreviewBase::wxPrintPreviewBase(wxPrintout *printout,
                                       wxPrintout *printoutForPrinting,
                                       wxPrintData *data)
{
    if (data)
        m_printDialogData = (*data);

    Init(printout, printoutForPrinting);
}

wxPrintPreviewBase::wxPrintPreviewBase(wxPrintout *printout,
                                       wxPrintout *printoutForPrinting,
                                       wxPrintDialogData *data)
{
    if (data)
        m_printDialogData = (*data);

    Init(printout, printoutForPrinting);
}

void wxPrintPreviewBase::Init(wxPrintout *printout,
                              wxPrintout *printoutForPrinting)
{
    m_isOk = true;
    m_previewPrintout = printout;
    if (m_previewPrintout)
        m_previewPrintout->SetIsPreview(true);

    m_printPrintout = printoutForPrinting;

    m_previewCanvas = NULL;
    m_previewFrame = NULL;
    m_previewBitmap = NULL;
    m_currentPage = 1;
    m_currentZoom = 70;
    m_topMargin = 40;
    m_leftMargin = 40;
    m_pageWidth = 0;
    m_pageHeight = 0;
    m_printingPrepared = false;
    m_minPage = 1;
    m_maxPage = 1;
}

wxPrintPreviewBase::~wxPrintPreviewBase()
{
    if (m_previewPrintout)
        delete m_previewPrintout;
    if (m_previewBitmap)
        delete m_previewBitmap;
    if (m_printPrintout)
        delete m_printPrintout;
}

bool wxPrintPreviewBase::SetCurrentPage(int pageNum)
{
    if (m_currentPage == pageNum)
        return true;

    m_currentPage = pageNum;
    if (m_previewBitmap)
    {
        delete m_previewBitmap;
        m_previewBitmap = NULL;
    }

    if (m_previewCanvas)
    {
        AdjustScrollbars(m_previewCanvas);

        if (!RenderPage(pageNum))
            return false;
        m_previewCanvas->Refresh();
        m_previewCanvas->SetFocus();
    }
    return true;
}

int wxPrintPreviewBase::GetCurrentPage() const
    { return m_currentPage; }
void wxPrintPreviewBase::SetPrintout(wxPrintout *printout)
    { m_previewPrintout = printout; }
wxPrintout *wxPrintPreviewBase::GetPrintout() const
    { return m_previewPrintout; }
wxPrintout *wxPrintPreviewBase::GetPrintoutForPrinting() const
    { return m_printPrintout; }
void wxPrintPreviewBase::SetFrame(wxFrame *frame)
    { m_previewFrame = frame; }
void wxPrintPreviewBase::SetCanvas(wxPreviewCanvas *canvas)
    { m_previewCanvas = canvas; }
wxFrame *wxPrintPreviewBase::GetFrame() const
    { return m_previewFrame; }
wxPreviewCanvas *wxPrintPreviewBase::GetCanvas() const
    { return m_previewCanvas; }

void wxPrintPreviewBase::CalcRects(wxPreviewCanvas *canvas, wxRect& pageRect, wxRect& paperRect)
{
    // Calculate the rectangles for the printable area of the page and the
    // entire paper as they appear on the canvas on-screen.
    int canvasWidth, canvasHeight;
    canvas->GetSize(&canvasWidth, &canvasHeight);

    float zoomScale = float(m_currentZoom) / 100;
    float screenPrintableWidth = zoomScale * m_pageWidth * m_previewScaleX;
    float screenPrintableHeight = zoomScale * m_pageHeight * m_previewScaleY;

    wxRect devicePaperRect = m_previewPrintout->GetPaperRectPixels();
    wxCoord devicePrintableWidth, devicePrintableHeight;
    m_previewPrintout->GetPageSizePixels(&devicePrintableWidth, &devicePrintableHeight);
    float scaleX = screenPrintableWidth / devicePrintableWidth;
    float scaleY = screenPrintableHeight / devicePrintableHeight;
    paperRect.width = wxCoord(scaleX * devicePaperRect.width);
    paperRect.height = wxCoord(scaleY * devicePaperRect.height);

    paperRect.x = wxCoord((canvasWidth - paperRect.width)/ 2.0);
    if (paperRect.x < m_leftMargin)
        paperRect.x = m_leftMargin;
    paperRect.y = wxCoord((canvasHeight - paperRect.height)/ 2.0);
    if (paperRect.y < m_topMargin)
        paperRect.y = m_topMargin;

    pageRect.x = paperRect.x - wxCoord(scaleX * devicePaperRect.x);
    pageRect.y = paperRect.y - wxCoord(scaleY * devicePaperRect.y);
    pageRect.width = wxCoord(screenPrintableWidth);
    pageRect.height = wxCoord(screenPrintableHeight);
}


bool wxPrintPreviewBase::PaintPage(wxPreviewCanvas *canvas, wxDC& dc)
{
    DrawBlankPage(canvas, dc);

    if (!m_previewBitmap)
        if (!RenderPage(m_currentPage))
            return false;
    if (!m_previewBitmap)
        return false;
    if (!canvas)
        return false;

    wxRect pageRect, paperRect;
    CalcRects(canvas, pageRect, paperRect);
    wxMemoryDC temp_dc;
    temp_dc.SelectObject(*m_previewBitmap);

    dc.Blit(pageRect.x, pageRect.y,
        m_previewBitmap->GetWidth(), m_previewBitmap->GetHeight(), &temp_dc, 0, 0);

    temp_dc.SelectObject(wxNullBitmap);
    return true;
}

// Adjusts the scrollbars for the current scale
void wxPrintPreviewBase::AdjustScrollbars(wxPreviewCanvas *canvas)
{
    if (!canvas)
        return ;

    wxRect pageRect, paperRect;
    CalcRects(canvas, pageRect, paperRect);
     int totalWidth = paperRect.width + 2 * m_leftMargin;
    int totalHeight = paperRect.height + 2 * m_topMargin;
    int scrollUnitsX = totalWidth / 10;
    int scrollUnitsY = totalHeight / 10;
    wxSize virtualSize = canvas->GetVirtualSize();
    if (virtualSize.GetWidth() != totalWidth || virtualSize.GetHeight() != totalHeight)
        canvas->SetScrollbars(10, 10, scrollUnitsX, scrollUnitsY, 0, 0, true);
}

bool wxPrintPreviewBase::RenderPageIntoDC(wxDC& dc, int pageNum)
{
    m_previewPrintout->SetDC(&dc);
    m_previewPrintout->SetPageSizePixels(m_pageWidth, m_pageHeight);

    // Need to delay OnPreparePrinting() until here, so we have enough
    // information.
    if (!m_printingPrepared)
    {
        m_previewPrintout->OnPreparePrinting();
        int selFrom, selTo;
        m_previewPrintout->GetPageInfo(&m_minPage, &m_maxPage, &selFrom, &selTo);
        m_printingPrepared = true;
    }

    m_previewPrintout->OnBeginPrinting();

    if (!m_previewPrintout->OnBeginDocument(m_printDialogData.GetFromPage(), m_printDialogData.GetToPage()))
    {
        wxMessageBox(_("Could not start document preview."), _("Print Preview Failure"), wxOK);
        return false;
    }

    m_previewPrintout->OnPrintPage(pageNum);
    m_previewPrintout->OnEndDocument();
    m_previewPrintout->OnEndPrinting();

    m_previewPrintout->SetDC(NULL);

    return true;
}

bool wxPrintPreviewBase::RenderPageIntoBitmap(wxBitmap& bmp, int pageNum)
{
#if wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW
    // try high quality rendering first:
    static bool s_hqPreviewFailed = false;
    if ( !s_hqPreviewFailed )
    {
        wxPrinterDC printerDC(m_printDialogData.GetPrintData());
        if ( RenderPageIntoBitmapHQ(this,
                                    &wxPrintPreviewBase::RenderPageIntoDC,
                                    printerDC,
                                    bmp, pageNum,
                                    m_pageWidth, m_pageHeight) )
        {
            return true;
        }
        else
        {
            wxLogTrace(_T("printing"),
                       _T("high-quality preview failed, falling back to normal"));
            s_hqPreviewFailed = true; // don't bother re-trying
        }
    }
#endif // wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW

    wxMemoryDC memoryDC;
    memoryDC.SelectObject(bmp);
    memoryDC.Clear();

    return RenderPageIntoDC(memoryDC, pageNum);
}

bool wxPrintPreviewBase::RenderPage(int pageNum)
{
    wxBusyCursor busy;

    if (!m_previewCanvas)
    {
        wxFAIL_MSG(_T("wxPrintPreviewBase::RenderPage: must use wxPrintPreviewBase::SetCanvas to let me know about the canvas!"));
        return false;
    }

    wxRect pageRect, paperRect;
    CalcRects(m_previewCanvas, pageRect, paperRect);

    if (!m_previewBitmap)
    {
        m_previewBitmap = new wxBitmap(pageRect.width, pageRect.height);

        if (!m_previewBitmap || !m_previewBitmap->Ok())
        {
            if (m_previewBitmap) {
                delete m_previewBitmap;
                m_previewBitmap = NULL;
            }
            wxMessageBox(_("Sorry, not enough memory to create a preview."), _("Print Preview Failure"), wxOK);
            return false;
        }
    }

    if ( !RenderPageIntoBitmap(*m_previewBitmap, pageNum) )
    {
        wxMessageBox(_("Could not start document preview."), _("Print Preview Failure"), wxOK);

        delete m_previewBitmap;
        m_previewBitmap = NULL;
        return false;
    }

#if wxUSE_STATUSBAR
    wxString status;
    if (m_maxPage != 0)
        status = wxString::Format(_("Page %d of %d"), pageNum, m_maxPage);
    else
        status = wxString::Format(_("Page %d"), pageNum);

    if (m_previewFrame)
        m_previewFrame->SetStatusText(status);
#endif

    return true;
}

bool wxPrintPreviewBase::DrawBlankPage(wxPreviewCanvas *canvas, wxDC& dc)
{
    wxRect pageRect, paperRect;

    CalcRects(canvas, pageRect, paperRect);

    // Draw shadow, allowing for 1-pixel border AROUND the actual paper
    wxCoord shadowOffset = 4;

    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxBLACK_BRUSH);
    dc.DrawRectangle(paperRect.x + shadowOffset, paperRect.y + paperRect.height + 1,
        paperRect.width, shadowOffset);

    dc.DrawRectangle(paperRect.x + paperRect.width, paperRect.y + shadowOffset,
        shadowOffset, paperRect.height);

    // Draw blank page allowing for 1-pixel border AROUND the actual paper
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(paperRect.x - 2, paperRect.y - 1,
        paperRect.width + 3, paperRect.height + 2);

    return true;
}

void wxPrintPreviewBase::SetZoom(int percent)
{
    if (m_currentZoom == percent)
        return;

    m_currentZoom = percent;
    if (m_previewBitmap)
    {
        delete m_previewBitmap;
        m_previewBitmap = NULL;
    }

    if (m_previewCanvas)
    {
        AdjustScrollbars(m_previewCanvas);
        RenderPage(m_currentPage);
        ((wxScrolledWindow *) m_previewCanvas)->Scroll(0, 0);
        m_previewCanvas->ClearBackground();
        m_previewCanvas->Refresh();
        m_previewCanvas->SetFocus();
    }
}

wxPrintDialogData& wxPrintPreviewBase::GetPrintDialogData()
{
    return m_printDialogData;
}

int wxPrintPreviewBase::GetZoom() const
{ return m_currentZoom; }
int wxPrintPreviewBase::GetMaxPage() const
{ return m_maxPage; }
int wxPrintPreviewBase::GetMinPage() const
{ return m_minPage; }
bool wxPrintPreviewBase::IsOk() const
{ return m_isOk; }
void wxPrintPreviewBase::SetOk(bool ok)
{ m_isOk = ok; }

//----------------------------------------------------------------------------
// wxPrintPreview
//----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxPrintPreview, wxPrintPreviewBase)

wxPrintPreview::wxPrintPreview(wxPrintout *printout,
                   wxPrintout *printoutForPrinting,
                   wxPrintDialogData *data) :
    wxPrintPreviewBase( printout, printoutForPrinting, data )
{
    m_pimpl = wxPrintFactory::GetFactory()->
        CreatePrintPreview( printout, printoutForPrinting, data );
}

wxPrintPreview::wxPrintPreview(wxPrintout *printout,
                   wxPrintout *printoutForPrinting,
                   wxPrintData *data ) :
    wxPrintPreviewBase( printout, printoutForPrinting, data )
{
    m_pimpl = wxPrintFactory::GetFactory()->
        CreatePrintPreview( printout, printoutForPrinting, data );
}

wxPrintPreview::~wxPrintPreview()
{
    delete m_pimpl;

    // don't delete twice
    m_printPrintout = NULL;
    m_previewPrintout = NULL;
    m_previewBitmap = NULL;
}

bool wxPrintPreview::SetCurrentPage(int pageNum)
{
    return m_pimpl->SetCurrentPage( pageNum );
}

int wxPrintPreview::GetCurrentPage() const
{
    return m_pimpl->GetCurrentPage();
}

void wxPrintPreview::SetPrintout(wxPrintout *printout)
{
    m_pimpl->SetPrintout( printout );
}

wxPrintout *wxPrintPreview::GetPrintout() const
{
    return m_pimpl->GetPrintout();
}

wxPrintout *wxPrintPreview::GetPrintoutForPrinting() const
{
    return m_pimpl->GetPrintoutForPrinting();
}

void wxPrintPreview::SetFrame(wxFrame *frame)
{
    m_pimpl->SetFrame( frame );
}

void wxPrintPreview::SetCanvas(wxPreviewCanvas *canvas)
{
    m_pimpl->SetCanvas( canvas );
}

wxFrame *wxPrintPreview::GetFrame() const
{
    return m_pimpl->GetFrame();
}

wxPreviewCanvas *wxPrintPreview::GetCanvas() const
{
    return m_pimpl->GetCanvas();
}

bool wxPrintPreview::PaintPage(wxPreviewCanvas *canvas, wxDC& dc)
{
    return m_pimpl->PaintPage( canvas, dc );
}

bool wxPrintPreview::DrawBlankPage(wxPreviewCanvas *canvas, wxDC& dc)
{
    return m_pimpl->DrawBlankPage( canvas, dc );
}

void wxPrintPreview::AdjustScrollbars(wxPreviewCanvas *canvas)
{
    m_pimpl->AdjustScrollbars( canvas );
}

bool wxPrintPreview::RenderPage(int pageNum)
{
    return m_pimpl->RenderPage( pageNum );
}

void wxPrintPreview::SetZoom(int percent)
{
    m_pimpl->SetZoom( percent );
}

int wxPrintPreview::GetZoom() const
{
    return m_pimpl->GetZoom();
}

wxPrintDialogData& wxPrintPreview::GetPrintDialogData()
{
    return m_pimpl->GetPrintDialogData();
}

int wxPrintPreview::GetMaxPage() const
{
    return m_pimpl->GetMaxPage();
}

int wxPrintPreview::GetMinPage() const
{
    return m_pimpl->GetMinPage();
}

bool wxPrintPreview::IsOk() const
{
    return m_pimpl->Ok();
}

void wxPrintPreview::SetOk(bool ok)
{
    m_pimpl->SetOk( ok );
}

bool wxPrintPreview::Print(bool interactive)
{
    return m_pimpl->Print( interactive );
}

void wxPrintPreview::DetermineScaling()
{
    m_pimpl->DetermineScaling();
}

//----------------------------------------------------------------------------
// experimental backport of high-quality preview on Windows
//----------------------------------------------------------------------------

#if wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW

// The preview, as implemented in wxPrintPreviewBase (and as used prior to wx3)
// is inexact: it uses screen DC, which has much lower resolution and has
// other properties different from printer DC, so the preview is not quite
// right.
//
// To make matters worse, if the application depends heavily on GetTextExtent()
// or does text layout itself, the output in preview and on paper can be very
// different. In particular, wxHtmlEasyPrinting is affected and the preview
// can be easily off by several pages.
//
// To fix this, we attempt to render the preview into high-resolution bitmap
// using DC with same resolution etc. as the printer DC. This takes lot of
// memory, so the code is more complicated than it could be, but the results
// are much better.
//
// Finally, this code is specific to wxMSW, because it doesn't make sense to
// bother with it on other platforms. Both OSX and modern GNOME/GTK+
// environments have builtin accurate preview (that applications should use
// instead) and the differences between screen and printer DC in wxGTK are so
// large than this trick doesn't help at all.

namespace
{

// If there's not enough memory, we need to render the preview in parts.
// Unfortunately we cannot simply use wxMemoryDC, because it reports its size
// as bitmap's size, and we need to use smaller bitmap while still reporting
// original ("correct") DC size, because printing code frequently uses
// GetSize() to determine scaling factor. This DC class handles this.

class PageFragmentDC : public wxMemoryDC
{
public:
    PageFragmentDC(wxDC *printer, wxBitmap& bmp,
                   const wxPoint& offset,
                   const wxSize& fullSize)
        : wxMemoryDC(printer),
          m_offset(offset),
          m_fullSize(fullSize)
    {
        SetDeviceOrigin(0, 0);
        SelectObject(bmp);
    }

    virtual void SetDeviceOrigin(wxCoord x, wxCoord y)
    {
        wxMemoryDC::SetDeviceOrigin(x - m_offset.x, y - m_offset.y);
    }

    virtual void DoGetDeviceOrigin(wxCoord *x, wxCoord *y) const
    {
        wxMemoryDC::DoGetDeviceOrigin(x, y);
        if ( x ) *x += m_offset.x;
        if ( x ) *y += m_offset.y;
    }

    virtual void DoGetSize(int *width, int *height) const
    {
        if ( width )
            *width = m_fullSize.x;
        if ( height )
            *height = m_fullSize.y;
    }

private:
    wxPoint m_offset;
    wxSize m_fullSize;
};

// estimate how big chunks we can render, given available RAM
long ComputeFragmentSize(long printerDepth,
                         long width,
                         long height)
{
    // Compute the amount of memory needed to generate the preview.
    // Memory requirements of RenderPageFragment() are as follows:
    //
    // (memory DC - always)
    //    width * height * printerDepth/8
    // (wxImage + wxDIB instance)
    //    width * height * (3 + 4)
    //    (this could be reduced to *3 if using wxGraphicsContext)
    //
    // So, given amount of memory M, we can render at most
    //
    //    height = M / (width * (printerDepth/8 + F))
    //
    // where F is 3 or 7 depending on whether wxGraphicsContext is used or not

    wxMemorySize memAvail = wxGetFreeMemory();
    if ( memAvail == -1 )
    {
        // we don't know;  10meg shouldn't be a problem hopefully
        memAvail = 10000000;
    }
    else
    {
        // limit ourselves to half of available RAM to have a margin for other
        // apps, for our rendering code, and for miscalculations
        memAvail /= 2;
    }

    const float perPixel = float(printerDepth)/8 + (3 + 4);

    const long perLine = long(width * perPixel);
    const long maxstep = (memAvail / perLine).GetValue();
    const long step = wxMin(height, maxstep);

    wxLogTrace(_T("printing"),
               _T("using %liMB of RAM (%li lines) for preview, %li %lipx fragments"),
               long((memAvail >> 20).GetValue()),
               maxstep,
               (height+step-1) / step,
               step);

    return step;
}

} // anonymous namespace


static bool RenderPageFragment(wxPrintPreviewBase *preview,
                               RenderPageIntoDCFunc RenderPageIntoDC,
                               float scaleX, float scaleY,
                               int *nextFinalLine,
                               wxPrinterDC& printer,
                               wxMemoryDC& finalDC,
                               const wxRect& rect,
                               int pageWidth, int pageHeight,
                               int pageNum)
{
    // compute 'rect' equivalent in the small final bitmap:
    const wxRect smallRect(wxPoint(0, *nextFinalLine),
                           wxPoint(int(rect.GetRight() * scaleX),
                                   int(rect.GetBottom() * scaleY)));
    wxLogTrace(_T("printing"),
               _T("rendering fragment of page %i: [%i,%i,%i,%i] scaled down to [%i,%i,%i,%i]"),
               pageNum,
               rect.x, rect.y, rect.GetRight(), rect.GetBottom(),
               smallRect.x, smallRect.y, smallRect.GetRight(), smallRect.GetBottom()
               );

    // create DC and bitmap compatible with printer DC:
    wxBitmap large(rect.width, rect.height, printer);
    if ( !large.IsOk() )
        return false;

    // render part of the page into it:
    {
        PageFragmentDC memoryDC(&printer, large,
                                rect.GetPosition(),
                                wxSize(pageWidth, pageHeight));
        if ( !memoryDC.IsOk() )
            return false;

        memoryDC.Clear();

        if ( !(preview->*RenderPageIntoDC)(memoryDC, pageNum) )
            return false;
    } // release bitmap from memoryDC

    // now scale the rendered part down and blit it into final output:

    wxImage img;
    {
        wxDIB dib(large);
        if ( !dib.IsOk() )
            return false;
        large = wxNullBitmap; // free memory a.s.a.p.
        img = dib.ConvertToImage();
    } // free the DIB now that it's no longer needed, too

    if ( !img.IsOk() )
        return false;

    img.Rescale(smallRect.width, smallRect.height, wxIMAGE_QUALITY_HIGH);
    if ( !img.IsOk() )
        return false;

    wxBitmap bmp(img);
    if ( !bmp.IsOk() )
        return false;

    img = wxNullImage;
    finalDC.DrawBitmap(bmp, smallRect.x, smallRect.y);
    if ( bmp.IsOk() )
    {
        *nextFinalLine += smallRect.height;
        return true;
    }

    return false;
}

static bool RenderPageIntoBitmapHQ(wxPrintPreviewBase *preview,
                                   RenderPageIntoDCFunc RenderPageIntoDC,
                                   wxPrinterDC& printerDC,
                                   wxBitmap& bmp, int pageNum,
                                   int pageWidth, int pageHeight)
{
    wxLogTrace(_T("printing"), _T("rendering HQ preview of page %i"), pageNum);

    if ( !printerDC.IsOk() )
        return false;

    // compute scale factor
    const float scaleX = float(bmp.GetWidth()) / float(pageWidth);
    const float scaleY =  float(bmp.GetHeight()) / float(pageHeight);

    wxMemoryDC bmpDC;
    bmpDC.SelectObject(bmp);
    bmpDC.Clear();

    const int initialStep = ComputeFragmentSize(printerDC.GetDepth(),
                                                pageWidth, pageHeight);

    wxRect todo(0, 0, pageWidth, initialStep); // rect to render
    int nextFinalLine = 0; // first not-yet-rendered output line

    while ( todo.y < pageHeight )
    {
        todo.SetBottom(wxMin(todo.GetBottom(), pageHeight - 1));

        if ( !RenderPageFragment(preview, RenderPageIntoDC,
                                 scaleX, scaleY,
                                 &nextFinalLine,
                                 printerDC,
                                 bmpDC,
                                 todo,
                                 pageWidth, pageHeight,
                                 pageNum) )
        {
            if ( todo.height < 20 )
            {
                // something is very wrong if we can't render even at this
                // slow space, let's bail out and fall back to low quality
                // preview
                wxLogTrace(_T("printing"),
                           _T("it seems that HQ preview doesn't work at all"));
                return false;
            }

            // it's possible our memory calculation was off, or conditions
            // changed, or there's not enough _bitmap_ resources; try if using
            // smaller bitmap would help:
            todo.height /= 2;

            wxLogTrace(_T("printing"),
                       _T("preview of fragment failed, reducing height to %ipx"),
                       todo.height);

            continue; // retry at the same position again
        }

        // move to the next segment
        todo.Offset(0, todo.height);
    }

    return true;
}

#endif // wxUSE_HIGH_QUALITY_PREVIEW_IN_WXMSW

#endif // wxUSE_PRINTING_ARCHITECTURE
