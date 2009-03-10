/////////////////////////////////////////////////////////////////////////////
// Name:        wx/html/helpwnd.h
// Purpose:     wxHtmlHelpWindow
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden and Vaclav Slavik
// RCS-ID:      $Id: helpwnd.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) Harm van der Heijden and Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPWND_H_
#define _WX_HELPWND_H_

#include "wx/defs.h"

#if wxUSE_WXHTML_HELP

#include "wx/helpbase.h"
#include "wx/html/helpdata.h"
#include "wx/window.h"
#include "wx/frame.h"
#include "wx/config.h"
#include "wx/splitter.h"
#include "wx/notebook.h"
#include "wx/listbox.h"
#include "wx/choice.h"
#include "wx/combobox.h"
#include "wx/checkbox.h"
#include "wx/stattext.h"
#include "wx/html/htmlwin.h"
#include "wx/html/htmprint.h"

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxTreeEvent;
class WXDLLIMPEXP_FWD_CORE wxTreeCtrl;

// style flags for the Help Frame
#define wxHF_TOOLBAR                0x0001
#define wxHF_CONTENTS               0x0002
#define wxHF_INDEX                  0x0004
#define wxHF_SEARCH                 0x0008
#define wxHF_BOOKMARKS              0x0010
#define wxHF_OPEN_FILES             0x0020
#define wxHF_PRINT                  0x0040
#define wxHF_FLAT_TOOLBAR           0x0080
#define wxHF_MERGE_BOOKS            0x0100
#define wxHF_ICONS_BOOK             0x0200
#define wxHF_ICONS_BOOK_CHAPTER     0x0400
#define wxHF_ICONS_FOLDER           0x0000 // this is 0 since it is default
#define wxHF_DEFAULT_STYLE          (wxHF_TOOLBAR | wxHF_CONTENTS | \
                                     wxHF_INDEX | wxHF_SEARCH | \
                                     wxHF_BOOKMARKS | wxHF_PRINT)
//compatibility:
#define wxHF_OPENFILES               wxHF_OPEN_FILES
#define wxHF_FLATTOOLBAR             wxHF_FLAT_TOOLBAR
#define wxHF_DEFAULTSTYLE            wxHF_DEFAULT_STYLE

struct wxHtmlHelpFrameCfg
{
    int x, y, w, h;
    long sashpos;
    bool navig_on;
};

struct wxHtmlHelpMergedIndexItem;
class wxHtmlHelpMergedIndex;

class WXDLLIMPEXP_FWD_CORE wxHelpControllerBase;
class WXDLLIMPEXP_FWD_HTML wxHtmlHelpController;

/*!
 * Help window
 */

class WXDLLIMPEXP_HTML wxHtmlHelpWindow : public wxWindow
{
    DECLARE_DYNAMIC_CLASS(wxHtmlHelpWindow)

public:
    wxHtmlHelpWindow(wxHtmlHelpData* data = NULL) { Init(data); }
    wxHtmlHelpWindow(wxWindow* parent, wxWindowID wxWindowID,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    int style = wxTAB_TRAVERSAL|wxNO_BORDER,
                    int helpStyle = wxHF_DEFAULT_STYLE,
                    wxHtmlHelpData* data = NULL);
    bool Create(wxWindow* parent, wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                int style = wxTAB_TRAVERSAL|wxNO_BORDER,
                int helpStyle = wxHF_DEFAULT_STYLE);
    virtual ~wxHtmlHelpWindow();

    wxHtmlHelpData* GetData() { return m_Data; }
    wxHtmlHelpController* GetController() const { return m_helpController; }
    void SetController(wxHtmlHelpController* controller);

    // Displays page x. If not found it will offect the user a choice of
    // searching books.
    // Looking for the page runs in these steps:
    // 1. try to locate file named x (if x is for example "doc/howto.htm")
    // 2. try to open starting page of book x
    // 3. try to find x in contents (if x is for example "How To ...")
    // 4. try to find x in index (if x is for example "How To ...")
    bool Display(const wxString& x);

    // Alternative version that works with numeric ID.
    // (uses extension to MS format, <param name="ID" value=id>, see docs)
    bool Display(const int id);

    // Displays help window and focuses contents.
    bool DisplayContents();

    // Displays help window and focuses index.
    bool DisplayIndex();

    // Searches for keyword. Returns true and display page if found, return
    // false otherwise
    // Syntax of keyword is Altavista-like:
    // * words are separated by spaces
    //   (but "\"hello world\"" is only one world "hello world")
    // * word may be pretended by + or -
    //   (+ : page must contain the word ; - : page can't contain the word)
    // * if there is no + or - before the word, + is default
    bool KeywordSearch(const wxString& keyword,
                       wxHelpSearchMode mode = wxHELP_SEARCH_ALL);

    void UseConfig(wxConfigBase *config, const wxString& rootpath = wxEmptyString)
        {
            m_Config = config;
            m_ConfigRoot = rootpath;
            ReadCustomization(config, rootpath);
        }

    // Saves custom settings into cfg config. it will use the path 'path'
    // if given, otherwise it will save info into currently selected path.
    // saved values : things set by SetFonts, SetBorders.
    void ReadCustomization(wxConfigBase *cfg, const wxString& path = wxEmptyString);
    void WriteCustomization(wxConfigBase *cfg, const wxString& path = wxEmptyString);

    // call this to let wxHtmlHelpWindow know page changed
    void NotifyPageChanged();

    // Refreshes Contents and Index tabs
    void RefreshLists();

    // Gets the HTML window
    wxHtmlWindow* GetHtmlWindow() const { return m_HtmlWin; }

    // Gets the splitter window
    wxSplitterWindow* GetSplitterWindow() const { return m_Splitter; }

    // Gets the toolbar
    wxToolBar* GetToolBar() const { return m_toolBar; }

    // Gets the configuration data
    wxHtmlHelpFrameCfg& GetCfgData() { return m_Cfg; }

    // Gets the tree control
    wxTreeCtrl *GetTreeCtrl() const { return m_ContentsBox; }

protected:
    void Init(wxHtmlHelpData* data = NULL);

    // Adds items to m_Contents tree control
    void CreateContents();

    // Adds items to m_IndexList
    void CreateIndex();

    // Add books to search choice panel
    void CreateSearch();

    // Updates "merged index" structure that combines indexes of all books
    // into better searchable structure
    void UpdateMergedIndex();

    // Add custom buttons to toolbar
    virtual void AddToolbarButtons(wxToolBar *toolBar, int style);

    // Displays options dialog (fonts etc.)
    virtual void OptionsDialog();

    void OnToolbar(wxCommandEvent& event);
    void OnContentsSel(wxTreeEvent& event);
    void OnIndexSel(wxCommandEvent& event);
    void OnIndexFind(wxCommandEvent& event);
    void OnIndexAll(wxCommandEvent& event);
    void OnSearchSel(wxCommandEvent& event);
    void OnSearch(wxCommandEvent& event);
    void OnBookmarksSel(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);

    // Images:
    enum {
        IMG_Book = 0,
        IMG_Folder,
        IMG_Page
    };

protected:
    wxHtmlHelpData* m_Data;
    bool m_DataCreated;  // m_Data created by frame, or supplied?
    wxString m_TitleFormat;  // title of the help frame
    // below are various pointers to GUI components
    wxHtmlWindow *m_HtmlWin;
    wxSplitterWindow *m_Splitter;
    wxPanel *m_NavigPan;
    wxNotebook *m_NavigNotebook;
    wxTreeCtrl *m_ContentsBox;
    wxTextCtrl *m_IndexText;
    wxButton *m_IndexButton;
    wxButton *m_IndexButtonAll;
    wxListBox *m_IndexList;
    wxTextCtrl *m_SearchText;
    wxButton *m_SearchButton;
    wxListBox *m_SearchList;
    wxChoice *m_SearchChoice;
    wxStaticText *m_IndexCountInfo;
    wxCheckBox *m_SearchCaseSensitive;
    wxCheckBox *m_SearchWholeWords;
    wxToolBar*  m_toolBar;

    wxComboBox *m_Bookmarks;
    wxArrayString m_BookmarksNames, m_BookmarksPages;

    wxHtmlHelpFrameCfg m_Cfg;

    wxConfigBase *m_Config;
    wxString m_ConfigRoot;

    // pagenumbers of controls in notebook (usually 0,1,2)
    int m_ContentsPage;
    int m_IndexPage;
    int m_SearchPage;

    // lists of available fonts (used in options dialog)
    wxArrayString *m_NormalFonts, *m_FixedFonts;
    int m_FontSize; // 0,1,2 = small,medium,big
    wxString m_NormalFace, m_FixedFace;

    bool m_UpdateContents;

#if wxUSE_PRINTING_ARCHITECTURE
    wxHtmlEasyPrinting *m_Printer;
#endif
    wxHashTable *m_PagesHash;
    wxHtmlHelpController* m_helpController;

    int m_hfStyle;

private:
    void DoIndexFind();
    void DoIndexAll();
    void DisplayIndexItem(const wxHtmlHelpMergedIndexItem *it);
    wxHtmlHelpMergedIndex *m_mergedIndex;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxHtmlHelpWindow)
};

/*!
 * Command IDs
 */

enum
{
    //wxID_HTML_HELPFRAME = wxID_HIGHEST + 1,
    wxID_HTML_PANEL = wxID_HIGHEST + 2,
    wxID_HTML_BACK,
    wxID_HTML_FORWARD,
    wxID_HTML_UPNODE,
    wxID_HTML_UP,
    wxID_HTML_DOWN,
    wxID_HTML_PRINT,
    wxID_HTML_OPENFILE,
    wxID_HTML_OPTIONS,
    wxID_HTML_BOOKMARKSLIST,
    wxID_HTML_BOOKMARKSADD,
    wxID_HTML_BOOKMARKSREMOVE,
    wxID_HTML_TREECTRL,
    wxID_HTML_INDEXPAGE,
    wxID_HTML_INDEXLIST,
    wxID_HTML_INDEXTEXT,
    wxID_HTML_INDEXBUTTON,
    wxID_HTML_INDEXBUTTONALL,
    wxID_HTML_NOTEBOOK,
    wxID_HTML_SEARCHPAGE,
    wxID_HTML_SEARCHTEXT,
    wxID_HTML_SEARCHLIST,
    wxID_HTML_SEARCHBUTTON,
    wxID_HTML_SEARCHCHOICE,
    wxID_HTML_COUNTINFO
};

#endif // wxUSE_WXHTML_HELP

#endif
