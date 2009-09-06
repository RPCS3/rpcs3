/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/accel.h
// Purpose:     wxAcceleratorTable class
// Author:      Robert Roebling
// RCS-ID:      $Id: accel.h 42752 2006-10-30 19:26:48Z VZ $
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_ACCEL_H_
#define _WX_GENERIC_ACCEL_H_

class WXDLLEXPORT wxKeyEvent;

// ----------------------------------------------------------------------------
// wxAcceleratorTable
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxAcceleratorTable : public wxObject
{
public:
    wxAcceleratorTable();
    wxAcceleratorTable(int n, const wxAcceleratorEntry entries[]);
    virtual ~wxAcceleratorTable();

    bool Ok() const { return IsOk(); }
    bool IsOk() const;

    void Add(const wxAcceleratorEntry& entry);
    void Remove(const wxAcceleratorEntry& entry);

    // implementation
    // --------------

    wxMenuItem *GetMenuItem(const wxKeyEvent& event) const;
    int GetCommand(const wxKeyEvent& event) const;

    const wxAcceleratorEntry *GetEntry(const wxKeyEvent& event) const;

protected:
    // ref counting code
    virtual wxObjectRefData *CreateRefData() const;
    virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const;

private:
    DECLARE_DYNAMIC_CLASS(wxAcceleratorTable)
};

#endif // _WX_GENERIC_ACCEL_H_

