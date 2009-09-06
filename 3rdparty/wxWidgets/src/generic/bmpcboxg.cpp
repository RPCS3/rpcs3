/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/bmpcboxg.cpp
// Purpose:     wxBitmapComboBox
// Author:      Jaakko Salli
// Modified by:
// Created:     Aug-31-2006
// RCS-ID:      $Id: bmpcboxg.cpp 44665 2007-03-07 23:29:03Z VZ $
// Copyright:   (c) 2005 Jaakko Salli
// Licence:     wxWindows licence
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

#if wxUSE_BITMAPCOMBOBOX

#include "wx/bmpcbox.h"

#if defined(wxGENERIC_BITMAPCOMBOBOX)

#ifndef WX_PRECOMP
    #include "wx/log.h"
#endif

#include "wx/odcombo.h"
#include "wx/settings.h"
#include "wx/dc.h"

#if wxUSE_IMAGE
    #include "wx/image.h"
#endif


const wxChar wxBitmapComboBoxNameStr[] = wxT("bitmapComboBox");


// These macros allow wxArrayPtrVoid to be used in more convenient manner
#define GetBitmapPtr(n)     ((wxBitmap*)m_bitmaps[n])


#define IMAGE_SPACING_RIGHT         4  // Space left of image

#define IMAGE_SPACING_LEFT          4  // Space right of image, left of text

#define IMAGE_SPACING_VERTICAL      2  // Space top and bottom of image

#define IMAGE_SPACING_CTRL_VERTICAL 7  // Spacing used in control size calculation

#define EXTRA_FONT_HEIGHT           0  // Add to increase min. height of list items


// ============================================================================
// implementation
// ============================================================================


BEGIN_EVENT_TABLE(wxBitmapComboBox, wxOwnerDrawnComboBox)
    EVT_SIZE(wxBitmapComboBox::OnSize)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(wxBitmapComboBox, wxOwnerDrawnComboBox)

void wxBitmapComboBox::Init()
{
    m_fontHeight = 0;
    m_imgAreaWidth = 0;
    m_inResize = false;
}

wxBitmapComboBox::wxBitmapComboBox(wxWindow *parent,
                                  wxWindowID id,
                                  const wxString& value,
                                  const wxPoint& pos,
                                  const wxSize& size,
                                  const wxArrayString& choices,
                                  long style,
                                  const wxValidator& validator,
                                  const wxString& name)
    : wxOwnerDrawnComboBox(),
      wxBitmapComboBoxBase()
{
    Init();

    Create(parent,id,value,pos,size,choices,style,validator,name);
}

bool wxBitmapComboBox::Create(wxWindow *parent,
                              wxWindowID id,
                              const wxString& value,
                              const wxPoint& pos,
                              const wxSize& size,
                              const wxArrayString& choices,
                              long style,
                              const wxValidator& validator,
                              const wxString& name)
{
    if ( !wxOwnerDrawnComboBox::Create(parent, id, value,
                                       pos, size,
                                       choices, style,
                                       validator, name) )
    {
        return false;
    }

    PostCreate();

    return true;
}

bool wxBitmapComboBox::Create(wxWindow *parent,
                              wxWindowID id,
                              const wxString& value,
                              const wxPoint& pos,
                              const wxSize& size,
                              int n,
                              const wxString choices[],
                              long style,
                              const wxValidator& validator,
                              const wxString& name)
{
    if ( !wxOwnerDrawnComboBox::Create(parent, id, value,
                                       pos, size, n,
                                       choices, style,
                                       validator, name) )
    {
        return false;
    }

    PostCreate();

    return true;
}

void wxBitmapComboBox::PostCreate()
{
    m_fontHeight = GetCharHeight() + EXTRA_FONT_HEIGHT;

    while ( m_bitmaps.GetCount() < GetCount() )
        m_bitmaps.Add( new wxBitmap() );
}

wxBitmapComboBox::~wxBitmapComboBox()
{
    Clear();
}

// ----------------------------------------------------------------------------
// Item manipulation
// ----------------------------------------------------------------------------

void wxBitmapComboBox::SetItemBitmap(unsigned int n, const wxBitmap& bitmap)
{
    wxCHECK_RET( n < GetCount(), wxT("invalid item index") );
    OnAddBitmap(bitmap);
    *GetBitmapPtr(n) = bitmap;

    if ( (int)n == GetSelection() )
        Refresh();
}

wxBitmap wxBitmapComboBox::GetItemBitmap(unsigned int n) const
{
    wxCHECK_MSG( n < GetCount(), wxNullBitmap, wxT("invalid item index") );
    return *GetBitmapPtr(n);
}

int wxBitmapComboBox::Insert(const wxString& item, const wxBitmap& bitmap,
                             unsigned int pos, void *clientData)
{
    int n = DoInsertWithImage(item, bitmap, pos);
    if ( n != wxNOT_FOUND )
        SetClientData(n, clientData);

    return n;
}

int wxBitmapComboBox::Insert(const wxString& item, const wxBitmap& bitmap,
                             unsigned int pos, wxClientData *clientData)
{
    int n = DoInsertWithImage(item, bitmap, pos);
    if ( n != wxNOT_FOUND )
        SetClientObject(n, clientData);

    return n;
}

bool wxBitmapComboBox::OnAddBitmap(const wxBitmap& bitmap)
{
    if ( bitmap.Ok() )
    {
        int width = bitmap.GetWidth();
        int height = bitmap.GetHeight();

        if ( m_usedImgSize.x <= 0 )
        {
            //
            // If size not yet determined, get it from this image.
            m_usedImgSize.x = width;
            m_usedImgSize.y = height;

            InvalidateBestSize();
            wxSize newSz = GetBestSize();
            wxSize sz = GetSize();
            if ( newSz.y > sz.y )
                SetSize(sz.x, newSz.y);
            else
                DetermineIndent();
        }

        wxCHECK_MSG(width == m_usedImgSize.x && height == m_usedImgSize.y,
                    false,
                    wxT("you can only add images of same size"));
    }

    return true;
}

bool wxBitmapComboBox::DoInsertBitmap(const wxBitmap& bitmap, unsigned int pos)
{
    if ( !OnAddBitmap(bitmap) )
        return false;

    // NB: We must try to set the image before DoInsert or
    //     DoAppend because OnMeasureItem might be called
    //     before it returns.
    m_bitmaps.Insert( new wxBitmap(bitmap), pos);

    return true;
}

int wxBitmapComboBox::DoAppendWithImage(const wxString& item, const wxBitmap& image)
{
    unsigned int pos = m_bitmaps.size();

    if ( !DoInsertBitmap(image, pos) )
        return wxNOT_FOUND;

    int index = wxOwnerDrawnComboBox::DoAppend(item);

    if ( index < 0 )
        index = m_bitmaps.size();

    // Need to re-check the index incase DoAppend sorted
    if ( (unsigned int) index != pos )
    {
        wxBitmap* bmp = GetBitmapPtr(pos);
        m_bitmaps.RemoveAt(pos);
        m_bitmaps.Insert(bmp, index);
    }

    return index;
}

int wxBitmapComboBox::DoInsertWithImage(const wxString& item,
                                        const wxBitmap& image,
                                        unsigned int pos)
{
    wxCHECK_MSG( IsValidInsert(pos), wxNOT_FOUND, wxT("invalid item index") );

    if ( !DoInsertBitmap(image, pos) )
        return wxNOT_FOUND;

    return wxOwnerDrawnComboBox::DoInsert(item, pos);
}

int wxBitmapComboBox::DoAppend(const wxString& item)
{
    return DoAppendWithImage(item, wxNullBitmap);
}

int wxBitmapComboBox::DoInsert(const wxString& item, unsigned int pos)
{
    return DoInsertWithImage(item, wxNullBitmap, pos);
}

void wxBitmapComboBox::Clear()
{
    wxOwnerDrawnComboBox::Clear();

    unsigned int i;

    for ( i=0; i<m_bitmaps.size(); i++ )
        delete GetBitmapPtr(i);

    m_bitmaps.Empty();

    m_usedImgSize.x = 0;
    m_usedImgSize.y = 0;

    DetermineIndent();
}

void wxBitmapComboBox::Delete(unsigned int n)
{
    wxOwnerDrawnComboBox::Delete(n);
    delete GetBitmapPtr(n);
    m_bitmaps.RemoveAt(n);
}

// ----------------------------------------------------------------------------
// wxBitmapComboBox event handlers and such
// ----------------------------------------------------------------------------

void wxBitmapComboBox::DetermineIndent()
{
    //
    // Recalculate amount of empty space needed in front of
    // text in control itself.
    int indent = m_imgAreaWidth = 0;

    if ( m_usedImgSize.x > 0 )
    {
        indent = m_usedImgSize.x + IMAGE_SPACING_LEFT + IMAGE_SPACING_RIGHT;
        m_imgAreaWidth = indent;

        indent -= 3;
    }

    SetCustomPaintWidth(indent);
}

void wxBitmapComboBox::OnSize(wxSizeEvent& event)
{
    // Prevent infinite looping
    if ( !m_inResize )
    {
        m_inResize = true;
        DetermineIndent();
        m_inResize = false;
    }

    event.Skip();
}

wxSize wxBitmapComboBox::DoGetBestSize() const
{
    wxSize sz = wxOwnerDrawnComboBox::DoGetBestSize();

    // Scale control to match height of highest image.
    int h2 = m_usedImgSize.y + IMAGE_SPACING_CTRL_VERTICAL;

    if ( h2 > sz.y )
        sz.y = h2;

    CacheBestSize(sz);
    return sz;
}

// ----------------------------------------------------------------------------
// wxBitmapComboBox miscellaneous
// ----------------------------------------------------------------------------

bool wxBitmapComboBox::SetFont(const wxFont& font)
{
    bool res = wxOwnerDrawnComboBox::SetFont(font);
    m_fontHeight = GetCharHeight() + EXTRA_FONT_HEIGHT;
    return res;
}

// ----------------------------------------------------------------------------
// wxBitmapComboBox item drawing and measuring
// ----------------------------------------------------------------------------

void wxBitmapComboBox::OnDrawBackground(wxDC& dc,
                                        const wxRect& rect,
                                        int item,
                                        int flags) const
{
    if ( GetCustomPaintWidth() == 0 ||
         !(flags & wxODCB_PAINTING_SELECTED) ||
         item < 0 )
    {
        wxOwnerDrawnComboBox::OnDrawBackground(dc, rect, item, flags);
        return;
    }

    //
    // Just paint simple selection background under where is text
    // (ie. emulate what MSW image choice does).
    //

    int xPos = 0;  // Starting x of selection rectangle
    const int vSizeDec = 1;  // Vertical size reduction of selection rectangle edges

    xPos = GetCustomPaintWidth() + 2;

    wxCoord x, y;
    GetTextExtent(GetString(item), &x, &y, 0, 0);

    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));

    wxColour selCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    dc.SetPen(selCol);
    dc.SetBrush(selCol);
    dc.DrawRectangle(rect.x+xPos,
                     rect.y+vSizeDec,
                     x + 4,
                     rect.height-(vSizeDec*2));
}

void wxBitmapComboBox::OnDrawItem(wxDC& dc,
                                 const wxRect& rect,
                                 int item,
                                 int flags) const
{
    wxString text;
    int imgAreaWidth = m_imgAreaWidth;
    bool drawText;

    if ( imgAreaWidth == 0 )
    {
        wxOwnerDrawnComboBox::OnDrawItem(dc, rect, item, flags);
        return;
    }

    if ( flags & wxODCB_PAINTING_CONTROL )
    {
        text = GetValue();
        if ( HasFlag(wxCB_READONLY) )
            drawText = true;
        else
            drawText = false;
    }
    else
    {
        text = GetString(item);
        drawText = true;
    }

    const wxBitmap& bmp = *GetBitmapPtr(item);
    if ( bmp.Ok() )
    {
        wxCoord w = bmp.GetWidth();
        wxCoord h = bmp.GetHeight();

        // Draw the image centered
        dc.DrawBitmap(bmp,
                      rect.x + (m_usedImgSize.x-w)/2 + IMAGE_SPACING_LEFT,
                      rect.y + (rect.height-h)/2,
                      true);
    }

    if ( drawText )
        dc.DrawText(GetString(item),
                    rect.x + imgAreaWidth + 1,
                    rect.y + (rect.height-dc.GetCharHeight())/2);
}

wxCoord wxBitmapComboBox::OnMeasureItem(size_t WXUNUSED(item)) const
{
    int imgHeightArea = m_usedImgSize.y + 2;
    return imgHeightArea > m_fontHeight ? imgHeightArea : m_fontHeight;
}

wxCoord wxBitmapComboBox::OnMeasureItemWidth(size_t item) const
{
    wxCoord x, y;
    GetTextExtent(GetString(item), &x, &y, 0, 0);
    x += m_imgAreaWidth;
    return x;
}

#endif // defined(wxGENERIC_BITMAPCOMBOBOX)

#endif // wxUSE_BITMAPCOMBOBOX
