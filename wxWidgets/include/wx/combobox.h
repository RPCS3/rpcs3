///////////////////////////////////////////////////////////////////////////////
// Name:        wx/combobox.h
// Purpose:     wxComboBox declaration
// Author:      Vadim Zeitlin
// Modified by:
// Created:     24.12.00
// RCS-ID:      $Id: combobox.h 42727 2006-10-30 16:04:27Z VZ $
// Copyright:   (c) 1996-2000 wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_COMBOBOX_H_BASE_
#define _WX_COMBOBOX_H_BASE_

#include "wx/defs.h"

#if wxUSE_COMBOBOX

extern WXDLLEXPORT_DATA(const wxChar) wxComboBoxNameStr[];

// ----------------------------------------------------------------------------
// wxComboBoxBase: this interface defines the methods wxComboBox must implement
// ----------------------------------------------------------------------------

#include "wx/textctrl.h"
#include "wx/ctrlsub.h"

class WXDLLEXPORT wxComboBoxBase : public wxItemContainer
{
public:
    // wxTextCtrl-like methods wxComboBox must implement
    virtual wxString GetValue() const = 0;
    virtual void SetValue(const wxString& value) = 0;

    virtual void Copy() = 0;
    virtual void Cut() = 0;
    virtual void Paste() = 0;
    virtual void SetInsertionPoint(long pos) = 0;
    virtual long GetInsertionPoint() const = 0;
    virtual wxTextPos GetLastPosition() const = 0;
    virtual void Replace(long from, long to, const wxString& value) = 0;
    virtual void SetSelection(long from, long to) = 0;
    virtual void SetEditable(bool editable) = 0;

    virtual void SetInsertionPointEnd()
        { SetInsertionPoint(GetLastPosition()); }
    virtual void Remove(long from, long to)
        { Replace(from, to, wxEmptyString); }

    virtual bool IsEditable() const = 0;

    virtual void Undo() = 0;
    virtual void Redo() = 0;
    virtual void SelectAll() = 0;

    virtual bool CanCopy() const = 0;
    virtual bool CanCut() const = 0;
    virtual bool CanPaste() const = 0;
    virtual bool CanUndo() const = 0;
    virtual bool CanRedo() const = 0;

    // may return value different from GetSelection() when the combobox
    // dropdown is shown and the user selected, but not yet accepted, a value
    // different from the old one in it
    virtual int GetCurrentSelection() const { return GetSelection(); }

    // redeclare inherited SetSelection() overload here as well to avoid
    // virtual function hiding
    virtual void SetSelection(int n) = 0;
};

// ----------------------------------------------------------------------------
// include the platform-dependent header defining the real class
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)
    #include "wx/univ/combobox.h"
#elif defined(__WXMSW__)
    #include "wx/msw/combobox.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/combobox.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/combobox.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/combobox.h"
#elif defined(__WXMAC__)
    #include "wx/mac/combobox.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/combobox.h"
#elif defined(__WXPM__)
    #include "wx/os2/combobox.h"
#endif

#endif // wxUSE_COMBOBOX

#endif
    // _WX_COMBOBOX_H_BASE_
