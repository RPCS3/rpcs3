/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/treectrl.cpp
// Purpose:     wxTreeCtrl
// Author:      Julian Smart
// Modified by: Vadim Zeitlin to be less MSW-specific on 10.10.98
// Created:     1997
// RCS-ID:      $Id: treectrl.cpp 53084 2008-04-07 20:12:57Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TREECTRL

#include "wx/treectrl.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/msw/missing.h"
    #include "wx/dynarray.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/settings.h"
#endif

#include "wx/msw/private.h"

// Set this to 1 to be _absolutely_ sure that repainting will work for all
// comctl32.dll versions
#define wxUSE_COMCTL32_SAFELY 0

#include "wx/imaglist.h"
#include "wx/msw/dragimag.h"

// macros to hide the cast ugliness
// --------------------------------

// get HTREEITEM from wxTreeItemId
#define HITEM(item)     ((HTREEITEM)(((item).m_pItem)))


// older SDKs are missing these
#ifndef TVN_ITEMCHANGINGA

#define TVN_ITEMCHANGINGA (TVN_FIRST-16)
#define TVN_ITEMCHANGINGW (TVN_FIRST-17)

typedef struct tagNMTVITEMCHANGE
{
    NMHDR hdr;
    UINT uChanged;
    HTREEITEM hItem;
    UINT uStateNew;
    UINT uStateOld;
    LPARAM lParam;
} NMTVITEMCHANGE;

#endif


// this helper class is used on vista systems for preventing unwanted
// item state changes in the vista tree control.  It is only effective in
// multi-select mode on vista systems.

// The vista tree control includes some new code that originally broke the
// multi-selection tree, causing seemingly spurious item selection state changes
// during Shift or Ctrl-click item selection. (To witness the original broken
// behavior, simply make IsLocked() below always return false). This problem was
// solved by using the following class to 'unlock' an item's selection state.

class TreeItemUnlocker
{
public:
    // unlock a single item
    TreeItemUnlocker(HTREEITEM item) { ms_unlockedItem = item; }

    // unlock all items, don't use unless absolutely necessary
    TreeItemUnlocker() { ms_unlockedItem = (HTREEITEM)-1; }

    // lock everything back
    ~TreeItemUnlocker() { ms_unlockedItem = NULL; }


    // check if the item state is currently locked
    static bool IsLocked(HTREEITEM item)
        { return ms_unlockedItem != (HTREEITEM)-1 && item != ms_unlockedItem; }

private:
    static HTREEITEM ms_unlockedItem;
};

HTREEITEM TreeItemUnlocker::ms_unlockedItem = NULL;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// wrappers for TreeView_GetItem/TreeView_SetItem
static bool IsItemSelected(HWND hwndTV, HTREEITEM hItem)
{

    TV_ITEM tvi;
    tvi.mask = TVIF_STATE | TVIF_HANDLE;
    tvi.stateMask = TVIS_SELECTED;
    tvi.hItem = hItem;

    TreeItemUnlocker unlocker(hItem);

    if ( !TreeView_GetItem(hwndTV, &tvi) )
    {
        wxLogLastError(wxT("TreeView_GetItem"));
    }

    return (tvi.state & TVIS_SELECTED) != 0;
}

static bool SelectItem(HWND hwndTV, HTREEITEM hItem, bool select = true)
{
    TV_ITEM tvi;
    tvi.mask = TVIF_STATE | TVIF_HANDLE;
    tvi.stateMask = TVIS_SELECTED;
    tvi.state = select ? TVIS_SELECTED : 0;
    tvi.hItem = hItem;

    TreeItemUnlocker unlocker(hItem);

    if ( TreeView_SetItem(hwndTV, &tvi) == -1 )
    {
        wxLogLastError(wxT("TreeView_SetItem"));
        return false;
    }

    return true;
}

static inline void UnselectItem(HWND hwndTV, HTREEITEM htItem)
{
    SelectItem(hwndTV, htItem, false);
}

static inline void ToggleItemSelection(HWND hwndTV, HTREEITEM htItem)
{
    SelectItem(hwndTV, htItem, !IsItemSelected(hwndTV, htItem));
}

// helper function which selects all items in a range and, optionally,
// unselects all others
static void SelectRange(HWND hwndTV,
                        HTREEITEM htFirst,
                        HTREEITEM htLast,
                        bool unselectOthers = true)
{
    // find the first (or last) item and select it
    bool cont = true;
    HTREEITEM htItem = (HTREEITEM)TreeView_GetRoot(hwndTV);
    while ( htItem && cont )
    {
        if ( (htItem == htFirst) || (htItem == htLast) )
        {
            if ( !IsItemSelected(hwndTV, htItem) )
            {
                SelectItem(hwndTV, htItem);
            }

            cont = false;
        }
        else
        {
            if ( unselectOthers && IsItemSelected(hwndTV, htItem) )
            {
                UnselectItem(hwndTV, htItem);
            }
        }

        htItem = (HTREEITEM)TreeView_GetNextVisible(hwndTV, htItem);
    }

    // select the items in range
    cont = htFirst != htLast;
    while ( htItem && cont )
    {
        if ( !IsItemSelected(hwndTV, htItem) )
        {
            SelectItem(hwndTV, htItem);
        }

        cont = (htItem != htFirst) && (htItem != htLast);

        htItem = (HTREEITEM)TreeView_GetNextVisible(hwndTV, htItem);
    }

    // unselect the rest
    if ( unselectOthers )
    {
        while ( htItem )
        {
            if ( IsItemSelected(hwndTV, htItem) )
            {
                UnselectItem(hwndTV, htItem);
            }

            htItem = (HTREEITEM)TreeView_GetNextVisible(hwndTV, htItem);
        }
    }

    // seems to be necessary - otherwise the just selected items don't always
    // appear as selected
    UpdateWindow(hwndTV);
}

// helper function which tricks the standard control into changing the focused
// item without changing anything else (if someone knows why Microsoft doesn't
// allow to do it by just setting TVIS_FOCUSED flag, please tell me!)
static void SetFocus(HWND hwndTV, HTREEITEM htItem)
{
    // the current focus
    HTREEITEM htFocus = (HTREEITEM)TreeView_GetSelection(hwndTV);

    if ( htItem )
    {
        // set the focus
        if ( htItem != htFocus )
        {
            // remember the selection state of the item
            bool wasSelected = IsItemSelected(hwndTV, htItem);

            if ( htFocus && IsItemSelected(hwndTV, htFocus) )
            {
                // prevent the tree from unselecting the old focus which it
                // would do by default (TreeView_SelectItem unselects the
                // focused item)
                TreeView_SelectItem(hwndTV, 0);
                SelectItem(hwndTV, htFocus);
            }

            TreeView_SelectItem(hwndTV, htItem);

            if ( !wasSelected )
            {
                // need to clear the selection which TreeView_SelectItem() gave
                // us
                UnselectItem(hwndTV, htItem);
            }
            //else: was selected, still selected - ok
        }
        //else: nothing to do, focus already there
    }
    else
    {
        if ( htFocus )
        {
            bool wasFocusSelected = IsItemSelected(hwndTV, htFocus);

            // just clear the focus
            TreeView_SelectItem(hwndTV, 0);

            if ( wasFocusSelected )
            {
                // restore the selection state
                SelectItem(hwndTV, htFocus);
            }
        }
        //else: nothing to do, no focus already
    }
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// a convenient wrapper around TV_ITEM struct which adds a ctor
#ifdef __VISUALC__
#pragma warning( disable : 4097 ) // inheriting from typedef
#endif

struct wxTreeViewItem : public TV_ITEM
{
    wxTreeViewItem(const wxTreeItemId& item,    // the item handle
                   UINT mask_,                  // fields which are valid
                   UINT stateMask_ = 0)         // for TVIF_STATE only
    {
        wxZeroMemory(*this);

        // hItem member is always valid
        mask = mask_ | TVIF_HANDLE;
        stateMask = stateMask_;
        hItem = HITEM(item);
    }
};

// ----------------------------------------------------------------------------
// This class is our userdata/lParam for the TV_ITEMs stored in the treeview.
//
// We need this for a couple of reasons:
//
// 1) This class is needed for support of different images: the Win32 common
// control natively supports only 2 images (the normal one and another for the
// selected state). We wish to provide support for 2 more of them for folder
// items (i.e. those which have children): for expanded state and for expanded
// selected state. For this we use this structure to store the additional items
// images.
//
// 2) This class is also needed to hold the HITEM so that we can sort
// it correctly in the MSW sort callback.
//
// In addition it makes other workarounds such as this easier and helps
// simplify the code.
// ----------------------------------------------------------------------------

class wxTreeItemParam
{
public:
    wxTreeItemParam()
    {
        m_data = NULL;

        for ( size_t n = 0; n < WXSIZEOF(m_images); n++ )
        {
            m_images[n] = -1;
        }
    }

    // dtor deletes the associated data as well
    virtual ~wxTreeItemParam() { delete m_data; }

    // accessors
        // get the real data associated with the item
    wxTreeItemData *GetData() const { return m_data; }
        // change it
    void SetData(wxTreeItemData *data) { m_data = data; }

        // do we have such image?
    bool HasImage(wxTreeItemIcon which) const { return m_images[which] != -1; }
        // get image, falling back to the other images if this one is not
        // specified
    int GetImage(wxTreeItemIcon which) const
    {
        int image = m_images[which];
        if ( image == -1 )
        {
            switch ( which )
            {
                case wxTreeItemIcon_SelectedExpanded:
                    image = GetImage(wxTreeItemIcon_Expanded);
                    if ( image != -1 )
                        break;
                    //else: fall through

                case wxTreeItemIcon_Selected:
                case wxTreeItemIcon_Expanded:
                    image = GetImage(wxTreeItemIcon_Normal);
                    break;

                case wxTreeItemIcon_Normal:
                    // no fallback
                    break;

                default:
                    wxFAIL_MSG( _T("unsupported wxTreeItemIcon value") );
            }
        }

        return image;
    }
        // change the given image
    void SetImage(int image, wxTreeItemIcon which) { m_images[which] = image; }

        // get item
    const wxTreeItemId& GetItem() const { return m_item; }
        // set item
    void SetItem(const wxTreeItemId& item) { m_item = item; }

protected:
    // all the images associated with the item
    int m_images[wxTreeItemIcon_Max];

    // item for sort callbacks
    wxTreeItemId m_item;

    // the real client data
    wxTreeItemData *m_data;

    DECLARE_NO_COPY_CLASS(wxTreeItemParam)
};

// wxVirutalNode is used in place of a single root when 'hidden' root is
// specified.
class wxVirtualNode : public wxTreeViewItem
{
public:
    wxVirtualNode(wxTreeItemParam *param)
        : wxTreeViewItem(TVI_ROOT, 0)
    {
        m_param = param;
    }

    ~wxVirtualNode()
    {
        delete m_param;
    }

    wxTreeItemParam *GetParam() const { return m_param; }
    void SetParam(wxTreeItemParam *param) { delete m_param; m_param = param; }

private:
    wxTreeItemParam *m_param;

    DECLARE_NO_COPY_CLASS(wxVirtualNode)
};

#ifdef __VISUALC__
#pragma warning( default : 4097 )
#endif

// a macro to get the virtual root, returns NULL if none
#define GET_VIRTUAL_ROOT() ((wxVirtualNode *)m_pVirtualRoot)

// returns true if the item is the virtual root
#define IS_VIRTUAL_ROOT(item) (HITEM(item) == TVI_ROOT)

// a class which encapsulates the tree traversal logic: it vists all (unless
// OnVisit() returns false) items under the given one
class wxTreeTraversal
{
public:
    wxTreeTraversal(const wxTreeCtrl *tree)
    {
        m_tree = tree;
    }

    // give it a virtual dtor: not really needed as the class is never used
    // polymorphically and not even allocated on heap at all, but this is safer
    // (in case it ever is) and silences the compiler warnings for now
    virtual ~wxTreeTraversal() { }

    // do traverse the tree: visit all items (recursively by default) under the
    // given one; return true if all items were traversed or false if the
    // traversal was aborted because OnVisit returned false
    bool DoTraverse(const wxTreeItemId& root, bool recursively = true);

    // override this function to do whatever is needed for each item, return
    // false to stop traversing
    virtual bool OnVisit(const wxTreeItemId& item) = 0;

protected:
    const wxTreeCtrl *GetTree() const { return m_tree; }

private:
    bool Traverse(const wxTreeItemId& root, bool recursively);

    const wxTreeCtrl *m_tree;

    DECLARE_NO_COPY_CLASS(wxTreeTraversal)
};

// internal class for getting the selected items
class TraverseSelections : public wxTreeTraversal
{
public:
    TraverseSelections(const wxTreeCtrl *tree,
                       wxArrayTreeItemIds& selections)
        : wxTreeTraversal(tree), m_selections(selections)
        {
            m_selections.Empty();

            if (tree->GetCount() > 0)
                DoTraverse(tree->GetRootItem());
        }

    virtual bool OnVisit(const wxTreeItemId& item)
    {
        const wxTreeCtrl * const tree = GetTree();

        // can't visit a virtual node.
        if ( (tree->GetRootItem() == item) && tree->HasFlag(wxTR_HIDE_ROOT) )
        {
            return true;
        }

        if ( ::IsItemSelected(GetHwndOf(tree), HITEM(item)) )
        {
            m_selections.Add(item);
        }

        return true;
    }

    size_t GetCount() const { return m_selections.GetCount(); }

private:
    wxArrayTreeItemIds& m_selections;

    DECLARE_NO_COPY_CLASS(TraverseSelections)
};

// internal class for counting tree items
class TraverseCounter : public wxTreeTraversal
{
public:
    TraverseCounter(const wxTreeCtrl *tree,
                    const wxTreeItemId& root,
                    bool recursively)
        : wxTreeTraversal(tree)
        {
            m_count = 0;

            DoTraverse(root, recursively);
        }

    virtual bool OnVisit(const wxTreeItemId& WXUNUSED(item))
    {
        m_count++;

        return true;
    }

    size_t GetCount() const { return m_count; }

private:
    size_t m_count;

    DECLARE_NO_COPY_CLASS(TraverseCounter)
};

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxTreeCtrlStyle )

wxBEGIN_FLAGS( wxTreeCtrlStyle )
    // new style border flags, we put them first to
    // use them for streaming out
    wxFLAGS_MEMBER(wxBORDER_SIMPLE)
    wxFLAGS_MEMBER(wxBORDER_SUNKEN)
    wxFLAGS_MEMBER(wxBORDER_DOUBLE)
    wxFLAGS_MEMBER(wxBORDER_RAISED)
    wxFLAGS_MEMBER(wxBORDER_STATIC)
    wxFLAGS_MEMBER(wxBORDER_NONE)

    // old style border flags
    wxFLAGS_MEMBER(wxSIMPLE_BORDER)
    wxFLAGS_MEMBER(wxSUNKEN_BORDER)
    wxFLAGS_MEMBER(wxDOUBLE_BORDER)
    wxFLAGS_MEMBER(wxRAISED_BORDER)
    wxFLAGS_MEMBER(wxSTATIC_BORDER)
    wxFLAGS_MEMBER(wxBORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxFULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB )
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

    wxFLAGS_MEMBER(wxTR_EDIT_LABELS)
    wxFLAGS_MEMBER(wxTR_NO_BUTTONS)
    wxFLAGS_MEMBER(wxTR_HAS_BUTTONS)
    wxFLAGS_MEMBER(wxTR_TWIST_BUTTONS)
    wxFLAGS_MEMBER(wxTR_NO_LINES)
    wxFLAGS_MEMBER(wxTR_FULL_ROW_HIGHLIGHT)
    wxFLAGS_MEMBER(wxTR_LINES_AT_ROOT)
    wxFLAGS_MEMBER(wxTR_HIDE_ROOT)
    wxFLAGS_MEMBER(wxTR_ROW_LINES)
    wxFLAGS_MEMBER(wxTR_HAS_VARIABLE_ROW_HEIGHT)
    wxFLAGS_MEMBER(wxTR_SINGLE)
    wxFLAGS_MEMBER(wxTR_MULTIPLE)
    wxFLAGS_MEMBER(wxTR_EXTENDED)
    wxFLAGS_MEMBER(wxTR_DEFAULT_STYLE)

wxEND_FLAGS( wxTreeCtrlStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxTreeCtrl, wxControl,"wx/treectrl.h")

wxBEGIN_PROPERTIES_TABLE(wxTreeCtrl)
    wxEVENT_PROPERTY( TextUpdated , wxEVT_COMMAND_TEXT_UPDATED , wxCommandEvent )
    wxEVENT_RANGE_PROPERTY( TreeEvent , wxEVT_COMMAND_TREE_BEGIN_DRAG , wxEVT_COMMAND_TREE_STATE_IMAGE_CLICK , wxTreeEvent )
    wxPROPERTY_FLAGS( WindowStyle , wxTreeCtrlStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxTreeCtrl)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_5( wxTreeCtrl , wxWindow* , Parent , wxWindowID , Id , wxPoint , Position , wxSize , Size , long , WindowStyle )
#else
IMPLEMENT_DYNAMIC_CLASS(wxTreeCtrl, wxControl)
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// indices in gs_expandEvents table below
enum
{
    IDX_COLLAPSE,
    IDX_EXPAND,
    IDX_WHAT_MAX
};

enum
{
    IDX_DONE,
    IDX_DOING,
    IDX_HOW_MAX
};

// handy table for sending events - it has to be initialized during run-time
// now so can't be const any more
static /* const */ wxEventType gs_expandEvents[IDX_WHAT_MAX][IDX_HOW_MAX];

/*
   but logically it's a const table with the following entries:
=
{
    { wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxEVT_COMMAND_TREE_ITEM_COLLAPSING },
    { wxEVT_COMMAND_TREE_ITEM_EXPANDED,  wxEVT_COMMAND_TREE_ITEM_EXPANDING  }
};
*/

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// tree traversal
// ----------------------------------------------------------------------------

bool wxTreeTraversal::DoTraverse(const wxTreeItemId& root, bool recursively)
{
    if ( !OnVisit(root) )
        return false;

    return Traverse(root, recursively);
}

bool wxTreeTraversal::Traverse(const wxTreeItemId& root, bool recursively)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(root, cookie);
    while ( child.IsOk() )
    {
        // depth first traversal
        if ( recursively && !Traverse(child, true) )
            return false;

        if ( !OnVisit(child) )
            return false;

        child = m_tree->GetNextChild(root, cookie);
    }

    return true;
}

// ----------------------------------------------------------------------------
// construction and destruction
// ----------------------------------------------------------------------------

void wxTreeCtrl::Init()
{
    m_textCtrl = NULL;
    m_hasAnyAttr = false;
#if wxUSE_DRAGIMAGE
    m_dragImage = NULL;
#endif
    m_pVirtualRoot = NULL;

    // initialize the global array of events now as it can't be done statically
    // with the wxEVT_XXX values being allocated during run-time only
    gs_expandEvents[IDX_COLLAPSE][IDX_DONE] = wxEVT_COMMAND_TREE_ITEM_COLLAPSED;
    gs_expandEvents[IDX_COLLAPSE][IDX_DOING] = wxEVT_COMMAND_TREE_ITEM_COLLAPSING;
    gs_expandEvents[IDX_EXPAND][IDX_DONE] = wxEVT_COMMAND_TREE_ITEM_EXPANDED;
    gs_expandEvents[IDX_EXPAND][IDX_DOING] = wxEVT_COMMAND_TREE_ITEM_EXPANDING;
}

bool wxTreeCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxValidator& validator,
                        const wxString& name)
{
    Init();

    if ( (style & wxBORDER_MASK) == wxBORDER_DEFAULT )
        style |= wxBORDER_SUNKEN;

    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    WXDWORD exStyle = 0;
    DWORD wstyle = MSWGetStyle(m_windowStyle, & exStyle);
    wstyle |= WS_TABSTOP | TVS_SHOWSELALWAYS;

    if ((m_windowStyle & wxTR_NO_LINES) == 0)
        wstyle |= TVS_HASLINES;
    if ( m_windowStyle & wxTR_HAS_BUTTONS )
        wstyle |= TVS_HASBUTTONS;

    if ( m_windowStyle & wxTR_EDIT_LABELS )
        wstyle |= TVS_EDITLABELS;

    if ( m_windowStyle & wxTR_LINES_AT_ROOT )
        wstyle |= TVS_LINESATROOT;

    if ( m_windowStyle & wxTR_FULL_ROW_HIGHLIGHT )
    {
        if ( wxApp::GetComCtl32Version() >= 471 )
            wstyle |= TVS_FULLROWSELECT;
    }

#if !defined(__WXWINCE__) && defined(TVS_INFOTIP)
    // Need so that TVN_GETINFOTIP messages will be sent
    wstyle |= TVS_INFOTIP;
#endif

    // Create the tree control.
    if ( !MSWCreateControl(WC_TREEVIEW, wstyle, pos, size) )
        return false;

#if wxUSE_COMCTL32_SAFELY
    wxWindow::SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    wxWindow::SetForegroundColour(wxWindow::GetParent()->GetForegroundColour());
#elif 1
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetForegroundColour(wxWindow::GetParent()->GetForegroundColour());
#else
    // This works around a bug in the Windows tree control whereby for some versions
    // of comctrl32, setting any colour actually draws the background in black.
    // This will initialise the background to the system colour.
    // THIS FIX NOW REVERTED since it caused problems on _other_ systems.
    // Assume the user has an updated comctl32.dll.
    ::SendMessage(GetHwnd(), TVM_SETBKCOLOR, 0,-1);
    wxWindow::SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetForegroundColour(wxWindow::GetParent()->GetForegroundColour());
#endif


    // VZ: this is some experimental code which may be used to get the
    //     TVS_CHECKBOXES style functionality for comctl32.dll < 4.71.
    //     AFAIK, the standard DLL does about the same thing anyhow.
#if 0
    if ( m_windowStyle & wxTR_MULTIPLE )
    {
        wxBitmap bmp;

        // create the DC compatible with the current screen
        HDC hdcMem = CreateCompatibleDC(NULL);

        // create a mono bitmap of the standard size
        int x = ::GetSystemMetrics(SM_CXMENUCHECK);
        int y = ::GetSystemMetrics(SM_CYMENUCHECK);
        wxImageList imagelistCheckboxes(x, y, false, 2);
        HBITMAP hbmpCheck = CreateBitmap(x, y,   // bitmap size
                                         1,      // # of color planes
                                         1,      // # bits needed for one pixel
                                         0);     // array containing colour data
        SelectObject(hdcMem, hbmpCheck);

        // then draw a check mark into it
        RECT rect = { 0, 0, x, y };
        if ( !::DrawFrameControl(hdcMem, &rect,
                                 DFC_BUTTON,
                                 DFCS_BUTTONCHECK | DFCS_CHECKED) )
        {
            wxLogLastError(wxT("DrawFrameControl(check)"));
        }

        bmp.SetHBITMAP((WXHBITMAP)hbmpCheck);
        imagelistCheckboxes.Add(bmp);

        if ( !::DrawFrameControl(hdcMem, &rect,
                                 DFC_BUTTON,
                                 DFCS_BUTTONCHECK) )
        {
            wxLogLastError(wxT("DrawFrameControl(uncheck)"));
        }

        bmp.SetHBITMAP((WXHBITMAP)hbmpCheck);
        imagelistCheckboxes.Add(bmp);

        // clean up
        ::DeleteDC(hdcMem);

        // set the imagelist
        SetStateImageList(&imagelistCheckboxes);
    }
#endif // 0

    wxSetCCUnicodeFormat(GetHwnd());

    return true;
}

wxTreeCtrl::~wxTreeCtrl()
{
    // delete any attributes
    if ( m_hasAnyAttr )
    {
        WX_CLEAR_HASH_MAP(wxMapTreeAttr, m_attrs);

        // prevent TVN_DELETEITEM handler from deleting the attributes again!
        m_hasAnyAttr = false;
    }

    DeleteTextCtrl();

    // delete user data to prevent memory leaks
    // also deletes hidden root node storage.
    DeleteAllItems();
}

// ----------------------------------------------------------------------------
// accessors
// ----------------------------------------------------------------------------

/* static */ wxVisualAttributes
wxTreeCtrl::GetClassDefaultAttributes(wxWindowVariant variant)
{
    wxVisualAttributes attrs = GetCompositeControlsDefaultAttributes(variant);

    // common controls have their own default font
    attrs.font = wxGetCCDefaultFont();

    return attrs;
}


// simple wrappers which add error checking in debug mode

bool wxTreeCtrl::DoGetItem(wxTreeViewItem *tvItem) const
{
    wxCHECK_MSG( tvItem->hItem != TVI_ROOT, false,
                 _T("can't retrieve virtual root item") );

    if ( !TreeView_GetItem(GetHwnd(), tvItem) )
    {
        wxLogLastError(wxT("TreeView_GetItem"));

        return false;
    }

    return true;
}

void wxTreeCtrl::DoSetItem(wxTreeViewItem *tvItem)
{
    TreeItemUnlocker unlocker(tvItem->hItem);

    if ( TreeView_SetItem(GetHwnd(), tvItem) == -1 )
    {
        wxLogLastError(wxT("TreeView_SetItem"));
    }
}

unsigned int wxTreeCtrl::GetCount() const
{
    return (unsigned int)TreeView_GetCount(GetHwnd());
}

unsigned int wxTreeCtrl::GetIndent() const
{
    return TreeView_GetIndent(GetHwnd());
}

void wxTreeCtrl::SetIndent(unsigned int indent)
{
    TreeView_SetIndent(GetHwnd(), indent);
}

void wxTreeCtrl::SetAnyImageList(wxImageList *imageList, int which)
{
    // no error return
    (void) TreeView_SetImageList(GetHwnd(),
                                 imageList ? imageList->GetHIMAGELIST() : 0,
                                 which);
}

void wxTreeCtrl::SetImageList(wxImageList *imageList)
{
    if (m_ownsImageListNormal)
        delete m_imageListNormal;

    SetAnyImageList(m_imageListNormal = imageList, TVSIL_NORMAL);
    m_ownsImageListNormal = false;
}

void wxTreeCtrl::SetStateImageList(wxImageList *imageList)
{
    if (m_ownsImageListState) delete m_imageListState;
    SetAnyImageList(m_imageListState = imageList, TVSIL_STATE);
    m_ownsImageListState = false;
}

size_t wxTreeCtrl::GetChildrenCount(const wxTreeItemId& item,
                                    bool recursively) const
{
    wxCHECK_MSG( item.IsOk(), 0u, wxT("invalid tree item") );

    TraverseCounter counter(this, item, recursively);
    return counter.GetCount() - 1;
}

// ----------------------------------------------------------------------------
// control colours
// ----------------------------------------------------------------------------

bool wxTreeCtrl::SetBackgroundColour(const wxColour &colour)
{
#if !wxUSE_COMCTL32_SAFELY
    if ( !wxWindowBase::SetBackgroundColour(colour) )
        return false;

    ::SendMessage(GetHwnd(), TVM_SETBKCOLOR, 0, colour.GetPixel());
#endif

    return true;
}

bool wxTreeCtrl::SetForegroundColour(const wxColour &colour)
{
#if !wxUSE_COMCTL32_SAFELY
    if ( !wxWindowBase::SetForegroundColour(colour) )
        return false;

    ::SendMessage(GetHwnd(), TVM_SETTEXTCOLOR, 0, colour.GetPixel());
#endif

    return true;
}

// ----------------------------------------------------------------------------
// Item access
// ----------------------------------------------------------------------------

bool wxTreeCtrl::IsHiddenRoot(const wxTreeItemId& item) const
{
    return HITEM(item) == TVI_ROOT && HasFlag(wxTR_HIDE_ROOT);
}

wxString wxTreeCtrl::GetItemText(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxEmptyString, wxT("invalid tree item") );

    wxChar buf[512];  // the size is arbitrary...

    wxTreeViewItem tvItem(item, TVIF_TEXT);
    tvItem.pszText = buf;
    tvItem.cchTextMax = WXSIZEOF(buf);
    if ( !DoGetItem(&tvItem) )
    {
        // don't return some garbage which was on stack, but an empty string
        buf[0] = wxT('\0');
    }

    return wxString(buf);
}

void wxTreeCtrl::SetItemText(const wxTreeItemId& item, const wxString& text)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    if ( IS_VIRTUAL_ROOT(item) )
        return;

    wxTreeViewItem tvItem(item, TVIF_TEXT);
    tvItem.pszText = (wxChar *)text.c_str();  // conversion is ok
    DoSetItem(&tvItem);

    // when setting the text of the item being edited, the text control should
    // be updated to reflect the new text as well, otherwise calling
    // SetItemText() in the OnBeginLabelEdit() handler doesn't have any effect
    //
    // don't use GetEditControl() here because m_textCtrl is not set yet
    HWND hwndEdit = TreeView_GetEditControl(GetHwnd());
    if ( hwndEdit )
    {
        if ( item == m_idEdited )
        {
            ::SetWindowText(hwndEdit, text);
        }
    }
}

int wxTreeCtrl::GetItemImage(const wxTreeItemId& item,
                             wxTreeItemIcon which) const
{
    wxCHECK_MSG( item.IsOk(), -1, wxT("invalid tree item") );

    if ( IsHiddenRoot(item) )
    {
        // no images for hidden root item
        return -1;
    }

    wxTreeItemParam *param = GetItemParam(item);

    return param && param->HasImage(which) ? param->GetImage(which) : -1;
}

void wxTreeCtrl::SetItemImage(const wxTreeItemId& item, int image,
                              wxTreeItemIcon which)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );
    wxCHECK_RET( which >= 0 &&
                 which < wxTreeItemIcon_Max,
                 wxT("invalid image index"));


    if ( IsHiddenRoot(item) )
    {
        // no images for hidden root item
        return;
    }

    wxTreeItemParam *data = GetItemParam(item);
    if ( !data )
        return;

    data->SetImage(image, which);

    RefreshItem(item);
}

wxTreeItemParam *wxTreeCtrl::GetItemParam(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), NULL, wxT("invalid tree item") );

    wxTreeViewItem tvItem(item, TVIF_PARAM);

    // hidden root may still have data.
    if ( IS_VIRTUAL_ROOT(item) )
    {
        return GET_VIRTUAL_ROOT()->GetParam();
    }

    // visible node.
    if ( !DoGetItem(&tvItem) )
    {
        return NULL;
    }

    return (wxTreeItemParam *)tvItem.lParam;
}

wxTreeItemData *wxTreeCtrl::GetItemData(const wxTreeItemId& item) const
{
    wxTreeItemParam *data = GetItemParam(item);

    return data ? data->GetData() : NULL;
}

void wxTreeCtrl::SetItemData(const wxTreeItemId& item, wxTreeItemData *data)
{
    // first, associate this piece of data with this item
    if ( data )
    {
        data->SetId(item);
    }

    wxTreeItemParam *param = GetItemParam(item);

    wxCHECK_RET( param, wxT("failed to change tree items data") );

    param->SetData(data);
}

void wxTreeCtrl::SetItemHasChildren(const wxTreeItemId& item, bool has)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    if ( IS_VIRTUAL_ROOT(item) )
        return;

    wxTreeViewItem tvItem(item, TVIF_CHILDREN);
    tvItem.cChildren = (int)has;
    DoSetItem(&tvItem);
}

void wxTreeCtrl::SetItemBold(const wxTreeItemId& item, bool bold)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    if ( IS_VIRTUAL_ROOT(item) )
        return;

    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_BOLD);
    tvItem.state = bold ? TVIS_BOLD : 0;
    DoSetItem(&tvItem);
}

void wxTreeCtrl::SetItemDropHighlight(const wxTreeItemId& item, bool highlight)
{
    if ( IS_VIRTUAL_ROOT(item) )
        return;

    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_DROPHILITED);
    tvItem.state = highlight ? TVIS_DROPHILITED : 0;
    DoSetItem(&tvItem);
}

void wxTreeCtrl::RefreshItem(const wxTreeItemId& item)
{
    if ( IS_VIRTUAL_ROOT(item) )
        return;

    wxRect rect;
    if ( GetBoundingRect(item, rect) )
    {
        RefreshRect(rect);
    }
}

wxColour wxTreeCtrl::GetItemTextColour(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxNullColour, wxT("invalid tree item") );

    wxMapTreeAttr::const_iterator it = m_attrs.find(item.m_pItem);
    return it == m_attrs.end() ? wxNullColour : it->second->GetTextColour();
}

wxColour wxTreeCtrl::GetItemBackgroundColour(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxNullColour, wxT("invalid tree item") );

    wxMapTreeAttr::const_iterator it = m_attrs.find(item.m_pItem);
    return it == m_attrs.end() ? wxNullColour : it->second->GetBackgroundColour();
}

wxFont wxTreeCtrl::GetItemFont(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxNullFont, wxT("invalid tree item") );

    wxMapTreeAttr::const_iterator it = m_attrs.find(item.m_pItem);
    return it == m_attrs.end() ? wxNullFont : it->second->GetFont();
}

void wxTreeCtrl::SetItemTextColour(const wxTreeItemId& item,
                                   const wxColour& col)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    wxTreeItemAttr *attr;
    wxMapTreeAttr::iterator it = m_attrs.find(item.m_pItem);
    if ( it == m_attrs.end() )
    {
        m_hasAnyAttr = true;

        m_attrs[item.m_pItem] =
        attr = new wxTreeItemAttr;
    }
    else
    {
        attr = it->second;
    }

    attr->SetTextColour(col);

    RefreshItem(item);
}

void wxTreeCtrl::SetItemBackgroundColour(const wxTreeItemId& item,
                                         const wxColour& col)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    wxTreeItemAttr *attr;
    wxMapTreeAttr::iterator it = m_attrs.find(item.m_pItem);
    if ( it == m_attrs.end() )
    {
        m_hasAnyAttr = true;

        m_attrs[item.m_pItem] =
        attr = new wxTreeItemAttr;
    }
    else // already in the hash
    {
        attr = it->second;
    }

    attr->SetBackgroundColour(col);

    RefreshItem(item);
}

void wxTreeCtrl::SetItemFont(const wxTreeItemId& item, const wxFont& font)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    wxTreeItemAttr *attr;
    wxMapTreeAttr::iterator it = m_attrs.find(item.m_pItem);
    if ( it == m_attrs.end() )
    {
        m_hasAnyAttr = true;

        m_attrs[item.m_pItem] =
        attr = new wxTreeItemAttr;
    }
    else // already in the hash
    {
        attr = it->second;
    }

    attr->SetFont(font);

    // Reset the item's text to ensure that the bounding rect will be adjusted
    // for the new font.
    SetItemText(item, GetItemText(item));

    RefreshItem(item);
}

// ----------------------------------------------------------------------------
// Item status
// ----------------------------------------------------------------------------

bool wxTreeCtrl::IsVisible(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), false, wxT("invalid tree item") );

    if ( item == wxTreeItemId(TVI_ROOT) )
    {
        // virtual (hidden) root is never visible
        return false;
    }

    // Bug in Gnu-Win32 headers, so don't use the macro TreeView_GetItemRect
    RECT rect;

    // this ugliness comes directly from MSDN - it *is* the correct way to pass
    // the HTREEITEM with TVM_GETITEMRECT
    *(HTREEITEM *)&rect = HITEM(item);

    // true means to get rect for just the text, not the whole line
    if ( !::SendMessage(GetHwnd(), TVM_GETITEMRECT, true, (LPARAM)&rect) )
    {
        // if TVM_GETITEMRECT returned false, then the item is definitely not
        // visible (because its parent is not expanded)
        return false;
    }

    // however if it returned true, the item might still be outside the
    // currently visible part of the tree, test for it (notice that partly
    // visible means visible here)
    return rect.bottom > 0 && rect.top < GetClientSize().y;
}

bool wxTreeCtrl::ItemHasChildren(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), false, wxT("invalid tree item") );

    wxTreeViewItem tvItem(item, TVIF_CHILDREN);
    DoGetItem(&tvItem);

    return tvItem.cChildren != 0;
}

bool wxTreeCtrl::IsExpanded(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), false, wxT("invalid tree item") );

    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_EXPANDED);
    DoGetItem(&tvItem);

    return (tvItem.state & TVIS_EXPANDED) != 0;
}

bool wxTreeCtrl::IsSelected(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), false, wxT("invalid tree item") );

    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_SELECTED);
    DoGetItem(&tvItem);

    return (tvItem.state & TVIS_SELECTED) != 0;
}

bool wxTreeCtrl::IsBold(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), false, wxT("invalid tree item") );

    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_BOLD);
    DoGetItem(&tvItem);

    return (tvItem.state & TVIS_BOLD) != 0;
}

// ----------------------------------------------------------------------------
// navigation
// ----------------------------------------------------------------------------

wxTreeItemId wxTreeCtrl::GetRootItem() const
{
    // Root may be real (visible) or virtual (hidden).
    if ( GET_VIRTUAL_ROOT() )
        return TVI_ROOT;

    return wxTreeItemId(TreeView_GetRoot(GetHwnd()));
}

wxTreeItemId wxTreeCtrl::GetSelection() const
{
    wxCHECK_MSG( !(m_windowStyle & wxTR_MULTIPLE), wxTreeItemId(),
                 wxT("this only works with single selection controls") );

    return wxTreeItemId(TreeView_GetSelection(GetHwnd()));
}

wxTreeItemId wxTreeCtrl::GetItemParent(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );

    HTREEITEM hItem;

    if ( IS_VIRTUAL_ROOT(item) )
    {
        // no parent for the virtual root
        hItem = 0;
    }
    else // normal item
    {
        hItem = TreeView_GetParent(GetHwnd(), HITEM(item));
        if ( !hItem && HasFlag(wxTR_HIDE_ROOT) )
        {
            // the top level items should have the virtual root as their parent
            hItem = TVI_ROOT;
        }
    }

    return wxTreeItemId(hItem);
}

wxTreeItemId wxTreeCtrl::GetFirstChild(const wxTreeItemId& item,
                                       wxTreeItemIdValue& cookie) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );

    // remember the last child returned in 'cookie'
    cookie = TreeView_GetChild(GetHwnd(), HITEM(item));

    return wxTreeItemId(cookie);
}

wxTreeItemId wxTreeCtrl::GetNextChild(const wxTreeItemId& WXUNUSED(item),
                                      wxTreeItemIdValue& cookie) const
{
    wxTreeItemId fromCookie(cookie);

    HTREEITEM hitem = HITEM(fromCookie);

    hitem = TreeView_GetNextSibling(GetHwnd(), hitem);

    wxTreeItemId item(hitem);

    cookie = item.m_pItem;

    return item;
}

#if WXWIN_COMPATIBILITY_2_4

wxTreeItemId wxTreeCtrl::GetFirstChild(const wxTreeItemId& item,
                                       long& cookie) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );

    cookie = (long)TreeView_GetChild(GetHwnd(), HITEM(item));

    return wxTreeItemId((void *)cookie);
}

wxTreeItemId wxTreeCtrl::GetNextChild(const wxTreeItemId& WXUNUSED(item),
                                      long& cookie) const
{
    wxTreeItemId fromCookie((void *)cookie);

    HTREEITEM hitem = HITEM(fromCookie);

    hitem = TreeView_GetNextSibling(GetHwnd(), hitem);

    wxTreeItemId item(hitem);

    cookie = (long)item.m_pItem;

    return item;
}

#endif // WXWIN_COMPATIBILITY_2_4

wxTreeItemId wxTreeCtrl::GetLastChild(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );

    // can this be done more efficiently?
    wxTreeItemIdValue cookie;

    wxTreeItemId childLast,
    child = GetFirstChild(item, cookie);
    while ( child.IsOk() )
    {
        childLast = child;
        child = GetNextChild(item, cookie);
    }

    return childLast;
}

wxTreeItemId wxTreeCtrl::GetNextSibling(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );
    return wxTreeItemId(TreeView_GetNextSibling(GetHwnd(), HITEM(item)));
}

wxTreeItemId wxTreeCtrl::GetPrevSibling(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );
    return wxTreeItemId(TreeView_GetPrevSibling(GetHwnd(), HITEM(item)));
}

wxTreeItemId wxTreeCtrl::GetFirstVisibleItem() const
{
    return wxTreeItemId(TreeView_GetFirstVisible(GetHwnd()));
}

wxTreeItemId wxTreeCtrl::GetNextVisible(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );
    wxASSERT_MSG( IsVisible(item), wxT("The item you call GetNextVisible() for must be visible itself!"));

    return wxTreeItemId(TreeView_GetNextVisible(GetHwnd(), HITEM(item)));
}

wxTreeItemId wxTreeCtrl::GetPrevVisible(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), wxTreeItemId(), wxT("invalid tree item") );
    wxASSERT_MSG( IsVisible(item), wxT("The item you call GetPrevVisible() for must be visible itself!"));

    return wxTreeItemId(TreeView_GetPrevVisible(GetHwnd(), HITEM(item)));
}

// ----------------------------------------------------------------------------
// multiple selections emulation
// ----------------------------------------------------------------------------

bool wxTreeCtrl::IsItemChecked(const wxTreeItemId& item) const
{
    wxCHECK_MSG( item.IsOk(), false, wxT("invalid tree item") );

    // receive the desired information.
    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_STATEIMAGEMASK);
    DoGetItem(&tvItem);

    // state image indices are 1 based
    return ((tvItem.state >> 12) - 1) == 1;
}

void wxTreeCtrl::SetItemCheck(const wxTreeItemId& item, bool check)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    // receive the desired information.
    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_STATEIMAGEMASK);

    DoGetItem(&tvItem);

    // state images are one-based
    tvItem.state = (check ? 2 : 1) << 12;

    DoSetItem(&tvItem);
}

size_t wxTreeCtrl::GetSelections(wxArrayTreeItemIds& selections) const
{
    TraverseSelections selector(this, selections);

    return selector.GetCount();
}

// ----------------------------------------------------------------------------
// Usual operations
// ----------------------------------------------------------------------------

wxTreeItemId wxTreeCtrl::DoInsertAfter(const wxTreeItemId& parent,
                                       const wxTreeItemId& hInsertAfter,
                                       const wxString& text,
                                       int image, int selectedImage,
                                       wxTreeItemData *data)
{
    wxCHECK_MSG( parent.IsOk() || !TreeView_GetRoot(GetHwnd()),
                 wxTreeItemId(),
                 _T("can't have more than one root in the tree") );

    TV_INSERTSTRUCT tvIns;
    tvIns.hParent = HITEM(parent);
    tvIns.hInsertAfter = HITEM(hInsertAfter);

    // this is how we insert the item as the first child: supply a NULL
    // hInsertAfter
    if ( !tvIns.hInsertAfter )
    {
        tvIns.hInsertAfter = TVI_FIRST;
    }

    UINT mask = 0;
    if ( !text.empty() )
    {
        mask |= TVIF_TEXT;
        tvIns.item.pszText = (wxChar *)text.c_str();  // cast is ok
    }
    else
    {
        tvIns.item.pszText = NULL;
        tvIns.item.cchTextMax = 0;
    }

    // create the param which will store the other item parameters
    wxTreeItemParam *param = new wxTreeItemParam;

    // we return the images on demand as they depend on whether the item is
    // expanded or collapsed too in our case
    mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvIns.item.iImage = I_IMAGECALLBACK;
    tvIns.item.iSelectedImage = I_IMAGECALLBACK;

    param->SetImage(image, wxTreeItemIcon_Normal);
    param->SetImage(selectedImage, wxTreeItemIcon_Selected);

    mask |= TVIF_PARAM;
    tvIns.item.lParam = (LPARAM)param;
    tvIns.item.mask = mask;

    // don't use the hack below for the children of hidden root: this results
    // in a crash inside comctl32.dll when we call TreeView_GetItemRect()
    const bool firstChild = !IsHiddenRoot(parent) &&
                                !TreeView_GetChild(GetHwnd(), HITEM(parent));

    HTREEITEM id = TreeView_InsertItem(GetHwnd(), &tvIns);
    if ( id == 0 )
    {
        wxLogLastError(wxT("TreeView_InsertItem"));
    }

    // apparently some Windows versions (2000 and XP are reported to do this)
    // sometimes don't refresh the tree after adding the first child and so we
    // need this to make the "[+]" appear
    if ( firstChild )
    {
        RECT rect;
        TreeView_GetItemRect(GetHwnd(), HITEM(parent), &rect, FALSE);
        ::InvalidateRect(GetHwnd(), &rect, FALSE);
    }

    // associate the application tree item with Win32 tree item handle
    param->SetItem(id);

    // setup wxTreeItemData
    if ( data != NULL )
    {
        param->SetData(data);
        data->SetId(id);
    }

    return wxTreeItemId(id);
}

// for compatibility only
#if WXWIN_COMPATIBILITY_2_4

void wxTreeCtrl::SetImageList(wxImageList *imageList, int)
{
    SetImageList(imageList);
}

int wxTreeCtrl::GetItemSelectedImage(const wxTreeItemId& item) const
{
    return GetItemImage(item, wxTreeItemIcon_Selected);
}

void wxTreeCtrl::SetItemSelectedImage(const wxTreeItemId& item, int image)
{
    SetItemImage(item, image, wxTreeItemIcon_Selected);
}

#endif // WXWIN_COMPATIBILITY_2_4

wxTreeItemId wxTreeCtrl::AddRoot(const wxString& text,
                                 int image, int selectedImage,
                                 wxTreeItemData *data)
{

    if ( HasFlag(wxTR_HIDE_ROOT) )
    {
        // create a virtual root item, the parent for all the others
        wxTreeItemParam *param = new wxTreeItemParam;
        param->SetData(data);

        m_pVirtualRoot = new wxVirtualNode(param);

        return TVI_ROOT;
    }

    return DoInsertAfter(wxTreeItemId(), wxTreeItemId(),
                           text, image, selectedImage, data);
}

wxTreeItemId wxTreeCtrl::DoInsertItem(const wxTreeItemId& parent,
                                      size_t index,
                                      const wxString& text,
                                      int image, int selectedImage,
                                      wxTreeItemData *data)
{
    wxTreeItemId idPrev;
    if ( index == (size_t)-1 )
    {
        // special value: append to the end
        idPrev = TVI_LAST;
    }
    else // find the item from index
    {
        wxTreeItemIdValue cookie;
        wxTreeItemId idCur = GetFirstChild(parent, cookie);
        while ( index != 0 && idCur.IsOk() )
        {
            index--;

            idPrev = idCur;
            idCur = GetNextChild(parent, cookie);
        }

        // assert, not check: if the index is invalid, we will append the item
        // to the end
        wxASSERT_MSG( index == 0, _T("bad index in wxTreeCtrl::InsertItem") );
    }

    return DoInsertAfter(parent, idPrev, text, image, selectedImage, data);
}

void wxTreeCtrl::Delete(const wxTreeItemId& item)
{
    // unlock tree selections on vista, without this the
    // tree ctrl will eventually crash after item deletion
    TreeItemUnlocker unlock_all;

    if ( !TreeView_DeleteItem(GetHwnd(), HITEM(item)) )
    {
        wxLogLastError(wxT("TreeView_DeleteItem"));
    }
}

// delete all children (but don't delete the item itself)
void wxTreeCtrl::DeleteChildren(const wxTreeItemId& item)
{
    // unlock tree selections on vista for the duration of this call
    TreeItemUnlocker unlock_all;

    wxTreeItemIdValue cookie;

    wxArrayTreeItemIds children;
    wxTreeItemId child = GetFirstChild(item, cookie);
    while ( child.IsOk() )
    {
        children.Add(child);

        child = GetNextChild(item, cookie);
    }

    size_t nCount = children.Count();
    for ( size_t n = 0; n < nCount; n++ )
    {
        if ( !TreeView_DeleteItem(GetHwnd(), HITEM(children[n])) )
        {
            wxLogLastError(wxT("TreeView_DeleteItem"));
        }
    }
}

void wxTreeCtrl::DeleteAllItems()
{
    // unlock tree selections on vista for the duration of this call
    TreeItemUnlocker unlock_all;

    // delete the "virtual" root item.
    if ( GET_VIRTUAL_ROOT() )
    {
        delete GET_VIRTUAL_ROOT();
        m_pVirtualRoot = NULL;
    }

    // and all the real items

    if ( !TreeView_DeleteAllItems(GetHwnd()) )
    {
        wxLogLastError(wxT("TreeView_DeleteAllItems"));
    }
}

void wxTreeCtrl::DoExpand(const wxTreeItemId& item, int flag)
{
    wxASSERT_MSG( flag == TVE_COLLAPSE ||
                  flag == (TVE_COLLAPSE | TVE_COLLAPSERESET) ||
                  flag == TVE_EXPAND   ||
                  flag == TVE_TOGGLE,
                  wxT("Unknown flag in wxTreeCtrl::DoExpand") );

    // A hidden root can be neither expanded nor collapsed.
    wxCHECK_RET( !IsHiddenRoot(item),
                 wxT("Can't expand/collapse hidden root node!") );

    // TreeView_Expand doesn't send TVN_ITEMEXPAND(ING) messages, so we must
    // emulate them. This behaviour has changed slightly with comctl32.dll
    // v 4.70 - now it does send them but only the first time. To maintain
    // compatible behaviour and also in order to not have surprises with the
    // future versions, don't rely on this and still do everything ourselves.
    // To avoid that the messages be sent twice when the item is expanded for
    // the first time we must clear TVIS_EXPANDEDONCE style manually.

    wxTreeViewItem tvItem(item, TVIF_STATE, TVIS_EXPANDEDONCE);
    tvItem.state = 0;
    DoSetItem(&tvItem);

    if ( TreeView_Expand(GetHwnd(), HITEM(item), flag) != 0 )
    {
        // note that the {EXPAND|COLLAPS}ING event is sent by TreeView_Expand()
        // itself
        wxTreeEvent event(gs_expandEvents[IsExpanded(item) ? IDX_EXPAND
                                                           : IDX_COLLAPSE]
                                         [IDX_DONE],
                           this, item);
        (void)GetEventHandler()->ProcessEvent(event);
    }
    //else: change didn't took place, so do nothing at all
}

void wxTreeCtrl::Expand(const wxTreeItemId& item)
{
    DoExpand(item, TVE_EXPAND);
}

void wxTreeCtrl::Collapse(const wxTreeItemId& item)
{
    DoExpand(item, TVE_COLLAPSE);
}

void wxTreeCtrl::CollapseAndReset(const wxTreeItemId& item)
{
    DoExpand(item, TVE_COLLAPSE | TVE_COLLAPSERESET);
}

void wxTreeCtrl::Toggle(const wxTreeItemId& item)
{
    DoExpand(item, TVE_TOGGLE);
}

#if WXWIN_COMPATIBILITY_2_4

void wxTreeCtrl::ExpandItem(const wxTreeItemId& item, int action)
{
    DoExpand(item, action);
}

#endif

void wxTreeCtrl::Unselect()
{
    wxASSERT_MSG( !(m_windowStyle & wxTR_MULTIPLE),
                  wxT("doesn't make sense, may be you want UnselectAll()?") );

    // just remove the selection
    SelectItem(wxTreeItemId());
}

void wxTreeCtrl::UnselectAll()
{
    if ( m_windowStyle & wxTR_MULTIPLE )
    {
        wxArrayTreeItemIds selections;
        size_t count = GetSelections(selections);
        for ( size_t n = 0; n < count; n++ )
        {
            ::UnselectItem(GetHwnd(), HITEM(selections[n]));
        }

        m_htSelStart.Unset();
    }
    else
    {
        // just remove the selection
        Unselect();
    }
}

void wxTreeCtrl::SelectItem(const wxTreeItemId& item, bool select)
{
    wxCHECK_RET( !IsHiddenRoot(item), _T("can't select hidden root item") );

    if ( m_windowStyle & wxTR_MULTIPLE )
    {
        ::SelectItem(GetHwnd(), HITEM(item), select);
    }
    else
    {
        wxASSERT_MSG( select,
                      _T("SelectItem(false) works only for multiselect") );

        // inspite of the docs (MSDN Jan 99 edition), we don't seem to receive
        // the notification from the control (i.e. TVN_SELCHANG{ED|ING}), so
        // send them ourselves

        wxTreeEvent event(wxEVT_COMMAND_TREE_SEL_CHANGING, this, item);
        if ( !GetEventHandler()->ProcessEvent(event) || event.IsAllowed() )
        {
            if ( !TreeView_SelectItem(GetHwnd(), HITEM(item)) )
            {
                wxLogLastError(wxT("TreeView_SelectItem"));
            }
            else // ok
            {
                event.SetEventType(wxEVT_COMMAND_TREE_SEL_CHANGED);
                (void)GetEventHandler()->ProcessEvent(event);
            }
        }
        //else: program vetoed the change
    }
}

void wxTreeCtrl::EnsureVisible(const wxTreeItemId& item)
{
    wxCHECK_RET( !IsHiddenRoot(item), _T("can't show hidden root item") );

    // no error return
    TreeView_EnsureVisible(GetHwnd(), HITEM(item));
}

void wxTreeCtrl::ScrollTo(const wxTreeItemId& item)
{
    if ( !TreeView_SelectSetFirstVisible(GetHwnd(), HITEM(item)) )
    {
        wxLogLastError(wxT("TreeView_SelectSetFirstVisible"));
    }
}

wxTextCtrl *wxTreeCtrl::GetEditControl() const
{
    return m_textCtrl;
}

void wxTreeCtrl::DeleteTextCtrl()
{
    if ( m_textCtrl )
    {
        // the HWND corresponding to this control is deleted by the tree
        // control itself and we don't know when exactly this happens, so check
        // if the window still exists before calling UnsubclassWin()
        if ( !::IsWindow(GetHwndOf(m_textCtrl)) )
        {
            m_textCtrl->SetHWND(0);
        }

        m_textCtrl->UnsubclassWin();
        m_textCtrl->SetHWND(0);
        delete m_textCtrl;
        m_textCtrl = NULL;

        m_idEdited.Unset();
    }
}

wxTextCtrl *wxTreeCtrl::EditLabel(const wxTreeItemId& item,
                                  wxClassInfo *textControlClass)
{
    wxASSERT( textControlClass->IsKindOf(CLASSINFO(wxTextCtrl)) );

    DeleteTextCtrl();

    m_idEdited = item;
    m_textCtrl = (wxTextCtrl *)textControlClass->CreateObject();
    HWND hWnd = (HWND) TreeView_EditLabel(GetHwnd(), HITEM(item));

    // this is not an error - the TVN_BEGINLABELEDIT handler might have
    // returned false
    if ( !hWnd )
    {
        delete m_textCtrl;
        m_textCtrl = NULL;
        return NULL;
    }

    // textctrl is subclassed in MSWOnNotify
    return m_textCtrl;
}

// End label editing, optionally cancelling the edit
void wxTreeCtrl::DoEndEditLabel(bool discardChanges)
{
    TreeView_EndEditLabelNow(GetHwnd(), discardChanges);

    DeleteTextCtrl();
}

wxTreeItemId wxTreeCtrl::DoTreeHitTest(const wxPoint& point, int& flags) const
{
    TV_HITTESTINFO hitTestInfo;
    hitTestInfo.pt.x = (int)point.x;
    hitTestInfo.pt.y = (int)point.y;

    (void) TreeView_HitTest(GetHwnd(), &hitTestInfo);

    flags = 0;

    // avoid repetition
    #define TRANSLATE_FLAG(flag) if ( hitTestInfo.flags & TVHT_##flag ) \
                                    flags |= wxTREE_HITTEST_##flag

    TRANSLATE_FLAG(ABOVE);
    TRANSLATE_FLAG(BELOW);
    TRANSLATE_FLAG(NOWHERE);
    TRANSLATE_FLAG(ONITEMBUTTON);
    TRANSLATE_FLAG(ONITEMICON);
    TRANSLATE_FLAG(ONITEMINDENT);
    TRANSLATE_FLAG(ONITEMLABEL);
    TRANSLATE_FLAG(ONITEMRIGHT);
    TRANSLATE_FLAG(ONITEMSTATEICON);
    TRANSLATE_FLAG(TOLEFT);
    TRANSLATE_FLAG(TORIGHT);

    #undef TRANSLATE_FLAG

    return wxTreeItemId(hitTestInfo.hItem);
}

bool wxTreeCtrl::GetBoundingRect(const wxTreeItemId& item,
                                 wxRect& rect,
                                 bool textOnly) const
{
    RECT rc;

    // Virtual root items have no bounding rectangle
    if ( IS_VIRTUAL_ROOT(item) )
    {
        return false;
    }

    if ( TreeView_GetItemRect(GetHwnd(), HITEM(item),
                              &rc, textOnly) )
    {
        rect = wxRect(wxPoint(rc.left, rc.top), wxPoint(rc.right, rc.bottom));

        return true;
    }
    else
    {
        // couldn't retrieve rect: for example, item isn't visible
        return false;
    }
}

// ----------------------------------------------------------------------------
// sorting stuff
// ----------------------------------------------------------------------------

// this is just a tiny namespace which is friend to wxTreeCtrl and so can use
// functions such as IsDataIndirect()
class wxTreeSortHelper
{
public:
    static int CALLBACK Compare(LPARAM data1, LPARAM data2, LPARAM tree);

private:
    static wxTreeItemId GetIdFromData(LPARAM lParam)
    {
        return ((wxTreeItemParam*)lParam)->GetItem();
        }
};

int CALLBACK wxTreeSortHelper::Compare(LPARAM pItem1,
                                       LPARAM pItem2,
                                       LPARAM htree)
{
    wxCHECK_MSG( pItem1 && pItem2, 0,
                 wxT("sorting tree without data doesn't make sense") );

    wxTreeCtrl *tree = (wxTreeCtrl *)htree;

    return tree->OnCompareItems(GetIdFromData(pItem1),
                                GetIdFromData(pItem2));
}

void wxTreeCtrl::SortChildren(const wxTreeItemId& item)
{
    wxCHECK_RET( item.IsOk(), wxT("invalid tree item") );

    // rely on the fact that TreeView_SortChildren does the same thing as our
    // default behaviour, i.e. sorts items alphabetically and so call it
    // directly if we're not in derived class (much more efficient!)
    // RN: Note that if you find you're code doesn't sort as expected this
    //     may be why as if you don't use the DECLARE_CLASS/IMPLEMENT_CLASS
    //     combo for your derived wxTreeCtrl if will sort without
    //     OnCompareItems
    if ( GetClassInfo() == CLASSINFO(wxTreeCtrl) )
    {
        TreeView_SortChildren(GetHwnd(), HITEM(item), 0);
    }
    else
    {
        TV_SORTCB tvSort;
        tvSort.hParent = HITEM(item);
        tvSort.lpfnCompare = wxTreeSortHelper::Compare;
        tvSort.lParam = (LPARAM)this;
        TreeView_SortChildrenCB(GetHwnd(), &tvSort, 0 /* reserved */);
    }
}

// ----------------------------------------------------------------------------
// implementation
// ----------------------------------------------------------------------------

bool wxTreeCtrl::MSWShouldPreProcessMessage(WXMSG* msg)
{
    if ( msg->message == WM_KEYDOWN )
    {
        const bool isAltDown = ::GetKeyState(VK_MENU) < 0;

        // Only eat VK_RETURN if not being used by the application in conjunction with
        // modifiers
        if ( msg->wParam == VK_RETURN && !wxIsCtrlDown() && !wxIsShiftDown() && !isAltDown)
        {
            // we need VK_RETURN to generate wxEVT_COMMAND_TREE_ITEM_ACTIVATED
            return false;
        }
    }

    return wxTreeCtrlBase::MSWShouldPreProcessMessage(msg);
}

bool wxTreeCtrl::MSWCommand(WXUINT cmd, WXWORD id)
{
    if ( cmd == EN_UPDATE )
    {
        wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, id);
        event.SetEventObject( this );
        ProcessCommand(event);
    }
    else if ( cmd == EN_KILLFOCUS )
    {
        wxCommandEvent event(wxEVT_KILL_FOCUS, id);
        event.SetEventObject( this );
        ProcessCommand(event);
    }
    else
    {
        // nothing done
        return false;
    }

    // command processed
    return true;
}

// we hook into WndProc to process WM_MOUSEMOVE/WM_BUTTONUP messages - as we
// only do it during dragging, minimize wxWin overhead (this is important for
// WM_MOUSEMOVE as they're a lot of them) by catching Windows messages directly
// instead of passing by wxWin events
WXLRESULT wxTreeCtrl::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    bool processed = false;
    WXLRESULT rc = 0;
    bool isMultiple = HasFlag(wxTR_MULTIPLE);

    // This message is sent after a right-click, or when the "menu" key is pressed
    if ( nMsg == WM_CONTEXTMENU )
    {
        int x = GET_X_LPARAM(lParam),
            y = GET_Y_LPARAM(lParam);

        // the item for which the menu should be shown
        wxTreeItemId item;

        // the position where the menu should be shown in client coordinates
        // (so that it can be passed directly to PopupMenu())
        wxPoint pt;

        if ( x == -1 || y == -1 )
        {
            // this means that the event was generated from keyboard (e.g. with
            // Shift-F10 or special Windows menu key)
            //
            // use the Explorer standard of putting the menu at the left edge
            // of the text, in the vertical middle of the text
            item = wxTreeItemId(TreeView_GetSelection(GetHwnd()));
            if ( item.IsOk() )
            {
                // Use the bounding rectangle of only the text part
                wxRect rect;
                GetBoundingRect(item, rect, true);
                pt = wxPoint(rect.GetX(), rect.GetY() + rect.GetHeight() / 2);
            }
        }
        else // event from mouse, use mouse position
        {
            pt = ScreenToClient(wxPoint(x, y));

            TV_HITTESTINFO tvhti;
            tvhti.pt.x = pt.x;
            tvhti.pt.y = pt.y;
            if ( TreeView_HitTest(GetHwnd(), &tvhti) )
                item = wxTreeItemId(tvhti.hItem);
        }

        // create the event
        wxTreeEvent event(wxEVT_COMMAND_TREE_ITEM_MENU, this, item);

        event.m_pointDrag = pt;

        if ( GetEventHandler()->ProcessEvent(event) )
            processed = true;
        //else: continue with generating wxEVT_CONTEXT_MENU in base class code
    }
    else if ( (nMsg >= WM_MOUSEFIRST) && (nMsg <= WM_MOUSELAST) )
    {
        // we only process mouse messages here and these parameters have the
        // same meaning for all of them
        int x = GET_X_LPARAM(lParam),
            y = GET_Y_LPARAM(lParam);

        TV_HITTESTINFO tvht;
        tvht.pt.x = x;
        tvht.pt.y = y;

        HTREEITEM htItem = TreeView_HitTest(GetHwnd(), &tvht);

        switch ( nMsg )
        {
            case WM_LBUTTONDOWN:
                if ( htItem && isMultiple && (tvht.flags & TVHT_ONITEM) != 0 )
                {
                    m_htClickedItem = (WXHTREEITEM) htItem;
                    m_ptClick = wxPoint(x, y);

                    if ( wParam & MK_CONTROL )
                    {
                        SetFocus();

                        // toggle selected state
                        ::ToggleItemSelection(GetHwnd(), htItem);

                        ::SetFocus(GetHwnd(), htItem);

                        // reset on any click without Shift
                        m_htSelStart.Unset();

                        processed = true;
                    }
                    else if ( wParam & MK_SHIFT )
                    {
                        // this selects all items between the starting one and
                        // the current

                        if ( !m_htSelStart )
                        {
                            // take the focused item
                            m_htSelStart = TreeView_GetSelection(GetHwnd());
                        }

                        if ( m_htSelStart )
                            SelectRange(GetHwnd(), HITEM(m_htSelStart), htItem,
                                    !(wParam & MK_CONTROL));
                        else
                            ::SelectItem(GetHwnd(), htItem);

                        ::SetFocus(GetHwnd(), htItem);

                        processed = true;
                    }
                    else // normal click
                    {
                        // avoid doing anything if we click on the only
                        // currently selected item

                        SetFocus();

                        wxArrayTreeItemIds selections;
                        size_t count = GetSelections(selections);
                        if ( count == 0 ||
                             count > 1 ||
                             HITEM(selections[0]) != htItem )
                        {
                            // clear the previously selected items, if the
                            // user clicked outside of the present selection.
                            // otherwise, perform the deselection on mouse-up.
                            // this allows multiple drag and drop to work.

                            if (!IsItemSelected(GetHwnd(), htItem))
                            {
                                UnselectAll();

                                // prevent the click from starting in-place editing
                                // which should only happen if we click on the
                                // already selected item (and nothing else is
                                // selected)

                                TreeView_SelectItem(GetHwnd(), 0);
                                ::SelectItem(GetHwnd(), htItem);
                            }
                            ::SetFocus(GetHwnd(), htItem);
                            processed = true;
                        }
                        else // click on a single selected item
                        {
                            // don't interfere with the default processing in
                            // WM_MOUSEMOVE handler below as the default window
                            // proc will start the drag itself if we let have
                            // WM_LBUTTONDOWN
                            m_htClickedItem.Unset();
                        }

                        // reset on any click without Shift
                        m_htSelStart.Unset();
                    }
                }
                break;

            case WM_RBUTTONDOWN:
                // default handler removes the highlight from the currently
                // focused item when right mouse button is pressed on another
                // one but keeps the remaining items highlighted, which is
                // confusing, so override this default behaviour for tree with
                // multiple selections
                if ( isMultiple )
                {
                    if ( !IsItemSelected(GetHwnd(), htItem) )
                    {
                        UnselectAll();
                        SelectItem(htItem);
                        ::SetFocus(GetHwnd(), htItem);
                    }

                    // fire EVT_RIGHT_DOWN
                    HandleMouseEvent(nMsg, x, y, wParam);

                    // send NM_RCLICK
                    NMHDR nmhdr;
                    nmhdr.hwndFrom = GetHwnd();
                    nmhdr.idFrom = ::GetWindowLong(GetHwnd(), GWL_ID);
                    nmhdr.code = NM_RCLICK;
                    ::SendMessage(::GetParent(GetHwnd()), WM_NOTIFY,
                                  nmhdr.idFrom, (LPARAM)&nmhdr);

                    // prevent tree control default processing, as we've
                    // already done everything
                    processed = true;
                }
                break;

            case WM_MOUSEMOVE:
#ifndef __WXWINCE__
                if ( m_htClickedItem )
                {
                    int cx = abs(m_ptClick.x - x);
                    int cy = abs(m_ptClick.y - y);

                    if ( cx > ::GetSystemMetrics(SM_CXDRAG) ||
                            cy > ::GetSystemMetrics(SM_CYDRAG) )
                    {
                        NM_TREEVIEW tv;
                        wxZeroMemory(tv);

                        tv.hdr.hwndFrom = GetHwnd();
                        tv.hdr.idFrom = ::GetWindowLong(GetHwnd(), GWL_ID);
                        tv.hdr.code = TVN_BEGINDRAG;

                        tv.itemNew.hItem = HITEM(m_htClickedItem);


                        TVITEM tviAux;
                        wxZeroMemory(tviAux);

                        tviAux.hItem = HITEM(m_htClickedItem);
                        tviAux.mask = TVIF_STATE | TVIF_PARAM;
                        tviAux.stateMask = 0xffffffff;
                        TreeView_GetItem(GetHwnd(), &tviAux);

                        tv.itemNew.state = tviAux.state;
                        tv.itemNew.lParam = tviAux.lParam;

                        tv.ptDrag.x = x;
                        tv.ptDrag.y = y;

                        // do it before SendMessage() call below to avoid
                        // reentrancies here if there is another WM_MOUSEMOVE
                        // in the queue already
                        m_htClickedItem.Unset();

                        ::SendMessage(GetHwndOf(GetParent()), WM_NOTIFY,
                                      tv.hdr.idFrom, (LPARAM)&tv );

                        // don't pass it to the default window proc, it would
                        // start dragging again
                        processed = true;
                    }
                }
#endif // __WXWINCE__

#if wxUSE_DRAGIMAGE
                if ( m_dragImage )
                {
                    m_dragImage->Move(wxPoint(x, y));
                    if ( htItem )
                    {
                        // highlight the item as target (hiding drag image is
                        // necessary - otherwise the display will be corrupted)
                        m_dragImage->Hide();
                        TreeView_SelectDropTarget(GetHwnd(), htItem);
                        m_dragImage->Show();
                    }
                }
#endif // wxUSE_DRAGIMAGE
                break;

            case WM_LBUTTONUP:

                // facilitates multiple drag-and-drop
                if (htItem && isMultiple)
                {
                    wxArrayTreeItemIds selections;
                    size_t count = GetSelections(selections);

                    if (count > 1 &&
                        !(wParam & MK_CONTROL) &&
                        !(wParam & MK_SHIFT))
                    {
                        UnselectAll();
                        TreeView_SelectItem(GetHwnd(), htItem);
                        ::SelectItem(GetHwnd(), htItem);
                        ::SetFocus(GetHwnd(), htItem);
                    }
                    m_htClickedItem.Unset();
                }

                // fall through

            case WM_RBUTTONUP:
#if wxUSE_DRAGIMAGE
                if ( m_dragImage )
                {
                    m_dragImage->EndDrag();
                    delete m_dragImage;
                    m_dragImage = NULL;

                    // generate the drag end event
                    wxTreeEvent event(wxEVT_COMMAND_TREE_END_DRAG, this, htItem);
                    event.m_pointDrag = wxPoint(x, y);
                    (void)GetEventHandler()->ProcessEvent(event);

                    // if we don't do it, the tree seems to think that 2 items
                    // are selected simultaneously which is quite weird
                    TreeView_SelectDropTarget(GetHwnd(), 0);
                }
#endif // wxUSE_DRAGIMAGE
                break;
        }
    }
    else if ( (nMsg == WM_SETFOCUS || nMsg == WM_KILLFOCUS) && isMultiple )
    {
        // the tree control greys out the selected item when it loses focus and
        // paints it as selected again when it regains it, but it won't do it
        // for the other items itself - help it
        wxArrayTreeItemIds selections;
        size_t count = GetSelections(selections);
        RECT rect;
        for ( size_t n = 0; n < count; n++ )
        {
            // TreeView_GetItemRect() will return false if item is not visible,
            // which may happen perfectly well
            if ( TreeView_GetItemRect(GetHwnd(), HITEM(selections[n]),
                                      &rect, TRUE) )
            {
                ::InvalidateRect(GetHwnd(), &rect, FALSE);
            }
        }
    }
    else if ( nMsg == WM_KEYDOWN && isMultiple )
    {
        bool bCtrl = wxIsCtrlDown(),
             bShift = wxIsShiftDown();

        HTREEITEM htSel = (HTREEITEM)TreeView_GetSelection(GetHwnd());
        switch ( wParam )
        {
            case VK_SPACE:
                if ( bCtrl )
                {
                    ::ToggleItemSelection(GetHwnd(), htSel);
                }
                else
                {
                    UnselectAll();

                    ::SelectItem(GetHwnd(), htSel);
                }

                processed = true;
                break;

            case VK_UP:
            case VK_DOWN:
                if ( !bCtrl && !bShift )
                {
                    // no modifiers, just clear selection and then let the default
                    // processing to take place
                    UnselectAll();
                }
                else if ( htSel )
                {
                    (void)wxControl::MSWWindowProc(nMsg, wParam, lParam);

                    HTREEITEM htNext = (HTREEITEM)
                        TreeView_GetNextItem
                        (
                            GetHwnd(),
                            htSel,
                            wParam == VK_UP ? TVGN_PREVIOUSVISIBLE
                                            : TVGN_NEXTVISIBLE
                        );

                    if ( !htNext )
                    {
                        // at the top/bottom
                        htNext = htSel;
                    }

                    if ( bShift )
                    {
                        if ( !m_htSelStart )
                            m_htSelStart = htSel;

                        SelectRange(GetHwnd(), HITEM(m_htSelStart), htNext);
                    }
                    else // bCtrl
                    {
                        // without changing selection
                        ::SetFocus(GetHwnd(), htNext);
                    }

                    processed = true;
                }
                break;

            case VK_HOME:
            case VK_END:
            case VK_PRIOR:
            case VK_NEXT:
                // TODO: handle Shift/Ctrl with these keys
                if ( !bCtrl && !bShift )
                {
                    UnselectAll();

                    m_htSelStart.Unset();
                }
        }
    }
    else if ( nMsg == WM_COMMAND )
    {
        // if we receive a EN_KILLFOCUS command from the in-place edit control
        // used for label editing, make sure to end editing
        WORD id, cmd;
        WXHWND hwnd;
        UnpackCommand(wParam, lParam, &id, &hwnd, &cmd);

        if ( cmd == EN_KILLFOCUS )
        {
            if ( m_textCtrl && m_textCtrl->GetHandle() == hwnd )
            {
                DoEndEditLabel();

                processed = true;
            }
        }
    }

    if ( !processed )
        rc = wxControl::MSWWindowProc(nMsg, wParam, lParam);

    return rc;
}

WXLRESULT
wxTreeCtrl::MSWDefWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if ( nMsg == WM_CHAR )
    {
        // don't let the control process Space and Return keys because it
        // doesn't do anything useful with them anyhow but always beeps
        // annoyingly when it receives them and there is no way to turn it off
        // simply if you just process TREEITEM_ACTIVATED event to which Space
        // and Enter presses are mapped in your code
        if ( wParam == VK_SPACE || wParam == VK_RETURN )
            return 0;
    }
#if wxUSE_DRAGIMAGE
    else if ( nMsg == WM_KEYDOWN )
    {
        if ( wParam == VK_ESCAPE )
        {
            if ( m_dragImage )
            {
                m_dragImage->EndDrag();
                delete m_dragImage;
                m_dragImage = NULL;

                // if we don't do it, the tree seems to think that 2 items
                // are selected simultaneously which is quite weird
                TreeView_SelectDropTarget(GetHwnd(), 0);
            }
        }
    }
#endif // wxUSE_DRAGIMAGE

    return wxControl::MSWDefWindowProc(nMsg, wParam, lParam);
}

// process WM_NOTIFY Windows message
bool wxTreeCtrl::MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result)
{
    wxTreeEvent event(wxEVT_NULL, this);
    wxEventType eventType = wxEVT_NULL;
    NMHDR *hdr = (NMHDR *)lParam;

    switch ( hdr->code )
    {
        case TVN_BEGINDRAG:
            eventType = wxEVT_COMMAND_TREE_BEGIN_DRAG;
            // fall through

        case TVN_BEGINRDRAG:
            {
                if ( eventType == wxEVT_NULL )
                    eventType = wxEVT_COMMAND_TREE_BEGIN_RDRAG;
                //else: left drag, already set above

                NM_TREEVIEW *tv = (NM_TREEVIEW *)lParam;

                event.m_item = tv->itemNew.hItem;
                event.m_pointDrag = wxPoint(tv->ptDrag.x, tv->ptDrag.y);

                // don't allow dragging by default: the user code must
                // explicitly say that it wants to allow it to avoid breaking
                // the old apps
                event.Veto();
            }
            break;

        case TVN_BEGINLABELEDIT:
            {
                eventType = wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT;
                TV_DISPINFO *info = (TV_DISPINFO *)lParam;

                // although the user event handler may still veto it, it is
                // important to set it now so that calls to SetItemText() from
                // the event handler would change the text controls contents
                m_idEdited =
                event.m_item = info->item.hItem;
                event.m_label = info->item.pszText;
                event.m_editCancelled = false;
            }
            break;

        case TVN_DELETEITEM:
            {
                eventType = wxEVT_COMMAND_TREE_DELETE_ITEM;
                NM_TREEVIEW *tv = (NM_TREEVIEW *)lParam;

                event.m_item = tv->itemOld.hItem;

                if ( m_hasAnyAttr )
                {
                    wxMapTreeAttr::iterator it = m_attrs.find(tv->itemOld.hItem);
                    if ( it != m_attrs.end() )
                    {
                        delete it->second;
                        m_attrs.erase(it);
                    }
                }
            }
            break;

        case TVN_ENDLABELEDIT:
            {
                eventType = wxEVT_COMMAND_TREE_END_LABEL_EDIT;
                TV_DISPINFO *info = (TV_DISPINFO *)lParam;

                event.m_item = info->item.hItem;
                event.m_label = info->item.pszText;
                event.m_editCancelled = info->item.pszText == NULL;
                break;
            }

#ifndef __WXWINCE__
        // These *must* not be removed or TVN_GETINFOTIP will
        // not be processed each time the mouse is moved
        // and the tooltip will only ever update once.
        case TTN_NEEDTEXTA:
        case TTN_NEEDTEXTW:
            {
                *result = 0;

                break;
            }

#ifdef TVN_GETINFOTIP
        case TVN_GETINFOTIP:
            {
                eventType = wxEVT_COMMAND_TREE_ITEM_GETTOOLTIP;
                NMTVGETINFOTIP *info = (NMTVGETINFOTIP*)lParam;

                // Which item are we trying to get a tooltip for?
                event.m_item = info->hItem;

                break;
            }
#endif // TVN_GETINFOTIP
#endif // !__WXWINCE__

        case TVN_GETDISPINFO:
            eventType = wxEVT_COMMAND_TREE_GET_INFO;
            // fall through

        case TVN_SETDISPINFO:
            {
                if ( eventType == wxEVT_NULL )
                    eventType = wxEVT_COMMAND_TREE_SET_INFO;
                //else: get, already set above

                TV_DISPINFO *info = (TV_DISPINFO *)lParam;

                event.m_item = info->item.hItem;
                break;
            }

        case TVN_ITEMEXPANDING:
        case TVN_ITEMEXPANDED:
            {
                NM_TREEVIEW *tv = (NM_TREEVIEW*)lParam;

                int what;
                switch ( tv->action )
                {
                    default:
                        wxLogDebug(wxT("unexpected code %d in TVN_ITEMEXPAND message"), tv->action);
                        // fall through

                    case TVE_EXPAND:
                        what = IDX_EXPAND;
                        break;

                    case TVE_COLLAPSE:
                        what = IDX_COLLAPSE;
                        break;
                }

                int how = hdr->code == TVN_ITEMEXPANDING ? IDX_DOING
                                                         : IDX_DONE;

                eventType = gs_expandEvents[what][how];

                event.m_item = tv->itemNew.hItem;
            }
            break;

        case TVN_KEYDOWN:
            {
                eventType = wxEVT_COMMAND_TREE_KEY_DOWN;
                TV_KEYDOWN *info = (TV_KEYDOWN *)lParam;

                // fabricate the lParam and wParam parameters sufficiently
                // similar to the ones from a "real" WM_KEYDOWN so that
                // CreateKeyEvent() works correctly
                const bool isAltDown = ::GetKeyState(VK_MENU) < 0;
                WXLPARAM lParam = (isAltDown ? KF_ALTDOWN : 0) << 16;

                WXWPARAM wParam = info->wVKey;

                int keyCode = wxCharCodeMSWToWX(wParam);
                if ( !keyCode )
                {
                    // wxCharCodeMSWToWX() returns 0 to indicate that this is a
                    // simple ASCII key
                    keyCode = wParam;
                }

                event.m_evtKey = CreateKeyEvent(wxEVT_KEY_DOWN,
                                                keyCode,
                                                lParam,
                                                wParam);

                // a separate event for Space/Return
                if ( !wxIsCtrlDown() && !wxIsShiftDown() && !isAltDown &&
                     ((info->wVKey == VK_SPACE) || (info->wVKey == VK_RETURN)) )
                {
                   wxTreeItemId item;
                   if ( !HasFlag(wxTR_MULTIPLE) )
                       item = GetSelection();

                   wxTreeEvent event2(wxEVT_COMMAND_TREE_ITEM_ACTIVATED,
                                        this, item);
                   (void)GetEventHandler()->ProcessEvent(event2);
                }
            }
            break;


        // Vista's tree control has introduced some problems with our
        // multi-selection tree.  When TreeView_SelectItem() is called,
        // the wrong items are deselected.

        // Fortunately, Vista provides a new notification, TVN_ITEMCHANGING
        // that can be used to regulate this incorrect behavior.  The
        // following messages will allow only the unlocked item's selection
        // state to change

        case TVN_ITEMCHANGINGA:
        case TVN_ITEMCHANGINGW:
            {
                // we only need to handles these in multi-select trees
                if ( HasFlag(wxTR_MULTIPLE) )
                {
                    // get info about the item about to be changed
                    NMTVITEMCHANGE* info = (NMTVITEMCHANGE*)lParam;
                    if (TreeItemUnlocker::IsLocked(info->hItem))
                    {
                        // item's state is locked, don't allow the change
                        // returning 1 will disallow the change
                        *result = 1;
                        return true;
                    }
                }

                // allow the state change
            }
            return false;

        // NB: MSLU is broken and sends TVN_SELCHANGEDA instead of
        //     TVN_SELCHANGEDW in Unicode mode under Win98. Therefore
        //     we have to handle both messages:
        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            eventType = wxEVT_COMMAND_TREE_SEL_CHANGED;
            // fall through

        case TVN_SELCHANGINGA:
        case TVN_SELCHANGINGW:
            {
                if ( eventType == wxEVT_NULL )
                    eventType = wxEVT_COMMAND_TREE_SEL_CHANGING;
                //else: already set above

                if (hdr->code == TVN_SELCHANGINGW ||
                    hdr->code == TVN_SELCHANGEDW)
                {
                    NM_TREEVIEWW *tv = (NM_TREEVIEWW *)lParam;
                    event.m_item = tv->itemNew.hItem;
                    event.m_itemOld = tv->itemOld.hItem;
                }
                else
                {
                    NM_TREEVIEWA *tv = (NM_TREEVIEWA *)lParam;
                    event.m_item = tv->itemNew.hItem;
                    event.m_itemOld = tv->itemOld.hItem;
                }
            }
            break;

            // instead of explicitly checking for _WIN32_IE, check if the
            // required symbols are available in the headers
#if defined(CDDS_PREPAINT) && !wxUSE_COMCTL32_SAFELY
        case NM_CUSTOMDRAW:
            {
                LPNMTVCUSTOMDRAW lptvcd = (LPNMTVCUSTOMDRAW)lParam;
                NMCUSTOMDRAW& nmcd = lptvcd->nmcd;
                switch ( nmcd.dwDrawStage )
                {
                    case CDDS_PREPAINT:
                        // if we've got any items with non standard attributes,
                        // notify us before painting each item
                        *result = m_hasAnyAttr ? CDRF_NOTIFYITEMDRAW
                                               : CDRF_DODEFAULT;
                        break;

                    case CDDS_ITEMPREPAINT:
                        {
                            wxMapTreeAttr::iterator
                                it = m_attrs.find((void *)nmcd.dwItemSpec);

                            if ( it == m_attrs.end() )
                            {
                                // nothing to do for this item
                                *result = CDRF_DODEFAULT;
                                break;
                            }

                            wxTreeItemAttr * const attr = it->second;

                            wxTreeViewItem tvItem((void *)nmcd.dwItemSpec,
                                                  TVIF_STATE, TVIS_DROPHILITED);
                            DoGetItem(&tvItem);
                            const UINT tvItemState = tvItem.state;

                            // selection colours should override ours,
                            // otherwise it is too confusing to the user
                            if ( !(nmcd.uItemState & CDIS_SELECTED) &&
                                 !(tvItemState & TVIS_DROPHILITED) )
                            {
                                wxColour colBack;
                                if ( attr->HasBackgroundColour() )
                                {
                                    colBack = attr->GetBackgroundColour();
                                    lptvcd->clrTextBk = wxColourToRGB(colBack);
                                }
                            }

                            // but we still want to keep the special foreground
                            // colour when we don't have focus (we can't keep
                            // it when we do, it would usually be unreadable on
                            // the almost inverted bg colour...)
                            if ( ( !(nmcd.uItemState & CDIS_SELECTED) ||
                                    FindFocus() != this ) &&
                                 !(tvItemState & TVIS_DROPHILITED) )
                            {
                                wxColour colText;
                                if ( attr->HasTextColour() )
                                {
                                    colText = attr->GetTextColour();
                                    lptvcd->clrText = wxColourToRGB(colText);
                                }
                            }

                            if ( attr->HasFont() )
                            {
                                HFONT hFont = GetHfontOf(attr->GetFont());

                                ::SelectObject(nmcd.hdc, hFont);

                                *result = CDRF_NEWFONT;
                            }
                            else // no specific font
                            {
                                *result = CDRF_DODEFAULT;
                            }
                        }
                        break;

                    default:
                        *result = CDRF_DODEFAULT;
                }
            }

            // we always process it
            return true;
#endif // have owner drawn support in headers

        case NM_CLICK:
            {
                DWORD pos = GetMessagePos();
                POINT point;
                point.x = LOWORD(pos);
                point.y = HIWORD(pos);
                ::MapWindowPoints(HWND_DESKTOP, GetHwnd(), &point, 1);
                int flags = 0;
                wxTreeItemId item = HitTest(wxPoint(point.x, point.y), flags);
                if (flags & wxTREE_HITTEST_ONITEMSTATEICON)
                {
                    event.m_item = item;
                    eventType = wxEVT_COMMAND_TREE_STATE_IMAGE_CLICK;
                }
                break;
            }

        case NM_DBLCLK:
        case NM_RCLICK:
            {
                TV_HITTESTINFO tvhti;
                ::GetCursorPos(&tvhti.pt);
                ::ScreenToClient(GetHwnd(), &tvhti.pt);
                if ( TreeView_HitTest(GetHwnd(), &tvhti) )
                {
                    if ( tvhti.flags & TVHT_ONITEM )
                    {
                        event.m_item = tvhti.hItem;
                        eventType = (int)hdr->code == NM_DBLCLK
                                    ? wxEVT_COMMAND_TREE_ITEM_ACTIVATED
                                    : wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK;

                        event.m_pointDrag.x = tvhti.pt.x;
                        event.m_pointDrag.y = tvhti.pt.y;
                    }

                    break;
                }
            }
            // fall through

        default:
            return wxControl::MSWOnNotify(idCtrl, lParam, result);
    }

    event.SetEventType(eventType);

    if ( event.m_item.IsOk() )
        event.SetClientObject(GetItemData(event.m_item));

    bool processed = GetEventHandler()->ProcessEvent(event);

    // post processing
    switch ( hdr->code )
    {
        case NM_DBLCLK:
            // we translate NM_DBLCLK into ACTIVATED event, so don't interpret
            // the return code of this event handler as the return value for
            // NM_DBLCLK - otherwise, double clicking the item to toggle its
            // expanded status would never work
            *result = false;
            break;

        case NM_RCLICK:
            // prevent tree control from sending WM_CONTEXTMENU to our parent
            // (which it does if NM_RCLICK is not handled) because we want to
            // send it to the control itself
            *result =
            processed = true;

            ::SendMessage(GetHwnd(), WM_CONTEXTMENU,
                          (WPARAM)GetHwnd(), ::GetMessagePos());
            break;

        case TVN_BEGINDRAG:
        case TVN_BEGINRDRAG:
#if wxUSE_DRAGIMAGE
            if ( event.IsAllowed() )
            {
                // normally this is impossible because the m_dragImage is
                // deleted once the drag operation is over
                wxASSERT_MSG( !m_dragImage, _T("starting to drag once again?") );

                m_dragImage = new wxDragImage(*this, event.m_item);
                m_dragImage->BeginDrag(wxPoint(0,0), this);
                m_dragImage->Show();
            }
#endif // wxUSE_DRAGIMAGE
            break;

        case TVN_DELETEITEM:
            {
                // NB: we might process this message using wxWidgets event
                //     tables, but due to overhead of wxWin event system we
                //     prefer to do it here ourself (otherwise deleting a tree
                //     with many items is just too slow)
                NM_TREEVIEW *tv = (NM_TREEVIEW *)lParam;

                wxTreeItemParam *param =
                        (wxTreeItemParam *)tv->itemOld.lParam;
                delete param;

                processed = true; // Make sure we don't get called twice
            }
            break;

        case TVN_BEGINLABELEDIT:
            // return true to cancel label editing
            *result = !event.IsAllowed();

            // set ES_WANTRETURN ( like we do in BeginLabelEdit )
            if ( event.IsAllowed() )
            {
                HWND hText = TreeView_GetEditControl(GetHwnd());
                if ( hText )
                {
                    // MBN: if m_textCtrl already has an HWND, it is a stale
                    // pointer from a previous edit (because the user
                    // didn't modify the label before dismissing the control,
                    // and TVN_ENDLABELEDIT was not sent), so delete it
                    if ( m_textCtrl && m_textCtrl->GetHWND() )
                        DeleteTextCtrl();
                    if ( !m_textCtrl )
                        m_textCtrl = new wxTextCtrl();
                    m_textCtrl->SetParent(this);
                    m_textCtrl->SetHWND((WXHWND)hText);
                    m_textCtrl->SubclassWin((WXHWND)hText);

                    // set wxTE_PROCESS_ENTER style for the text control to
                    // force it to process the Enter presses itself, otherwise
                    // they could be stolen from it by the dialog
                    // navigation code
                    m_textCtrl->SetWindowStyle(m_textCtrl->GetWindowStyle()
                                               | wxTE_PROCESS_ENTER);
                }
            }
            else // we had set m_idEdited before
            {
                m_idEdited.Unset();
            }
            break;

        case TVN_ENDLABELEDIT:
            // return true to set the label to the new string: note that we
            // also must pretend that we did process the message or it is going
            // to be passed to DefWindowProc() which will happily return false
            // cancelling the label change
            *result = event.IsAllowed();
            processed = true;

            // ensure that we don't have the text ctrl which is going to be
            // deleted any more
            DeleteTextCtrl();
            break;

#ifndef __WXWINCE__
#ifdef TVN_GETINFOTIP
         case TVN_GETINFOTIP:
            {
                // If the user permitted a tooltip change, change it
                if (event.IsAllowed())
                {
                    SetToolTip(event.m_label);
                }
            }
            break;
#endif
#endif

        case TVN_SELCHANGING:
        case TVN_ITEMEXPANDING:
            // return true to prevent the action from happening
            *result = !event.IsAllowed();
            break;

        case TVN_ITEMEXPANDED:
            // the item is not refreshed properly after expansion when it has
            // an image depending on the expanded/collapsed state - bug in
            // comctl32.dll or our code?
            {
                NM_TREEVIEW *tv = (NM_TREEVIEW *)lParam;
                wxTreeItemId id(tv->itemNew.hItem);

                int image = GetItemImage(id, wxTreeItemIcon_Expanded);
                if ( image != -1 )
                {
                    RefreshItem(id);
                }
            }
            break;

        case TVN_GETDISPINFO:
            // NB: so far the user can't set the image himself anyhow, so do it
            //     anyway - but this may change later
            //if ( /* !processed && */ )
            {
                wxTreeItemId item = event.m_item;
                TV_DISPINFO *info = (TV_DISPINFO *)lParam;

                const wxTreeItemParam * const param = GetItemParam(item);
                if ( !param )
                    break;

                if ( info->item.mask & TVIF_IMAGE )
                {
                    info->item.iImage =
                        param->GetImage
                        (
                         IsExpanded(item) ? wxTreeItemIcon_Expanded
                                          : wxTreeItemIcon_Normal
                        );
                }
                if ( info->item.mask & TVIF_SELECTEDIMAGE )
                {
                    info->item.iSelectedImage =
                        param->GetImage
                        (
                         IsExpanded(item) ? wxTreeItemIcon_SelectedExpanded
                                          : wxTreeItemIcon_Selected
                        );
                }
            }
            break;

        //default:
            // for the other messages the return value is ignored and there is
            // nothing special to do
    }
    return processed;
}

// ----------------------------------------------------------------------------
// State control.
// ----------------------------------------------------------------------------

// why do they define INDEXTOSTATEIMAGEMASK but not the inverse?
#define STATEIMAGEMASKTOINDEX(state) (((state) & TVIS_STATEIMAGEMASK) >> 12)

void wxTreeCtrl::SetState(const wxTreeItemId& node, int state)
{
    TV_ITEM tvi;
    tvi.hItem = (HTREEITEM)node.m_pItem;
    tvi.mask = TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;

    // Select the specified state, or -1 == cycle to the next one.
    if ( state == -1 )
    {
        TreeView_GetItem(GetHwnd(), &tvi);

        state = STATEIMAGEMASKTOINDEX(tvi.state) + 1;
        if ( state == m_imageListState->GetImageCount() )
            state = 1;
    }

    wxCHECK_RET( state < m_imageListState->GetImageCount(),
                 _T("wxTreeCtrl::SetState(): item index out of bounds") );

    tvi.state = INDEXTOSTATEIMAGEMASK(state);

    TreeView_SetItem(GetHwnd(), &tvi);
}

int wxTreeCtrl::GetState(const wxTreeItemId& node)
{
    TV_ITEM tvi;
    tvi.hItem = (HTREEITEM)node.m_pItem;
    tvi.mask = TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;
    TreeView_GetItem(GetHwnd(), &tvi);

    return STATEIMAGEMASKTOINDEX(tvi.state);
}

#endif // wxUSE_TREECTRL
