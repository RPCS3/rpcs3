/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtextsymboldlg.cpp
// Purpose:
// Author:      Julian Smart
// Modified by:
// Created:     10/5/2006 3:11:58 PM
// RCS-ID:      $Id: richtextsymboldlg.cpp 63745 2010-03-23 08:59:44Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_RICHTEXT

#include "wx/richtext/richtextsymboldlg.h"

#ifndef WX_PRECOMP
    #include "wx/sizer.h"
    #include "wx/stattext.h"
    #include "wx/combobox.h"
    #include "wx/button.h"
    #include "wx/settings.h"
    #include "wx/icon.h"
    #include "wx/listbox.h"
#endif

#include "wx/dcbuffer.h"

// Only for cached font name
#include "wx/richtext/richtextctrl.h"

/* Microsoft Unicode subset numbering
 */

typedef enum
{
  U_BASIC_LATIN = 0,
  U_LATIN_1_SUPPLEMENT = 1,
  U_LATIN_EXTENDED_A = 2,
  U_LATIN_EXTENDED_B = 3,
  U_IPA_EXTENSIONS = 4,
  U_SPACING_MODIFIER_LETTERS = 5,
  U_COMBINING_DIACRITICAL_MARKS = 6,
  U_BASIC_GREEK = 7,
  U_GREEK_SYMBOLS_AND_COPTIC = 8,
  U_CYRILLIC = 9,
  U_ARMENIAN = 10,
  U_HEBREW_EXTENDED = 12,
  U_BASIC_HEBREW = 11,
  U_BASIC_ARABIC = 13,
  U_ARABIC_EXTENDED = 14,
  U_DEVANAGARI = 15,
  U_BENGALI = 16,
  U_GURMUKHI = 17,
  U_GUJARATI = 18,
  U_ORIYA = 19,
  U_TAMIL = 20,
  U_TELUGU = 21,
  U_KANNADA = 22,
  U_MALAYALAM = 23,
  U_THAI = 24,
  U_LAO = 25,
  U_GEORGIAN_EXTENDED = 27,
  U_BASIC_GEORGIAN = 26,
  U_HANGUL_JAMO = 28,
  U_LATIN_EXTENDED_ADDITIONAL = 29,
  U_GREEK_EXTENDED = 30,
  U_GENERAL_PUNCTUATION = 31,
  U_SUPERSCRIPTS_AND_SUBSCRIPTS = 32,
  U_CURRENCY_SYMBOLS = 33,
  U_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS = 34,
  U_LETTERLIKE_SYMBOLS = 35,
  U_NUMBER_FORMS = 36,
  U_ARROWS = 37,
  U_MATHEMATICAL_OPERATORS = 38,
  U_MISCELLANEOUS_TECHNICAL = 39,
  U_CONTROL_PICTURES = 40,
  U_OPTICAL_CHARACTER_RECOGNITION = 41,
  U_ENCLOSED_ALPHANUMERICS = 42,
  U_BOX_DRAWING = 43,
  U_BLOCK_ELEMENTS = 44,
  U_GEOMETRIC_SHAPES = 45,
  U_MISCELLANEOUS_SYMBOLS = 46,
  U_DINGBATS = 47,
  U_CJK_SYMBOLS_AND_PUNCTUATION = 48,
  U_HIRAGANA = 49,
  U_KATAKANA = 50,
  U_BOPOMOFO = 51,
  U_HANGUL_COMPATIBILITY_JAMO = 52,
  U_CJK_MISCELLANEOUS = 53,
  U_ENCLOSED_CJK = 54,
  U_CJK_COMPATIBILITY = 55,
  U_HANGUL = 56,
  U_HANGUL_SUPPLEMENTARY_A = 57,
  U_HANGUL_SUPPLEMENTARY_B = 58,
  U_CJK_UNIFIED_IDEOGRAPHS = 59,
  U_PRIVATE_USE_AREA = 60,
  U_CJK_COMPATIBILITY_IDEOGRAPHS = 61,
  U_ALPHABETIC_PRESENTATION_FORMS = 62,
  U_ARABIC_PRESENTATION_FORMS_A = 63,
  U_COMBINING_HALF_MARKS = 64,
  U_CJK_COMPATIBILITY_FORMS = 65,
  U_SMALL_FORM_VARIANTS = 66,
  U_ARABIC_PRESENTATION_FORMS_B = 67,
  U_SPECIALS = 69,
  U_HALFWIDTH_AND_FULLWIDTH_FORMS = 68,
  U_LAST_PLUS_ONE
} wxUnicodeSubsetCodes;

/* Unicode subsets */
#ifdef __UNICODE__

static struct
{
    int m_low, m_high;
    wxUnicodeSubsetCodes m_subset;
    const wxChar* m_name;
} g_UnicodeSubsetTable[] =
{
  { 0x0000, 0x007E,
    U_BASIC_LATIN, wxT("Basic Latin") },
  { 0x00A0, 0x00FF,
    U_LATIN_1_SUPPLEMENT, wxT("Latin-1 Supplement") },
  { 0x0100, 0x017F,
    U_LATIN_EXTENDED_A, wxT("Latin Extended-A") },
  { 0x0180, 0x024F,
    U_LATIN_EXTENDED_B, wxT("Latin Extended-B") },
  { 0x0250, 0x02AF,
    U_IPA_EXTENSIONS, wxT("IPA Extensions") },
  { 0x02B0, 0x02FF,
    U_SPACING_MODIFIER_LETTERS, wxT("Spacing Modifier Letters") },
  { 0x0300, 0x036F,
    U_COMBINING_DIACRITICAL_MARKS, wxT("Combining Diacritical Marks") },
  { 0x0370, 0x03CF,
    U_BASIC_GREEK, wxT("Basic Greek") },
  { 0x03D0, 0x03FF,
    U_GREEK_SYMBOLS_AND_COPTIC, wxT("Greek Symbols and Coptic") },
  { 0x0400, 0x04FF,
    U_CYRILLIC, wxT("Cyrillic") },
  { 0x0530, 0x058F,
    U_ARMENIAN, wxT("Armenian") },
  { 0x0590, 0x05CF,
    U_HEBREW_EXTENDED, wxT("Hebrew Extended") },
  { 0x05D0, 0x05FF,
    U_BASIC_HEBREW, wxT("Basic Hebrew") },
  { 0x0600, 0x0652,
    U_BASIC_ARABIC, wxT("Basic Arabic") },
  { 0x0653, 0x06FF,
    U_ARABIC_EXTENDED, wxT("Arabic Extended") },
  { 0x0900, 0x097F,
    U_DEVANAGARI, wxT("Devanagari") },
  { 0x0980, 0x09FF,
    U_BENGALI, wxT("Bengali") },
  { 0x0A00, 0x0A7F,
    U_GURMUKHI, wxT("Gurmukhi") },
  { 0x0A80, 0x0AFF,
    U_GUJARATI, wxT("Gujarati") },
  { 0x0B00, 0x0B7F,
    U_ORIYA, wxT("Oriya") },
  { 0x0B80, 0x0BFF,
    U_TAMIL, wxT("Tamil") },
  { 0x0C00, 0x0C7F,
    U_TELUGU, wxT("Telugu") },
  { 0x0C80, 0x0CFF,
    U_KANNADA, wxT("Kannada") },
  { 0x0D00, 0x0D7F,
    U_MALAYALAM, wxT("Malayalam") },
  { 0x0E00, 0x0E7F,
    U_THAI, wxT("Thai") },
  { 0x0E80, 0x0EFF,
    U_LAO, wxT("Lao") },
  { 0x10A0, 0x10CF,
    U_GEORGIAN_EXTENDED, wxT("Georgian Extended") },
  { 0x10D0, 0x10FF,
    U_BASIC_GEORGIAN, wxT("Basic Georgian") },
  { 0x1100, 0x11FF,
    U_HANGUL_JAMO, wxT("Hangul Jamo") },
  { 0x1E00, 0x1EFF,
    U_LATIN_EXTENDED_ADDITIONAL, wxT("Latin Extended Additional") },
  { 0x1F00, 0x1FFF,
    U_GREEK_EXTENDED, wxT("Greek Extended") },
  { 0x2000, 0x206F,
    U_GENERAL_PUNCTUATION, wxT("General Punctuation") },
  { 0x2070, 0x209F,
    U_SUPERSCRIPTS_AND_SUBSCRIPTS, wxT("Superscripts and Subscripts") },
  { 0x20A0, 0x20CF,
    U_CURRENCY_SYMBOLS, wxT("Currency Symbols") },
  { 0x20D0, 0x20FF,
    U_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS, wxT("Combining Diacritical Marks for Symbols") },
  { 0x2100, 0x214F,
    U_LETTERLIKE_SYMBOLS, wxT("Letterlike Symbols") },
  { 0x2150, 0x218F,
    U_NUMBER_FORMS, wxT("Number Forms") },
  { 0x2190, 0x21FF,
    U_ARROWS, wxT("Arrows") },
  { 0x2200, 0x22FF,
    U_MATHEMATICAL_OPERATORS, wxT("Mathematical Operators") },
  { 0x2300, 0x23FF,
    U_MISCELLANEOUS_TECHNICAL, wxT("Miscellaneous Technical") },
  { 0x2400, 0x243F,
    U_CONTROL_PICTURES, wxT("Control Pictures") },
  { 0x2440, 0x245F,
    U_OPTICAL_CHARACTER_RECOGNITION, wxT("Optical Character Recognition") },
  { 0x2460, 0x24FF,
    U_ENCLOSED_ALPHANUMERICS, wxT("Enclosed Alphanumerics") },
  { 0x2500, 0x257F,
    U_BOX_DRAWING, wxT("Box Drawing") },
  { 0x2580, 0x259F,
    U_BLOCK_ELEMENTS, wxT("Block Elements") },
  { 0x25A0, 0x25FF,
    U_GEOMETRIC_SHAPES, wxT("Geometric Shapes") },
  { 0x2600, 0x26FF,
    U_MISCELLANEOUS_SYMBOLS, wxT("Miscellaneous Symbols") },
  { 0x2700, 0x27BF,
    U_DINGBATS, wxT("Dingbats") },
  { 0x3000, 0x303F,
    U_CJK_SYMBOLS_AND_PUNCTUATION, wxT("CJK Symbols and Punctuation") },
  { 0x3040, 0x309F,
    U_HIRAGANA, wxT("Hiragana") },
  { 0x30A0, 0x30FF,
    U_KATAKANA, wxT("Katakana") },
  { 0x3100, 0x312F,
    U_BOPOMOFO, wxT("Bopomofo") },
  { 0x3130, 0x318F,
    U_HANGUL_COMPATIBILITY_JAMO, wxT("Hangul Compatibility Jamo") },
  { 0x3190, 0x319F,
    U_CJK_MISCELLANEOUS, wxT("CJK Miscellaneous") },
  { 0x3200, 0x32FF,
    U_ENCLOSED_CJK, wxT("Enclosed CJK") },
  { 0x3300, 0x33FF,
    U_CJK_COMPATIBILITY, wxT("CJK Compatibility") },
  { 0x3400, 0x4DB5,
    U_CJK_UNIFIED_IDEOGRAPHS, wxT("CJK Unified Ideographs Extension A") },
  { 0x4E00, 0x9FFF,
    U_CJK_UNIFIED_IDEOGRAPHS, wxT("CJK Unified Ideographs") },
  { 0xAC00, 0xD7A3,
    U_HANGUL, wxT("Hangul Syllables") },
  { 0xE000, 0xF8FF,
    U_PRIVATE_USE_AREA, wxT("Private Use Area") },
  { 0xF900, 0xFAFF,
    U_CJK_COMPATIBILITY_IDEOGRAPHS, wxT("CJK Compatibility Ideographs") },
  { 0xFB00, 0xFB4F,
    U_ALPHABETIC_PRESENTATION_FORMS, wxT("Alphabetic Presentation Forms") },
  { 0xFB50, 0xFDFF,
    U_ARABIC_PRESENTATION_FORMS_A, wxT("Arabic Presentation Forms-A") },
  { 0xFE20, 0xFE2F,
    U_COMBINING_HALF_MARKS, wxT("Combining Half Marks") },
  { 0xFE30, 0xFE4F,
    U_CJK_COMPATIBILITY_FORMS, wxT("CJK Compatibility Forms") },
  { 0xFE50, 0xFE6F,
    U_SMALL_FORM_VARIANTS, wxT("Small Form Variants") },
  { 0xFE70, 0xFEFE,
    U_ARABIC_PRESENTATION_FORMS_B, wxT("Arabic Presentation Forms-B") },
  { 0xFEFF, 0xFEFF,
    U_SPECIALS, wxT("Specials") },
  { 0xFF00, 0xFFEF,
    U_HALFWIDTH_AND_FULLWIDTH_FORMS, wxT("Halfwidth and Fullwidth Forms") },
  { 0xFFF0, 0xFFFD,
    U_SPECIALS, wxT("Specials") }
};

#endif // __UNICODE__

#if 0
// Not yet used, but could be used to test under Win32 whether this subset is available
// for the given font. The Win32 function is allegedly not accurate, however.
bool wxSubsetValidForFont(int subsetIndex, FONTSIGNATURE *fontSig)
{
    return (fontSig->fsUsb[g_UnicodeSubsetTable[subsetIndex].m_subset/32] & (1 << (g_UnicodeSubsetTable[subsetIndex].m_subset % 32)));
}
#endif

bool wxSymbolPickerDialog::sm_showToolTips = false;

/*!
 * wxSymbolPickerDialog type definition
 */

IMPLEMENT_DYNAMIC_CLASS( wxSymbolPickerDialog, wxDialog )

/*!
 * wxSymbolPickerDialog event table definition
 */

BEGIN_EVENT_TABLE( wxSymbolPickerDialog, wxDialog )
    EVT_LISTBOX(ID_SYMBOLPICKERDIALOG_LISTCTRL, wxSymbolPickerDialog::OnSymbolSelected)

////@begin wxSymbolPickerDialog event table entries
    EVT_COMBOBOX( ID_SYMBOLPICKERDIALOG_FONT, wxSymbolPickerDialog::OnFontCtrlSelected )

#if defined(__UNICODE__)
    EVT_COMBOBOX( ID_SYMBOLPICKERDIALOG_SUBSET, wxSymbolPickerDialog::OnSubsetSelected )
#endif

#if defined(__UNICODE__)
    EVT_COMBOBOX( ID_SYMBOLPICKERDIALOG_FROM, wxSymbolPickerDialog::OnFromUnicodeSelected )
#endif

#if defined(__WXMSW__) || defined(__WXGTK__) || defined(__WXPM__) || defined(__WXMGL__) || defined(__WXMOTIF__) || defined(__WXCOCOA__) || defined(__WXX11__) || defined(__WXPALMOS__)
    EVT_UPDATE_UI( wxID_OK, wxSymbolPickerDialog::OnOkUpdate )
#endif

#if defined(__WXMAC__)
    EVT_UPDATE_UI( wxID_OK, wxSymbolPickerDialog::OnOkUpdate )
#endif

////@end wxSymbolPickerDialog event table entries

END_EVENT_TABLE()

/*!
 * wxSymbolPickerDialog constructors
 */

wxSymbolPickerDialog::wxSymbolPickerDialog( )
{
    Init();
}

wxSymbolPickerDialog::wxSymbolPickerDialog( const wxString& symbol, const wxString& fontName, const wxString& normalTextFont, wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(symbol, fontName, normalTextFont, parent, id, caption, pos, size, style);
}

/*!
 * wxSymbolPickerDialog creator
 */

bool wxSymbolPickerDialog::Create( const wxString& symbol, const wxString& fontName, const wxString& normalTextFont, wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    m_fontName = fontName;
    m_normalTextFontName = normalTextFont;
    m_symbol = symbol;

////@begin wxSymbolPickerDialog creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS|wxDIALOG_EX_CONTEXTHELP);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end wxSymbolPickerDialog creation
    return true;
}

/*!
 * Member initialisation for wxSymbolPickerDialog
 */

void wxSymbolPickerDialog::Init()
{
////@begin wxSymbolPickerDialog member initialisation
    m_fromUnicode = true;
    m_fontCtrl = NULL;
#if defined(__UNICODE__)
    m_subsetCtrl = NULL;
#endif
    m_symbolsCtrl = NULL;
    m_symbolStaticCtrl = NULL;
    m_characterCodeCtrl = NULL;
#if defined(__UNICODE__)
    m_fromUnicodeCtrl = NULL;
#endif
////@end wxSymbolPickerDialog member initialisation
    m_dontUpdate = false;
}

/*!
 * Control creation for wxSymbolPickerDialog
 */

void wxSymbolPickerDialog::CreateControls()
{
#ifdef __WXMAC__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

////@begin wxSymbolPickerDialog content construction
    wxSymbolPickerDialog* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxGROW, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer5, 1, wxGROW, 5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemDialog1, wxID_STATIC, _("&Font:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxArrayString m_fontCtrlStrings;
    m_fontCtrl = new wxComboBox( itemDialog1, ID_SYMBOLPICKERDIALOG_FONT, wxEmptyString, wxDefaultPosition, wxSize(240, -1), m_fontCtrlStrings, wxCB_READONLY );
    m_fontCtrl->SetHelpText(_("The font from which to take the symbol."));
    if (wxSymbolPickerDialog::ShowToolTips())
        m_fontCtrl->SetToolTip(_("The font from which to take the symbol."));
    itemBoxSizer5->Add(m_fontCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer5->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

#if defined(__UNICODE__)
    wxStaticText* itemStaticText9 = new wxStaticText( itemDialog1, wxID_STATIC, _("&Subset:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

#endif

#if defined(__UNICODE__)
    wxArrayString m_subsetCtrlStrings;
    m_subsetCtrl = new wxComboBox( itemDialog1, ID_SYMBOLPICKERDIALOG_SUBSET, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_subsetCtrlStrings, wxCB_READONLY );
    m_subsetCtrl->SetHelpText(_("Shows a Unicode subset."));
    if (wxSymbolPickerDialog::ShowToolTips())
        m_subsetCtrl->SetToolTip(_("Shows a Unicode subset."));
    itemBoxSizer5->Add(m_subsetCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

#endif

    m_symbolsCtrl = new wxSymbolListCtrl( itemDialog1, ID_SYMBOLPICKERDIALOG_LISTCTRL, wxDefaultPosition, wxSize(500, 220), 0 );
    itemBoxSizer3->Add(m_symbolsCtrl, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer12, 0, wxGROW, 5);

    m_symbolStaticCtrl = new wxStaticText( itemDialog1, wxID_STATIC, _("xxxx"), wxDefaultPosition, wxSize(40, -1), wxALIGN_CENTRE );
    itemBoxSizer12->Add(m_symbolStaticCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    itemBoxSizer12->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText15 = new wxStaticText( itemDialog1, wxID_STATIC, _("&Character code:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    m_characterCodeCtrl = new wxTextCtrl( itemDialog1, ID_SYMBOLPICKERDIALOG_CHARACTERCODE, wxEmptyString, wxDefaultPosition, wxSize(140, -1), wxTE_READONLY|wxTE_CENTRE );
    m_characterCodeCtrl->SetHelpText(_("The character code."));
    if (wxSymbolPickerDialog::ShowToolTips())
        m_characterCodeCtrl->SetToolTip(_("The character code."));
    itemBoxSizer12->Add(m_characterCodeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer12->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

#if defined(__UNICODE__)
    wxStaticText* itemStaticText18 = new wxStaticText( itemDialog1, wxID_STATIC, _("&From:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

#endif

#if defined(__UNICODE__)
    wxArrayString m_fromUnicodeCtrlStrings;
    m_fromUnicodeCtrlStrings.Add(_("ASCII"));
    m_fromUnicodeCtrlStrings.Add(_("Unicode"));
    m_fromUnicodeCtrl = new wxComboBox( itemDialog1, ID_SYMBOLPICKERDIALOG_FROM, _("Unicode"), wxDefaultPosition, wxDefaultSize, m_fromUnicodeCtrlStrings, wxCB_READONLY );
    m_fromUnicodeCtrl->SetStringSelection(_("Unicode"));
    m_fromUnicodeCtrl->SetHelpText(_("The range to show."));
    if (wxSymbolPickerDialog::ShowToolTips())
        m_fromUnicodeCtrl->SetToolTip(_("The range to show."));
    itemBoxSizer12->Add(m_fromUnicodeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

#endif

#if defined(__WXMSW__) || defined(__WXGTK__) || defined(__WXPM__) || defined(__WXMGL__) || defined(__WXMOTIF__) || defined(__WXCOCOA__) || defined(__WXX11__) || defined(__WXPALMOS__)
    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer20, 0, wxGROW, 5);

    itemBoxSizer20->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton22 = new wxButton( itemDialog1, wxID_OK, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton22->SetDefault();
    itemButton22->SetHelpText(_("Inserts the chosen symbol."));
    if (wxSymbolPickerDialog::ShowToolTips())
        itemButton22->SetToolTip(_("Inserts the chosen symbol."));
    itemBoxSizer20->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton23 = new wxButton( itemDialog1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton23->SetHelpText(_("Closes the dialog without inserting a symbol."));
    if (wxSymbolPickerDialog::ShowToolTips())
        itemButton23->SetToolTip(_("Closes the dialog without inserting a symbol."));
    itemBoxSizer20->Add(itemButton23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

#endif

#if defined(__WXMAC__)
    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer24, 0, wxGROW, 5);

    itemBoxSizer24->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton26 = new wxButton( itemDialog1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton26->SetHelpText(_("Closes the dialog without inserting a symbol."));
    if (wxSymbolPickerDialog::ShowToolTips())
        itemButton26->SetToolTip(_("Closes the dialog without inserting a symbol."));
    itemBoxSizer24->Add(itemButton26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton27 = new wxButton( itemDialog1, wxID_OK, _("Insert"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton27->SetDefault();
    itemButton27->SetHelpText(_("Inserts the chosen symbol."));
    if (wxSymbolPickerDialog::ShowToolTips())
        itemButton27->SetToolTip(_("Inserts the chosen symbol."));
    itemBoxSizer24->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

#endif

////@end wxSymbolPickerDialog content construction

}

/// Data transfer
bool wxSymbolPickerDialog::TransferDataToWindow()
{
    m_dontUpdate = true;

    if (m_fontCtrl->GetCount() == 0)
    {
        wxArrayString faceNames = wxRichTextCtrl::GetAvailableFontNames();
        faceNames.Sort();

        faceNames.Insert(_("(Normal text)"), 0);
        m_fontCtrl->Append(faceNames);
    }

    if (m_fontName.empty())
        m_fontCtrl->SetSelection(0);
    else
    {
        if (m_fontCtrl->FindString(m_fontName) != wxNOT_FOUND)
            m_fontCtrl->SetStringSelection(m_fontName);
        else
            m_fontCtrl->SetSelection(0);
    }

#if defined(__UNICODE__)
    if (m_subsetCtrl->GetCount() == 0)
    {
        // Insert items into subset combo
        int i;
        for (i = 0; i < (int) (sizeof(g_UnicodeSubsetTable)/sizeof(g_UnicodeSubsetTable[0])); i++)
        {
            m_subsetCtrl->Append(g_UnicodeSubsetTable[i].m_name);
        }
        m_subsetCtrl->SetSelection(0);
    }
#endif

#if defined(__UNICODE__)
    m_symbolsCtrl->SetUnicodeMode(m_fromUnicode);
#endif

    if (!m_symbol.empty())
    {
        int sel = (int) m_symbol[0];
        m_symbolsCtrl->SetSelection(sel);
    }

    UpdateSymbolDisplay();

    m_dontUpdate = false;

    return true;
}

void wxSymbolPickerDialog::UpdateSymbolDisplay(bool updateSymbolList, bool showAtSubset)
{
    wxFont font;
    wxString fontNameToUse;
    if (m_fontName.empty())
        fontNameToUse = m_normalTextFontName;
    else
        fontNameToUse = m_fontName;

    if (!fontNameToUse.empty())
    {
        font = wxFont(14, wxDEFAULT, wxNORMAL, wxNORMAL, false, fontNameToUse);
    }
    else
        font = *wxNORMAL_FONT;

    if (updateSymbolList)
    {
        m_symbolsCtrl->SetFont(font);
    }

    if (!m_symbol.empty())
    {
        m_symbolStaticCtrl->SetFont(font);
        m_symbolStaticCtrl->SetLabel(m_symbol);

        int symbol = (int) m_symbol[0];
        m_characterCodeCtrl->SetValue(wxString::Format(wxT("%X hex (%d dec)"), symbol, symbol));
    }
    else
    {
        m_symbolStaticCtrl->SetLabel(wxEmptyString);
        m_characterCodeCtrl->SetValue(wxEmptyString);
    }

#if defined(__UNICODE__)
    if (showAtSubset)
        ShowAtSubset();
#else
    wxUnusedVar(showAtSubset);
#endif
}

/// Show at the current subset selection
void wxSymbolPickerDialog::ShowAtSubset()
{
#if defined(__UNICODE__)
    if (m_fromUnicode)
    {
        int sel = m_subsetCtrl->GetSelection();
        int low = g_UnicodeSubsetTable[sel].m_low;
        m_symbolsCtrl->EnsureVisible(low);
    }
#endif
}

// Handle font selection
void wxSymbolPickerDialog::OnFontCtrlSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (m_fontCtrl->GetSelection() == 0)
        m_fontName = wxEmptyString;
    else
        m_fontName = m_fontCtrl->GetStringSelection();

    UpdateSymbolDisplay();
}

/// Respond to symbol selection
void wxSymbolPickerDialog::OnSymbolSelected( wxCommandEvent& event )
{
    if (m_dontUpdate)
        return;

    int sel = event.GetSelection();
    if (sel == wxNOT_FOUND)
        m_symbol = wxEmptyString;
    else
    {
        m_symbol = wxEmptyString;
        m_symbol << (wxChar) sel;
    }

#if defined(__UNICODE__)
    if (sel != -1 && m_fromUnicode)
    {
        // Need to make the subset selection reflect the current symbol
        int i;
        for (i = 0; i < (int) (sizeof(g_UnicodeSubsetTable)/sizeof(g_UnicodeSubsetTable[0])); i++)
        {
            if (sel >= g_UnicodeSubsetTable[i].m_low && sel <= g_UnicodeSubsetTable[i].m_high)
            {
                m_dontUpdate = true;
                m_subsetCtrl->SetSelection(i);
                m_dontUpdate = false;
                break;
            }
        }
    }
#endif

    UpdateSymbolDisplay(false, false);
}

#if defined(__UNICODE__)
// Handle Unicode/ASCII selection
void wxSymbolPickerDialog::OnFromUnicodeSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (m_dontUpdate)
        return;

    m_fromUnicode = (m_fromUnicodeCtrl->GetSelection() == 1);
    m_symbolsCtrl->SetUnicodeMode(m_fromUnicode);
    UpdateSymbolDisplay(false);
}

// Handle subset selection
void wxSymbolPickerDialog::OnSubsetSelected( wxCommandEvent& WXUNUSED(event) )
{
    if (m_dontUpdate)
        return;

    ShowAtSubset();
}
#endif

/*!
 * wxEVT_UPDATE_UI event handler for wxID_OK
 */

void wxSymbolPickerDialog::OnOkUpdate( wxUpdateUIEvent& event )
{
    event.Enable(HasSelection());
}

/// Set Unicode mode
void wxSymbolPickerDialog::SetUnicodeMode(bool unicodeMode)
{
#if defined(__UNICODE__)
    m_dontUpdate = true;
    m_fromUnicode = unicodeMode;
    if (m_fromUnicodeCtrl)
        m_fromUnicodeCtrl->SetSelection(m_fromUnicode ? 1 : 0);
    UpdateSymbolDisplay();
    m_dontUpdate = false;
#else
    wxUnusedVar(unicodeMode);
#endif
}

/// Get the selected symbol character
int wxSymbolPickerDialog::GetSymbolChar() const
{
    if (m_symbol.empty())
        return -1;
    else
        return (int) m_symbol[0];
}


/*!
 * Get bitmap resources
 */

wxBitmap wxSymbolPickerDialog::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin wxSymbolPickerDialog bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end wxSymbolPickerDialog bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon wxSymbolPickerDialog::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin wxSymbolPickerDialog icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end wxSymbolPickerDialog icon retrieval
}

/*!
 * The scrolling symbol list.
 */

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxSymbolListCtrl, wxVScrolledWindow)
    EVT_PAINT(wxSymbolListCtrl::OnPaint)
    EVT_SIZE(wxSymbolListCtrl::OnSize)

    EVT_KEY_DOWN(wxSymbolListCtrl::OnKeyDown)
    EVT_LEFT_DOWN(wxSymbolListCtrl::OnLeftDown)
    EVT_LEFT_DCLICK(wxSymbolListCtrl::OnLeftDClick)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_ABSTRACT_CLASS(wxSymbolListCtrl, wxVScrolledWindow)

// ----------------------------------------------------------------------------
// wxSymbolListCtrl creation
// ----------------------------------------------------------------------------

void wxSymbolListCtrl::Init()
{
    m_current = wxNOT_FOUND;
    m_doubleBuffer = NULL;
    m_cellSize = wxSize(40, 40);
    m_minSymbolValue = 0;
    m_maxSymbolValue = 255;
    m_symbolsPerLine = 0;
    m_unicodeMode = false;
}

bool wxSymbolListCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    style |= wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE;

    if ((style & wxBORDER_MASK) == wxBORDER_DEFAULT)
#ifdef __WXMSW__
        style |= GetThemedBorderStyle();
#else
        style |= wxBORDER_SUNKEN;
#endif

    if ( !wxVScrolledWindow::Create(parent, id, pos, size, style, name) )
        return false;

    // make sure the native widget has the right colour since we do
    // transparent drawing by default
    SetBackgroundColour(GetBackgroundColour());
    m_colBgSel = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

    // flicker-free drawing requires this
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);

    SetFont(*wxNORMAL_FONT);

    SetupCtrl();

    return true;
}

wxSymbolListCtrl::~wxSymbolListCtrl()
{
    delete m_doubleBuffer;
}

// ----------------------------------------------------------------------------
// selection handling
// ----------------------------------------------------------------------------

bool wxSymbolListCtrl::IsSelected(int item) const
{
    return item == m_current;
}

bool wxSymbolListCtrl::DoSetCurrent(int current)
{
    wxASSERT_MSG( current == wxNOT_FOUND ||
                    (current >= m_minSymbolValue && current <= m_maxSymbolValue),
                  _T("wxSymbolListCtrl::DoSetCurrent(): invalid symbol value") );

    if ( current == m_current )
    {
        // nothing to do
        return false;
    }

    if ( m_current != wxNOT_FOUND )
        RefreshLine(SymbolValueToLineNumber(m_current));

    m_current = current;

    if ( m_current != wxNOT_FOUND )
    {
        int lineNo = SymbolValueToLineNumber(m_current);

        // if the line is not visible at all, we scroll it into view but we
        // don't need to refresh it -- it will be redrawn anyhow
        if ( !IsVisible(lineNo) )
        {
            ScrollToLine(lineNo);
        }
        else // line is at least partly visible
        {
            // it is, indeed, only partly visible, so scroll it into view to
            // make it entirely visible
            while ( unsigned(lineNo) == GetLastVisibleLine() &&
                    ScrollToLine(GetVisibleBegin()+1) )
                ;

            // but in any case refresh it as even if it was only partly visible
            // before we need to redraw it entirely as its background changed
            RefreshLine(lineNo);
        }
    }

    return true;
}

void wxSymbolListCtrl::SendSelectedEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_LISTBOX_SELECTED, GetId());
    event.SetEventObject(this);
    event.SetInt(m_current);

    (void)GetEventHandler()->ProcessEvent(event);
}

void wxSymbolListCtrl::SetSelection(int selection)
{
    wxCHECK_RET( selection == wxNOT_FOUND ||
                  (selection >= m_minSymbolValue && selection < m_maxSymbolValue),
                  _T("wxSymbolListCtrl::SetSelection(): invalid symbol value") );

    DoSetCurrent(selection);
}

// ----------------------------------------------------------------------------
// wxSymbolListCtrl appearance parameters
// ----------------------------------------------------------------------------

void wxSymbolListCtrl::SetMargins(const wxPoint& pt)
{
    if ( pt != m_ptMargins )
    {
        m_ptMargins = pt;

        Refresh();
    }
}

void wxSymbolListCtrl::SetSelectionBackground(const wxColour& col)
{
    m_colBgSel = col;
}

// ----------------------------------------------------------------------------
// wxSymbolListCtrl painting
// ----------------------------------------------------------------------------

wxCoord wxSymbolListCtrl::OnGetLineHeight(size_t WXUNUSED(line)) const
{
    return m_cellSize.y + 2*m_ptMargins.y + 1 /* for divider */ ;
}

// draws a line of symbols
void wxSymbolListCtrl::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
    wxColour oldTextColour = dc.GetTextForeground();
    int startSymbol = n*m_symbolsPerLine;

    int i;
    for (i = 0; i < m_symbolsPerLine; i++)
    {
        bool resetColour = false;
        int symbol = startSymbol+i;
        if (symbol == m_current)
        {
            dc.SetBrush(wxBrush(m_colBgSel));

            dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
            resetColour = true;

            wxPen oldPen = dc.GetPen();
            dc.SetPen(*wxTRANSPARENT_PEN);

            dc.DrawRectangle(rect.x + i*m_cellSize.x, rect.y, m_cellSize.x, rect.y+rect.height);
            dc.SetPen(oldPen);
        }

        // Don't draw first line
        if (i != 0)
            dc.DrawLine(rect.x + i*m_cellSize.x, rect.y, i*m_cellSize.x, rect.y+rect.height);

        if (symbol >= m_minSymbolValue && symbol <= m_maxSymbolValue)
        {
            wxString text;
            text << (wxChar) symbol;

            wxCoord w, h;
            dc.GetTextExtent(text, & w, & h);

            int x = rect.x + i*m_cellSize.x + (m_cellSize.x - w)/2;
            int y = rect.y + (m_cellSize.y - h)/2;
            dc.DrawText(text, x, y);
        }

        if (resetColour)
            dc.SetTextForeground(oldTextColour);
    }

    // Draw horizontal separator line
    dc.DrawLine(rect.x, rect.y+rect.height-1, rect.x+rect.width, rect.y+rect.height-1);
}

void wxSymbolListCtrl::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    // If size is larger, recalculate double buffer bitmap
    wxSize clientSize = GetClientSize();

    if ( !m_doubleBuffer ||
         clientSize.x > m_doubleBuffer->GetWidth() ||
         clientSize.y > m_doubleBuffer->GetHeight() )
    {
        delete m_doubleBuffer;
        m_doubleBuffer = new wxBitmap(clientSize.x+25,clientSize.y+25);
    }

    wxBufferedPaintDC dc(this,*m_doubleBuffer);

    // the update rectangle
    wxRect rectUpdate = GetUpdateClientRect();

    // fill it with background colour
    dc.SetBackground(GetBackgroundColour());
    dc.Clear();

    // set the font to be displayed
    dc.SetFont(GetFont());

    // the bounding rectangle of the current line
    wxRect rectLine;
    rectLine.width = clientSize.x;

    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)));
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    dc.SetBackgroundMode(wxTRANSPARENT);

    // iterate over all visible lines
    const size_t lineMax = GetVisibleEnd();
    for ( size_t line = GetFirstVisibleLine(); line < lineMax; line++ )
    {
        const wxCoord hLine = OnGetLineHeight(line);

        rectLine.height = hLine;

        // and draw the ones which intersect the update rect
        if ( rectLine.Intersects(rectUpdate) )
        {
            // don't allow drawing outside of the lines rectangle
            wxDCClipper clip(dc, rectLine);

            wxRect rect = rectLine;
            rect.Deflate(m_ptMargins.x, m_ptMargins.y);
            OnDrawItem(dc, rect, line);
        }
        else // no intersection
        {
            if ( rectLine.GetTop() > rectUpdate.GetBottom() )
            {
                // we are already below the update rect, no need to continue
                // further
                break;
            }
            //else: the next line may intersect the update rect
        }

        rectLine.y += hLine;
    }
}

// ============================================================================
// wxSymbolListCtrl keyboard/mouse handling
// ============================================================================

void wxSymbolListCtrl::DoHandleItemClick(int item, int WXUNUSED(flags))
{
    if (m_current != item)
    {
        m_current = item;
        Refresh();
        SendSelectedEvent();
    }
}

// ----------------------------------------------------------------------------
// keyboard handling
// ----------------------------------------------------------------------------

void wxSymbolListCtrl::OnKeyDown(wxKeyEvent& event)
{
    // No keyboard interface for now
    event.Skip();
#if 0
    // flags for DoHandleItemClick()
    int flags = ItemClick_Kbd;

    int currentLineNow = SymbolValueToLineNumber(m_current);

    int currentLine;
    switch ( event.GetKeyCode() )
    {
        case WXK_HOME:
            currentLine = 0;
            break;

        case WXK_END:
            currentLine = GetLineCount() - 1;
            break;

        case WXK_DOWN:
            if ( currentLineNow == (int)GetLineCount() - 1 )
                return;

            currentLine = currentLineNow + 1;
            break;

        case WXK_UP:
            if ( m_current == wxNOT_FOUND )
                currentLine = GetLineCount() - 1;
            else if ( currentLineNow != 0 )
                currentLine = currentLineNow - 1;
            else // currentLineNow == 0
                return;
            break;

        case WXK_PAGEDOWN:
            PageDown();
            currentLine = GetFirstVisibleLine();
            break;

        case WXK_PAGEUP:
            if ( currentLineNow == (int)GetFirstVisibleLine() )
            {
                PageUp();
            }

            currentLine = GetFirstVisibleLine();
            break;

        case WXK_SPACE:
            // hack: pressing space should work like a mouse click rather than
            // like a keyboard arrow press, so trick DoHandleItemClick() in
            // thinking we were clicked
            flags &= ~ItemClick_Kbd;
            currentLine = currentLineNow;
            break;

#ifdef __WXMSW__
        case WXK_TAB:
            // Since we are using wxWANTS_CHARS we need to send navigation
            // events for the tabs on MSW
            {
                wxNavigationKeyEvent ne;
                ne.SetDirection(!event.ShiftDown());
                ne.SetCurrentFocus(this);
                ne.SetEventObject(this);
                GetParent()->GetEventHandler()->ProcessEvent(ne);
            }
            // fall through to default
#endif
        default:
            event.Skip();
            currentLine = 0; // just to silent the stupid compiler warnings
            wxUnusedVar(currentNow);
            return;
    }

#if 0
    if ( event.ShiftDown() )
       flags |= ItemClick_Shift;
    if ( event.ControlDown() )
        flags |= ItemClick_Ctrl;

    DoHandleItemClick(current, flags);
#endif
#endif
}

// ----------------------------------------------------------------------------
// wxSymbolListCtrl mouse handling
// ----------------------------------------------------------------------------

void wxSymbolListCtrl::OnLeftDown(wxMouseEvent& event)
{
    SetFocus();

    int item = HitTest(event.GetPosition());

    if ( item != wxNOT_FOUND )
    {
        int flags = 0;
        if ( event.ShiftDown() )
           flags |= ItemClick_Shift;

        // under Mac Apple-click is used in the same way as Ctrl-click
        // elsewhere
#ifdef __WXMAC__
        if ( event.MetaDown() )
#else
        if ( event.ControlDown() )
#endif
            flags |= ItemClick_Ctrl;

        DoHandleItemClick(item, flags);
    }
}

void wxSymbolListCtrl::OnLeftDClick(wxMouseEvent& eventMouse)
{
    int item = HitTest(eventMouse.GetPosition());
    if ( item != wxNOT_FOUND )
    {

        // if item double-clicked was not yet selected, then treat
        // this event as a left-click instead
        if ( item == m_current )
        {
            wxCommandEvent event(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, GetId());
            event.SetEventObject(this);
            event.SetInt(item);

            (void)GetEventHandler()->ProcessEvent(event);
        }
        else
        {
            OnLeftDown(eventMouse);
        }

    }
}

// calculate line number from symbol value
int wxSymbolListCtrl::SymbolValueToLineNumber(int item)
{
    return (int) (item/m_symbolsPerLine);
}

// initialise control from current min/max values
void wxSymbolListCtrl::SetupCtrl(bool scrollToSelection)
{
    wxSize sz = GetClientSize();

    m_symbolsPerLine = sz.x/(m_cellSize.x+m_ptMargins.x);
    int noLines = (1 + SymbolValueToLineNumber(m_maxSymbolValue));

    SetLineCount(noLines);
    Refresh();

    if (scrollToSelection && m_current != wxNOT_FOUND && m_current >= m_minSymbolValue && m_current <= m_maxSymbolValue)
    {
        ScrollToLine(SymbolValueToLineNumber(m_current));
    }
}

// make this item visible
void wxSymbolListCtrl::EnsureVisible(int item)
{
    if (item != wxNOT_FOUND && item >= m_minSymbolValue && item <= m_maxSymbolValue)
    {
        ScrollToLine(SymbolValueToLineNumber(item));
    }
}


// hit testing
int wxSymbolListCtrl::HitTest(const wxPoint& pt)
{
    wxCoord lineHeight = OnGetLineHeight(0);

    int atLine = GetVisibleBegin() + (pt.y/lineHeight);
    int symbol = (atLine*m_symbolsPerLine) + (pt.x/(m_cellSize.x+1));

    if (symbol >= m_minSymbolValue && symbol <= m_maxSymbolValue)
        return symbol;

    return -1;
}

// Respond to size change
void wxSymbolListCtrl::OnSize(wxSizeEvent& event)
{
    SetupCtrl();
    event.Skip();
}

// set the current font
bool wxSymbolListCtrl::SetFont(const wxFont& font)
{
    wxVScrolledWindow::SetFont(font);

    SetupCtrl();

    return true;
}

// set Unicode/ASCII mode
void wxSymbolListCtrl::SetUnicodeMode(bool unicodeMode)
{
    bool changed = false;
    if (unicodeMode && !m_unicodeMode)
    {
        changed = true;

        m_minSymbolValue = 0;
        m_maxSymbolValue = 65535;
    }
    else if (!unicodeMode && m_unicodeMode)
    {
        changed = true;
        m_minSymbolValue = 0;
        m_maxSymbolValue = 255;
    }
    m_unicodeMode = unicodeMode;

    if (changed)
        SetupCtrl();
}

// ----------------------------------------------------------------------------
// use the same default attributes as wxListBox
// ----------------------------------------------------------------------------

//static
wxVisualAttributes
wxSymbolListCtrl::GetClassDefaultAttributes(wxWindowVariant variant)
{
    return wxListBox::GetClassDefaultAttributes(variant);
}

#endif // wxUSE_RICHTEXT
