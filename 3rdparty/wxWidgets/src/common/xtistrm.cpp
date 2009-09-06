/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/xtistrm.cpp
// Purpose:     streaming runtime metadata information
// Author:      Stefan Csomor
// Modified by:
// Created:     27/07/03
// RCS-ID:      $Id: xtistrm.cpp 38939 2006-04-27 12:47:14Z ABX $
// Copyright:   (c) 2003 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_EXTENDED_RTTI

#include "wx/xtistrm.h"

#ifndef WX_PRECOMP
    #include "wx/object.h"
    #include "wx/hash.h"
    #include "wx/event.h"
#endif

#include "wx/tokenzr.h"
#include "wx/txtstrm.h"

#include "wx/beforestd.h"
#include <map>
#include <vector>
#include <string>
#include "wx/afterstd.h"

using namespace std ;

struct wxWriter::wxWriterInternal
{
    map< const wxObject* , int > m_writtenObjects ;
    int m_nextId ;
} ;

wxWriter::wxWriter()
{
    m_data = new wxWriterInternal ;
    m_data->m_nextId = 0 ;
}

wxWriter::~wxWriter()
{
    delete m_data ;
}

struct wxWriter::wxWriterInternalPropertiesData
{
    char nothing ;
} ;

void wxWriter::ClearObjectContext()
{
    delete m_data ;
    m_data = new wxWriterInternal() ;
    m_data->m_nextId = 0 ;
}

void wxWriter::WriteObject(const wxObject *object, const wxClassInfo *classInfo , wxPersister *persister , const wxString &name , wxxVariantArray &metadata )
{
    DoBeginWriteTopLevelEntry( name ) ;
    WriteObject( object , classInfo , persister , false , metadata) ;
    DoEndWriteTopLevelEntry( name ) ;
}

void wxWriter::WriteObject(const wxObject *object, const wxClassInfo *classInfo , wxPersister *persister , bool isEmbedded, wxxVariantArray &metadata )
{
    if ( !classInfo->BeforeWriteObject( object , this , persister , metadata) )
        return ;

    if ( persister->BeforeWriteObject( this , object , classInfo , metadata) )
    {
        if ( object == NULL )
            DoWriteNullObject() ;
        else if ( IsObjectKnown( object ) )
            DoWriteRepeatedObject( GetObjectID(object) ) ;
        else
        {
            int oid = m_data->m_nextId++ ;
            if ( !isEmbedded )
                m_data->m_writtenObjects[object] = oid ;

            // in case this object is a wxDynamicObject we also have to insert is superclass
            // instance with the same id, so that object relations are streamed out correctly
            const wxDynamicObject* dynobj = dynamic_cast<const wxDynamicObject *>( object ) ;
            if ( !isEmbedded && dynobj )
                m_data->m_writtenObjects[dynobj->GetSuperClassInstance()] = oid ;

            DoBeginWriteObject( object , classInfo , oid , metadata ) ;
            wxWriterInternalPropertiesData data ;
            WriteAllProperties( object , classInfo , persister , &data ) ;
            DoEndWriteObject( object , classInfo , oid  ) ;
        }
        persister->AfterWriteObject( this ,object , classInfo ) ;
    }
}

void wxWriter::FindConnectEntry(const wxEvtHandler * evSource,const wxDelegateTypeInfo* dti, const wxObject* &sink , const wxHandlerInfo *&handler)
{
    wxList *dynamicEvents = evSource->GetDynamicEventTable() ;

    if ( dynamicEvents )
    {
        wxList::compatibility_iterator node = dynamicEvents->GetFirst();
        while (node)
        {
            wxDynamicEventTableEntry *entry = (wxDynamicEventTableEntry*)node->GetData();

            // find the match
            if ( entry->m_fn &&
                (dti->GetEventType() == entry->m_eventType) &&
                (entry->m_id == -1 ) &&
                (entry->m_eventSink != NULL ) )
            {
                sink = entry->m_eventSink ;
                const wxClassInfo* sinkClassInfo = sink->GetClassInfo() ;
                const wxHandlerInfo* sinkHandler = sinkClassInfo->GetFirstHandler() ;
                while ( sinkHandler )
                {
                    if ( sinkHandler->GetEventFunction() == entry->m_fn )
                    {
                        handler = sinkHandler ;
                        break ;
                    }
                    sinkHandler = sinkHandler->GetNext() ;
                }
                break ;
            }
            node = node->GetNext();
        }
    }
}
void wxWriter::WriteAllProperties( const wxObject * obj , const wxClassInfo* ci , wxPersister *persister, wxWriterInternalPropertiesData * data )
{
    wxPropertyInfoMap map ;
    ci->GetProperties( map ) ;
    for ( int i = 0 ; i < ci->GetCreateParamCount() ; ++i )
    {
        wxString name = ci->GetCreateParamName(i) ;
        const wxPropertyInfo* prop = map.find(name)->second ;
        if ( prop )
        {
            WriteOneProperty( obj , prop->GetDeclaringClass() , prop , persister , data ) ;
        }
        else
        {
            wxLogError( _("Create Parameter not found in declared RTTI Parameters") ) ;
        }
        map.erase( name ) ;
    }
    { // Extra block for broken compilers
        for( wxPropertyInfoMap::iterator iter = map.begin() ; iter != map.end() ; ++iter )
        {
            const wxPropertyInfo* prop = iter->second ;
            if ( prop->GetFlags() & wxPROP_OBJECT_GRAPH )
            {
                WriteOneProperty( obj , prop->GetDeclaringClass() , prop , persister , data ) ;
            }
        }
    }
    { // Extra block for broken compilers
        for( wxPropertyInfoMap::iterator iter = map.begin() ; iter != map.end() ; ++iter )
        {
            const wxPropertyInfo* prop = iter->second ;
            if ( !(prop->GetFlags() & wxPROP_OBJECT_GRAPH) )
            {
                WriteOneProperty( obj , prop->GetDeclaringClass() , prop , persister , data ) ;
            }
        }
    }
}

void wxWriter::WriteOneProperty( const wxObject *obj , const wxClassInfo* ci , const wxPropertyInfo* pi , wxPersister *persister , wxWriterInternalPropertiesData *WXUNUSED(data) )
{
    if ( pi->GetFlags() & wxPROP_DONT_STREAM )
        return ;

    // make sure that we are picking the correct object for accessing the property
    const wxDynamicObject* dynobj = dynamic_cast< const wxDynamicObject* > (obj ) ;
    if ( dynobj && (dynamic_cast<const wxDynamicClassInfo*>(ci) == NULL) )
        obj = dynobj->GetSuperClassInstance() ;

    if ( pi->GetTypeInfo()->GetKind() == wxT_COLLECTION )
    {
        wxxVariantArray data ;
        pi->GetAccessor()->GetPropertyCollection(obj, data) ;
        const wxTypeInfo * elementType = dynamic_cast< const wxCollectionTypeInfo* >( pi->GetTypeInfo() )->GetElementType() ;
        for ( size_t i = 0 ; i < data.GetCount() ; ++i )
        {
            if ( i == 0 )
                DoBeginWriteProperty( pi ) ;

            DoBeginWriteElement() ;
            wxxVariant value = data[i] ;
            if ( persister->BeforeWriteProperty( this , obj, pi , value ) )
            {
                const wxClassTypeInfo* cti = dynamic_cast< const wxClassTypeInfo* > ( elementType ) ;
                if ( cti )
                {
                    const wxClassInfo* pci = cti->GetClassInfo() ;
                    wxObject *vobj = pci->VariantToInstance( value ) ;
                    wxxVariantArray md ;
                    WriteObject( vobj , (vobj ? vobj->GetClassInfo() : pci ) , persister , cti->GetKind()== wxT_OBJECT , md ) ;
                }
                else
                {
                    DoWriteSimpleType( value ) ;
                }
            }
            DoEndWriteElement() ;
            if ( i == data.GetCount() - 1 )
                 DoEndWriteProperty( pi ) ;
       }
   }
    else
    {
        const wxDelegateTypeInfo* dti = dynamic_cast< const wxDelegateTypeInfo* > ( pi->GetTypeInfo() ) ;
        if ( dti )
        {
            const wxObject* sink = NULL ;
            const wxHandlerInfo *handler = NULL ;

            const wxEvtHandler * evSource = dynamic_cast<const wxEvtHandler *>(obj) ;
            if ( evSource )
            {
                FindConnectEntry( evSource , dti , sink , handler ) ;
                if ( persister->BeforeWriteDelegate( this , obj , ci , pi , sink , handler ) )
                {
                    if ( sink != NULL && handler != NULL )
                    {
                        DoBeginWriteProperty( pi ) ;
                        if ( IsObjectKnown( sink ) )
                        {
                            DoWriteDelegate( obj , ci , pi , sink , GetObjectID( sink ) , sink->GetClassInfo() , handler ) ;
                            DoEndWriteProperty( pi ) ;
                        }
                        else
                        {
                            wxLogError( _("Streaming delegates for not already streamed objects not yet supported") ) ;
                        }
                    }
                }
            }
            else
            {
                wxLogError(_("Illegal Object Class (Non-wxEvtHandler) as Event Source") ) ;
            }
        }
        else
        {
            wxxVariant value ;
            pi->GetAccessor()->GetProperty(obj, value) ;

            // avoid streaming out void objects
            if( value.IsEmpty() )
                return ;

            if ( pi->GetFlags() & wxPROP_ENUM_STORE_LONG )
            {
                const wxEnumTypeInfo *eti = dynamic_cast<const wxEnumTypeInfo*>( pi->GetTypeInfo() ) ;
                if ( eti )
                {
                    eti->ConvertFromLong( value.wxTEMPLATED_MEMBER_CALL(Get , long) , value ) ;
                }
                else
                {
                    wxLogError( _("Type must have enum - long conversion") ) ;
                }
            }

            // avoid streaming out default values
            if ( pi->GetTypeInfo()->HasStringConverters() && !pi->GetDefaultValue().IsEmpty() )
            {
                if ( value.GetAsString() == pi->GetDefaultValue().GetAsString() )
                    return ;
            }

            // avoid streaming out null objects
            const wxClassTypeInfo* cti = dynamic_cast< const wxClassTypeInfo* > ( pi->GetTypeInfo() ) ;

            if ( cti && value.GetAsObject() == NULL )
                return ;

            if ( persister->BeforeWriteProperty( this , obj, pi , value ) )
            {
                DoBeginWriteProperty( pi ) ;
                if ( cti )
                {
                    const wxClassInfo* pci = cti->GetClassInfo() ;
                    wxObject *vobj = pci->VariantToInstance( value ) ;
                    if ( vobj && pi->GetTypeInfo()->HasStringConverters() )
                    {
                        wxString stringValue ;
                        cti->ConvertToString( value , stringValue ) ;
                        wxxVariant convertedValue(stringValue) ;
                        DoWriteSimpleType( convertedValue ) ;
                    }
                    else
                    {
                        wxxVariantArray md ;
                        WriteObject( vobj , (vobj ? vobj->GetClassInfo() : pci ) , persister , cti->GetKind()== wxT_OBJECT , md) ;
                    }
                }
                else
                {
                    DoWriteSimpleType( value ) ;
                }
                DoEndWriteProperty( pi ) ;
            }
        }
    }
}

int wxWriter::GetObjectID(const wxObject *obj)
{
    if ( !IsObjectKnown( obj ) )
        return wxInvalidObjectID ;

    return m_data->m_writtenObjects[obj] ;
}

bool wxWriter::IsObjectKnown( const wxObject *obj )
{
    return m_data->m_writtenObjects.find( obj ) != m_data->m_writtenObjects.end() ;
}


// ----------------------------------------------------------------------------
// reading objects in
// ----------------------------------------------------------------------------

struct wxReader::wxReaderInternal
{
    map<int,wxClassInfo*> m_classInfos;
};

wxReader::wxReader()
{
    m_data = new wxReaderInternal;
}

wxReader::~wxReader()
{
    delete m_data;
}

wxClassInfo* wxReader::GetObjectClassInfo(int objectID)
{
    if ( objectID == wxNullObjectID || objectID == wxInvalidObjectID )
    {
        wxLogError( _("Invalid or Null Object ID passed to GetObjectClassInfo" ) ) ;
        return NULL ;
    }
    if ( m_data->m_classInfos.find(objectID) == m_data->m_classInfos.end() )
    {
        wxLogError( _("Unknown Object passed to GetObjectClassInfo" ) ) ;
        return NULL ;
    }
    return m_data->m_classInfos[objectID] ;
}

void wxReader::SetObjectClassInfo(int objectID, wxClassInfo *classInfo )
{
    if ( objectID == wxNullObjectID || objectID == wxInvalidObjectID )
    {
        wxLogError( _("Invalid or Null Object ID passed to GetObjectClassInfo" ) ) ;
        return ;
    }
    if ( m_data->m_classInfos.find(objectID) != m_data->m_classInfos.end() )
    {
        wxLogError( _("Already Registered Object passed to SetObjectClassInfo" ) ) ;
        return ;
    }
    m_data->m_classInfos[objectID] = classInfo ;
}

bool wxReader::HasObjectClassInfo( int objectID )
{
    if ( objectID == wxNullObjectID || objectID == wxInvalidObjectID )
    {
        wxLogError( _("Invalid or Null Object ID passed to HasObjectClassInfo" ) ) ;
        return NULL ;
    }
    return m_data->m_classInfos.find(objectID) != m_data->m_classInfos.end() ;
}


// ----------------------------------------------------------------------------
// reading xml in
// ----------------------------------------------------------------------------

/*
Reading components has not to be extended for components
as properties are always sought by typeinfo over all levels
and create params are always toplevel class only
*/


// ----------------------------------------------------------------------------
// depersisting to memory
// ----------------------------------------------------------------------------

struct wxRuntimeDepersister::wxRuntimeDepersisterInternal
{
    map<int,wxObject *> m_objects;

    void SetObject(int objectID, wxObject *obj )
    {
        if ( m_objects.find(objectID) != m_objects.end() )
        {
            wxLogError( _("Passing a already registered object to SetObject") ) ;
            return  ;
        }
        m_objects[objectID] = obj ;
    }
    wxObject* GetObject( int objectID )
    {
        if ( objectID == wxNullObjectID )
            return NULL ;
        if ( m_objects.find(objectID) == m_objects.end() )
        {
            wxLogError( _("Passing an unkown object to GetObject") ) ;
            return NULL ;
        }

        return m_objects[objectID] ;
    }
} ;

wxRuntimeDepersister::wxRuntimeDepersister()
{
    m_data = new wxRuntimeDepersisterInternal() ;
}

wxRuntimeDepersister::~wxRuntimeDepersister()
{
    delete m_data ;
}

void wxRuntimeDepersister::AllocateObject(int objectID, wxClassInfo *classInfo ,
                                          wxxVariantArray &WXUNUSED(metadata))
{
    wxObject *O;
    O = classInfo->CreateObject();
    m_data->SetObject(objectID, O);
}

void wxRuntimeDepersister::CreateObject(int objectID,
                                        const wxClassInfo *classInfo,
                                        int paramCount,
                                        wxxVariant *params,
                                        int *objectIdValues,
                                        const wxClassInfo **objectClassInfos ,
                                        wxxVariantArray &WXUNUSED(metadata))
{
    wxObject *o;
    o = m_data->GetObject(objectID);
    for ( int i = 0 ; i < paramCount ; ++i )
    {
        if ( objectIdValues[i] != wxInvalidObjectID )
        {
            wxObject *o;
            o = m_data->GetObject(objectIdValues[i]);
            // if this is a dynamic object and we are asked for another class
            // than wxDynamicObject we cast it down manually.
            wxDynamicObject *dyno = dynamic_cast< wxDynamicObject * > (o) ;
            if ( dyno!=NULL && (objectClassInfos[i] != dyno->GetClassInfo()) )
            {
                o = dyno->GetSuperClassInstance() ;
            }
            params[i] = objectClassInfos[i]->InstanceToVariant(o) ;
        }
    }
    classInfo->Create(o, paramCount, params);
}

void wxRuntimeDepersister::ConstructObject(int objectID,
                                        const wxClassInfo *classInfo,
                                        int paramCount,
                                        wxxVariant *params,
                                        int *objectIdValues,
                                        const wxClassInfo **objectClassInfos ,
                                        wxxVariantArray &WXUNUSED(metadata))
{
    wxObject *o;
    for ( int i = 0 ; i < paramCount ; ++i )
    {
        if ( objectIdValues[i] != wxInvalidObjectID )
        {
            wxObject *o;
            o = m_data->GetObject(objectIdValues[i]);
            // if this is a dynamic object and we are asked for another class
            // than wxDynamicObject we cast it down manually.
            wxDynamicObject *dyno = dynamic_cast< wxDynamicObject * > (o) ;
            if ( dyno!=NULL && (objectClassInfos[i] != dyno->GetClassInfo()) )
            {
                o = dyno->GetSuperClassInstance() ;
            }
            params[i] = objectClassInfos[i]->InstanceToVariant(o) ;
        }
    }
    o = classInfo->ConstructObject(paramCount, params);
    m_data->SetObject(objectID, o);
}


void wxRuntimeDepersister::DestroyObject(int objectID, wxClassInfo *WXUNUSED(classInfo))
{
    wxObject *o;
    o = m_data->GetObject(objectID);
    delete o ;
}

void wxRuntimeDepersister::SetProperty(int objectID,
                                       const wxClassInfo *classInfo,
                                       const wxPropertyInfo* propertyInfo,
                                       const wxxVariant &value)
{
    wxObject *o;
    o = m_data->GetObject(objectID);
    classInfo->SetProperty( o , propertyInfo->GetName() , value ) ;
}

void wxRuntimeDepersister::SetPropertyAsObject(int objectID,
                                               const wxClassInfo *classInfo,
                                               const wxPropertyInfo* propertyInfo,
                                               int valueObjectId)
{
    wxObject *o, *valo;
    o = m_data->GetObject(objectID);
    valo = m_data->GetObject(valueObjectId);
    const wxClassInfo* valClassInfo = (dynamic_cast<const wxClassTypeInfo*>(propertyInfo->GetTypeInfo()))->GetClassInfo() ;
    // if this is a dynamic object and we are asked for another class
    // than wxDynamicObject we cast it down manually.
    wxDynamicObject *dynvalo = dynamic_cast< wxDynamicObject * > (valo) ;
    if ( dynvalo!=NULL  && (valClassInfo != dynvalo->GetClassInfo()) )
    {
        valo = dynvalo->GetSuperClassInstance() ;
    }

    classInfo->SetProperty( o , propertyInfo->GetName() , valClassInfo->InstanceToVariant(valo) ) ;
}

void wxRuntimeDepersister::SetConnect(int eventSourceObjectID,
                                      const wxClassInfo *WXUNUSED(eventSourceClassInfo),
                                      const wxPropertyInfo *delegateInfo ,
                                      const wxClassInfo *WXUNUSED(eventSinkClassInfo) ,
                                      const wxHandlerInfo* handlerInfo ,
                                      int eventSinkObjectID )
{
    wxEvtHandler *ehsource = dynamic_cast< wxEvtHandler* >( m_data->GetObject( eventSourceObjectID ) ) ;
    wxEvtHandler *ehsink = dynamic_cast< wxEvtHandler *>(m_data->GetObject(eventSinkObjectID) ) ;

    if ( ehsource && ehsink )
    {
        const wxDelegateTypeInfo *delegateTypeInfo = dynamic_cast<const wxDelegateTypeInfo*>(delegateInfo->GetTypeInfo());
        if( delegateTypeInfo && delegateTypeInfo->GetLastEventType() == -1 )
        {
            ehsource->Connect( -1 , delegateTypeInfo->GetEventType() ,
                handlerInfo->GetEventFunction() , NULL /*user data*/ ,
                ehsink ) ;
        }
        else
        {
            for ( wxEventType iter = delegateTypeInfo->GetEventType() ; iter <= delegateTypeInfo->GetLastEventType() ; ++iter )
            {
                ehsource->Connect( -1 , iter ,
                    handlerInfo->GetEventFunction() , NULL /*user data*/ ,
                    ehsink ) ;
            }
        }
    }
}

wxObject *wxRuntimeDepersister::GetObject(int objectID)
{
    return m_data->GetObject( objectID ) ;
}

// adds an element to a property collection
void wxRuntimeDepersister::AddToPropertyCollection( int objectID ,
                                                   const wxClassInfo *classInfo,
                                                   const wxPropertyInfo* propertyInfo ,
                                                   const wxxVariant &value)
{
    wxObject *o;
    o = m_data->GetObject(objectID);
    classInfo->AddToPropertyCollection( o , propertyInfo->GetName() , value ) ;
}

// sets the corresponding property (value is an object)
void wxRuntimeDepersister::AddToPropertyCollectionAsObject(int objectID,
                                                           const wxClassInfo *classInfo,
                                                           const wxPropertyInfo* propertyInfo ,
                                                           int valueObjectId)
{
    wxObject *o, *valo;
    o = m_data->GetObject(objectID);
    valo = m_data->GetObject(valueObjectId);
    const wxCollectionTypeInfo * collectionTypeInfo = dynamic_cast< const wxCollectionTypeInfo * >(propertyInfo->GetTypeInfo() ) ;
    const wxClassInfo* valClassInfo = (dynamic_cast<const wxClassTypeInfo*>(collectionTypeInfo->GetElementType()))->GetClassInfo() ;
    // if this is a dynamic object and we are asked for another class
    // than wxDynamicObject we cast it down manually.
    wxDynamicObject *dynvalo = dynamic_cast< wxDynamicObject * > (valo) ;
    if ( dynvalo!=NULL  && (valClassInfo != dynvalo->GetClassInfo()) )
    {
        valo = dynvalo->GetSuperClassInstance() ;
    }

    classInfo->AddToPropertyCollection( o , propertyInfo->GetName() , valClassInfo->InstanceToVariant(valo) ) ;
}

// ----------------------------------------------------------------------------
// depersisting to code
// ----------------------------------------------------------------------------

struct wxCodeDepersister::wxCodeDepersisterInternal
{
#if wxUSE_UNICODE
    map<int,wstring> m_objectNames ;
#else
    map<int,string> m_objectNames ;
#endif

    void SetObjectName(int objectID, const wxString &name )
    {
        if ( m_objectNames.find(objectID) != m_objectNames.end() )
        {
            wxLogError( _("Passing a already registered object to SetObjectName") ) ;
            return  ;
        }
        m_objectNames[objectID] = (const wxChar *)name;
    }

    wxString GetObjectName( int objectID )
    {
        if ( objectID == wxNullObjectID )
            return wxT("NULL") ;

        if ( m_objectNames.find(objectID) == m_objectNames.end() )
        {
            wxLogError( _("Passing an unkown object to GetObject") ) ;
            return wxEmptyString ;
        }
        return wxString( m_objectNames[objectID].c_str() ) ;
    }
} ;

wxCodeDepersister::wxCodeDepersister(wxTextOutputStream *out)
: m_fp(out)
{
    m_data = new wxCodeDepersisterInternal ;
}

wxCodeDepersister::~wxCodeDepersister()
{
    delete m_data ;
}

void wxCodeDepersister::AllocateObject(int objectID, wxClassInfo *classInfo ,
                                       wxxVariantArray &WXUNUSED(metadata))
{
    wxString objectName = wxString::Format( wxT("LocalObject_%d") , objectID ) ;
    m_fp->WriteString( wxString::Format( wxT("\t%s *%s = new %s;\n"),
        classInfo->GetClassName(),
        objectName.c_str(),
        classInfo->GetClassName()) );
    m_data->SetObjectName( objectID , objectName ) ;
}

void wxCodeDepersister::DestroyObject(int objectID, wxClassInfo *WXUNUSED(classInfo))
{
    m_fp->WriteString( wxString::Format( wxT("\tdelete %s;\n"),
        m_data->GetObjectName( objectID).c_str() ) );
}

wxString wxCodeDepersister::ValueAsCode( const wxxVariant &param )
{
    wxString value ;
    const wxTypeInfo* type = param.GetTypeInfo() ;
    if ( type->GetKind() == wxT_CUSTOM )
    {
        const wxCustomTypeInfo* cti = dynamic_cast<const wxCustomTypeInfo*>(type) ;
        if ( cti )
        {
            value.Printf( wxT("%s(%s)"), cti->GetTypeName().c_str(),param.GetAsString().c_str() );
        }
        else
        {
            wxLogError ( _("Internal error, illegal wxCustomTypeInfo") ) ;
        }
    }
    else if ( type->GetKind() == wxT_STRING )
    {
        value.Printf( wxT("\"%s\""),param.GetAsString().c_str() );
    }
    else
    {
        value.Printf( wxT("%s"), param.GetAsString().c_str() );
    }
    return value ;
}

void wxCodeDepersister::CreateObject(int objectID,
                                     const wxClassInfo *WXUNUSED(classInfo),
                                     int paramCount,
                                     wxxVariant *params,
                                     int *objectIDValues,
                                     const wxClassInfo **WXUNUSED(objectClassInfos) ,
                                     wxxVariantArray &WXUNUSED(metadata)
                                     )
{
    int i;
    m_fp->WriteString( wxString::Format( wxT("\t%s->Create("), m_data->GetObjectName(objectID).c_str() ) );
    for (i = 0; i < paramCount; i++)
    {
        if ( objectIDValues[i] != wxInvalidObjectID )
            m_fp->WriteString( wxString::Format( wxT("%s"), m_data->GetObjectName( objectIDValues[i] ).c_str() ) );
        else
        {
            m_fp->WriteString( wxString::Format( wxT("%s"), ValueAsCode(params[i]).c_str() ) );
        }
        if (i < paramCount - 1)
            m_fp->WriteString( wxT(", "));
    }
    m_fp->WriteString( wxT(");\n") );
}

void wxCodeDepersister::ConstructObject(int objectID,
                                     const wxClassInfo *classInfo,
                                     int paramCount,
                                     wxxVariant *params,
                                     int *objectIDValues,
                                     const wxClassInfo **WXUNUSED(objectClassInfos) ,
                                     wxxVariantArray &WXUNUSED(metadata)
                                     )
{
    wxString objectName = wxString::Format( wxT("LocalObject_%d") , objectID ) ;
    m_fp->WriteString( wxString::Format( wxT("\t%s *%s = new %s("),
        classInfo->GetClassName(),
        objectName.c_str(),
        classInfo->GetClassName()) );
    m_data->SetObjectName( objectID , objectName ) ;

    int i;
    for (i = 0; i < paramCount; i++)
    {
        if ( objectIDValues[i] != wxInvalidObjectID )
            m_fp->WriteString( wxString::Format( wxT("%s"), m_data->GetObjectName( objectIDValues[i] ).c_str() ) );
        else
        {
            m_fp->WriteString( wxString::Format( wxT("%s"), ValueAsCode(params[i]).c_str() ) );
        }
        if (i < paramCount - 1)
            m_fp->WriteString( wxT(", ") );
    }
    m_fp->WriteString( wxT(");\n") );
}

void wxCodeDepersister::SetProperty(int objectID,
                                    const wxClassInfo *WXUNUSED(classInfo),
                                    const wxPropertyInfo* propertyInfo,
                                    const wxxVariant &value)
{
    m_fp->WriteString( wxString::Format( wxT("\t%s->%s(%s);\n"),
        m_data->GetObjectName(objectID).c_str(),
        propertyInfo->GetAccessor()->GetSetterName().c_str(),
        ValueAsCode(value).c_str()) );
}

void wxCodeDepersister::SetPropertyAsObject(int objectID,
                                            const wxClassInfo *WXUNUSED(classInfo),
                                            const wxPropertyInfo* propertyInfo,
                                            int valueObjectId)
{
    if ( propertyInfo->GetTypeInfo()->GetKind() == wxT_OBJECT )
        m_fp->WriteString( wxString::Format( wxT("\t%s->%s(*%s);\n"),
        m_data->GetObjectName(objectID).c_str(),
        propertyInfo->GetAccessor()->GetSetterName().c_str(),
        m_data->GetObjectName( valueObjectId).c_str() ) );
    else
        m_fp->WriteString( wxString::Format( wxT("\t%s->%s(%s);\n"),
        m_data->GetObjectName(objectID).c_str(),
        propertyInfo->GetAccessor()->GetSetterName().c_str(),
        m_data->GetObjectName( valueObjectId).c_str() ) );
}

void wxCodeDepersister::AddToPropertyCollection( int objectID ,
                                                const wxClassInfo *WXUNUSED(classInfo),
                                                const wxPropertyInfo* propertyInfo ,
                                                const wxxVariant &value)
{
    m_fp->WriteString( wxString::Format( wxT("\t%s->%s(%s);\n"),
        m_data->GetObjectName(objectID).c_str(),
        propertyInfo->GetAccessor()->GetAdderName().c_str(),
        ValueAsCode(value).c_str()) );
}

// sets the corresponding property (value is an object)
void wxCodeDepersister::AddToPropertyCollectionAsObject(int WXUNUSED(objectID),
                                                        const wxClassInfo *WXUNUSED(classInfo),
                                                        const wxPropertyInfo* WXUNUSED(propertyInfo) ,
                                                        int WXUNUSED(valueObjectId))
{
    // TODO
}

void wxCodeDepersister::SetConnect(int eventSourceObjectID,
                                   const wxClassInfo *WXUNUSED(eventSourceClassInfo),
                                   const wxPropertyInfo *delegateInfo ,
                                   const wxClassInfo *eventSinkClassInfo ,
                                   const wxHandlerInfo* handlerInfo ,
                                   int eventSinkObjectID )
{
    wxString ehsource = m_data->GetObjectName( eventSourceObjectID ) ;
    wxString ehsink = m_data->GetObjectName(eventSinkObjectID) ;
    wxString ehsinkClass = eventSinkClassInfo->GetClassName() ;
    const wxDelegateTypeInfo *delegateTypeInfo = dynamic_cast<const wxDelegateTypeInfo*>(delegateInfo->GetTypeInfo());
    if ( delegateTypeInfo )
    {
        int eventType = delegateTypeInfo->GetEventType() ;
        wxString handlerName = handlerInfo->GetName() ;

        m_fp->WriteString( wxString::Format(  wxT("\t%s->Connect( %s->GetId() , %d , (wxObjectEventFunction)(wxEventFunction) & %s::%s , NULL , %s ) ;") ,
            ehsource.c_str() , ehsource.c_str() , eventType , ehsinkClass.c_str() , handlerName.c_str() , ehsink.c_str() ) );
    }
    else
    {
        wxLogError(_("delegate has no type info"));
    }
}

#include "wx/arrimpl.cpp"

WX_DEFINE_OBJARRAY(wxxVariantArray);

#endif // wxUSE_EXTENDED_RTTI
