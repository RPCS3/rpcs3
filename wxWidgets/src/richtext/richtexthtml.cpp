/////////////////////////////////////////////////////////////////////////////
// Name:        src/richtext/richtexthtml.cpp
// Purpose:     HTML I/O for wxRichTextCtrl
// Author:      Julian Smart
// Modified by:
// Created:     2005-09-30
// RCS-ID:      $Id: richtexthtml.cpp 64162 2010-04-27 16:10:27Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_RICHTEXT

#include "wx/richtext/richtexthtml.h"
#include "wx/richtext/richtextstyles.h"

#ifndef WX_PRECOMP
#endif

#include "wx/filename.h"
#include "wx/wfstream.h"
#include "wx/txtstrm.h"

#if wxUSE_FILESYSTEM
#include "wx/filesys.h"
#include "wx/fs_mem.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxRichTextHTMLHandler, wxRichTextFileHandler)

int wxRichTextHTMLHandler::sm_fileCounter = 1;

wxRichTextHTMLHandler::wxRichTextHTMLHandler(const wxString& name, const wxString& ext, int type)
    : wxRichTextFileHandler(name, ext, type), m_buffer(NULL), m_font(false), m_inTable(false)
{
    m_fontSizeMapping.Add(8);
    m_fontSizeMapping.Add(10);
    m_fontSizeMapping.Add(13);
    m_fontSizeMapping.Add(17);
    m_fontSizeMapping.Add(22);
    m_fontSizeMapping.Add(30);
    m_fontSizeMapping.Add(100);
}

/// Can we handle this filename (if using files)? By default, checks the extension.
bool wxRichTextHTMLHandler::CanHandle(const wxString& filename) const
{
    wxString path, file, ext;
    wxSplitPath(filename, & path, & file, & ext);

    return (ext.Lower() == wxT("html") || ext.Lower() == wxT("htm"));
}


#if wxUSE_STREAMS
bool wxRichTextHTMLHandler::DoLoadFile(wxRichTextBuffer *WXUNUSED(buffer), wxInputStream& WXUNUSED(stream))
{
    return false;
}

/*
 * We need to output only _changes_ in character formatting.
 */

bool wxRichTextHTMLHandler::DoSaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream)
{
    m_buffer = buffer;

    ClearTemporaryImageLocations();

    buffer->Defragment();

#if wxUSE_UNICODE
    wxCSConv* customEncoding = NULL;
    wxMBConv* conv = NULL;
    if (!GetEncoding().IsEmpty())
    {
        customEncoding = new wxCSConv(GetEncoding());
        if (!customEncoding->IsOk())
        {
            delete customEncoding;
            customEncoding = NULL;
        }
    }
    if (customEncoding)
        conv = customEncoding;
    else
        conv = & wxConvUTF8;
#endif

    {
#if wxUSE_UNICODE
        wxTextOutputStream str(stream, wxEOL_NATIVE, *conv);
#else
        wxTextOutputStream str(stream, wxEOL_NATIVE);
#endif

        wxTextAttrEx currentParaStyle = buffer->GetAttributes();
        wxTextAttrEx currentCharStyle = buffer->GetAttributes();

        if ((GetFlags() & wxRICHTEXT_HANDLER_NO_HEADER_FOOTER) == 0)
            str << wxT("<html><head></head><body>\n");

        OutputFont(currentParaStyle, str);

        m_font = false;
        m_inTable = false;

        m_indents.Clear();
        m_listTypes.Clear();

        wxRichTextObjectList::compatibility_iterator node = buffer->GetChildren().GetFirst();
        while (node)
        {
            wxRichTextParagraph* para = wxDynamicCast(node->GetData(), wxRichTextParagraph);
            wxASSERT (para != NULL);

            if (para)
            {
                wxTextAttrEx paraStyle(para->GetCombinedAttributes());

                BeginParagraphFormatting(currentParaStyle, paraStyle, str);

                wxRichTextObjectList::compatibility_iterator node2 = para->GetChildren().GetFirst();
                while (node2)
                {
                    wxRichTextObject* obj = node2->GetData();
                    wxRichTextPlainText* textObj = wxDynamicCast(obj, wxRichTextPlainText);
                    if (textObj && !textObj->IsEmpty())
                    {
                        wxTextAttrEx charStyle(para->GetCombinedAttributes(obj->GetAttributes()));
                        BeginCharacterFormatting(currentCharStyle, charStyle, paraStyle, str);

                        wxString text = textObj->GetText();

                        if (charStyle.HasTextEffects() && (charStyle.GetTextEffects() & wxTEXT_ATTR_EFFECT_CAPITALS))
                            text.MakeUpper();

                        wxString toReplace = wxRichTextLineBreakChar;
                        text.Replace(toReplace, wxT("<br>"));

                        str << text;

                        EndCharacterFormatting(currentCharStyle, charStyle, paraStyle, str);
                    }

                    wxRichTextImage* image = wxDynamicCast(obj, wxRichTextImage);
                    if( image && (!image->IsEmpty() || image->GetImageBlock().GetData()))
                        WriteImage( image, stream );

                    node2 = node2->GetNext();
                }

                EndParagraphFormatting(currentParaStyle, paraStyle, str);

                str << wxT("\n");
            }
            node = node->GetNext();
        }

        CloseLists(-1, str);

        str << wxT("</font>");

        if ((GetFlags() & wxRICHTEXT_HANDLER_NO_HEADER_FOOTER) == 0)
            str << wxT("</body></html>");

        str << wxT("\n");
    }

#if wxUSE_UNICODE
    if (customEncoding)
        delete customEncoding;
#endif

    m_buffer = NULL;

    return true;
}

void wxRichTextHTMLHandler::BeginCharacterFormatting(const wxTextAttrEx& currentStyle, const wxTextAttrEx& thisStyle, const wxTextAttrEx& WXUNUSED(paraStyle), wxTextOutputStream& str)
{
    wxString style;

    // Is there any change in the font properties of the item?
    if (thisStyle.GetFont().GetFaceName() != currentStyle.GetFont().GetFaceName())
    {
        wxString faceName(thisStyle.GetFont().GetFaceName());
        style += wxString::Format(wxT(" face=\"%s\""), faceName.c_str());
    }
    if (thisStyle.GetFont().GetPointSize() != currentStyle.GetFont().GetPointSize())
        style += wxString::Format(wxT(" size=\"%ld\""), PtToSize(thisStyle.GetFont().GetPointSize()));
    if (thisStyle.GetTextColour() != currentStyle.GetTextColour() )
    {
        wxString color(thisStyle.GetTextColour().GetAsString(wxC2S_HTML_SYNTAX));
        style += wxString::Format(wxT(" color=\"%s\""), color.c_str());
    }

    if (style.size())
    {
        str << wxString::Format(wxT("<font %s >"), style.c_str());
        m_font = true;
    }

    if (thisStyle.GetFont().GetWeight() == wxBOLD)
        str << wxT("<b>");
    if (thisStyle.GetFont().GetStyle() == wxITALIC)
        str << wxT("<i>");
    if (thisStyle.GetFont().GetUnderlined())
        str << wxT("<u>");

    if (thisStyle.HasURL())
        str << wxT("<a href=\"") << thisStyle.GetURL() << wxT("\">");
}

void wxRichTextHTMLHandler::EndCharacterFormatting(const wxTextAttrEx& WXUNUSED(currentStyle), const wxTextAttrEx& thisStyle, const wxTextAttrEx& WXUNUSED(paraStyle), wxTextOutputStream& stream)
{
    if (thisStyle.HasURL())
        stream << wxT("</a>");

    if (thisStyle.GetFont().GetUnderlined())
        stream << wxT("</u>");
    if (thisStyle.GetFont().GetStyle() == wxITALIC)
        stream << wxT("</i>");
    if (thisStyle.GetFont().GetWeight() == wxBOLD)
        stream << wxT("</b>");

    if (m_font)
    {
        m_font = false;
        stream << wxT("</font>");
    }
}

/// Begin paragraph formatting
void wxRichTextHTMLHandler::BeginParagraphFormatting(const wxTextAttrEx& WXUNUSED(currentStyle), const wxTextAttrEx& thisStyle, wxTextOutputStream& str)
{
    if (thisStyle.HasPageBreak())
    {
        str << wxT("<div style=\"page-break-after:always\"></div>\n");
    }

    if (thisStyle.HasLeftIndent() && thisStyle.GetLeftIndent() != 0)
    {
        if (thisStyle.HasBulletStyle())
        {
            int indent = thisStyle.GetLeftIndent();

            // Close levels high than this
            CloseLists(indent, str);

            if (m_indents.GetCount() > 0 && indent == m_indents.Last())
            {
                // Same level, no need to start a new list
            }
            else if (m_indents.GetCount() == 0 || indent > m_indents.Last())
            {
                m_indents.Add(indent);

                wxString tag;
                int listType = TypeOfList(thisStyle, tag);
                m_listTypes.Add(listType);

                // wxHTML needs an extra <p> before a list when using <p> ... </p> in previous paragraphs.
                // TODO: pass a flag that indicates we're using wxHTML.
                str << wxT("<p>\n");

                str << tag;
            }

            str << wxT("<li> ");
        }
        else
        {
            CloseLists(-1, str);

            wxString align = GetAlignment(thisStyle);
            str << wxString::Format(wxT("<p align=\"%s\""), align.c_str());

            wxString styleStr;

            if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) && thisStyle.HasParagraphSpacingBefore())
            {
                float spacingBeforeMM = thisStyle.GetParagraphSpacingBefore() / 10.0;

                styleStr += wxString::Format(wxT("margin-top: %.2fmm; "), spacingBeforeMM);
            }
            if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) && thisStyle.HasParagraphSpacingAfter())
            {
                float spacingAfterMM = thisStyle.GetParagraphSpacingAfter() / 10.0;

                styleStr += wxString::Format(wxT("margin-bottom: %.2fmm; "), spacingAfterMM);
            }

            float indentLeftMM = (thisStyle.GetLeftIndent() + thisStyle.GetLeftSubIndent())/10.0;
            if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) && (indentLeftMM > 0.0))
            {
                styleStr += wxString::Format(wxT("margin-left: %.2fmm; "), indentLeftMM);
            }
            float indentRightMM = thisStyle.GetRightIndent()/10.0;
            if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) && thisStyle.HasRightIndent() && (indentRightMM > 0.0))
            {
                styleStr += wxString::Format(wxT("margin-right: %.2fmm; "), indentRightMM);
            }
            // First line indentation
            float firstLineIndentMM = - thisStyle.GetLeftSubIndent() / 10.0;
            if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) && (firstLineIndentMM > 0.0))
            {
                styleStr += wxString::Format(wxT("text-indent: %.2fmm; "), firstLineIndentMM);
            }

            if (!styleStr.IsEmpty())
                str << wxT(" style=\"") << styleStr << wxT("\"");

            str << wxT(">");

            // TODO: convert to pixels
            int indentPixels = indentLeftMM*10/4;

            if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) == 0)
            {
                // Use a table to do indenting if we don't have CSS
                str << wxString::Format(wxT("<table border=0 cellpadding=0 cellspacing=0><tr><td width=\"%d\"></td><td>"), indentPixels);
                m_inTable = true;
            }

            if (((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) == 0) && (thisStyle.GetLeftSubIndent() < 0))
            {
                str << SymbolicIndent( - thisStyle.GetLeftSubIndent());
            }
        }
    }
    else
    {
        CloseLists(-1, str);

        wxString align = GetAlignment(thisStyle);
        str << wxString::Format(wxT("<p align=\"%s\""), align.c_str());

        wxString styleStr;

        if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) && thisStyle.HasParagraphSpacingBefore())
        {
            float spacingBeforeMM = thisStyle.GetParagraphSpacingBefore() / 10.0;

            styleStr += wxString::Format(wxT("margin-top: %.2fmm; "), spacingBeforeMM);
        }
        if ((GetFlags() & wxRICHTEXT_HANDLER_USE_CSS) && thisStyle.HasParagraphSpacingAfter())
        {
            float spacingAfterMM = thisStyle.GetParagraphSpacingAfter() / 10.0;

            styleStr += wxString::Format(wxT("margin-bottom: %.2fmm; "), spacingAfterMM);
        }

        if (!styleStr.IsEmpty())
            str << wxT(" style=\"") << styleStr << wxT("\"");

        str << wxT(">");
    }
    OutputFont(thisStyle, str);
}

/// End paragraph formatting
void wxRichTextHTMLHandler::EndParagraphFormatting(const wxTextAttrEx& WXUNUSED(currentStyle), const wxTextAttrEx& thisStyle, wxTextOutputStream& stream)
{
    if (thisStyle.HasFont())
        stream << wxT("</font>");

    if (m_inTable)
    {
        stream << wxT("</td></tr></table></p>\n");
        m_inTable = false;
    }
    else if (!thisStyle.HasBulletStyle())
        stream << wxT("</p>\n");
}

/// Closes lists to level (-1 means close all)
void wxRichTextHTMLHandler::CloseLists(int level, wxTextOutputStream& str)
{
    // Close levels high than this
    int i = m_indents.GetCount()-1;
    while (i >= 0)
    {
        int l = m_indents[i];
        if (l > level)
        {
            if (m_listTypes[i] == 0)
                str << wxT("</ol>");
            else
                str << wxT("</ul>");
            m_indents.RemoveAt(i);
            m_listTypes.RemoveAt(i);
        }
        else
            break;
        i --;
     }
}

/// Output font tag
void wxRichTextHTMLHandler::OutputFont(const wxTextAttrEx& style, wxTextOutputStream& stream)
{
    if (style.HasFont())
    {
        stream << wxString::Format(wxT("<font face=\"%s\" size=\"%ld\""), style.GetFont().GetFaceName().c_str(), PtToSize(style.GetFont().GetPointSize()));
        if (style.HasTextColour())
            stream << wxString::Format(wxT(" color=\"%s\""), style.GetTextColour().GetAsString(wxC2S_HTML_SYNTAX).c_str());
        stream << wxT(" >");
    }
}

int wxRichTextHTMLHandler::TypeOfList( const wxTextAttrEx& thisStyle, wxString& tag )
{
    // We can use number attribute of li tag but not all the browsers support it.
    // also wxHtmlWindow doesn't support type attribute.

    bool m_is_ul = false;
    if (thisStyle.GetBulletStyle() == (wxTEXT_ATTR_BULLET_STYLE_ARABIC|wxTEXT_ATTR_BULLET_STYLE_PERIOD))
        tag = wxT("<ol type=\"1\">");
    else if (thisStyle.GetBulletStyle() == wxTEXT_ATTR_BULLET_STYLE_LETTERS_UPPER)
        tag = wxT("<ol type=\"A\">");
    else if (thisStyle.GetBulletStyle() == wxTEXT_ATTR_BULLET_STYLE_LETTERS_LOWER)
        tag = wxT("<ol type=\"a\">");
    else if (thisStyle.GetBulletStyle() == wxTEXT_ATTR_BULLET_STYLE_ROMAN_UPPER)
        tag = wxT("<ol type=\"I\">");
    else if (thisStyle.GetBulletStyle() == wxTEXT_ATTR_BULLET_STYLE_ROMAN_LOWER)
        tag = wxT("<ol type=\"i\">");
    else
    {
        tag = wxT("<ul>");
        m_is_ul = true;
    }

    if (m_is_ul)
        return 1;
    else
        return 0;
}

wxString wxRichTextHTMLHandler::GetAlignment( const wxTextAttrEx& thisStyle )
{
    switch( thisStyle.GetAlignment() )
    {
    case wxTEXT_ALIGNMENT_LEFT:
        return  wxT("left");
    case wxTEXT_ALIGNMENT_RIGHT:
        return wxT("right");
    case wxTEXT_ALIGNMENT_CENTER:
        return wxT("center");
    case wxTEXT_ALIGNMENT_JUSTIFIED:
        return wxT("justify");
    default:
        return wxT("left");
    }
}

void wxRichTextHTMLHandler::WriteImage(wxRichTextImage* image, wxOutputStream& stream)
{
    wxTextOutputStream str(stream);

    str << wxT("<img src=\"");

#if wxUSE_FILESYSTEM
    if (GetFlags() & wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_MEMORY)
    {
        if (!image->GetImage().Ok() && image->GetImageBlock().GetData())
            image->LoadFromBlock();
        if (image->GetImage().Ok() && !image->GetImageBlock().GetData())
            image->MakeBlock();

        if (image->GetImage().Ok())
        {
            wxString ext(image->GetImageBlock().GetExtension());
            wxString tempFilename(wxString::Format(wxT("image%d.%s"), sm_fileCounter, (const wxChar*) ext));
            wxMemoryFSHandler::AddFile(tempFilename, image->GetImage(), image->GetImageBlock().GetImageType());

            m_imageLocations.Add(tempFilename);

            str << wxT("memory:") << tempFilename;
        }
        else
            str << wxT("memory:?");

        sm_fileCounter ++;
    }
    else if (GetFlags() & wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_FILES)
    {
        if (!image->GetImage().Ok() && image->GetImageBlock().GetData())
            image->LoadFromBlock();
        if (image->GetImage().Ok() && !image->GetImageBlock().GetData())
            image->MakeBlock();

        if (image->GetImage().Ok())
        {
            wxString tempDir(GetTempDir());
            if (tempDir.IsEmpty())
                tempDir = wxFileName::GetTempDir();

            wxString ext(image->GetImageBlock().GetExtension());
            wxString tempFilename(wxString::Format(wxT("%s/image%d.%s"), (const wxChar*) tempDir, sm_fileCounter, (const wxChar*) ext));
            image->GetImageBlock().Write(tempFilename);

            m_imageLocations.Add(tempFilename);

            str << wxFileSystem::FileNameToURL(tempFilename);
        }
        else
            str << wxT("file:?");

        sm_fileCounter ++;
    }
    else // if (GetFlags() & wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_BASE64) // this is implied
#endif
    {
        str << wxT("data:");
        str << GetMimeType(image->GetImageBlock().GetImageType());
        str << wxT(";base64,");

        if (image->GetImage().Ok() && !image->GetImageBlock().GetData())
            image->MakeBlock();

        wxChar* data = b64enc( image->GetImageBlock().GetData(), image->GetImageBlock().GetDataSize() );
        str << data;

        delete[] data;
    }

    str << wxT("\" />");
}

long wxRichTextHTMLHandler::PtToSize(long size)
{
    int i;
    int len = m_fontSizeMapping.GetCount();
    for (i = 0; i < len; i++)
        if (size <= m_fontSizeMapping[i])
            return i+1;
    return 7;
}

wxString wxRichTextHTMLHandler::SymbolicIndent(long indent)
{
    wxString in;
    for(;indent > 0; indent -= 20)
        in.Append( wxT("&nbsp;") );
    return in;
}

const wxChar* wxRichTextHTMLHandler::GetMimeType(int imageType)
{
    switch(imageType)
    {
    case wxBITMAP_TYPE_BMP:
        return wxT("image/bmp");
    case wxBITMAP_TYPE_TIF:
        return wxT("image/tiff");
    case wxBITMAP_TYPE_GIF:
        return wxT("image/gif");
    case wxBITMAP_TYPE_PNG:
        return wxT("image/png");
    case wxBITMAP_TYPE_JPEG:
        return wxT("image/jpeg");
    default:
        return wxT("image/unknown");
    }
}

// exim-style base64 encoder
wxChar* wxRichTextHTMLHandler::b64enc( unsigned char* input, size_t in_len )
{
    // elements of enc64 array must be 8 bit values
    // otherwise encoder will fail
    // hmmm.. Does wxT macro define a char as 16 bit value
    // when compiling with UNICODE option?
    static const wxChar enc64[] = wxT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
    wxChar* output = new wxChar[4*((in_len+2)/3)+1];
    wxChar* p = output;

    while( in_len-- > 0 )
    {
        register wxChar a, b;

        a = *input++;

        *p++ = enc64[ (a >> 2) & 0x3f ];

        if( in_len-- == 0 )
        {
            *p++ = enc64[ (a << 4 ) & 0x30 ];
            *p++ = '=';
            *p++ = '=';
            break;
        }

        b = *input++;

        *p++ = enc64[(( a << 4 ) | ((b >> 4) &0xf )) & 0x3f];

        if( in_len-- == 0 )
        {
            *p++ = enc64[ (b << 2) & 0x3f ];
            *p++ = '=';
            break;
        }

        a = *input++;

        *p++ = enc64[ ((( b << 2 ) & 0x3f ) | ((a >> 6)& 0x3)) & 0x3f ];

        *p++ = enc64[ a & 0x3f ];
    }
    *p = 0;

    return output;
}
#endif
// wxUSE_STREAMS

/// Delete the in-memory or temporary files generated by the last operation
bool wxRichTextHTMLHandler::DeleteTemporaryImages()
{
    return DeleteTemporaryImages(GetFlags(), m_imageLocations);
}

/// Delete the in-memory or temporary files generated by the last operation
bool wxRichTextHTMLHandler::DeleteTemporaryImages(int flags, const wxArrayString& imageLocations)
{
    size_t i;
    for (i = 0; i < imageLocations.GetCount(); i++)
    {
        wxString location = imageLocations[i];

        if (flags & wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_MEMORY)
        {
#if wxUSE_FILESYSTEM
            wxMemoryFSHandler::RemoveFile(location);
#endif
        }
        else if (flags & wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_FILES)
        {
            if (wxFileExists(location))
                wxRemoveFile(location);
        }
    }

    return true;
}


#endif
// wxUSE_RICHTEXT

