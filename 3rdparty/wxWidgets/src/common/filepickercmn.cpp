///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/filepickercmn.cpp
// Purpose:     wxFilePickerCtrl class implementation
// Author:      Francesco Montorsi (readapted code written by Vadim Zeitlin)
// Modified by:
// Created:     15/04/2006
// RCS-ID:      $Id: filepickercmn.cpp 42219 2006-10-21 19:53:05Z PC $
// Copyright:   (c) Vadim Zeitlin, Francesco Montorsi
// Licence:     wxWindows licence
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

#if wxUSE_FILEPICKERCTRL || wxUSE_DIRPICKERCTRL

#include "wx/filepicker.h"
#include "wx/filename.h"

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
#endif

// ============================================================================
// implementation
// ============================================================================

const wxChar wxFilePickerCtrlNameStr[] = wxT("filepicker");
const wxChar wxFilePickerWidgetNameStr[] = wxT("filepickerwidget");
const wxChar wxDirPickerCtrlNameStr[] = wxT("dirpicker");
const wxChar wxDirPickerWidgetNameStr[] = wxT("dirpickerwidget");
const wxChar wxFilePickerWidgetLabel[] = wxT("Browse");
const wxChar wxDirPickerWidgetLabel[] = wxT("Browse");

DEFINE_EVENT_TYPE(wxEVT_COMMAND_FILEPICKER_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_DIRPICKER_CHANGED)
IMPLEMENT_DYNAMIC_CLASS(wxFileDirPickerEvent, wxCommandEvent)

// ----------------------------------------------------------------------------
// wxFileDirPickerCtrlBase
// ----------------------------------------------------------------------------

bool wxFileDirPickerCtrlBase::CreateBase(wxWindow *parent,
                                         wxWindowID id,
                                         const wxString &path,
                                         const wxString &message,
                                         const wxString &wildcard,
                                         const wxPoint &pos,
                                         const wxSize &size,
                                         long style,
                                         const wxValidator& validator,
                                         const wxString &name )
{
    wxASSERT_MSG(path.empty() || CheckPath(path), wxT("Invalid initial path!"));

    if (!wxPickerBase::CreateBase(parent, id, path, pos, size,
                                   style, validator, name))
        return false;

    if (!HasFlag(wxFLP_OPEN) && !HasFlag(wxFLP_SAVE))
        m_windowStyle |= wxFLP_OPEN;     // wxFD_OPEN is the default

    // check that the styles are not contradictory
    wxASSERT_MSG( !(HasFlag(wxFLP_SAVE) && HasFlag(wxFLP_OPEN)),
                  _T("can't specify both wxFLP_SAVE and wxFLP_OPEN at once") );

    wxASSERT_MSG( !HasFlag(wxFLP_SAVE) || !HasFlag(wxFLP_FILE_MUST_EXIST),
                   _T("wxFLP_FILE_MUST_EXIST can't be used with wxFLP_SAVE" ) );

    wxASSERT_MSG( !HasFlag(wxFLP_OPEN) || !HasFlag(wxFLP_OVERWRITE_PROMPT),
                  _T("wxFLP_OVERWRITE_PROMPT can't be used with wxFLP_OPEN") );

    // create a wxFilePickerWidget or a wxDirPickerWidget...
    m_pickerIface = CreatePicker(this, path, message, wildcard);
    if ( !m_pickerIface )
        return false;
    m_picker = m_pickerIface->AsControl();

    // complete sizer creation
    wxPickerBase::PostCreation();

    m_picker->Connect(GetEventType(),
            wxFileDirPickerEventHandler(wxFileDirPickerCtrlBase::OnFileDirChange),
            NULL, this);

    // default's wxPickerBase textctrl limit is too small for this control:
    // make it bigger
    if (m_text) m_text->SetMaxLength(512);

    return true;
}

wxString wxFileDirPickerCtrlBase::GetPath() const
{
    return m_pickerIface->GetPath();
}

void wxFileDirPickerCtrlBase::SetPath(const wxString &path)
{
    m_pickerIface->SetPath(path);
    UpdateTextCtrlFromPicker();
}

void wxFileDirPickerCtrlBase::UpdatePickerFromTextCtrl()
{
    wxASSERT(m_text);

    if (m_bIgnoreNextTextCtrlUpdate)
    {
        // ignore this update
        m_bIgnoreNextTextCtrlUpdate = false;
        return;
    }

    // remove the eventually present path-separator from the end of the textctrl
    // string otherwise we would generate a wxFileDirPickerEvent when changing
    // from e.g. /home/user to /home/user/ and we want to avoid it !
    wxString newpath(GetTextCtrlValue());
    if (!CheckPath(newpath))
        return;       // invalid user input

    if (m_pickerIface->GetPath() != newpath)
    {
        m_pickerIface->SetPath(newpath);

        // update current working directory, if necessary
        // NOTE: the path separator is required because if newpath is "C:"
        //       then no change would happen
        if (IsCwdToUpdate())
            wxSetWorkingDirectory(newpath);

        // fire an event
        wxFileDirPickerEvent event(GetEventType(), this, GetId(), newpath);
        GetEventHandler()->ProcessEvent(event);
    }
}

void wxFileDirPickerCtrlBase::UpdateTextCtrlFromPicker()
{
    if (!m_text)
        return;     // no textctrl to update

    // NOTE: this SetValue() will generate an unwanted wxEVT_COMMAND_TEXT_UPDATED
    //       which will trigger a unneeded UpdateFromTextCtrl(); thus before using
    //       SetValue() we set the m_bIgnoreNextTextCtrlUpdate flag...
    m_bIgnoreNextTextCtrlUpdate = true;
    m_text->SetValue(m_pickerIface->GetPath());
}



// ----------------------------------------------------------------------------
// wxFileDirPickerCtrlBase - event handlers
// ----------------------------------------------------------------------------

void wxFileDirPickerCtrlBase::OnFileDirChange(wxFileDirPickerEvent &ev)
{
    UpdateTextCtrlFromPicker();

    // the wxFilePickerWidget sent us a colour-change notification.
    // forward this event to our parent
    wxFileDirPickerEvent event(GetEventType(), this, GetId(), ev.GetPath());
    GetEventHandler()->ProcessEvent(event);
}

#endif  // wxUSE_FILEPICKERCTRL || wxUSE_DIRPICKERCTRL

// ----------------------------------------------------------------------------
// wxFileDirPickerCtrl
// ----------------------------------------------------------------------------

#if wxUSE_FILEPICKERCTRL

IMPLEMENT_DYNAMIC_CLASS(wxFilePickerCtrl, wxPickerBase)

bool wxFilePickerCtrl::CheckPath(const wxString& path) const
{
    // if wxFLP_SAVE was given or wxFLP_FILE_MUST_EXIST has NOT been given we
    // must accept any path
    return HasFlag(wxFLP_SAVE) ||
            !HasFlag(wxFLP_FILE_MUST_EXIST) ||
                wxFileName::FileExists(path);
}

wxString wxFilePickerCtrl::GetTextCtrlValue() const
{
    // filter it through wxFileName to remove any spurious path separator
    return wxFileName(m_text->GetValue()).GetFullPath();
}

#endif // wxUSE_FILEPICKERCTRL

// ----------------------------------------------------------------------------
// wxDirPickerCtrl
// ----------------------------------------------------------------------------

#if wxUSE_DIRPICKERCTRL
IMPLEMENT_DYNAMIC_CLASS(wxDirPickerCtrl, wxPickerBase)

bool wxDirPickerCtrl::CheckPath(const wxString& path) const
{
    // if wxDIRP_DIR_MUST_EXIST has NOT been given we must accept any path
    return !HasFlag(wxDIRP_DIR_MUST_EXIST) || wxFileName::DirExists(path);
}

wxString wxDirPickerCtrl::GetTextCtrlValue() const
{
    // filter it through wxFileName to remove any spurious path separator
    return wxFileName::DirName(m_text->GetValue()).GetPath();
}

#endif // wxUSE_DIRPICKERCTRL
