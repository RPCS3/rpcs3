///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/notebook.h
// Purpose:     universal version of wxNotebook
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.02.01
// RCS-ID:      $Id: notebook.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_NOTEBOOK_H_
#define _WX_UNIV_NOTEBOOK_H_

#include "wx/arrstr.h"

class WXDLLEXPORT wxSpinButton;

// ----------------------------------------------------------------------------
// the actions supported by this control
// ----------------------------------------------------------------------------

// change the page: to the next/previous/given one
#define wxACTION_NOTEBOOK_NEXT      wxT("nexttab")
#define wxACTION_NOTEBOOK_PREV      wxT("prevtab")
#define wxACTION_NOTEBOOK_GOTO      wxT("gototab")

// ----------------------------------------------------------------------------
// wxNotebook
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxNotebook : public wxNotebookBase
{
public:
    // ctors and such
    // --------------

    wxNotebook() { Init(); }

    wxNotebook(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxString& name = wxNotebookNameStr)
    {
        Init();

        (void)Create(parent, id, pos, size, style, name);
    }

    // quasi ctor
    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxNotebookNameStr);

    // dtor
    virtual ~wxNotebook();

    // implement wxNotebookBase pure virtuals
    // --------------------------------------

    virtual int SetSelection(size_t nPage) { return DoSetSelection(nPage, SetSelection_SendEvent); }
    virtual int GetSelection() const { return (int) m_sel; }

    // changes selected page without sending events
    int ChangeSelection(size_t nPage) { return DoSetSelection(nPage); }

    virtual bool SetPageText(size_t nPage, const wxString& strText);
    virtual wxString GetPageText(size_t nPage) const;

    virtual int GetPageImage(size_t nPage) const;
    virtual bool SetPageImage(size_t nPage, int nImage);

    virtual void SetPageSize(const wxSize& size);
    virtual void SetPadding(const wxSize& padding);
    virtual void SetTabSize(const wxSize& sz);

    virtual wxSize CalcSizeFromPage(const wxSize& sizePage) const;

    virtual bool DeleteAllPages();

    virtual bool InsertPage(size_t nPage,
                            wxNotebookPage *pPage,
                            const wxString& strText,
                            bool bSelect = false,
                            int imageId = -1);

    // style tests
    // -----------

    // return true if all tabs have the same width
    bool FixedSizeTabs() const { return HasFlag(wxNB_FIXEDWIDTH); }

    // return wxTOP/wxBOTTOM/wxRIGHT/wxLEFT
    wxDirection GetTabOrientation() const;

    // return true if the notebook has tabs at the sidesand not at the top (or
    // bottom) as usual
    bool IsVertical() const;

    // hit testing
    // -----------

    virtual int HitTest(const wxPoint& pt, long *flags = NULL) const;

    // input handling
    // --------------

    virtual bool PerformAction(const wxControlAction& action,
                               long numArg = 0l,
                               const wxString& strArg = wxEmptyString);

    static wxInputHandler *GetStdInputHandler(wxInputHandler *handlerDef);
    virtual wxInputHandler *DoGetStdInputHandler(wxInputHandler *handlerDef)
    {
        return GetStdInputHandler(handlerDef);
    }

    // refresh the currently selected tab
    void RefreshCurrent();

protected:
    virtual wxNotebookPage *DoRemovePage(size_t nPage);

    // drawing
    virtual void DoDraw(wxControlRenderer *renderer);
    void DoDrawTab(wxDC& dc, const wxRect& rect, size_t n);

    // resizing
    virtual wxSize DoGetBestClientSize() const;
    virtual void DoMoveWindow(int x, int y, int width, int height);
    virtual void DoSetSize(int x, int y,
                           int width, int height,
                           int sizeFlags = wxSIZE_AUTO);

    int DoSetSelection(size_t nPage, int flags = 0);

    // common part of all ctors
    void Init();

    // resize the tab to fit its title (and icon if any)
    void ResizeTab(int page);

    // recalculate the geometry of the notebook completely
    void Relayout();

    // is the spin button currently shown?
    bool HasSpinBtn() const;

    // calculate last (fully) visible tab: updates m_lastVisible
    void CalcLastVisibleTab();

    // show or hide the spin control for tabs scrolling depending on whether it
    // is needed or not
    void UpdateSpinBtn();

    // position the spin button
    void PositionSpinBtn();

    // refresh the given tab only
    void RefreshTab(int page, bool forceSelected = false);

    // refresh all tabs
    void RefreshAllTabs();

    // get the tab rect (inefficient, don't use this in a loop)
    wxRect GetTabRect(int page) const;

    // get the rectangle containing all tabs
    wxRect GetAllTabsRect() const;

    // get the part occupied by the tabs - slightly smaller than
    // GetAllTabsRect() because the tabs may be indented from it
    wxRect GetTabsPart() const;

    // calculate the tab size (without padding)
    wxSize CalcTabSize(int page) const;

    // get the (cached) size of a tab
    void GetTabSize(int page, wxCoord *w, wxCoord *h) const;

    // get the (cached) width of the tab
    wxCoord GetTabWidth(int page) const
        { return FixedSizeTabs() ? m_widthMax : m_widths[page]; }

    // return true if the tab has an associated image
    bool HasImage(int page) const
        { return m_imageList && m_images[page] != -1; }

    // get the part of the notebook reserved for the pages (slightly larger
    // than GetPageRect() as we draw a border and leave marginin between)
    wxRect GetPagePart() const;

    // get the page rect in our client coords
    wxRect GetPageRect() const;

    // get our client size from the page size
    wxSize GetSizeForPage(const wxSize& size) const;

    // scroll the tabs so that the first page shown becomes the given one
    void ScrollTo(int page);

    // scroll the tabs so that the first page shown becomes the given one
    void ScrollLastTo(int page);

    // the pages titles
    wxArrayString m_titles;

    // the current selection
    size_t m_sel;

    // the spin button to change the pages
    wxSpinButton *m_spinbtn;

    // the offset of the first page shown (may be changed with m_spinbtn)
    wxCoord m_offset;

    // the first and last currently visible tabs: the name is not completely
    // accurate as m_lastVisible is, in fact, the first tab which is *not*
    // visible: so the visible tabs are those with indexes such that
    // m_firstVisible <= n < m_lastVisible
    size_t m_firstVisible,
           m_lastVisible;

    // the last fully visible item, usually just m_lastVisible - 1 but may be
    // different from it
    size_t m_lastFullyVisible;

    // the height of tabs in a normal notebook or the width of tabs in a
    // notebook with tabs on a side
    wxCoord m_heightTab;

    // the biggest height (or width) of a notebook tab (used only if
    // FixedSizeTabs()) or -1 if not calculated yet
    wxCoord m_widthMax;

    // the cached widths (or heights) of tabs
    wxArrayInt m_widths;

    // the icon indices
    wxArrayInt m_images;

    // the accel indexes for labels
    wxArrayInt m_accels;

    // the padding
    wxSize m_sizePad;

    DECLARE_DYNAMIC_CLASS(wxNotebook)
};

#endif // _WX_UNIV_NOTEBOOK_H_

