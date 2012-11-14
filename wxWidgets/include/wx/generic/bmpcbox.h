/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/bmpcbox.h
// Purpose:     wxBitmapComboBox
// Author:      Jaakko Salli
// Modified by:
// Created:     Aug-30-2006
// RCS-ID:      $Id: bmpcbox.h 42046 2006-10-16 09:30:01Z ABX $
// Copyright:   (c) Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_BMPCBOX_H_
#define _WX_GENERIC_BMPCBOX_H_


#define wxGENERIC_BITMAPCOMBOBOX     1

#include "wx/odcombo.h"

// ----------------------------------------------------------------------------
// wxBitmapComboBox: a wxComboBox that allows images to be shown
// in front of string items.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxBitmapComboBox : public wxOwnerDrawnComboBox,
                                         public wxBitmapComboBoxBase
{
public:

    // ctors and such
    wxBitmapComboBox() : wxOwnerDrawnComboBox(), wxBitmapComboBoxBase()
    {
        Init();
    }

    wxBitmapComboBox(wxWindow *parent,
                     wxWindowID id = wxID_ANY,
                     const wxString& value = wxEmptyString,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     int n = 0,
                     const wxString choices[] = NULL,
                     long style = 0,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxBitmapComboBoxNameStr)
        : wxOwnerDrawnComboBox(),
          wxBitmapComboBoxBase()
    {
        Init();

        (void)Create(parent, id, value, pos, size, n,
                     choices, style, validator, name);
    }

    wxBitmapComboBox(wxWindow *parent,
                     wxWindowID id,
                     const wxString& value,
                     const wxPoint& pos,
                     const wxSize& size,
                     const wxArrayString& choices,
                     long style,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxBitmapComboBoxNameStr);

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                int n,
                const wxString choices[],
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxBitmapComboBoxNameStr);

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxBitmapComboBoxNameStr);

    virtual ~wxBitmapComboBox();

    // Adds item with image to the end of the combo box.
    int Append(const wxString& item, const wxBitmap& bitmap = wxNullBitmap)
        { return DoAppendWithImage(item, bitmap); }

    int Append(const wxString& item, const wxBitmap& bitmap, void *clientData)
        { int n = DoAppendWithImage(item, bitmap); SetClientData(n, clientData); return n; }
    int Append(const wxString& item, const wxBitmap& bitmap, wxClientData *clientData)
        { int n = DoAppendWithImage(item, bitmap); SetClientObject(n, clientData); return n; }

    // Returns size of image used in list.
    virtual wxSize GetBitmapSize() const
    {
        return m_usedImgSize;
    }

    // Returns the image of the item with the given index.
    virtual wxBitmap GetItemBitmap(unsigned int n) const;

    // Inserts item with image into the list before pos. Not valid for wxCB_SORT or wxCB_SORT
    // styles, use Append instead.
    int Insert(const wxString& item, const wxBitmap& bitmap, unsigned int pos)
        { return DoInsertWithImage(item, bitmap, pos); }

    int Insert(const wxString& item, const wxBitmap& bitmap,
               unsigned int pos, void *clientData);
    int Insert(const wxString& item, const wxBitmap& bitmap,
               unsigned int pos, wxClientData *clientData);

    // Sets the image for the given item.
    virtual void SetItemBitmap(unsigned int n, const wxBitmap& bitmap);

    virtual void Clear();
    virtual void Delete(unsigned int n);

protected:

    virtual void OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const;
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const;
    virtual wxCoord OnMeasureItem(size_t item) const;
    virtual wxCoord OnMeasureItemWidth(size_t item) const;

    virtual int DoAppendWithImage(const wxString& item, const wxBitmap& bitmap);
    virtual int DoInsertWithImage(const wxString& item, const wxBitmap& bitmap,
                                  unsigned int pos);

    virtual int DoAppend(const wxString& item);
    virtual int DoInsert(const wxString& item, unsigned int pos);

    virtual bool SetFont(const wxFont& font);

    virtual wxSize DoGetBestSize() const;

    // Event handlers
    void OnSize(wxSizeEvent& event);

    // Recalculates amount of empty space needed in front of
    // text in control itself.
    void DetermineIndent();

    bool OnAddBitmap(const wxBitmap& bitmap);

    // Adds image to position - called in Append/Insert before
    // string is added.
    bool DoInsertBitmap(const wxBitmap& image, unsigned int pos);


    wxArrayPtrVoid      m_bitmaps;  // Images associated with items
    wxSize              m_usedImgSize;  // Size of bitmaps

private:
    int                 m_imgAreaWidth;  // Width and height of area next to text field
    int                 m_fontHeight;
    bool                m_inResize;

    void Init();
    void PostCreate();

    DECLARE_EVENT_TABLE()

    DECLARE_DYNAMIC_CLASS(wxBitmapComboBox)
};

#endif // _WX_GENERIC_BMPCBOX_H_
