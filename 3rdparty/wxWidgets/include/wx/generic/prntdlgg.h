/////////////////////////////////////////////////////////////////////////////
// Name:        prntdlgg.h
// Purpose:     wxGenericPrintDialog, wxGenericPrintSetupDialog,
//              wxGenericPageSetupDialog
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: prntdlgg.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __PRINTDLGH_G_
#define __PRINTDLGH_G_

#include "wx/defs.h"

#if wxUSE_PRINTING_ARCHITECTURE

#include "wx/dialog.h"
#include "wx/cmndata.h"
#include "wx/prntbase.h"
#include "wx/printdlg.h"
#include "wx/listctrl.h"

#if wxUSE_POSTSCRIPT
    #include "wx/dcps.h"
#endif

class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxCheckBox;
class WXDLLIMPEXP_FWD_CORE wxComboBox;
class WXDLLIMPEXP_FWD_CORE wxStaticText;
class WXDLLIMPEXP_FWD_CORE wxRadioBox;
class WXDLLIMPEXP_FWD_CORE wxPageSetupData;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// This is not clear why all these enums start with 10 or 30 but do not change it
// without good reason to avoid some subtle backwards compatibility breakage

enum
{
    wxPRINTID_STATIC = 10,
    wxPRINTID_RANGE,
    wxPRINTID_FROM,
    wxPRINTID_TO,
    wxPRINTID_COPIES,
    wxPRINTID_PRINTTOFILE,
    wxPRINTID_SETUP
};

enum
{
    wxPRINTID_LEFTMARGIN = 30,
    wxPRINTID_RIGHTMARGIN,
    wxPRINTID_TOPMARGIN,
    wxPRINTID_BOTTOMMARGIN
};

enum
{
    wxPRINTID_PRINTCOLOUR = 10,
    wxPRINTID_ORIENTATION,
    wxPRINTID_COMMAND,
    wxPRINTID_OPTIONS,
    wxPRINTID_PAPERSIZE,
    wxPRINTID_PRINTER
};

#if wxUSE_POSTSCRIPT

//----------------------------------------------------------------------------
// wxPostScriptNativeData
//----------------------------------------------------------------------------

class WXDLLEXPORT wxPostScriptPrintNativeData: public wxPrintNativeDataBase
{
public:
    wxPostScriptPrintNativeData();
    virtual ~wxPostScriptPrintNativeData();

    virtual bool TransferTo( wxPrintData &data );
    virtual bool TransferFrom( const wxPrintData &data );

    virtual bool Ok() const { return IsOk(); }
    virtual bool IsOk() const { return true; }

    const wxString& GetPrinterCommand() const { return m_printerCommand; }
    const wxString& GetPrinterOptions() const { return m_printerOptions; }
    const wxString& GetPreviewCommand() const { return m_previewCommand; }
    const wxString& GetFontMetricPath() const { return m_afmPath; }
    double GetPrinterScaleX() const { return m_printerScaleX; }
    double GetPrinterScaleY() const { return m_printerScaleY; }
    long GetPrinterTranslateX() const { return m_printerTranslateX; }
    long GetPrinterTranslateY() const { return m_printerTranslateY; }

    void SetPrinterCommand(const wxString& command) { m_printerCommand = command; }
    void SetPrinterOptions(const wxString& options) { m_printerOptions = options; }
    void SetPreviewCommand(const wxString& command) { m_previewCommand = command; }
    void SetFontMetricPath(const wxString& path) { m_afmPath = path; }
    void SetPrinterScaleX(double x) { m_printerScaleX = x; }
    void SetPrinterScaleY(double y) { m_printerScaleY = y; }
    void SetPrinterScaling(double x, double y) { m_printerScaleX = x; m_printerScaleY = y; }
    void SetPrinterTranslateX(long x) { m_printerTranslateX = x; }
    void SetPrinterTranslateY(long y) { m_printerTranslateY = y; }
    void SetPrinterTranslation(long x, long y) { m_printerTranslateX = x; m_printerTranslateY = y; }

#if wxUSE_STREAMS
    wxOutputStream *GetOutputStream() { return m_outputStream; }
    void SetOutputStream( wxOutputStream *output ) { m_outputStream = output; }
#endif

private:
    wxString        m_printerCommand;
    wxString        m_previewCommand;
    wxString        m_printerOptions;
    wxString        m_afmPath;
    double          m_printerScaleX;
    double          m_printerScaleY;
    long            m_printerTranslateX;
    long            m_printerTranslateY;
#if wxUSE_STREAMS
    wxOutputStream *m_outputStream;
#endif

private:
    DECLARE_DYNAMIC_CLASS(wxPostScriptPrintNativeData)
};

// ----------------------------------------------------------------------------
// Simulated Print and Print Setup dialogs for non-Windows platforms (and
// Windows using PostScript print/preview)
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericPrintDialog : public wxPrintDialogBase
{
public:
    wxGenericPrintDialog(wxWindow *parent,
                         wxPrintDialogData* data = (wxPrintDialogData*)NULL);
    wxGenericPrintDialog(wxWindow *parent, wxPrintData* data);

    virtual ~wxGenericPrintDialog();

    void OnSetup(wxCommandEvent& event);
    void OnRange(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);

    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    virtual int ShowModal();

    wxPrintData& GetPrintData()
        { return m_printDialogData.GetPrintData(); }

    wxPrintDialogData& GetPrintDialogData() { return m_printDialogData; }
    wxDC *GetPrintDC();

public:
//    wxStaticText*       m_printerMessage;
    wxButton*           m_setupButton;
//    wxButton*           m_helpButton;
    wxRadioBox*         m_rangeRadioBox;
    wxTextCtrl*         m_fromText;
    wxTextCtrl*         m_toText;
    wxTextCtrl*         m_noCopiesText;
    wxCheckBox*         m_printToFileCheckBox;
//    wxCheckBox*         m_collateCopiesCheckBox;

    wxPrintDialogData   m_printDialogData;

protected:
    void Init(wxWindow *parent);

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGenericPrintDialog)
};

class WXDLLEXPORT wxGenericPrintSetupDialog : public wxDialog
{
public:
    // There are no configuration options for the dialog, so we
    // just pass the wxPrintData object (no wxPrintSetupDialogData class needed)
    wxGenericPrintSetupDialog(wxWindow *parent, wxPrintData* data);
    virtual ~wxGenericPrintSetupDialog();

    void Init(wxPrintData* data);

    void OnPrinter(wxListEvent& event);

    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    virtual wxComboBox *CreatePaperTypeChoice();

public:
    wxListCtrl*         m_printerListCtrl;
    wxRadioBox*         m_orientationRadioBox;
    wxTextCtrl*         m_printerCommandText;
    wxTextCtrl*         m_printerOptionsText;
    wxCheckBox*         m_colourCheckBox;
    wxComboBox*         m_paperTypeChoice;

    wxPrintData         m_printData;
    wxPrintData&        GetPrintData() { return m_printData; }

    // After pressing OK, write data here.
    wxPrintData*        m_targetData;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(wxGenericPrintSetupDialog)
};
#endif
    // wxUSE_POSTSCRIPT

class WXDLLEXPORT wxGenericPageSetupDialog : public wxPageSetupDialogBase
{
public:
    wxGenericPageSetupDialog(wxWindow *parent = NULL,
                             wxPageSetupDialogData* data = NULL);
    virtual ~wxGenericPageSetupDialog();

    virtual bool TransferDataFromWindow();
    virtual bool TransferDataToWindow();

    virtual wxPageSetupDialogData& GetPageSetupDialogData();

    void OnPrinter(wxCommandEvent& event);
    wxComboBox *CreatePaperTypeChoice(int* x, int* y);

public:
    wxButton*       m_printerButton;
    wxRadioBox*     m_orientationRadioBox;
    wxTextCtrl*     m_marginLeftText;
    wxTextCtrl*     m_marginTopText;
    wxTextCtrl*     m_marginRightText;
    wxTextCtrl*     m_marginBottomText;
    wxComboBox*       m_paperTypeChoice;

    wxPageSetupDialogData m_pageData;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxGenericPageSetupDialog)
};

#endif

#endif
// __PRINTDLGH_G_
