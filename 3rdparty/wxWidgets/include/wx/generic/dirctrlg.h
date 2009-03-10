/////////////////////////////////////////////////////////////////////////////
// Name:        dirctrlg.h
// Purpose:     wxGenericDirCtrl class
//              Builds on wxDirCtrl class written by Robert Roebling for the
//              wxFile application, modified by Harm van der Heijden.
//              Further modified for Windows.
// Author:      Robert Roebling, Harm van der Heijden, Julian Smart et al
// Modified by:
// Created:     21/3/2000
// RCS-ID:      $Id: dirctrlg.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Robert Roebling, Harm van der Heijden, Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIRCTRL_H_
#define _WX_DIRCTRL_H_

#if wxUSE_DIRDLG || wxUSE_FILEDLG
    #include "wx/imaglist.h"
#endif

#if wxUSE_DIRDLG

#include "wx/treectrl.h"
#include "wx/dialog.h"
#include "wx/dirdlg.h"
#include "wx/choice.h"

//-----------------------------------------------------------------------------
// classes
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxImageList;
class WXDLLIMPEXP_FWD_BASE wxHashTable;

//-----------------------------------------------------------------------------
// Extra styles for wxGenericDirCtrl
//-----------------------------------------------------------------------------

enum
{
    // Only allow directory viewing/selection, no files
    wxDIRCTRL_DIR_ONLY       = 0x0010,
    // When setting the default path, select the first file in the directory
    wxDIRCTRL_SELECT_FIRST   = 0x0020,
    // Show the filter list
    wxDIRCTRL_SHOW_FILTERS   = 0x0040,
    // Use 3D borders on internal controls
    wxDIRCTRL_3D_INTERNAL    = 0x0080,
    // Editable labels
    wxDIRCTRL_EDIT_LABELS    = 0x0100
};

//-----------------------------------------------------------------------------
// wxDirItemData
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxDirItemData : public wxTreeItemData
{
public:
    wxDirItemData(const wxString& path, const wxString& name, bool isDir);
    virtual ~wxDirItemData(){}
    void SetNewDirName(const wxString& path);

    bool HasSubDirs() const;
    bool HasFiles(const wxString& spec = wxEmptyString) const;

    wxString m_path, m_name;
    bool m_isHidden;
    bool m_isExpanded;
    bool m_isDir;
};

//-----------------------------------------------------------------------------
// wxDirCtrl
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxDirFilterListCtrl;

class WXDLLEXPORT wxGenericDirCtrl: public wxControl
{
public:
    wxGenericDirCtrl();
    wxGenericDirCtrl(wxWindow *parent, const wxWindowID id = wxID_ANY,
              const wxString &dir = wxDirDialogDefaultFolderStr,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxDIRCTRL_3D_INTERNAL|wxSUNKEN_BORDER,
              const wxString& filter = wxEmptyString,
              int defaultFilter = 0,
              const wxString& name = wxTreeCtrlNameStr )
    {
        Init();
        Create(parent, id, dir, pos, size, style, filter, defaultFilter, name);
    }

    bool Create(wxWindow *parent, const wxWindowID id = wxID_ANY,
              const wxString &dir = wxDirDialogDefaultFolderStr,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxDIRCTRL_3D_INTERNAL|wxSUNKEN_BORDER,
              const wxString& filter = wxEmptyString,
              int defaultFilter = 0,
              const wxString& name = wxTreeCtrlNameStr );

    virtual void Init();

    virtual ~wxGenericDirCtrl();

    void OnExpandItem(wxTreeEvent &event );
    void OnCollapseItem(wxTreeEvent &event );
    void OnBeginEditItem(wxTreeEvent &event );
    void OnEndEditItem(wxTreeEvent &event );
    void OnSize(wxSizeEvent &event );

    // Try to expand as much of the given path as possible.
    virtual bool ExpandPath(const wxString& path);
    // collapse the path
    virtual bool CollapsePath(const wxString& path);

    // Accessors

    virtual inline wxString GetDefaultPath() const { return m_defaultPath; }
    virtual void SetDefaultPath(const wxString& path) { m_defaultPath = path; }

    // Get dir or filename
    virtual wxString GetPath() const;

    // Get selected filename path only (else empty string).
    // I.e. don't count a directory as a selection
    virtual wxString GetFilePath() const;
    virtual void SetPath(const wxString& path);

    virtual void ShowHidden( bool show );
    virtual bool GetShowHidden() { return m_showHidden; }

    virtual wxString GetFilter() const { return m_filter; }
    virtual void SetFilter(const wxString& filter);

    virtual int GetFilterIndex() const { return m_currentFilter; }
    virtual void SetFilterIndex(int n);

    virtual wxTreeItemId GetRootId() { return m_rootId; }

    virtual wxTreeCtrl* GetTreeCtrl() const { return m_treeCtrl; }
    virtual wxDirFilterListCtrl* GetFilterListCtrl() const { return m_filterListCtrl; }

    // Helper
    virtual void SetupSections();

#if WXWIN_COMPATIBILITY_2_4
    // Parse the filter into an array of filters and an array of descriptions
    virtual int ParseFilter(const wxString& filterStr, wxArrayString& filters, wxArrayString& descriptions);
#endif // WXWIN_COMPATIBILITY_2_4

    // Find the child that matches the first part of 'path'.
    // E.g. if a child path is "/usr" and 'path' is "/usr/include"
    // then the child for /usr is returned.
    // If the path string has been used (we're at the leaf), done is set to true
    virtual wxTreeItemId FindChild(wxTreeItemId parentId, const wxString& path, bool& done);

    // Resize the components of the control
    virtual void DoResize();

    // Collapse & expand the tree, thus re-creating it from scratch:
    virtual void ReCreateTree();

    // Collapse the entire tree
    virtual void CollapseTree();

protected:
    virtual void ExpandRoot();
    virtual void ExpandDir(wxTreeItemId parentId);
    virtual void CollapseDir(wxTreeItemId parentId);
    virtual const wxTreeItemId AddSection(const wxString& path, const wxString& name, int imageId = 0);
    virtual wxTreeItemId AppendItem (const wxTreeItemId & parent,
                const wxString & text,
                int image = -1, int selectedImage = -1,
                wxTreeItemData * data = NULL);
    //void FindChildFiles(wxTreeItemId id, int dirFlags, wxArrayString& filenames);
    virtual wxTreeCtrl* CreateTreeCtrl(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long treeStyle);

    // Extract description and actual filter from overall filter string
    bool ExtractWildcard(const wxString& filterStr, int n, wxString& filter, wxString& description);

private:
    bool            m_showHidden;
    wxTreeItemId    m_rootId;
    wxString        m_defaultPath; // Starting path
    long            m_styleEx; // Extended style
    wxString        m_filter;  // Wildcards in same format as per wxFileDialog
    int             m_currentFilter; // The current filter index
    wxString        m_currentFilterStr; // Current filter string
    wxTreeCtrl*     m_treeCtrl;
    wxDirFilterListCtrl* m_filterListCtrl;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGenericDirCtrl)
    DECLARE_NO_COPY_CLASS(wxGenericDirCtrl)
};

//-----------------------------------------------------------------------------
// wxDirFilterListCtrl
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxDirFilterListCtrl: public wxChoice
{
public:
    wxDirFilterListCtrl() { Init(); }
    wxDirFilterListCtrl(wxGenericDirCtrl* parent, const wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = 0)
    {
        Init();
        Create(parent, id, pos, size, style);
    }

    bool Create(wxGenericDirCtrl* parent, const wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = 0);

    void Init();

    virtual ~wxDirFilterListCtrl() {}

    //// Operations
    void FillFilterList(const wxString& filter, int defaultFilter);

    //// Events
    void OnSelFilter(wxCommandEvent& event);

protected:
    wxGenericDirCtrl*    m_dirCtrl;

    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(wxDirFilterListCtrl)
    DECLARE_NO_COPY_CLASS(wxDirFilterListCtrl)
};

#if !defined(__WXMSW__) && !defined(__WXMAC__) && !defined(__WXPM__)
    #define wxDirCtrl wxGenericDirCtrl
#endif

// Symbols for accessing individual controls
#define wxID_TREECTRL          7000
#define wxID_FILTERLISTCTRL    7001

#endif // wxUSE_DIRDLG

//-------------------------------------------------------------------------
// wxFileIconsTable - use wxTheFileIconsTable which is created as necessary
//-------------------------------------------------------------------------

#if wxUSE_DIRDLG || wxUSE_FILEDLG

class WXDLLEXPORT wxFileIconsTable
{
public:
    wxFileIconsTable();
    ~wxFileIconsTable();

    enum iconId_Type
    {
        folder,
        folder_open,
        computer,
        drive,
        cdrom,
        floppy,
        removeable,
        file,
        executable
    };

    int GetIconID(const wxString& extension, const wxString& mime = wxEmptyString);
    wxImageList *GetSmallImageList();

protected:
    void Create();  // create on first use

    wxImageList *m_smallImageList;
    wxHashTable *m_HashTable;
};

// The global fileicons table
extern WXDLLEXPORT_DATA(wxFileIconsTable *) wxTheFileIconsTable;

#endif // wxUSE_DIRDLG || wxUSE_FILEDLG

#endif
    // _WX_DIRCTRLG_H_
