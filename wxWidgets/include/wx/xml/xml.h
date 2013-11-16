/////////////////////////////////////////////////////////////////////////////
// Name:        xml.h
// Purpose:     wxXmlDocument - XML parser & data holder class
// Author:      Vaclav Slavik
// Created:     2000/03/05
// RCS-ID:      $Id: xml.h 59768 2009-03-23 12:35:12Z VZ $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#ifndef _WX_XML_H_
#define _WX_XML_H_

#include "wx/defs.h"

#if wxUSE_XML

#include "wx/string.h"
#include "wx/object.h"
#include "wx/list.h"

#ifdef WXMAKINGDLL_XML
    #define WXDLLIMPEXP_XML WXEXPORT
#elif defined(WXUSINGDLL)
    #define WXDLLIMPEXP_XML WXIMPORT
#else // not making nor using DLL
    #define WXDLLIMPEXP_XML
#endif

class WXDLLIMPEXP_FWD_XML wxXmlNode;
class WXDLLIMPEXP_FWD_XML wxXmlProperty;
class WXDLLIMPEXP_FWD_XML wxXmlDocument;
class WXDLLIMPEXP_FWD_XML wxXmlIOHandler;
class WXDLLIMPEXP_FWD_BASE wxInputStream;
class WXDLLIMPEXP_FWD_BASE wxOutputStream;


// Represents XML node type.
enum wxXmlNodeType
{
    // note: values are synchronized with xmlElementType from libxml
    wxXML_ELEMENT_NODE       =  1,
    wxXML_ATTRIBUTE_NODE     =  2,
    wxXML_TEXT_NODE          =  3,
    wxXML_CDATA_SECTION_NODE =  4,
    wxXML_ENTITY_REF_NODE    =  5,
    wxXML_ENTITY_NODE        =  6,
    wxXML_PI_NODE            =  7,
    wxXML_COMMENT_NODE       =  8,
    wxXML_DOCUMENT_NODE      =  9,
    wxXML_DOCUMENT_TYPE_NODE = 10,
    wxXML_DOCUMENT_FRAG_NODE = 11,
    wxXML_NOTATION_NODE      = 12,
    wxXML_HTML_DOCUMENT_NODE = 13
};


// Represents node property(ies).
// Example: in <img src="hello.gif" id="3"/> "src" is property with value
//          "hello.gif" and "id" is prop. with value "3".

class WXDLLIMPEXP_XML wxXmlProperty
{
public:
    wxXmlProperty() : m_next(NULL) {}
    wxXmlProperty(const wxString& name, const wxString& value,
                  wxXmlProperty *next = NULL)
            : m_name(name), m_value(value), m_next(next) {}
    virtual ~wxXmlProperty() {}

    wxString GetName() const { return m_name; }
    wxString GetValue() const { return m_value; }
    wxXmlProperty *GetNext() const { return m_next; }

    void SetName(const wxString& name) { m_name = name; }
    void SetValue(const wxString& value) { m_value = value; }
    void SetNext(wxXmlProperty *next) { m_next = next; }

private:
    wxString m_name;
    wxString m_value;
    wxXmlProperty *m_next;
};



// Represents node in XML document. Node has name and may have content
// and properties. Most common node types are wxXML_TEXT_NODE (name and props
// are irrelevant) and wxXML_ELEMENT_NODE (e.g. in <title>hi</title> there is
// element with name="title", irrelevant content and one child (wxXML_TEXT_NODE
// with content="hi").
//
// If wxUSE_UNICODE is 0, all strings are encoded in the encoding given to Load
// (default is UTF-8).

class WXDLLIMPEXP_XML wxXmlNode
{
public:
    wxXmlNode() : m_properties(NULL), m_parent(NULL),
                  m_children(NULL), m_next(NULL) {}
    wxXmlNode(wxXmlNode *parent, wxXmlNodeType type,
              const wxString& name, const wxString& content = wxEmptyString,
              wxXmlProperty *props = NULL, wxXmlNode *next = NULL);
    virtual ~wxXmlNode();

    // copy ctor & operator=. Note that this does NOT copy syblings
    // and parent pointer, i.e. m_parent and m_next will be NULL
    // after using copy ctor and are never unmodified by operator=.
    // On the other hand, it DOES copy children and properties.
    wxXmlNode(const wxXmlNode& node);
    wxXmlNode& operator=(const wxXmlNode& node);

    // user-friendly creation:
    wxXmlNode(wxXmlNodeType type, const wxString& name,
              const wxString& content = wxEmptyString);
    virtual void AddChild(wxXmlNode *child);
    virtual bool InsertChild(wxXmlNode *child, wxXmlNode *followingNode);
#if wxABI_VERSION >= 20808
    bool InsertChildAfter(wxXmlNode *child, wxXmlNode *precedingNode);
#endif
    virtual bool RemoveChild(wxXmlNode *child);
    virtual void AddProperty(const wxString& name, const wxString& value);
    virtual bool DeleteProperty(const wxString& name);

    // access methods:
    wxXmlNodeType GetType() const { return m_type; }
    wxString GetName() const { return m_name; }
    wxString GetContent() const { return m_content; }

    bool IsWhitespaceOnly() const;
    int GetDepth(wxXmlNode *grandparent = NULL) const;

    // Gets node content from wxXML_ENTITY_NODE
    // The problem is, <tag>content<tag> is represented as
    // wxXML_ENTITY_NODE name="tag", content=""
    //    |-- wxXML_TEXT_NODE or
    //        wxXML_CDATA_SECTION_NODE name="" content="content"
    wxString GetNodeContent() const;

    wxXmlNode *GetParent() const { return m_parent; }
    wxXmlNode *GetNext() const { return m_next; }
    wxXmlNode *GetChildren() const { return m_children; }

    wxXmlProperty *GetProperties() const { return m_properties; }
    bool GetPropVal(const wxString& propName, wxString *value) const;
    wxString GetPropVal(const wxString& propName,
                        const wxString& defaultVal) const;
    bool HasProp(const wxString& propName) const;

    void SetType(wxXmlNodeType type) { m_type = type; }
    void SetName(const wxString& name) { m_name = name; }
    void SetContent(const wxString& con) { m_content = con; }

    void SetParent(wxXmlNode *parent) { m_parent = parent; }
    void SetNext(wxXmlNode *next) { m_next = next; }
    void SetChildren(wxXmlNode *child) { m_children = child; }

    void SetProperties(wxXmlProperty *prop) { m_properties = prop; }
    virtual void AddProperty(wxXmlProperty *prop);

#if wxABI_VERSION >= 20811
    wxString GetAttribute(const wxString& attrName,
                         const wxString& defaultVal) const
    {
        return GetPropVal(attrName, defaultVal);
    }
    bool GetAttribute(const wxString& attrName, wxString *value) const
    {
        return GetPropVal(attrName, value);
    }
    void AddAttribute(const wxString& attrName, const wxString& value)
    {
        AddProperty(attrName, value);
    }
    wxXmlProperty* GetAttributes() const
    { 
        return GetProperties();
    }
#endif // wx >= 2.8.11

private:
    wxXmlNodeType m_type;
    wxString m_name;
    wxString m_content;
    wxXmlProperty *m_properties;
    wxXmlNode *m_parent, *m_children, *m_next;

    void DoCopy(const wxXmlNode& node);
};



// special indentation value for wxXmlDocument::Save
#define wxXML_NO_INDENTATION           (-1)

// flags for wxXmlDocument::Load
enum wxXmlDocumentLoadFlag
{
    wxXMLDOC_NONE = 0,
    wxXMLDOC_KEEP_WHITESPACE_NODES = 1
};


// This class holds XML data/document as parsed by XML parser.

class WXDLLIMPEXP_XML wxXmlDocument : public wxObject
{
public:
    wxXmlDocument();
    wxXmlDocument(const wxString& filename,
                  const wxString& encoding = wxT("UTF-8"));
    wxXmlDocument(wxInputStream& stream,
                  const wxString& encoding = wxT("UTF-8"));
    virtual ~wxXmlDocument() { wxDELETE(m_root); }

    wxXmlDocument(const wxXmlDocument& doc);
    wxXmlDocument& operator=(const wxXmlDocument& doc);

    // Parses .xml file and loads data. Returns TRUE on success, FALSE
    // otherwise.
    virtual bool Load(const wxString& filename,
                      const wxString& encoding = wxT("UTF-8"), int flags = wxXMLDOC_NONE);
    virtual bool Load(wxInputStream& stream,
                      const wxString& encoding = wxT("UTF-8"), int flags = wxXMLDOC_NONE);
    
    // Saves document as .xml file.
    virtual bool Save(const wxString& filename, int indentstep = 1) const;
    virtual bool Save(wxOutputStream& stream, int indentstep = 1) const;

    bool IsOk() const { return m_root != NULL; }

    // Returns root node of the document.
    wxXmlNode *GetRoot() const { return m_root; }

    // Returns version of document (may be empty).
    wxString GetVersion() const { return m_version; }
    // Returns encoding of document (may be empty).
    // Note: this is the encoding original file was saved in, *not* the
    // encoding of in-memory representation!
    wxString GetFileEncoding() const { return m_fileEncoding; }

    // Write-access methods:
    wxXmlNode *DetachRoot() { wxXmlNode *old=m_root; m_root=NULL; return old; }
    void SetRoot(wxXmlNode *node) { wxDELETE(m_root); m_root = node; }
    void SetVersion(const wxString& version) { m_version = version; }
    void SetFileEncoding(const wxString& encoding) { m_fileEncoding = encoding; }

#if !wxUSE_UNICODE
    // Returns encoding of in-memory representation of the document
    // (same as passed to Load or ctor, defaults to UTF-8).
    // NB: this is meaningless in Unicode build where data are stored as wchar_t*
    wxString GetEncoding() const { return m_encoding; }
    void SetEncoding(const wxString& enc) { m_encoding = enc; }
#endif

private:
    wxString   m_version;
    wxString   m_fileEncoding;
#if !wxUSE_UNICODE
    wxString   m_encoding;
#endif
    wxXmlNode *m_root;

    void DoCopy(const wxXmlDocument& doc);

    DECLARE_CLASS(wxXmlDocument)
};

#endif // wxUSE_XML

#endif // _WX_XML_H_
