///////////////////////////////////////////////////////////////////////////////
// Name:        ole/droptgt.h
// Purpose:     declaration of the wxDropTarget class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.03.98
// RCS-ID:      $Id: droptgt.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _WX_OLEDROPTGT_H
#define   _WX_OLEDROPTGT_H

#if wxUSE_DRAG_AND_DROP

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class  wxIDropTarget;
struct IDataObject;

// ----------------------------------------------------------------------------
// An instance of the class wxDropTarget may be associated with any wxWindow
// derived object via SetDropTarget() function. If this is done, the virtual
// methods of wxDropTarget are called when something is dropped on the window.
//
// Note that wxDropTarget is an abstract base class (ABC) and you should derive
// your own class from it implementing pure virtual function in order to use it
// (all of them, including protected ones which are called by the class itself)
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDropTarget : public wxDropTargetBase
{
public:
    // ctor & dtor
    wxDropTarget(wxDataObject *dataObject = NULL);
    virtual ~wxDropTarget();

    // normally called by wxWindow on window creation/destruction, but might be
    // called `manually' as well. Register() returns true on success.
    bool Register(WXHWND hwnd);
    void Revoke(WXHWND hwnd);

    // provide default implementation for base class pure virtuals
    virtual bool OnDrop(wxCoord x, wxCoord y);
    virtual bool GetData();

    // implementation only from now on
    // -------------------------------

    // do we accept this kind of data?
    bool IsAcceptedData(IDataObject *pIDataSource) const;

    // give us the data source from IDropTarget::Drop() - this is later used by
    // GetData() when it's called from inside OnData()
    void SetDataSource(IDataObject *pIDataSource);

private:
    // helper used by IsAcceptedData() and GetData()
    wxDataFormat GetSupportedFormat(IDataObject *pIDataSource) const;

    wxIDropTarget *m_pIDropTarget; // the pointer to our COM interface
    IDataObject   *m_pIDataSource; // the pointer to the source data object

    DECLARE_NO_COPY_CLASS(wxDropTarget)
};

#endif  //wxUSE_DRAG_AND_DROP

#endif  //_WX_OLEDROPTGT_H
