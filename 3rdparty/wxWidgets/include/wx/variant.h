/////////////////////////////////////////////////////////////////////////////
// Name:        wx/variant.h
// Purpose:     wxVariant class, container for any type
// Author:      Julian Smart
// Modified by:
// Created:     10/09/98
// RCS-ID:      $Id: variant.h 42997 2006-11-03 21:37:08Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VARIANT_H_
#define _WX_VARIANT_H_

#include "wx/defs.h"

#if wxUSE_VARIANT

#include "wx/object.h"
#include "wx/string.h"
#include "wx/arrstr.h"
#include "wx/list.h"
#include "wx/cpp.h"

#if wxUSE_DATETIME
    #include "wx/datetime.h"
#endif // wxUSE_DATETIME

#if wxUSE_ODBC
    #include "wx/db.h"  // will #include sqltypes.h
#endif //ODBC

#include "wx/iosfwrap.h"

/*
 * wxVariantData stores the actual data in a wxVariant object,
 * to allow it to store any type of data.
 * Derive from this to provide custom data handling.
 *
 * NB: To prevent addition of extra vtbl pointer to wxVariantData,
 *     we don't multiple-inherit from wxObjectRefData. Instead,
 *     we simply replicate the wxObject ref-counting scheme.
 *
 * NB: When you construct a wxVariantData, it will have refcount
 *     of one. Refcount will not be further increased when
 *     it is passed to wxVariant. This simulates old common
 *     scenario where wxVariant took ownership of wxVariantData
 *     passed to it.
 *     If you create wxVariantData for other reasons than passing
 *     it to wxVariant, technically you are not required to call
 *     DecRef() before deleting it.
 *
 * TODO: in order to replace wxPropertyValue, we would need
 * to consider adding constructors that take pointers to C++ variables,
 * or removing that functionality from the wxProperty library.
 * Essentially wxPropertyValue takes on some of the wxValidator functionality
 * by storing pointers and not just actual values, allowing update of C++ data
 * to be handled automatically. Perhaps there's another way of doing this without
 * overloading wxVariant with unnecessary functionality.
 */

class WXDLLIMPEXP_BASE wxVariantData: public wxObject
{
    friend class wxVariant;
public:
    wxVariantData()
        : wxObject(), m_count(1)
    { }

    // Override these to provide common functionality
    virtual bool Eq(wxVariantData& data) const = 0;

#if wxUSE_STD_IOSTREAM
    virtual bool Write(wxSTD ostream& WXUNUSED(str)) const { return false; }
#endif
    virtual bool Write(wxString& WXUNUSED(str)) const { return false; }
#if wxUSE_STD_IOSTREAM
    virtual bool Read(wxSTD istream& WXUNUSED(str)) { return false; }
#endif
    virtual bool Read(wxString& WXUNUSED(str)) { return false; }
    // What type is it? Return a string name.
    virtual wxString GetType() const = 0;
    // If it based on wxObject return the ClassInfo.
    virtual wxClassInfo* GetValueClassInfo() { return NULL; }

    void IncRef() { m_count++; }
    void DecRef()
    {
        if ( --m_count == 0 )
            delete this;
    }

    int GetRefCount() const { return m_count; }

protected:
    // Protected dtor should make some incompatible code
    // break more louder. That is, they should do data->DecRef()
    // instead of delete data.
    virtual ~wxVariantData() { }

private:
    int     m_count;

private:
    DECLARE_ABSTRACT_CLASS(wxVariantData)
};

/*
 * wxVariant can store any kind of data, but has some basic types
 * built in.
 */

class WXDLLIMPEXP_BASE wxVariant: public wxObject
{
public:
    wxVariant();

    wxVariant(const wxVariant& variant);
    wxVariant(wxVariantData* data, const wxString& name = wxEmptyString);
    virtual ~wxVariant();

    // generic assignment
    void operator= (const wxVariant& variant);

    // Assignment using data, e.g.
    // myVariant = new wxStringVariantData("hello");
    void operator= (wxVariantData* variantData);

    bool operator== (const wxVariant& variant) const;
    bool operator!= (const wxVariant& variant) const;

    // Sets/gets name
    inline void SetName(const wxString& name) { m_name = name; }
    inline const wxString& GetName() const { return m_name; }

    // Tests whether there is data
    bool IsNull() const;

    // For compatibility with wxWidgets <= 2.6, this doesn't increase
    // reference count.
    wxVariantData* GetData() const { return m_data; }
    void SetData(wxVariantData* data) ;

    // make a 'clone' of the object
    void Ref(const wxVariant& clone);

    // destroy a reference
    void UnRef();

    // Make NULL (i.e. delete the data)
    void MakeNull();

    // Delete data and name
    void Clear();

    // Returns a string representing the type of the variant,
    // e.g. "string", "bool", "stringlist", "list", "double", "long"
    wxString GetType() const;

    bool IsType(const wxString& type) const;
    bool IsValueKindOf(const wxClassInfo* type) const;

    // write contents to a string (e.g. for debugging)
    wxString MakeString() const;

    // double
    wxVariant(double val, const wxString& name = wxEmptyString);
    bool operator== (double value) const;
    bool operator!= (double value) const;
    void operator= (double value) ;
    inline operator double () const {  return GetDouble(); }
    inline double GetReal() const { return GetDouble(); }
    double GetDouble() const;

    // long
    wxVariant(long val, const wxString& name = wxEmptyString);
    wxVariant(int val, const wxString& name = wxEmptyString);
    wxVariant(short val, const wxString& name = wxEmptyString);
    bool operator== (long value) const;
    bool operator!= (long value) const;
    void operator= (long value) ;
    inline operator long () const {  return GetLong(); }
    inline long GetInteger() const { return GetLong(); }
    long GetLong() const;

    // bool
#ifdef HAVE_BOOL
    wxVariant(bool val, const wxString& name = wxEmptyString);
    bool operator== (bool value) const;
    bool operator!= (bool value) const;
    void operator= (bool value) ;
    inline operator bool () const {  return GetBool(); }
    bool GetBool() const ;
#endif

    // wxDateTime
#if wxUSE_DATETIME
    wxVariant(const wxDateTime& val, const wxString& name = wxEmptyString);
#if wxUSE_ODBC
    wxVariant(const DATE_STRUCT* valptr, const wxString& name = wxEmptyString);
    wxVariant(const TIME_STRUCT* valptr, const wxString& name = wxEmptyString);
    wxVariant(const TIMESTAMP_STRUCT* valptr, const wxString& name = wxEmptyString);
#endif
    bool operator== (const wxDateTime& value) const;
    bool operator!= (const wxDateTime& value) const;
    void operator= (const wxDateTime& value) ;
#if wxUSE_ODBC
    void operator= (const DATE_STRUCT* value) ;
    void operator= (const TIME_STRUCT* value) ;
    void operator= (const TIMESTAMP_STRUCT* value) ;
#endif
    inline operator wxDateTime () const { return GetDateTime(); }
    wxDateTime GetDateTime() const;
#endif

    // wxString
    wxVariant(const wxString& val, const wxString& name = wxEmptyString);
    wxVariant(const wxChar* val, const wxString& name = wxEmptyString); // Necessary or VC++ assumes bool!
    bool operator== (const wxString& value) const;
    bool operator!= (const wxString& value) const;
    void operator= (const wxString& value) ;
    void operator= (const wxChar* value) ; // Necessary or VC++ assumes bool!
    inline operator wxString () const {  return MakeString(); }
    wxString GetString() const;

    // wxChar
    wxVariant(wxChar val, const wxString& name = wxEmptyString);
    bool operator== (wxChar value) const;
    bool operator!= (wxChar value) const;
    void operator= (wxChar value) ;
    inline operator wxChar () const { return GetChar(); }
    wxChar GetChar() const ;

    // wxArrayString
    wxVariant(const wxArrayString& val, const wxString& name = wxEmptyString);
    bool operator== (const wxArrayString& value) const;
    bool operator!= (const wxArrayString& value) const;
    void operator= (const wxArrayString& value);
    inline operator wxArrayString () const { return GetArrayString(); }
    wxArrayString GetArrayString() const;

    // void*
    wxVariant(void* ptr, const wxString& name = wxEmptyString);
    bool operator== (void* value) const;
    bool operator!= (void* value) const;
    void operator= (void* value);
    inline operator void* () const {  return GetVoidPtr(); }
    void* GetVoidPtr() const;

    // wxObject*
    wxVariant(wxObject* ptr, const wxString& name = wxEmptyString);
    bool operator== (wxObject* value) const;
    bool operator!= (wxObject* value) const;
    void operator= (wxObject* value);
    wxObject* GetWxObjectPtr() const;


#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED( wxVariant(const wxStringList& val, const wxString& name = wxEmptyString) );
    wxDEPRECATED( bool operator== (const wxStringList& value) const );
    wxDEPRECATED( bool operator!= (const wxStringList& value) const );
    wxDEPRECATED( void operator= (const wxStringList& value) );
    wxDEPRECATED( wxStringList& GetStringList() const );
#endif

    // ------------------------------
    // list operations
    // ------------------------------

    wxVariant(const wxList& val, const wxString& name = wxEmptyString); // List of variants
    bool operator== (const wxList& value) const;
    bool operator!= (const wxList& value) const;
    void operator= (const wxList& value) ;
    // Treat a list variant as an array
    wxVariant operator[] (size_t idx) const;
    wxVariant& operator[] (size_t idx) ;
    wxList& GetList() const ;

    // Return the number of elements in a list
    size_t GetCount() const;

    // Make empty list
    void NullList();

    // Append to list
    void Append(const wxVariant& value);

    // Insert at front of list
    void Insert(const wxVariant& value);

    // Returns true if the variant is a member of the list
    bool Member(const wxVariant& value) const;

    // Deletes the nth element of the list
    bool Delete(size_t item);

    // Clear list
    void ClearList();

public:
    // Type conversion
    bool Convert(long* value) const;
    bool Convert(bool* value) const;
    bool Convert(double* value) const;
    bool Convert(wxString* value) const;
    bool Convert(wxChar* value) const;
#if wxUSE_DATETIME
    bool Convert(wxDateTime* value) const;
#endif // wxUSE_DATETIME

// Attributes
protected:
    wxVariantData*  m_data;
    wxString        m_name;

private:
    DECLARE_DYNAMIC_CLASS(wxVariant)
};

#define DECLARE_VARIANT_OBJECT(classname) \
    DECLARE_VARIANT_OBJECT_EXPORTED(classname, wxEMPTY_PARAMETER_VALUE)

#define DECLARE_VARIANT_OBJECT_EXPORTED(classname,expdecl) \
expdecl classname& operator << ( classname &object, const wxVariant &variant ); \
expdecl wxVariant& operator << ( wxVariant &variant, const classname &object );

#define IMPLEMENT_VARIANT_OBJECT(classname) \
    IMPLEMENT_VARIANT_OBJECT_EXPORTED(classname, wxEMPTY_PARAMETER_VALUE)

#define IMPLEMENT_VARIANT_OBJECT_EXPORTED_NO_EQ(classname,expdecl) \
class classname##VariantData: public wxVariantData \
{ \
public:\
    classname##VariantData() {} \
    classname##VariantData( const classname &value ) { m_value = value; } \
\
    classname &GetValue() { return m_value; } \
\
    virtual bool Eq(wxVariantData& data) const; \
\
    virtual wxString GetType() const; \
    virtual wxClassInfo* GetValueClassInfo(); \
\
protected:\
    classname m_value; \
\
private: \
    DECLARE_CLASS(classname##VariantData) \
};\
\
IMPLEMENT_CLASS(classname##VariantData, wxVariantData)\
\
wxString classname##VariantData::GetType() const\
{\
    return m_value.GetClassInfo()->GetClassName();\
}\
\
wxClassInfo* classname##VariantData::GetValueClassInfo()\
{\
    return m_value.GetClassInfo();\
}\
\
expdecl classname& operator << ( classname &value, const wxVariant &variant )\
{\
    wxASSERT( wxIsKindOf( variant.GetData(), classname##VariantData ) );\
    \
    classname##VariantData *data = (classname##VariantData*) variant.GetData();\
    value = data->GetValue();\
    return value;\
}\
\
expdecl wxVariant& operator << ( wxVariant &variant, const classname &value )\
{\
    classname##VariantData *data = new classname##VariantData( value );\
    variant.SetData( data );\
    return variant;\
}

// implements a wxVariantData-derived class using for the Eq() method the
// operator== which must have been provided by "classname"
#define IMPLEMENT_VARIANT_OBJECT_EXPORTED(classname,expdecl) \
IMPLEMENT_VARIANT_OBJECT_EXPORTED_NO_EQ(classname,wxEMPTY_PARAMETER_VALUE expdecl) \
\
bool classname##VariantData::Eq(wxVariantData& data) const \
{\
    wxASSERT( wxIsKindOf((&data), classname##VariantData) );\
\
    classname##VariantData & otherData = (classname##VariantData &) data;\
\
    return otherData.m_value == m_value;\
}\


// implements a wxVariantData-derived class using for the Eq() method a shallow
// comparison (through wxObject::IsSameAs function)
#define IMPLEMENT_VARIANT_OBJECT_SHALLOWCMP(classname) \
    IMPLEMENT_VARIANT_OBJECT_EXPORTED_SHALLOWCMP(classname, wxEMPTY_PARAMETER_VALUE)
#define IMPLEMENT_VARIANT_OBJECT_EXPORTED_SHALLOWCMP(classname,expdecl) \
IMPLEMENT_VARIANT_OBJECT_EXPORTED_NO_EQ(classname,wxEMPTY_PARAMETER_VALUE expdecl) \
\
bool classname##VariantData::Eq(wxVariantData& data) const \
{\
    wxASSERT( wxIsKindOf((&data), classname##VariantData) );\
\
    classname##VariantData & otherData = (classname##VariantData &) data;\
\
    return (otherData.m_value.IsSameAs(m_value));\
}\


// Since we want type safety wxVariant we need to fetch and dynamic_cast
// in a seemingly safe way so the compiler can check, so we define
// a dynamic_cast /wxDynamicCast analogue.

#define wxGetVariantCast(var,classname) \
    ((classname*)(var.IsValueKindOf(&classname::ms_classInfo) ?\
                  var.GetWxObjectPtr() : NULL));

extern wxVariant WXDLLIMPEXP_BASE wxNullVariant;

#endif // wxUSE_VARIANT

#endif // _WX_VARIANT_H_
