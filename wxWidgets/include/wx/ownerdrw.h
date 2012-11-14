///////////////////////////////////////////////////////////////////////////////
// Name:        ownerdrw.h
// Purpose:     interface for owner-drawn GUI elements
// Author:      Vadim Zeitlin
// Modified by:
// Created:     11.11.97
// RCS-ID:      $Id: ownerdrw.h 62511 2009-10-30 14:11:03Z JMS $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _OWNERDRW_H
#define   _OWNERDRW_H

#include "wx/defs.h"

#if wxUSE_OWNER_DRAWN

#include "wx/bitmap.h"
#include "wx/colour.h"
#include "wx/font.h"

// ----------------------------------------------------------------------------
// wxOwnerDrawn - a mix-in base class, derive from it to implement owner-drawn
//                behaviour
//
// wxOwnerDrawn supports drawing of an item with non standard font, color and
// also supports 3 bitmaps: either a checked/unchecked bitmap for a checkable
// element or one unchangeable bitmap otherwise.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxOwnerDrawn
{
public:
  // ctor & dtor
  wxOwnerDrawn(const wxString& str = wxEmptyString,
               bool bCheckable = false,
               bool bMenuItem = false); // FIXME kludge for colors
  virtual ~wxOwnerDrawn();

  // fix appearance
  void SetFont(const wxFont& font)
      { m_font = font; m_bOwnerDrawn = true; }

  wxFont& GetFont() const { return (wxFont &)m_font; }

  void SetTextColour(const wxColour& colText)
      { m_colText = colText; m_bOwnerDrawn = true; }

  wxColour& GetTextColour() const { return (wxColour&) m_colText; }

  void SetBackgroundColour(const wxColour& colBack)
      { m_colBack = colBack; m_bOwnerDrawn = true; }

  wxColour& GetBackgroundColour() const
      { return (wxColour&) m_colBack ; }

  void SetBitmaps(const wxBitmap& bmpChecked,
                  const wxBitmap& bmpUnchecked = wxNullBitmap)
      { m_bmpChecked = bmpChecked;
        m_bmpUnchecked = bmpUnchecked;
        m_bOwnerDrawn = true; }

  void SetBitmap(const wxBitmap& bmpChecked)
      { m_bmpChecked = bmpChecked;
        m_bOwnerDrawn = true; }

  void SetDisabledBitmap( const wxBitmap& bmpDisabled )
      { m_bmpDisabled = bmpDisabled;
        m_bOwnerDrawn = true; }

  const wxBitmap& GetBitmap(bool bChecked = true) const
      { return (bChecked ? m_bmpChecked : m_bmpUnchecked); }

  const wxBitmap& GetDisabledBitmap() const
      { return m_bmpDisabled; }

  // the height of the menu checkmark (or bitmap) is determined by the font
  // for the current item, but the width should be always the same (for the
  // items to be aligned), so by default it's taken to be the same as for
  // the last item (and default width for the first one).
  //
  // NB: default is too small for bitmaps, but ok for checkmarks.
  void SetMarginWidth(int nWidth)
  {
      ms_nLastMarginWidth = m_nMarginWidth = (size_t) nWidth;
      if ( ((size_t) nWidth) != ms_nDefaultMarginWidth )
          m_bOwnerDrawn = true;
  }

  int GetMarginWidth() const { return (int) m_nMarginWidth; }
  static int GetDefaultMarginWidth() { return (int) ms_nDefaultMarginWidth; }

  // accessors
  void SetName(const wxString& strName)  { m_strName = strName; }
  const wxString& GetName() const { return m_strName;    }
  void SetCheckable(bool checkable) { m_bCheckable = checkable; }
  bool IsCheckable() const { return m_bCheckable; }

  // this is for menu items only: accel string is drawn right aligned after the
  // menu item if not empty
  void SetAccelString(const wxString& strAccel) { m_strAccel = strAccel; }

  // this function might seem strange, but if it returns false it means that
  // no non-standard attribute are set, so there is no need for this control
  // to be owner-drawn. Moreover, you can force owner-drawn to false if you
  // want to change, say, the color for the item but only if it is owner-drawn
  // (see wxMenuItem::wxMenuItem for example)
  bool IsOwnerDrawn() const { return m_bOwnerDrawn;   }

  // switch on/off owner-drawing the item
  void SetOwnerDrawn(bool ownerDrawn = true) { m_bOwnerDrawn = ownerDrawn; }
  void ResetOwnerDrawn() { m_bOwnerDrawn = false;  }

public:
  // constants used in OnDrawItem
  // (they have the same values as corresponding Win32 constants)
  enum wxODAction
  {
    wxODDrawAll       = 0x0001,   // redraw entire control
    wxODSelectChanged = 0x0002,   // selection changed (see Status.Select)
    wxODFocusChanged  = 0x0004    // keyboard focus changed (see Status.Focus)
  };

  enum wxODStatus
  {
    wxODSelected  = 0x0001,         // control is currently selected
    wxODGrayed    = 0x0002,         // item is to be grayed
    wxODDisabled  = 0x0004,         // item is to be drawn as disabled
    wxODChecked   = 0x0008,         // item is to be checked
    wxODHasFocus  = 0x0010,         // item has the keyboard focus
    wxODDefault   = 0x0020,         // item is the default item
    wxODHidePrefix= 0x0100          // hide keyboard cues (w2k and xp only)
  };

  // virtual functions to implement drawing (return true if processed)
  virtual bool OnMeasureItem(size_t *pwidth, size_t *pheight);
  virtual bool OnDrawItem(wxDC& dc, const wxRect& rc, wxODAction act, wxODStatus stat);

protected:
  // return true if this is a menu item
  bool IsMenuItem() const;

  // get the font to use, whether m_font is set or not
  wxFont GetFontToUse() const;

  // Same as wxOwnerDrawn::SetMarginWidth() but does not affect
  // ms_nLastMarginWidth. Exists solely to work around bug #4068,
  // and will not exist in wxWidgets 2.9.0 and later.
  void SetOwnMarginWidth(int nWidth)
  {
      m_nMarginWidth = (size_t) nWidth;
      if ( ((size_t) nWidth) != ms_nDefaultMarginWidth )
          m_bOwnerDrawn = true;
  }


  wxString  m_strName,      // label for a manu item
            m_strAccel;     // the accel string ("Ctrl-F17") if any

private:
  static size_t ms_nDefaultMarginWidth; // menu check mark width
  static size_t ms_nLastMarginWidth;    // handy for aligning all items

  bool      m_bCheckable,   // used only for menu or check listbox items
            m_bOwnerDrawn,  // true if something is non standard
            m_isMenuItem;   // true if this is a menu item

  wxFont    m_font;         // font to use for drawing
  wxColour  m_colText,      // color ----"---"---"----
            m_colBack;      // background color
  wxBitmap  m_bmpChecked,   // bitmap to put near the item
            m_bmpUnchecked, // (checked is used also for 'uncheckable' items)
            m_bmpDisabled;

  size_t    m_nHeight,      // font height
            m_nMinHeight,   // minimum height, as determined by user's system settings
            m_nMarginWidth; // space occupied by bitmap to the left of the item
};

#endif // wxUSE_OWNER_DRAWN

#endif
  // _OWNERDRW_H
