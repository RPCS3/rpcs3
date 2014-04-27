///////////////////////////////////////////////////////////////////////////////
// Name:        msw/ole/dataobj.h
// Purpose:     declaration of the wxDataObject class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     10.05.98
// RCS-ID:      $Id: dataobj.h 37406 2006-02-09 03:45:14Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _WX_MSW_OLE_DATAOBJ_H
#define   _WX_MSW_OLE_DATAOBJ_H

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

struct IDataObject;

// ----------------------------------------------------------------------------
// wxDataObject is a "smart" and polymorphic piece of data.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDataObject : public wxDataObjectBase
{
public:
    // ctor & dtor
    wxDataObject();
    virtual ~wxDataObject();

    // retrieve IDataObject interface (for other OLE related classes)
    IDataObject *GetInterface() const { return m_pIDataObject; }

    // tell the object that it should be now owned by IDataObject - i.e. when
    // it is deleted, it should delete us as well
    void SetAutoDelete();

    // return true if we support this format in "Get" direction
    bool IsSupportedFormat(const wxDataFormat& format) const
        { return wxDataObjectBase::IsSupported(format, Get); }

    // if this method returns false, this wxDataObject will be copied to
    // the clipboard with its size prepended to it, which is compatible with
    // older wx versions
    //
    // if returns true, then this wxDataObject will be copied to the clipboard
    // without any additional information and ::HeapSize() function will be used
    // to get the size of that data
    virtual bool NeedsVerbatimData(const wxDataFormat& WXUNUSED(format)) const
    {
        // return false from here only for compatibility with earlier wx versions
        return true;
    }

    // function to return symbolic name of clipboard format (for debug messages)
#ifdef __WXDEBUG__
    static const wxChar *GetFormatName(wxDataFormat format);

    #define wxGetFormatName(format) wxDataObject::GetFormatName(format)
#else // !Debug
    #define wxGetFormatName(format) _T("")
#endif // Debug/!Debug
    // they need to be accessed from wxIDataObject, so made them public,
    // or wxIDataObject friend
public:
    virtual const void* GetSizeFromBuffer( const void* buffer, size_t* size,
                                           const wxDataFormat& format );
    virtual void* SetSizeInBuffer( void* buffer, size_t size,
                                   const wxDataFormat& format );
    virtual size_t GetBufferOffset( const wxDataFormat& format );

private:
    IDataObject *m_pIDataObject; // pointer to the COM interface

    DECLARE_NO_COPY_CLASS(wxDataObject)
};

#endif  //_WX_MSW_OLE_DATAOBJ_H
