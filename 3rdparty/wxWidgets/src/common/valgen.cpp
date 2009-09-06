/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/valgen.cpp
// Purpose:     wxGenericValidator class
// Author:      Kevin Smith
// Modified by:
// Created:     Jan 22 1999
// RCS-ID:      $Id: valgen.cpp 39463 2006-05-29 21:26:35Z ABX $
// Copyright:   (c) 1999 Kevin Smith
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_VALIDATORS

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/utils.h"
    #include "wx/intl.h"
    #include "wx/choice.h"
    #include "wx/combobox.h"
    #include "wx/radiobox.h"
    #include "wx/radiobut.h"
    #include "wx/checkbox.h"
    #include "wx/scrolbar.h"
    #include "wx/gauge.h"
    #include "wx/stattext.h"
    #include "wx/textctrl.h"
    #include "wx/button.h"
    #include "wx/listbox.h"
    #include "wx/slider.h"
    #include "wx/checklst.h"
#endif

#include "wx/spinctrl.h"

#if wxUSE_SPINBTN
    #include "wx/spinbutt.h"
#endif
#if wxUSE_TOGGLEBTN
    #include "wx/tglbtn.h"
#endif

#include "wx/valgen.h"

IMPLEMENT_CLASS(wxGenericValidator, wxValidator)

wxGenericValidator::wxGenericValidator(bool *val)
{
    Initialize();
    m_pBool = val;
}

wxGenericValidator::wxGenericValidator(int *val)
{
    Initialize();
    m_pInt = val;
}

wxGenericValidator::wxGenericValidator(wxString *val)
{
    Initialize();
    m_pString = val;
}

wxGenericValidator::wxGenericValidator(wxArrayInt *val)
{
    Initialize();
    m_pArrayInt = val;
}

wxGenericValidator::wxGenericValidator(const wxGenericValidator& val)
    : wxValidator()
{
    Copy(val);
}

bool wxGenericValidator::Copy(const wxGenericValidator& val)
{
    wxValidator::Copy(val);

    m_pBool = val.m_pBool;
    m_pInt = val.m_pInt;
    m_pString = val.m_pString;
    m_pArrayInt = val.m_pArrayInt;

    return true;
}

// Called to transfer data to the window
bool wxGenericValidator::TransferToWindow(void)
{
    if ( !m_validatorWindow )
        return false;

    // bool controls
#if wxUSE_CHECKBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckBox)) )
    {
        wxCheckBox* pControl = (wxCheckBox*) m_validatorWindow;
        if (m_pBool)
        {
            pControl->SetValue(*m_pBool);
            return true;
        }
    } else
#endif
#if wxUSE_RADIOBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioButton)) )
    {
        wxRadioButton* pControl = (wxRadioButton*) m_validatorWindow;
        if (m_pBool)
        {
            pControl->SetValue(*m_pBool) ;
            return true;
        }
    } else
#endif
#if wxUSE_TOGGLEBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxToggleButton)) )
    {
        wxToggleButton * pControl = (wxToggleButton *) m_validatorWindow;
        if (m_pBool)
        {
            pControl->SetValue(*m_pBool);
            return true;
        }
    } else
#endif

    // int controls
#if wxUSE_GAUGE
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxGauge)) )
    {
        wxGauge* pControl = (wxGauge*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetValue(*m_pInt);
            return true;
        }
    } else
#endif
#if wxUSE_RADIOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioBox)) )
    {
        wxRadioBox* pControl = (wxRadioBox*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetSelection(*m_pInt) ;
            return true;
        }
    } else
#endif
#if wxUSE_SCROLLBAR
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxScrollBar)) )
    {
        wxScrollBar* pControl = (wxScrollBar*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetThumbPosition(*m_pInt) ;
            return true;
        }
    } else
#endif
#if wxUSE_SPINCTRL && !defined(__WXMOTIF__)
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinCtrl)) )
    {
        wxSpinCtrl* pControl = (wxSpinCtrl*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetValue(*m_pInt);
            return true;
        }
    } else
#endif
#if wxUSE_SPINBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinButton)) )
    {
        wxSpinButton* pControl = (wxSpinButton*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetValue(*m_pInt) ;
            return true;
        }
    } else
#endif
#if wxUSE_SLIDER
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSlider)) )
    {
        wxSlider* pControl = (wxSlider*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetValue(*m_pInt) ;
            return true;
        }
    } else
#endif

    // string controls
#if wxUSE_BUTTON
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxButton)) )
    {
        wxButton* pControl = (wxButton*) m_validatorWindow;
        if (m_pString)
        {
            pControl->SetLabel(*m_pString) ;
            return true;
        }
    } else
#endif
#if wxUSE_COMBOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) )
    {
        wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetSelection(*m_pInt) ;
            return true;
        }
        else if (m_pString)
        {
            if (pControl->FindString(* m_pString) != wxNOT_FOUND)
            {
                pControl->SetStringSelection(* m_pString);
            }
            if ((m_validatorWindow->GetWindowStyle() & wxCB_READONLY) == 0)
            {
                pControl->SetValue(* m_pString);
            }
            return true;
        }
    } else
#endif
#if wxUSE_CHOICE
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxChoice)) )
    {
        wxChoice* pControl = (wxChoice*) m_validatorWindow;
        if (m_pInt)
        {
            pControl->SetSelection(*m_pInt) ;
            return true;
        }
        else if (m_pString)
        {
            if (pControl->FindString(* m_pString) != wxNOT_FOUND)
            {
                pControl->SetStringSelection(* m_pString);
            }
            return true;
        }
    } else
#endif
#if wxUSE_STATTEXT
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxStaticText)) )
    {
        wxStaticText* pControl = (wxStaticText*) m_validatorWindow;
        if (m_pString)
        {
            pControl->SetLabel(*m_pString) ;
            return true;
        }
    } else
#endif
#if wxUSE_TEXTCTRL
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) )
    {
        wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
        if (m_pString)
        {
            pControl->SetValue(*m_pString) ;
            return true;
        }
        else if (m_pInt)
        {
            wxString str;
            str.Printf(wxT("%d"), *m_pInt);
            pControl->SetValue(str);
            return true;
        }
    } else
#endif

    // array controls
#if wxUSE_CHECKLISTBOX
    // NOTE: wxCheckListBox is a wxListBox, so wxCheckListBox MUST come first:
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckListBox)) )
    {
        wxCheckListBox* pControl = (wxCheckListBox*) m_validatorWindow;
        if (m_pArrayInt)
        {
            // clear all selections
            size_t i,
                   count = pControl->GetCount();
            for ( i = 0 ; i < count; i++ )
                pControl->Check(i, false);

            // select each item in our array
            count = m_pArrayInt->GetCount();
            for ( i = 0 ; i < count; i++ )
                pControl->Check(m_pArrayInt->Item(i));

            return true;
        }
        else
            return false;
    } else
#endif
#if wxUSE_LISTBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxListBox)) )
    {
        wxListBox* pControl = (wxListBox*) m_validatorWindow;
        if (m_pArrayInt)
        {
            // clear all selections
            size_t i,
                   count = pControl->GetCount();
            for ( i = 0 ; i < count; i++ )
                pControl->Deselect(i);

            // select each item in our array
            count = m_pArrayInt->GetCount();
            for ( i = 0 ; i < count; i++ )
                pControl->SetSelection(m_pArrayInt->Item(i));

            return true;
        }
    } else
#endif
        ;   // to match the last 'else' above

  // unrecognized control, or bad pointer
  return false;
}

// Called to transfer data from the window
bool wxGenericValidator::TransferFromWindow(void)
{
    if ( !m_validatorWindow )
        return false;

    // BOOL CONTROLS **************************************
#if wxUSE_CHECKBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckBox)) )
    {
        wxCheckBox* pControl = (wxCheckBox*) m_validatorWindow;
        if (m_pBool)
        {
            *m_pBool = pControl->GetValue() ;
            return true;
        }
    } else
#endif
#if wxUSE_RADIOBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioButton)) )
    {
        wxRadioButton* pControl = (wxRadioButton*) m_validatorWindow;
        if (m_pBool)
        {
            *m_pBool = pControl->GetValue() ;
            return true;
        }
    } else
#endif
#if wxUSE_TOGGLEBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxToggleButton)) )
    {
        wxToggleButton *pControl = (wxToggleButton *) m_validatorWindow;
        if (m_pBool)
        {
            *m_pBool = pControl->GetValue() ;
            return true;
        }
    } else
#endif

    // INT CONTROLS ***************************************
#if wxUSE_GAUGE
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxGauge)) )
    {
        wxGauge* pControl = (wxGauge*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt = pControl->GetValue() ;
            return true;
        }
    } else
#endif
#if wxUSE_RADIOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioBox)) )
    {
        wxRadioBox* pControl = (wxRadioBox*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt = pControl->GetSelection() ;
            return true;
        }
    } else
#endif
#if wxUSE_SCROLLBAR
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxScrollBar)) )
    {
        wxScrollBar* pControl = (wxScrollBar*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt = pControl->GetThumbPosition() ;
            return true;
        }
    } else
#endif
#if wxUSE_SPINCTRL && !defined(__WXMOTIF__)
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinCtrl)) )
    {
        wxSpinCtrl* pControl = (wxSpinCtrl*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt=pControl->GetValue();
            return true;
        }
    } else
#endif
#if wxUSE_SPINBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinButton)) )
    {
        wxSpinButton* pControl = (wxSpinButton*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt = pControl->GetValue() ;
            return true;
        }
    } else
#endif
#if wxUSE_SLIDER
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSlider)) )
    {
        wxSlider* pControl = (wxSlider*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt = pControl->GetValue() ;
            return true;
        }
    } else
#endif

    // STRING CONTROLS ************************************
#if wxUSE_BUTTON
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxButton)) )
    {
        wxButton* pControl = (wxButton*) m_validatorWindow;
        if (m_pString)
        {
            *m_pString = pControl->GetLabel() ;
            return true;
        }
    } else
#endif
#if wxUSE_COMBOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) )
    {
        wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt = pControl->GetSelection() ;
            return true;
        }
        else if (m_pString)
        {
            if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY)
                *m_pString = pControl->GetStringSelection();
            else
                *m_pString = pControl->GetValue();
            return true;
        }
    } else
#endif
#if wxUSE_CHOICE
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxChoice)) )
    {
        wxChoice* pControl = (wxChoice*) m_validatorWindow;
        if (m_pInt)
        {
            *m_pInt = pControl->GetSelection() ;
            return true;
        }
        else if (m_pString)
        {
            *m_pString = pControl->GetStringSelection();
            return true;
        }
    } else
#endif
#if wxUSE_STATTEXT
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxStaticText)) )
    {
        wxStaticText* pControl = (wxStaticText*) m_validatorWindow;
        if (m_pString)
        {
            *m_pString = pControl->GetLabel() ;
            return true;
        }
    } else
#endif
#if wxUSE_TEXTCTRL
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) )
    {
        wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
        if (m_pString)
        {
            *m_pString = pControl->GetValue() ;
            return true;
        }
        else if (m_pInt)
        {
            *m_pInt = wxAtoi(pControl->GetValue());
            return true;
        }
    } else
#endif

    // ARRAY CONTROLS *************************************
#if wxUSE_CHECKLISTBOX
    // NOTE: wxCheckListBox isa wxListBox, so wxCheckListBox MUST come first:
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckListBox)) )
    {
        wxCheckListBox* pControl = (wxCheckListBox*) m_validatorWindow;
        if (m_pArrayInt)
        {
            // clear our array
            m_pArrayInt->Clear();

            // add each selected item to our array
            size_t i,
                   count = pControl->GetCount();
            for ( i = 0; i < count; i++ )
            {
                if (pControl->IsChecked(i))
                    m_pArrayInt->Add(i);
            }

            return true;
        }
        else
            return false;
    } else
#endif
#if wxUSE_LISTBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxListBox)) )
    {
        wxListBox* pControl = (wxListBox*) m_validatorWindow;
        if (m_pArrayInt)
        {
            // clear our array
            m_pArrayInt->Clear();

            // add each selected item to our array
            size_t i,
                   count = pControl->GetCount();
            for ( i = 0; i < count; i++ )
            {
                if (pControl->IsSelected(i))
                    m_pArrayInt->Add(i);
            }

            return true;
        }
    } else
#endif

    // unrecognized control, or bad pointer
        return false;

    return false;
}

/*
  Called by constructors to initialize ALL data members
*/
void wxGenericValidator::Initialize()
{
    m_pBool = 0;
    m_pInt = 0;
    m_pString = 0;
    m_pArrayInt = 0;
}

#endif
  // wxUSE_VALIDATORS
