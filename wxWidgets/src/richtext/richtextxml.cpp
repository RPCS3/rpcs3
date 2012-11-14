/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtextxml.cpp
// Purpose:     XML and HTML I/O for wxRichTextCtrl
// Author:      Julian Smart
// Modified by:
// Created:     2005-09-30
// RCS-ID:      $Id: richtextxml.cpp 67247 2011-03-19 12:27:23Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_RICHTEXT && wxUSE_XML

#include "wx/richtext/richtextxml.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/module.h"
#endif

#include "wx/filename.h"
#include "wx/clipbrd.h"
#include "wx/wfstream.h"
#include "wx/sstream.h"
#include "wx/txtstrm.h"
#include "wx/tokenzr.h"
#include "wx/xml/xml.h"

IMPLEMENT_DYNAMIC_CLASS(wxRichTextXMLHandler, wxRichTextFileHandler)

#if wxUSE_STREAMS
bool wxRichTextXMLHandler::DoLoadFile(wxRichTextBuffer *buffer, wxInputStream& stream)
{
    if (!stream.IsOk())
        return false;

    buffer->ResetAndClearCommands();
    buffer->Clear();

    wxXmlDocument* xmlDoc = new wxXmlDocument;
    bool success = true;

    // This is the encoding to convert to (memory encoding rather than file encoding)
    wxString encoding(wxT("UTF-8"));

#if !wxUSE_UNICODE && wxUSE_INTL
    encoding = wxLocale::GetSystemEncodingName();
#endif

    if (!xmlDoc->Load(stream, encoding))
    {
        buffer->ResetAndClearCommands();
        success = false;
    }
    else
    {
        if (xmlDoc->GetRoot() && xmlDoc->GetRoot()->GetType() == wxXML_ELEMENT_NODE && xmlDoc->GetRoot()->GetName() == wxT("richtext"))
        {
            wxXmlNode* child = xmlDoc->GetRoot()->GetChildren();
            while (child)
            {
                if (child->GetType() == wxXML_ELEMENT_NODE)
                {
                    wxString name = child->GetName();
                    if (name == wxT("richtext-version"))
                    {
                    }
                    else
                        ImportXML(buffer, child);
                }

                child = child->GetNext();
            }
        }
        else
        {
            success = false;
        }
    }

    delete xmlDoc;

    buffer->UpdateRanges();

    return success;
}

/// Recursively import an object
bool wxRichTextXMLHandler::ImportXML(wxRichTextBuffer* buffer, wxXmlNode* node)
{
    wxString name = node->GetName();

    bool doneChildren = false;

    if (name == wxT("paragraphlayout"))
    {
        wxString partial = node->GetPropVal(wxT("partialparagraph"), wxEmptyString);
        if (partial == wxT("true"))
            buffer->SetPartialParagraph(true);
    }
    else if (name == wxT("paragraph"))
    {
        wxRichTextParagraph* para = new wxRichTextParagraph(buffer);
        buffer->AppendChild(para);

        GetStyle(para->GetAttributes(), node, true);

        wxXmlNode* child = node->GetChildren();
        while (child)
        {
            wxString childName = child->GetName();
            if (childName == wxT("text"))
            {
                wxString text;
                wxXmlNode* textChild = child->GetChildren();
                while (textChild)
                {
                    if (textChild->GetType() == wxXML_TEXT_NODE ||
                        textChild->GetType() == wxXML_CDATA_SECTION_NODE)
                    {
                        wxString text2 = textChild->GetContent();

                        // Strip whitespace from end
                        if (!text2.empty() && text2[text2.length()-1] == wxT('\n'))
                            text2 = text2.Mid(0, text2.length()-1);

                        if (!text2.empty() && text2[0] == wxT('"'))
                            text2 = text2.Mid(1);
                        if (!text2.empty() && text2[text2.length()-1] == wxT('"'))
                            text2 = text2.Mid(0, text2.length() - 1);

                        text += text2;
                    }
                    textChild = textChild->GetNext();
                }

                wxRichTextPlainText* textObject = new wxRichTextPlainText(text, para);
                GetStyle(textObject->GetAttributes(), child, false);

                para->AppendChild(textObject);
            }
            else if (childName == wxT("symbol"))
            {
                // This is a symbol that XML can't read in the normal way
                wxString text;
                wxXmlNode* textChild = child->GetChildren();
                while (textChild)
                {
                    if (textChild->GetType() == wxXML_TEXT_NODE ||
                        textChild->GetType() == wxXML_CDATA_SECTION_NODE)
                    {
                        wxString text2 = textChild->GetContent();
                        text += text2;
                    }
                    textChild = textChild->GetNext();
                }

                wxString actualText;
                actualText << (wxChar) wxAtoi(text);

                wxRichTextPlainText* textObject = new wxRichTextPlainText(actualText, para);
                GetStyle(textObject->GetAttributes(), child, false);

                para->AppendChild(textObject);
            }
            else if (childName == wxT("image"))
            {
                int imageType = wxBITMAP_TYPE_PNG;
                wxString value = child->GetPropVal(wxT("imagetype"), wxEmptyString);
                if (!value.empty())
                    imageType = wxAtoi(value);

                wxString data;

                wxXmlNode* imageChild = child->GetChildren();
                while (imageChild)
                {
                    wxString childName = imageChild->GetName();
                    if (childName == wxT("data"))
                    {
                        wxXmlNode* dataChild = imageChild->GetChildren();
                        while (dataChild)
                        {
                            data = dataChild->GetContent();
                            // wxLogDebug(data);
                            dataChild = dataChild->GetNext();
                        }

                    }
                    imageChild = imageChild->GetNext();
                }

                if (!data.empty())
                {
                    wxRichTextImage* imageObj = new wxRichTextImage(para);
                    GetStyle(imageObj->GetAttributes(), child, false);
                    para->AppendChild(imageObj);

                    wxStringInputStream strStream(data);

                    imageObj->GetImageBlock().ReadHex(strStream, data.length(), imageType);
                }
            }
            child = child->GetNext();
        }

        doneChildren = true;
    }
    else if (name == wxT("stylesheet"))
    {
        if (GetFlags() & wxRICHTEXT_HANDLER_INCLUDE_STYLESHEET)
        {
            wxRichTextStyleSheet* sheet = new wxRichTextStyleSheet;
            wxString sheetName = node->GetPropVal(wxT("name"), wxEmptyString);
            wxString sheetDescription = node->GetPropVal(wxT("description"), wxEmptyString);
            sheet->SetName(sheetName);
            sheet->SetDescription(sheetDescription);

            wxXmlNode* child = node->GetChildren();
            while (child)
            {
                ImportStyleDefinition(sheet, child);

                child = child->GetNext();
            }

            // Notify that styles have changed. If this is vetoed by the app,
            // the new sheet will be deleted. If it is not vetoed, the
            // old sheet will be deleted and replaced with the new one.
            buffer->SetStyleSheetAndNotify(sheet);
        }
        doneChildren = true;
    }

    if (!doneChildren)
    {
        wxXmlNode* child = node->GetChildren();
        while (child)
        {
            ImportXML(buffer, child);
            child = child->GetNext();
        }
    }

    return true;
}

bool wxRichTextXMLHandler::ImportStyleDefinition(wxRichTextStyleSheet* sheet, wxXmlNode* node)
{
    wxString styleType = node->GetName();
    wxString styleName = node->GetPropVal(wxT("name"), wxEmptyString);
    wxString baseStyleName = node->GetPropVal(wxT("basestyle"), wxEmptyString);

    if (styleName.IsEmpty())
        return false;

    if (styleType == wxT("characterstyle"))
    {
        wxRichTextCharacterStyleDefinition* def = new wxRichTextCharacterStyleDefinition(styleName);
        def->SetBaseStyle(baseStyleName);

        wxXmlNode* child = node->GetChildren();
        while (child)
        {
            if (child->GetName() == wxT("style"))
            {
                wxTextAttrEx attr;
                GetStyle(attr, child, false);
                def->SetStyle(attr);
            }
            child = child->GetNext();
        }

        sheet->AddCharacterStyle(def);
    }
    else if (styleType == wxT("paragraphstyle"))
    {
        wxRichTextParagraphStyleDefinition* def = new wxRichTextParagraphStyleDefinition(styleName);

        wxString nextStyleName = node->GetPropVal(wxT("nextstyle"), wxEmptyString);
        def->SetNextStyle(nextStyleName);
        def->SetBaseStyle(baseStyleName);

        wxXmlNode* child = node->GetChildren();
        while (child)
        {
            if (child->GetName() == wxT("style"))
            {
                wxTextAttrEx attr;
                GetStyle(attr, child, true);
                def->SetStyle(attr);
            }
            child = child->GetNext();
        }

        sheet->AddParagraphStyle(def);
    }
    else if (styleType == wxT("liststyle"))
    {
        wxRichTextListStyleDefinition* def = new wxRichTextListStyleDefinition(styleName);

        wxString nextStyleName = node->GetPropVal(wxT("nextstyle"), wxEmptyString);
        def->SetNextStyle(nextStyleName);
        def->SetBaseStyle(baseStyleName);

        wxXmlNode* child = node->GetChildren();
        while (child)
        {
            if (child->GetName() == wxT("style"))
            {
                wxTextAttrEx attr;
                GetStyle(attr, child, true);

                wxString styleLevel = child->GetPropVal(wxT("level"), wxEmptyString);
                if (styleLevel.IsEmpty())
                {
                    def->SetStyle(attr);
                }
                else
                {
                    int level = wxAtoi(styleLevel);
                    if (level > 0 && level <= 10)
                    {
                        def->SetLevelAttributes(level-1, attr);
                    }
                }
            }
            child = child->GetNext();
        }

        sheet->AddListStyle(def);
    }

    return true;
}

//-----------------------------------------------------------------------------
//  xml support routines
//-----------------------------------------------------------------------------

bool wxRichTextXMLHandler::HasParam(wxXmlNode* node, const wxString& param)
{
    return (GetParamNode(node, param) != NULL);
}

wxXmlNode *wxRichTextXMLHandler::GetParamNode(wxXmlNode* node, const wxString& param)
{
    wxCHECK_MSG(node, NULL, wxT("You can't access node data before it was initialized!"));

    wxXmlNode *n = node->GetChildren();

    while (n)
    {
        if (n->GetType() == wxXML_ELEMENT_NODE && n->GetName() == param)
            return n;
        n = n->GetNext();
    }
    return NULL;
}


wxString wxRichTextXMLHandler::GetNodeContent(wxXmlNode *node)
{
    wxXmlNode *n = node;
    if (n == NULL) return wxEmptyString;
    n = n->GetChildren();

    while (n)
    {
        if (n->GetType() == wxXML_TEXT_NODE ||
            n->GetType() == wxXML_CDATA_SECTION_NODE)
            return n->GetContent();
        n = n->GetNext();
    }
    return wxEmptyString;
}


wxString wxRichTextXMLHandler::GetParamValue(wxXmlNode *node, const wxString& param)
{
    if (param.empty())
        return GetNodeContent(node);
    else
        return GetNodeContent(GetParamNode(node, param));
}

wxString wxRichTextXMLHandler::GetText(wxXmlNode *node, const wxString& param, bool WXUNUSED(translate))
{
    wxXmlNode *parNode = GetParamNode(node, param);
    if (!parNode)
        parNode = node;
    wxString str1(GetNodeContent(parNode));
    return str1;
}

// For use with earlier versions of wxWidgets
#ifndef WXUNUSED_IN_UNICODE
#if wxUSE_UNICODE
#define WXUNUSED_IN_UNICODE(x) WXUNUSED(x)
#else
#define WXUNUSED_IN_UNICODE(x) x
#endif
#endif

// write string to output:
inline static void OutputString(wxOutputStream& stream, const wxString& str,
                                wxMBConv *WXUNUSED_IN_UNICODE(convMem) = NULL, wxMBConv *convFile = NULL)
{
    if (str.empty()) return;
#if wxUSE_UNICODE
    if (convFile)
    {
        const wxWX2MBbuf buf(str.mb_str(*convFile));
        stream.Write((const char*)buf, strlen((const char*)buf));
    }
    else
    {
        const wxWX2MBbuf buf(str.mb_str(wxConvUTF8));
        stream.Write((const char*)buf, strlen((const char*)buf));
    }
#else
    if ( convFile == NULL )
        stream.Write(str.mb_str(), str.Len());
    else
    {
        wxString str2(str.wc_str(*convMem), *convFile);
        stream.Write(str2.mb_str(), str2.Len());
    }
#endif
}

// Same as above, but create entities first.
// Translates '<' to "&lt;", '>' to "&gt;" and '&' to "&amp;"
static void OutputStringEnt(wxOutputStream& stream, const wxString& str,
                            wxMBConv *convMem = NULL, wxMBConv *convFile = NULL)
{
    wxString buf;
    size_t i, last, len;
    wxChar c;

    len = str.Len();
    last = 0;
    for (i = 0; i < len; i++)
    {
        c = str.GetChar(i);

        // Original code excluded "&amp;" but we _do_ want to convert
        // the ampersand beginning &amp; because otherwise when read in,
        // the original "&amp;" becomes "&".

        if (c == wxT('<') || c == wxT('>') || c == wxT('"') ||
            (c == wxT('&') /* && (str.Mid(i+1, 4) != wxT("amp;")) */ ))
        {
            OutputString(stream, str.Mid(last, i - last), convMem, convFile);
            switch (c)
            {
            case wxT('<'):
                OutputString(stream, wxT("&lt;"), NULL, NULL);
                break;
            case wxT('>'):
                OutputString(stream, wxT("&gt;"), NULL, NULL);
                break;
            case wxT('&'):
                OutputString(stream, wxT("&amp;"), NULL, NULL);
                break;
            case wxT('"'):
                OutputString(stream, wxT("&quot;"), NULL, NULL);
                break;
            default: break;
            }
            last = i + 1;
        }
        else if (wxUChar(c) > 127)
        {
            OutputString(stream, str.Mid(last, i - last), convMem, convFile);

            wxString s(wxT("&#"));
#if wxUSE_UNICODE
            s << (int) c;
#else
            s << (int) wxUChar(c);
#endif
            s << wxT(";");
            OutputString(stream, s, NULL, NULL);
            last = i + 1;
        }
    }
    OutputString(stream, str.Mid(last, i - last), convMem, convFile);
}

static wxString AttributeToXML(const wxString& str)
{
    wxString str1;
    size_t i, last, len;
    wxChar c;

    len = str.Len();
    last = 0;
    for (i = 0; i < len; i++)
    {
        c = str.GetChar(i);

        // Original code excluded "&amp;" but we _do_ want to convert
        // the ampersand beginning &amp; because otherwise when read in,
        // the original "&amp;" becomes "&".

        if (c == wxT('<') || c == wxT('>') || c == wxT('"') ||
            (c == wxT('&') /* && (str.Mid(i+1, 4) != wxT("amp;")) */ ))
        {
            str1 += str.Mid(last, i - last);
            switch (c)
            {
            case wxT('<'):
                str1 += wxT("&lt;");
                break;
            case wxT('>'):
                str1 += wxT("&gt;");
                break;
            case wxT('&'):
                str1 += wxT("&amp;");
                break;
            case wxT('"'):
                str1 += wxT("&quot;");
                break;
            default: break;
            }
            last = i + 1;
        }
        else if (wxUChar(c) > 127)
        {
            str1 += str.Mid(last, i - last);

            wxString s(wxT("&#"));
#if wxUSE_UNICODE
            s << (int) c;
#else
            s << (int) wxUChar(c);
#endif
            s << wxT(";");
            str1 += s;
            last = i + 1;
        }
    }
    str1 += str.Mid(last, i - last);
    return str1;
}

inline static void OutputIndentation(wxOutputStream& stream, int indent)
{
    wxString str = wxT("\n");
    for (int i = 0; i < indent; i++)
        str << wxT(' ') << wxT(' ');
    OutputString(stream, str, NULL, NULL);
}

// Convert a colour to a 6-digit hex string
static wxString ColourToHexString(const wxColour& col)
{
    wxString hex;

    hex += wxDecToHex(col.Red());
    hex += wxDecToHex(col.Green());
    hex += wxDecToHex(col.Blue());

    return hex;
}

// Convert 6-digit hex string to a colour
static wxColour HexStringToColour(const wxString& hex)
{
    unsigned char r = (unsigned char)wxHexToDec(hex.Mid(0, 2));
    unsigned char g = (unsigned char)wxHexToDec(hex.Mid(2, 2));
    unsigned char b = (unsigned char)wxHexToDec(hex.Mid(4, 2));

    return wxColour(r, g, b);
}

bool wxRichTextXMLHandler::DoSaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream)
{
    if (!stream.IsOk())
        return false;

    wxString version(wxT("1.0") ) ;

    bool deleteConvFile = false;
    wxString fileEncoding;
    wxMBConv* convFile = NULL;

#if wxUSE_UNICODE
    fileEncoding = wxT("UTF-8");
    convFile = & wxConvUTF8;
#else
    fileEncoding = wxT("ISO-8859-1");
    convFile = & wxConvISO8859_1;
#endif

    // If SetEncoding has been called, change the output encoding.
    if (!m_encoding.empty() && m_encoding.Lower() != fileEncoding.Lower())
    {
        if (m_encoding == wxT("<System>"))
        {
#if wxUSE_INTL
            fileEncoding = wxLocale::GetSystemEncodingName();
#endif // wxUSE_INTL
        }
        else
        {
            fileEncoding = m_encoding;
        }

        // GetSystemEncodingName may not have returned a name
        if (fileEncoding.empty())
#if wxUSE_UNICODE
            fileEncoding = wxT("UTF-8");
#else
            fileEncoding = wxT("ISO-8859-1");
#endif
        convFile = new wxCSConv(fileEncoding);
        deleteConvFile = true;
    }

#if !wxUSE_UNICODE
    wxMBConv* convMem = wxConvCurrent;
#else
    wxMBConv* convMem = NULL;
#endif

    wxString s ;
    s.Printf(wxT("<?xml version=\"%s\" encoding=\"%s\"?>\n"),
        (const wxChar*) version, (const wxChar*) fileEncoding );
    OutputString(stream, s, NULL, NULL);
    OutputString(stream, wxT("<richtext version=\"1.0.0.0\" xmlns=\"http://www.wxwidgets.org\">") , NULL, NULL);

    int level = 1;

    if (buffer->GetStyleSheet() && (GetFlags() & wxRICHTEXT_HANDLER_INCLUDE_STYLESHEET))
    {
        OutputIndentation(stream, level);
        wxString nameAndDescr;
        if (!buffer->GetStyleSheet()->GetName().IsEmpty())
            nameAndDescr << wxT(" name=\"") << buffer->GetStyleSheet()->GetName() << wxT("\"");
        if (!buffer->GetStyleSheet()->GetDescription().IsEmpty())
            nameAndDescr << wxT(" description=\"") << buffer->GetStyleSheet()->GetDescription() << wxT("\"");
        OutputString(stream, wxString(wxT("<stylesheet")) + nameAndDescr + wxT(">"), convMem, convFile);

        int i;

        for (i = 0; i < (int) buffer->GetStyleSheet()->GetCharacterStyleCount(); i++)
        {
            wxRichTextCharacterStyleDefinition* def = buffer->GetStyleSheet()->GetCharacterStyle(i);
            ExportStyleDefinition(stream, convMem, convFile, def, level + 1);
        }

        for (i = 0; i < (int) buffer->GetStyleSheet()->GetParagraphStyleCount(); i++)
        {
            wxRichTextParagraphStyleDefinition* def = buffer->GetStyleSheet()->GetParagraphStyle(i);
            ExportStyleDefinition(stream, convMem, convFile, def, level + 1);
        }

        for (i = 0; i < (int) buffer->GetStyleSheet()->GetListStyleCount(); i++)
        {
            wxRichTextListStyleDefinition* def = buffer->GetStyleSheet()->GetListStyle(i);
            ExportStyleDefinition(stream, convMem, convFile, def, level + 1);
        }

        OutputIndentation(stream, level);
        OutputString(stream, wxT("</stylesheet>"), convMem, convFile);
    }


    bool success = ExportXML(stream, convMem, convFile, *buffer, level);

    OutputString(stream, wxT("\n</richtext>") , NULL, NULL);
    OutputString(stream, wxT("\n"), NULL, NULL);

    if (deleteConvFile)
        delete convFile;

    return success;
}

/// Recursively export an object
bool wxRichTextXMLHandler::ExportXML(wxOutputStream& stream, wxMBConv* convMem, wxMBConv* convFile, wxRichTextObject& obj, int indent)
{
    wxString objectName;
    if (obj.IsKindOf(CLASSINFO(wxRichTextParagraphLayoutBox)))
        objectName = wxT("paragraphlayout");
    else if (obj.IsKindOf(CLASSINFO(wxRichTextParagraph)))
        objectName = wxT("paragraph");
    else if (obj.IsKindOf(CLASSINFO(wxRichTextPlainText)))
        objectName = wxT("text");
    else if (obj.IsKindOf(CLASSINFO(wxRichTextImage)))
        objectName = wxT("image");
    else
        objectName = wxT("object");

    bool terminateTag = true;

    if (obj.IsKindOf(CLASSINFO(wxRichTextPlainText)))
    {
        wxRichTextPlainText& textObj = (wxRichTextPlainText&) obj;

        wxString style = CreateStyle(obj.GetAttributes(), false);

        int i;
        int last = 0;
        const wxString& text = textObj.GetText();
        int len = (int) text.Length();

        if (len == 0)
        {
            i = 0;
            OutputIndentation(stream, indent);
            OutputString(stream, wxT("<") + objectName, convMem, convFile);
            OutputString(stream, style + wxT(">"), convMem, convFile);
            OutputString(stream, wxT("</text>"), convMem, convFile);
        }
        else for (i = 0; i < len; i++)
        {
#if wxUSE_UNICODE
            int c = (int) text[i];
#else
            int c = (int) wxUChar(text[i]);
#endif
            if ((c < 32 || c == 34) && /* c != 9 && */ c != 10 && c != 13)
            {
                if (i > 0)
                {
                    wxString fragment(text.Mid(last, i-last));
                    if (!fragment.IsEmpty())
                    {
                        OutputIndentation(stream, indent);
                        OutputString(stream, wxT("<") + objectName, convMem, convFile);

                        OutputString(stream, style + wxT(">"), convMem, convFile);

                        if (!fragment.empty() && (fragment[0] == wxT(' ') || fragment[fragment.length()-1] == wxT(' ')))
                        {
                            OutputString(stream, wxT("\""), convMem, convFile);
                            OutputStringEnt(stream, fragment, convMem, convFile);
                            OutputString(stream, wxT("\""), convMem, convFile);
                        }
                        else
                            OutputStringEnt(stream, fragment, convMem, convFile);

                        OutputString(stream, wxT("</text>"), convMem, convFile);
                    }
                }

                // Output this character as a number in a separate tag, because XML can't cope
                // with entities below 32 except for 10 and 13
                last = i + 1;
                OutputIndentation(stream, indent);
                OutputString(stream, wxT("<symbol"), convMem, convFile);

                OutputString(stream, style + wxT(">"), convMem, convFile);
                OutputString(stream, wxString::Format(wxT("%d"), c), convMem, convFile);

                OutputString(stream, wxT("</symbol>"), convMem, convFile);
            }
        }

        wxString fragment;
        if (last == 0)
            fragment = text;
        else
            fragment = text.Mid(last, i-last);

        if (last < len)
        {
            OutputIndentation(stream, indent);
            OutputString(stream, wxT("<") + objectName, convMem, convFile);

            OutputString(stream, style + wxT(">"), convMem, convFile);

            if (!fragment.empty() && (fragment[0] == wxT(' ') || fragment[fragment.length()-1] == wxT(' ')))
            {
                OutputString(stream, wxT("\""), convMem, convFile);
                OutputStringEnt(stream, fragment, convMem, convFile);
                OutputString(stream, wxT("\""), convMem, convFile);
            }
            else
                OutputStringEnt(stream, fragment, convMem, convFile);
        }
        else
            terminateTag = false;
    }
    else if (obj.IsKindOf(CLASSINFO(wxRichTextImage)))
    {
        wxRichTextImage& imageObj = (wxRichTextImage&) obj;

        wxString style = CreateStyle(obj.GetAttributes(), false);

        if (imageObj.GetImage().Ok() && !imageObj.GetImageBlock().Ok())
            imageObj.MakeBlock();

        OutputIndentation(stream, indent);
        OutputString(stream, wxT("<") + objectName, convMem, convFile);
        if (!imageObj.GetImageBlock().Ok())
        {
            // No data
            OutputString(stream, style + wxT(">"), convMem, convFile);
        }
        else
        {
            OutputString(stream, wxString::Format(wxT(" imagetype=\"%d\""), (int) imageObj.GetImageBlock().GetImageType()) + style + wxT(">"));
        }

        OutputIndentation(stream, indent+1);
        OutputString(stream, wxT("<data>"), convMem, convFile);

        imageObj.GetImageBlock().WriteHex(stream);

        OutputString(stream, wxT("</data>"), convMem, convFile);
    }
    else if (obj.IsKindOf(CLASSINFO(wxRichTextCompositeObject)))
    {
        OutputIndentation(stream, indent);
        OutputString(stream, wxT("<") + objectName, convMem, convFile);

        bool isPara = false;
        if (objectName == wxT("paragraph") || objectName == wxT("paragraphlayout"))
            isPara = true;

        wxString style = CreateStyle(obj.GetAttributes(), isPara);

        if (objectName == wxT("paragraphlayout") && ((wxRichTextParagraphLayoutBox&) obj).GetPartialParagraph())
            style << wxT(" partialparagraph=\"true\"");

        OutputString(stream, style + wxT(">"), convMem, convFile);

        wxRichTextCompositeObject& composite = (wxRichTextCompositeObject&) obj;
        size_t i;
        for (i = 0; i < composite.GetChildCount(); i++)
        {
            wxRichTextObject* child = composite.GetChild(i);
            ExportXML(stream, convMem, convFile, *child, indent+1);
        }
    }

    if (objectName != wxT("text"))
        OutputIndentation(stream, indent);

    if (terminateTag)
        OutputString(stream, wxT("</") + objectName + wxT(">"), convMem, convFile);

    return true;
}

bool wxRichTextXMLHandler::ExportStyleDefinition(wxOutputStream& stream, wxMBConv* convMem, wxMBConv* convFile, wxRichTextStyleDefinition* def, int level)
{
    wxRichTextCharacterStyleDefinition* charDef = wxDynamicCast(def, wxRichTextCharacterStyleDefinition);
    wxRichTextParagraphStyleDefinition* paraDef = wxDynamicCast(def, wxRichTextParagraphStyleDefinition);
    wxRichTextListStyleDefinition* listDef = wxDynamicCast(def, wxRichTextListStyleDefinition);

    wxString baseStyle = def->GetBaseStyle();
    wxString baseStyleProp;
    if (!baseStyle.IsEmpty())
        baseStyleProp = wxT(" basestyle=\"") + baseStyle + wxT("\"");

    wxString descr = def->GetDescription();
    wxString descrProp;
    if (!descr.IsEmpty())
        descrProp = wxT(" description=\"") + descr + wxT("\"");

    if (charDef)
    {
        OutputIndentation(stream, level);
        OutputString(stream, wxT("<characterstyle") + baseStyleProp + descrProp + wxT(">"), convMem, convFile);

        level ++;

        wxString style = CreateStyle(def->GetStyle(), false);

        OutputIndentation(stream, level);
        OutputString(stream, wxT("<style ") + style + wxT(">"), convMem, convFile);

        OutputIndentation(stream, level);
        OutputString(stream, wxT("</style>"), convMem, convFile);

        level --;

        OutputIndentation(stream, level);
        OutputString(stream, wxT("</characterstyle>"), convMem, convFile);
    }
    else if (listDef)
    {
        OutputIndentation(stream, level);

        if (!listDef->GetNextStyle().IsEmpty())
            baseStyleProp << wxT(" nextstyle=\"") << listDef->GetNextStyle() << wxT("\"");

        OutputString(stream, wxT("<liststyle") + baseStyleProp + descrProp + wxT(">"), convMem, convFile);

        level ++;

        wxString style = CreateStyle(def->GetStyle(), true);

        OutputIndentation(stream, level);
        OutputString(stream, wxT("<style ") + style + wxT(">"), convMem, convFile);

        OutputIndentation(stream, level);
        OutputString(stream, wxT("</style>"), convMem, convFile);

        int i;
        for (i = 0; i < 10; i ++)
        {
            wxRichTextAttr* levelAttr = listDef->GetLevelAttributes(i);
            if (levelAttr)
            {
                wxString style = CreateStyle(*levelAttr, true);
                wxString levelStr = wxString::Format(wxT(" level=\"%d\" "), (i+1));

                OutputIndentation(stream, level);
                OutputString(stream, wxT("<style ") + levelStr + style + wxT(">"), convMem, convFile);

                OutputIndentation(stream, level);
                OutputString(stream, wxT("</style>"), convMem, convFile);
            }
        }

        level --;

        OutputIndentation(stream, level);
        OutputString(stream, wxT("</liststyle>"), convMem, convFile);
    }
    else if (paraDef)
    {
        OutputIndentation(stream, level);

        if (!paraDef->GetNextStyle().IsEmpty())
            baseStyleProp << wxT(" nextstyle=\"") << paraDef->GetNextStyle() << wxT("\"");

        OutputString(stream, wxT("<paragraphstyle") + baseStyleProp + descrProp + wxT(">"), convMem, convFile);

        level ++;

        wxString style = CreateStyle(def->GetStyle(), false);

        OutputIndentation(stream, level);
        OutputString(stream, wxT("<style ") + style + wxT(">"), convMem, convFile);

        OutputIndentation(stream, level);
        OutputString(stream, wxT("</style>"), convMem, convFile);

        level --;

        OutputIndentation(stream, level);
        OutputString(stream, wxT("</paragraphstyle>"), convMem, convFile);
    }

    return true;
}

/// Create style parameters
wxString wxRichTextXMLHandler::CreateStyle(const wxTextAttrEx& attr, bool isPara)
{
    wxString str;
    if (attr.HasTextColour() && attr.GetTextColour().Ok())
    {
        str << wxT(" textcolor=\"#") << ColourToHexString(attr.GetTextColour()) << wxT("\"");
    }
    if (attr.HasBackgroundColour() && attr.GetBackgroundColour().Ok())
    {
        str << wxT(" bgcolor=\"#") << ColourToHexString(attr.GetBackgroundColour()) << wxT("\"");
    }

    if (attr.GetFont().Ok())
    {
        if (attr.HasFontSize())
            str << wxT(" fontsize=\"") << attr.GetFont().GetPointSize() << wxT("\"");

        //if (attr.HasFontFamily())
        //    str << wxT(" fontfamily=\"") << attr.GetFont().GetFamily() << wxT("\"");

        if (attr.HasFontItalic())
            str << wxT(" fontstyle=\"") << attr.GetFont().GetStyle() << wxT("\"");

        if (attr.HasFontWeight())
            str << wxT(" fontweight=\"") << attr.GetFont().GetWeight() << wxT("\"");

        if (attr.HasFontUnderlined())
            str << wxT(" fontunderlined=\"") << (int) attr.GetFont().GetUnderlined() << wxT("\"");

        if (attr.HasFontFaceName())
            str << wxT(" fontface=\"") << attr.GetFont().GetFaceName() << wxT("\"");
    }

    if (attr.HasTextEffects())
    {
        str << wxT(" texteffects=\"");
        str << attr.GetTextEffects();
        str << wxT("\"");

        str << wxT(" texteffectflags=\"");
        str << attr.GetTextEffectFlags();
        str << wxT("\"");
    }

    if (!attr.GetCharacterStyleName().empty())
        str << wxT(" characterstyle=\"") << wxString(attr.GetCharacterStyleName()) << wxT("\"");

    if (attr.HasURL())
        str << wxT(" url=\"") << AttributeToXML(attr.GetURL()) << wxT("\"");

    if (isPara)
    {
        if (attr.HasAlignment())
            str << wxT(" alignment=\"") << (int) attr.GetAlignment() << wxT("\"");

        if (attr.HasLeftIndent())
        {
            str << wxT(" leftindent=\"") << (int) attr.GetLeftIndent() << wxT("\"");
            str << wxT(" leftsubindent=\"") << (int) attr.GetLeftSubIndent() << wxT("\"");
        }

        if (attr.HasRightIndent())
            str << wxT(" rightindent=\"") << (int) attr.GetRightIndent() << wxT("\"");

        if (attr.HasParagraphSpacingAfter())
            str << wxT(" parspacingafter=\"") << (int) attr.GetParagraphSpacingAfter() << wxT("\"");

        if (attr.HasParagraphSpacingBefore())
            str << wxT(" parspacingbefore=\"") << (int) attr.GetParagraphSpacingBefore() << wxT("\"");

        if (attr.HasLineSpacing())
            str << wxT(" linespacing=\"") << (int) attr.GetLineSpacing() << wxT("\"");

        if (attr.HasBulletStyle())
            str << wxT(" bulletstyle=\"") << (int) attr.GetBulletStyle() << wxT("\"");

        if (attr.HasBulletNumber())
            str << wxT(" bulletnumber=\"") << (int) attr.GetBulletNumber() << wxT("\"");

        if (attr.HasBulletText())
        {
            // If using a bullet symbol, convert to integer in case it's a non-XML-friendly character.
            // Otherwise, assume it's XML-friendly text such as outline numbering, e.g. 1.2.3.1
            if (!attr.GetBulletText().IsEmpty() && (attr.GetBulletStyle() & wxTEXT_ATTR_BULLET_STYLE_SYMBOL))
                str << wxT(" bulletsymbol=\"") << (int) (attr.GetBulletText()[0]) << wxT("\"");
            else
                str << wxT(" bullettext=\"") << attr.GetBulletText() << wxT("\"");

            str << wxT(" bulletfont=\"") << attr.GetBulletFont() << wxT("\"");
        }

        if (attr.HasBulletName())
            str << wxT(" bulletname=\"") << attr.GetBulletName() << wxT("\"");

        if (!attr.GetParagraphStyleName().empty())
            str << wxT(" parstyle=\"") << wxString(attr.GetParagraphStyleName()) << wxT("\"");

        if (!attr.GetListStyleName().empty())
            str << wxT(" liststyle=\"") << wxString(attr.GetListStyleName()) << wxT("\"");

        if (attr.HasTabs())
        {
            str << wxT(" tabs=\"");
            size_t i;
            for (i = 0; i < attr.GetTabs().GetCount(); i++)
            {
                if (i > 0)
                    str << wxT(",");
                str << attr.GetTabs()[i];
            }
            str << wxT("\"");
        }

        if (attr.HasPageBreak())
        {
            str << wxT(" pagebreak=\"1\"");
        }

        if (attr.HasOutlineLevel())
            str << wxT(" outlinelevel=\"") << (int) attr.GetOutlineLevel() << wxT("\"");

    }

    return str;
}

/// Replace face name with current name for platform.
/// TODO: introduce a virtual function or settable table to
/// do this comprehensively.
bool wxRichTextFixFaceName(wxString& facename)
{
    if (facename.IsEmpty())
        return false;

#ifdef __WXMSW__
    if (facename == wxT("Times"))
    {
        facename = wxT("Times New Roman");
        return true;
    }
    else if (facename == wxT("Helvetica"))
    {
        facename = wxT("Arial");
        return true;
    }
    else if (facename == wxT("Courier"))
    {
        facename = wxT("Courier New");
        return true;
    }
    else
        return false;
#else
    if (facename == wxT("Times New Roman"))
    {
        facename = wxT("Times");
        return true;
    }
    else if (facename == wxT("Arial"))
    {
        facename = wxT("Helvetica");
        return true;
    }
    else if (facename == wxT("Courier New"))
    {
        facename = wxT("Courier");
        return true;
    }
    else
        return false;
#endif
}


/// Get style parameters
bool wxRichTextXMLHandler::GetStyle(wxTextAttrEx& attr, wxXmlNode* node, bool isPara)
{
    wxString fontFacename;
    int fontSize = 12;
    int fontFamily = wxDEFAULT;
    int fontWeight = wxNORMAL;
    int fontStyle = wxNORMAL;
    bool fontUnderlined = false;
    int fontFlags = 0;
    const wxString emptyString; // save a temporary string construction in GetPropVal

    fontFacename = node->GetPropVal(wxT("fontface"), emptyString);
    if (!fontFacename.IsEmpty())
    {
        fontFlags |= wxTEXT_ATTR_FONT_FACE;

        if (GetFlags() & wxRICHTEXT_HANDLER_CONVERT_FACENAMES)
            wxRichTextFixFaceName(fontFacename);
    }

    wxString value;
    //value = node->GetPropVal(wxT("fontfamily"), emptyString);
    //if (!value.empty())
    //    fontFamily = wxAtoi(value);

    value = node->GetPropVal(wxT("fontstyle"), emptyString);
    if (!value.empty())
    {
        fontStyle = wxAtoi(value);
        fontFlags |= wxTEXT_ATTR_FONT_ITALIC;
    }

    value = node->GetPropVal(wxT("fontsize"), emptyString);
    if (!value.empty())
    {
        fontSize = wxAtoi(value);
        fontFlags |= wxTEXT_ATTR_FONT_SIZE;
    }

    value = node->GetPropVal(wxT("fontweight"), emptyString);
    if (!value.empty())
    {
        fontWeight = wxAtoi(value);
        fontFlags |= wxTEXT_ATTR_FONT_WEIGHT;
    }

    value = node->GetPropVal(wxT("fontunderlined"), emptyString);
    if (!value.empty())
    {
        fontUnderlined = wxAtoi(value) != 0;
        fontFlags |= wxTEXT_ATTR_FONT_UNDERLINE;
    }

    attr.SetFlags(fontFlags);

    // FindOrCreateFont is an expensive operation on GTK+ (because pango functions are called)
    // so only use it on other platforms
    if (attr.HasFlag(wxTEXT_ATTR_FONT))
#ifdef __WXGTK__
        attr.SetFont(wxFont(fontSize, fontFamily, fontStyle, fontWeight, fontUnderlined, fontFacename));
#else
        attr.SetFont(* wxTheFontList->FindOrCreateFont(fontSize, fontFamily, fontStyle, fontWeight, fontUnderlined, fontFacename));
#endif

    // Restore correct font flags
    attr.SetFlags(fontFlags);

    value = node->GetPropVal(wxT("textcolor"), emptyString);
    if (!value.empty())
    {
        if (value[0] == wxT('#'))
            attr.SetTextColour(HexStringToColour(value.Mid(1)));
        else
            attr.SetTextColour(value);
    }

    value = node->GetPropVal(wxT("bgcolor"), emptyString);
    if (!value.empty())
    {
        if (value[0] == wxT('#'))
            attr.SetBackgroundColour(HexStringToColour(value.Mid(1)));
        else
            attr.SetBackgroundColour(value);
    }

    value = node->GetPropVal(wxT("characterstyle"), emptyString);
    if (!value.empty())
        attr.SetCharacterStyleName(value);

    value = node->GetPropVal(wxT("texteffects"), emptyString);
    if (!value.IsEmpty())
    {
        attr.SetTextEffects(wxAtoi(value));
    }

    value = node->GetPropVal(wxT("texteffectflags"), emptyString);
    if (!value.IsEmpty())
    {
        attr.SetTextEffectFlags(wxAtoi(value));
    }

    value = node->GetPropVal(wxT("url"), emptyString);
    if (!value.empty())
        attr.SetURL(value);

    // Set paragraph attributes
    if (isPara)
    {
        value = node->GetPropVal(wxT("alignment"), emptyString);
        if (!value.empty())
            attr.SetAlignment((wxTextAttrAlignment) wxAtoi(value));

        int leftSubIndent = 0;
        int leftIndent = 0;
        bool hasLeftIndent = false;

        value = node->GetPropVal(wxT("leftindent"), emptyString);
        if (!value.empty())
        {
            leftIndent = wxAtoi(value);
            hasLeftIndent = true;
        }

        value = node->GetPropVal(wxT("leftsubindent"), emptyString);
        if (!value.empty())
        {
            leftSubIndent = wxAtoi(value);
            hasLeftIndent = true;
        }

        if (hasLeftIndent)
            attr.SetLeftIndent(leftIndent, leftSubIndent);

        value = node->GetPropVal(wxT("rightindent"), emptyString);
        if (!value.empty())
            attr.SetRightIndent(wxAtoi(value));

        value = node->GetPropVal(wxT("parspacingbefore"), emptyString);
        if (!value.empty())
            attr.SetParagraphSpacingBefore(wxAtoi(value));

        value = node->GetPropVal(wxT("parspacingafter"), emptyString);
        if (!value.empty())
            attr.SetParagraphSpacingAfter(wxAtoi(value));

        value = node->GetPropVal(wxT("linespacing"), emptyString);
        if (!value.empty())
            attr.SetLineSpacing(wxAtoi(value));

        value = node->GetPropVal(wxT("bulletstyle"), emptyString);
        if (!value.empty())
            attr.SetBulletStyle(wxAtoi(value));

        value = node->GetPropVal(wxT("bulletnumber"), emptyString);
        if (!value.empty())
            attr.SetBulletNumber(wxAtoi(value));

        value = node->GetPropVal(wxT("bulletsymbol"), emptyString);
        if (!value.empty())
        {
            wxChar ch = wxAtoi(value);
            wxString s;
            s << ch;
            attr.SetBulletText(s);
        }

        value = node->GetPropVal(wxT("bullettext"), emptyString);
        if (!value.empty())
            attr.SetBulletText(value);

        value = node->GetPropVal(wxT("bulletfont"), emptyString);
        if (!value.empty())
            attr.SetBulletFont(value);

        value = node->GetPropVal(wxT("bulletname"), emptyString);
        if (!value.empty())
            attr.SetBulletName(value);

        value = node->GetPropVal(wxT("parstyle"), emptyString);
        if (!value.empty())
            attr.SetParagraphStyleName(value);

        value = node->GetPropVal(wxT("liststyle"), emptyString);
        if (!value.empty())
            attr.SetListStyleName(value);

        value = node->GetPropVal(wxT("tabs"), emptyString);
        if (!value.empty())
        {
            wxArrayInt tabs;
            wxStringTokenizer tkz(value, wxT(","));
            while (tkz.HasMoreTokens())
            {
                wxString token = tkz.GetNextToken();
                tabs.Add(wxAtoi(token));
            }
            attr.SetTabs(tabs);
        }

        value = node->GetPropVal(wxT("pagebreak"), emptyString);
        if (!value.IsEmpty())
        {
            attr.SetPageBreak(wxAtoi(value) != 0);
        }

        value = node->GetPropVal(wxT("outlinelevel"), emptyString);
        if (!value.IsEmpty())
        {
            attr.SetOutlineLevel(wxAtoi(value));
        }
    }

    return true;
}

#endif
    // wxUSE_STREAMS

#endif
    // wxUSE_RICHTEXT && wxUSE_XML

