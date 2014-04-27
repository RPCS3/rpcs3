///////////////////////////////////////////////////////////////////////////////
// Name:        wx/htmllbox.h
// Purpose:     wxHtmlListBox is a listbox whose items are wxHtmlCells
// Author:      Vadim Zeitlin
// Modified by:
// Created:     31.05.03
// RCS-ID:      $Id: htmllbox.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HTMLLBOX_H_
#define _WX_HTMLLBOX_H_

#include "wx/vlbox.h"               // base class
#include "wx/html/htmlwin.h"
#include "wx/ctrlsub.h"

#if wxUSE_FILESYSTEM
    #include "wx/filesys.h"
#endif // wxUSE_FILESYSTEM

class WXDLLIMPEXP_FWD_HTML wxHtmlCell;
class WXDLLIMPEXP_FWD_HTML wxHtmlWinParser;
class WXDLLIMPEXP_FWD_HTML wxHtmlListBoxCache;
class WXDLLIMPEXP_FWD_HTML wxHtmlListBoxStyle;

extern WXDLLIMPEXP_DATA_HTML(const wxChar) wxHtmlListBoxNameStr[];
extern WXDLLIMPEXP_DATA_HTML(const wxChar) wxSimpleHtmlListBoxNameStr[];

// ----------------------------------------------------------------------------
// wxHtmlListBox
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlListBox : public wxVListBox,
                                       public wxHtmlWindowInterface,
                                       public wxHtmlWindowMouseHelper
{
    DECLARE_ABSTRACT_CLASS(wxHtmlListBox)
public:
    // constructors and such
    // ---------------------

    // default constructor, you must call Create() later
    wxHtmlListBox();

    // normal constructor which calls Create() internally
    wxHtmlListBox(wxWindow *parent,
                  wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0,
                  const wxString& name = wxHtmlListBoxNameStr);

    // really creates the control and sets the initial number of items in it
    // (which may be changed later with SetItemCount())
    //
    // the only special style which may be specified here is wxLB_MULTIPLE
    //
    // returns true on success or false if the control couldn't be created
    bool Create(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxHtmlListBoxNameStr);

    // destructor cleans up whatever resources we use
    virtual ~wxHtmlListBox();

    // override some base class virtuals
    virtual void RefreshLine(size_t line);
    virtual void RefreshLines(size_t from, size_t to);
    virtual void RefreshAll();
    virtual void SetItemCount(size_t count);

#if wxUSE_FILESYSTEM
    // retrieve the file system used by the wxHtmlWinParser: if you use
    // relative paths in your HTML, you should use its ChangePathTo() method
    wxFileSystem& GetFileSystem() { return m_filesystem; }
    const wxFileSystem& GetFileSystem() const { return m_filesystem; }
#endif // wxUSE_FILESYSTEM

    virtual void OnInternalIdle();

protected:
    // this method must be implemented in the derived class and should return
    // the body (i.e. without <html>) of the HTML for the given item
    virtual wxString OnGetItem(size_t n) const = 0;

    // this function may be overridden to decorate HTML returned by OnGetItem()
    virtual wxString OnGetItemMarkup(size_t n) const;


    // this method allows to customize the selection appearance: it may be used
    // to specify the colour of the text which normally has the given colour
    // colFg when it is inside the selection
    //
    // by default, the original colour is not used at all and all text has the
    // same (default for this system) colour inside selection
    virtual wxColour GetSelectedTextColour(const wxColour& colFg) const;

    // this is the same as GetSelectedTextColour() but allows to customize the
    // background colour -- this is even more rarely used as you can change it
    // globally using SetSelectionBackground()
    virtual wxColour GetSelectedTextBgColour(const wxColour& colBg) const;


    // we implement both of these functions in terms of OnGetItem(), they are
    // not supposed to be overridden by our descendants
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
    virtual wxCoord OnMeasureItem(size_t n) const;

    // This method may be overriden to handle clicking on a link in
    // the listbox. By default, clicking links is ignored.
    virtual void OnLinkClicked(size_t n, const wxHtmlLinkInfo& link);

    // event handlers
    void OnSize(wxSizeEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);


    // common part of all ctors
    void Init();

    // ensure that the given item is cached
    void CacheItem(size_t n) const;

private:
    // wxHtmlWindowInterface methods:
    virtual void SetHTMLWindowTitle(const wxString& title);
    virtual void OnHTMLLinkClicked(const wxHtmlLinkInfo& link);
    virtual wxHtmlOpeningStatus OnHTMLOpeningURL(wxHtmlURLType type,
                                                 const wxString& url,
                                                 wxString *redirect) const;
    virtual wxPoint HTMLCoordsToWindow(wxHtmlCell *cell,
                                       const wxPoint& pos) const;
    virtual wxWindow* GetHTMLWindow();
    virtual wxColour GetHTMLBackgroundColour() const;
    virtual void SetHTMLBackgroundColour(const wxColour& clr);
    virtual void SetHTMLBackgroundImage(const wxBitmap& bmpBg);
    virtual void SetHTMLStatusText(const wxString& text);
    virtual wxCursor GetHTMLCursor(HTMLCursor type) const;

    // returns index of item that contains given HTML cell
    size_t GetItemForCell(const wxHtmlCell *cell) const;

    // return physical coordinates of root wxHtmlCell of n-th item
    wxPoint GetRootCellCoords(size_t n) const;

    // Converts physical coordinates stored in @a pos into coordinates
    // relative to the root cell of the item under mouse cursor, if any. If no
    // cell is found under the cursor, returns false.  Otherwise stores the new
    // coordinates back into @a pos and pointer to the cell under cursor into
    // @a cell and returns true.
    bool PhysicalCoordsToCell(wxPoint& pos, wxHtmlCell*& cell) const;

    // The opposite of PhysicalCoordsToCell: converts coordinates relative to
    // given cell to physical coordinates in the window
    wxPoint CellCoordsToPhysical(const wxPoint& pos, wxHtmlCell *cell) const;

private:
    // this class caches the pre-parsed HTML to speed up display
    wxHtmlListBoxCache *m_cache;

    // HTML parser we use
    wxHtmlWinParser *m_htmlParser;

#if wxUSE_FILESYSTEM
    // file system used by m_htmlParser
    wxFileSystem m_filesystem;
#endif // wxUSE_FILESYSTEM

    // rendering style for the parser which allows us to customize our colours
    wxHtmlListBoxStyle *m_htmlRendStyle;


    // it calls our GetSelectedTextColour() and GetSelectedTextBgColour()
    friend class wxHtmlListBoxStyle;
    friend class wxHtmlListBoxWinInterface;


    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxHtmlListBox)
};


// ----------------------------------------------------------------------------
// wxSimpleHtmlListBox
// ----------------------------------------------------------------------------

#define wxHLB_DEFAULT_STYLE     wxBORDER_SUNKEN
#define wxHLB_MULTIPLE          wxLB_MULTIPLE

class WXDLLIMPEXP_HTML wxSimpleHtmlListBox : public wxHtmlListBox,
                                             public wxItemContainer
{
public:
    // wxListbox-compatible constructors
    // ---------------------------------

    wxSimpleHtmlListBox() { }

    wxSimpleHtmlListBox(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = NULL,
                        long style = wxHLB_DEFAULT_STYLE,
                        const wxValidator& validator = wxDefaultValidator,
                        const wxString& name = wxSimpleHtmlListBoxNameStr)
    {
        Create(parent, id, pos, size, n, choices, style, validator, name);
    }

    wxSimpleHtmlListBox(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        const wxArrayString& choices,
                        long style = wxHLB_DEFAULT_STYLE,
                        const wxValidator& validator = wxDefaultValidator,
                        const wxString& name = wxSimpleHtmlListBoxNameStr)
    {
        Create(parent, id, pos, size, choices, style, validator, name);
    }

    bool Create(wxWindow *parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                int n = 0, const wxString choices[] = NULL,
                long style = wxHLB_DEFAULT_STYLE,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxSimpleHtmlListBoxNameStr);
    bool Create(wxWindow *parent, wxWindowID id,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                long style = wxHLB_DEFAULT_STYLE,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxSimpleHtmlListBoxNameStr);

    virtual ~wxSimpleHtmlListBox();

    // these must be overloaded otherwise the compiler will complain
    // about  wxItemContainerImmutable::[G|S]etSelection being pure virtuals...
    void SetSelection(int n)
        { wxVListBox::SetSelection(n); }
    int GetSelection() const
        { return wxVListBox::GetSelection(); }

    // see ctrlsub.h for more info about this:
    wxCONTROL_ITEMCONTAINER_CLIENTDATAOBJECT_RECAST


    // accessing strings
    // -----------------

    virtual unsigned int GetCount() const
        { return m_items.GetCount(); }

    virtual wxString GetString(unsigned int n) const;

    // override default unoptimized wxItemContainer::GetStrings() function
    wxArrayString GetStrings() const
        { return m_items; }

    virtual void SetString(unsigned int n, const wxString& s);

    virtual void Clear();
    virtual void Delete(unsigned int n);

    // override default unoptimized wxItemContainer::Append() function
    void Append(const wxArrayString& strings);

    // since we override one Append() overload, we need to overload all others too
    int Append(const wxString& item)
        { return wxItemContainer::Append(item); }
    int Append(const wxString& item, void *clientData)
        { return wxItemContainer::Append(item, clientData); }
    int Append(const wxString& item, wxClientData *clientData)
        { return wxItemContainer::Append(item, clientData); }


protected:

    virtual int DoAppend(const wxString& item);
    virtual int DoInsert(const wxString& item, unsigned int pos);

    virtual void DoSetItemClientData(unsigned int n, void *clientData)
        { m_HTMLclientData[n] = clientData; }

    virtual void *DoGetItemClientData(unsigned int n) const
        { return m_HTMLclientData[n]; }
    virtual void DoSetItemClientObject(unsigned int n, wxClientData *clientData)
        { m_HTMLclientData[n] = (void *)clientData; }
    virtual wxClientData *DoGetItemClientObject(unsigned int n) const
        { return (wxClientData *)m_HTMLclientData[n]; }

    // calls wxHtmlListBox::SetItemCount() and RefreshAll()
    void UpdateCount();

    // overload these functions just to change their visibility: users of
    // wxSimpleHtmlListBox shouldn't be allowed to call them directly!
    virtual void SetItemCount(size_t count)
        { wxHtmlListBox::SetItemCount(count); }
    virtual void SetLineCount(size_t count)
        { wxHtmlListBox::SetLineCount(count); }

    virtual wxString OnGetItem(size_t n) const
        { return m_items[n]; }

    wxArrayString   m_items;
    wxArrayPtrVoid  m_HTMLclientData;

    // Note: For the benefit of old compilers (like gcc-2.8) this should
    // not be named m_clientdata as that clashes with the name of an
    // anonymous struct member in wxEvtHandler, which we derive from.

    DECLARE_NO_COPY_CLASS(wxSimpleHtmlListBox)
};

#endif // _WX_HTMLLBOX_H_

