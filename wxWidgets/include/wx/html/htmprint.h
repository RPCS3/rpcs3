/////////////////////////////////////////////////////////////////////////////
// Name:        htmprint.h
// Purpose:     html printing classes
// Author:      Vaclav Slavik
// Created:     25/09/99
// RCS-ID:      $Id: htmprint.h 62758 2009-12-01 20:21:46Z BP $
// Copyright:   (c) Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HTMPRINT_H_
#define _WX_HTMPRINT_H_

#include "wx/defs.h"

#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE

#include "wx/html/htmlcell.h"
#include "wx/html/winpars.h"
#include "wx/html/htmlfilt.h"

#include "wx/print.h"
#include "wx/printdlg.h"

#include <limits.h> // INT_MAX

//--------------------------------------------------------------------------------
// wxHtmlDCRenderer
//                  This class is capable of rendering HTML into specified
//                  portion of DC
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlDCRenderer : public wxObject
{
public:
    wxHtmlDCRenderer();
    virtual ~wxHtmlDCRenderer();

    // Following 3 methods *must* be called before any call to Render:

    // Assign DC to this render
    void SetDC(wxDC *dc, double pixel_scale = 1.0);

    // Sets size of output rectangle, in pixels. Note that you *can't* change
    // width of the rectangle between calls to Render! (You can freely change height.)
    void SetSize(int width, int height);

    // Sets the text to be displayed.
    // Basepath is base directory (html string would be stored there if it was in
    // file). It is used to determine path for loading images, for example.
    // isdir is false if basepath is filename, true if it is directory name
    // (see wxFileSystem for detailed explanation)
    void SetHtmlText(const wxString& html, const wxString& basepath = wxEmptyString, bool isdir = true);

    // Sets fonts to be used when displaying HTML page. (if size null then default sizes used).
    void SetFonts(const wxString& normal_face, const wxString& fixed_face, const int *sizes = NULL);

    // Sets font sizes to be relative to the given size or the system
    // default size; use either specified or default font
    void SetStandardFonts(int size = -1,
                          const wxString& normal_face = wxEmptyString,
                          const wxString& fixed_face = wxEmptyString);

    // [x,y] is position of upper-left corner of printing rectangle (see SetSize)
    // from is y-coordinate of the very first visible cell
    // to is y-coordinate of the next following page break, if any
    // Returned value is y coordinate of first cell than didn't fit onto page.
    // Use this value as 'from' in next call to Render in order to print multiple pages
    // document
    // If dont_render is TRUE then nothing is rendered into DC and it only counts
    // pixels and return y coord of the next page
    //
    // known_pagebreaks and number_of_pages are used only when counting pages;
    // otherwise, their default values should be used. Their purpose is to
    // support pagebreaks using a subset of CSS2's <DIV>. The <DIV> handler
    // needs to know what pagebreaks have already been set so that it doesn't
    // set the same pagebreak twice.
    //
    // CAUTION! Render() changes DC's user scale and does NOT restore it!
    int Render(int x, int y, wxArrayInt& known_pagebreaks, int from = 0,
               int dont_render = FALSE, int to = INT_MAX);

    // returns total height of the html document
    // (compare Render's return value with this)
    int GetTotalHeight();

private:
    wxDC *m_DC;
    wxHtmlWinParser *m_Parser;
    wxFileSystem *m_FS;
    wxHtmlContainerCell *m_Cells;
    int m_MaxWidth, m_Width, m_Height;

    DECLARE_NO_COPY_CLASS(wxHtmlDCRenderer)
};





enum {
    wxPAGE_ODD,
    wxPAGE_EVEN,
    wxPAGE_ALL
};



//--------------------------------------------------------------------------------
// wxHtmlPrintout
//                  This class is derived from standard wxWidgets printout class
//                  and is used to print HTML documents.
//--------------------------------------------------------------------------------


class WXDLLIMPEXP_HTML wxHtmlPrintout : public wxPrintout
{
public:
    wxHtmlPrintout(const wxString& title = wxT("Printout"));
    virtual ~wxHtmlPrintout();

    void SetHtmlText(const wxString& html, const wxString &basepath = wxEmptyString, bool isdir = true);
            // prepares the class for printing this html document.
            // Must be called before using the class, in fact just after constructor
            //
            // basepath is base directory (html string would be stored there if it was in
            // file). It is used to determine path for loading images, for example.
            // isdir is false if basepath is filename, true if it is directory name
            // (see wxFileSystem for detailed explanation)

    void SetHtmlFile(const wxString &htmlfile);
            // same as SetHtmlText except that it takes regular file as the parameter

    void SetHeader(const wxString& header, int pg = wxPAGE_ALL);
    void SetFooter(const wxString& footer, int pg = wxPAGE_ALL);
            // sets header/footer for the document. The argument is interpreted as HTML document.
            // You can use macros in it:
            //   @PAGENUM@ is replaced by page number
            //   @PAGESCNT@ is replaced by total number of pages
            //
            // pg is one of wxPAGE_ODD, wxPAGE_EVEN and wx_PAGE_ALL constants.
            // You can set different header/footer for odd and even pages

    // Sets fonts to be used when displaying HTML page. (if size null then default sizes used).
    void SetFonts(const wxString& normal_face, const wxString& fixed_face, const int *sizes = NULL);

    // Sets font sizes to be relative to the given size or the system
    // default size; use either specified or default font
    void SetStandardFonts(int size = -1,
                          const wxString& normal_face = wxEmptyString,
                          const wxString& fixed_face = wxEmptyString);

    void SetMargins(float top = 25.2, float bottom = 25.2, float left = 25.2, float right = 25.2,
                    float spaces = 5);
            // sets margins in milimeters. Defaults to 1 inch for margins and 0.5cm for space
            // between text and header and/or footer

    // wxPrintout stuff:
    bool OnPrintPage(int page);
    bool HasPage(int page);
    void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);
    bool OnBeginDocument(int startPage, int endPage);
    void OnPreparePrinting();

    // Adds input filter
    static void AddFilter(wxHtmlFilter *filter);

    // Cleanup
    static void CleanUpStatics();

private:

    void RenderPage(wxDC *dc, int page);
            // renders one page into dc
    wxString TranslateHeader(const wxString& instr, int page);
            // substitute @PAGENUM@ and @PAGESCNT@ by real values
    void CountPages();
            // counts pages and fills m_NumPages and m_PageBreaks


private:
    int m_NumPages;
    //int m_PageBreaks[wxHTML_PRINT_MAX_PAGES];
    wxArrayInt m_PageBreaks;

    wxString m_Document, m_BasePath;
    bool m_BasePathIsDir;
    wxString m_Headers[2], m_Footers[2];

    int m_HeaderHeight, m_FooterHeight;
    wxHtmlDCRenderer *m_Renderer, *m_RendererHdr;
    float m_MarginTop, m_MarginBottom, m_MarginLeft, m_MarginRight, m_MarginSpace;

    // list of HTML filters
    static wxList m_Filters;

    DECLARE_NO_COPY_CLASS(wxHtmlPrintout)
};





//--------------------------------------------------------------------------------
// wxHtmlEasyPrinting
//                  This class provides very simple interface to printing
//                  architecture. It allows you to print HTML documents only
//                  with very few commands.
//
//                  Note : do not create this class on stack only.
//                         You should create an instance on app startup and
//                         use this instance for all printing. Why? The class
//                         stores page&printer settings in it.
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlEasyPrinting : public wxObject
{
public:
    wxHtmlEasyPrinting(const wxString& name = wxT("Printing"), wxWindow *parentWindow = NULL);
    virtual ~wxHtmlEasyPrinting();

    bool PreviewFile(const wxString &htmlfile);
    bool PreviewText(const wxString &htmltext, const wxString& basepath = wxEmptyString);
            // Preview file / html-text for printing
            // (and offers printing)
            // basepath is base directory for opening subsequent files (e.g. from <img> tag)

    bool PrintFile(const wxString &htmlfile);
    bool PrintText(const wxString &htmltext, const wxString& basepath = wxEmptyString);
            // Print file / html-text w/o preview

    void PageSetup();
            // pop up printer or page setup dialog

    void SetHeader(const wxString& header, int pg = wxPAGE_ALL);
    void SetFooter(const wxString& footer, int pg = wxPAGE_ALL);
            // sets header/footer for the document. The argument is interpreted as HTML document.
            // You can use macros in it:
            //   @PAGENUM@ is replaced by page number
            //   @PAGESCNT@ is replaced by total number of pages
            //
            // pg is one of wxPAGE_ODD, wxPAGE_EVEN and wx_PAGE_ALL constants.
            // You can set different header/footer for odd and even pages

    void SetFonts(const wxString& normal_face, const wxString& fixed_face, const int *sizes = 0);
    // Sets fonts to be used when displaying HTML page. (if size null then default sizes used)

    // Sets font sizes to be relative to the given size or the system
    // default size; use either specified or default font
    void SetStandardFonts(int size = -1,
                          const wxString& normal_face = wxEmptyString,
                          const wxString& fixed_face = wxEmptyString);

    wxPrintData *GetPrintData();
    wxPageSetupDialogData *GetPageSetupData() {return m_PageSetupData;}
            // return page setting data objects.
            // (You can set their parameters.)

#if wxABI_VERSION >= 20805
    wxWindow* GetParentWindow() const { return m_ParentWindow; }
            // get the parent window
    void SetParentWindow(wxWindow* window) { m_ParentWindow = window; }
            // set the parent window
#endif

#if wxABI_VERSION >= 20811
    const wxString& GetName() const { return m_Name; }
            // get the printout name
    void SetName(const wxString& name) { m_Name = name; }
            // set the printout name
#endif

protected:
    virtual wxHtmlPrintout *CreatePrintout();
    virtual bool DoPreview(wxHtmlPrintout *printout1, wxHtmlPrintout *printout2);
    virtual bool DoPrint(wxHtmlPrintout *printout);

private:
    wxPrintData *m_PrintData;
    wxPageSetupDialogData *m_PageSetupData;
    wxString m_Name;
    int m_FontsSizesArr[7];
    int *m_FontsSizes;
    wxString m_FontFaceFixed, m_FontFaceNormal;

    enum FontMode
    {
        FontMode_Explicit,
        FontMode_Standard
    };
    FontMode m_fontMode;

    wxString m_Headers[2], m_Footers[2];
    wxWindow *m_ParentWindow;

    DECLARE_NO_COPY_CLASS(wxHtmlEasyPrinting)
};




#endif  // wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE

#endif // _WX_HTMPRINT_H_

