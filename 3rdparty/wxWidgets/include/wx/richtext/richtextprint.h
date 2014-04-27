/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextprint.h
// Purpose:     Rich text printing classes
// Author:      Julian Smart
// Created:     2006-10-23
// RCS-ID:      $Id: richtextprint.h 55146 2008-08-21 16:07:54Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RICHTEXTPRINT_H_
#define _WX_RICHTEXTPRINT_H_

#include "wx/defs.h"

#if wxUSE_RICHTEXT & wxUSE_PRINTING_ARCHITECTURE

#include "wx/richtext/richtextbuffer.h"

#include "wx/print.h"
#include "wx/printdlg.h"

#define wxRICHTEXT_PRINT_MAX_PAGES 99999

// Header/footer page identifiers
enum wxRichTextOddEvenPage {
    wxRICHTEXT_PAGE_ODD,
    wxRICHTEXT_PAGE_EVEN,
    wxRICHTEXT_PAGE_ALL
};

// Header/footer text locations
enum wxRichTextPageLocation {
    wxRICHTEXT_PAGE_LEFT,
    wxRICHTEXT_PAGE_CENTRE,
    wxRICHTEXT_PAGE_RIGHT
};

/*!
 * Header/footer data
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextHeaderFooterData: public wxObject
{
public:
    wxRichTextHeaderFooterData() { Init(); }
    wxRichTextHeaderFooterData(const wxRichTextHeaderFooterData& data): wxObject() { Copy(data); }

    /// Initialise
    void Init() { m_headerMargin = 20; m_footerMargin = 20; m_showOnFirstPage = true; }

    /// Copy
    void Copy(const wxRichTextHeaderFooterData& data);

    /// Assignment
    void operator= (const wxRichTextHeaderFooterData& data) { Copy(data); }

    /// Set/get header text, e.g. wxRICHTEXT_PAGE_ODD, wxRICHTEXT_PAGE_LEFT
    void SetHeaderText(const wxString& text, wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_ALL, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE);
    wxString GetHeaderText(wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_EVEN, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE) const;

    /// Set/get footer text, e.g. wxRICHTEXT_PAGE_ODD, wxRICHTEXT_PAGE_LEFT
    void SetFooterText(const wxString& text, wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_ALL, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE);
    wxString GetFooterText(wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_EVEN, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE) const;

    /// Set/get text
    void SetText(const wxString& text, int headerFooter, wxRichTextOddEvenPage page, wxRichTextPageLocation location);
    wxString GetText(int headerFooter, wxRichTextOddEvenPage page, wxRichTextPageLocation location) const;

    /// Set/get margins between text and header or footer, in tenths of a millimeter
    void SetMargins(int headerMargin, int footerMargin) { m_headerMargin = headerMargin; m_footerMargin = footerMargin; }
    int GetHeaderMargin() const { return m_headerMargin; }
    int GetFooterMargin() const { return m_footerMargin; }

    /// Set/get whether to show header or footer on first page
    void SetShowOnFirstPage(bool showOnFirstPage) { m_showOnFirstPage = showOnFirstPage; }
    bool GetShowOnFirstPage() const { return m_showOnFirstPage; }

    /// Clear all text
    void Clear();

    /// Set/get font
    void SetFont(const wxFont& font) { m_font = font; }
    const wxFont& GetFont() const { return m_font; }

    /// Set/get colour
    void SetTextColour(const wxColour& col) { m_colour = col; }
    const wxColour& GetTextColour() const { return m_colour; }

    DECLARE_CLASS(wxRichTextHeaderFooterData)

private:

    // Strings for left, centre, right, top, bottom, odd, even
    wxString    m_text[12];
    wxFont      m_font;
    wxColour    m_colour;
    int         m_headerMargin;
    int         m_footerMargin;
    bool        m_showOnFirstPage;
};

/*!
 * wxRichTextPrintout
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextPrintout : public wxPrintout
{
public:
    wxRichTextPrintout(const wxString& title = wxT("Printout"));
    virtual ~wxRichTextPrintout();

    /// The buffer to print
    void SetRichTextBuffer(wxRichTextBuffer* buffer) { m_richTextBuffer = buffer; }
    wxRichTextBuffer* GetRichTextBuffer() const { return m_richTextBuffer; }

    /// Set/get header/footer data
    void SetHeaderFooterData(const wxRichTextHeaderFooterData& data) { m_headerFooterData = data; }
    const wxRichTextHeaderFooterData& GetHeaderFooterData() const { return m_headerFooterData; }

    /// Sets margins in 10ths of millimetre. Defaults to 1 inch for margins.
    void SetMargins(int top = 254, int bottom = 254, int left = 254, int right = 254);

    /// Calculate scaling and rectangles, setting the device context scaling
    void CalculateScaling(wxDC* dc, wxRect& textRect, wxRect& headerRect, wxRect& footerRect);

    // wxPrintout virtual functions
    virtual bool OnPrintPage(int page);
    virtual bool HasPage(int page);
    virtual void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);
    virtual bool OnBeginDocument(int startPage, int endPage);
    virtual void OnPreparePrinting();

private:

    /// Renders one page into dc
    void RenderPage(wxDC *dc, int page);

    /// Substitute keywords
    static bool SubstituteKeywords(wxString& str, const wxString& title, int pageNum, int pageCount);

private:

    wxRichTextBuffer*           m_richTextBuffer;
    int                         m_numPages;
    wxArrayInt                  m_pageBreaksStart;
    wxArrayInt                  m_pageBreaksEnd;
    int                         m_marginLeft, m_marginTop, m_marginRight, m_marginBottom;

    wxRichTextHeaderFooterData  m_headerFooterData;

    DECLARE_NO_COPY_CLASS(wxRichTextPrintout)
};

/*
 *! wxRichTextPrinting
 * A simple interface to perform wxRichTextBuffer printing.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextPrinting : public wxObject
{
public:
    wxRichTextPrinting(const wxString& name = wxT("Printing"), wxWindow *parentWindow = NULL);
    virtual ~wxRichTextPrinting();

    /// Preview the file or buffer
    bool PreviewFile(const wxString& richTextFile);
    bool PreviewBuffer(const wxRichTextBuffer& buffer);

    /// Print the file or buffer
    bool PrintFile(const wxString& richTextFile);
    bool PrintBuffer(const wxRichTextBuffer& buffer);

    /// Shows page setup dialog
    void PageSetup();

    /// Set/get header/footer data
    void SetHeaderFooterData(const wxRichTextHeaderFooterData& data) { m_headerFooterData = data; }
    const wxRichTextHeaderFooterData& GetHeaderFooterData() const { return m_headerFooterData; }

    /// Set/get header text, e.g. wxRICHTEXT_PAGE_ODD, wxRICHTEXT_PAGE_LEFT
    void SetHeaderText(const wxString& text, wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_ALL, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE);
    wxString GetHeaderText(wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_EVEN, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE) const;

    /// Set/get footer text, e.g. wxRICHTEXT_PAGE_ODD, wxRICHTEXT_PAGE_LEFT
    void SetFooterText(const wxString& text, wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_ALL, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE);
    wxString GetFooterText(wxRichTextOddEvenPage page = wxRICHTEXT_PAGE_EVEN, wxRichTextPageLocation location = wxRICHTEXT_PAGE_CENTRE) const;

    /// Show header/footer on first page, or not
    void SetShowOnFirstPage(bool show) { m_headerFooterData.SetShowOnFirstPage(show); }

    /// Set the font
    void SetHeaderFooterFont(const wxFont& font) { m_headerFooterData.SetFont(font); }

    /// Set the colour
    void SetHeaderFooterTextColour(const wxColour& font) { m_headerFooterData.SetTextColour(font); }

    /// Get print and page setup data
    wxPrintData *GetPrintData();
    wxPageSetupDialogData *GetPageSetupData() { return m_pageSetupData; }

    /// Set print and page setup data
    void SetPrintData(const wxPrintData& printData);
    void SetPageSetupData(const wxPageSetupData& pageSetupData);

    /// Set the rich text buffer pointer, deleting the existing object if present
    void SetRichTextBufferPreview(wxRichTextBuffer* buf);
    wxRichTextBuffer* GetRichTextBufferPreview() const { return m_richTextBufferPreview; }

    void SetRichTextBufferPrinting(wxRichTextBuffer* buf);
    wxRichTextBuffer* GetRichTextBufferPrinting() const { return m_richTextBufferPrinting; }

    /// Set/get the parent window
    void SetParentWindow(wxWindow* parent) { m_parentWindow = parent; }
    wxWindow* GetParentWindow() const { return m_parentWindow; }

    /// Set/get the title
    void SetTitle(const wxString& title) { m_title = title; }
    const wxString& GetTitle() const { return m_title; }

    /// Set/get the preview rect
    void SetPreviewRect(const wxRect& rect) { m_previewRect = rect; }
    const wxRect& GetPreviewRect() const { return m_previewRect; }

protected:
    virtual wxRichTextPrintout *CreatePrintout();
    virtual bool DoPreview(wxRichTextPrintout *printout1, wxRichTextPrintout *printout2);
    virtual bool DoPrint(wxRichTextPrintout *printout);

private:
    wxPrintData*                m_printData;
    wxPageSetupDialogData*      m_pageSetupData;

    wxRichTextHeaderFooterData  m_headerFooterData;
    wxString                    m_title;
    wxWindow*                   m_parentWindow;
    wxRichTextBuffer*           m_richTextBufferPreview;
    wxRichTextBuffer*           m_richTextBufferPrinting;
    wxRect                      m_previewRect;

    DECLARE_NO_COPY_CLASS(wxRichTextPrinting)
};

#endif  // wxUSE_RICHTEXT & wxUSE_PRINTING_ARCHITECTURE

#endif // _WX_RICHTEXTPRINT_H_

