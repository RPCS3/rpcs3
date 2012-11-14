/////////////////////////////////////////////////////////////////////////////
// Name:        wx/ctrlsub.h (read: "wxConTRoL with SUBitems")
// Purpose:     wxControlWithItems interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     22.10.99
// RCS-ID:      $Id: ctrlsub.h 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CTRLSUB_H_BASE_
#define _WX_CTRLSUB_H_BASE_

#include "wx/defs.h"

#if wxUSE_CONTROLS

#include "wx/control.h"      // base class

// ----------------------------------------------------------------------------
// wxItemContainer defines an interface which is implemented by all controls
// which have string subitems each of which may be selected.
//
// It is decomposed in wxItemContainerImmutable which omits all methods
// adding/removing items and is used by wxRadioBox and wxItemContainer itself.
//
// Examples: wxListBox, wxCheckListBox, wxChoice and wxComboBox (which
// implements an extended interface deriving from this one)
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxItemContainerImmutable
{
public:
    wxItemContainerImmutable() { }
    virtual ~wxItemContainerImmutable();

    // accessing strings
    // -----------------

    virtual unsigned int GetCount() const = 0;
    bool IsEmpty() const { return GetCount() == 0; }

    virtual wxString GetString(unsigned int n) const = 0;
    wxArrayString GetStrings() const;
    virtual void SetString(unsigned int n, const wxString& s) = 0;

    // finding string natively is either case sensitive or insensitive
    // but never both so fall back to this base version for not
    // supported search type
    virtual int FindString(const wxString& s, bool bCase = false) const
    {
        unsigned int count = GetCount();

        for ( unsigned int i = 0; i < count ; ++i )
        {
            if (GetString(i).IsSameAs( s , bCase ))
                return (int)i;
        }

        return wxNOT_FOUND;
    }


    // selection
    // ---------

    virtual void SetSelection(int n) = 0;
    virtual int GetSelection() const = 0;

    // set selection to the specified string, return false if not found
    bool SetStringSelection(const wxString& s);

    // return the selected string or empty string if none
    wxString GetStringSelection() const;

    // this is the same as SetSelection( for single-selection controls but
    // reads better for multi-selection ones
    void Select(int n) { SetSelection(n); }


protected:

    // check that the index is valid
    inline bool IsValid(unsigned int n) const { return n < GetCount(); }
    inline bool IsValidInsert(unsigned int n) const { return n <= GetCount(); }
};

class WXDLLEXPORT wxItemContainer : public wxItemContainerImmutable
{
public:
    wxItemContainer() { m_clientDataItemsType = wxClientData_None; }
    virtual ~wxItemContainer();

    // adding items
    // ------------

    int Append(const wxString& item)
        { return DoAppend(item); }
    int Append(const wxString& item, void *clientData)
        { int n = DoAppend(item); SetClientData(n, clientData); return n; }
    int Append(const wxString& item, wxClientData *clientData)
        { int n = DoAppend(item); SetClientObject(n, clientData); return n; }

    // only for rtti needs (separate name)
    void AppendString( const wxString& item)
        { Append( item ); }

    // append several items at once to the control
    void Append(const wxArrayString& strings);

    int Insert(const wxString& item, unsigned int pos)
        { return DoInsert(item, pos); }
    int Insert(const wxString& item, unsigned int pos, void *clientData);
    int Insert(const wxString& item, unsigned int pos, wxClientData *clientData);

    // deleting items
    // --------------

    virtual void Clear() = 0;
    virtual void Delete(unsigned int n) = 0;

    // misc
    // ----

    // client data stuff
    void SetClientData(unsigned int n, void* clientData);
    void* GetClientData(unsigned int n) const;

    void SetClientObject(unsigned int n, wxClientData* clientData);
    wxClientData* GetClientObject(unsigned int n) const;

    bool HasClientObjectData() const
        { return m_clientDataItemsType == wxClientData_Object; }
    bool HasClientUntypedData() const
        { return m_clientDataItemsType == wxClientData_Void; }

protected:
    virtual int DoAppend(const wxString& item) = 0;
    virtual int DoInsert(const wxString& item, unsigned int pos) = 0;

    virtual void DoSetItemClientData(unsigned int n, void* clientData) = 0;
    virtual void* DoGetItemClientData(unsigned int n) const = 0;
    virtual void DoSetItemClientObject(unsigned int n, wxClientData* clientData) = 0;
    virtual wxClientData* DoGetItemClientObject(unsigned int n) const = 0;


    // the type of the client data for the items
    wxClientDataType m_clientDataItemsType;
};

// this macro must (unfortunately) be used in any class deriving from both
// wxItemContainer and wxControl because otherwise there is ambiguity when
// calling GetClientXXX() functions -- the compiler can't choose between the
// two versions
#define wxCONTROL_ITEMCONTAINER_CLIENTDATAOBJECT_RECAST                    \
    void SetClientData(void *data)                                         \
        { wxEvtHandler::SetClientData(data); }                             \
    void *GetClientData() const                                            \
        { return wxEvtHandler::GetClientData(); }                          \
    void SetClientObject(wxClientData *data)                               \
        { wxEvtHandler::SetClientObject(data); }                           \
    wxClientData *GetClientObject() const                                  \
        { return wxEvtHandler::GetClientObject(); }                        \
    void SetClientData(unsigned int n, void* clientData)                   \
        { wxItemContainer::SetClientData(n, clientData); }                 \
    void* GetClientData(unsigned int n) const                              \
        { return wxItemContainer::GetClientData(n); }                      \
    void SetClientObject(unsigned int n, wxClientData* clientData)         \
        { wxItemContainer::SetClientObject(n, clientData); }               \
    wxClientData* GetClientObject(unsigned int n) const                    \
        { return wxItemContainer::GetClientObject(n); }

class WXDLLEXPORT wxControlWithItems : public wxControl, public wxItemContainer
{
public:
    wxControlWithItems() { }
    virtual ~wxControlWithItems();

    // we have to redefine these functions here to avoid ambiguities in classes
    // deriving from us which would arise otherwise because both base classses
    // have the methods with the same names - hopefully, a smart compiler can
    // optimize away these simple inline wrappers so we don't suffer much from
    // this
    wxCONTROL_ITEMCONTAINER_CLIENTDATAOBJECT_RECAST

    // usually the controls like list/combo boxes have their own background
    // colour
    virtual bool ShouldInheritColours() const { return false; }

protected:
    // fill in the client object or data field of the event as appropriate
    //
    // calls InitCommandEvent() and, if n != wxNOT_FOUND, also sets the per
    // item client data
    void InitCommandEventWithItems(wxCommandEvent& event, int n);

private:
    DECLARE_ABSTRACT_CLASS(wxControlWithItems)
    DECLARE_NO_COPY_CLASS(wxControlWithItems)
};


// ----------------------------------------------------------------------------
// inline functions
// ----------------------------------------------------------------------------

#endif // wxUSE_CONTROLS

#endif // _WX_CTRLSUB_H_BASE_
