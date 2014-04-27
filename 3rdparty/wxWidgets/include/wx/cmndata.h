/////////////////////////////////////////////////////////////////////////////
// Name:        wx/cmndata.h
// Purpose:     Common GDI data classes
// Author:      Julian Smart and others
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: cmndata.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CMNDATA_H_BASE_
#define _WX_CMNDATA_H_BASE_

#include "wx/window.h"
#include "wx/font.h"
#include "wx/encinfo.h"
#include "wx/colour.h"
#include "wx/gdicmn.h"

#if wxUSE_STREAMS
#include "wx/stream.h"
#endif


class WXDLLIMPEXP_FWD_CORE wxPrintNativeDataBase;


class WXDLLEXPORT wxColourData: public wxObject
{
public:
    wxColourData();
    wxColourData(const wxColourData& data);
    virtual ~wxColourData();

    void SetChooseFull(bool flag) { m_chooseFull = flag; }
    bool GetChooseFull() const { return m_chooseFull; }
    void SetColour(const wxColour& colour) { m_dataColour = colour; }
    const wxColour& GetColour() const { return m_dataColour; }
    wxColour& GetColour() { return m_dataColour; }

    // Array of 16 custom colours
    void SetCustomColour(int i, const wxColour& colour);
    wxColour GetCustomColour(int i);

    void operator=(const wxColourData& data);

public:
    wxColour        m_dataColour;
    wxColour        m_custColours[16];
    bool            m_chooseFull;

private:
    DECLARE_DYNAMIC_CLASS(wxColourData)
};

class WXDLLEXPORT wxFontData : public wxObject
{
public:
    wxFontData();
    virtual ~wxFontData();

    wxFontData(const wxFontData& data)
        : wxObject(),
          m_fontColour(data.m_fontColour),
          m_showHelp(data.m_showHelp),
          m_allowSymbols(data.m_allowSymbols),
          m_enableEffects(data.m_enableEffects),
          m_initialFont(data.m_initialFont),
          m_chosenFont(data.m_chosenFont),
          m_minSize(data.m_minSize),
          m_maxSize(data.m_maxSize),
          m_encoding(data.m_encoding),
          m_encodingInfo(data.m_encodingInfo)
    {
    }

    wxFontData& operator=(const wxFontData& data)
    {
        wxObject::operator=(data);
        m_fontColour     = data.m_fontColour;
        m_showHelp       = data.m_showHelp;
        m_allowSymbols   = data.m_allowSymbols;
        m_enableEffects  = data.m_enableEffects;
        m_initialFont    = data.m_initialFont;
        m_chosenFont     = data.m_chosenFont;
        m_minSize        = data.m_minSize;
        m_maxSize        = data.m_maxSize;
        m_encoding       = data.m_encoding;
        m_encodingInfo   = data.m_encodingInfo;
        return *this;
    }

    void SetAllowSymbols(bool flag) { m_allowSymbols = flag; }
    bool GetAllowSymbols() const { return m_allowSymbols; }

    void SetColour(const wxColour& colour) { m_fontColour = colour; }
    const wxColour& GetColour() const { return m_fontColour; }

    void SetShowHelp(bool flag) { m_showHelp = flag; }
    bool GetShowHelp() const { return m_showHelp; }

    void EnableEffects(bool flag) { m_enableEffects = flag; }
    bool GetEnableEffects() const { return m_enableEffects; }

    void SetInitialFont(const wxFont& font) { m_initialFont = font; }
    wxFont GetInitialFont() const { return m_initialFont; }

    void SetChosenFont(const wxFont& font) { m_chosenFont = font; }
    wxFont GetChosenFont() const { return m_chosenFont; }

    void SetRange(int minRange, int maxRange) { m_minSize = minRange; m_maxSize = maxRange; }

    // encoding info is split into 2 parts: the logical wxWin encoding
    // (wxFontEncoding) and a structure containing the native parameters for
    // it (wxNativeEncodingInfo)
    wxFontEncoding GetEncoding() const { return m_encoding; }
    void SetEncoding(wxFontEncoding encoding) { m_encoding = encoding; }

    wxNativeEncodingInfo& EncodingInfo() { return m_encodingInfo; }


    // public for backwards compatibility only: don't use directly
public:
    wxColour        m_fontColour;
    bool            m_showHelp;
    bool            m_allowSymbols;
    bool            m_enableEffects;
    wxFont          m_initialFont;
    wxFont          m_chosenFont;
    int             m_minSize;
    int             m_maxSize;

private:
    wxFontEncoding       m_encoding;
    wxNativeEncodingInfo m_encodingInfo;

private:
    DECLARE_DYNAMIC_CLASS(wxFontData)
};

#if wxUSE_PRINTING_ARCHITECTURE
/*
 * wxPrintData
 * Encapsulates printer information (not printer dialog information)
 */

enum wxPrintBin
{
    wxPRINTBIN_DEFAULT,

    wxPRINTBIN_ONLYONE,
    wxPRINTBIN_LOWER,
    wxPRINTBIN_MIDDLE,
    wxPRINTBIN_MANUAL,
    wxPRINTBIN_ENVELOPE,
    wxPRINTBIN_ENVMANUAL,
    wxPRINTBIN_AUTO,
    wxPRINTBIN_TRACTOR,
    wxPRINTBIN_SMALLFMT,
    wxPRINTBIN_LARGEFMT,
    wxPRINTBIN_LARGECAPACITY,
    wxPRINTBIN_CASSETTE,
    wxPRINTBIN_FORMSOURCE,

    wxPRINTBIN_USER
};

const int wxPRINTMEDIA_DEFAULT = 0;

class WXDLLEXPORT wxPrintData: public wxObject
{
public:
    wxPrintData();
    wxPrintData(const wxPrintData& printData);
    virtual ~wxPrintData();

    int GetNoCopies() const { return m_printNoCopies; }
    bool GetCollate() const { return m_printCollate; }
    int  GetOrientation() const { return m_printOrientation; }
    bool IsOrientationReversed() const { return m_printOrientationReversed; }

    // Is this data OK for showing the print dialog?
    bool Ok() const { return IsOk(); }
    bool IsOk() const ;

    const wxString& GetPrinterName() const { return m_printerName; }
    bool GetColour() const { return m_colour; }
    wxDuplexMode GetDuplex() const { return m_duplexMode; }
    wxPaperSize GetPaperId() const { return m_paperId; }
    const wxSize& GetPaperSize() const { return m_paperSize; } // Not used yet: confusable with paper size
                                                                      // in wxPageSetupDialogData
    wxPrintQuality GetQuality() const { return m_printQuality; }
    wxPrintBin GetBin() const { return m_bin; }
    wxPrintMode GetPrintMode() const { return m_printMode; }
    int GetMedia() const { return m_media; }

    void SetNoCopies(int v) { m_printNoCopies = v; }
    void SetCollate(bool flag) { m_printCollate = flag; }
    void SetOrientation(int orient) { m_printOrientation = orient; }
    void SetOrientationReversed(bool reversed) { m_printOrientationReversed = reversed; }

    void SetPrinterName(const wxString& name) { m_printerName = name; }
    void SetColour(bool colour) { m_colour = colour; }
    void SetDuplex(wxDuplexMode duplex) { m_duplexMode = duplex; }
    void SetPaperId(wxPaperSize sizeId) { m_paperId = sizeId; }
    void SetPaperSize(const wxSize& sz) { m_paperSize = sz; }
    void SetQuality(wxPrintQuality quality) { m_printQuality = quality; }
    void SetBin(wxPrintBin bin) { m_bin = bin; }
    void SetMedia(int media) { m_media = media; }
    void SetPrintMode(wxPrintMode printMode) { m_printMode = printMode; }

    wxString GetFilename() const { return m_filename; }
    void SetFilename( const wxString &filename ) { m_filename = filename; }

    void operator=(const wxPrintData& data);

    char* GetPrivData() const { return m_privData; }
    int GetPrivDataLen() const { return m_privDataLen; }
    void SetPrivData( char *privData, int len );


#if WXWIN_COMPATIBILITY_2_4
    // PostScript-specific data
    wxString GetPrinterCommand() const;
    wxString GetPrinterOptions() const;
    wxString GetPreviewCommand() const;
    wxString GetFontMetricPath() const;
    double GetPrinterScaleX() const;
    double GetPrinterScaleY() const;
    long GetPrinterTranslateX() const;
    long GetPrinterTranslateY() const;

    void SetPrinterCommand(const wxString& command);
    void SetPrinterOptions(const wxString& options);
    void SetPreviewCommand(const wxString& command);
    void SetFontMetricPath(const wxString& path);
    void SetPrinterScaleX(double x);
    void SetPrinterScaleY(double y);
    void SetPrinterScaling(double x, double y);
    void SetPrinterTranslateX(long x);
    void SetPrinterTranslateY(long y);
    void SetPrinterTranslation(long x, long y);
#endif

    // Convert between wxPrintData and native data
    void ConvertToNative();
    void ConvertFromNative();
    // Holds the native print data
    wxPrintNativeDataBase *GetNativeData() const { return m_nativeData; }

private:
    wxPrintBin      m_bin;
    int             m_media;
    wxPrintMode     m_printMode;

    int             m_printNoCopies;
    int             m_printOrientation;
    bool            m_printOrientationReversed;
    bool            m_printCollate;

    wxString        m_printerName;
    bool            m_colour;
    wxDuplexMode    m_duplexMode;
    wxPrintQuality  m_printQuality;
    wxPaperSize     m_paperId;
    wxSize          m_paperSize;

    wxString        m_filename;

    char* m_privData;
    int   m_privDataLen;

    wxPrintNativeDataBase  *m_nativeData;

private:
    DECLARE_DYNAMIC_CLASS(wxPrintData)
};

/*
 * wxPrintDialogData
 * Encapsulates information displayed and edited in the printer dialog box.
 * Contains a wxPrintData object which is filled in according to the values retrieved
 * from the dialog.
 */

class WXDLLEXPORT wxPrintDialogData: public wxObject
{
public:
    wxPrintDialogData();
    wxPrintDialogData(const wxPrintDialogData& dialogData);
    wxPrintDialogData(const wxPrintData& printData);
    virtual ~wxPrintDialogData();

    int GetFromPage() const { return m_printFromPage; }
    int GetToPage() const { return m_printToPage; }
    int GetMinPage() const { return m_printMinPage; }
    int GetMaxPage() const { return m_printMaxPage; }
    int GetNoCopies() const { return m_printNoCopies; }
    bool GetAllPages() const { return m_printAllPages; }
    bool GetSelection() const { return m_printSelection; }
    bool GetCollate() const { return m_printCollate; }
    bool GetPrintToFile() const { return m_printToFile; }
#if WXWIN_COMPATIBILITY_2_4
    bool GetSetupDialog() const { return m_printSetupDialog; }
#endif
    void SetFromPage(int v) { m_printFromPage = v; }
    void SetToPage(int v) { m_printToPage = v; }
    void SetMinPage(int v) { m_printMinPage = v; }
    void SetMaxPage(int v) { m_printMaxPage = v; }
    void SetNoCopies(int v) { m_printNoCopies = v; }
    void SetAllPages(bool flag) { m_printAllPages = flag; }
    void SetSelection(bool flag) { m_printSelection = flag; }
    void SetCollate(bool flag) { m_printCollate = flag; }
    void SetPrintToFile(bool flag) { m_printToFile = flag; }
#if WXWIN_COMPATIBILITY_2_4
    void SetSetupDialog(bool flag) { m_printSetupDialog = flag; }
#endif
    void EnablePrintToFile(bool flag) { m_printEnablePrintToFile = flag; }
    void EnableSelection(bool flag) { m_printEnableSelection = flag; }
    void EnablePageNumbers(bool flag) { m_printEnablePageNumbers = flag; }
    void EnableHelp(bool flag) { m_printEnableHelp = flag; }

    bool GetEnablePrintToFile() const { return m_printEnablePrintToFile; }
    bool GetEnableSelection() const { return m_printEnableSelection; }
    bool GetEnablePageNumbers() const { return m_printEnablePageNumbers; }
    bool GetEnableHelp() const { return m_printEnableHelp; }

    // Is this data OK for showing the print dialog?
    bool Ok() const { return IsOk(); }
    bool IsOk() const { return m_printData.Ok() ; }

    wxPrintData& GetPrintData() { return m_printData; }
    void SetPrintData(const wxPrintData& printData) { m_printData = printData; }

    void operator=(const wxPrintDialogData& data);
    void operator=(const wxPrintData& data); // Sets internal m_printData member

private:
    int             m_printFromPage;
    int             m_printToPage;
    int             m_printMinPage;
    int             m_printMaxPage;
    int             m_printNoCopies;
    bool            m_printAllPages;
    bool            m_printCollate;
    bool            m_printToFile;
    bool            m_printSelection;
    bool            m_printEnableSelection;
    bool            m_printEnablePageNumbers;
    bool            m_printEnableHelp;
    bool            m_printEnablePrintToFile;
#if WXWIN_COMPATIBILITY_2_4
    bool            m_printSetupDialog;
#endif
    wxPrintData     m_printData;

private:
    DECLARE_DYNAMIC_CLASS(wxPrintDialogData)
};

/*
* This is the data used (and returned) by the wxPageSetupDialog.
*/

// Compatibility with old name
#define wxPageSetupData wxPageSetupDialogData

class WXDLLEXPORT wxPageSetupDialogData: public wxObject
{
public:
    wxPageSetupDialogData();
    wxPageSetupDialogData(const wxPageSetupDialogData& dialogData);
    wxPageSetupDialogData(const wxPrintData& printData);
    virtual ~wxPageSetupDialogData();

    wxSize GetPaperSize() const { return m_paperSize; }
    wxPaperSize GetPaperId() const { return m_printData.GetPaperId(); }
    wxPoint GetMinMarginTopLeft() const { return m_minMarginTopLeft; }
    wxPoint GetMinMarginBottomRight() const { return m_minMarginBottomRight; }
    wxPoint GetMarginTopLeft() const { return m_marginTopLeft; }
    wxPoint GetMarginBottomRight() const { return m_marginBottomRight; }

    bool GetDefaultMinMargins() const { return m_defaultMinMargins; }
    bool GetEnableMargins() const { return m_enableMargins; }
    bool GetEnableOrientation() const { return m_enableOrientation; }
    bool GetEnablePaper() const { return m_enablePaper; }
    bool GetEnablePrinter() const { return m_enablePrinter; }
    bool GetDefaultInfo() const { return m_getDefaultInfo; }
    bool GetEnableHelp() const { return m_enableHelp; }

    // Is this data OK for showing the page setup dialog?
    bool Ok() const { return IsOk(); }
    bool IsOk() const { return m_printData.Ok() ; }

    // If a corresponding paper type is found in the paper database, will set the m_printData
    // paper size id member as well.
    void SetPaperSize(const wxSize& sz);

    void SetPaperId(wxPaperSize id) { m_printData.SetPaperId(id); }

    // Sets the wxPrintData id, plus the paper width/height if found in the paper database.
    void SetPaperSize(wxPaperSize id);

    void SetMinMarginTopLeft(const wxPoint& pt) { m_minMarginTopLeft = pt; }
    void SetMinMarginBottomRight(const wxPoint& pt) { m_minMarginBottomRight = pt; }
    void SetMarginTopLeft(const wxPoint& pt) { m_marginTopLeft = pt; }
    void SetMarginBottomRight(const wxPoint& pt) { m_marginBottomRight = pt; }
    void SetDefaultMinMargins(bool flag) { m_defaultMinMargins = flag; }
    void SetDefaultInfo(bool flag) { m_getDefaultInfo = flag; }

    void EnableMargins(bool flag) { m_enableMargins = flag; }
    void EnableOrientation(bool flag) { m_enableOrientation = flag; }
    void EnablePaper(bool flag) { m_enablePaper = flag; }
    void EnablePrinter(bool flag) { m_enablePrinter = flag; }
    void EnableHelp(bool flag) { m_enableHelp = flag; }

    // Use paper size defined in this object to set the wxPrintData
    // paper id
    void CalculateIdFromPaperSize();

    // Use paper id in wxPrintData to set this object's paper size
    void CalculatePaperSizeFromId();

    wxPageSetupDialogData& operator=(const wxPageSetupData& data);
    wxPageSetupDialogData& operator=(const wxPrintData& data);

    wxPrintData& GetPrintData() { return m_printData; }
    const wxPrintData& GetPrintData() const { return m_printData; }
    void SetPrintData(const wxPrintData& printData);

private:
    wxSize          m_paperSize; // The dimensions selected by the user (on return, same as in wxPrintData?)
    wxPoint         m_minMarginTopLeft;
    wxPoint         m_minMarginBottomRight;
    wxPoint         m_marginTopLeft;
    wxPoint         m_marginBottomRight;
    bool            m_defaultMinMargins;
    bool            m_enableMargins;
    bool            m_enableOrientation;
    bool            m_enablePaper;
    bool            m_enablePrinter;
    bool            m_getDefaultInfo; // Equiv. to PSD_RETURNDEFAULT
    bool            m_enableHelp;
    wxPrintData     m_printData;

private:
    DECLARE_DYNAMIC_CLASS(wxPageSetupDialogData)
};

#endif // wxUSE_PRINTING_ARCHITECTURE

#endif
// _WX_CMNDATA_H_BASE_
