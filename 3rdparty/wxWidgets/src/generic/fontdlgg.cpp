/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/fontdlgg.cpp
// Purpose:     Generic font dialog
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: fontdlgg.cpp 39627 2006-06-08 06:57:39Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_FONTDLG && (!defined(__WXGTK__) || defined(__WXGPE__) || defined(__WXUNIVERSAL__))

#ifndef WX_PRECOMP
    #include <stdio.h>
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/listbox.h"
    #include "wx/button.h"
    #include "wx/stattext.h"
    #include "wx/layout.h"
    #include "wx/dcclient.h"
    #include "wx/choice.h"
    #include "wx/checkbox.h"
    #include "wx/intl.h"
    #include "wx/settings.h"
    #include "wx/cmndata.h"
    #include "wx/sizer.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "wx/fontdlg.h"
#include "wx/generic/fontdlgg.h"

#if USE_SPINCTRL_FOR_POINT_SIZE
#include "wx/spinctrl.h"
#endif

//-----------------------------------------------------------------------------
// helper class - wxFontPreviewer
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxFontPreviewer : public wxWindow
{
public:
    wxFontPreviewer(wxWindow *parent, const wxSize& sz = wxDefaultSize) : wxWindow(parent, wxID_ANY, wxDefaultPosition, sz)
    {
    }

private:
    void OnPaint(wxPaintEvent& event);
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxFontPreviewer, wxWindow)
    EVT_PAINT(wxFontPreviewer::OnPaint)
END_EVENT_TABLE()

void wxFontPreviewer::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    wxSize size = GetSize();
    wxFont font = GetFont();

    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(0, 0, size.x, size.y);

    if ( font.Ok() )
    {
        dc.SetFont(font);
        // Calculate vertical centre
        long w = 0, h = 0;
        dc.GetTextExtent( wxT("X"), &w, &h);
        dc.SetTextForeground(GetForegroundColour());
        dc.SetClippingRegion(2, 2, size.x-4, size.y-4);
        dc.DrawText(_("ABCDEFGabcdefg12345"),
                     10, size.y/2 - h/2);
        dc.DestroyClippingRegion();
    }
}

//-----------------------------------------------------------------------------
// wxGenericFontDialog
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGenericFontDialog, wxDialog)

BEGIN_EVENT_TABLE(wxGenericFontDialog, wxDialog)
    EVT_CHECKBOX(wxID_FONT_UNDERLINE, wxGenericFontDialog::OnChangeFont)
    EVT_CHOICE(wxID_FONT_STYLE, wxGenericFontDialog::OnChangeFont)
    EVT_CHOICE(wxID_FONT_WEIGHT, wxGenericFontDialog::OnChangeFont)
    EVT_CHOICE(wxID_FONT_FAMILY, wxGenericFontDialog::OnChangeFont)
    EVT_CHOICE(wxID_FONT_COLOUR, wxGenericFontDialog::OnChangeFont)
#if USE_SPINCTRL_FOR_POINT_SIZE
    EVT_SPINCTRL(wxID_FONT_SIZE, wxGenericFontDialog::OnChangeSize)
    EVT_TEXT(wxID_FONT_SIZE, wxGenericFontDialog::OnChangeFont)
#else
    EVT_CHOICE(wxID_FONT_SIZE, wxGenericFontDialog::OnChangeFont)
#endif
    EVT_CLOSE(wxGenericFontDialog::OnCloseWindow)
END_EVENT_TABLE()


#define NUM_COLS 48
static wxString wxColourDialogNames[NUM_COLS]={wxT("ORANGE"),
                    wxT("GOLDENROD"),
                    wxT("WHEAT"),
                    wxT("SPRING GREEN"),
                    wxT("SKY BLUE"),
                    wxT("SLATE BLUE"),
                    wxT("MEDIUM VIOLET RED"),
                    wxT("PURPLE"),

                    wxT("RED"),
                    wxT("YELLOW"),
                    wxT("MEDIUM SPRING GREEN"),
                    wxT("PALE GREEN"),
                    wxT("CYAN"),
                    wxT("LIGHT STEEL BLUE"),
                    wxT("ORCHID"),
                    wxT("LIGHT MAGENTA"),

                    wxT("BROWN"),
                    wxT("YELLOW"),
                    wxT("GREEN"),
                    wxT("CADET BLUE"),
                    wxT("MEDIUM BLUE"),
                    wxT("MAGENTA"),
                    wxT("MAROON"),
                    wxT("ORANGE RED"),

                    wxT("FIREBRICK"),
                    wxT("CORAL"),
                    wxT("FOREST GREEN"),
                    wxT("AQUARAMINE"),
                    wxT("BLUE"),
                    wxT("NAVY"),
                    wxT("THISTLE"),
                    wxT("MEDIUM VIOLET RED"),

                    wxT("INDIAN RED"),
                    wxT("GOLD"),
                    wxT("MEDIUM SEA GREEN"),
                    wxT("MEDIUM BLUE"),
                    wxT("MIDNIGHT BLUE"),
                    wxT("GREY"),
                    wxT("PURPLE"),
                    wxT("KHAKI"),

                    wxT("BLACK"),
                    wxT("MEDIUM FOREST GREEN"),
                    wxT("KHAKI"),
                    wxT("DARK GREY"),
                    wxT("SEA GREEN"),
                    wxT("LIGHT GREY"),
                    wxT("MEDIUM SLATE BLUE"),
                    wxT("WHITE")
                    };

/*
 * Generic wxFontDialog
 */

void wxGenericFontDialog::Init()
{
    m_useEvents = false;
    m_previewer = NULL;
    Create( m_parent ) ;
}

wxGenericFontDialog::~wxGenericFontDialog()
{
}

void wxGenericFontDialog::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
    EndModal(wxID_CANCEL);
}

bool wxGenericFontDialog::DoCreate(wxWindow *parent)
{
    if ( !wxDialog::Create( parent , wxID_ANY , _T("Choose Font") , wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE,
        _T("fontdialog") ) )
    {
        wxFAIL_MSG( wxT("wxFontDialog creation failed") );
        return false;
    }

    InitializeFont();
    CreateWidgets();

    // sets initial font in preview area
    DoChangeFont();

    return true;
}

int wxGenericFontDialog::ShowModal()
{
    int ret = wxDialog::ShowModal();

    if (ret != wxID_CANCEL)
    {
        m_fontData.m_chosenFont = m_dialogFont;
    }

    return ret;
}

// This should be application-settable
static bool ShowToolTips() { return false; }

void wxGenericFontDialog::CreateWidgets()
{
    wxString *families = new wxString[6],
             *styles = new wxString[3],
             *weights = new wxString[3];
    families[0] =  _("Roman");
    families[1] = _("Decorative");
    families[2] = _("Modern");
    families[3] = _("Script");
    families[4] = _("Swiss" );
    families[5] = _("Teletype" );
    styles[0] = _("Normal");
    styles[1] = _("Italic");
    styles[2] = _("Slant");
    weights[0] = _("Normal");
    weights[1] = _("Light");
    weights[2] = _("Bold");

#if !USE_SPINCTRL_FOR_POINT_SIZE
    wxString *pointSizes = new wxString[40];
    int i;
    for ( i = 0; i < 40; i++)
    {
        wxChar buf[5];
        wxSprintf(buf, wxT("%d"), i + 1);
        pointSizes[i] = buf;
    }
#endif

    // layout

    bool is_pda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);
    int noCols, noRows;
    if (is_pda)
    {
        noCols = 2; noRows = 3;
    }
    else
    {
        noCols = 3; noRows = 2;
    }

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(itemBoxSizer2);
    this->SetAutoLayout(true);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 5);

    wxFlexGridSizer* itemGridSizer4 = new wxFlexGridSizer(noRows, noCols, 0, 0);
    itemBoxSizer3->Add(itemGridSizer4, 0, wxGROW, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemGridSizer4->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxGROW, 5);
    wxStaticText* itemStaticText6 = new wxStaticText( this, wxID_STATIC, _("&Font family:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxChoice* itemChoice7 = new wxChoice( this, wxID_FONT_FAMILY, wxDefaultPosition, wxDefaultSize, 5, families, 0 );
    itemChoice7->SetHelpText(_("The font family."));
    if (ShowToolTips())
        itemChoice7->SetToolTip(_("The font family."));
    itemBoxSizer5->Add(itemChoice7, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    itemGridSizer4->Add(itemBoxSizer8, 0, wxALIGN_CENTER_HORIZONTAL|wxGROW, 5);
    wxStaticText* itemStaticText9 = new wxStaticText( this, wxID_STATIC, _("&Style:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxChoice* itemChoice10 = new wxChoice( this, wxID_FONT_STYLE, wxDefaultPosition, wxDefaultSize, 3, styles, 0 );
    itemChoice10->SetHelpText(_("The font style."));
    if (ShowToolTips())
        itemChoice10->SetToolTip(_("The font style."));
    itemBoxSizer8->Add(itemChoice10, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemGridSizer4->Add(itemBoxSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxGROW, 5);
    wxStaticText* itemStaticText12 = new wxStaticText( this, wxID_STATIC, _("&Weight:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxChoice* itemChoice13 = new wxChoice( this, wxID_FONT_WEIGHT, wxDefaultPosition, wxDefaultSize, 3, weights, 0 );
    itemChoice13->SetHelpText(_("The font weight."));
    if (ShowToolTips())
        itemChoice13->SetToolTip(_("The font weight."));
    itemBoxSizer11->Add(itemChoice13, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxVERTICAL);
    itemGridSizer4->Add(itemBoxSizer14, 0, wxALIGN_CENTER_HORIZONTAL|wxGROW, 5);
    if (m_fontData.GetEnableEffects())
    {
        wxStaticText* itemStaticText15 = new wxStaticText( this, wxID_STATIC, _("C&olour:"), wxDefaultPosition, wxDefaultSize, 0 );
        itemBoxSizer14->Add(itemStaticText15, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

        wxSize colourSize = wxDefaultSize;
        if (is_pda)
            colourSize.x = 100;

        wxChoice* itemChoice16 = new wxChoice( this, wxID_FONT_COLOUR, wxDefaultPosition, colourSize, NUM_COLS, wxColourDialogNames, 0 );
        itemChoice16->SetHelpText(_("The font colour."));
        if (ShowToolTips())
            itemChoice16->SetToolTip(_("The font colour."));
        itemBoxSizer14->Add(itemChoice16, 0, wxALIGN_LEFT|wxALL, 5);
    }

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxVERTICAL);
    itemGridSizer4->Add(itemBoxSizer17, 0, wxALIGN_CENTER_HORIZONTAL|wxGROW, 5);
    wxStaticText* itemStaticText18 = new wxStaticText( this, wxID_STATIC, _("&Point size:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText18, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

#if USE_SPINCTRL_FOR_POINT_SIZE
    wxSpinCtrl* spinCtrl = new wxSpinCtrl(this, wxID_FONT_SIZE, wxT("12"), wxDefaultPosition, wxSize(80, wxDefaultCoord), wxSP_ARROW_KEYS, 1, 500, 12);
    spinCtrl->SetHelpText(_("The font point size."));
    if (ShowToolTips())
        spinCtrl->SetToolTip(_("The font point size."));

    itemBoxSizer17->Add(spinCtrl, 0, wxALIGN_LEFT|wxALL, 5);
#else
    wxChoice* itemChoice19 = new wxChoice( this, wxID_FONT_SIZE, wxDefaultPosition, wxDefaultSize, 40, pointSizes, 0 );
    itemChoice19->SetHelpText(_("The font point size."));
    if (ShowToolTips())
        itemChoice19->SetToolTip(_("The font point size."));
    itemBoxSizer17->Add(itemChoice19, 0, wxALIGN_LEFT|wxALL, 5);
#endif

    if (m_fontData.GetEnableEffects())
    {
        wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxVERTICAL);
        itemGridSizer4->Add(itemBoxSizer20, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
        wxCheckBox* itemCheckBox21 = new wxCheckBox( this, wxID_FONT_UNDERLINE, _("&Underline"), wxDefaultPosition, wxDefaultSize, 0 );
        itemCheckBox21->SetValue(false);
        itemCheckBox21->SetHelpText(_("Whether the font is underlined."));
        if (ShowToolTips())
            itemCheckBox21->SetToolTip(_("Whether the font is underlined."));
        itemBoxSizer20->Add(itemCheckBox21, 0, wxALIGN_LEFT|wxALL, 5);
    }

    if (!is_pda)
        itemBoxSizer3->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* itemStaticText23 = new wxStaticText( this, wxID_STATIC, _("Preview:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText23, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxFontPreviewer* itemWindow24 = new wxFontPreviewer( this );
    m_previewer = itemWindow24;
    itemWindow24->SetHelpText(_("Shows the font preview."));
    if (ShowToolTips())
        itemWindow24->SetToolTip(_("Shows the font preview."));
    itemBoxSizer3->Add(itemWindow24, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer25, 0, wxGROW, 5);
    itemBoxSizer25->Add(5, 5, 1, wxGROW|wxALL, 5);

#ifdef __WXMAC__
    wxButton* itemButton28 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton28->SetToolTip(_("Click to cancel the font selection."));
    itemBoxSizer25->Add(itemButton28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton27 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton27->SetDefault();
    itemButton27->SetHelpText(_("Click to confirm the font selection."));
    if (ShowToolTips())
        itemButton27->SetToolTip(_("Click to confirm the font selection."));
    itemBoxSizer25->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
#else
    wxButton* itemButton27 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton27->SetDefault();
    itemButton27->SetHelpText(_("Click to confirm the font selection."));
    if (ShowToolTips())
        itemButton27->SetToolTip(_("Click to confirm the font selection."));
    itemBoxSizer25->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton28 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton28->SetToolTip(_("Click to cancel the font selection."));
    itemBoxSizer25->Add(itemButton28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
#endif

    m_familyChoice = (wxChoice*) FindWindow(wxID_FONT_FAMILY);
    m_styleChoice = (wxChoice*) FindWindow(wxID_FONT_STYLE);
    m_weightChoice = (wxChoice*) FindWindow(wxID_FONT_WEIGHT);
    m_colourChoice = (wxChoice*) FindWindow(wxID_FONT_COLOUR);
    m_underLineCheckBox = (wxCheckBox*) FindWindow(wxID_FONT_UNDERLINE);

    m_familyChoice->SetStringSelection( wxFontFamilyIntToString(m_dialogFont.GetFamily()) );
    m_styleChoice->SetStringSelection(wxFontStyleIntToString(m_dialogFont.GetStyle()));
    m_weightChoice->SetStringSelection(wxFontWeightIntToString(m_dialogFont.GetWeight()));

    if (m_colourChoice)
    {
        wxString name(wxTheColourDatabase->FindName(m_fontData.GetColour()));
        if (name.length())
            m_colourChoice->SetStringSelection(name);
        else
            m_colourChoice->SetStringSelection(wxT("BLACK"));
    }

    if (m_underLineCheckBox)
    {
        m_underLineCheckBox->SetValue(m_dialogFont.GetUnderlined());
    }

#if USE_SPINCTRL_FOR_POINT_SIZE
    spinCtrl->SetValue(m_dialogFont.GetPointSize());
#else
    m_pointSizeChoice = (wxChoice*) FindWindow(wxID_FONT_SIZE);
    m_pointSizeChoice->SetSelection(m_dialogFont.GetPointSize()-1);
#endif

    GetSizer()->SetItemMinSize(m_previewer, is_pda ? 100 : 430, is_pda ? 40 : 100);
    GetSizer()->SetSizeHints(this);
    GetSizer()->Fit(this);

    Centre(wxBOTH);

    delete[] families;
    delete[] styles;
    delete[] weights;
#if !USE_SPINCTRL_FOR_POINT_SIZE
    delete[] pointSizes;
#endif

    // Don't block events any more
    m_useEvents = true;

}

void wxGenericFontDialog::InitializeFont()
{
    int fontFamily = wxSWISS;
    int fontWeight = wxNORMAL;
    int fontStyle = wxNORMAL;
    int fontSize = 12;
    bool fontUnderline = false;

    if (m_fontData.m_initialFont.Ok())
    {
        fontFamily = m_fontData.m_initialFont.GetFamily();
        fontWeight = m_fontData.m_initialFont.GetWeight();
        fontStyle = m_fontData.m_initialFont.GetStyle();
        fontSize = m_fontData.m_initialFont.GetPointSize();
        fontUnderline = m_fontData.m_initialFont.GetUnderlined();
    }

    m_dialogFont = wxFont(fontSize, fontFamily, fontStyle,
                          fontWeight, fontUnderline);

    if (m_previewer)
        m_previewer->SetFont(m_dialogFont);
}

void wxGenericFontDialog::OnChangeFont(wxCommandEvent& WXUNUSED(event))
{
    DoChangeFont();
}

void wxGenericFontDialog::DoChangeFont()
{
    if (!m_useEvents) return;

    int fontFamily = wxFontFamilyStringToInt(WXSTRINGCAST m_familyChoice->GetStringSelection());
    int fontWeight = wxFontWeightStringToInt(WXSTRINGCAST m_weightChoice->GetStringSelection());
    int fontStyle = wxFontStyleStringToInt(WXSTRINGCAST m_styleChoice->GetStringSelection());
#if USE_SPINCTRL_FOR_POINT_SIZE
    wxSpinCtrl* fontSizeCtrl = wxDynamicCast(FindWindow(wxID_FONT_SIZE), wxSpinCtrl);
    int fontSize = fontSizeCtrl->GetValue();
#else
    int fontSize = wxAtoi(m_pointSizeChoice->GetStringSelection());
#endif

    // Start with previous underline setting, we want to retain it even if we can't edit it
    // m_dialogFont is always initialized because of the call to InitializeFont
    int fontUnderline = m_dialogFont.GetUnderlined();

    if (m_underLineCheckBox)
    {
        fontUnderline = m_underLineCheckBox->GetValue();
    }

    m_dialogFont = wxFont(fontSize, fontFamily, fontStyle, fontWeight, (fontUnderline != 0));
    m_previewer->SetFont(m_dialogFont);

    if ( m_colourChoice )
    {
        if ( !m_colourChoice->GetStringSelection().empty() )
        {
            wxColour col = wxTheColourDatabase->Find(m_colourChoice->GetStringSelection());
            if (col.Ok())
            {
                m_fontData.m_fontColour = col;
            }
        }
    }
    // Update color here so that we can also use the color originally passed in
    // (EnableEffects may be false)
    if (m_fontData.m_fontColour.Ok())
        m_previewer->SetForegroundColour(m_fontData.m_fontColour);

    m_previewer->Refresh();
}

#if USE_SPINCTRL_FOR_POINT_SIZE
void wxGenericFontDialog::OnChangeSize(wxSpinEvent& WXUNUSED(event))
{
    DoChangeFont();
}
#endif

const wxChar *wxFontWeightIntToString(int weight)
{
    switch (weight)
    {
        case wxLIGHT:
            return wxT("Light");
        case wxBOLD:
            return wxT("Bold");
        case wxNORMAL:
        default:
            return wxT("Normal");
    }
}

const wxChar *wxFontStyleIntToString(int style)
{
    switch (style)
    {
        case wxITALIC:
            return wxT("Italic");
        case wxSLANT:
            return wxT("Slant");
        case wxNORMAL:
            default:
            return wxT("Normal");
    }
}

const wxChar *wxFontFamilyIntToString(int family)
{
    switch (family)
    {
        case wxROMAN:
            return wxT("Roman");
        case wxDECORATIVE:
            return wxT("Decorative");
        case wxMODERN:
            return wxT("Modern");
        case wxSCRIPT:
            return wxT("Script");
        case wxTELETYPE:
            return wxT("Teletype");
        case wxSWISS:
        default:
            return wxT("Swiss");
    }
}

int wxFontFamilyStringToInt(wxChar *family)
{
    if (!family)
        return wxSWISS;

    if (wxStrcmp(family, wxT("Roman")) == 0)
        return wxROMAN;
    else if (wxStrcmp(family, wxT("Decorative")) == 0)
        return wxDECORATIVE;
    else if (wxStrcmp(family, wxT("Modern")) == 0)
        return wxMODERN;
    else if (wxStrcmp(family, wxT("Script")) == 0)
        return wxSCRIPT;
    else if (wxStrcmp(family, wxT("Teletype")) == 0)
        return wxTELETYPE;
    else return wxSWISS;
}

int wxFontStyleStringToInt(wxChar *style)
{
    if (!style)
        return wxNORMAL;
    if (wxStrcmp(style, wxT("Italic")) == 0)
        return wxITALIC;
    else if (wxStrcmp(style, wxT("Slant")) == 0)
        return wxSLANT;
    else
        return wxNORMAL;
}

int wxFontWeightStringToInt(wxChar *weight)
{
    if (!weight)
        return wxNORMAL;
    if (wxStrcmp(weight, wxT("Bold")) == 0)
        return wxBOLD;
    else if (wxStrcmp(weight, wxT("Light")) == 0)
        return wxLIGHT;
    else
        return wxNORMAL;
}

#endif
    // wxUSE_FONTDLG
