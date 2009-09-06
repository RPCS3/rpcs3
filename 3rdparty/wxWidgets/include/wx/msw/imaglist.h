/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/imaglist.h
// Purpose:     wxImageList class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: imaglist.h 41271 2006-09-18 04:41:09Z KO $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IMAGLIST_H_
#define _WX_IMAGLIST_H_

#include "wx/bitmap.h"

// Eventually we'll make this a reference-counted wxGDIObject. For
// now, the app must take care of ownership issues. That is, the
// image lists must be explicitly deleted after the control(s) that uses them
// is (are) deleted, or when the app exits.
class WXDLLEXPORT wxImageList : public wxObject
{
public:
  /*
   * Public interface
   */

  wxImageList();

  // Creates an image list.
  // Specify the width and height of the images in the list,
  // whether there are masks associated with them (e.g. if creating images
  // from icons), and the initial size of the list.
  wxImageList(int width, int height, bool mask = true, int initialCount = 1)
  {
    Create(width, height, mask, initialCount);
  }
  virtual ~wxImageList();


  // Attributes
  ////////////////////////////////////////////////////////////////////////////

  // Returns the number of images in the image list.
  int GetImageCount() const;

  // Returns the size (same for all images) of the images in the list
  bool GetSize(int index, int &width, int &height) const;

  // Operations
  ////////////////////////////////////////////////////////////////////////////

  // Creates an image list
  // width, height specify the size of the images in the list (all the same).
  // mask specifies whether the images have masks or not.
  // initialNumber is the initial number of images to reserve.
  bool Create(int width, int height, bool mask = true, int initialNumber = 1);

  // Adds a bitmap, and optionally a mask bitmap.
  // Note that wxImageList creates *new* bitmaps, so you may delete
  // 'bitmap' and 'mask' after calling Add.
  int Add(const wxBitmap& bitmap, const wxBitmap& mask = wxNullBitmap);

  // Adds a bitmap, using the specified colour to create the mask bitmap
  // Note that wxImageList creates *new* bitmaps, so you may delete
  // 'bitmap' after calling Add.
  int Add(const wxBitmap& bitmap, const wxColour& maskColour);

  // Adds a bitmap and mask from an icon.
  int Add(const wxIcon& icon);

  // Replaces a bitmap, optionally passing a mask bitmap.
  // Note that wxImageList creates new bitmaps, so you may delete
  // 'bitmap' and 'mask' after calling Replace.
  bool Replace(int index, const wxBitmap& bitmap, const wxBitmap& mask = wxNullBitmap);

/* Not supported by Win95
  // Replacing a bitmap, using the specified colour to create the mask bitmap
  // Note that wxImageList creates new bitmaps, so you may delete
  // 'bitmap'.
  bool Replace(int index, const wxBitmap& bitmap, const wxColour& maskColour);
*/

  // Replaces a bitmap and mask from an icon.
  // You can delete 'icon' after calling Replace.
  bool Replace(int index, const wxIcon& icon);

  // Removes the image at the given index.
  bool Remove(int index);

  // Remove all images
  bool RemoveAll();

  // Draws the given image on a dc at the specified position.
  // If 'solidBackground' is true, Draw sets the image list background
  // colour to the background colour of the wxDC, to speed up
  // drawing by eliminating masked drawing where possible.
  bool Draw(int index, wxDC& dc, int x, int y,
            int flags = wxIMAGELIST_DRAW_NORMAL,
            bool solidBackground = false);

  // Get a bitmap
  wxBitmap GetBitmap(int index) const;

  // Get an icon
  wxIcon GetIcon(int index) const;

  // TODO: miscellaneous functionality
/*
  wxIcon *MakeIcon(int index);
  bool SetOverlayImage(int index, int overlayMask);

*/

  // TODO: Drag-and-drop related functionality.

#if 0
  // Creates a new drag image by combining the given image (typically a mouse cursor image)
  // with the current drag image.
  bool SetDragCursorImage(int index, const wxPoint& hotSpot);

  // If successful, returns a pointer to the temporary image list that is used for dragging;
  // otherwise, NULL.
  // dragPos: receives the current drag position.
  // hotSpot: receives the offset of the drag image relative to the drag position.
  static wxImageList *GetDragImageList(wxPoint& dragPos, wxPoint& hotSpot);

  // Call this function to begin dragging an image. This function creates a temporary image list
  // that is used for dragging. The image combines the specified image and its mask with the
  // current cursor. In response to subsequent mouse move messages, you can move the drag image
  // by using the DragMove member function. To end the drag operation, you can use the EndDrag
  // member function.
  bool BeginDrag(int index, const wxPoint& hotSpot);

  // Ends a drag operation.
  bool EndDrag();

  // Call this function to move the image that is being dragged during a drag-and-drop operation.
  // This function is typically called in response to a mouse move message. To begin a drag
  // operation, use the BeginDrag member function.
  static bool DragMove(const wxPoint& point);

  // During a drag operation, locks updates to the window specified by lockWindow and displays
  // the drag image at the position specified by point.
  // The coordinates are relative to the window's upper left corner, so you must compensate
  // for the widths of window elements, such as the border, title bar, and menu bar, when
  // specifying the coordinates.
  // If lockWindow is NULL, this function draws the image in the display context associated
  // with the desktop window, and coordinates are relative to the upper left corner of the screen.
  // This function locks all other updates to the given window during the drag operation.
  // If you need to do any drawing during a drag operation, such as highlighting the target
  // of a drag-and-drop operation, you can temporarily hide the dragged image by using the
  // wxImageList::DragLeave function.

  // lockWindow: pointer to the window that owns the drag image.
  // point:      position at which to display the drag image. Coordinates are relative to the
  //             upper left corner of the window (not the client area).

  static bool DragEnter( wxWindow *lockWindow, const wxPoint& point );

  // Unlocks the window specified by pWndLock and hides the drag image, allowing the
  // window to be updated.
  static bool DragLeave( wxWindow *lockWindow );

  /* Here's roughly how you'd use these functions if implemented in this Win95-like way:

  1) Starting to drag:

  wxImageList *dragImageList = new wxImageList(16, 16, true);
  dragImageList->Add(myDragImage); // Provide an image to combine with the current cursor
  dragImageList->BeginDrag(0, wxPoint(0, 0));
  wxShowCursor(false);        // wxShowCursor not yet implemented in wxWin
  myWindow->CaptureMouse();

  2) Dragging:

  // Called within mouse move event. Could also use dragImageList instead of assuming
  // these are static functions.
  // These two functions could possibly be combined into one, since DragEnter is
  // a bit obscure.
  wxImageList::DragMove(wxPoint(x, y));  // x, y are current cursor position
  wxImageList::DragEnter(NULL, wxPoint(x, y)); // NULL assumes dragging across whole screen

  3) Finishing dragging:

  dragImageList->EndDrag();
  myWindow->ReleaseMouse();
  wxShowCursor(true);
*/

#endif

  // Implementation
  ////////////////////////////////////////////////////////////////////////////

  // Returns the native image list handle
  WXHIMAGELIST GetHIMAGELIST() const { return m_hImageList; }

protected:
  WXHIMAGELIST m_hImageList;

  DECLARE_DYNAMIC_CLASS(wxImageList)
};

#endif
    // _WX_IMAGLIST_H_
