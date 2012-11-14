/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/printps.cpp
// Purpose:     Postscript print/preview framework
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: printps.cpp 42522 2006-10-27 13:07:40Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#if wxUSE_PRINTING_ARCHITECTURE && wxUSE_POSTSCRIPT && (!defined(__WXMSW__) || wxUSE_POSTSCRIPT_ARCHITECTURE_IN_MSW)

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/dc.h"
    #include "wx/app.h"
    #include "wx/msgdlg.h"
    #include "wx/intl.h"
    #include "wx/progdlg.h"
    #include "wx/log.h"
    #include "wx/dcprint.h"
#endif

#include "wx/generic/printps.h"
#include "wx/printdlg.h"
#include "wx/generic/prntdlgg.h"
#include "wx/generic/progdlgg.h"
#include "wx/paper.h"

#include <stdlib.h>

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

    IMPLEMENT_DYNAMIC_CLASS(wxPostScriptPrinter, wxPrinterBase)
    IMPLEMENT_CLASS(wxPostScriptPrintPreview, wxPrintPreviewBase)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Printer
// ----------------------------------------------------------------------------

wxPostScriptPrinter::wxPostScriptPrinter(wxPrintDialogData *data)
                   : wxPrinterBase(data)
{
}

wxPostScriptPrinter::~wxPostScriptPrinter()
{
}

bool wxPostScriptPrinter::Print(wxWindow *parent, wxPrintout *printout, bool prompt)
{
    sm_abortIt = false;
    sm_abortWindow = (wxWindow *) NULL;

    if (!printout)
    {
        sm_lastError = wxPRINTER_ERROR;
        return false;
    }

    printout->SetIsPreview(false);

    if (m_printDialogData.GetMinPage() < 1)
        m_printDialogData.SetMinPage(1);
    if (m_printDialogData.GetMaxPage() < 1)
        m_printDialogData.SetMaxPage(9999);

    // Create a suitable device context
    wxDC *dc;
    if (prompt)
    {
        dc = PrintDialog(parent);
        if (!dc)
            return false;
    }
    else
    {
        dc = new wxPostScriptDC(GetPrintDialogData().GetPrintData());
    }

    // May have pressed cancel.
    if (!dc || !dc->Ok())
    {
        if (dc) delete dc;
        sm_lastError = wxPRINTER_ERROR;
        return false;
    }

    wxSize ScreenPixels = wxGetDisplaySize();
    wxSize ScreenMM = wxGetDisplaySizeMM();

    printout->SetPPIScreen( (int) ((ScreenPixels.GetWidth() * 25.4) / ScreenMM.GetWidth()),
                            (int) ((ScreenPixels.GetHeight() * 25.4) / ScreenMM.GetHeight()) );
    printout->SetPPIPrinter( wxPostScriptDC::GetResolution(),
                             wxPostScriptDC::GetResolution() );

    // Set printout parameters
    printout->SetDC(dc);

    int w, h;
    dc->GetSize(&w, &h);
    printout->SetPageSizePixels((int)w, (int)h);
    printout->SetPaperRectPixels(wxRect(0, 0, w, h));
    int mw, mh;
    dc->GetSizeMM(&mw, &mh);
    printout->SetPageSizeMM((int)mw, (int)mh);

    // Create an abort window
    wxBeginBusyCursor();

    printout->OnPreparePrinting();

    // Get some parameters from the printout, if defined
    int fromPage, toPage;
    int minPage, maxPage;
    printout->GetPageInfo(&minPage, &maxPage, &fromPage, &toPage);

    if (maxPage == 0)
    {
        sm_lastError = wxPRINTER_ERROR;
        wxEndBusyCursor();
        return false;
    }

    // Only set min and max, because from and to have been
    // set by the user
    m_printDialogData.SetMinPage(minPage);
    m_printDialogData.SetMaxPage(maxPage);

    if (m_printDialogData.GetFromPage() < minPage)
        m_printDialogData.SetFromPage( minPage );
    if (m_printDialogData.GetToPage() > maxPage)
        m_printDialogData.SetToPage( maxPage );

    int
       pagesPerCopy = m_printDialogData.GetToPage()-m_printDialogData.GetFromPage()+1,
       totalPages = pagesPerCopy * m_printDialogData.GetNoCopies(),
       printedPages = 0;
    // Open the progress bar dialog
    wxProgressDialog *progressDialog = new wxProgressDialog (
       printout->GetTitle(),
       _("Printing..."),
       totalPages,
       parent,
       wxPD_CAN_ABORT|wxPD_AUTO_HIDE|wxPD_APP_MODAL);

    printout->OnBeginPrinting();

    sm_lastError = wxPRINTER_NO_ERROR;

    bool keepGoing = true;

    int copyCount;
    for (copyCount = 1; copyCount <= m_printDialogData.GetNoCopies(); copyCount ++)
    {
        if (!printout->OnBeginDocument(m_printDialogData.GetFromPage(), m_printDialogData.GetToPage()))
        {
            wxEndBusyCursor();
            wxLogError(_("Could not start printing."));
            sm_lastError = wxPRINTER_ERROR;
            break;
        }
        if (sm_abortIt)
        {
            sm_lastError = wxPRINTER_CANCELLED;
            break;
        }

        int pn;
        for (pn = m_printDialogData.GetFromPage(); keepGoing && (pn <= m_printDialogData.GetToPage()) && printout->HasPage(pn);
        pn++)
        {
            if (sm_abortIt)
            {
                keepGoing = false;
                sm_lastError = wxPRINTER_CANCELLED;
                break;
            }
            else
            {
               wxString msg;
               msg.Printf(_("Printing page %d..."), printedPages+1);
               if(progressDialog->Update(printedPages++, msg))
               {
                  dc->StartPage();
                  printout->OnPrintPage(pn);
                  dc->EndPage();
               }
               else
               {
                  sm_abortIt = true;
                  sm_lastError = wxPRINTER_CANCELLED;
                  keepGoing = false;
               }
            }
            wxYield();
        }
        printout->OnEndDocument();
    }

    printout->OnEndPrinting();
    delete progressDialog;

    wxEndBusyCursor();

    delete dc;

    return (sm_lastError == wxPRINTER_NO_ERROR);
}

wxDC* wxPostScriptPrinter::PrintDialog(wxWindow *parent)
{
    wxDC* dc = (wxDC*) NULL;

    wxGenericPrintDialog dialog( parent, &m_printDialogData );
    if (dialog.ShowModal() == wxID_OK)
    {
        dc = dialog.GetPrintDC();
        m_printDialogData = dialog.GetPrintDialogData();

        if (dc == NULL)
            sm_lastError = wxPRINTER_ERROR;
        else
            sm_lastError = wxPRINTER_NO_ERROR;
    }
    else
        sm_lastError = wxPRINTER_CANCELLED;

    return dc;
}

bool wxPostScriptPrinter::Setup(wxWindow *WXUNUSED(parent))
{
#if 0
    wxGenericPrintDialog* dialog = new wxGenericPrintDialog(parent, & m_printDialogData);
    dialog->GetPrintDialogData().SetSetupDialog(true);

    int ret = dialog->ShowModal();

    if (ret == wxID_OK)
    {
        m_printDialogData = dialog->GetPrintDialogData();
    }

    dialog->Destroy();

    return (ret == wxID_OK);
#endif

    return false;
}

// ----------------------------------------------------------------------------
// Print preview
// ----------------------------------------------------------------------------

void wxPostScriptPrintPreview::Init(wxPrintout * WXUNUSED(printout),
                                    wxPrintout * WXUNUSED(printoutForPrinting))
{
    // Have to call it here since base constructor can't call it
    DetermineScaling();
}

wxPostScriptPrintPreview::wxPostScriptPrintPreview(wxPrintout *printout,
                                                   wxPrintout *printoutForPrinting,
                                                   wxPrintDialogData *data)
                        : wxPrintPreviewBase(printout, printoutForPrinting, data)
{
    Init(printout, printoutForPrinting);
}

wxPostScriptPrintPreview::wxPostScriptPrintPreview(wxPrintout *printout,
                                                   wxPrintout *printoutForPrinting,
                                                   wxPrintData *data)
                        : wxPrintPreviewBase(printout, printoutForPrinting, data)
{
    Init(printout, printoutForPrinting);
}

wxPostScriptPrintPreview::~wxPostScriptPrintPreview()
{
}

bool wxPostScriptPrintPreview::Print(bool interactive)
{
    if (!m_printPrintout)
        return false;

    // Assume that on Unix, the preview may use the PostScript
    // (generic) version, but printing using the native system is required.
    // TODO: make a generic print preview class from which wxPostScriptPrintPreview
    // is derived.
#ifdef __UNIX__
    wxPrinter printer(& m_printDialogData);
#else
    wxPostScriptPrinter printer(& m_printDialogData);
#endif
    return printer.Print(m_previewFrame, m_printPrintout, interactive);
}

void wxPostScriptPrintPreview::DetermineScaling()
{
    wxPaperSize paperType = m_printDialogData.GetPrintData().GetPaperId();
    if (paperType == wxPAPER_NONE)
        paperType = wxPAPER_NONE;

    wxPrintPaperType *paper = wxThePrintPaperDatabase->FindPaperType(paperType);
    if (!paper)
        paper = wxThePrintPaperDatabase->FindPaperType(wxPAPER_A4);

    if (paper)
    {
        wxSize ScreenPixels = wxGetDisplaySize();
        wxSize ScreenMM = wxGetDisplaySizeMM();

        m_previewPrintout->SetPPIScreen( (int) ((ScreenPixels.GetWidth() * 25.4) / ScreenMM.GetWidth()),
                                         (int) ((ScreenPixels.GetHeight() * 25.4) / ScreenMM.GetHeight()) );
        m_previewPrintout->SetPPIPrinter(wxPostScriptDC::GetResolution(), wxPostScriptDC::GetResolution());

        wxSize sizeDevUnits(paper->GetSizeDeviceUnits());
        sizeDevUnits.x = (wxCoord)((float)sizeDevUnits.x * wxPostScriptDC::GetResolution() / 72.0);
        sizeDevUnits.y = (wxCoord)((float)sizeDevUnits.y * wxPostScriptDC::GetResolution() / 72.0);
        wxSize sizeTenthsMM(paper->GetSize());
        wxSize sizeMM(sizeTenthsMM.x / 10, sizeTenthsMM.y / 10);

        // If in landscape mode, we need to swap the width and height.
        if ( m_printDialogData.GetPrintData().GetOrientation() == wxLANDSCAPE )
        {
            m_pageWidth = sizeDevUnits.y;
            m_pageHeight = sizeDevUnits.x;
            m_previewPrintout->SetPageSizeMM(sizeMM.y, sizeMM.x);
        }
        else
        {
            m_pageWidth = sizeDevUnits.x;
            m_pageHeight = sizeDevUnits.y;
            m_previewPrintout->SetPageSizeMM(sizeMM.x, sizeMM.y);
        }
        m_previewPrintout->SetPageSizePixels(m_pageWidth, m_pageHeight);
        m_previewPrintout->SetPaperRectPixels(wxRect(0, 0, m_pageWidth, m_pageHeight));

        // At 100%, the page should look about page-size on the screen.
        m_previewScaleX = (float)0.8 * 72.0 / (float)wxPostScriptDC::GetResolution();
        m_previewScaleY = m_previewScaleX;
    }
}

#endif
