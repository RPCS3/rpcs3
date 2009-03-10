/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/xti.cpp
// Purpose:     runtime metadata information (extended class info
// Author:      Stefan Csomor
// Modified by:
// Created:     27/07/03
// RCS-ID:      $Id: xti.cpp 38857 2006-04-20 07:31:44Z ABX $
// Copyright:   (c) 1997 Julian Smart
//              (c) 2003 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_EXTENDED_RTTI

#ifndef WX_PRECOMP
    #include "wx/object.h"
    #include "wx/list.h"
    #include "wx/hash.h"
#endif

#include "wx/xti.h"
#include "wx/xml/xml.h"
#include "wx/tokenzr.h"
#include <string.h>

#include "wx/beforestd.h"
#include <map>
#include <string>
#include <list>
#include "wx/afterstd.h"

using namespace std ;

// ----------------------------------------------------------------------------
// Enum Support
// ----------------------------------------------------------------------------

wxEnumData::wxEnumData( wxEnumMemberData* data )
{
    m_members = data ;
    for ( m_count = 0; m_members[m_count].m_name ; m_count++)
    {} ;
}

bool wxEnumData::HasEnumMemberValue(const wxChar *name, int *value) const
{
    int i;
    for (i = 0; m_members[i].m_name ; i++ )
    {
        if (!wxStrcmp(name, m_members[i].m_name))
        {
            if ( value )
                *value = m_members[i].m_value;
            return true ;
        }
    }
    return false ;
}

int wxEnumData::GetEnumMemberValue(const wxChar *name) const
{
    int i;
    for (i = 0; m_members[i].m_name ; i++ )
    {
        if (!wxStrcmp(name, m_members[i].m_name))
        {
            return m_members[i].m_value;
        }
    }
    return 0 ;
}

const wxChar *wxEnumData::GetEnumMemberName(int value) const
{
    int i;
    for (i = 0; m_members[i].m_name ; i++)
        if (value == m_members[i].m_value)
            return m_members[i].m_name;

    return wxEmptyString ;
}

int wxEnumData::GetEnumMemberValueByIndex( int idx ) const
{
    // we should cache the count in order to avoid out-of-bounds errors
    return m_members[idx].m_value ;
}

const wxChar * wxEnumData::GetEnumMemberNameByIndex( int idx ) const
{
    // we should cache the count in order to avoid out-of-bounds errors
    return m_members[idx].m_name ;
}

// ----------------------------------------------------------------------------
// Type Information
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// value streaming
// ----------------------------------------------------------------------------

// streamer specializations
// for all built-in types

// bool

template<> void wxStringReadValue(const wxString &s , bool &data )
{
    int intdata ;
    wxSscanf(s, _T("%d"), &intdata ) ;
    data = (bool)intdata ;
}

template<> void wxStringWriteValue(wxString &s , const bool &data )
{
    s = wxString::Format(_T("%d"), data ) ;
}

// char

template<> void wxStringReadValue(const wxString &s , char &data )
{
    int intdata ;
    wxSscanf(s, _T("%d"), &intdata ) ;
    data = char(intdata) ;
}

template<> void wxStringWriteValue(wxString &s , const char &data )
{
    s = wxString::Format(_T("%d"), data ) ;
}

// unsigned char

template<> void wxStringReadValue(const wxString &s , unsigned char &data )
{
    int intdata ;
    wxSscanf(s, _T("%d"), &intdata ) ;
    data = (unsigned char)(intdata) ;
}

template<> void wxStringWriteValue(wxString &s , const unsigned char &data )
{
    s = wxString::Format(_T("%d"), data ) ;
}

// int

template<> void wxStringReadValue(const wxString &s , int &data )
{
    wxSscanf(s, _T("%d"), &data ) ;
}

template<> void wxStringWriteValue(wxString &s , const int &data )
{
    s = wxString::Format(_T("%d"), data ) ;
}

// unsigned int

template<> void wxStringReadValue(const wxString &s , unsigned int &data )
{
    wxSscanf(s, _T("%d"), &data ) ;
}

template<> void wxStringWriteValue(wxString &s , const unsigned int &data )
{
    s = wxString::Format(_T("%d"), data ) ;
}

// long

template<> void wxStringReadValue(const wxString &s , long &data )
{
    wxSscanf(s, _T("%ld"), &data ) ;
}

template<> void wxStringWriteValue(wxString &s , const long &data )
{
    s = wxString::Format(_T("%ld"), data ) ;
}

// unsigned long

template<> void wxStringReadValue(const wxString &s , unsigned long &data )
{
    wxSscanf(s, _T("%ld"), &data ) ;
}

template<> void wxStringWriteValue(wxString &s , const unsigned long &data )
{
    s = wxString::Format(_T("%ld"), data ) ;
}

// float

template<> void wxStringReadValue(const wxString &s , float &data )
{
    wxSscanf(s, _T("%f"), &data ) ;
}

template<> void wxStringWriteValue(wxString &s , const float &data )
{
    s = wxString::Format(_T("%f"), data ) ;
}

// double

template<> void wxStringReadValue(const wxString &s , double &data )
{
    wxSscanf(s, _T("%lf"), &data ) ;
}

template<> void wxStringWriteValue(wxString &s , const double &data )
{
    s = wxString::Format(_T("%lf"), data ) ;
}

// wxString

template<> void wxStringReadValue(const wxString &s , wxString &data )
{
    data = s ;
}

template<> void wxStringWriteValue(wxString &s , const wxString &data )
{
    s = data ;
}

// built-ins
//

#if wxUSE_FUNC_TEMPLATE_POINTER
#define wxBUILTIN_TYPE_INFO( element , type ) \
    wxBuiltInTypeInfo s_typeInfo##type(element , &wxToStringConverter<type> , &wxFromStringConverter<type> , typeid(type).name()) ;
#else
#define wxBUILTIN_TYPE_INFO( element , type ) \
    void _toString##element( const wxxVariant& data , wxString &result ) { wxToStringConverter<type>(data, result); } \
    void _fromString##element( const wxString& data , wxxVariant &result ) { wxFromStringConverter<type>(data, result); } \
    wxBuiltInTypeInfo s_typeInfo##type(element , &_toString##element , &_fromString##element , typeid(type).name()) ;
#endif

typedef unsigned char unsigned_char;
typedef unsigned int unsigned_int;
typedef unsigned long unsigned_long;

wxBuiltInTypeInfo s_typeInfovoid( wxT_VOID , NULL , NULL , typeid(void).name());
wxBUILTIN_TYPE_INFO( wxT_BOOL ,  bool);
wxBUILTIN_TYPE_INFO( wxT_CHAR ,  char);
wxBUILTIN_TYPE_INFO( wxT_UCHAR , unsigned_char);
wxBUILTIN_TYPE_INFO( wxT_INT , int);
wxBUILTIN_TYPE_INFO( wxT_UINT , unsigned_int);
wxBUILTIN_TYPE_INFO( wxT_LONG , long);
wxBUILTIN_TYPE_INFO( wxT_ULONG , unsigned_long);
wxBUILTIN_TYPE_INFO( wxT_FLOAT , float);
wxBUILTIN_TYPE_INFO( wxT_DOUBLE , double);
wxBUILTIN_TYPE_INFO( wxT_STRING , wxString);


// this are compiler induced specialization which are never used anywhere

wxILLEGAL_TYPE_SPECIALIZATION( char const * )
wxILLEGAL_TYPE_SPECIALIZATION( char * )
wxILLEGAL_TYPE_SPECIALIZATION( unsigned char * )
wxILLEGAL_TYPE_SPECIALIZATION( int * )
wxILLEGAL_TYPE_SPECIALIZATION( bool * )
wxILLEGAL_TYPE_SPECIALIZATION( long * )
wxILLEGAL_TYPE_SPECIALIZATION( wxString * )

wxCOLLECTION_TYPE_INFO( wxString , wxArrayString ) ;

template<> void wxCollectionToVariantArray( wxArrayString const &theArray, wxxVariantArray &value)
{
    wxArrayCollectionToVariantArray( theArray , value ) ;
}

wxTypeInfoMap *wxTypeInfo::ms_typeTable = NULL ;

wxTypeInfo *wxTypeInfo::FindType(const wxChar *typeName)
{
    wxTypeInfoMap::iterator iter = ms_typeTable->find(typeName) ;
    wxASSERT_MSG( iter != ms_typeTable->end() , wxT("lookup for a non-existent type-info") ) ;
    return (wxTypeInfo *)iter->second;
}

#if wxUSE_UNICODE
wxClassTypeInfo::wxClassTypeInfo( wxTypeKind kind , wxClassInfo* classInfo , converterToString_t to , converterFromString_t from , const char *name) :
wxTypeInfo( kind , to , from , name)
{ wxASSERT_MSG( kind == wxT_OBJECT_PTR || kind == wxT_OBJECT , wxT("Illegal Kind for Enum Type")) ; m_classInfo = classInfo ;}
#endif

wxClassTypeInfo::wxClassTypeInfo( wxTypeKind kind , wxClassInfo* classInfo , converterToString_t to , converterFromString_t from , const wxString &name) :
wxTypeInfo( kind , to , from , name)
{ wxASSERT_MSG( kind == wxT_OBJECT_PTR || kind == wxT_OBJECT , wxT("Illegal Kind for Enum Type")) ; m_classInfo = classInfo ;}

wxDelegateTypeInfo::wxDelegateTypeInfo( int eventType , wxClassInfo* eventClass , converterToString_t to , converterFromString_t from ) :
wxTypeInfo ( wxT_DELEGATE , to , from , wxEmptyString )
{ m_eventClass = eventClass ; m_eventType = eventType ; m_lastEventType = -1 ;}

wxDelegateTypeInfo::wxDelegateTypeInfo( int eventType , int lastEventType , wxClassInfo* eventClass , converterToString_t to , converterFromString_t from ) :
wxTypeInfo ( wxT_DELEGATE , to , from , wxEmptyString )
{ m_eventClass = eventClass ; m_eventType = eventType ; m_lastEventType = lastEventType; }

void wxTypeInfo::Register()
{
    if ( ms_typeTable == NULL )
        ms_typeTable = new wxTypeInfoMap() ;

    if( !m_name.empty() )
        (*ms_typeTable)[m_name] = this ;
}

void wxTypeInfo::Unregister()
{
    if( !m_name.empty() )
        ms_typeTable->erase(m_name);
}

// removing header dependancy on string tokenizer

void wxSetStringToArray( const wxString &s , wxArrayString &array )
{
    wxStringTokenizer tokenizer(s, wxT("| \t\n"), wxTOKEN_STRTOK);
    wxString flag;
    array.Clear() ;
    while (tokenizer.HasMoreTokens())
    {
        array.Add(tokenizer.GetNextToken()) ;
    }
}

// ----------------------------------------------------------------------------
// wxClassInfo
// ----------------------------------------------------------------------------

wxPropertyInfo::~wxPropertyInfo()
{
    if ( this == m_itsClass->m_firstProperty )
    {
        m_itsClass->m_firstProperty = m_next;
    }
    else
    {
        wxPropertyInfo *info = m_itsClass->m_firstProperty;
        while (info)
        {
            if ( info->m_next == this )
            {
                info->m_next = m_next;
                break;
            }

            info = info->m_next;
        }
    }
}

wxHandlerInfo::~wxHandlerInfo()
{
    if ( this == m_itsClass->m_firstHandler )
    {
        m_itsClass->m_firstHandler = m_next;
    }
    else
    {
        wxHandlerInfo *info = m_itsClass->m_firstHandler;
        while (info)
        {
            if ( info->m_next == this )
            {
                info->m_next = m_next;
                break;
            }

            info = info->m_next;
        }
    }
}

const wxPropertyAccessor *wxClassInfo::FindAccessor(const wxChar *PropertyName) const
{
    const wxPropertyInfo* info = FindPropertyInfo( PropertyName ) ;

    if ( info )
        return info->GetAccessor() ;

    return NULL ;
}

wxPropertyInfo *wxClassInfo::FindPropertyInfoInThisClass (const wxChar *PropertyName) const
{
    wxPropertyInfo* info = m_firstProperty ;

    while( info )
    {
        if ( wxStrcmp( info->GetName() , PropertyName ) == 0 )
            return info ;
        info = info->GetNext() ;
    }

    return 0;
}

const wxPropertyInfo *wxClassInfo::FindPropertyInfo (const wxChar *PropertyName) const
{
    const wxPropertyInfo* info = FindPropertyInfoInThisClass( PropertyName ) ;
    if ( info )
        return info ;

    const wxClassInfo** parents = GetParents() ;
    for ( int i = 0 ; parents[i] ; ++ i )
    {
        if ( ( info = parents[i]->FindPropertyInfo( PropertyName ) ) != NULL )
            return info ;
    }

    return 0;
}

wxHandlerInfo *wxClassInfo::FindHandlerInfoInThisClass (const wxChar *PropertyName) const
{
    wxHandlerInfo* info = m_firstHandler ;

    while( info )
    {
        if ( wxStrcmp( info->GetName() , PropertyName ) == 0 )
            return info ;
        info = info->GetNext() ;
    }

    return 0;
}

const wxHandlerInfo *wxClassInfo::FindHandlerInfo (const wxChar *PropertyName) const
{
    const wxHandlerInfo* info = FindHandlerInfoInThisClass( PropertyName ) ;

    if ( info )
        return info ;

    const wxClassInfo** parents = GetParents() ;
    for ( int i = 0 ; parents[i] ; ++ i )
    {
        if ( ( info = parents[i]->FindHandlerInfo( PropertyName ) ) != NULL )
            return info ;
    }

    return 0;
}

wxObjectStreamingCallback wxClassInfo::GetStreamingCallback() const
{
    if ( m_streamingCallback )
        return m_streamingCallback ;

    wxObjectStreamingCallback retval = NULL ;
    const wxClassInfo** parents = GetParents() ;
    for ( int i = 0 ; parents[i] && retval == NULL ; ++ i )
    {
        retval = parents[i]->GetStreamingCallback() ;
    }
    return retval ;
}

bool wxClassInfo::BeforeWriteObject( const wxObject *obj, wxWriter *streamer , wxPersister *persister , wxxVariantArray &metadata) const
{
    wxObjectStreamingCallback sb = GetStreamingCallback() ;
    if ( sb )
        return (*sb)(obj , streamer , persister , metadata ) ;

    return true ;
}

void wxClassInfo::SetProperty(wxObject *object, const wxChar *propertyName, const wxxVariant &value) const
{
    const wxPropertyAccessor *accessor;

    accessor = FindAccessor(propertyName);
    wxASSERT(accessor->HasSetter());
    accessor->SetProperty( object , value ) ;
}

wxxVariant wxClassInfo::GetProperty(wxObject *object, const wxChar *propertyName) const
{
    const wxPropertyAccessor *accessor;

    accessor = FindAccessor(propertyName);
    wxASSERT(accessor->HasGetter());
    wxxVariant result ;
    accessor->GetProperty(object,result);
    return result ;
}

wxxVariantArray wxClassInfo::GetPropertyCollection(wxObject *object, const wxChar *propertyName) const
{
    const wxPropertyAccessor *accessor;

    accessor = FindAccessor(propertyName);
    wxASSERT(accessor->HasGetter());
    wxxVariantArray result ;
    accessor->GetPropertyCollection(object,result);
    return result ;
}

void wxClassInfo::AddToPropertyCollection(wxObject *object, const wxChar *propertyName , const wxxVariant& value) const
{
    const wxPropertyAccessor *accessor;

    accessor = FindAccessor(propertyName);
    wxASSERT(accessor->HasAdder());
    accessor->AddToPropertyCollection( object , value ) ;
}

// void wxClassInfo::GetProperties( wxPropertyInfoMap &map ) const
// The map parameter (the name map that is) seems something special
// to MSVC and so we use a other name.
void wxClassInfo::GetProperties( wxPropertyInfoMap &infomap ) const
{
    const wxPropertyInfo *pi = GetFirstProperty() ;
    while( pi )
    {
        if ( infomap.find( pi->GetName() ) == infomap.end() )
            infomap[pi->GetName()] = (wxPropertyInfo*) pi ;

        pi = pi->GetNext() ;
    }

    const wxClassInfo** parents = GetParents() ;
    for ( int i = 0 ; parents[i] ; ++ i )
    {
        parents[i]->GetProperties( infomap ) ;
    }
}

/*
VARIANT TO OBJECT
*/

wxObject* wxxVariant::GetAsObject()
{
    const wxClassTypeInfo *ti = dynamic_cast<const wxClassTypeInfo*>( m_data->GetTypeInfo() ) ;
    if ( ti )
        return ti->GetClassInfo()->VariantToInstance(*this) ;
    else
        return NULL ;
}

// ----------------------------------------------------------------------------
// wxDynamicObject support
// ----------------------------------------------------------------------------
//
// Dynamic Objects are objects that have a real superclass instance and carry their
// own attributes in a hash map. Like this it is possible to create the objects and
// stream them, as if their class information was already available from compiled data

struct wxDynamicObject::wxDynamicObjectInternal
{
    wxDynamicObjectInternal() {}

#if wxUSE_UNICODE
    map<wstring,wxxVariant> m_properties ;
#else
    map<string,wxxVariant> m_properties ;
#endif
} ;

typedef list< wxDynamicObject* > wxDynamicObjectList ;

struct wxDynamicClassInfo::wxDynamicClassInfoInternal
{
    wxDynamicObjectList m_dynamicObjects ;
} ;

// instantiates this object with an instance of its superclass
wxDynamicObject::wxDynamicObject(wxObject* superClassInstance, const wxDynamicClassInfo *info)
{
    m_superClassInstance = superClassInstance ;
    m_classInfo = info ;
    m_data = new wxDynamicObjectInternal ;
}

wxDynamicObject::~wxDynamicObject()
{
    dynamic_cast<const wxDynamicClassInfo*>(m_classInfo)->m_data->m_dynamicObjects.remove( this ) ;
    delete m_data ;
    delete m_superClassInstance ;
}

void wxDynamicObject::SetProperty (const wxChar *propertyName, const wxxVariant &value)
{
    wxASSERT_MSG(m_classInfo->FindPropertyInfoInThisClass(propertyName),wxT("Accessing Unknown Property in a Dynamic Object") ) ;
    m_data->m_properties[propertyName] = value ;
}

wxxVariant wxDynamicObject::GetProperty (const wxChar *propertyName) const
{
    wxASSERT_MSG(m_classInfo->FindPropertyInfoInThisClass(propertyName),wxT("Accessing Unknown Property in a Dynamic Object") ) ;
    return m_data->m_properties[propertyName] ;
}

void wxDynamicObject::RemoveProperty( const wxChar *propertyName )
{
    wxASSERT_MSG(m_classInfo->FindPropertyInfoInThisClass(propertyName),wxT("Removing Unknown Property in a Dynamic Object") ) ;
    m_data->m_properties.erase( propertyName ) ;
}

void wxDynamicObject::RenameProperty( const wxChar *oldPropertyName , const wxChar *newPropertyName )
{
    wxASSERT_MSG(m_classInfo->FindPropertyInfoInThisClass(oldPropertyName),wxT("Renaming Unknown Property in a Dynamic Object") ) ;
    wxxVariant value = m_data->m_properties[oldPropertyName] ;
    m_data->m_properties.erase( oldPropertyName ) ;
    m_data->m_properties[newPropertyName] = value ;
}


// ----------------------------------------------------------------------------
// wxDynamiClassInfo
// ----------------------------------------------------------------------------

wxDynamicClassInfo::wxDynamicClassInfo( const wxChar *unitName, const wxChar *className , const wxClassInfo* superClass ) :
wxClassInfo( unitName, className , new const wxClassInfo*[2])
{
    GetParents()[0] = superClass ;
    GetParents()[1] = NULL ;
    m_data = new wxDynamicClassInfoInternal ;
}

wxDynamicClassInfo::~wxDynamicClassInfo()
{
    delete[] GetParents() ;
    delete m_data ;
}

wxObject *wxDynamicClassInfo::AllocateObject() const
{
    wxObject* parent = GetParents()[0]->AllocateObject() ;
    wxDynamicObject *obj = new wxDynamicObject( parent , this ) ;
    m_data->m_dynamicObjects.push_back( obj ) ;
    return obj ;
}

void wxDynamicClassInfo::Create (wxObject *object, int paramCount, wxxVariant *params) const
{
    wxDynamicObject *dynobj = dynamic_cast< wxDynamicObject *>( object ) ;
    wxASSERT_MSG( dynobj , wxT("cannot call wxDynamicClassInfo::Create on an object other than wxDynamicObject") ) ;
    GetParents()[0]->Create( dynobj->GetSuperClassInstance() , paramCount , params ) ;
}

// get number of parameters for constructor
int wxDynamicClassInfo::GetCreateParamCount() const
{
    return GetParents()[0]->GetCreateParamCount() ;
}

// get i-th constructor parameter
const wxChar* wxDynamicClassInfo::GetCreateParamName(int i) const
{
    return GetParents()[0]->GetCreateParamName( i ) ;
}

void wxDynamicClassInfo::SetProperty(wxObject *object, const wxChar *propertyName, const wxxVariant &value) const
{
    wxDynamicObject* dynobj = dynamic_cast< wxDynamicObject * >( object ) ;
    wxASSERT_MSG( dynobj , wxT("cannot call wxDynamicClassInfo::SetProperty on an object other than wxDynamicObject") ) ;
    if ( FindPropertyInfoInThisClass(propertyName) )
        dynobj->SetProperty( propertyName , value ) ;
    else
        GetParents()[0]->SetProperty( dynobj->GetSuperClassInstance() , propertyName , value ) ;
}

wxxVariant wxDynamicClassInfo::GetProperty(wxObject *object, const wxChar *propertyName) const
{
    wxDynamicObject* dynobj = dynamic_cast< wxDynamicObject * >( object ) ;
    wxASSERT_MSG( dynobj , wxT("cannot call wxDynamicClassInfo::SetProperty on an object other than wxDynamicObject") ) ;
    if ( FindPropertyInfoInThisClass(propertyName) )
        return dynobj->GetProperty( propertyName ) ;
    else
        return GetParents()[0]->GetProperty( dynobj->GetSuperClassInstance() , propertyName ) ;
}

void wxDynamicClassInfo::AddProperty( const wxChar *propertyName , const wxTypeInfo* typeInfo )
{
    new wxPropertyInfo( m_firstProperty , this , propertyName , typeInfo->GetTypeName() , new wxGenericPropertyAccessor( propertyName ) , wxxVariant() ) ;
}

void wxDynamicClassInfo::AddHandler( const wxChar *handlerName , wxObjectEventFunction address , const wxClassInfo* eventClassInfo )
{
    new wxHandlerInfo( m_firstHandler , this , handlerName , address , eventClassInfo ) ;
}

// removes an existing runtime-property
void wxDynamicClassInfo::RemoveProperty( const wxChar *propertyName )
{
    for ( wxDynamicObjectList::iterator iter = m_data->m_dynamicObjects.begin() ; iter != m_data->m_dynamicObjects.end() ; ++iter )
        (*iter)->RemoveProperty( propertyName ) ;
    delete FindPropertyInfoInThisClass(propertyName) ;
}

// removes an existing runtime-handler
void wxDynamicClassInfo::RemoveHandler( const wxChar *handlerName )
{
    delete FindHandlerInfoInThisClass(handlerName) ;
}

// renames an existing runtime-property
void wxDynamicClassInfo::RenameProperty( const wxChar *oldPropertyName , const wxChar *newPropertyName )
{
    wxPropertyInfo* pi = FindPropertyInfoInThisClass(oldPropertyName) ;
    wxASSERT_MSG( pi ,wxT("not existing property") ) ;
    pi->m_name = newPropertyName ;
    dynamic_cast<wxGenericPropertyAccessor*>(pi->GetAccessor())->RenameProperty( oldPropertyName , newPropertyName ) ;
    for ( wxDynamicObjectList::iterator iter = m_data->m_dynamicObjects.begin() ; iter != m_data->m_dynamicObjects.end() ; ++iter )
        (*iter)->RenameProperty( oldPropertyName , newPropertyName ) ;
}

// renames an existing runtime-handler
void wxDynamicClassInfo::RenameHandler( const wxChar *oldHandlerName , const wxChar *newHandlerName )
{
    wxASSERT_MSG(FindHandlerInfoInThisClass(oldHandlerName),wxT("not existing handler") ) ;
    FindHandlerInfoInThisClass(oldHandlerName)->m_name = newHandlerName ;
}

// ----------------------------------------------------------------------------
// wxGenericPropertyAccessor
// ----------------------------------------------------------------------------

struct wxGenericPropertyAccessor::wxGenericPropertyAccessorInternal
{
    char filler ;
} ;

wxGenericPropertyAccessor::wxGenericPropertyAccessor( const wxString& propertyName )
: wxPropertyAccessor( NULL , NULL , NULL , NULL )
{
    m_data = new wxGenericPropertyAccessorInternal ;
    m_propertyName = propertyName ;
    m_getterName = wxT("Get")+propertyName ;
    m_setterName = wxT("Set")+propertyName ;
}

wxGenericPropertyAccessor::~wxGenericPropertyAccessor()
{
    delete m_data ;
}
void wxGenericPropertyAccessor::SetProperty(wxObject *object, const wxxVariant &value) const
{
    wxDynamicObject* dynobj = dynamic_cast< wxDynamicObject * >( object ) ;
    wxASSERT_MSG( dynobj , wxT("cannot call wxDynamicClassInfo::SetProperty on an object other than wxDynamicObject") ) ;
    dynobj->SetProperty(m_propertyName , value ) ;
}

void wxGenericPropertyAccessor::GetProperty(const wxObject *object, wxxVariant& value) const
{
    const wxDynamicObject* dynobj = dynamic_cast< const wxDynamicObject * >( object ) ;
    wxASSERT_MSG( dynobj , wxT("cannot call wxDynamicClassInfo::SetProperty on an object other than wxDynamicObject") ) ;
    value = dynobj->GetProperty( m_propertyName ) ;
}

#endif // wxUSE_EXTENDED_RTTI
