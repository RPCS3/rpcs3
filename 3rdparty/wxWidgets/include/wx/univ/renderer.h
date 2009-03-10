///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/renderer.h
// Purpose:     wxRenderer class declaration
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.08.00
// RCS-ID:      $Id: renderer.h 43726 2006-11-30 23:44:55Z RD $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_RENDERER_H_
#define _WX_UNIV_RENDERER_H_

/*
   wxRenderer class is used to draw all wxWidgets controls. This is an ABC and
   the look of the application is determined by the concrete derivation of
   wxRenderer used in the program.

   It also contains a few static methods which may be used by the concrete
   renderers and provide the functionality which is often similar or identical
   in all renderers (using inheritance here would be more restrictive as the
   given concrete renderer may need an arbitrary subset of the base class
   methods).

   Finally note that wxRenderer supersedes wxRendererNative in wxUniv build and
   includes the latters functionality (which it may delegate to the generic
   implementation of the latter or reimplement itself).
 */

#include "wx/renderer.h"

class WXDLLEXPORT wxWindow;
class WXDLLEXPORT wxDC;
class WXDLLEXPORT wxCheckListBox;

#if wxUSE_LISTBOX
    class WXDLLEXPORT wxListBox;
#endif // wxUSE_LISTBOX

#if wxUSE_MENUS
   class WXDLLEXPORT wxMenu;
   class WXDLLEXPORT wxMenuGeometryInfo;
#endif // wxUSE_MENUS

class WXDLLEXPORT wxScrollBar;

#if wxUSE_TEXTCTRL
    class WXDLLEXPORT wxTextCtrl;
#endif

#if wxUSE_GAUGE
    class WXDLLEXPORT wxGauge;
#endif // wxUSE_GAUGE

#include "wx/string.h"
#include "wx/gdicmn.h"
#include "wx/icon.h"

// helper class used by wxMenu-related functions
class WXDLLEXPORT wxMenuGeometryInfo
{
public:
    // get the total size of the menu
    virtual wxSize GetSize() const = 0;

    virtual ~wxMenuGeometryInfo();
};

// ----------------------------------------------------------------------------
// wxRenderer: abstract renderers interface
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxRenderer : public wxDelegateRendererNative
{
public:
    // drawing functions
    // -----------------

    // draw the controls background
    virtual void DrawBackground(wxDC& dc,
                                const wxColour& col,
                                const wxRect& rect,
                                int flags,
                                wxWindow *window = NULL) = 0;

    // draw the button surface
    virtual void DrawButtonSurface(wxDC& dc,
                                   const wxColour& col,
                                   const wxRect& rect,
                                   int flags) = 0;


    // draw the focus rectangle around the label contained in the given rect
    //
    // only wxCONTROL_SELECTED makes sense in flags here
    virtual void DrawFocusRect(wxDC& dc, const wxRect& rect, int flags = 0) = 0;

    // draw the label inside the given rectangle with the specified alignment
    // and optionally emphasize the character with the given index
    virtual void DrawLabel(wxDC& dc,
                           const wxString& label,
                           const wxRect& rect,
                           int flags = 0,
                           int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                           int indexAccel = -1,
                           wxRect *rectBounds = NULL) = 0;

    // same but also draw a bitmap if it is valid
    virtual void DrawButtonLabel(wxDC& dc,
                                 const wxString& label,
                                 const wxBitmap& image,
                                 const wxRect& rect,
                                 int flags = 0,
                                 int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                                 int indexAccel = -1,
                                 wxRect *rectBounds = NULL) = 0;


    // draw the border and optionally return the rectangle containing the
    // region inside the border
    virtual void DrawBorder(wxDC& dc,
                            wxBorder border,
                            const wxRect& rect,
                            int flags = 0,
                            wxRect *rectIn = (wxRect *)NULL) = 0;

    // draw text control border (I hate to have a separate method for this but
    // it is needed to accommodate GTK+)
    virtual void DrawTextBorder(wxDC& dc,
                                wxBorder border,
                                const wxRect& rect,
                                int flags = 0,
                                wxRect *rectIn = (wxRect *)NULL) = 0;

    // draw push button border and return the rectangle left for the label
    virtual void DrawButtonBorder(wxDC& dc,
                                  const wxRect& rect,
                                  int flags = 0,
                                  wxRect *rectIn = (wxRect *)NULL) = 0;

    // draw a horizontal line
    virtual void DrawHorizontalLine(wxDC& dc,
                                    wxCoord y, wxCoord x1, wxCoord x2) = 0;

    // draw a vertical line
    virtual void DrawVerticalLine(wxDC& dc,
                                  wxCoord x, wxCoord y1, wxCoord y2) = 0;

    // draw a frame with the label (horizontal alignment can be specified)
    virtual void DrawFrame(wxDC& dc,
                           const wxString& label,
                           const wxRect& rect,
                           int flags = 0,
                           int alignment = wxALIGN_LEFT,
                           int indexAccel = -1) = 0;

    // draw an arrow in the given direction
    virtual void DrawArrow(wxDC& dc,
                           wxDirection dir,
                           const wxRect& rect,
                           int flags = 0) = 0;

    // draw a scrollbar arrow (may be the same as arrow but may be not)
    virtual void DrawScrollbarArrow(wxDC& dc,
                                    wxDirection dir,
                                    const wxRect& rect,
                                    int flags = 0) = 0;

    // draw the scrollbar thumb
    virtual void DrawScrollbarThumb(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rect,
                                    int flags = 0) = 0;

    // draw a (part of) scrollbar shaft
    virtual void DrawScrollbarShaft(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rect,
                                    int flags = 0) = 0;

    // draw the rectangle in the corner between two scrollbars
    virtual void DrawScrollCorner(wxDC& dc,
                                  const wxRect& rect) = 0;

    // draw an item of a wxListBox
    virtual void DrawItem(wxDC& dc,
                          const wxString& label,
                          const wxRect& rect,
                          int flags = 0) = 0;

    // draw an item of a wxCheckListBox
    virtual void DrawCheckItem(wxDC& dc,
                               const wxString& label,
                               const wxBitmap& bitmap,
                               const wxRect& rect,
                               int flags = 0) = 0;

    // draw a checkbutton (bitmap may be invalid to use default one)
    virtual void DrawCheckButton(wxDC& dc,
                                 const wxString& label,
                                 const wxBitmap& bitmap,
                                 const wxRect& rect,
                                 int flags = 0,
                                 wxAlignment align = wxALIGN_LEFT,
                                 int indexAccel = -1) = 0;

    // draw a radio button
    virtual void DrawRadioButton(wxDC& dc,
                                 const wxString& label,
                                 const wxBitmap& bitmap,
                                 const wxRect& rect,
                                 int flags = 0,
                                 wxAlignment align = wxALIGN_LEFT,
                                 int indexAccel = -1) = 0;

#if wxUSE_TOOLBAR
    // draw a toolbar button (label may be empty, bitmap may be invalid, if
    // both conditions are true this function draws a separator)
    virtual void DrawToolBarButton(wxDC& dc,
                                   const wxString& label,
                                   const wxBitmap& bitmap,
                                   const wxRect& rect,
                                   int flags = 0,
                                   long style = 0,
                                   int tbarStyle = 0) = 0;
#endif // wxUSE_TOOLBAR

#if wxUSE_TEXTCTRL
    // draw a (part of) line in the text control
    virtual void DrawTextLine(wxDC& dc,
                              const wxString& text,
                              const wxRect& rect,
                              int selStart = -1,
                              int selEnd = -1,
                              int flags = 0) = 0;

    // draw a line wrap indicator
    virtual void DrawLineWrapMark(wxDC& dc, const wxRect& rect) = 0;
#endif // wxUSE_TEXTCTRL

#if wxUSE_NOTEBOOK
    // draw a notebook tab
    virtual void DrawTab(wxDC& dc,
                         const wxRect& rect,
                         wxDirection dir,
                         const wxString& label,
                         const wxBitmap& bitmap = wxNullBitmap,
                         int flags = 0,
                         int indexAccel = -1) = 0;
#endif // wxUSE_NOTEBOOK

#if wxUSE_SLIDER

    // draw the slider shaft
    virtual void DrawSliderShaft(wxDC& dc,
                                 const wxRect& rect,
                                 int lenThumb,
                                 wxOrientation orient,
                                 int flags = 0,
                                 long style = 0,
                                 wxRect *rectShaft = NULL) = 0;

    // draw the slider thumb
    virtual void DrawSliderThumb(wxDC& dc,
                                 const wxRect& rect,
                                 wxOrientation orient,
                                 int flags = 0,
                                 long style = 0) = 0;

    // draw the slider ticks
    virtual void DrawSliderTicks(wxDC& dc,
                                 const wxRect& rect,
                                 int lenThumb,
                                 wxOrientation orient,
                                 int start,
                                 int end,
                                 int step = 1,
                                 int flags = 0,
                                 long style = 0) = 0;
#endif // wxUSE_SLIDER

#if wxUSE_MENUS
    // draw a menu bar item
    virtual void DrawMenuBarItem(wxDC& dc,
                                 const wxRect& rect,
                                 const wxString& label,
                                 int flags = 0,
                                 int indexAccel = -1) = 0;

    // draw a menu item (also used for submenus if flags has ISSUBMENU flag)
    //
    // the geometryInfo is calculated by GetMenuGeometry() function from below
    virtual void DrawMenuItem(wxDC& dc,
                              wxCoord y,
                              const wxMenuGeometryInfo& geometryInfo,
                              const wxString& label,
                              const wxString& accel,
                              const wxBitmap& bitmap = wxNullBitmap,
                              int flags = 0,
                              int indexAccel = -1) = 0;

    // draw a menu bar separator
    virtual void DrawMenuSeparator(wxDC& dc,
                                   wxCoord y,
                                   const wxMenuGeometryInfo& geomInfo) = 0;
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // draw a status bar field: wxCONTROL_ISDEFAULT bit in the flags is
    // interpreted specially and means "draw the status bar grip" here
    virtual void DrawStatusField(wxDC& dc,
                                 const wxRect& rect,
                                 const wxString& label,
                                 int flags = 0, int style = 0) = 0;
#endif // wxUSE_STATUSBAR

    // draw complete frame/dialog titlebar
    virtual void DrawFrameTitleBar(wxDC& dc,
                                   const wxRect& rect,
                                   const wxString& title,
                                   const wxIcon& icon,
                                   int flags,
                                   int specialButton = 0,
                                   int specialButtonFlags = 0) = 0;

    // draw frame borders
    virtual void DrawFrameBorder(wxDC& dc,
                                 const wxRect& rect,
                                 int flags) = 0;

    // draw frame titlebar background
    virtual void DrawFrameBackground(wxDC& dc,
                                     const wxRect& rect,
                                     int flags) = 0;

    // draw frame title
    virtual void DrawFrameTitle(wxDC& dc,
                                const wxRect& rect,
                                const wxString& title,
                                int flags) = 0;

    // draw frame icon
    virtual void DrawFrameIcon(wxDC& dc,
                               const wxRect& rect,
                               const wxIcon& icon,
                               int flags) = 0;

    // draw frame buttons
    virtual void DrawFrameButton(wxDC& dc,
                                 wxCoord x, wxCoord y,
                                 int button,
                                 int flags = 0) = 0;

    // misc functions
    // --------------

#if wxUSE_COMBOBOX
    // return the bitmaps to use for combobox button
    virtual void GetComboBitmaps(wxBitmap *bmpNormal,
                                 wxBitmap *bmpFocus,
                                 wxBitmap *bmpPressed,
                                 wxBitmap *bmpDisabled) = 0;
#endif // wxUSE_COMBOBOX

    // geometry functions
    // ------------------

    // get the dimensions of the border: rect.x/y contain the width/height of
    // the left/top side, width/heigh - of the right/bottom one
    virtual wxRect GetBorderDimensions(wxBorder border) const = 0;

    // the scrollbars may be drawn either inside the window border or outside
    // it - this function is used to decide how to draw them
    virtual bool AreScrollbarsInsideBorder() const = 0;

    // adjust the size of the control of the given class: for most controls,
    // this just takes into account the border, but for some (buttons, for
    // example) it is more complicated - the result being, in any case, that
    // the control looks "nice" if it uses the adjusted rectangle
    virtual void AdjustSize(wxSize *size, const wxWindow *window) = 0;

#if wxUSE_SCROLLBAR
    // get the size of a scrollbar arrow
    virtual wxSize GetScrollbarArrowSize() const = 0;
#endif // wxUSE_SCROLLBAR

    // get the height of a listbox item from the base font height
    virtual wxCoord GetListboxItemHeight(wxCoord fontHeight) = 0;

    // get the size of a checkbox/radio button bitmap
    virtual wxSize GetCheckBitmapSize() const = 0;
    virtual wxSize GetRadioBitmapSize() const = 0;
    virtual wxCoord GetCheckItemMargin() const = 0;

#if wxUSE_TOOLBAR
    // get the standard size of a toolbar button and also return the size of
    // a toolbar separator in the provided pointer
    virtual wxSize GetToolBarButtonSize(wxCoord *separator) const = 0;

    // get the margins between/around the toolbar buttons
    virtual wxSize GetToolBarMargin() const = 0;
#endif // wxUSE_TOOLBAR

#if wxUSE_TEXTCTRL
    // convert between text rectangle and client rectangle for text controls:
    // the former is typicall smaller to leave margins around text
    virtual wxRect GetTextTotalArea(const wxTextCtrl *text,
                                    const wxRect& rectText) const = 0;

    // extra space is for line indicators
    virtual wxRect GetTextClientArea(const wxTextCtrl *text,
                                     const wxRect& rectTotal,
                                     wxCoord *extraSpaceBeyond) const = 0;
#endif // wxUSE_TEXTCTRL

#if wxUSE_NOTEBOOK
    // get the overhang of a selected tab
    virtual wxSize GetTabIndent() const = 0;

    // get the padding around the text in a tab
    virtual wxSize GetTabPadding() const = 0;
#endif // wxUSE_NOTEBOOK

#if wxUSE_SLIDER
    // get the default size of the slider in lesser dimension (i.e. height of a
    // horizontal slider or width of a vertical one)
    virtual wxCoord GetSliderDim() const = 0;

    // get the length of the slider ticks displayed along side slider
    virtual wxCoord GetSliderTickLen() const = 0;

    // get the slider shaft rect from the total slider rect
    virtual wxRect GetSliderShaftRect(const wxRect& rect,
                                      int lenThumb,
                                      wxOrientation orient,
                                      long style = 0) const = 0;

    // get the size of the slider thumb for the given total slider rect
    virtual wxSize GetSliderThumbSize(const wxRect& rect,
                                      int lenThumb,
                                      wxOrientation orient) const = 0;
#endif // wxUSE_SLIDER

    // get the size of one progress bar step (in horz and vertical directions)
    virtual wxSize GetProgressBarStep() const = 0;

#if wxUSE_MENUS
    // get the size of rectangle to use in the menubar for the given text rect
    virtual wxSize GetMenuBarItemSize(const wxSize& sizeText) const = 0;

    // get the struct storing all layout info needed to draw all menu items
    // (this can't be calculated for each item separately as they should be
    // aligned)
    //
    // the returned pointer must be deleted by the caller
    virtual wxMenuGeometryInfo *GetMenuGeometry(wxWindow *win,
                                                const wxMenu& menu) const = 0;
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // get the borders around the status bar fields (x and y fields of the
    // return value)
    virtual wxSize GetStatusBarBorders() const = 0;

    // get the border between the status bar fields
    virtual wxCoord GetStatusBarBorderBetweenFields() const = 0;

    // get the mergin between a field and its border
    virtual wxSize GetStatusBarFieldMargins() const = 0;
#endif // wxUSE_STATUSBAR

    // get client area rectangle of top level window (i.e. subtract
    // decorations from given rectangle)
    virtual wxRect GetFrameClientArea(const wxRect& rect, int flags) const = 0;

    // get size of whole top level window, given size of its client area size
    virtual wxSize GetFrameTotalSize(const wxSize& clientSize, int flags) const = 0;

    // get the minimal size of top level window
    virtual wxSize GetFrameMinSize(int flags) const = 0;

    // get titlebar icon size
    virtual wxSize GetFrameIconSize() const = 0;

    // returns one of wxHT_TOPLEVEL_XXX constants
    virtual int HitTestFrame(const wxRect& rect,
                             const wxPoint& pt,
                             int flags = 0) const = 0;

    // virtual dtor for any base class
    virtual ~wxRenderer();
};

// ----------------------------------------------------------------------------
// wxDelegateRenderer: it is impossible to inherit from any of standard
// renderers as their declarations are in private code, but you can use this
// class to override only some of the Draw() functions - all the other ones
// will be left to the original renderer
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDelegateRenderer : public wxRenderer
{
public:
    wxDelegateRenderer(wxRenderer *renderer) : m_renderer(renderer) { }

    virtual void DrawBackground(wxDC& dc,
                                const wxColour& col,
                                const wxRect& rect,
                                int flags,
                                wxWindow *window = NULL )
        { m_renderer->DrawBackground(dc, col, rect, flags, window ); }
    virtual void DrawButtonSurface(wxDC& dc,
                                   const wxColour& col,
                                   const wxRect& rect,
                                   int flags)
        { m_renderer->DrawButtonSurface(dc, col, rect, flags); }
    virtual void DrawFocusRect(wxDC& dc, const wxRect& rect, int flags = 0)
        { m_renderer->DrawFocusRect(dc, rect, flags); }
    virtual void DrawLabel(wxDC& dc,
                           const wxString& label,
                           const wxRect& rect,
                           int flags = 0,
                           int align = wxALIGN_LEFT | wxALIGN_TOP,
                           int indexAccel = -1,
                           wxRect *rectBounds = NULL)
        { m_renderer->DrawLabel(dc, label, rect,
                                flags, align, indexAccel, rectBounds); }
    virtual void DrawButtonLabel(wxDC& dc,
                                 const wxString& label,
                                 const wxBitmap& image,
                                 const wxRect& rect,
                                 int flags = 0,
                                 int align = wxALIGN_LEFT | wxALIGN_TOP,
                                 int indexAccel = -1,
                                 wxRect *rectBounds = NULL)
        { m_renderer->DrawButtonLabel(dc, label, image, rect,
                                      flags, align, indexAccel, rectBounds); }
    virtual void DrawBorder(wxDC& dc,
                            wxBorder border,
                            const wxRect& rect,
                            int flags = 0,
                            wxRect *rectIn = (wxRect *)NULL)
        { m_renderer->DrawBorder(dc, border, rect, flags, rectIn); }
    virtual void DrawTextBorder(wxDC& dc,
                                wxBorder border,
                                const wxRect& rect,
                                int flags = 0,
                                wxRect *rectIn = (wxRect *)NULL)
        { m_renderer->DrawTextBorder(dc, border, rect, flags, rectIn); }
    virtual void DrawButtonBorder(wxDC& dc,
                                  const wxRect& rect,
                                  int flags = 0,
                                  wxRect *rectIn = (wxRect *)NULL)
        { m_renderer->DrawButtonBorder(dc, rect, flags, rectIn); }
    virtual void DrawFrame(wxDC& dc,
                           const wxString& label,
                           const wxRect& rect,
                           int flags = 0,
                           int align = wxALIGN_LEFT,
                           int indexAccel = -1)
        { m_renderer->DrawFrame(dc, label, rect, flags, align, indexAccel); }
    virtual void DrawHorizontalLine(wxDC& dc,
                                    wxCoord y, wxCoord x1, wxCoord x2)
        { m_renderer->DrawHorizontalLine(dc, y, x1, x2); }
    virtual void DrawVerticalLine(wxDC& dc,
                                  wxCoord x, wxCoord y1, wxCoord y2)
        { m_renderer->DrawVerticalLine(dc, x, y1, y2); }
    virtual void DrawArrow(wxDC& dc,
                           wxDirection dir,
                           const wxRect& rect,
                           int flags = 0)
        { m_renderer->DrawArrow(dc, dir, rect, flags); }
    virtual void DrawScrollbarArrow(wxDC& dc,
                           wxDirection dir,
                           const wxRect& rect,
                           int flags = 0)
        { m_renderer->DrawScrollbarArrow(dc, dir, rect, flags); }
    virtual void DrawScrollbarThumb(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rect,
                                    int flags = 0)
        { m_renderer->DrawScrollbarThumb(dc, orient, rect, flags); }
    virtual void DrawScrollbarShaft(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rect,
                                    int flags = 0)
        { m_renderer->DrawScrollbarShaft(dc, orient, rect, flags); }
    virtual void DrawScrollCorner(wxDC& dc,
                                  const wxRect& rect)
        { m_renderer->DrawScrollCorner(dc, rect); }
    virtual void DrawItem(wxDC& dc,
                          const wxString& label,
                          const wxRect& rect,
                          int flags = 0)
        { m_renderer->DrawItem(dc, label, rect, flags); }
    virtual void DrawCheckItem(wxDC& dc,
                               const wxString& label,
                               const wxBitmap& bitmap,
                               const wxRect& rect,
                               int flags = 0)
        { m_renderer->DrawCheckItem(dc, label, bitmap, rect, flags); }
    virtual void DrawCheckButton(wxDC& dc,
                                 const wxString& label,
                                 const wxBitmap& bitmap,
                                 const wxRect& rect,
                                 int flags = 0,
                                 wxAlignment align = wxALIGN_LEFT,
                                 int indexAccel = -1)
        { m_renderer->DrawCheckButton(dc, label, bitmap, rect,
                                      flags, align, indexAccel); }
    virtual void DrawRadioButton(wxDC& dc,
                                 const wxString& label,
                                 const wxBitmap& bitmap,
                                 const wxRect& rect,
                                 int flags = 0,
                                 wxAlignment align = wxALIGN_LEFT,
                                 int indexAccel = -1)
        { m_renderer->DrawRadioButton(dc, label, bitmap, rect,
                                      flags, align, indexAccel); }
#if wxUSE_TOOLBAR
    virtual void DrawToolBarButton(wxDC& dc,
                                   const wxString& label,
                                   const wxBitmap& bitmap,
                                   const wxRect& rect,
                                   int flags = 0,
                                   long style = 0,
                                   int tbarStyle = 0)
        { m_renderer->DrawToolBarButton(dc, label, bitmap, rect, flags, style, tbarStyle); }
#endif // wxUSE_TOOLBAR

#if wxUSE_TEXTCTRL
    virtual void DrawTextLine(wxDC& dc,
                              const wxString& text,
                              const wxRect& rect,
                              int selStart = -1,
                              int selEnd = -1,
                              int flags = 0)
        { m_renderer->DrawTextLine(dc, text, rect, selStart, selEnd, flags); }
    virtual void DrawLineWrapMark(wxDC& dc, const wxRect& rect)
        { m_renderer->DrawLineWrapMark(dc, rect); }
#endif // wxUSE_TEXTCTRL

#if wxUSE_NOTEBOOK
    virtual void DrawTab(wxDC& dc,
                         const wxRect& rect,
                         wxDirection dir,
                         const wxString& label,
                         const wxBitmap& bitmap = wxNullBitmap,
                         int flags = 0,
                         int accel = -1)
        { m_renderer->DrawTab(dc, rect, dir, label, bitmap, flags, accel); }
#endif // wxUSE_NOTEBOOK

#if wxUSE_SLIDER

    virtual void DrawSliderShaft(wxDC& dc,
                                 const wxRect& rect,
                                 int lenThumb,
                                 wxOrientation orient,
                                 int flags = 0,
                                 long style = 0,
                                 wxRect *rectShaft = NULL)
        { m_renderer->DrawSliderShaft(dc, rect, lenThumb, orient, flags, style, rectShaft); }
    virtual void DrawSliderThumb(wxDC& dc,
                                 const wxRect& rect,
                                 wxOrientation orient,
                                 int flags = 0,
                                 long style = 0)
        { m_renderer->DrawSliderThumb(dc, rect, orient, flags, style); }
    virtual void DrawSliderTicks(wxDC& dc,
                                 const wxRect& rect,
                                 int lenThumb,
                                 wxOrientation orient,
                                 int start,
                                 int end,
                                 int WXUNUSED(step) = 1,
                                 int flags = 0,
                                 long style = 0)
        { m_renderer->DrawSliderTicks(dc, rect, lenThumb, orient,
                                      start, end, start, flags, style); }
#endif // wxUSE_SLIDER

#if wxUSE_MENUS
    virtual void DrawMenuBarItem(wxDC& dc,
                                 const wxRect& rect,
                                 const wxString& label,
                                 int flags = 0,
                                 int indexAccel = -1)
        { m_renderer->DrawMenuBarItem(dc, rect, label, flags, indexAccel); }
    virtual void DrawMenuItem(wxDC& dc,
                              wxCoord y,
                              const wxMenuGeometryInfo& gi,
                              const wxString& label,
                              const wxString& accel,
                              const wxBitmap& bitmap = wxNullBitmap,
                              int flags = 0,
                              int indexAccel = -1)
        { m_renderer->DrawMenuItem(dc, y, gi, label, accel,
                                   bitmap, flags, indexAccel); }
    virtual void DrawMenuSeparator(wxDC& dc,
                                   wxCoord y,
                                   const wxMenuGeometryInfo& geomInfo)
        { m_renderer->DrawMenuSeparator(dc, y, geomInfo); }
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    virtual void DrawStatusField(wxDC& dc,
                                 const wxRect& rect,
                                 const wxString& label,
                                 int flags = 0, int style = 0)
        { m_renderer->DrawStatusField(dc, rect, label, flags, style); }
#endif // wxUSE_STATUSBAR

    virtual void DrawFrameTitleBar(wxDC& dc,
                                   const wxRect& rect,
                                   const wxString& title,
                                   const wxIcon& icon,
                                   int flags,
                                   int specialButton = 0,
                                   int specialButtonFlag = 0)
        { m_renderer->DrawFrameTitleBar(dc, rect, title, icon, flags,
                                        specialButton, specialButtonFlag); }
    virtual void DrawFrameBorder(wxDC& dc,
                                 const wxRect& rect,
                                 int flags)
        { m_renderer->DrawFrameBorder(dc, rect, flags); }
    virtual void DrawFrameBackground(wxDC& dc,
                                     const wxRect& rect,
                                     int flags)
        { m_renderer->DrawFrameBackground(dc, rect, flags); }
    virtual void DrawFrameTitle(wxDC& dc,
                                const wxRect& rect,
                                const wxString& title,
                                int flags)
        { m_renderer->DrawFrameTitle(dc, rect, title, flags); }
    virtual void DrawFrameIcon(wxDC& dc,
                               const wxRect& rect,
                               const wxIcon& icon,
                               int flags)
        { m_renderer->DrawFrameIcon(dc, rect, icon, flags); }
    virtual void DrawFrameButton(wxDC& dc,
                                 wxCoord x, wxCoord y,
                                 int button,
                                 int flags = 0)
        { m_renderer->DrawFrameButton(dc, x, y, button, flags); }

#if wxUSE_COMBOBOX
    virtual void GetComboBitmaps(wxBitmap *bmpNormal,
                                 wxBitmap *bmpFocus,
                                 wxBitmap *bmpPressed,
                                 wxBitmap *bmpDisabled)
        { m_renderer->GetComboBitmaps(bmpNormal, bmpFocus,
                                      bmpPressed, bmpDisabled); }
#endif // wxUSE_COMBOBOX

    virtual void AdjustSize(wxSize *size, const wxWindow *window)
        { m_renderer->AdjustSize(size, window); }
    virtual wxRect GetBorderDimensions(wxBorder border) const
        { return m_renderer->GetBorderDimensions(border); }
    virtual bool AreScrollbarsInsideBorder() const
        { return m_renderer->AreScrollbarsInsideBorder(); }

#if wxUSE_SCROLLBAR
    virtual wxSize GetScrollbarArrowSize() const
        { return m_renderer->GetScrollbarArrowSize(); }
#endif // wxUSE_SCROLLBAR

    virtual wxCoord GetListboxItemHeight(wxCoord fontHeight)
        { return m_renderer->GetListboxItemHeight(fontHeight); }
    virtual wxSize GetCheckBitmapSize() const
        { return m_renderer->GetCheckBitmapSize(); }
    virtual wxSize GetRadioBitmapSize() const
        { return m_renderer->GetRadioBitmapSize(); }
    virtual wxCoord GetCheckItemMargin() const
        { return m_renderer->GetCheckItemMargin(); }

#if wxUSE_TOOLBAR
    virtual wxSize GetToolBarButtonSize(wxCoord *separator) const
        { return m_renderer->GetToolBarButtonSize(separator); }
    virtual wxSize GetToolBarMargin() const
        { return m_renderer->GetToolBarMargin(); }
#endif // wxUSE_TOOLBAR

#if wxUSE_TEXTCTRL
    virtual wxRect GetTextTotalArea(const wxTextCtrl *text,
                                    const wxRect& rect) const
        { return m_renderer->GetTextTotalArea(text, rect); }
    virtual wxRect GetTextClientArea(const wxTextCtrl *text,
                                     const wxRect& rect,
                                     wxCoord *extraSpaceBeyond) const
        { return m_renderer->GetTextClientArea(text, rect, extraSpaceBeyond); }
#endif // wxUSE_TEXTCTRL

#if wxUSE_NOTEBOOK
    virtual wxSize GetTabIndent() const { return m_renderer->GetTabIndent(); }
    virtual wxSize GetTabPadding() const { return m_renderer->GetTabPadding(); }
#endif // wxUSE_NOTEBOOK

#if wxUSE_SLIDER
    virtual wxCoord GetSliderDim() const
        { return m_renderer->GetSliderDim(); }
    virtual wxCoord GetSliderTickLen() const
        { return m_renderer->GetSliderTickLen(); }

    virtual wxRect GetSliderShaftRect(const wxRect& rect,
                                      int lenThumb,
                                      wxOrientation orient,
                                      long style = 0) const
        { return m_renderer->GetSliderShaftRect(rect, lenThumb, orient, style); }
    virtual wxSize GetSliderThumbSize(const wxRect& rect,
                                      int lenThumb,
                                      wxOrientation orient) const
        { return m_renderer->GetSliderThumbSize(rect, lenThumb, orient); }
#endif // wxUSE_SLIDER

    virtual wxSize GetProgressBarStep() const
        { return m_renderer->GetProgressBarStep(); }

#if wxUSE_MENUS
    virtual wxSize GetMenuBarItemSize(const wxSize& sizeText) const
        { return m_renderer->GetMenuBarItemSize(sizeText); }
    virtual wxMenuGeometryInfo *GetMenuGeometry(wxWindow *win,
                                                const wxMenu& menu) const
        { return m_renderer->GetMenuGeometry(win, menu); }
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    virtual wxSize GetStatusBarBorders() const
        { return m_renderer->GetStatusBarBorders(); }
    virtual wxCoord GetStatusBarBorderBetweenFields() const
        { return m_renderer->GetStatusBarBorderBetweenFields(); }
    virtual wxSize GetStatusBarFieldMargins() const
        { return m_renderer->GetStatusBarFieldMargins(); }
#endif // wxUSE_STATUSBAR

    virtual wxRect GetFrameClientArea(const wxRect& rect, int flags) const
        { return m_renderer->GetFrameClientArea(rect, flags); }
    virtual wxSize GetFrameTotalSize(const wxSize& clientSize, int flags) const
        { return m_renderer->GetFrameTotalSize(clientSize, flags); }
    virtual wxSize GetFrameMinSize(int flags) const
        { return m_renderer->GetFrameMinSize(flags); }
    virtual wxSize GetFrameIconSize() const
        { return m_renderer->GetFrameIconSize(); }
    virtual int HitTestFrame(const wxRect& rect,
                             const wxPoint& pt,
                             int flags) const
        { return m_renderer->HitTestFrame(rect, pt, flags); }

    virtual int  DrawHeaderButton(wxWindow *win,
                                  wxDC& dc,
                                  const wxRect& rect,
                                  int flags = 0,
                                  wxHeaderSortIconType sortIcon = wxHDR_SORT_ICON_NONE,
                                  wxHeaderButtonParams* params = NULL)
        { return m_renderer->DrawHeaderButton(win, dc, rect, flags, sortIcon, params); }
    virtual void DrawTreeItemButton(wxWindow *win,
                                    wxDC& dc,
                                    const wxRect& rect,
                                    int flags = 0)
        { m_renderer->DrawTreeItemButton(win, dc, rect, flags); }

protected:
    wxRenderer *m_renderer;
};

// ----------------------------------------------------------------------------
// wxControlRenderer: wraps the wxRenderer functions in a form easy to use from
// OnPaint()
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxControlRenderer
{
public:
    // create a renderer for this dc with this "fundamental" renderer
    wxControlRenderer(wxWindow *control, wxDC& dc, wxRenderer *renderer);

    // operations
    void DrawLabel(const wxBitmap& bitmap = wxNullBitmap,
                   wxCoord marginX = 0, wxCoord marginY = 0);
#if wxUSE_LISTBOX
    void DrawItems(const wxListBox *listbox,
                   size_t itemFirst, size_t itemLast);
#endif // wxUSE_LISTBOX
#if wxUSE_CHECKLISTBOX
    void DrawCheckItems(const wxCheckListBox *listbox,
                        size_t itemFirst, size_t itemLast);
#endif // wxUSE_CHECKLISTBOX
    void DrawButtonBorder();
    // the line must be either horizontal or vertical
    void DrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2);
    void DrawFrame();
    void DrawBitmap(const wxBitmap& bitmap);
    void DrawBackgroundBitmap();
    void DrawScrollbar(const wxScrollBar *scrollbar, int thumbPosOld);
#if wxUSE_GAUGE
    void DrawProgressBar(const wxGauge *gauge);
#endif // wxUSE_GAUGE

    // accessors
    wxWindow *GetWindow() const { return m_window; }
    wxRenderer *GetRenderer() const { return m_renderer; }

    wxDC& GetDC() { return m_dc; }

    const wxRect& GetRect() const { return m_rect; }
    wxRect& GetRect() { return m_rect; }

    // static helpers
    static void DrawBitmap(wxDC &dc,
                           const wxBitmap& bitmap,
                           const wxRect& rect,
                           int alignment = wxALIGN_CENTRE |
                                           wxALIGN_CENTRE_VERTICAL,
                           wxStretch stretch = wxSTRETCH_NOT);

private:

#if wxUSE_LISTBOX
    // common part of DrawItems() and DrawCheckItems()
    void DoDrawItems(const wxListBox *listbox,
                     size_t itemFirst, size_t itemLast,
                     bool isCheckLbox = false);
#endif // wxUSE_LISTBOX

    wxWindow *m_window;
    wxRenderer *m_renderer;
    wxDC& m_dc;
    wxRect m_rect;
};

#endif // _WX_UNIV_RENDERER_H_
