/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/tglbtn.cpp
// Purpose:     Definition of the wxToggleButton class, which implements a
//              toggle button under wxMSW.
// Author:      John Norris, minor changes by Axel Schlueter
//              and William Gallafent.
// Modified by:
// Created:     08.02.01
// RCS-ID:      $Id: tglbtn.cpp 41632 2006-10-04 23:02:13Z VZ $
// Copyright:   (c) 2000 Johnny C. Norris II
// License:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TOGGLEBTN

#include "wx/tglbtn.h"

#ifndef WX_PRECOMP
    #include "wx/button.h"
    #include "wx/brush.h"
    #include "wx/dcscreen.h"
    #include "wx/settings.h"

    #include "wx/log.h"
#endif // WX_PRECOMP

#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxToggleButton, wxControl)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TOGGLEBUTTON_CLICKED)

#define BUTTON_HEIGHT_FROM_CHAR_HEIGHT(cy) (11*EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy)/10)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxToggleButton
// ----------------------------------------------------------------------------

bool wxToggleButton::MSWCommand(WXUINT WXUNUSED(param), WXWORD WXUNUSED(id))
{
   wxCommandEvent event(wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, m_windowId);
   event.SetInt(GetValue());
   event.SetEventObject(this);
   ProcessCommand(event);
   return true;
}

// Single check box item
bool wxToggleButton::Create(wxWindow *parent, wxWindowID id,
                            const wxString& label,
                            const wxPoint& pos,
                            const wxSize& size, long style,
                            const wxValidator& validator,
                            const wxString& name)
{
    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    if ( !MSWCreateControl(wxT("BUTTON"), label, pos, size) )
      return false;

    return true;
}

wxBorder wxToggleButton::GetDefaultBorder() const
{
    return wxBORDER_NONE;
}

WXDWORD wxToggleButton::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    WXDWORD msStyle = wxControl::MSWGetStyle(style, exstyle);

#ifndef BS_PUSHLIKE
#define BS_PUSHLIKE 0x00001000L
#endif

    msStyle |= BS_AUTOCHECKBOX | BS_PUSHLIKE | WS_TABSTOP;

    if(style & wxBU_LEFT)
      msStyle |= BS_LEFT;
    if(style & wxBU_RIGHT)
      msStyle |= BS_RIGHT;
    if(style & wxBU_TOP)
      msStyle |= BS_TOP;
    if(style & wxBU_BOTTOM)
      msStyle |= BS_BOTTOM;

    return msStyle;
}

wxSize wxToggleButton::DoGetBestSize() const
{
   wxString label = wxGetWindowText(GetHWND());
   int wBtn;
   GetTextExtent(GetLabelText(label), &wBtn, NULL);

   int wChar, hChar;
   wxGetCharSize(GetHWND(), &wChar, &hChar, GetFont());

   // add a margin - the button is wider than just its label
   wBtn += 3*wChar;

   // the button height is proportional to the height of the font used
   int hBtn = BUTTON_HEIGHT_FROM_CHAR_HEIGHT(hChar);

#if wxUSE_BUTTON
   // make all buttons of at least standard size unless wxBU_EXACTFIT is given
   if ( !HasFlag(wxBU_EXACTFIT) )
   {
       const wxSize szMin = wxButton::GetDefaultSize();
       if ( wBtn < szMin.x )
           wBtn = szMin.x;
       if ( hBtn < szMin.y )
           hBtn = szMin.y;
   }
#endif // wxUSE_BUTTON

   wxSize sz(wBtn, hBtn);

   CacheBestSize(sz);
   return sz;
}

void wxToggleButton::SetValue(bool val)
{
   ::SendMessage(GetHwnd(), BM_SETCHECK, val, 0);
}

#ifndef BST_CHECKED
#define BST_CHECKED 0x0001
#endif

bool wxToggleButton::GetValue() const
{
#ifdef __WIN32__
   return (::SendMessage(GetHwnd(), BM_GETCHECK, 0, 0) == BST_CHECKED);
#else
   return ((0x001 & ::SendMessage(GetHwnd(), BM_GETCHECK, 0, 0)) == 0x001);
#endif
}

void wxToggleButton::Command(wxCommandEvent & event)
{
   SetValue((event.GetInt() != 0));
   ProcessCommand(event);
}

#endif // wxUSE_TOGGLEBTN
