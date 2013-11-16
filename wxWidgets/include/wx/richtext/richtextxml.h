/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richeditxml.h
// Purpose:     XML and HTML I/O for wxRichTextCtrl
// Author:      Julian Smart
// Modified by:
// Created:     2005-09-30
// RCS-ID:      $Id: richtextxml.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RICHTEXTXML_H_
#define _WX_RICHTEXTXML_H_

/*!
 * Includes
 */

#include "wx/richtext/richtextbuffer.h"
#include "wx/richtext/richtextstyles.h"

#if wxUSE_RICHTEXT && wxUSE_XML

/*!
 * wxRichTextXMLHandler
 */

class WXDLLIMPEXP_FWD_XML wxXmlNode;

class WXDLLIMPEXP_RICHTEXT wxRichTextXMLHandler: public wxRichTextFileHandler
{
    DECLARE_CLASS(wxRichTextXMLHandler)
public:
    wxRichTextXMLHandler(const wxString& name = wxT("XML"), const wxString& ext = wxT("xml"), int type = wxRICHTEXT_TYPE_XML)
        : wxRichTextFileHandler(name, ext, type)
        { }

#if wxUSE_STREAMS
    /// Recursively export an object
    bool ExportXML(wxOutputStream& stream, wxMBConv* convMem, wxMBConv* convFile, wxRichTextObject& obj, int level);
    bool ExportStyleDefinition(wxOutputStream& stream, wxMBConv* convMem, wxMBConv* convFile, wxRichTextStyleDefinition* def, int level);

    /// Recursively import an object
    bool ImportXML(wxRichTextBuffer* buffer, wxXmlNode* node);
    bool ImportStyleDefinition(wxRichTextStyleSheet* sheet, wxXmlNode* node);

    /// Create style parameters
    wxString CreateStyle(const wxTextAttrEx& attr, bool isPara = false);

    /// Get style parameters
    bool GetStyle(wxTextAttrEx& attr, wxXmlNode* node, bool isPara = false);
#endif

    /// Can we save using this handler?
    virtual bool CanSave() const { return true; }

    /// Can we load using this handler?
    virtual bool CanLoad() const { return true; }

// Implementation

    bool HasParam(wxXmlNode* node, const wxString& param);
    wxXmlNode *GetParamNode(wxXmlNode* node, const wxString& param);
    wxString GetNodeContent(wxXmlNode *node);
    wxString GetParamValue(wxXmlNode *node, const wxString& param);
    wxString GetText(wxXmlNode *node, const wxString& param = wxEmptyString, bool translate = false);

protected:
#if wxUSE_STREAMS
    virtual bool DoLoadFile(wxRichTextBuffer *buffer, wxInputStream& stream);
    virtual bool DoSaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream);
#endif
};

#endif
    // wxUSE_RICHTEXT && wxUSE_XML

#endif
    // _WX_RICHTEXTXML_H_
