/////////////////////////////////////////////////////////////////////////////
// Name:        wx/bmpcbox.h
// Purpose:     wxBitmapComboBox base header
// Author:      Jaakko Salli
// Modified by:
// Created:     Aug-31-2006
// Copyright:   (c) Jaakko Salli
// RCS-ID:      $Id: bmpcbox.h 42046 2006-10-16 09:30:01Z ABX $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BMPCBOX_H_BASE_
#define _WX_BMPCBOX_H_BASE_


#include "wx/defs.h"

#if wxUSE_BITMAPCOMBOBOX

#include "wx/bitmap.h"


extern WXDLLIMPEXP_DATA_ADV(const wxChar) wxBitmapComboBoxNameStr[];


class WXDLLIMPEXP_ADV wxBitmapComboBoxBase
{
public:
    // ctors and such
    wxBitmapComboBoxBase() { }

    virtual ~wxBitmapComboBoxBase() { }

    // Returns the image of the item with the given index.
    virtual wxBitmap GetItemBitmap(unsigned int n) const = 0;

    // Sets the image for the given item.
    virtual void SetItemBitmap(unsigned int n, const wxBitmap& bitmap) = 0;

    // Returns size of the image used in list
    virtual wxSize GetBitmapSize() const = 0;
};


#include "wx/generic/bmpcbox.h"

#endif // wxUSE_BITMAPCOMBOBOX

#endif // _WX_BMPCBOX_H_BASE_
