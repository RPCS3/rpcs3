/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/tbar95.h
// Purpose:     wxToolBar (Windows 95 toolbar) class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: tbar95.h 58446 2009-01-26 23:32:16Z VS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_TBAR95_H_
#define _WX_MSW_TBAR95_H_

#if wxUSE_TOOLBAR

#include "wx/dynarray.h"
#include "wx/imaglist.h"

class WXDLLEXPORT wxToolBar : public wxToolBarBase
{
public:
    // ctors and dtor
    wxToolBar() { Init(); }

    wxToolBar(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxNO_BORDER | wxTB_HORIZONTAL,
                const wxString& name = wxToolBarNameStr)
    {
        Init();

        Create(parent, id, pos, size, style, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxNO_BORDER | wxTB_HORIZONTAL,
                const wxString& name = wxToolBarNameStr);

    virtual ~wxToolBar();

    // override/implement base class virtuals
    virtual wxToolBarToolBase *FindToolForPosition(wxCoord x, wxCoord y) const;

    virtual bool Realize();

    virtual void SetToolBitmapSize(const wxSize& size);
    virtual wxSize GetToolSize() const;

    virtual void SetRows(int nRows);

#if wxABI_VERSION >= 20802
    // TODO: In 2.9 these should probably be virtual, and declared in the base class...
    void SetToolNormalBitmap(int id, const wxBitmap& bitmap);
    void SetToolDisabledBitmap(int id, const wxBitmap& bitmap);
#endif
    
    // implementation only from now on
    // -------------------------------

    virtual void SetWindowStyleFlag(long style);

    virtual bool MSWCommand(WXUINT param, WXWORD id);
    virtual bool MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result);

    void OnMouseEvent(wxMouseEvent& event);
    void OnSysColourChanged(wxSysColourChangedEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

    void SetFocus() {}

    static WXHBITMAP MapBitmap(WXHBITMAP bitmap, int width, int height);

    // override WndProc mainly to process WM_SIZE
    virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);

    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const;

protected:
    // common part of all ctors
    void Init();

    // create the native toolbar control
    bool MSWCreateToolbar(const wxPoint& pos, const wxSize& size);

    // recreate the control completely
    void Recreate();

    // implement base class pure virtuals
    virtual bool DoInsertTool(size_t pos, wxToolBarToolBase *tool);
    virtual bool DoDeleteTool(size_t pos, wxToolBarToolBase *tool);

    virtual void DoEnableTool(wxToolBarToolBase *tool, bool enable);
    virtual void DoToggleTool(wxToolBarToolBase *tool, bool toggle);
    virtual void DoSetToggle(wxToolBarToolBase *tool, bool toggle);

    virtual wxToolBarToolBase *CreateTool(int id,
                                          const wxString& label,
                                          const wxBitmap& bmpNormal,
                                          const wxBitmap& bmpDisabled,
                                          wxItemKind kind,
                                          wxObject *clientData,
                                          const wxString& shortHelp,
                                          const wxString& longHelp);
    virtual wxToolBarToolBase *CreateTool(wxControl *control);

    // return the appropriate size and flags for the toolbar control
    virtual wxSize DoGetBestSize() const;

    // handlers for various events
    bool HandleSize(WXWPARAM wParam, WXLPARAM lParam);
    bool HandlePaint(WXWPARAM wParam, WXLPARAM lParam);
    void HandleMouseMove(WXWPARAM wParam, WXLPARAM lParam);

    // should be called whenever the toolbar size changes
    void UpdateSize();

    // create m_disabledImgList (but doesn't fill it), set it to NULL if it is
    // unneeded
    void CreateDisabledImageList();

    // get the Windows toolbar style of this control
    long GetMSWToolbarStyle() const;


    // the big bitmap containing all bitmaps of the toolbar buttons
    WXHBITMAP m_hBitmap;

    // the image list with disabled images, may be NULL if we use
    // system-provided versions of them
    wxImageList *m_disabledImgList;

    // the total number of toolbar elements
    size_t m_nButtons;

    // the tool the cursor is in
    wxToolBarToolBase *m_pInTool;

private:
    // makes sure tool bitmap size is sufficient for all tools
    void AdjustToolBitmapSize();

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxToolBar)
    DECLARE_NO_COPY_CLASS(wxToolBar)
};

#endif // wxUSE_TOOLBAR

#endif // _WX_MSW_TBAR95_H_

