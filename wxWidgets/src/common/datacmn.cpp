///////////////////////////////////////////////////////////////////////////////
// Name:        common/datacmn.cpp
// Purpose:     contains definitions of various global wxWidgets variables
// Author:      Vadim Zeitlin
// Modified by:
// Created:     10.04.03 (from src/*/data.cpp files)
// RCS-ID:      $Id: datacmn.cpp 43874 2006-12-09 14:52:59Z VZ $
// Copyright:   (c) 1997-2002 wxWidgets development team
// License:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

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

#ifndef WX_PRECOMP
#endif // WX_PRECOMP

#include "wx/accel.h"

// ============================================================================
// implementation
// ============================================================================

// 'Null' objects
#if wxUSE_ACCEL
wxAcceleratorTable wxNullAcceleratorTable;
#endif // wxUSE_ACCEL

// Default window names
extern WXDLLEXPORT_DATA(const wxChar) wxButtonNameStr[] = wxT("button");
extern WXDLLEXPORT_DATA(const wxChar) wxCheckBoxNameStr[] = wxT("check");
extern WXDLLEXPORT_DATA(const wxChar) wxComboBoxNameStr[] = wxT("comboBox");
extern WXDLLEXPORT_DATA(const wxChar) wxDialogNameStr[] = wxT("dialog");
extern WXDLLEXPORT_DATA(const wxChar) wxFrameNameStr[] = wxT("frame");
extern WXDLLEXPORT_DATA(const wxChar) wxStaticBoxNameStr[] = wxT("groupBox");
extern WXDLLEXPORT_DATA(const wxChar) wxListBoxNameStr[] = wxT("listBox");
extern WXDLLEXPORT_DATA(const wxChar) wxStaticLineNameStr[] = wxT("staticLine");
extern WXDLLEXPORT_DATA(const wxChar) wxStaticTextNameStr[] = wxT("staticText");
extern WXDLLEXPORT_DATA(const wxChar) wxStaticBitmapNameStr[] = wxT("staticBitmap");
extern WXDLLEXPORT_DATA(const wxChar) wxNotebookNameStr[] = wxT("notebook");
extern WXDLLEXPORT_DATA(const wxChar) wxPanelNameStr[] = wxT("panel");
extern WXDLLEXPORT_DATA(const wxChar) wxRadioBoxNameStr[] = wxT("radioBox");
extern WXDLLEXPORT_DATA(const wxChar) wxRadioButtonNameStr[] = wxT("radioButton");
extern WXDLLEXPORT_DATA(const wxChar) wxBitmapRadioButtonNameStr[] = wxT("radioButton");
extern WXDLLEXPORT_DATA(const wxChar) wxScrollBarNameStr[] = wxT("scrollBar");
extern WXDLLEXPORT_DATA(const wxChar) wxSliderNameStr[] = wxT("slider");
extern WXDLLEXPORT_DATA(const wxChar) wxStatusLineNameStr[] = wxT("status_line");
extern WXDLLEXPORT_DATA(const wxChar) wxTextCtrlNameStr[] = wxT("text");
extern WXDLLEXPORT_DATA(const wxChar) wxTreeCtrlNameStr[] = wxT("treeCtrl");
extern WXDLLEXPORT_DATA(const wxChar) wxToolBarNameStr[] = wxT("toolbar");

// Default messages
extern WXDLLEXPORT_DATA(const wxChar) wxMessageBoxCaptionStr[] = wxT("Message");
extern WXDLLEXPORT_DATA(const wxChar) wxFileSelectorPromptStr[] = wxT("Select a file");
extern WXDLLEXPORT_DATA(const wxChar) wxDirSelectorPromptStr[] = wxT("Select a directory");

// Other default strings
extern WXDLLEXPORT_DATA(const wxChar) wxFileSelectorDefaultWildcardStr[] =
#if defined(__WXMSW__) || defined(__OS2__)
    wxT("*.*")
#else // Unix/Mac
    wxT("*")
#endif
    ;
extern WXDLLEXPORT_DATA(const wxChar) wxDirDialogNameStr[] = wxT("wxDirCtrl");
extern WXDLLEXPORT_DATA(const wxChar) wxDirDialogDefaultFolderStr[] = wxT("/");

extern WXDLLEXPORT_DATA(const wxChar) wxFileDialogNameStr[] = wxT("filedlg");
#if defined(__WXMSW__) || defined(__OS2__)
WXDLLEXPORT_DATA(const wxChar *) wxUserResourceStr = wxT("TEXT");
#endif
