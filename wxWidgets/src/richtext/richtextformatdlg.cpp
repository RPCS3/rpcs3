/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtextformatdlg.cpp
// Purpose:     Formatting dialog for wxRichTextCtrl
// Author:      Julian Smart
// Modified by:
// Created:     2006-10-01
// RCS-ID:      $Id: richtextformatdlg.cpp 62010 2009-09-22 10:03:45Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_RICHTEXT

#include "wx/richtext/richtextformatdlg.h"

#ifndef WX_PRECOMP
    #include "wx/listbox.h"
    #include "wx/combobox.h"
    #include "wx/textctrl.h"
    #include "wx/sizer.h"
    #include "wx/stattext.h"
    #include "wx/statline.h"
    #include "wx/radiobut.h"
    #include "wx/icon.h"
    #include "wx/bitmap.h"
    #include "wx/dcclient.h"
    #include "wx/frame.h"
    #include "wx/checkbox.h"
    #include "wx/button.h"
#endif // WX_PRECOMP

#include "wx/bookctrl.h"
#include "wx/colordlg.h"
#include "wx/settings.h"
#include "wx/module.h"
#include "wx/imaglist.h"

#include "wx/richtext/richtextctrl.h"
#include "wx/richtext/richtextstyles.h"

#ifdef __WXMAC__
#include "../../src/richtext/richtextfontpage.cpp"
#include "../../src/richtext/richtextindentspage.cpp"
#include "../../src/richtext/richtexttabspage.cpp"
#include "../../src/richtext/richtextbulletspage.cpp"
#include "../../src/richtext/richtextstylepage.cpp"
#include "../../src/richtext/richtextliststylepage.cpp"
#else
#include "richtextfontpage.cpp"
#include "richtextindentspage.cpp"
#include "richtexttabspage.cpp"
#include "richtextbulletspage.cpp"
// Digital Mars can't cope with this much code
#ifndef __DMC__
  #include "richtextliststylepage.cpp"
#endif
#include "richtextstylepage.cpp"
#endif

#if 0 // def __WXMAC__
#define wxRICHTEXT_USE_TOOLBOOK true
#else
#define wxRICHTEXT_USE_TOOLBOOK false
#endif

bool wxRichTextFormattingDialog::sm_showToolTips = false;

IMPLEMENT_CLASS(wxRichTextFormattingDialog, wxPropertySheetDialog)

BEGIN_EVENT_TABLE(wxRichTextFormattingDialog, wxPropertySheetDialog)
    EVT_BOOKCTRL_PAGE_CHANGED(wxID_ANY, wxRichTextFormattingDialog::OnTabChanged)
END_EVENT_TABLE()

wxRichTextFormattingDialogFactory* wxRichTextFormattingDialog::ms_FormattingDialogFactory = NULL;

void wxRichTextFormattingDialog::Init()
{
    m_imageList = NULL;
    m_styleDefinition = NULL;
    m_styleSheet = NULL;
}

wxRichTextFormattingDialog::~wxRichTextFormattingDialog()
{
    delete m_imageList;
    delete m_styleDefinition;
}

bool wxRichTextFormattingDialog::Create(long flags, wxWindow* parent, const wxString& title, wxWindowID id,
        const wxPoint& pos, const wxSize& sz, long style)
{
    SetExtraStyle(wxDIALOG_EX_CONTEXTHELP|wxWS_EX_VALIDATE_RECURSIVELY);

    int resizeBorder = wxRESIZE_BORDER;

    GetFormattingDialogFactory()->SetSheetStyle(this);

#ifdef __WXMAC__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    wxPropertySheetDialog::Create(parent, id, title, pos, sz,
        style | (int)wxPlatform::IfNot(wxOS_WINDOWS_CE, resizeBorder)
    );

    GetFormattingDialogFactory()->CreateButtons(this);
    GetFormattingDialogFactory()->CreatePages(flags, this);

    LayoutDialog();

    return true;
}

/// Get attributes from the given range
bool wxRichTextFormattingDialog::GetStyle(wxRichTextCtrl* ctrl, const wxRichTextRange& range)
{
    if (ctrl->GetBuffer().GetStyleForRange(range.ToInternal(), m_attributes))
        return UpdateDisplay();
    else
        return false;
}

/// Apply attributes to the given range, only applying if necessary (wxRICHTEXT_SETSTYLE_OPTIMIZE)
bool wxRichTextFormattingDialog::ApplyStyle(wxRichTextCtrl* ctrl, const wxRichTextRange& range, int flags)
{
    return ctrl->SetStyleEx(range, m_attributes, flags);
}

/// Set the attributes and optionally update the display
bool wxRichTextFormattingDialog::SetStyle(const wxTextAttrEx& style, bool update)
{
    m_attributes = style;
    if (update)
        UpdateDisplay();
    return true;
}

/// Set the style definition and optionally update the display
bool wxRichTextFormattingDialog::SetStyleDefinition(const wxRichTextStyleDefinition& styleDef, wxRichTextStyleSheet* sheet, bool update)
{
    m_styleSheet = sheet;

    if (m_styleDefinition)
        delete m_styleDefinition;
    m_styleDefinition = styleDef.Clone();

    return SetStyle(m_styleDefinition->GetStyle(), update);
}

/// Transfers the data and from to the window
bool wxRichTextFormattingDialog::TransferDataToWindow()
{
    if (m_styleDefinition)
        m_attributes = m_styleDefinition->GetStyle();

    if (!wxPropertySheetDialog::TransferDataToWindow())
        return false;

    return true;
}

bool wxRichTextFormattingDialog::TransferDataFromWindow()
{
    if (!wxPropertySheetDialog::TransferDataFromWindow())
        return false;

    if (m_styleDefinition)
        m_styleDefinition->GetStyle() = m_attributes;

    return true;
}

/// Update the display
bool wxRichTextFormattingDialog::UpdateDisplay()
{
    return TransferDataToWindow();
}

/// Apply the styles when a different tab is selected, so the previews are
/// up to date
void wxRichTextFormattingDialog::OnTabChanged(wxBookCtrlEvent& event)
{
    if (GetBookCtrl() != event.GetEventObject())
    {
        event.Skip();
        return;
    }

    int oldPageId = event.GetOldSelection();
    if (oldPageId != -1)
    {
        wxWindow* page = GetBookCtrl()->GetPage(oldPageId);
        if (page)
            page->TransferDataFromWindow();
    }

    int pageId = event.GetSelection();
    if (pageId != -1)
    {
        wxWindow* page = GetBookCtrl()->GetPage(pageId);
        if (page)
            page->TransferDataToWindow();
    }
}

/// Respond to help command
void wxRichTextFormattingDialog::OnHelp(wxCommandEvent& event)
{
    int selPage = GetBookCtrl()->GetSelection();
    if (selPage != wxNOT_FOUND)
    {
        int pageId = m_pageIds[selPage];
        if (!GetFormattingDialogFactory()->ShowHelp(pageId, this))
            event.Skip();
    }
}

void wxRichTextFormattingDialog::SetFormattingDialogFactory(wxRichTextFormattingDialogFactory* factory)
{
    if (ms_FormattingDialogFactory)
        delete ms_FormattingDialogFactory;
    ms_FormattingDialogFactory = factory;
}

/*!
 * Factory for formatting dialog
 */

/// Create all pages, under the dialog's book control, also calling AddPage
bool wxRichTextFormattingDialogFactory::CreatePages(long pages, wxRichTextFormattingDialog* dialog)
{
    if (dialog->GetImageList())
        dialog->GetBookCtrl()->SetImageList(dialog->GetImageList());

    int availablePageCount = GetPageIdCount();
    int i;
    bool selected = false;
    for (i = 0; i < availablePageCount; i ++)
    {
        int pageId = GetPageId(i);
        if (pageId != -1 && (pages & pageId))
        {
            wxString title;
            wxPanel* panel = CreatePage(pageId, title, dialog);
            wxASSERT( panel != NULL );
            if (panel)
            {
                int imageIndex = GetPageImage(pageId);
                dialog->GetBookCtrl()->AddPage(panel, title, !selected, imageIndex);
                selected = true;

                dialog->AddPageId(pageId);
            }
        }
    }

    return true;
}

/// Create a page, given a page identifier
wxPanel* wxRichTextFormattingDialogFactory::CreatePage(int page, wxString& title, wxRichTextFormattingDialog* dialog)
{
    if (page == wxRICHTEXT_FORMAT_STYLE_EDITOR)
    {
        wxRichTextStylePage* page = new wxRichTextStylePage(dialog->GetBookCtrl(), wxID_ANY);
        title = _("Style");
        return page;
    }
    else if (page == wxRICHTEXT_FORMAT_FONT)
    {
        wxRichTextFontPage* page = new wxRichTextFontPage(dialog->GetBookCtrl(), wxID_ANY);
        title = _("Font");
        return page;
    }
    else if (page == wxRICHTEXT_FORMAT_INDENTS_SPACING)
    {
        wxRichTextIndentsSpacingPage* page = new wxRichTextIndentsSpacingPage(dialog->GetBookCtrl(), wxID_ANY);
        title = _("Indents && Spacing");
        return page;
    }
    else if (page == wxRICHTEXT_FORMAT_TABS)
    {
        wxRichTextTabsPage* page = new wxRichTextTabsPage(dialog->GetBookCtrl(), wxID_ANY);
        title = _("Tabs");
        return page;
    }
    else if (page == wxRICHTEXT_FORMAT_BULLETS)
    {
        wxRichTextBulletsPage* page = new wxRichTextBulletsPage(dialog->GetBookCtrl(), wxID_ANY);
        title = _("Bullets");
        return page;
    }
#ifndef __DMC__
    else if (page == wxRICHTEXT_FORMAT_LIST_STYLE)
    {
        wxRichTextListStylePage* page = new wxRichTextListStylePage(dialog->GetBookCtrl(), wxID_ANY);
        title = _("List Style");
        return page;
    }
#endif
    else
        return NULL;
}

/// Enumerate all available page identifiers
int wxRichTextFormattingDialogFactory::GetPageId(int i) const
{
    int pages[] = {
        wxRICHTEXT_FORMAT_STYLE_EDITOR,
        wxRICHTEXT_FORMAT_FONT,
        wxRICHTEXT_FORMAT_INDENTS_SPACING,
        wxRICHTEXT_FORMAT_BULLETS,
        wxRICHTEXT_FORMAT_TABS,
        wxRICHTEXT_FORMAT_LIST_STYLE };

    if (i < 0 || i > 5)
        return -1;

    return pages[i];
}

/// Get the number of available page identifiers
int wxRichTextFormattingDialogFactory::GetPageIdCount() const
{
#ifdef __DMC__
    return 5;
#else
    return 6;
#endif
}

/// Set the sheet style, called at the start of wxRichTextFormattingDialog::Create
bool wxRichTextFormattingDialogFactory::SetSheetStyle(wxRichTextFormattingDialog* dialog)
{
    bool useToolBook = wxRICHTEXT_USE_TOOLBOOK;
    if (useToolBook)
    {
        int sheetStyle = wxPROPSHEET_SHRINKTOFIT;
#ifdef __WXMAC__
        sheetStyle |= wxPROPSHEET_BUTTONTOOLBOOK;
#else
        sheetStyle |= wxPROPSHEET_TOOLBOOK;
#endif

        dialog->SetSheetStyle(sheetStyle);
        dialog->SetSheetInnerBorder(0);
        dialog->SetSheetOuterBorder(0);
    }

    return true;
}

/// Create the main dialog buttons
bool wxRichTextFormattingDialogFactory::CreateButtons(wxRichTextFormattingDialog* dialog)
{
    bool useToolBook = wxRICHTEXT_USE_TOOLBOOK;

    // If using a toolbook, also follow Mac style and don't create buttons
    int flags = wxOK|wxCANCEL;
#ifndef __WXWINCE__
    if (dialog->GetWindowStyleFlag() & wxRICHTEXT_FORMAT_HELP_BUTTON)
        flags |= wxHELP;
#endif

    if (!useToolBook)
        dialog->CreateButtons(flags);

    return true;
}

/*
 * Module to initialise and clean up handlers
 */

class wxRichTextFormattingDialogModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxRichTextFormattingDialogModule)
public:
    wxRichTextFormattingDialogModule() {}
    bool OnInit() { wxRichTextFormattingDialog::SetFormattingDialogFactory(new wxRichTextFormattingDialogFactory); return true; }
    void OnExit() { wxRichTextFormattingDialog::SetFormattingDialogFactory(NULL); }
};

IMPLEMENT_DYNAMIC_CLASS(wxRichTextFormattingDialogModule, wxModule)

/*
 * Font preview control
 */

BEGIN_EVENT_TABLE(wxRichTextFontPreviewCtrl, wxWindow)
    EVT_PAINT(wxRichTextFontPreviewCtrl::OnPaint)
END_EVENT_TABLE()

void wxRichTextFontPreviewCtrl::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    wxSize size = GetSize();
    wxFont font = GetFont();

    if ((GetTextEffects() & wxTEXT_ATTR_EFFECT_SUPERSCRIPT) || (GetTextEffects() & wxTEXT_ATTR_EFFECT_SUBSCRIPT))
    {
        double size = static_cast<double>(font.GetPointSize()) / wxSCRIPT_MUL_FACTOR;
        font.SetPointSize( static_cast<int>(size) );
    }

    if ( font.Ok() )
    {
        dc.SetFont(font);
        // Calculate vertical and horizontal centre
        long w = 0, h = 0;

        wxString text(_("ABCDEFGabcdefg12345"));
        if (GetTextEffects() & wxTEXT_ATTR_EFFECT_CAPITALS)
            text.MakeUpper();

        dc.GetTextExtent( text, &w, &h);
        int cx = wxMax(2, (size.x/2) - (w/2));
        int cy = wxMax(2, (size.y/2) - (h/2));

        if ( GetTextEffects() & wxTEXT_ATTR_EFFECT_SUPERSCRIPT )
            cy -= h/2;
        if ( GetTextEffects() & wxTEXT_ATTR_EFFECT_SUBSCRIPT )
            cy += h/2;

        dc.SetTextForeground(GetForegroundColour());
        dc.SetClippingRegion(2, 2, size.x-4, size.y-4);
        dc.DrawText(text, cx, cy);

        if (GetTextEffects() & wxTEXT_ATTR_EFFECT_STRIKETHROUGH)
        {
            dc.SetPen(wxPen(GetForegroundColour(), 1));
            dc.DrawLine(cx, (int) (cy + h/2 + 0.5), cx + w, (int) (cy + h/2 + 0.5));
        }

        dc.DestroyClippingRegion();
    }
}

// Helper for pages to get the top-level dialog
wxRichTextFormattingDialog* wxRichTextFormattingDialog::GetDialog(wxWindow* win)
{
    wxWindow* p = win->GetParent();
    while (p && !p->IsKindOf(CLASSINFO(wxRichTextFormattingDialog)))
        p = p->GetParent();
    wxRichTextFormattingDialog* dialog = wxDynamicCast(p, wxRichTextFormattingDialog);
    return dialog;
}


// Helper for pages to get the attributes
wxTextAttrEx* wxRichTextFormattingDialog::GetDialogAttributes(wxWindow* win)
{
    wxRichTextFormattingDialog* dialog = GetDialog(win);
    if (dialog)
        return & dialog->GetAttributes();
    else
        return NULL;
}

// Helper for pages to get the style
wxRichTextStyleDefinition* wxRichTextFormattingDialog::GetDialogStyleDefinition(wxWindow* win)
{
    wxRichTextFormattingDialog* dialog = GetDialog(win);
    if (dialog)
        return dialog->GetStyleDefinition();
    else
        return NULL;
}

/*
 * A control for displaying a small preview of a colour or bitmap
 */

BEGIN_EVENT_TABLE(wxRichTextColourSwatchCtrl, wxControl)
    EVT_MOUSE_EVENTS(wxRichTextColourSwatchCtrl::OnMouseEvent)
END_EVENT_TABLE()

IMPLEMENT_CLASS(wxRichTextColourSwatchCtrl, wxControl)

wxRichTextColourSwatchCtrl::wxRichTextColourSwatchCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
    if ((style & wxBORDER_MASK) == wxBORDER_DEFAULT)
#ifdef __WXMSW__
        style |= GetThemedBorderStyle();
#else
        style |= wxBORDER_SUNKEN;
#endif
    wxControl::Create(parent, id, pos, size, style);

    SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

wxRichTextColourSwatchCtrl::~wxRichTextColourSwatchCtrl()
{
}

void wxRichTextColourSwatchCtrl::OnMouseEvent(wxMouseEvent& event)
{
    if (event.LeftDown())
    {
        wxWindow* parent = GetParent();
        while (parent != NULL && !parent->IsKindOf(CLASSINFO(wxDialog)) && !parent->IsKindOf(CLASSINFO(wxFrame)))
            parent = parent->GetParent();

        wxColourData data;
        data.SetChooseFull(true);
        data.SetColour(m_colour);
#if wxUSE_COLOURDLG
        wxColourDialog *dialog = new wxColourDialog(parent, &data);
        // Crashes on wxMac (no m_peer)
#ifndef __WXMAC__
        dialog->SetTitle(_("Colour"));
#endif
        if (dialog->ShowModal() == wxID_OK)
        {
            wxColourData retData = dialog->GetColourData();
            m_colour = retData.GetColour();
            SetBackgroundColour(m_colour);
        }
        dialog->Destroy();
#endif // wxUSE_COLOURDLG
        Refresh();

        wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
        GetEventHandler()->ProcessEvent(event);
    }
}

#if wxUSE_HTML

/*!
 * wxRichTextFontListBox class declaration
 * A listbox to display styles.
 */

IMPLEMENT_CLASS(wxRichTextFontListBox, wxHtmlListBox)

BEGIN_EVENT_TABLE(wxRichTextFontListBox, wxHtmlListBox)
END_EVENT_TABLE()

wxRichTextFontListBox::wxRichTextFontListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style)
{
    if ((style & wxBORDER_MASK) == wxBORDER_DEFAULT)
#ifdef __WXMSW__
        style |= GetThemedBorderStyle();
#else
        style |= wxBORDER_SUNKEN;
#endif

    Init();
    Create(parent, id, pos, size, style);
}

bool wxRichTextFontListBox::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style)
{
    return wxHtmlListBox::Create(parent, id, pos, size, style);
}

wxRichTextFontListBox::~wxRichTextFontListBox()
{
}

/// Returns the HTML for this item
wxString wxRichTextFontListBox::OnGetItem(size_t n) const
{
    if (m_faceNames.GetCount() == 0)
        return wxEmptyString;

    wxString str = CreateHTML(m_faceNames[n]);
    return str;
}

/// Get font name for index
wxString wxRichTextFontListBox::GetFaceName(size_t i) const
{
    return m_faceNames[i];
}

/// Set selection for string, returning the index.
int wxRichTextFontListBox::SetFaceNameSelection(const wxString& name)
{
    int i = m_faceNames.Index(name);
    SetSelection(i);

    return i;
}

/// Updates the font list
void wxRichTextFontListBox::UpdateFonts()
{
    wxArrayString facenames = wxRichTextCtrl::GetAvailableFontNames();
    m_faceNames = facenames;
    m_faceNames.Sort();

    SetItemCount(m_faceNames.GetCount());
    Refresh();
}

#if 0
// Convert a colour to a 6-digit hex string
static wxString ColourToHexString(const wxColour& col)
{
    wxString hex;

    hex += wxDecToHex(col.Red());
    hex += wxDecToHex(col.Green());
    hex += wxDecToHex(col.Blue());

    return hex;
}
#endif

/// Creates a suitable HTML fragment for a definition
wxString wxRichTextFontListBox::CreateHTML(const wxString& facename) const
{
    wxString str = wxT("<font");

    str << wxT(" size=\"+2\"");;

    if (!facename.IsEmpty() && facename != _("(none)"))
        str << wxT(" face=\"") << facename << wxT("\"");
/*
    if (def->GetStyle().GetTextColour().Ok())
        str << wxT(" color=\"#") << ColourToHexString(def->GetStyle().GetTextColour()) << wxT("\"");
*/

    str << wxT(">");

    bool hasBold = false;

    if (hasBold)
        str << wxT("<b>");

    str += facename;

    if (hasBold)
        str << wxT("</b>");

    str << wxT("</font>");

    return str;
}

#endif
    // wxUSE_HTML


#endif
    // wxUSE_RICHTEXT
