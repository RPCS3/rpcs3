/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xti.h
// Purpose:     runtime metadata information (extended class info)
// Author:      Stefan Csomor
// Modified by:
// Created:     27/07/03
// RCS-ID:      $Id: xti.h 52488 2008-03-14 14:13:05Z JS $
// Copyright:   (c) 1997 Julian Smart
//              (c) 2003 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XTIH__
#define _WX_XTIH__

// We want to support properties, event sources and events sinks through
// explicit declarations, using templates and specialization to make the
// effort as painless as possible.
//
// This means we have the following domains :
//
// - Type Information for categorizing built in types as well as custom types
// this includes information about enums, their values and names
// - Type safe value storage : a kind of wxVariant, called right now wxxVariant
// which will be merged with wxVariant
// - Property Information and Property Accessors providing access to a class'
// values and exposed event delegates
// - Information about event handlers
// - extended Class Information for accessing all these

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"
#include "wx/memory.h"
#include "wx/flags.h"
#include "wx/string.h"
#include "wx/arrstr.h"
#include "wx/hashmap.h"
#include "wx/log.h"
#include  "wx/intl.h"

#include <typeinfo>

// we will move this later to defs.h

#if defined(__GNUC__) && !wxCHECK_GCC_VERSION( 3 , 4 )
#  define wxUSE_MEMBER_TEMPLATES 0
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#  define wxUSE_MEMBER_TEMPLATES 0
#  define wxUSE_FUNC_TEMPLATE_POINTER 0
#endif

#ifndef wxUSE_MEMBER_TEMPLATES
#  define wxUSE_MEMBER_TEMPLATES 1
#endif

#ifndef wxUSE_FUNC_TEMPLATE_POINTER
#  define wxUSE_FUNC_TEMPLATE_POINTER 1
#endif

#if wxUSE_MEMBER_TEMPLATES
#  define wxTEMPLATED_MEMBER_CALL( method , type ) method<type>()
#  define wxTEMPLATED_MEMBER_FIX( type )
#else
#  define wxTEMPLATED_MEMBER_CALL( method , type ) method((type*)NULL)
#  define wxTEMPLATED_MEMBER_FIX( type ) type* =NULL
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#  define wxTEMPLATED_FUNCTION_FIX( type ) , wxTEMPLATED_MEMBER_FIX(type)
#  define wxINFUNC_CLASS_TYPE_FIX( type ) typedef type type;
#else
#  define wxTEMPLATED_FUNCTION_FIX( type )
#  define wxINFUNC_CLASS_TYPE_FIX( type )
#endif

#define EMPTY_MACROVALUE /**/

class WXDLLIMPEXP_BASE wxObject;
class WXDLLIMPEXP_BASE wxClassInfo;
class WXDLLIMPEXP_BASE wxDynamicClassInfo;
class WXDLLIMPEXP_BASE wxHashTable;
class WXDLLIMPEXP_BASE wxObjectRefData;
class WXDLLIMPEXP_BASE wxEvent;
class WXDLLIMPEXP_BASE wxEvtHandler;

typedef void (wxObject::*wxObjectEventFunction)(wxEvent&);

#if wxUSE_FUNC_TEMPLATE_POINTER
#  define wxTO_STRING(type) wxToStringConverter<type>
#  define wxTO_STRING_IMP(type)
#  define wxFROM_STRING(type) wxFromStringConverter<type>
#  define wxFROM_STRING_IMP(type)
#else
#  define wxTO_STRING(type) ToString##type
#  define wxTO_STRING_IMP(type) inline void ToString##type( const wxxVariant& data , wxString &result ) { wxToStringConverter<type>(data, result); }
#  define wxFROM_STRING(type) FromString##type
#  define wxFROM_STRING_IMP(type) inline void FromString##type( const wxString& data , wxxVariant &result ) { wxFromStringConverter<type>(data, result); }
#endif

// ----------------------------------------------------------------------------
// Enum Support
//
// In the header files there would no change from pure c++ code, in the
// implementation, an enum would have
// to be enumerated eg :
//
// wxBEGIN_ENUM( wxFlavor )
//   wxENUM_MEMBER( Vanilla )
//   wxENUM_MEMBER( Chocolate )
//   wxENUM_MEMBER( Strawberry )
// wxEND_ENUM( wxFlavor )
// ----------------------------------------------------------------------------

struct WXDLLIMPEXP_BASE wxEnumMemberData
{
    const wxChar*   m_name;
    int             m_value;
};

class WXDLLIMPEXP_BASE wxEnumData
{
public :
    wxEnumData( wxEnumMemberData* data ) ;

    // returns true if the member has been found and sets the int value
    // pointed to accordingly (if ptr != null )
    // if not found returns false, value left unchanged
    bool HasEnumMemberValue( const wxChar *name , int *value = NULL ) const ;

    // returns the value of the member, if not found in debug mode an
    // assert is issued, in release 0 is returned
    int GetEnumMemberValue(const wxChar *name ) const ;

    // returns the name of the enum member having the passed in value
    // returns an emtpy string if not found
    const wxChar *GetEnumMemberName(int value) const ;

    // returns the number of members in this enum
    int GetEnumCount() const { return m_count ; }

    // returns the value of the nth member
    int GetEnumMemberValueByIndex( int n ) const ;

    // returns the value of the nth member
    const wxChar *GetEnumMemberNameByIndex( int n ) const ;
private :
    wxEnumMemberData *m_members;
    int m_count ;
};

#define wxBEGIN_ENUM( e ) \
    wxEnumMemberData s_enumDataMembers##e[] = {

#define wxENUM_MEMBER( v ) { wxT(#v), v } ,

#define wxEND_ENUM( e ) { NULL , 0 } } ; \
    wxEnumData s_enumData##e( s_enumDataMembers##e ) ; \
    wxEnumData *wxGetEnumData(e) { return &s_enumData##e ; } \
    template<>  void wxStringReadValue(const wxString& s , e &data ) \
{ \
    data = (e) s_enumData##e.GetEnumMemberValue(s) ; \
} \
    template<>  void wxStringWriteValue(wxString &s , const e &data ) \
{ \
    s =  s_enumData##e.GetEnumMemberName((int)data) ; \
} \
    void FromLong##e( long data , wxxVariant& result ) { result = wxxVariant((e)data) ;} \
    void ToLong##e( const wxxVariant& data , long &result ) { result = (long) data.wxTEMPLATED_MEMBER_CALL(Get , e) ;} \
    wxTO_STRING_IMP( e ) \
    wxFROM_STRING_IMP( e ) \
    wxEnumTypeInfo s_typeInfo##e(wxT_ENUM , &s_enumData##e , &wxTO_STRING( e ) , &wxFROM_STRING( e ) , &ToLong##e , &FromLong##e , typeid(e).name() ) ;

// ----------------------------------------------------------------------------
// Set Support
//
// in the header :
//
// enum wxFlavor
// {
//  Vanilla,
//  Chocolate,
//  Strawberry,
// };
//
// typedef wxBitset<wxFlavor> wxCoupe ;
//
// in the implementation file :
//
// wxBEGIN_ENUM( wxFlavor )
//  wxENUM_MEMBER( Vanilla )
//  wxENUM_MEMBER( Chocolate )
//  wxENUM_MEMBER( Strawberry )
// wxEND_ENUM( wxFlavor )
//
// wxIMPLEMENT_SET_STREAMING( wxCoupe , wxFlavor )
//
// implementation note : no partial specialization for streaming, but a delegation to a
// different class
//
// ----------------------------------------------------------------------------

// in order to remove dependancy on string tokenizer
void WXDLLIMPEXP_BASE wxSetStringToArray( const wxString &s , wxArrayString &array ) ;

template<typename e>
void wxSetFromString(const wxString &s , wxBitset<e> &data )
{
    wxEnumData* edata = wxGetEnumData((e) 0) ;
    data.reset() ;

    wxArrayString array ;
    wxSetStringToArray( s , array ) ;
    wxString flag;
    for ( int i = 0 ; i < array.Count() ; ++i )
    {
        flag = array[i] ;
        int ivalue ;
        if ( edata->HasEnumMemberValue( flag , &ivalue ) )
        {
            data.set( (e) ivalue ) ;
        }
    }
}

template<typename e>
void wxSetToString( wxString &s , const wxBitset<e> &data )
{
    wxEnumData* edata = wxGetEnumData((e) 0) ;
    int count = edata->GetEnumCount() ;
    int i ;
    s.Clear() ;
    for ( i = 0 ; i < count ; i++ )
    {
        e value = (e) edata->GetEnumMemberValueByIndex(i) ;
        if ( data.test( value ) )
        {
            // this could also be done by the templated calls
            if ( !s.empty() )
                s +=wxT("|") ;
            s += edata->GetEnumMemberNameByIndex(i) ;
        }
    }
}

#define wxIMPLEMENT_SET_STREAMING(SetName,e) \
    template<>  void wxStringReadValue(const wxString &s , wxBitset<e> &data ) \
{ \
    wxSetFromString( s , data ) ; \
} \
    template<>  void wxStringWriteValue( wxString &s , const wxBitset<e> &data ) \
{ \
    wxSetToString( s , data ) ; \
} \
    void FromLong##SetName( long data , wxxVariant& result ) { result = wxxVariant(SetName((unsigned long)data)) ;} \
    void ToLong##SetName( const wxxVariant& data , long &result ) { result = (long) data.wxTEMPLATED_MEMBER_CALL(Get , SetName).to_ulong() ;} \
    wxTO_STRING_IMP( SetName ) \
    wxFROM_STRING_IMP( SetName ) \
    wxEnumTypeInfo s_typeInfo##SetName(wxT_SET , &s_enumData##e , &wxTO_STRING( SetName ) , &wxFROM_STRING( SetName ) , &ToLong##SetName , &FromLong##SetName, typeid(SetName).name() ) ;  \
}

template<typename e>
void wxFlagsFromString(const wxString &s , e &data )
{
    wxEnumData* edata = wxGetEnumData((e*) 0) ;
    data.m_data = 0 ;

    wxArrayString array ;
    wxSetStringToArray( s , array ) ;
    wxString flag;
    for ( size_t i = 0 ; i < array.Count() ; ++i )
    {
        flag = array[i] ;
        int ivalue ;
        if ( edata->HasEnumMemberValue( flag , &ivalue ) )
        {
            data.m_data |= ivalue ;
        }
    }
}

template<typename e>
void wxFlagsToString( wxString &s , const e& data )
{
    wxEnumData* edata = wxGetEnumData((e*) 0) ;
    int count = edata->GetEnumCount() ;
    int i ;
    s.Clear() ;
    long dataValue = data.m_data ;
    for ( i = 0 ; i < count ; i++ )
    {
        int value = edata->GetEnumMemberValueByIndex(i) ;
        // make this to allow for multi-bit constants to work
        if ( value && ( dataValue & value ) == value )
        {
            // clear the flags we just set
            dataValue &= ~value ;
            // this could also be done by the templated calls
            if ( !s.empty() )
                s +=wxT("|") ;
            s += edata->GetEnumMemberNameByIndex(i) ;
        }
    }
}

#define wxBEGIN_FLAGS( e ) \
    wxEnumMemberData s_enumDataMembers##e[] = {

#define wxFLAGS_MEMBER( v ) { wxT(#v), v } ,

#define wxEND_FLAGS( e ) { NULL , 0 } } ; \
    wxEnumData s_enumData##e( s_enumDataMembers##e ) ; \
    wxEnumData *wxGetEnumData(e*) { return &s_enumData##e ; } \
    template<>  void wxStringReadValue(const wxString &s , e &data ) \
{ \
    wxFlagsFromString<e>( s , data ) ; \
} \
    template<>  void wxStringWriteValue( wxString &s , const e& data ) \
{ \
    wxFlagsToString<e>( s , data ) ; \
} \
    void FromLong##e( long data , wxxVariant& result ) { result = wxxVariant(e(data)) ;} \
    void ToLong##e( const wxxVariant& data , long &result ) { result = (long) data.wxTEMPLATED_MEMBER_CALL(Get , e).m_data ;} \
    wxTO_STRING_IMP( e ) \
    wxFROM_STRING_IMP( e ) \
    wxEnumTypeInfo s_typeInfo##e(wxT_SET , &s_enumData##e , &wxTO_STRING( e ) , &wxFROM_STRING( e ) , &ToLong##e , &FromLong##e, typeid(e).name()  ) ;
// ----------------------------------------------------------------------------
// Type Information
// ----------------------------------------------------------------------------
//
//
//  All data exposed by the RTTI is characterized using the following classes.
//  The first characterization is done by wxTypeKind. All enums up to and including
//  wxT_CUSTOM represent so called simple types. These cannot be divided any further.
//  They can be converted to and from wxStrings, that's all.


enum wxTypeKind
{
    wxT_VOID = 0, // unknown type
    wxT_BOOL,
    wxT_CHAR,
    wxT_UCHAR,
    wxT_INT,
    wxT_UINT,
    wxT_LONG,
    wxT_ULONG,
    wxT_FLOAT,
    wxT_DOUBLE,
    wxT_STRING, // must be wxString
    wxT_SET, // must be wxBitset<> template
    wxT_ENUM,
    wxT_CUSTOM, // user defined type (e.g. wxPoint)

    wxT_LAST_SIMPLE_TYPE_KIND = wxT_CUSTOM ,

    wxT_OBJECT_PTR, // object reference
    wxT_OBJECT , // embedded object
    wxT_COLLECTION , // collection

    wxT_DELEGATE , // for connecting against an event source

    wxT_LAST_TYPE_KIND = wxT_DELEGATE // sentinel for bad data, asserts, debugging
};

class WXDLLIMPEXP_BASE wxxVariant ;
class WXDLLIMPEXP_BASE wxTypeInfo ;

WX_DECLARE_STRING_HASH_MAP_WITH_DECL( wxTypeInfo* , wxTypeInfoMap , class WXDLLIMPEXP_BASE ) ;

class WXDLLIMPEXP_BASE wxTypeInfo
{
public :
    typedef void (*converterToString_t)( const wxxVariant& data , wxString &result ) ;
    typedef void (*converterFromString_t)( const wxString& data , wxxVariant &result ) ;

    wxTypeInfo(wxTypeKind kind,
               converterToString_t to = NULL, converterFromString_t from = NULL,
               const wxString &name = wxEmptyString):
            m_toString(to), m_fromString(from), m_kind(kind), m_name(name)
    {
        Register();
    }
#if wxUSE_UNICODE
    wxTypeInfo(wxTypeKind kind,
               converterToString_t to, converterFromString_t from,
               const char *name):
            m_toString(to), m_fromString(from), m_kind(kind), m_name(wxString::FromAscii(name))
    {
        Register();
    }
#endif

    virtual ~wxTypeInfo()
    {
        Unregister() ;
    }

    // return the kind of this type (wxT_... constants)
    wxTypeKind GetKind() const { return m_kind ; }

    // returns the unique name of this type
    const wxString& GetTypeName() const { return m_name ; }

    // is this type a delegate type
    bool IsDelegateType() const { return m_kind == wxT_DELEGATE ; }

    // is this type a custom type
    bool IsCustomType() const { return m_kind == wxT_CUSTOM ; }

    // is this type an object type
    bool IsObjectType() const { return m_kind == wxT_OBJECT || m_kind == wxT_OBJECT_PTR ; }

    // can the content of this type be converted to and from strings ?
    bool HasStringConverters() const { return m_toString != NULL && m_fromString != NULL ; }

    // convert a wxxVariant holding data of this type into a string
    void ConvertToString( const wxxVariant& data , wxString &result ) const

    { if ( m_toString ) (*m_toString)( data , result ) ; else wxLogError( _("String conversions not supported") ) ; }

    // convert a string into a wxxVariant holding the corresponding data in this type
    void ConvertFromString( const wxString& data , wxxVariant &result ) const
    { if( m_fromString ) (*m_fromString)( data , result ) ; else wxLogError( _("String conversions not supported") ) ; }

#if wxUSE_UNICODE
    static wxTypeInfo        *FindType(const char *typeName) { return FindType( wxString::FromAscii(typeName) ) ; }
#endif
    static wxTypeInfo        *FindType(const wxChar *typeName);

private :

    void Register();
    void Unregister();

    converterToString_t m_toString ;
    converterFromString_t m_fromString ;

    static wxTypeInfoMap*      ms_typeTable ;

    wxTypeKind m_kind;
    wxString m_name;
};

class WXDLLIMPEXP_BASE wxBuiltInTypeInfo : public wxTypeInfo
{
public :
    wxBuiltInTypeInfo( wxTypeKind kind , converterToString_t to = NULL , converterFromString_t from = NULL , const wxString &name = wxEmptyString ) :
       wxTypeInfo( kind , to , from , name )
       { wxASSERT_MSG( GetKind() < wxT_SET , wxT("Illegal Kind for Base Type") ) ; }
#if wxUSE_UNICODE
    wxBuiltInTypeInfo( wxTypeKind kind , converterToString_t to  , converterFromString_t from  , const char *name  ) :
       wxTypeInfo( kind , to , from , name )
       { wxASSERT_MSG( GetKind() < wxT_SET , wxT("Illegal Kind for Base Type") ) ; }
#endif
} ;

class WXDLLIMPEXP_BASE wxCustomTypeInfo : public wxTypeInfo
{
public :
    wxCustomTypeInfo( const wxString &name , converterToString_t to , converterFromString_t from ) :
       wxTypeInfo( wxT_CUSTOM , to , from , name )
       {}
#if wxUSE_UNICODE
    wxCustomTypeInfo( const char *name  , converterToString_t to , converterFromString_t from ) :
       wxTypeInfo( wxT_CUSTOM , to , from , name )
       {}
#endif
} ;

class WXDLLIMPEXP_BASE wxEnumTypeInfo : public wxTypeInfo
{
public :
    typedef void (*converterToLong_t)( const wxxVariant& data , long &result ) ;
    typedef void (*converterFromLong_t)( long data , wxxVariant &result ) ;

    wxEnumTypeInfo( wxTypeKind kind , wxEnumData* enumInfo , converterToString_t to , converterFromString_t from ,
        converterToLong_t toLong , converterFromLong_t fromLong , const wxString &name  ) :
    wxTypeInfo( kind , to , from , name ) , m_toLong( toLong ) , m_fromLong( fromLong )
    { wxASSERT_MSG( kind == wxT_ENUM || kind == wxT_SET , wxT("Illegal Kind for Enum Type")) ; m_enumInfo = enumInfo ;}

#if wxUSE_UNICODE
    wxEnumTypeInfo( wxTypeKind kind , wxEnumData* enumInfo , converterToString_t to , converterFromString_t from ,
        converterToLong_t toLong , converterFromLong_t fromLong , const char * name   ) :
    wxTypeInfo( kind , to , from , name ) , m_toLong( toLong ) , m_fromLong( fromLong )
    { wxASSERT_MSG( kind == wxT_ENUM || kind == wxT_SET , wxT("Illegal Kind for Enum Type")) ; m_enumInfo = enumInfo ;}
#endif
    const wxEnumData* GetEnumData() const { return m_enumInfo ; }

    // convert a wxxVariant holding data of this type into a long
    void ConvertToLong( const wxxVariant& data , long &result ) const

    { if( m_toLong ) (*m_toLong)( data , result ) ; else wxLogError( _("Long Conversions not supported") ) ; }

    // convert a long into a wxxVariant holding the corresponding data in this type
    void ConvertFromLong( long data , wxxVariant &result ) const
    { if( m_fromLong ) (*m_fromLong)( data , result ) ; else wxLogError( _("Long Conversions not supported") ) ;}

private :
    converterToLong_t m_toLong ;
    converterFromLong_t m_fromLong ;

    wxEnumData *m_enumInfo; // Kind == wxT_ENUM or Kind == wxT_SET
} ;

class WXDLLIMPEXP_BASE wxClassTypeInfo : public wxTypeInfo
{
public :
    wxClassTypeInfo( wxTypeKind kind , wxClassInfo* classInfo , converterToString_t to = NULL , converterFromString_t from = NULL , const wxString &name = wxEmptyString) ;
#if wxUSE_UNICODE
    wxClassTypeInfo( wxTypeKind kind , wxClassInfo* classInfo , converterToString_t to  , converterFromString_t from  , const char *name ) ;
#endif
    const wxClassInfo *GetClassInfo() const { return m_classInfo ; }
private :
    wxClassInfo *m_classInfo; // Kind == wxT_OBJECT - could be NULL
} ;

class WXDLLIMPEXP_BASE wxCollectionTypeInfo : public wxTypeInfo
{
public :
    wxCollectionTypeInfo( const wxString &elementName , converterToString_t to , converterFromString_t from  , const wxString &name) :
       wxTypeInfo( wxT_COLLECTION , to , from , name )
       { m_elementTypeName = elementName ; m_elementType = NULL ;}
#if wxUSE_UNICODE
    wxCollectionTypeInfo( const char *elementName , converterToString_t to , converterFromString_t from  , const char *name ) :
       wxTypeInfo( wxT_COLLECTION , to , from , name )
       { m_elementTypeName = wxString::FromAscii( elementName ) ; m_elementType = NULL ;}
#endif
       const wxTypeInfo* GetElementType() const
       {
           if ( m_elementType == NULL )
               m_elementType = wxTypeInfo::FindType( m_elementTypeName ) ;
           return m_elementType ; }
private :
    mutable wxTypeInfo * m_elementType ;
    wxString    m_elementTypeName ;
} ;

// a delegate is an exposed event source

class WXDLLIMPEXP_BASE wxDelegateTypeInfo : public wxTypeInfo
{
public :
    wxDelegateTypeInfo( int eventType , wxClassInfo* eventClass , converterToString_t to = NULL , converterFromString_t from = NULL ) ;
    wxDelegateTypeInfo( int eventType , int lastEventType, wxClassInfo* eventClass , converterToString_t to = NULL , converterFromString_t from = NULL ) ;
    int GetEventType() const { return m_eventType ; }
    int GetLastEventType() const { return m_lastEventType ; }
    const wxClassInfo* GetEventClass() const { return m_eventClass ; }
private :
    const wxClassInfo *m_eventClass; // (extended will merge into classinfo)
    int m_eventType ;
    int m_lastEventType ;
} ;

template<typename T> const wxTypeInfo* wxGetTypeInfo( T * ) { return wxTypeInfo::FindType(typeid(T).name()) ; }

// this macro is for usage with custom, non-object derived classes and structs, wxPoint is such a custom type

#if wxUSE_FUNC_TEMPLATE_POINTER
#define wxCUSTOM_TYPE_INFO( e , toString , fromString ) \
    wxCustomTypeInfo s_typeInfo##e(typeid(e).name() , &toString , &fromString) ;
#else
#define wxCUSTOM_TYPE_INFO( e , toString , fromString ) \
    void ToString##e( const wxxVariant& data , wxString &result ) { toString(data, result); } \
    void FromString##e( const wxString& data , wxxVariant &result ) { fromString(data, result); } \
    wxCustomTypeInfo s_typeInfo##e(typeid(e).name() , &ToString##e , &FromString##e) ;
#endif

#define wxCOLLECTION_TYPE_INFO( element , collection ) \
    wxCollectionTypeInfo s_typeInfo##collection( typeid(element).name() , NULL , NULL , typeid(collection).name() ) ;

// sometimes a compiler invents specializations that are nowhere called, use this macro to satisfy the refs, currently
// we don't have to play tricks, but if we will have to according to the compiler, we will use that macro for that

#define wxILLEGAL_TYPE_SPECIALIZATION( a )

// ----------------------------------------------------------------------------
// wxxVariant as typesafe data holder
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxxVariantData
{
public:
    virtual ~wxxVariantData() {}

    // return a heap allocated duplicate
    virtual wxxVariantData* Clone() const = 0 ;

    // returns the type info of the contentc
    virtual const wxTypeInfo* GetTypeInfo() const = 0 ;
} ;

template<typename T> class wxxVariantDataT : public wxxVariantData
{
public:
    wxxVariantDataT(const T& d) : m_data(d) {}
    virtual ~wxxVariantDataT() {}

    // get a ref to the stored data
    T & Get() { return m_data; }

    // get a const ref to the stored data
    const T & Get() const { return m_data; }

    // set the data
    void Set(const T& d) { m_data =  d; }

    // return a heap allocated duplicate
    virtual wxxVariantData* Clone() const { return new wxxVariantDataT<T>( Get() ) ; }

    // returns the type info of the contentc
    virtual const wxTypeInfo* GetTypeInfo() const { return wxGetTypeInfo( (T*) NULL ) ; }

private:
    T m_data;
};

class WXDLLIMPEXP_BASE wxxVariant
{
public :
    wxxVariant() { m_data = NULL ; }
    wxxVariant( wxxVariantData* data , const wxString& name = wxEmptyString ) : m_data(data) , m_name(name) {}
    wxxVariant( const wxxVariant &d ) { if ( d.m_data ) m_data = d.m_data->Clone() ; else m_data = NULL ; m_name = d.m_name ; }

    template<typename T> wxxVariant( const T& data , const wxString& name = wxEmptyString ) :
    m_data(new wxxVariantDataT<T>(data) ), m_name(name) {}

    ~wxxVariant() { delete m_data ; }

    // get a ref to the stored data
    template<typename T> T& Get(wxTEMPLATED_MEMBER_FIX(T))
    {
        wxxVariantDataT<T> *dataptr = dynamic_cast<wxxVariantDataT<T>*> (m_data) ;
        wxASSERT_MSG( dataptr , wxString::Format(wxT("Cast to %s not possible"), typeid(T).name()) ) ;
        return dataptr->Get() ;
    }

    // get a ref to the stored data
    template<typename T> const T& Get(wxTEMPLATED_MEMBER_FIX(T)) const
    {
        const wxxVariantDataT<T> *dataptr = dynamic_cast<const wxxVariantDataT<T>*> (m_data) ;
        wxASSERT_MSG( dataptr , wxString::Format(wxT("Cast to %s not possible"), typeid(T).name()) ) ;
        return dataptr->Get() ;
    }

    bool IsEmpty() const { return m_data == NULL ; }

    template<typename T> bool HasData(wxTEMPLATED_MEMBER_FIX(T)) const
    {
        const wxxVariantDataT<T> *dataptr = dynamic_cast<const wxxVariantDataT<T>*> (m_data) ;
        return dataptr != NULL ;
    }

    // stores the data
    template<typename T> void Set(const T& data) const
    {
        delete m_data ;
        m_data = new wxxVariantDataT<T>(data) ;
    }

    wxxVariant& operator=(const wxxVariant &d)
    {
        delete m_data;
        m_data = d.m_data ? d.m_data->Clone() : NULL ;
        m_name = d.m_name ;
        return *this ;
    }

    // gets the stored data casted to a wxObject* , returning NULL if cast is not possible
    wxObject* GetAsObject() ;

    // get the typeinfo of the stored object
    const wxTypeInfo* GetTypeInfo() const { return m_data->GetTypeInfo() ; }

    // returns this value as string
    wxString GetAsString() const
    {
        wxString s ;
        GetTypeInfo()->ConvertToString( *this , s ) ;
        return s ;
    }
    const wxString& GetName() const { return m_name ; }
private :
    wxxVariantData* m_data ;
    wxString m_name ;
} ;

#include "wx/dynarray.h"

WX_DECLARE_OBJARRAY_WITH_DECL(wxxVariant, wxxVariantArray, class WXDLLIMPEXP_BASE);

// templated streaming, every type must have their specialization for these methods

template<typename T>
void wxStringReadValue( const wxString &s , T &data );

template<typename T>
void wxStringWriteValue( wxString &s , const T &data);

template<typename T>
void wxToStringConverter( const wxxVariant &v, wxString &s wxTEMPLATED_FUNCTION_FIX(T)) { wxStringWriteValue( s , v.wxTEMPLATED_MEMBER_CALL(Get , T) ) ; }

template<typename T>
void wxFromStringConverter( const wxString &s, wxxVariant &v wxTEMPLATED_FUNCTION_FIX(T)) { T d ; wxStringReadValue( s , d ) ; v = wxxVariant(d) ; }

// ----------------------------------------------------------------------------
// Property Support
//
// wxPropertyInfo is used to inquire of the property by name.  It doesn't
// provide access to the property, only information about it.  If you
// want access, look at wxPropertyAccessor.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxSetter
{
public:
    wxSetter( const wxString name ) { m_name = name ; }
    virtual ~wxSetter() {}
    virtual void Set( wxObject *object, const wxxVariant &variantValue ) const = 0;
    const wxString& GetName() const { return m_name ; }
private:
    wxString m_name;
};

class WXDLLIMPEXP_BASE wxGetter
{
public:
    wxGetter( const wxString name ) { m_name = name ; }
    virtual ~wxGetter() {}
    virtual void Get( const wxObject *object , wxxVariant& result) const = 0;
    const wxString& GetName() const { return m_name ; }
private:
    wxString m_name;
};

class WXDLLIMPEXP_BASE wxCollectionGetter
{
public :
    wxCollectionGetter( const wxString name ) { m_name = name ; }
    virtual ~wxCollectionGetter() {}
    virtual void Get( const wxObject *object , wxxVariantArray& result) const = 0;
    const wxString& GetName() const { return m_name ; }
private :
    wxString m_name ;
} ;

template<typename coll_t> void WXDLLIMPEXP_BASE wxCollectionToVariantArray( const coll_t& coll , wxxVariantArray& result ) ;

class WXDLLIMPEXP_BASE wxAdder
{
public :
    wxAdder( const wxString name ) { m_name = name ; }
    virtual ~wxAdder() {}
    virtual void Add( wxObject *object, const wxxVariant &variantValue ) const= 0;
    const wxString& GetName() const { return m_name ; }
private :
    wxString m_name ;
} ;


#define wxSETTER( property, Klass, valueType, setterMethod ) \
class wxSetter##property : public wxSetter \
{ \
public: \
    wxINFUNC_CLASS_TYPE_FIX(Klass) \
    wxSetter##property() : wxSetter( wxT(#setterMethod) ) {} \
    virtual ~wxSetter##property() {} \
    void Set( wxObject *object, const wxxVariant &variantValue ) const \
{ \
    Klass *obj = dynamic_cast<Klass*>(object) ;  \
    if ( variantValue.wxTEMPLATED_MEMBER_CALL(HasData, valueType) ) \
    obj->setterMethod(variantValue.wxTEMPLATED_MEMBER_CALL(Get , valueType)) ; \
            else \
            obj->setterMethod(*variantValue.wxTEMPLATED_MEMBER_CALL(Get , valueType*)) ; \
} \
} ;

#define wxGETTER( property, Klass, valueType , gettermethod ) \
class wxGetter##property : public wxGetter \
{ \
public : \
    wxINFUNC_CLASS_TYPE_FIX(Klass) \
    wxGetter##property() : wxGetter( wxT(#gettermethod) ) {} \
    virtual ~wxGetter##property() {} \
    void Get( const wxObject *object , wxxVariant &result) const \
{ \
    const Klass *obj = dynamic_cast<const Klass*>(object) ;  \
    result = wxxVariant( obj->gettermethod() ) ; \
} \
} ;

#define wxADDER( property, Klass, valueType , addermethod ) \
class wxAdder##property : public wxAdder \
{ \
public: \
    wxINFUNC_CLASS_TYPE_FIX(Klass) \
    wxAdder##property() : wxAdder( wxT(#addermethod) ) {} \
    virtual ~wxAdder##property() {} \
    void Add( wxObject *object, const wxxVariant &variantValue ) const \
{ \
    Klass *obj = dynamic_cast<Klass*>(object) ;  \
    if ( variantValue.wxTEMPLATED_MEMBER_CALL(HasData, valueType) ) \
    obj->addermethod(variantValue.wxTEMPLATED_MEMBER_CALL(Get , valueType)) ; \
            else \
            obj->addermethod(*variantValue.wxTEMPLATED_MEMBER_CALL(Get , valueType*)) ; \
} \
} ;

#define wxCOLLECTION_GETTER( property, Klass, valueType , gettermethod ) \
class wxCollectionGetter##property : public wxCollectionGetter \
{ \
public : \
    wxINFUNC_CLASS_TYPE_FIX(Klass) \
    wxCollectionGetter##property() : wxCollectionGetter( wxT(#gettermethod) ) {} \
    virtual ~wxCollectionGetter##property() {} \
    void Get( const wxObject *object , wxxVariantArray &result) const \
{ \
    const Klass *obj = dynamic_cast<const Klass*>(object) ;  \
    wxCollectionToVariantArray( obj->gettermethod() , result ) ; \
} \
} ;

class WXDLLIMPEXP_BASE wxPropertyAccessor
{
public :
    wxPropertyAccessor( wxSetter *setter , wxGetter *getter , wxAdder *adder , wxCollectionGetter *collectionGetter )
    { m_setter = setter ; m_getter = getter ; m_adder = adder ; m_collectionGetter = collectionGetter ;}

    virtual ~wxPropertyAccessor() {}

    // Setting a simple property (non-collection)
    virtual void SetProperty(wxObject *object, const wxxVariant &value) const
    { if ( m_setter ) m_setter->Set( object , value ) ; else wxLogError( _("SetProperty called w/o valid setter") ) ;}

    // Getting a simple property (non-collection)
    virtual void GetProperty(const wxObject *object, wxxVariant &result) const
    { if ( m_getter ) m_getter->Get( object , result ) ; else wxLogError( _("GetProperty called w/o valid getter") ) ;}

    // Adding an element to a collection property
    virtual void AddToPropertyCollection(wxObject *object, const wxxVariant &value) const
    { if ( m_adder ) m_adder->Add( object , value ) ; else wxLogError( _("AddToPropertyCollection called w/o valid adder") ) ;}

    // Getting a collection property
    virtual void GetPropertyCollection( const wxObject *obj, wxxVariantArray &result) const
    { if ( m_collectionGetter ) m_collectionGetter->Get( obj , result) ; else wxLogError( _("GetPropertyCollection called w/o valid collection getter") ) ;}

    virtual bool HasSetter() const { return m_setter != NULL ; }
    virtual bool HasCollectionGetter() const { return m_collectionGetter != NULL ; }
    virtual bool HasGetter() const { return m_getter != NULL ; }
    virtual bool HasAdder() const { return m_adder != NULL ; }

    virtual const wxString& GetCollectionGetterName() const
    { return m_collectionGetter->GetName() ; }
    virtual const wxString&  GetGetterName() const
    { return m_getter->GetName() ; }
    virtual const wxString& GetSetterName() const
    { return m_setter->GetName() ; }
    virtual const wxString& GetAdderName() const
    { return m_adder->GetName() ; }

protected :
    wxSetter *m_setter ;
    wxAdder *m_adder ;
    wxGetter *m_getter ;
    wxCollectionGetter* m_collectionGetter ;
};

class WXDLLIMPEXP_BASE wxGenericPropertyAccessor : public wxPropertyAccessor
{
public :
    wxGenericPropertyAccessor( const wxString &propName ) ;
    virtual ~wxGenericPropertyAccessor() ;

    void RenameProperty( const wxString& WXUNUSED_UNLESS_DEBUG(oldName),
        const wxString& newName )
    {
        wxASSERT( oldName == m_propertyName ) ; m_propertyName = newName ;
    }
    virtual bool HasSetter() const { return true ; }
    virtual bool HasGetter() const { return true ; }
    virtual bool HasAdder() const { return false ; }
    virtual bool HasCollectionGetter() const { return false ; }

    virtual const wxString&  GetGetterName() const
    { return m_getterName ; }
    virtual const wxString& GetSetterName() const
    { return m_setterName ; }

    virtual void SetProperty(wxObject *object, const wxxVariant &value) const ;
    virtual void GetProperty(const wxObject *object, wxxVariant &value) const ;

    // Adding an element to a collection property
    virtual void AddToPropertyCollection(wxObject *WXUNUSED(object), const wxxVariant &WXUNUSED(value)) const
    { wxLogError( _("AddToPropertyCollection called on a generic accessor") ) ;}

    // Getting a collection property
    virtual void GetPropertyCollection( const wxObject *WXUNUSED(obj), wxxVariantArray &WXUNUSED(result)) const
    { wxLogError ( _("GetPropertyCollection called on a generic accessor") ) ;}
private :
    struct wxGenericPropertyAccessorInternal ;
    wxGenericPropertyAccessorInternal* m_data ;
    wxString m_propertyName ;
    wxString m_setterName ;
    wxString m_getterName ;
} ;

typedef long wxPropertyInfoFlags ;
enum {
    // will be removed in future releases
    wxPROP_DEPRECATED       = 0x00000001 ,
    // object graph property, will be streamed with priority (after constructor properties)
    wxPROP_OBJECT_GRAPH     = 0x00000002 ,
    // this will only be streamed out and in as enum/set, the internal representation is still a long
    wxPROP_ENUM_STORE_LONG  = 0x00000004 ,
    // don't stream out this property, needed eg to avoid streaming out children that are always created by their parents
    wxPROP_DONT_STREAM = 0x00000008 ,
}  ;

class WXDLLIMPEXP_BASE wxPropertyInfo
{
    friend class WXDLLIMPEXP_BASE wxDynamicClassInfo ;
public :
    wxPropertyInfo(wxPropertyInfo* &iter,
                   wxClassInfo* itsClass,
                   const wxString& name,
                   const wxString& typeName,
                   wxPropertyAccessor *accessor,
                   wxxVariant dv,
                   wxPropertyInfoFlags flags = 0,
                   const wxString& helpString = wxEmptyString,
                   const wxString& groupString = wxEmptyString) :
                   m_itsClass(itsClass),
           m_name(name),
           m_typeInfo(NULL),
           m_typeName(typeName) ,
           m_collectionElementTypeInfo(NULL),
           m_accessor(accessor),
           m_defaultValue(dv),
           m_flags(flags),
           m_helpString(helpString),
           m_groupString(groupString)
       {
           Insert(iter);
       }

#if wxUSE_UNICODE
    wxPropertyInfo(wxPropertyInfo* &iter,
                   wxClassInfo* itsClass,
                   const wxString& name,
                   const char* typeName,
                   wxPropertyAccessor *accessor,
                   wxxVariant dv,
                   wxPropertyInfoFlags flags = 0,
                   const wxString& helpString = wxEmptyString,
                   const wxString& groupString = wxEmptyString) :
                   m_itsClass(itsClass),
           m_name(name),
           m_typeInfo(NULL),
           m_typeName(wxString::FromAscii(typeName)) ,
           m_collectionElementTypeInfo(NULL),
           m_accessor(accessor),
           m_defaultValue(dv),
           m_flags(flags),
           m_helpString(helpString),
           m_groupString(groupString)
       {
           Insert(iter);
       }
#endif
    wxPropertyInfo(wxPropertyInfo* &iter,
                   wxClassInfo* itsClass,
                   const wxString& name,
                   wxDelegateTypeInfo* type,
                   wxPropertyAccessor *accessor,
                   wxxVariant dv,
                   wxPropertyInfoFlags flags = 0,
                   const wxString& helpString = wxEmptyString,
                   const wxString& groupString = wxEmptyString) :
           m_itsClass(itsClass),
           m_name(name),
           m_typeInfo(type),
           m_collectionElementTypeInfo(NULL),
           m_accessor(accessor),
           m_defaultValue(dv),
           m_flags(flags),
           m_helpString(helpString),
           m_groupString(groupString)
       {
           Insert(iter);
       }

       wxPropertyInfo(wxPropertyInfo* &iter,
                      wxClassInfo* itsClass, const wxString& name,
                      const wxString& collectionTypeName,
                      const wxString& elementTypeName,
                      wxPropertyAccessor *accessor,
                      wxPropertyInfoFlags flags = 0,
                      const wxString& helpString = wxEmptyString,
                      const wxString& groupString = wxEmptyString) :
           m_itsClass(itsClass),
           m_name(name),
           m_typeInfo(NULL),
           m_typeName(collectionTypeName) ,
           m_collectionElementTypeInfo(NULL),
           m_collectionElementTypeName(elementTypeName),
           m_accessor(accessor) ,
           m_flags(flags),
           m_helpString(helpString),
           m_groupString(groupString)
       {
           Insert(iter);
       }

#if wxUSE_UNICODE
              wxPropertyInfo(wxPropertyInfo* &iter,
                      wxClassInfo* itsClass, const wxString& name,
                      const char* collectionTypeName,
                      const char* elementTypeName,
                      wxPropertyAccessor *accessor,
                      wxPropertyInfoFlags flags = 0,
                      const wxString& helpString = wxEmptyString,
                      const wxString& groupString = wxEmptyString) :
           m_itsClass(itsClass),
           m_name(name),
           m_typeInfo(NULL),
           m_typeName(wxString::FromAscii(collectionTypeName)) ,
           m_collectionElementTypeInfo(NULL),
           m_collectionElementTypeName(wxString::FromAscii(elementTypeName)),
           m_accessor(accessor) ,
           m_flags(flags),
           m_helpString(helpString),
           m_groupString(groupString)
       {
           Insert(iter);
       }
#endif
       ~wxPropertyInfo() ;

       // return the class this property is declared in
       const wxClassInfo*  GetDeclaringClass() const { return m_itsClass ; }

       // return the name of this property
       const wxString&     GetName() const { return m_name ; }

       // returns the flags of this property
       wxPropertyInfoFlags GetFlags() const { return m_flags ;}

       // returns the short help string of this property
       const wxString&     GetHelpString() const { return m_helpString ; }

       // returns the group string of this property
       const wxString&     GetGroupString() const { return m_groupString ; }

       // return the element type info of this property (for collections, otherwise NULL)
       const wxTypeInfo *  GetCollectionElementTypeInfo() const
       {
           if ( m_collectionElementTypeInfo == NULL )
               m_collectionElementTypeInfo = wxTypeInfo::FindType(m_collectionElementTypeName) ;
           return m_collectionElementTypeInfo ;
       }

       // return the type info of this property
       const wxTypeInfo *  GetTypeInfo() const
       {
           if ( m_typeInfo == NULL )
               m_typeInfo = wxTypeInfo::FindType(m_typeName) ;
           return m_typeInfo ;
       }

       // return the accessor for this property
       wxPropertyAccessor* GetAccessor() const { return m_accessor ; }

       // returns NULL if this is the last property of this class
       wxPropertyInfo*     GetNext() const { return m_next ; }

       // returns the default value of this property, its kind may be wxT_VOID if it is not valid
       wxxVariant          GetDefaultValue() const { return m_defaultValue ; }
private :
    void Insert(wxPropertyInfo* &iter)
    {
        m_next = NULL ;
        if ( iter == NULL )
            iter = this ;
        else
        {
            wxPropertyInfo* i = iter ;
            while( i->m_next )
                i = i->m_next ;

            i->m_next = this ;
        }
    }

    wxClassInfo*        m_itsClass ;
    wxString            m_name ;
    mutable wxTypeInfo*         m_typeInfo ;
    wxString            m_typeName ;
    mutable wxTypeInfo*         m_collectionElementTypeInfo ;
    wxString            m_collectionElementTypeName ;
    wxPropertyAccessor* m_accessor ;
    wxxVariant          m_defaultValue;
    wxPropertyInfoFlags m_flags ;
    wxString            m_helpString ;
    wxString            m_groupString ;
    // string representation of the default value
    //  to be assigned by the designer to the property
    //  when the component is dropped on the container.
    wxPropertyInfo*     m_next ;
};

WX_DECLARE_STRING_HASH_MAP_WITH_DECL( wxPropertyInfo* , wxPropertyInfoMap , class WXDLLIMPEXP_BASE ) ;

#define wxBEGIN_PROPERTIES_TABLE(theClass) \
    wxPropertyInfo *theClass::GetPropertiesStatic()  \
{  \
    typedef theClass class_t; \
    static wxPropertyInfo* first = NULL ;

#define wxEND_PROPERTIES_TABLE() \
    return first ; }

#define wxHIDE_PROPERTY( pname ) \
    static wxPropertyInfo _propertyInfo##pname( first , class_t::GetClassInfoStatic() , wxT(#pname) , typeid(void).name() ,NULL , wxxVariant() , wxPROP_DONT_STREAM , wxEmptyString , wxEmptyString ) ;

#define wxPROPERTY( pname , type , setter , getter , defaultValue , flags , help , group) \
    wxSETTER( pname , class_t , type , setter ) \
    static wxSetter##pname _setter##pname ; \
    wxGETTER( pname , class_t , type , getter ) \
    static wxGetter##pname _getter##pname ; \
    static wxPropertyAccessor _accessor##pname( &_setter##pname , &_getter##pname , NULL , NULL ) ; \
    static wxPropertyInfo _propertyInfo##pname( first , class_t::GetClassInfoStatic() , wxT(#pname) , typeid(type).name() ,&_accessor##pname , wxxVariant(defaultValue) , flags , group , help ) ;

#define wxPROPERTY_FLAGS( pname , flags , type , setter , getter ,defaultValue , pflags , help , group) \
    wxSETTER( pname , class_t , type , setter ) \
    static wxSetter##pname _setter##pname ; \
    wxGETTER( pname , class_t , type , getter ) \
    static wxGetter##pname _getter##pname ; \
    static wxPropertyAccessor _accessor##pname( &_setter##pname , &_getter##pname , NULL , NULL ) ; \
    static wxPropertyInfo _propertyInfo##pname( first , class_t::GetClassInfoStatic() , wxT(#pname) , typeid(flags).name() ,&_accessor##pname , wxxVariant(defaultValue), wxPROP_ENUM_STORE_LONG | pflags , help , group ) ;

#define wxREADONLY_PROPERTY( pname , type , getter ,defaultValue , flags , help , group) \
    wxGETTER( pname , class_t , type , getter ) \
    static wxGetter##pname _getter##pname ; \
    static wxPropertyAccessor _accessor##pname( NULL , &_getter##pname , NULL , NULL ) ; \
    static wxPropertyInfo _propertyInfo##pname( first , class_t::GetClassInfoStatic() , wxT(#pname) , typeid(type).name() ,&_accessor##pname , wxxVariant(defaultValue), flags , help , group ) ;

#define wxREADONLY_PROPERTY_FLAGS( pname , flags , type , getter ,defaultValue , pflags , help , group) \
    wxGETTER( pname , class_t , type , getter ) \
    static wxGetter##pname _getter##pname ; \
    static wxPropertyAccessor _accessor##pname( NULL , &_getter##pname , NULL , NULL ) ; \
    static wxPropertyInfo _propertyInfo##pname( first , class_t::GetClassInfoStatic() , wxT(#pname) , typeid(flags).name() ,&_accessor##pname , wxxVariant(defaultValue), wxPROP_ENUM_STORE_LONG | pflags , help , group ) ;

#define wxPROPERTY_COLLECTION( pname , colltype , addelemtype , adder , getter , flags , help , group ) \
    wxADDER( pname , class_t , addelemtype , adder ) \
    static wxAdder##pname _adder##pname ; \
    wxCOLLECTION_GETTER( pname , class_t , colltype , getter ) \
    static wxCollectionGetter##pname _collectionGetter##pname ; \
    static wxPropertyAccessor _accessor##pname( NULL , NULL ,&_adder##pname , &_collectionGetter##pname ) ; \
    static wxPropertyInfo _propertyInfo##pname( first , class_t::GetClassInfoStatic() , wxT(#pname) , typeid(colltype).name() ,typeid(addelemtype).name() ,&_accessor##pname , flags , help , group ) ;

#define wxREADONLY_PROPERTY_COLLECTION( pname , colltype , addelemtype , getter , flags , help , group) \
    wxCOLLECTION_GETTER( pname , class_t , colltype , getter ) \
    static wxCollectionGetter##pname _collectionGetter##pname ; \
    static wxPropertyAccessor _accessor##pname( NULL , NULL , NULL , &_collectionGetter##pname ) ; \
    static wxPropertyInfo _propertyInfo##pname( first ,class_t::GetClassInfoStatic() ,  wxT(#pname) , typeid(colltype).name() ,typeid(addelemtype).name() ,&_accessor##pname , flags , help , group  ) ;


#define wxEVENT_PROPERTY( name , eventType , eventClass ) \
    static wxDelegateTypeInfo _typeInfo##name( eventType , CLASSINFO( eventClass ) ) ; \
    static wxPropertyInfo _propertyInfo##name( first ,class_t::GetClassInfoStatic() , wxT(#name) , &_typeInfo##name , NULL , wxxVariant() ) ; \

#define wxEVENT_RANGE_PROPERTY( name , eventType , lastEventType , eventClass ) \
    static wxDelegateTypeInfo _typeInfo##name( eventType , lastEventType , CLASSINFO( eventClass ) ) ; \
    static wxPropertyInfo _propertyInfo##name( first , class_t::GetClassInfoStatic() , wxT(#name) , &_typeInfo##name , NULL , wxxVariant() ) ; \

// ----------------------------------------------------------------------------
// Implementation Helper for Simple Properties
// ----------------------------------------------------------------------------

#define wxIMPLEMENT_PROPERTY(name, type) \
private:\
    type m_##name; \
public: \
  void  Set##name( type const & p) { m_##name = p; } \
  type const & Get##name() const  { return m_##name; }

// ----------------------------------------------------------------------------
// Handler Info
//
// this is describing an event sink
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxHandlerInfo
{
    friend class WXDLLIMPEXP_BASE wxDynamicClassInfo ;
public :
    wxHandlerInfo(wxHandlerInfo* &iter,
                   wxClassInfo* itsClass,
                  const wxString& name,
                  wxObjectEventFunction address,
                  const wxClassInfo* eventClassInfo) :
            m_eventFunction(address),
            m_name(name),
            m_eventClassInfo(eventClassInfo) ,
            m_itsClass(itsClass)
       {
           m_next = NULL ;
           if ( iter == NULL )
               iter = this ;
           else
           {
               wxHandlerInfo* i = iter ;
               while( i->m_next )
                   i = i->m_next ;

               i->m_next = this ;
           }
       }

       ~wxHandlerInfo() ;

       // return the name of this handler
       const wxString& GetName() const { return m_name ; }

       // return the class info of the event
       const wxClassInfo *GetEventClassInfo() const { return m_eventClassInfo ; }

       // get the handler function pointer
       wxObjectEventFunction GetEventFunction() const { return m_eventFunction ; }

       // returns NULL if this is the last handler of this class
       wxHandlerInfo*     GetNext() const { return m_next ; }

       // return the class this property is declared in
       const wxClassInfo*   GetDeclaringClass() const { return m_itsClass ; }

private :
    wxObjectEventFunction m_eventFunction ;
    wxString            m_name;
    const wxClassInfo*  m_eventClassInfo ;
    wxHandlerInfo*      m_next ;
    wxClassInfo*        m_itsClass ;
};

#define wxHANDLER(name,eventClassType) \
    static wxHandlerInfo _handlerInfo##name( first , class_t::GetClassInfoStatic() , wxT(#name) , (wxObjectEventFunction) (wxEventFunction) &name , CLASSINFO( eventClassType ) ) ;

#define wxBEGIN_HANDLERS_TABLE(theClass) \
    wxHandlerInfo *theClass::GetHandlersStatic()  \
{  \
    typedef theClass class_t; \
    static wxHandlerInfo* first = NULL ;

#define wxEND_HANDLERS_TABLE() \
    return first ; }

// ----------------------------------------------------------------------------
// Constructor Bridges
//
// allow to set up constructors with params during runtime
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxConstructorBridge
{
public :
    virtual void Create(wxObject * &o, wxxVariant *args) = 0;
};

// a direct constructor bridge calls the operator new for this class and
// passes all params to the constructor. needed for classes that cannot be
// instantiated using alloc-create semantics
class WXDLLIMPEXP_BASE wxDirectConstructorBrigde : public wxConstructorBridge
{
public :
    virtual void Create(wxObject * &o, wxxVariant *args) = 0;
} ;

// Creator Bridges for all Numbers of Params

// no params

template<typename Class>
struct wxConstructorBridge_0 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create();
    }
};

struct wxConstructorBridge_Dummy : public wxConstructorBridge
{
    void Create(wxObject *&, wxxVariant *)
    {
    }
} ;

#define wxCONSTRUCTOR_0(klass) \
    wxConstructorBridge_0<klass> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { NULL } ; \
    const int klass::ms_constructorPropertiesCount = 0 ;

#define wxCONSTRUCTOR_DUMMY(klass) \
    wxConstructorBridge_Dummy constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { NULL } ; \
    const int klass::ms_constructorPropertiesCount = 0 ;

// 1 param

template<typename Class, typename T0>
struct wxConstructorBridge_1 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0)
            );
    }
};

#define wxCONSTRUCTOR_1(klass,t0,v0) \
    wxConstructorBridge_1<klass,t0> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0) } ; \
    const int klass::ms_constructorPropertiesCount = 1 ;

// 2 params

template<typename Class,
typename T0, typename T1>
struct wxConstructorBridge_2 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1)
            );
    }
};

#define wxCONSTRUCTOR_2(klass,t0,v0,t1,v1) \
    wxConstructorBridge_2<klass,t0,t1> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  } ; \
    const int klass::ms_constructorPropertiesCount = 2;

// direct constructor version

template<typename Class,
typename T0, typename T1>
struct wxDirectConstructorBridge_2 : public wxDirectConstructorBrigde
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        o = new Class(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1)
            );
    }
};

#define wxDIRECT_CONSTRUCTOR_2(klass,t0,v0,t1,v1) \
    wxDirectConstructorBridge_2<klass,t0,t1> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  } ; \
    const int klass::ms_constructorPropertiesCount = 2;


// 3 params

template<typename Class,
typename T0, typename T1, typename T2>
struct wxConstructorBridge_3 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2)
            );
    }
};

#define wxCONSTRUCTOR_3(klass,t0,v0,t1,v1,t2,v2) \
    wxConstructorBridge_3<klass,t0,t1,t2> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  , wxT(#v2)  } ; \
    const int klass::ms_constructorPropertiesCount = 3 ;

// direct constructor version

template<typename Class,
typename T0, typename T1, typename T2>
struct wxDirectConstructorBridge_3 : public wxDirectConstructorBrigde
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        o = new Class(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2)
            );
    }
};

#define wxDIRECT_CONSTRUCTOR_3(klass,t0,v0,t1,v1,t2,v2) \
    wxDirectConstructorBridge_3<klass,t0,t1,t2> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1) , wxT(#v2) } ; \
    const int klass::ms_constructorPropertiesCount = 3;

// 4 params

template<typename Class,
typename T0, typename T1, typename T2, typename T3>
struct wxConstructorBridge_4 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2) ,
            args[3].wxTEMPLATED_MEMBER_CALL(Get , T3)
            );
    }
};

#define wxCONSTRUCTOR_4(klass,t0,v0,t1,v1,t2,v2,t3,v3) \
    wxConstructorBridge_4<klass,t0,t1,t2,t3> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  , wxT(#v2)  , wxT(#v3)  } ; \
    const int klass::ms_constructorPropertiesCount = 4 ;

// 5 params

template<typename Class,
typename T0, typename T1, typename T2, typename T3, typename T4>
struct wxConstructorBridge_5 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2) ,
            args[3].wxTEMPLATED_MEMBER_CALL(Get , T3) ,
            args[4].wxTEMPLATED_MEMBER_CALL(Get , T4)
            );
    }
};

#define wxCONSTRUCTOR_5(klass,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4) \
    wxConstructorBridge_5<klass,t0,t1,t2,t3,t4> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  , wxT(#v2)  , wxT(#v3)  , wxT(#v4)  } ; \
    const int klass::ms_constructorPropertiesCount = 5;

// 6 params

template<typename Class,
typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
struct wxConstructorBridge_6 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2) ,
            args[3].wxTEMPLATED_MEMBER_CALL(Get , T3) ,
            args[4].wxTEMPLATED_MEMBER_CALL(Get , T4) ,
            args[5].wxTEMPLATED_MEMBER_CALL(Get , T5)
            );
    }
};

#define wxCONSTRUCTOR_6(klass,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5) \
    wxConstructorBridge_6<klass,t0,t1,t2,t3,t4,t5> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  , wxT(#v2)  , wxT(#v3)  , wxT(#v4)  , wxT(#v5)  } ; \
    const int klass::ms_constructorPropertiesCount = 6;

// direct constructor version

template<typename Class,
typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
struct wxDirectConstructorBridge_6 : public wxDirectConstructorBrigde
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        o = new Class(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2) ,
            args[3].wxTEMPLATED_MEMBER_CALL(Get , T3) ,
            args[4].wxTEMPLATED_MEMBER_CALL(Get , T4) ,
            args[5].wxTEMPLATED_MEMBER_CALL(Get , T5)
            );
    }
};

#define wxDIRECT_CONSTRUCTOR_6(klass,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5) \
    wxDirectConstructorBridge_6<klass,t0,t1,t2,t3,t4,t5> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  , wxT(#v2)  , wxT(#v3)  , wxT(#v4)  , wxT(#v5)  } ; \
    const int klass::ms_constructorPropertiesCount = 6;

// 7 params

template<typename Class,
typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct wxConstructorBridge_7 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2) ,
            args[3].wxTEMPLATED_MEMBER_CALL(Get , T3) ,
            args[4].wxTEMPLATED_MEMBER_CALL(Get , T4) ,
            args[5].wxTEMPLATED_MEMBER_CALL(Get , T5) ,
            args[6].wxTEMPLATED_MEMBER_CALL(Get , T6)
            );
    }
};

#define wxCONSTRUCTOR_7(klass,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6) \
    wxConstructorBridge_7<klass,t0,t1,t2,t3,t4,t5,t6> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  , wxT(#v2)  , wxT(#v3)  , wxT(#v4)  , wxT(#v5)  , wxT(#v6) } ; \
    const int klass::ms_constructorPropertiesCount = 7;

// 8 params

template<typename Class,
typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
struct wxConstructorBridge_8 : public wxConstructorBridge
{
    void Create(wxObject * &o, wxxVariant *args)
    {
        Class *obj = dynamic_cast<Class*>(o);
        obj->Create(
            args[0].wxTEMPLATED_MEMBER_CALL(Get , T0) ,
            args[1].wxTEMPLATED_MEMBER_CALL(Get , T1) ,
            args[2].wxTEMPLATED_MEMBER_CALL(Get , T2) ,
            args[3].wxTEMPLATED_MEMBER_CALL(Get , T3) ,
            args[4].wxTEMPLATED_MEMBER_CALL(Get , T4) ,
            args[5].wxTEMPLATED_MEMBER_CALL(Get , T5) ,
            args[6].wxTEMPLATED_MEMBER_CALL(Get , T6) ,
            args[7].wxTEMPLATED_MEMBER_CALL(Get , T7)
            );
    }
};

#define wxCONSTRUCTOR_8(klass,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7) \
    wxConstructorBridge_8<klass,t0,t1,t2,t3,t4,t5,t6,t7> constructor##klass ; \
    wxConstructorBridge* klass::ms_constructor = &constructor##klass ; \
    const wxChar *klass::ms_constructorProperties[] = { wxT(#v0)  , wxT(#v1)  , wxT(#v2)  , wxT(#v3)  , wxT(#v4)  , wxT(#v5)  , wxT(#v6) , wxT(#v7) } ; \
    const int klass::ms_constructorPropertiesCount = 8;
// ----------------------------------------------------------------------------
// wxClassInfo
// ----------------------------------------------------------------------------

typedef wxObject *(*wxObjectConstructorFn)(void);
typedef wxObject* (*wxVariantToObjectConverter)( wxxVariant &data ) ;
typedef wxxVariant (*wxObjectToVariantConverter)( wxObject* ) ;

class WXDLLIMPEXP_BASE wxWriter;
class WXDLLIMPEXP_BASE wxPersister;

typedef bool (*wxObjectStreamingCallback) ( const wxObject *, wxWriter * , wxPersister * , wxxVariantArray & ) ;

class WXDLLIMPEXP_BASE wxClassInfo
{
    friend class WXDLLIMPEXP_BASE wxPropertyInfo ;
    friend class WXDLLIMPEXP_BASE wxHandlerInfo ;
public:
    wxClassInfo(const wxClassInfo **_Parents,
        const wxChar *_UnitName,
        const wxChar *_ClassName,
        int size,
        wxObjectConstructorFn ctor ,
        wxPropertyInfo *_Props ,
        wxHandlerInfo *_Handlers ,
        wxConstructorBridge* _Constructor ,
        const wxChar ** _ConstructorProperties ,
        const int _ConstructorPropertiesCount ,
        wxVariantToObjectConverter _PtrConverter1 ,
        wxVariantToObjectConverter _Converter2 ,
        wxObjectToVariantConverter _Converter3 ,
        wxObjectStreamingCallback _streamingCallback = NULL
        ) :

            m_className(_ClassName),
            m_objectSize(size),
            m_objectConstructor(ctor),
            m_next(sm_first),
            m_firstProperty(_Props),
            m_firstHandler(_Handlers),
            m_parents(_Parents),
            m_unitName(_UnitName),
            m_constructor(_Constructor),
            m_constructorProperties(_ConstructorProperties),
            m_constructorPropertiesCount(_ConstructorPropertiesCount),
            m_variantOfPtrToObjectConverter(_PtrConverter1),
            m_variantToObjectConverter(_Converter2),
            m_objectToVariantConverter(_Converter3),
            m_streamingCallback(_streamingCallback)
    {
        sm_first = this;
        Register() ;
    }

    wxClassInfo(const wxChar *_UnitName, const wxChar *_ClassName,
                const wxClassInfo **_Parents) :
            m_className(_ClassName),
            m_objectSize(0),
            m_objectConstructor(NULL),
            m_next(sm_first),
            m_firstProperty(NULL),
            m_firstHandler(NULL),
            m_parents(_Parents),
            m_unitName(_UnitName),
            m_constructor(NULL),
            m_constructorProperties(NULL),
            m_constructorPropertiesCount(0),
            m_variantOfPtrToObjectConverter(NULL),
            m_variantToObjectConverter(NULL),
            m_objectToVariantConverter(NULL),
            m_streamingCallback(NULL)
    {
        sm_first = this;
        Register() ;
    }

    virtual ~wxClassInfo() ;

    // allocates an instance of this class, this object does not have to be initialized or fully constructed
    // as this call will be followed by a call to Create
    virtual wxObject *AllocateObject() const { return m_objectConstructor ? (*m_objectConstructor)() : 0; }

    // 'old naming' for AllocateObject staying here for backward compatibility
    wxObject *CreateObject() const { return AllocateObject() ; }

    // direct construction call for classes that cannot construct instances via alloc/create
    wxObject *ConstructObject(int ParamCount, wxxVariant *Params) const
    {
        if ( ParamCount != m_constructorPropertiesCount )
        {
            wxLogError( _("Illegal Parameter Count for ConstructObject Method") ) ;
            return NULL ;
        }
        wxObject *object = NULL ;
        m_constructor->Create( object , Params ) ;
        return object ;
    }

    bool NeedsDirectConstruction() const { return dynamic_cast<wxDirectConstructorBrigde*>( m_constructor) != NULL ; }

    const wxChar       *GetClassName() const { return m_className; }
    const wxChar       *GetBaseClassName1() const
        { return m_parents[0] ? m_parents[0]->GetClassName() : NULL; }
    const wxChar       *GetBaseClassName2() const
        { return (m_parents[0] && m_parents[1]) ? m_parents[1]->GetClassName() : NULL; }
    const wxChar       *GetIncludeName() const { return m_unitName ; }
    const wxClassInfo **GetParents() const { return m_parents; }
    int                 GetSize() const { return m_objectSize; }
    bool                IsDynamic() const { return (NULL != m_objectConstructor); }

    wxObjectConstructorFn      GetConstructor() const { return m_objectConstructor; }
    static const wxClassInfo  *GetFirst() { return sm_first; }
    const wxClassInfo         *GetNext() const { return m_next; }
    static wxClassInfo        *FindClass(const wxChar *className);

    // Climb upwards through inheritance hierarchy.
    // Dual inheritance is catered for.

    bool IsKindOf(const wxClassInfo *info) const
    {
        if ( info != 0 )
        {
            if ( info == this )
                return true ;

            for ( int i = 0 ; m_parents[i] ; ++ i )
            {
                if ( m_parents[i]->IsKindOf( info ) )
                    return true ;
            }
        }
        return false ;
    }

    // if there is a callback registered with that class it will be called
    // before this object will be written to disk, it can veto streaming out
    // this object by returning false, if this class has not registered a
    // callback, the search will go up the inheritance tree if no callback has
    // been registered true will be returned by default
    bool BeforeWriteObject( const wxObject *obj, wxWriter *streamer , wxPersister *persister , wxxVariantArray &metadata) const  ;

    // gets the streaming callback from this class or any superclass
    wxObjectStreamingCallback GetStreamingCallback() const ;

#if WXWIN_COMPATIBILITY_2_4
    // Initializes parent pointers and hash table for fast searching.
    wxDEPRECATED( static void InitializeClasses() );
    // Cleans up hash table used for fast searching.
    wxDEPRECATED( static void CleanUpClasses() );
#endif
    static void CleanUp();

    // returns the first property
    const wxPropertyInfo* GetFirstProperty() const { return m_firstProperty ; }

    // returns the first handler
    const wxHandlerInfo* GetFirstHandler() const { return m_firstHandler ; }

    // Call the Create upon an instance of the class, in the end the object is fully
    // initialized
    virtual void Create (wxObject *object, int ParamCount, wxxVariant *Params) const
    {
        if ( ParamCount != m_constructorPropertiesCount )
        {
            wxLogError( _("Illegal Parameter Count for Create Method") ) ;
            return ;
        }
        m_constructor->Create( object , Params ) ;
    }

    // get number of parameters for constructor
    virtual int GetCreateParamCount() const { return m_constructorPropertiesCount; }

    // get n-th constructor parameter
    virtual const wxChar* GetCreateParamName(int n) const { return m_constructorProperties[n] ; }

    // Runtime access to objects for simple properties (get/set) by property name, and variant data
    virtual void SetProperty (wxObject *object, const wxChar *propertyName, const wxxVariant &value) const ;
    virtual wxxVariant GetProperty (wxObject *object, const wxChar *propertyName) const;

    // Runtime access to objects for collection properties by property name
    virtual wxxVariantArray GetPropertyCollection(wxObject *object, const wxChar *propertyName) const ;
    virtual void AddToPropertyCollection(wxObject *object, const wxChar *propertyName , const wxxVariant& value) const ;

    // we must be able to cast variants to wxObject pointers, templates seem not to be suitable
    wxObject* VariantToInstance( wxxVariant &data ) const
    {
        if ( data.GetTypeInfo()->GetKind() == wxT_OBJECT )
            return m_variantToObjectConverter( data ) ;
        else
            return m_variantOfPtrToObjectConverter( data ) ;
    }

    wxxVariant InstanceToVariant( wxObject *object ) const { return m_objectToVariantConverter( object ) ; }

    // find property by name
    virtual const wxPropertyInfo *FindPropertyInfo (const wxChar *PropertyName) const ;

    // find handler by name
    virtual const wxHandlerInfo *FindHandlerInfo (const wxChar *PropertyName) const ;

    // find property by name
    virtual wxPropertyInfo *FindPropertyInfoInThisClass (const wxChar *PropertyName) const ;

    // find handler by name
    virtual wxHandlerInfo *FindHandlerInfoInThisClass (const wxChar *PropertyName) const ;

    // puts all the properties of this class and its superclasses in the map, as long as there is not yet
    // an entry with the same name (overriding mechanism)
    void GetProperties( wxPropertyInfoMap &map ) const ;
public:
    const wxChar            *m_className;
    int                      m_objectSize;
    wxObjectConstructorFn    m_objectConstructor;

    // class info object live in a linked list:
    // pointers to its head and the next element in it

    static wxClassInfo      *sm_first;
    wxClassInfo             *m_next;

    // FIXME: this should be private (currently used directly by way too
    //        many clients)
    static wxHashTable      *sm_classTable;

protected :
    wxPropertyInfo *         m_firstProperty ;
    wxHandlerInfo *          m_firstHandler ;
private:
    const wxClassInfo**      m_parents ;
    const wxChar*            m_unitName;

    wxConstructorBridge*     m_constructor ;
    const wxChar **          m_constructorProperties ;
    const int                m_constructorPropertiesCount ;
    wxVariantToObjectConverter m_variantOfPtrToObjectConverter ;
    wxVariantToObjectConverter m_variantToObjectConverter ;
    wxObjectToVariantConverter m_objectToVariantConverter ;
    wxObjectStreamingCallback m_streamingCallback ;
    const wxPropertyAccessor *FindAccessor (const wxChar *propertyName) const ;


    // InitializeClasses() helper
    static wxClassInfo *GetBaseByName(const wxChar *name) ;

protected:
    // registers the class
    void Register();
    void Unregister();

    DECLARE_NO_COPY_CLASS(wxClassInfo)
};


WXDLLIMPEXP_BASE wxObject *wxCreateDynamicObject(const wxChar *name);

// ----------------------------------------------------------------------------
// wxDynamicObject
// ----------------------------------------------------------------------------
//
// this object leads to having a pure runtime-instantiation

class WXDLLIMPEXP_BASE wxDynamicClassInfo : public wxClassInfo
{
    friend class WXDLLIMPEXP_BASE wxDynamicObject ;
public :
    wxDynamicClassInfo( const wxChar *_UnitName, const wxChar *_ClassName , const wxClassInfo* superClass ) ;
    virtual ~wxDynamicClassInfo() ;

    // constructs a wxDynamicObject with an instance
    virtual wxObject *AllocateObject() const ;

    // Call the Create method for a class
    virtual void Create (wxObject *object, int ParamCount, wxxVariant *Params) const ;

    // get number of parameters for constructor
    virtual int GetCreateParamCount() const ;

    // get i-th constructor parameter
    virtual const wxChar* GetCreateParamName(int i) const ;

    // Runtime access to objects by property name, and variant data
    virtual void SetProperty (wxObject *object, const wxChar *PropertyName, const wxxVariant &Value) const ;
    virtual wxxVariant GetProperty (wxObject *object, const wxChar *PropertyName) const ;

    // adds a property to this class at runtime
    void AddProperty( const wxChar *propertyName , const wxTypeInfo* typeInfo ) ;

    // removes an existing runtime-property
    void RemoveProperty( const wxChar *propertyName ) ;

    // renames an existing runtime-property
    void RenameProperty( const wxChar *oldPropertyName , const wxChar *newPropertyName ) ;

    // as a handler to this class at runtime
    void AddHandler( const wxChar *handlerName , wxObjectEventFunction address , const wxClassInfo* eventClassInfo ) ;

    // removes an existing runtime-handler
    void RemoveHandler( const wxChar *handlerName ) ;

    // renames an existing runtime-handler
    void RenameHandler( const wxChar *oldHandlerName , const wxChar *newHandlerName ) ;
private :
    struct wxDynamicClassInfoInternal ;
    wxDynamicClassInfoInternal* m_data ;
} ;

// ----------------------------------------------------------------------------
// Dynamic class macros
// ----------------------------------------------------------------------------

#define _DECLARE_DYNAMIC_CLASS(name)           \
 public:                                      \
 static wxClassInfo ms_classInfo;          \
 static const wxClassInfo* ms_classParents[] ; \
 static wxPropertyInfo* GetPropertiesStatic() ; \
 static wxHandlerInfo* GetHandlersStatic() ; \
 static wxClassInfo *GetClassInfoStatic()   \
{ return &name::ms_classInfo; } \
    virtual wxClassInfo *GetClassInfo() const   \
{ return &name::ms_classInfo; }

/*
#define _DECLARE_DYNAMIC_CLASS(name)           \
 public:                                      \
 static wxClassInfo ms_class##name;          \
 static const wxClassInfo* ms_classParents##name[] ; \
 static wxPropertyInfo* GetPropertiesStatic() ; \
 static wxHandlerInfo* GetHandlersStatic() ; \
 static wxClassInfo *GetClassInfoStatic()   \
{ return &name::ms_class##name; } \
    virtual wxClassInfo *GetClassInfo() const   \
{ return &name::ms_class##name; }
*/
#define DECLARE_DYNAMIC_CLASS(name)           \
    static wxConstructorBridge* ms_constructor ; \
    static const wxChar * ms_constructorProperties[] ; \
    static const int ms_constructorPropertiesCount ; \
    _DECLARE_DYNAMIC_CLASS(name)

#define DECLARE_DYNAMIC_CLASS_NO_ASSIGN(name)   \
    DECLARE_NO_ASSIGN_CLASS(name)               \
    DECLARE_DYNAMIC_CLASS(name)

#define DECLARE_DYNAMIC_CLASS_NO_COPY(name)   \
    DECLARE_NO_COPY_CLASS(name)               \
    DECLARE_DYNAMIC_CLASS(name)

#define DECLARE_ABSTRACT_CLASS(name) _DECLARE_DYNAMIC_CLASS(name)
#define DECLARE_CLASS(name) DECLARE_DYNAMIC_CLASS(name)

// -----------------------------------
// for concrete classes
// -----------------------------------

// Single inheritance with one base class

#define _TYPEINFO_CLASSES(n , toString , fromString ) \
    wxClassTypeInfo s_typeInfo##n(wxT_OBJECT , &n::ms_classInfo , toString , fromString , typeid(n).name()) ; \
    wxClassTypeInfo s_typeInfoPtr##n(wxT_OBJECT_PTR , &n::ms_classInfo , toString , fromString , typeid(n*).name()) ;

#define _IMPLEMENT_DYNAMIC_CLASS(name, basename, unit , callback)                 \
    wxObject* wxConstructorFor##name()                             \
{ return new name; }                                          \
    const wxClassInfo* name::ms_classParents[] = { &basename::ms_classInfo ,NULL } ; \
    wxObject* wxVariantOfPtrToObjectConverter##name ( wxxVariant &data ) { return data.wxTEMPLATED_MEMBER_CALL(Get , name*) ; } \
    wxxVariant wxObjectToVariantConverter##name ( wxObject *data ) { return wxxVariant( dynamic_cast<name*> (data)  ) ; } \
    wxClassInfo name::ms_classInfo(name::ms_classParents , wxT(unit) , wxT(#name),   \
    (int) sizeof(name),                              \
    (wxObjectConstructorFn) wxConstructorFor##name   ,   \
    name::GetPropertiesStatic(),name::GetHandlersStatic(),name::ms_constructor , name::ms_constructorProperties ,     \
    name::ms_constructorPropertiesCount , wxVariantOfPtrToObjectConverter##name , NULL , wxObjectToVariantConverter##name , callback);

#define _IMPLEMENT_DYNAMIC_CLASS_WITH_COPY(name, basename, unit, callback )                 \
    wxObject* wxConstructorFor##name()                             \
{ return new name; }                                          \
    const wxClassInfo* name::ms_classParents[] = { &basename::ms_classInfo ,NULL } ; \
    wxObject* wxVariantToObjectConverter##name ( wxxVariant &data ) { return &data.wxTEMPLATED_MEMBER_CALL(Get , name) ; } \
    wxObject* wxVariantOfPtrToObjectConverter##name ( wxxVariant &data ) { return data.wxTEMPLATED_MEMBER_CALL(Get , name*) ; } \
    wxxVariant wxObjectToVariantConverter##name ( wxObject *data ) { return wxxVariant( dynamic_cast<name*> (data)  ) ; } \
    wxClassInfo name::ms_classInfo(name::ms_classParents , wxT(unit) , wxT(#name),   \
    (int) sizeof(name),                              \
    (wxObjectConstructorFn) wxConstructorFor##name   ,   \
    name::GetPropertiesStatic(),name::GetHandlersStatic(),name::ms_constructor , name::ms_constructorProperties,     \
    name::ms_constructorPropertiesCount , wxVariantOfPtrToObjectConverter##name , wxVariantToObjectConverter##name , wxObjectToVariantConverter##name, callback);

#define IMPLEMENT_DYNAMIC_CLASS_WITH_COPY( name , basename ) \
    _IMPLEMENT_DYNAMIC_CLASS_WITH_COPY( name , basename , "" , NULL ) \
    _TYPEINFO_CLASSES(name, NULL , NULL) \
    const wxPropertyInfo *name::GetPropertiesStatic() { return (wxPropertyInfo*) NULL ; } \
    const wxHandlerInfo *name::GetHandlersStatic() { return (wxHandlerInfo*) NULL ; } \
    wxCONSTRUCTOR_DUMMY( name )

#define IMPLEMENT_DYNAMIC_CLASS( name , basename ) \
    _IMPLEMENT_DYNAMIC_CLASS( name , basename , "" , NULL ) \
     _TYPEINFO_CLASSES(name, NULL , NULL) \
   wxPropertyInfo *name::GetPropertiesStatic() { return (wxPropertyInfo*) NULL ; } \
    wxHandlerInfo *name::GetHandlersStatic() { return (wxHandlerInfo*) NULL ; } \
    wxCONSTRUCTOR_DUMMY( name )

#define IMPLEMENT_DYNAMIC_CLASS_XTI( name , basename , unit ) \
    _IMPLEMENT_DYNAMIC_CLASS( name , basename , unit , NULL ) \
    _TYPEINFO_CLASSES(name, NULL , NULL)

#define IMPLEMENT_DYNAMIC_CLASS_XTI_CALLBACK( name , basename , unit , callback ) \
    _IMPLEMENT_DYNAMIC_CLASS( name , basename , unit , &callback ) \
    _TYPEINFO_CLASSES(name, NULL , NULL)

#define IMPLEMENT_DYNAMIC_CLASS_WITH_COPY_XTI( name , basename , unit ) \
    _IMPLEMENT_DYNAMIC_CLASS_WITH_COPY( name , basename , unit , NULL  ) \
    _TYPEINFO_CLASSES(name, NULL , NULL)

#define IMPLEMENT_DYNAMIC_CLASS_WITH_COPY_AND_STREAMERS_XTI( name , basename , unit , toString , fromString ) \
    _IMPLEMENT_DYNAMIC_CLASS_WITH_COPY( name , basename , unit , NULL  ) \
    _TYPEINFO_CLASSES(name, toString , fromString)

// this is for classes that do not derive from wxobject, there are no creators for these

#define IMPLEMENT_DYNAMIC_CLASS_NO_WXOBJECT_NO_BASE_XTI( name , unit ) \
    const wxClassInfo* name::ms_classParents[] = { NULL } ; \
    wxClassInfo name::ms_classInfo(name::ms_classParents , wxEmptyString , wxT(#name),   \
    (int) sizeof(name),                              \
    (wxObjectConstructorFn) 0   ,   \
    name::GetPropertiesStatic(),name::GetHandlersStatic(),0 , 0 ,     \
    0 , 0 , 0 );    \
    _TYPEINFO_CLASSES(name, NULL , NULL)

// this is for subclasses that still do not derive from wxobject

#define IMPLEMENT_DYNAMIC_CLASS_NO_WXOBJECT_XTI( name , basename, unit ) \
    const wxClassInfo* name::ms_classParents[] = { &basename::ms_classInfo ,NULL } ; \
    wxClassInfo name::ms_classInfo(name::ms_classParents , wxEmptyString , wxT(#name),   \
    (int) sizeof(name),                              \
    (wxObjectConstructorFn) 0   ,   \
    name::GetPropertiesStatic(),name::GetHandlersStatic(),0 , 0 ,     \
    0 , 0 , 0 );    \
    _TYPEINFO_CLASSES(name, NULL , NULL)


// Multiple inheritance with two base classes

#define _IMPLEMENT_DYNAMIC_CLASS2(name, basename, basename2, unit)                 \
    wxObject* wxConstructorFor##name()                             \
{ return new name; }                                          \
    const wxClassInfo* name::ms_classParents[] = { &basename::ms_classInfo ,&basename2::ms_classInfo , NULL } ; \
    wxObject* wxVariantToObjectConverter##name ( wxxVariant &data ) { return data.wxTEMPLATED_MEMBER_CALL(Get , name*) ; } \
    wxxVariant wxObjectToVariantConverter##name ( wxObject *data ) { return wxxVariant( dynamic_cast<name*> (data)  ) ; } \
    wxClassInfo name::ms_classInfo(name::ms_classParents , wxT(unit) , wxT(#name),   \
    (int) sizeof(name),                              \
    (wxObjectConstructorFn) wxConstructorFor##name   ,   \
    name::GetPropertiesStatic(),name::GetHandlersStatic(),name::ms_constructor , name::ms_constructorProperties ,     \
    name::ms_constructorPropertiesCount , wxVariantToObjectConverter##name, wxVariantToObjectConverter##name , wxObjectToVariantConverter##name);    \

#define IMPLEMENT_DYNAMIC_CLASS2( name , basename , basename2) \
    _IMPLEMENT_DYNAMIC_CLASS2( name , basename , basename2 , "") \
    _TYPEINFO_CLASSES(name, NULL , NULL) \
    wxPropertyInfo *name::GetPropertiesStatic() { return (wxPropertyInfo*) NULL ; } \
    wxHandlerInfo *name::GetHandlersStatic() { return (wxHandlerInfo*) NULL ; } \
    wxCONSTRUCTOR_DUMMY( name )

#define IMPLEMENT_DYNAMIC_CLASS2_XTI( name , basename , basename2, unit) \
    _IMPLEMENT_DYNAMIC_CLASS2( name , basename , basename2 , unit) \
    _TYPEINFO_CLASSES(name, NULL , NULL)


// -----------------------------------
// for abstract classes
// -----------------------------------

// Single inheritance with one base class

#define _IMPLEMENT_ABSTRACT_CLASS(name, basename)                \
    const wxClassInfo* name::ms_classParents[] = { &basename::ms_classInfo ,NULL } ; \
    wxObject* wxVariantToObjectConverter##name ( wxxVariant &data ) { return data.wxTEMPLATED_MEMBER_CALL(Get , name*) ; } \
    wxObject* wxVariantOfPtrToObjectConverter##name ( wxxVariant &data ) { return data.wxTEMPLATED_MEMBER_CALL(Get , name*) ; } \
    wxxVariant wxObjectToVariantConverter##name ( wxObject *data ) { return wxxVariant( dynamic_cast<name*> (data)  ) ; } \
    wxClassInfo name::ms_classInfo(name::ms_classParents , wxEmptyString , wxT(#name),   \
    (int) sizeof(name),                              \
    (wxObjectConstructorFn) 0   ,   \
    name::GetPropertiesStatic(),name::GetHandlersStatic(),0 , 0 ,     \
    0 , wxVariantOfPtrToObjectConverter##name ,wxVariantToObjectConverter##name , wxObjectToVariantConverter##name);    \
    _TYPEINFO_CLASSES(name, NULL , NULL)

#define IMPLEMENT_ABSTRACT_CLASS( name , basename ) \
    _IMPLEMENT_ABSTRACT_CLASS( name , basename ) \
    wxHandlerInfo *name::GetHandlersStatic() { return (wxHandlerInfo*) NULL ; } \
    wxPropertyInfo *name::GetPropertiesStatic() { return (wxPropertyInfo*) NULL ; }

// Multiple inheritance with two base classes

#define IMPLEMENT_ABSTRACT_CLASS2(name, basename1, basename2)   \
    wxClassInfo name::ms_classInfo(wxT(#name), wxT(#basename1),  \
    wxT(#basename2), (int) sizeof(name),                \
    (wxObjectConstructorFn) 0);

#define IMPLEMENT_CLASS IMPLEMENT_ABSTRACT_CLASS
#define IMPLEMENT_CLASS2 IMPLEMENT_ABSTRACT_CLASS2

#define wxBEGIN_EVENT_TABLE( a , b ) BEGIN_EVENT_TABLE( a , b )
#define wxEND_EVENT_TABLE() END_EVENT_TABLE()

// --------------------------------------------------------------------------
// Collection Support
// --------------------------------------------------------------------------

template<typename iter , typename collection_t > void wxListCollectionToVariantArray( const collection_t& coll , wxxVariantArray &value )
{
    iter current = coll.GetFirst() ;
    while (current)
    {
        value.Add( new wxxVariant(current->GetData()) ) ;
        current = current->GetNext();
    }
}

template<typename collection_t> void wxArrayCollectionToVariantArray( const collection_t& coll , wxxVariantArray &value )
{
    for( size_t i = 0 ; i < coll.GetCount() ; i++ )
    {
        value.Add( new wxxVariant(coll[i]) ) ;
    }
}


#endif // _WX_XTIH__
