/////////////////////////////////////////////////////////////////////////////
// Name:        wx/richtext/richtextbuffer.h
// Purpose:     Buffer for wxRichTextCtrl
// Author:      Julian Smart
// Modified by:
// Created:     2005-09-30
// RCS-ID:      $Id: richtextbuffer.h 64777 2010-06-28 21:21:53Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RICHTEXTBUFFER_H_
#define _WX_RICHTEXTBUFFER_H_

/*

  Data structures
  ===============

  Data is represented by a hierarchy of objects, all derived from
  wxRichTextObject.

  The top of the hierarchy is the buffer, a kind of wxRichTextParagraphLayoutBox.
  These boxes will allow flexible placement of text boxes on a page, but
  for now there is a single box representing the document, and this box is
  a wxRichTextParagraphLayoutBox which contains further wxRichTextParagraph
  objects, each of which can include text and images.

  Each object maintains a range (start and end position) measured
  from the start of the main parent box.
  A paragraph object knows its range, and a text fragment knows its range
  too. So, a character or image in a page has a position relative to the
  start of the document, and a character in an embedded text box has
  a position relative to that text box. For now, we will not be dealing with
  embedded objects but it's something to bear in mind for later.

  Note that internally, a range (5,5) represents a range of one character.
  In the public wx[Rich]TextCtrl API, this would be passed to e.g. SetSelection
  as (5,6). A paragraph with one character might have an internal range of (0, 1)
  since the end of the paragraph takes up one position.

  Layout
  ======

  When Layout is called on an object, it is given a size which the object
  must limit itself to, or one or more flexible directions (vertical
  or horizontal). So for example a centered paragraph is given the page
  width to play with (minus any margins), but can extend indefinitely
  in the vertical direction. The implementation of Layout can then
  cache the calculated size and position within the parent.

 */

/*!
 * Includes
 */

#include "wx/defs.h"

#if wxUSE_RICHTEXT

#include "wx/list.h"
#include "wx/textctrl.h"
#include "wx/bitmap.h"
#include "wx/image.h"
#include "wx/cmdproc.h"
#include "wx/txtstrm.h"

#if wxUSE_DATAOBJ
#include "wx/dataobj.h"
#endif

// Setting wxRICHTEXT_USE_OWN_CARET to 1 implements a
// caret reliably without using wxClientDC in case there
// are platform-specific problems with the generic caret.
#if defined(__WXGTK__) || (defined(wxMAC_USE_CORE_GRAPHICS) && wxMAC_USE_CORE_GRAPHICS)
#define wxRICHTEXT_USE_OWN_CARET 1
#else
#define wxRICHTEXT_USE_OWN_CARET 0
#endif

// Switch off for binary compatibility, on for faster drawing
#define wxRICHTEXT_USE_OPTIMIZED_LINE_DRAWING 0

/*!
 * Special characters
 */

extern WXDLLIMPEXP_RICHTEXT const wxChar wxRichTextLineBreakChar;

/*!
 * File types
 */

#define wxRICHTEXT_TYPE_ANY             0
#define wxRICHTEXT_TYPE_TEXT            1
#define wxRICHTEXT_TYPE_XML             2
#define wxRICHTEXT_TYPE_HTML            3
#define wxRICHTEXT_TYPE_RTF             4
#define wxRICHTEXT_TYPE_PDF             5

/*!
 * Forward declarations
 */

class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextCtrl;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextObject;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextCacheObject;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextObjectList;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextLine;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextParagraph;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextFileHandler;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextStyleSheet;
class WXDLLIMPEXP_FWD_RICHTEXT wxTextAttrEx;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextListStyleDefinition;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextEvent;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextRenderer;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextBuffer;

/*!
 * Flags determining the available space, passed to Layout
 */

#define wxRICHTEXT_FIXED_WIDTH      0x01
#define wxRICHTEXT_FIXED_HEIGHT     0x02
#define wxRICHTEXT_VARIABLE_WIDTH   0x04
#define wxRICHTEXT_VARIABLE_HEIGHT  0x08

// Only lay out the part of the buffer that lies within
// the rect passed to Layout.
#define wxRICHTEXT_LAYOUT_SPECIFIED_RECT 0x10

/*!
 * Flags to pass to Draw
 */

// Ignore paragraph cache optimization, e.g. for printing purposes
// where one line may be drawn higher (on the next page) compared
// with the previous line
#define wxRICHTEXT_DRAW_IGNORE_CACHE    0x01

/*!
 * Flags returned from hit-testing
 */

// The point was not on this object
#define wxRICHTEXT_HITTEST_NONE     0x01
// The point was before the position returned from HitTest
#define wxRICHTEXT_HITTEST_BEFORE   0x02
// The point was after the position returned from HitTest
#define wxRICHTEXT_HITTEST_AFTER    0x04
// The point was on the position returned from HitTest
#define wxRICHTEXT_HITTEST_ON       0x08
// The point was on space outside content
#define wxRICHTEXT_HITTEST_OUTSIDE  0x10

/*!
 * Flags for GetRangeSize
 */

#define wxRICHTEXT_FORMATTED        0x01
#define wxRICHTEXT_UNFORMATTED      0x02
#define wxRICHTEXT_CACHE_SIZE       0x04
#define wxRICHTEXT_HEIGHT_ONLY      0x08

/*!
 * Flags for SetStyle/SetListStyle
 */

#define wxRICHTEXT_SETSTYLE_NONE            0x00

// Specifies that this operation should be undoable
#define wxRICHTEXT_SETSTYLE_WITH_UNDO       0x01

// Specifies that the style should not be applied if the
// combined style at this point is already the style in question.
#define wxRICHTEXT_SETSTYLE_OPTIMIZE        0x02

// Specifies that the style should only be applied to paragraphs,
// and not the content. This allows content styling to be
// preserved independently from that of e.g. a named paragraph style.
#define wxRICHTEXT_SETSTYLE_PARAGRAPHS_ONLY 0x04

// Specifies that the style should only be applied to characters,
// and not the paragraph. This allows content styling to be
// preserved independently from that of e.g. a named paragraph style.
#define wxRICHTEXT_SETSTYLE_CHARACTERS_ONLY 0x08

// For SetListStyle only: specifies starting from the given number, otherwise
// deduces number from existing attributes
#define wxRICHTEXT_SETSTYLE_RENUMBER        0x10

// For SetListStyle only: specifies the list level for all paragraphs, otherwise
// the current indentation will be used
#define wxRICHTEXT_SETSTYLE_SPECIFY_LEVEL   0x20

// Resets the existing style before applying the new style
#define wxRICHTEXT_SETSTYLE_RESET           0x40

// Removes the given style instead of applying it
#define wxRICHTEXT_SETSTYLE_REMOVE          0x80

/*!
 * Flags for text insertion
 */

#define wxRICHTEXT_INSERT_NONE                              0x00
#define wxRICHTEXT_INSERT_WITH_PREVIOUS_PARAGRAPH_STYLE     0x01
#define wxRICHTEXT_INSERT_INTERACTIVE                       0x02

/*!
 * Extra formatting flags not in wxTextAttr
 */

#define wxTEXT_ATTR_PARA_SPACING_AFTER      0x00000800
#define wxTEXT_ATTR_PARA_SPACING_BEFORE     0x00001000
#define wxTEXT_ATTR_LINE_SPACING            0x00002000
#define wxTEXT_ATTR_CHARACTER_STYLE_NAME    0x00004000
#define wxTEXT_ATTR_PARAGRAPH_STYLE_NAME    0x00008000
#define wxTEXT_ATTR_LIST_STYLE_NAME         0x00010000
#define wxTEXT_ATTR_BULLET_STYLE            0x00020000
#define wxTEXT_ATTR_BULLET_NUMBER           0x00040000
#define wxTEXT_ATTR_BULLET_TEXT             0x00080000
#define wxTEXT_ATTR_BULLET_NAME             0x00100000
#define wxTEXT_ATTR_URL                     0x00200000
#define wxTEXT_ATTR_PAGE_BREAK              0x00400000
#define wxTEXT_ATTR_EFFECTS                 0x00800000
#define wxTEXT_ATTR_OUTLINE_LEVEL           0x01000000

// A special flag telling the buffer to keep the first paragraph style
// as-is, when deleting a paragraph marker. In future we might pass a
// flag to InsertFragment and DeleteRange to indicate the appropriate mode.
#define wxTEXT_ATTR_KEEP_FIRST_PARA_STYLE   0x10000000

/*!
 * Styles for wxTextAttrEx::SetBulletStyle
 */

#define wxTEXT_ATTR_BULLET_STYLE_NONE               0x00000000
#define wxTEXT_ATTR_BULLET_STYLE_ARABIC             0x00000001
#define wxTEXT_ATTR_BULLET_STYLE_LETTERS_UPPER      0x00000002
#define wxTEXT_ATTR_BULLET_STYLE_LETTERS_LOWER      0x00000004
#define wxTEXT_ATTR_BULLET_STYLE_ROMAN_UPPER        0x00000008
#define wxTEXT_ATTR_BULLET_STYLE_ROMAN_LOWER        0x00000010
#define wxTEXT_ATTR_BULLET_STYLE_SYMBOL             0x00000020
#define wxTEXT_ATTR_BULLET_STYLE_BITMAP             0x00000040
#define wxTEXT_ATTR_BULLET_STYLE_PARENTHESES        0x00000080
#define wxTEXT_ATTR_BULLET_STYLE_PERIOD             0x00000100
#define wxTEXT_ATTR_BULLET_STYLE_STANDARD           0x00000200
#define wxTEXT_ATTR_BULLET_STYLE_RIGHT_PARENTHESIS  0x00000400
#define wxTEXT_ATTR_BULLET_STYLE_OUTLINE            0x00000800

#define wxTEXT_ATTR_BULLET_STYLE_ALIGN_LEFT         0x00000000
#define wxTEXT_ATTR_BULLET_STYLE_ALIGN_RIGHT        0x00001000
#define wxTEXT_ATTR_BULLET_STYLE_ALIGN_CENTRE       0x00002000

/*!
 * Styles for wxTextAttrEx::SetTextEffects
 */

#define wxTEXT_ATTR_EFFECT_NONE                     0x00000000
#define wxTEXT_ATTR_EFFECT_CAPITALS                 0x00000001
#define wxTEXT_ATTR_EFFECT_SMALL_CAPITALS           0x00000002
#define wxTEXT_ATTR_EFFECT_STRIKETHROUGH            0x00000004
#define wxTEXT_ATTR_EFFECT_DOUBLE_STRIKETHROUGH     0x00000008
#define wxTEXT_ATTR_EFFECT_SHADOW                   0x00000010
#define wxTEXT_ATTR_EFFECT_EMBOSS                   0x00000020
#define wxTEXT_ATTR_EFFECT_OUTLINE                  0x00000040
#define wxTEXT_ATTR_EFFECT_ENGRAVE                  0x00000080
#define wxTEXT_ATTR_EFFECT_SUPERSCRIPT              0x00000100
#define wxTEXT_ATTR_EFFECT_SUBSCRIPT                0x00000200

/*!
 * Line spacing values
 */

#define wxTEXT_ATTR_LINE_SPACING_NORMAL         10
#define wxTEXT_ATTR_LINE_SPACING_HALF           15
#define wxTEXT_ATTR_LINE_SPACING_TWICE          20

/*!
 * Character and paragraph combined styles
 */

#define wxTEXT_ATTR_CHARACTER (wxTEXT_ATTR_FONT|wxTEXT_ATTR_EFFECTS|wxTEXT_ATTR_BACKGROUND_COLOUR|wxTEXT_ATTR_TEXT_COLOUR|wxTEXT_ATTR_CHARACTER_STYLE_NAME|wxTEXT_ATTR_URL)

#define wxTEXT_ATTR_PARAGRAPH (wxTEXT_ATTR_ALIGNMENT|wxTEXT_ATTR_LEFT_INDENT|wxTEXT_ATTR_RIGHT_INDENT|wxTEXT_ATTR_TABS|\
    wxTEXT_ATTR_PARA_SPACING_BEFORE|wxTEXT_ATTR_PARA_SPACING_AFTER|wxTEXT_ATTR_LINE_SPACING|\
    wxTEXT_ATTR_BULLET_STYLE|wxTEXT_ATTR_BULLET_NUMBER|wxTEXT_ATTR_BULLET_TEXT|wxTEXT_ATTR_BULLET_NAME|\
    wxTEXT_ATTR_PARAGRAPH_STYLE_NAME|wxTEXT_ATTR_LIST_STYLE_NAME|wxTEXT_ATTR_OUTLINE_LEVEL|wxTEXT_ATTR_PAGE_BREAK)

#define wxTEXT_ATTR_ALL (wxTEXT_ATTR_CHARACTER|wxTEXT_ATTR_PARAGRAPH)

/*!
 * Default superscript/subscript font multiplication factor
 */

#define wxSCRIPT_MUL_FACTOR             1.5

/*!
 * wxRichTextRange class declaration
 * This stores beginning and end positions for a range of data.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextRange
{
public:
// Constructors

    wxRichTextRange() { m_start = 0; m_end = 0; }
    wxRichTextRange(long start, long end) { m_start = start; m_end = end; }
    wxRichTextRange(const wxRichTextRange& range) { m_start = range.m_start; m_end = range.m_end; }
    ~wxRichTextRange() {}

    void operator =(const wxRichTextRange& range) { m_start = range.m_start; m_end = range.m_end; }
    bool operator ==(const wxRichTextRange& range) const { return (m_start == range.m_start && m_end == range.m_end); }
    bool operator !=(const wxRichTextRange& range) const { return (m_start != range.m_start || m_end != range.m_end); }
    wxRichTextRange operator -(const wxRichTextRange& range) const { return wxRichTextRange(m_start - range.m_start, m_end - range.m_end); }
    wxRichTextRange operator +(const wxRichTextRange& range) const { return wxRichTextRange(m_start + range.m_start, m_end + range.m_end); }

    void SetRange(long start, long end) { m_start = start; m_end = end; }

    void SetStart(long start) { m_start = start; }
    long GetStart() const { return m_start; }

    void SetEnd(long end) { m_end = end; }
    long GetEnd() const { return m_end; }

    /// Returns true if this range is completely outside 'range'
    bool IsOutside(const wxRichTextRange& range) const { return range.m_start > m_end || range.m_end < m_start; }

    /// Returns true if this range is completely within 'range'
    bool IsWithin(const wxRichTextRange& range) const { return m_start >= range.m_start && m_end <= range.m_end; }

    /// Returns true if the given position is within this range. Allow
    /// for the possibility of an empty range - assume the position
    /// is within this empty range. NO, I think we should not match with an empty range.
    // bool Contains(long pos) const { return pos >= m_start && (pos <= m_end || GetLength() == 0); }
    bool Contains(long pos) const { return pos >= m_start && pos <= m_end ; }

    /// Limit this range to be within 'range'
    bool LimitTo(const wxRichTextRange& range) ;

    /// Gets the length of the range
    long GetLength() const { return m_end - m_start + 1; }

    /// Swaps the start and end
    void Swap() { long tmp = m_start; m_start = m_end; m_end = tmp; }

    /// Convert to internal form: (n, n) is the range of a single character.
    wxRichTextRange ToInternal() const { return wxRichTextRange(m_start, m_end-1); }

    /// Convert from internal to public API form: (n, n+1) is the range of a single character.
    wxRichTextRange FromInternal() const { return wxRichTextRange(m_start, m_end+1); }

protected:
    long m_start;
    long m_end;
};

#define wxRICHTEXT_ALL  wxRichTextRange(-2, -2)
#define wxRICHTEXT_NONE  wxRichTextRange(-1, -1)

/*!
 * wxTextAttrEx is an extended version of wxTextAttr with more paragraph attributes.
 */

class WXDLLIMPEXP_RICHTEXT wxTextAttrEx: public wxTextAttr
{
public:
    // ctors
    wxTextAttrEx(const wxTextAttrEx& attr);
    wxTextAttrEx(const wxTextAttr& attr) { Init(); (*this) = attr; }
    wxTextAttrEx() { Init(); }

    // Initialise this object
    void Init();

    // Copy
    void Copy(const wxTextAttrEx& attr);

    // Assignment from a wxTextAttrEx object
    void operator= (const wxTextAttrEx& attr);

    // Assignment from a wxTextAttr object
    void operator= (const wxTextAttr& attr);

    // Equality test
    bool operator== (const wxTextAttrEx& attr) const;

    // setters
    void SetCharacterStyleName(const wxString& name) { m_characterStyleName = name; SetFlags(GetFlags() | wxTEXT_ATTR_CHARACTER_STYLE_NAME); }
    void SetParagraphStyleName(const wxString& name) { m_paragraphStyleName = name; SetFlags(GetFlags() | wxTEXT_ATTR_PARAGRAPH_STYLE_NAME); }
    void SetListStyleName(const wxString& name) { m_listStyleName = name; SetFlags(GetFlags() | wxTEXT_ATTR_LIST_STYLE_NAME); }
    void SetParagraphSpacingAfter(int spacing) { m_paragraphSpacingAfter = spacing; SetFlags(GetFlags() | wxTEXT_ATTR_PARA_SPACING_AFTER); }
    void SetParagraphSpacingBefore(int spacing) { m_paragraphSpacingBefore = spacing; SetFlags(GetFlags() | wxTEXT_ATTR_PARA_SPACING_BEFORE); }
    void SetLineSpacing(int spacing) { m_lineSpacing = spacing; SetFlags(GetFlags() | wxTEXT_ATTR_LINE_SPACING); }
    void SetBulletStyle(int style) { m_bulletStyle = style; SetFlags(GetFlags() | wxTEXT_ATTR_BULLET_STYLE); }
    void SetBulletNumber(int n) { m_bulletNumber = n; SetFlags(GetFlags() | wxTEXT_ATTR_BULLET_NUMBER); }
    void SetBulletText(const wxString& text) { m_bulletText = text; SetFlags(GetFlags() | wxTEXT_ATTR_BULLET_TEXT); }
    void SetBulletName(const wxString& name) { m_bulletName = name; SetFlags(GetFlags() | wxTEXT_ATTR_BULLET_NAME); }
    void SetBulletFont(const wxString& bulletFont) { m_bulletFont = bulletFont; }
    void SetURL(const wxString& url) { m_urlTarget = url; SetFlags(GetFlags() | wxTEXT_ATTR_URL); }
    void SetPageBreak(bool pageBreak = true) { SetFlags(pageBreak ? (GetFlags() | wxTEXT_ATTR_PAGE_BREAK) : (GetFlags() & ~wxTEXT_ATTR_PAGE_BREAK)); }
    void SetTextEffects(int effects) { m_textEffects = effects; SetFlags(GetFlags() | wxTEXT_ATTR_EFFECTS); }
    void SetTextEffectFlags(int effects) { m_textEffectFlags = effects; }
    void SetOutlineLevel(int level) { m_outlineLevel = level; SetFlags(GetFlags() | wxTEXT_ATTR_OUTLINE_LEVEL); }

    const wxString& GetCharacterStyleName() const { return m_characterStyleName; }
    const wxString& GetParagraphStyleName() const { return m_paragraphStyleName; }
    const wxString& GetListStyleName() const { return m_listStyleName; }
    int GetParagraphSpacingAfter() const { return m_paragraphSpacingAfter; }
    int GetParagraphSpacingBefore() const { return m_paragraphSpacingBefore; }
    int GetLineSpacing() const { return m_lineSpacing; }
    int GetBulletStyle() const { return m_bulletStyle; }
    int GetBulletNumber() const { return m_bulletNumber; }
    const wxString& GetBulletText() const { return m_bulletText; }
    const wxString& GetBulletName() const { return m_bulletName; }
    const wxString& GetBulletFont() const { return m_bulletFont; }
    const wxString& GetURL() const { return m_urlTarget; }
    int GetTextEffects() const { return m_textEffects; }
    int GetTextEffectFlags() const { return m_textEffectFlags; }
    int GetOutlineLevel() const { return m_outlineLevel; }

    bool HasFontWeight() const { return (GetFlags() & wxTEXT_ATTR_FONT_WEIGHT) != 0; }
    bool HasFontSize() const { return (GetFlags() & wxTEXT_ATTR_FONT_SIZE) != 0; }
    bool HasFontItalic() const { return (GetFlags() & wxTEXT_ATTR_FONT_ITALIC) != 0; }
    bool HasFontUnderlined() const { return (GetFlags() & wxTEXT_ATTR_FONT_UNDERLINE) != 0; }
    bool HasFontFaceName() const { return (GetFlags() & wxTEXT_ATTR_FONT_FACE) != 0; }

    bool HasParagraphSpacingAfter() const { return HasFlag(wxTEXT_ATTR_PARA_SPACING_AFTER); }
    bool HasParagraphSpacingBefore() const { return HasFlag(wxTEXT_ATTR_PARA_SPACING_BEFORE); }
    bool HasLineSpacing() const { return HasFlag(wxTEXT_ATTR_LINE_SPACING); }
    bool HasCharacterStyleName() const { return HasFlag(wxTEXT_ATTR_CHARACTER_STYLE_NAME) && !m_characterStyleName.IsEmpty(); }
    bool HasParagraphStyleName() const { return HasFlag(wxTEXT_ATTR_PARAGRAPH_STYLE_NAME) && !m_paragraphStyleName.IsEmpty(); }
    bool HasListStyleName() const { return HasFlag(wxTEXT_ATTR_LIST_STYLE_NAME) || !m_listStyleName.IsEmpty(); }
    bool HasBulletStyle() const { return HasFlag(wxTEXT_ATTR_BULLET_STYLE); }
    bool HasBulletNumber() const { return HasFlag(wxTEXT_ATTR_BULLET_NUMBER); }
    bool HasBulletText() const { return HasFlag(wxTEXT_ATTR_BULLET_TEXT); }
    bool HasBulletName() const { return HasFlag(wxTEXT_ATTR_BULLET_NAME); }
    bool HasURL() const { return HasFlag(wxTEXT_ATTR_URL); }
    bool HasPageBreak() const { return HasFlag(wxTEXT_ATTR_PAGE_BREAK); }
    bool HasTextEffects() const { return HasFlag(wxTEXT_ATTR_EFFECTS); }
    bool HasTextEffect(int effect) const { return HasFlag(wxTEXT_ATTR_EFFECTS) && ((GetTextEffectFlags() & effect) != 0); }
    bool HasOutlineLevel() const { return HasFlag(wxTEXT_ATTR_OUTLINE_LEVEL); }

    // Is this a character style?
    bool IsCharacterStyle() const { return (0 != (GetFlags() & wxTEXT_ATTR_CHARACTER)); }
    bool IsParagraphStyle() const { return (0 != (GetFlags() & wxTEXT_ATTR_PARAGRAPH)); }

    // returns false if we have any attributes set, true otherwise
    bool IsDefault() const
    {
        return (GetFlags() == 0);
    }

    // return the attribute having the valid font and colours: it uses the
    // attributes set in attr and falls back first to attrDefault and then to
    // the text control font/colours for those attributes which are not set
    static wxTextAttrEx CombineEx(const wxTextAttrEx& attr,
                              const wxTextAttrEx& attrDef,
                              const wxTextCtrlBase *text);

private:
    // Paragraph styles
    int                 m_paragraphSpacingAfter;
    int                 m_paragraphSpacingBefore;
    int                 m_lineSpacing;
    int                 m_bulletStyle;
    int                 m_bulletNumber;
    int                 m_textEffects;
    int                 m_textEffectFlags;
    int                 m_outlineLevel;
    wxString            m_bulletText;
    wxString            m_bulletFont;
    wxString            m_bulletName;
    wxString            m_urlTarget;

    // Character style
    wxString            m_characterStyleName;

    // Paragraph style
    wxString            m_paragraphStyleName;

    // List style
    wxString            m_listStyleName;
};

/*!
 * wxRichTextAttr stores attributes without a wxFont object, so is a much more
 * efficient way to query styles.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextAttr
{
public:
    // ctors
    wxRichTextAttr(const wxTextAttrEx& attr);
    wxRichTextAttr(const wxRichTextAttr& attr);
    wxRichTextAttr() { Init(); }
    wxRichTextAttr(const wxColour& colText,
               const wxColour& colBack = wxNullColour,
               wxTextAttrAlignment alignment = wxTEXT_ALIGNMENT_DEFAULT);

    // Initialise this object.
    void Init();

    // Copy
    void Copy(const wxRichTextAttr& attr);

    // Assignment from a wxRichTextAttr object.
    void operator= (const wxRichTextAttr& attr);

    // Assignment from a wxTextAttrEx object.
    void operator= (const wxTextAttrEx& attr);

    // Equality test
    bool operator== (const wxRichTextAttr& attr) const;

    // Making a wxTextAttrEx object.
    operator wxTextAttrEx () const ;

    // Create font from font attributes.
    wxFont CreateFont() const;

    // Get attributes from font.
    bool GetFontAttributes(const wxFont& font);

    // setters
    void SetTextColour(const wxColour& colText) { m_colText = colText; m_flags |= wxTEXT_ATTR_TEXT_COLOUR; }
    void SetBackgroundColour(const wxColour& colBack) { m_colBack = colBack; m_flags |= wxTEXT_ATTR_BACKGROUND_COLOUR; }
    void SetAlignment(wxTextAttrAlignment alignment) { m_textAlignment = alignment; m_flags |= wxTEXT_ATTR_ALIGNMENT; }
    void SetTabs(const wxArrayInt& tabs) { m_tabs = tabs; m_flags |= wxTEXT_ATTR_TABS; }
    void SetLeftIndent(int indent, int subIndent = 0) { m_leftIndent = indent; m_leftSubIndent = subIndent; m_flags |= wxTEXT_ATTR_LEFT_INDENT; }
    void SetRightIndent(int indent) { m_rightIndent = indent; m_flags |= wxTEXT_ATTR_RIGHT_INDENT; }

    void SetFontSize(int pointSize) { m_fontSize = pointSize; m_flags |= wxTEXT_ATTR_FONT_SIZE; }
    void SetFontStyle(int fontStyle) { m_fontStyle = fontStyle; m_flags |= wxTEXT_ATTR_FONT_ITALIC; }
    void SetFontWeight(int fontWeight) { m_fontWeight = fontWeight; m_flags |= wxTEXT_ATTR_FONT_WEIGHT; }
    void SetFontFaceName(const wxString& faceName) { m_fontFaceName = faceName; m_flags |= wxTEXT_ATTR_FONT_FACE; }
    void SetFontUnderlined(bool underlined) { m_fontUnderlined = underlined; m_flags |= wxTEXT_ATTR_FONT_UNDERLINE; }

    void SetFlags(long flags) { m_flags = flags; }

    void SetCharacterStyleName(const wxString& name) { m_characterStyleName = name; m_flags |= wxTEXT_ATTR_CHARACTER_STYLE_NAME; }
    void SetParagraphStyleName(const wxString& name) { m_paragraphStyleName = name; m_flags |= wxTEXT_ATTR_PARAGRAPH_STYLE_NAME; }
    void SetListStyleName(const wxString& name) { m_listStyleName = name; SetFlags(GetFlags() | wxTEXT_ATTR_LIST_STYLE_NAME); }
    void SetParagraphSpacingAfter(int spacing) { m_paragraphSpacingAfter = spacing; m_flags |= wxTEXT_ATTR_PARA_SPACING_AFTER; }
    void SetParagraphSpacingBefore(int spacing) { m_paragraphSpacingBefore = spacing; m_flags |= wxTEXT_ATTR_PARA_SPACING_BEFORE; }
    void SetLineSpacing(int spacing) { m_lineSpacing = spacing; m_flags |= wxTEXT_ATTR_LINE_SPACING; }
    void SetBulletStyle(int style) { m_bulletStyle = style; m_flags |= wxTEXT_ATTR_BULLET_STYLE; }
    void SetBulletNumber(int n) { m_bulletNumber = n; m_flags |= wxTEXT_ATTR_BULLET_NUMBER; }
    void SetBulletText(const wxString& text) { m_bulletText = text; m_flags |= wxTEXT_ATTR_BULLET_TEXT; }
    void SetBulletFont(const wxString& bulletFont) { m_bulletFont = bulletFont; }
    void SetBulletName(const wxString& name) { m_bulletName = name; m_flags |= wxTEXT_ATTR_BULLET_NAME; }
    void SetURL(const wxString& url) { m_urlTarget = url; m_flags |= wxTEXT_ATTR_URL; }
    void SetPageBreak(bool pageBreak = true) { SetFlags(pageBreak ? (GetFlags() | wxTEXT_ATTR_PAGE_BREAK) : (GetFlags() & ~wxTEXT_ATTR_PAGE_BREAK)); }
    void SetTextEffects(int effects) { m_textEffects = effects; SetFlags(GetFlags() | wxTEXT_ATTR_EFFECTS); }
    void SetTextEffectFlags(int effects) { m_textEffectFlags = effects; }
    void SetOutlineLevel(int level) { m_outlineLevel = level; SetFlags(GetFlags() | wxTEXT_ATTR_OUTLINE_LEVEL); }

    const wxColour& GetTextColour() const { return m_colText; }
    const wxColour& GetBackgroundColour() const { return m_colBack; }
    wxTextAttrAlignment GetAlignment() const { return m_textAlignment; }
    const wxArrayInt& GetTabs() const { return m_tabs; }
    long GetLeftIndent() const { return m_leftIndent; }
    long GetLeftSubIndent() const { return m_leftSubIndent; }
    long GetRightIndent() const { return m_rightIndent; }
    long GetFlags() const { return m_flags; }

    int GetFontSize() const { return m_fontSize; }
    int GetFontStyle() const { return m_fontStyle; }
    int GetFontWeight() const { return m_fontWeight; }
    bool GetFontUnderlined() const { return m_fontUnderlined; }
    const wxString& GetFontFaceName() const { return m_fontFaceName; }

    const wxString& GetCharacterStyleName() const { return m_characterStyleName; }
    const wxString& GetParagraphStyleName() const { return m_paragraphStyleName; }
    const wxString& GetListStyleName() const { return m_listStyleName; }
    int GetParagraphSpacingAfter() const { return m_paragraphSpacingAfter; }
    int GetParagraphSpacingBefore() const { return m_paragraphSpacingBefore; }
    int GetLineSpacing() const { return m_lineSpacing; }
    int GetBulletStyle() const { return m_bulletStyle; }
    int GetBulletNumber() const { return m_bulletNumber; }
    const wxString& GetBulletText() const { return m_bulletText; }
    const wxString& GetBulletFont() const { return m_bulletFont; }
    const wxString& GetBulletName() const { return m_bulletName; }
    const wxString& GetURL() const { return m_urlTarget; }
    int GetTextEffects() const { return m_textEffects; }
    int GetTextEffectFlags() const { return m_textEffectFlags; }
    int GetOutlineLevel() const { return m_outlineLevel; }

    // accessors
    bool HasTextColour() const { return m_colText.Ok() && HasFlag(wxTEXT_ATTR_TEXT_COLOUR) ; }
    bool HasBackgroundColour() const { return m_colBack.Ok() && HasFlag(wxTEXT_ATTR_BACKGROUND_COLOUR) ; }
    bool HasAlignment() const { return (m_textAlignment != wxTEXT_ALIGNMENT_DEFAULT) && ((m_flags & wxTEXT_ATTR_ALIGNMENT) != 0) ; }
    bool HasTabs() const { return (m_flags & wxTEXT_ATTR_TABS) != 0 ; }
    bool HasLeftIndent() const { return (m_flags & wxTEXT_ATTR_LEFT_INDENT) != 0 ; }
    bool HasRightIndent() const { return (m_flags & wxTEXT_ATTR_RIGHT_INDENT) != 0 ; }
    bool HasFontWeight() const { return (m_flags & wxTEXT_ATTR_FONT_WEIGHT) != 0; }
    bool HasFontSize() const { return (m_flags & wxTEXT_ATTR_FONT_SIZE) != 0; }
    bool HasFontItalic() const { return (m_flags & wxTEXT_ATTR_FONT_ITALIC) != 0; }
    bool HasFontUnderlined() const { return (m_flags & wxTEXT_ATTR_FONT_UNDERLINE) != 0; }
    bool HasFontFaceName() const { return (m_flags & wxTEXT_ATTR_FONT_FACE) != 0; }
    bool HasFont() const { return (m_flags & (wxTEXT_ATTR_FONT)) != 0; }

    bool HasParagraphSpacingAfter() const { return (m_flags & wxTEXT_ATTR_PARA_SPACING_AFTER) != 0; }
    bool HasParagraphSpacingBefore() const { return (m_flags & wxTEXT_ATTR_PARA_SPACING_BEFORE) != 0; }
    bool HasLineSpacing() const { return (m_flags & wxTEXT_ATTR_LINE_SPACING) != 0; }
    bool HasCharacterStyleName() const { return (m_flags & wxTEXT_ATTR_CHARACTER_STYLE_NAME) != 0 && !m_characterStyleName.IsEmpty(); }
    bool HasParagraphStyleName() const { return (m_flags & wxTEXT_ATTR_PARAGRAPH_STYLE_NAME) != 0 && !m_paragraphStyleName.IsEmpty(); }
    bool HasListStyleName() const { return HasFlag(wxTEXT_ATTR_LIST_STYLE_NAME) || !m_listStyleName.IsEmpty(); }
    bool HasBulletStyle() const { return (m_flags & wxTEXT_ATTR_BULLET_STYLE) != 0; }
    bool HasBulletNumber() const { return (m_flags & wxTEXT_ATTR_BULLET_NUMBER) != 0; }
    bool HasBulletText() const { return (m_flags & wxTEXT_ATTR_BULLET_TEXT) != 0; }
    bool HasBulletName() const { return (m_flags & wxTEXT_ATTR_BULLET_NAME) != 0; }
    bool HasURL() const { return HasFlag(wxTEXT_ATTR_URL); }
    bool HasPageBreak() const { return HasFlag(wxTEXT_ATTR_PAGE_BREAK); }
    bool HasTextEffects() const { return HasFlag(wxTEXT_ATTR_EFFECTS); }
    bool HasTextEffect(int effect) const { return HasFlag(wxTEXT_ATTR_EFFECTS) && ((GetTextEffectFlags() & effect) != 0); }
    bool HasOutlineLevel() const { return HasFlag(wxTEXT_ATTR_OUTLINE_LEVEL); }

    bool HasFlag(long flag) const { return (m_flags & flag) != 0; }

    // Is this a character style?
    bool IsCharacterStyle() const { return (0 != (GetFlags() & wxTEXT_ATTR_CHARACTER)); }
    bool IsParagraphStyle() const { return (0 != (GetFlags() & wxTEXT_ATTR_PARAGRAPH)); }

    // returns false if we have any attributes set, true otherwise
    bool IsDefault() const
    {
        return GetFlags() == 0;
    }

    // Merges the given attributes. Does not affect 'this'. If compareWith
    // is non-NULL, then it will be used to mask out those attributes that are the same in style
    // and compareWith, for situations where we don't want to explicitly set inherited attributes.
    bool Apply(const wxRichTextAttr& style, const wxRichTextAttr* compareWith = NULL);

    // Merges the given attributes and returns the result. Does not affect 'this'. If compareWith
    // is non-NULL, then it will be used to mask out those attributes that are the same in style
    // and compareWith, for situations where we don't want to explicitly set inherited attributes.
    wxRichTextAttr Combine(const wxRichTextAttr& style, const wxRichTextAttr* compareWith = NULL) const;

private:
    long                m_flags;

    // Paragraph styles
    wxArrayInt          m_tabs; // array of int: tab stops in 1/10 mm
    int                 m_leftIndent; // left indent in 1/10 mm
    int                 m_leftSubIndent; // left indent for all but the first
                                         // line in a paragraph relative to the
                                         // first line, in 1/10 mm
    int                 m_rightIndent; // right indent in 1/10 mm
    wxTextAttrAlignment m_textAlignment;

    int                 m_paragraphSpacingAfter;
    int                 m_paragraphSpacingBefore;
    int                 m_lineSpacing;
    int                 m_bulletStyle;
    int                 m_bulletNumber;
    int                 m_textEffects;
    int                 m_textEffectFlags;
    int                 m_outlineLevel;
    wxString            m_bulletText;
    wxString            m_bulletFont;
    wxString            m_bulletName;
    wxString            m_urlTarget;

    // Character styles
    wxColour            m_colText,
                        m_colBack;
    int                 m_fontSize;
    int                 m_fontStyle;
    int                 m_fontWeight;
    bool                m_fontUnderlined;
    wxString            m_fontFaceName;

    // Character style
    wxString            m_characterStyleName;

    // Paragraph style
    wxString            m_paragraphStyleName;

    // List style
    wxString            m_listStyleName;
};

/*!
 * wxRichTextObject class declaration
 * This is the base for drawable objects.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextObject: public wxObject
{
    DECLARE_CLASS(wxRichTextObject)
public:
// Constructors

    wxRichTextObject(wxRichTextObject* parent = NULL);
    virtual ~wxRichTextObject();

// Overrideables

    /// Draw the item, within the given range. Some objects may ignore the range (for
    /// example paragraphs) while others must obey it (lines, to implement wrapping)
    virtual bool Draw(wxDC& dc, const wxRichTextRange& range, const wxRichTextRange& selectionRange, const wxRect& rect, int descent, int style) = 0;

    /// Lay the item out at the specified position with the given size constraint.
    /// Layout must set the cached size.
    virtual bool Layout(wxDC& dc, const wxRect& rect, int style) = 0;

    /// Hit-testing: returns a flag indicating hit test details, plus
    /// information about position
    virtual int HitTest(wxDC& WXUNUSED(dc), const wxPoint& WXUNUSED(pt), long& WXUNUSED(textPosition)) { return false; }

    /// Finds the absolute position and row height for the given character position
    virtual bool FindPosition(wxDC& WXUNUSED(dc), long WXUNUSED(index), wxPoint& WXUNUSED(pt), int* WXUNUSED(height), bool WXUNUSED(forceLineStart)) { return false; }

    /// Get the best size, i.e. the ideal starting size for this object irrespective
    /// of available space. For a short text string, it will be the size that exactly encloses
    /// the text. For a longer string, it might use the parent width for example.
    virtual wxSize GetBestSize() const { return m_size; }

    /// Get the object size for the given range. Returns false if the range
    /// is invalid for this object.
    virtual bool GetRangeSize(const wxRichTextRange& range, wxSize& size, int& descent, wxDC& dc, int flags, wxPoint position = wxPoint(0,0)) const  = 0;

    /// Do a split, returning an object containing the second part, and setting
    /// the first part in 'this'.
    virtual wxRichTextObject* DoSplit(long WXUNUSED(pos)) { return NULL; }

    /// Calculate range. By default, guess that the object is 1 unit long.
    virtual void CalculateRange(long start, long& end) { end = start ; m_range.SetRange(start, end); }

    /// Delete range
    virtual bool DeleteRange(const wxRichTextRange& WXUNUSED(range)) { return false; }

    /// Returns true if the object is empty
    virtual bool IsEmpty() const { return false; }

    /// Get any text in this object for the given range
    virtual wxString GetTextForRange(const wxRichTextRange& WXUNUSED(range)) const { return wxEmptyString; }

    /// Returns true if this object can merge itself with the given one.
    virtual bool CanMerge(wxRichTextObject* WXUNUSED(object)) const { return false; }

    /// Returns true if this object merged itself with the given one.
    /// The calling code will then delete the given object.
    virtual bool Merge(wxRichTextObject* WXUNUSED(object)) { return false; }

    /// Dump to output stream for debugging
    virtual void Dump(wxTextOutputStream& stream);

// Accessors

    /// Get/set the cached object size as calculated by Layout.
    virtual wxSize GetCachedSize() const { return m_size; }
    virtual void SetCachedSize(const wxSize& sz) { m_size = sz; }

    /// Get/set the object position
    virtual wxPoint GetPosition() const { return m_pos; }
    virtual void SetPosition(const wxPoint& pos) { m_pos = pos; }

    /// Get the rectangle enclosing the object
    virtual wxRect GetRect() const { return wxRect(GetPosition(), GetCachedSize()); }

    /// Set the range
    void SetRange(const wxRichTextRange& range) { m_range = range; }

    /// Get the range
    const wxRichTextRange& GetRange() const { return m_range; }
    wxRichTextRange& GetRange() { return m_range; }

    /// Get/set dirty flag (whether the object needs Layout to be called)
    virtual bool GetDirty() const { return m_dirty; }
    virtual void SetDirty(bool dirty) { m_dirty = dirty; }

    /// Is this composite?
    virtual bool IsComposite() const { return false; }

    /// Get/set the parent.
    virtual wxRichTextObject* GetParent() const { return m_parent; }
    virtual void SetParent(wxRichTextObject* parent) { m_parent = parent; }

    /// Set the margin around the object
    virtual void SetMargins(int margin);
    virtual void SetMargins(int leftMargin, int rightMargin, int topMargin, int bottomMargin);
    virtual int GetLeftMargin() const { return m_leftMargin; }
    virtual int GetRightMargin() const { return m_rightMargin; }
    virtual int GetTopMargin() const { return m_topMargin; }
    virtual int GetBottomMargin() const { return m_bottomMargin; }

    /// Set attributes object
    void SetAttributes(const wxTextAttrEx& attr) { m_attributes = attr; }
    const wxTextAttrEx& GetAttributes() const { return m_attributes; }
    wxTextAttrEx& GetAttributes() { return m_attributes; }

    /// Set/get stored descent
    void SetDescent(int descent) { m_descent = descent; }
    int GetDescent() const { return m_descent; }

    /// Gets the containing buffer
    wxRichTextBuffer* GetBuffer() const;

// Operations

    /// Clone the object
    virtual wxRichTextObject* Clone() const { return NULL; }

    /// Copy
    void Copy(const wxRichTextObject& obj);

    /// Reference-counting allows us to use the same object in multiple
    /// lists (not yet used)
    void Reference() { m_refCount ++; }
    void Dereference();

    /// Convert units in tenths of a millimetre to device units
    int ConvertTenthsMMToPixels(wxDC& dc, int units);
    static int ConvertTenthsMMToPixels(int ppi, int units);

protected:
    wxSize                  m_size;
    wxPoint                 m_pos;
    int                     m_descent; // Descent for this object (if any)
    bool                    m_dirty;
    int                     m_refCount;
    wxRichTextObject*       m_parent;

    /// The range of this object (start position to end position)
    wxRichTextRange         m_range;

    /// Margins
    int                     m_leftMargin;
    int                     m_rightMargin;
    int                     m_topMargin;
    int                     m_bottomMargin;

    /// Attributes
    wxTextAttrEx            m_attributes;
};

WX_DECLARE_LIST_WITH_DECL( wxRichTextObject, wxRichTextObjectList, class WXDLLIMPEXP_RICHTEXT );

/*!
 * wxRichTextCompositeObject class declaration
 * Objects of this class can contain other objects.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextCompositeObject: public wxRichTextObject
{
    DECLARE_CLASS(wxRichTextCompositeObject)
public:
// Constructors

    wxRichTextCompositeObject(wxRichTextObject* parent = NULL);
    virtual ~wxRichTextCompositeObject();

// Overrideables

    /// Hit-testing: returns a flag indicating hit test details, plus
    /// information about position
    virtual int HitTest(wxDC& dc, const wxPoint& pt, long& textPosition);

    /// Finds the absolute position and row height for the given character position
    virtual bool FindPosition(wxDC& dc, long index, wxPoint& pt, int* height, bool forceLineStart);

    /// Calculate range
    virtual void CalculateRange(long start, long& end);

    /// Delete range
    virtual bool DeleteRange(const wxRichTextRange& range);

    /// Get any text in this object for the given range
    virtual wxString GetTextForRange(const wxRichTextRange& range) const;

    /// Dump to output stream for debugging
    virtual void Dump(wxTextOutputStream& stream);

// Accessors

    /// Get the children
    wxRichTextObjectList& GetChildren() { return m_children; }
    const wxRichTextObjectList& GetChildren() const { return m_children; }

    /// Get the child count
    size_t GetChildCount() const ;

    /// Get the nth child
    wxRichTextObject* GetChild(size_t n) const ;

    /// Get/set dirty flag
    virtual bool GetDirty() const { return m_dirty; }
    virtual void SetDirty(bool dirty) { m_dirty = dirty; }

    /// Is this composite?
    virtual bool IsComposite() const { return true; }

    /// Returns true if the buffer is empty
    virtual bool IsEmpty() const { return GetChildCount() == 0; }

// Operations

    /// Copy
    void Copy(const wxRichTextCompositeObject& obj);

    /// Assignment
    void operator= (const wxRichTextCompositeObject& obj) { Copy(obj); }

    /// Append a child, returning the position
    size_t AppendChild(wxRichTextObject* child) ;

    /// Insert the child in front of the given object, or at the beginning
    bool InsertChild(wxRichTextObject* child, wxRichTextObject* inFrontOf) ;

    /// Delete the child
    bool RemoveChild(wxRichTextObject* child, bool deleteChild = false) ;

    /// Delete all children
    bool DeleteChildren() ;

    /// Recursively merge all pieces that can be merged.
    bool Defragment();

protected:
    wxRichTextObjectList    m_children;
};

/*!
 * wxRichTextBox class declaration
 * This defines a 2D space to lay out objects
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextBox: public wxRichTextCompositeObject
{
    DECLARE_DYNAMIC_CLASS(wxRichTextBox)
public:
// Constructors

    wxRichTextBox(wxRichTextObject* parent = NULL);
    wxRichTextBox(const wxRichTextBox& obj): wxRichTextCompositeObject() { Copy(obj); }

// Overrideables

    /// Draw the item
    virtual bool Draw(wxDC& dc, const wxRichTextRange& range, const wxRichTextRange& selectionRange, const wxRect& rect, int descent, int style);

    /// Lay the item out
    virtual bool Layout(wxDC& dc, const wxRect& rect, int style);

    /// Get/set the object size for the given range. Returns false if the range
    /// is invalid for this object.
    virtual bool GetRangeSize(const wxRichTextRange& range, wxSize& size, int& descent, wxDC& dc, int flags, wxPoint position = wxPoint(0,0)) const;

// Accessors

// Operations

    /// Clone
    virtual wxRichTextObject* Clone() const { return new wxRichTextBox(*this); }

    /// Copy
    void Copy(const wxRichTextBox& obj);

protected:
};

/*!
 * wxRichTextParagraphBox class declaration
 * This box knows how to lay out paragraphs.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextParagraphLayoutBox: public wxRichTextBox
{
    DECLARE_DYNAMIC_CLASS(wxRichTextParagraphLayoutBox)
public:
// Constructors

    wxRichTextParagraphLayoutBox(wxRichTextObject* parent = NULL);
    wxRichTextParagraphLayoutBox(const wxRichTextParagraphLayoutBox& obj): wxRichTextBox() { Init(); Copy(obj); }

// Overrideables

    /// Draw the item
    virtual bool Draw(wxDC& dc, const wxRichTextRange& range, const wxRichTextRange& selectionRange, const wxRect& rect, int descent, int style);

    /// Lay the item out
    virtual bool Layout(wxDC& dc, const wxRect& rect, int style);

    /// Get/set the object size for the given range. Returns false if the range
    /// is invalid for this object.
    virtual bool GetRangeSize(const wxRichTextRange& range, wxSize& size, int& descent, wxDC& dc, int flags, wxPoint position = wxPoint(0,0)) const;

    /// Delete range
    virtual bool DeleteRange(const wxRichTextRange& range);

    /// Get any text in this object for the given range
    virtual wxString GetTextForRange(const wxRichTextRange& range) const;

// Accessors

    /// Associate a control with the buffer, for operations that for example require refreshing the window.
    void SetRichTextCtrl(wxRichTextCtrl* ctrl) { m_ctrl = ctrl; }

    /// Get the associated control.
    wxRichTextCtrl* GetRichTextCtrl() const { return m_ctrl; }

    /// Get/set whether the last paragraph is partial or complete
    void SetPartialParagraph(bool partialPara) { m_partialParagraph = partialPara; }
    bool GetPartialParagraph() const { return m_partialParagraph; }

    /// If this is a buffer, returns the current style sheet. The base layout box
    /// class doesn't have an associated style sheet.
    virtual wxRichTextStyleSheet* GetStyleSheet() const { return NULL; }

// Operations

    /// Initialize the object.
    void Init();

    /// Clear all children
    virtual void Clear();

    /// Clear and initialize with one blank paragraph
    virtual void Reset();

    /// Convenience function to add a paragraph of text
    virtual wxRichTextRange AddParagraph(const wxString& text, wxTextAttrEx* paraStyle = NULL);

    /// Convenience function to add an image
    virtual wxRichTextRange AddImage(const wxImage& image, wxTextAttrEx* paraStyle = NULL);

    /// Adds multiple paragraphs, based on newlines.
    virtual wxRichTextRange AddParagraphs(const wxString& text, wxTextAttrEx* paraStyle = NULL);

    /// Get the line at the given position. If caretPosition is true, the position is
    /// a caret position, which is normally a smaller number.
    virtual wxRichTextLine* GetLineAtPosition(long pos, bool caretPosition = false) const;

    /// Get the line at the given y pixel position, or the last line.
    virtual wxRichTextLine* GetLineAtYPosition(int y) const;

    /// Get the paragraph at the given character or caret position
    virtual wxRichTextParagraph* GetParagraphAtPosition(long pos, bool caretPosition = false) const;

    /// Get the line size at the given position
    virtual wxSize GetLineSizeAtPosition(long pos, bool caretPosition = false) const;

    /// Given a position, get the number of the visible line (potentially many to a paragraph),
    /// starting from zero at the start of the buffer. We also have to pass a bool (startOfLine)
    /// that indicates whether the caret is being shown at the end of the previous line or at the start
    /// of the next, since the caret can be shown at 2 visible positions for the same underlying
    /// position.
    virtual long GetVisibleLineNumber(long pos, bool caretPosition = false, bool startOfLine = false) const;

    /// Given a line number, get the corresponding wxRichTextLine object.
    virtual wxRichTextLine* GetLineForVisibleLineNumber(long lineNumber) const;

    /// Get the leaf object in a paragraph at this position.
    /// Given a line number, get the corresponding wxRichTextLine object.
    virtual wxRichTextObject* GetLeafObjectAtPosition(long position) const;

    /// Get the paragraph by number
    virtual wxRichTextParagraph* GetParagraphAtLine(long paragraphNumber) const;

    /// Get the paragraph for a given line
    virtual wxRichTextParagraph* GetParagraphForLine(wxRichTextLine* line) const;

    /// Get the length of the paragraph
    virtual int GetParagraphLength(long paragraphNumber) const;

    /// Get the number of paragraphs
    virtual int GetParagraphCount() const { return wx_static_cast(int, GetChildCount()); }

    /// Get the number of visible lines
    virtual int GetLineCount() const;

    /// Get the text of the paragraph
    virtual wxString GetParagraphText(long paragraphNumber) const;

    /// Convert zero-based line column and paragraph number to a position.
    virtual long XYToPosition(long x, long y) const;

    /// Convert zero-based position to line column and paragraph number
    virtual bool PositionToXY(long pos, long* x, long* y) const;

    /// Set text attributes: character and/or paragraph styles.
    virtual bool SetStyle(const wxRichTextRange& range, const wxRichTextAttr& style, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO);
    virtual bool SetStyle(const wxRichTextRange& range, const wxTextAttrEx& style, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO);

    /// Get the conbined text attributes for this position.
    virtual bool GetStyle(long position, wxTextAttrEx& style);
    virtual bool GetStyle(long position, wxRichTextAttr& style);

    /// Get the content (uncombined) attributes for this position.
    virtual bool GetUncombinedStyle(long position, wxTextAttrEx& style);
    virtual bool GetUncombinedStyle(long position, wxRichTextAttr& style);

    /// Implementation helper for GetStyle. If combineStyles is true, combine base, paragraph and
    /// context attributes.
    virtual bool DoGetStyle(long position, wxTextAttrEx& style, bool combineStyles = true);

    /// Get the combined style for a range - if any attribute is different within the range,
    /// that attribute is not present within the flags
    virtual bool GetStyleForRange(const wxRichTextRange& range, wxTextAttrEx& style);

    /// Combines 'style' with 'currentStyle' for the purpose of summarising the attributes of a range of
    /// content.
    bool CollectStyle(wxTextAttrEx& currentStyle, const wxTextAttrEx& style, long& multipleStyleAttributes, int& multipleTextEffectAttributes);

    /// Set list style
    virtual bool SetListStyle(const wxRichTextRange& range, wxRichTextListStyleDefinition* def, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);
    virtual bool SetListStyle(const wxRichTextRange& range, const wxString& defName, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);

    /// Clear list for given range
    virtual bool ClearListStyle(const wxRichTextRange& range, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO);

    /// Number/renumber any list elements in the given range.
    /// def/defName can be NULL/empty to indicate that the existing list style should be used.
    virtual bool NumberList(const wxRichTextRange& range, wxRichTextListStyleDefinition* def = NULL, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);
    virtual bool NumberList(const wxRichTextRange& range, const wxString& defName, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);

    /// Promote the list items within the given range. promoteBy can be a positive or negative number, e.g. 1 or -1
    /// def/defName can be NULL/empty to indicate that the existing list style should be used.
    virtual bool PromoteList(int promoteBy, const wxRichTextRange& range, wxRichTextListStyleDefinition* def = NULL, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int specifiedLevel = -1);
    virtual bool PromoteList(int promoteBy, const wxRichTextRange& range, const wxString& defName, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int specifiedLevel = -1);

    /// Helper for NumberList and PromoteList, that does renumbering and promotion simultaneously
    /// def/defName can be NULL/empty to indicate that the existing list style should be used.
    virtual bool DoNumberList(const wxRichTextRange& range, const wxRichTextRange& promotionRange, int promoteBy, wxRichTextListStyleDefinition* def, int flags = wxRICHTEXT_SETSTYLE_WITH_UNDO, int startFrom = 1, int specifiedLevel = -1);

    /// Fills in the attributes for numbering a paragraph after previousParagraph.
    virtual bool FindNextParagraphNumber(wxRichTextParagraph* previousParagraph, wxRichTextAttr& attr) const;

    /// Test if this whole range has character attributes of the specified kind. If any
    /// of the attributes are different within the range, the test fails. You
    /// can use this to implement, for example, bold button updating. style must have
    /// flags indicating which attributes are of interest.
    virtual bool HasCharacterAttributes(const wxRichTextRange& range, const wxTextAttrEx& style) const;
    virtual bool HasCharacterAttributes(const wxRichTextRange& range, const wxRichTextAttr& style) const;

    /// Test if this whole range has paragraph attributes of the specified kind. If any
    /// of the attributes are different within the range, the test fails. You
    /// can use this to implement, for example, centering button updating. style must have
    /// flags indicating which attributes are of interest.
    virtual bool HasParagraphAttributes(const wxRichTextRange& range, const wxTextAttrEx& style) const;
    virtual bool HasParagraphAttributes(const wxRichTextRange& range, const wxRichTextAttr& style) const;

    /// Clone
    virtual wxRichTextObject* Clone() const { return new wxRichTextParagraphLayoutBox(*this); }

    /// Insert fragment into this box at the given position. If partialParagraph is true,
    /// it is assumed that the last (or only) paragraph is just a piece of data with no paragraph
    /// marker.
    virtual bool InsertFragment(long position, wxRichTextParagraphLayoutBox& fragment);

    /// Make a copy of the fragment corresponding to the given range, putting it in 'fragment'.
    virtual bool CopyFragment(const wxRichTextRange& range, wxRichTextParagraphLayoutBox& fragment);

    /// Apply the style sheet to the buffer, for example if the styles have changed.
    virtual bool ApplyStyleSheet(wxRichTextStyleSheet* styleSheet);

    /// Copy
    void Copy(const wxRichTextParagraphLayoutBox& obj);

    /// Assignment
    void operator= (const wxRichTextParagraphLayoutBox& obj) { Copy(obj); }

    /// Calculate ranges
    virtual void UpdateRanges() { long end; CalculateRange(0, end); }

    /// Get all the text
    virtual wxString GetText() const;

    /// Set default style for new content. Setting it to a default attribute
    /// makes new content take on the 'basic' style.
    virtual bool SetDefaultStyle(const wxTextAttrEx& style);

    /// Get default style
    virtual const wxTextAttrEx& GetDefaultStyle() const { return m_defaultAttributes; }

    /// Set basic (overall) style
    virtual void SetBasicStyle(const wxTextAttrEx& style) { m_attributes = style; }
    virtual void SetBasicStyle(const wxRichTextAttr& style) { m_attributes = style; }

    /// Get basic (overall) style
    virtual const wxTextAttrEx& GetBasicStyle() const { return m_attributes; }

    /// Invalidate the buffer. With no argument, invalidates whole buffer.
    void Invalidate(const wxRichTextRange& invalidRange = wxRICHTEXT_ALL);

    /// Get invalid range, rounding to entire paragraphs if argument is true.
    wxRichTextRange GetInvalidRange(bool wholeParagraphs = false) const;

protected:
    wxRichTextCtrl* m_ctrl;
    wxTextAttrEx    m_defaultAttributes;

    /// The invalidated range that will need full layout
    wxRichTextRange m_invalidRange;

    // Is the last paragraph partial or complete?
    bool            m_partialParagraph;
};

/*!
 * wxRichTextLine class declaration
 * This object represents a line in a paragraph, and stores
 * offsets from the start of the paragraph representing the
 * start and end positions of the line.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextLine
{
public:
// Constructors

    wxRichTextLine(wxRichTextParagraph* parent);
    wxRichTextLine(const wxRichTextLine& obj) { Init( NULL); Copy(obj); }
    virtual ~wxRichTextLine() {}

// Overrideables

// Accessors

    /// Set the range
    void SetRange(const wxRichTextRange& range) { m_range = range; }
    void SetRange(long from, long to) { m_range = wxRichTextRange(from, to); }

    /// Get the parent paragraph
    wxRichTextParagraph* GetParent() { return m_parent; }

    /// Get the range
    const wxRichTextRange& GetRange() const { return m_range; }
    wxRichTextRange& GetRange() { return m_range; }

    /// Get the absolute range
    wxRichTextRange GetAbsoluteRange() const;

    /// Get/set the line size as calculated by Layout.
    virtual wxSize GetSize() const { return m_size; }
    virtual void SetSize(const wxSize& sz) { m_size = sz; }

    /// Get/set the object position relative to the parent
    virtual wxPoint GetPosition() const { return m_pos; }
    virtual void SetPosition(const wxPoint& pos) { m_pos = pos; }

    /// Get the absolute object position
    virtual wxPoint GetAbsolutePosition() const;

    /// Get the rectangle enclosing the line
    virtual wxRect GetRect() const { return wxRect(GetAbsolutePosition(), GetSize()); }

    /// Set/get stored descent
    void SetDescent(int descent) { m_descent = descent; }
    int GetDescent() const { return m_descent; }

#if wxRICHTEXT_USE_OPTIMIZED_LINE_DRAWING
    wxArrayInt& GetObjectSizes() { return m_objectSizes; }
    const wxArrayInt& GetObjectSizes() const { return m_objectSizes; }
#endif

// Operations

    /// Initialisation
    void Init(wxRichTextParagraph* parent);

    /// Copy
    void Copy(const wxRichTextLine& obj);

    /// Clone
    virtual wxRichTextLine* Clone() const { return new wxRichTextLine(*this); }

protected:

    /// The range of the line (start position to end position)
    /// This is relative to the parent paragraph.
    wxRichTextRange     m_range;

    /// Size and position measured relative to top of paragraph
    wxPoint             m_pos;
    wxSize              m_size;

    /// Maximum descent for this line (location of text baseline)
    int                 m_descent;

    // The parent object
    wxRichTextParagraph* m_parent;

    // Sizes of the objects within a line so we don't have to get text extents
#if wxRICHTEXT_USE_OPTIMIZED_LINE_DRAWING
    wxArrayInt          m_objectSizes;
#endif
};

WX_DECLARE_LIST_WITH_DECL( wxRichTextLine, wxRichTextLineList , class WXDLLIMPEXP_RICHTEXT );

/*!
 * wxRichTextParagraph class declaration
 * This object represents a single paragraph (or in a straight text editor, a line).
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextParagraph: public wxRichTextBox
{
    DECLARE_DYNAMIC_CLASS(wxRichTextParagraph)
public:
// Constructors

    wxRichTextParagraph(wxRichTextObject* parent = NULL, wxTextAttrEx* style = NULL);
    wxRichTextParagraph(const wxString& text, wxRichTextObject* parent = NULL, wxTextAttrEx* paraStyle = NULL, wxTextAttrEx* charStyle = NULL);
    virtual ~wxRichTextParagraph();
    wxRichTextParagraph(const wxRichTextParagraph& obj): wxRichTextBox() { Copy(obj); }

// Overrideables

    /// Draw the item
    virtual bool Draw(wxDC& dc, const wxRichTextRange& range, const wxRichTextRange& selectionRange, const wxRect& rect, int descent, int style);

    /// Lay the item out
    virtual bool Layout(wxDC& dc, const wxRect& rect, int style);

    /// Get/set the object size for the given range. Returns false if the range
    /// is invalid for this object.
    virtual bool GetRangeSize(const wxRichTextRange& range, wxSize& size, int& descent, wxDC& dc, int flags, wxPoint position = wxPoint(0,0)) const;

    /// Finds the absolute position and row height for the given character position
    virtual bool FindPosition(wxDC& dc, long index, wxPoint& pt, int* height, bool forceLineStart);

    /// Hit-testing: returns a flag indicating hit test details, plus
    /// information about position
    virtual int HitTest(wxDC& dc, const wxPoint& pt, long& textPosition);

    /// Calculate range
    virtual void CalculateRange(long start, long& end);

// Accessors

    /// Get the cached lines
    wxRichTextLineList& GetLines() { return m_cachedLines; }

// Operations

    /// Copy
    void Copy(const wxRichTextParagraph& obj);

    /// Clone
    virtual wxRichTextObject* Clone() const { return new wxRichTextParagraph(*this); }

    /// Clear the cached lines
    void ClearLines();

// Implementation

    /// Apply paragraph styles such as centering to the wrapped lines
    virtual void ApplyParagraphStyle(const wxTextAttrEx& attr, const wxRect& rect);

    /// Insert text at the given position
    virtual bool InsertText(long pos, const wxString& text);

    /// Split an object at this position if necessary, and return
    /// the previous object, or NULL if inserting at beginning.
    virtual wxRichTextObject* SplitAt(long pos, wxRichTextObject** previousObject = NULL);

    /// Move content to a list from this point
    virtual void MoveToList(wxRichTextObject* obj, wxList& list);

    /// Add content back from list
    virtual void MoveFromList(wxList& list);

    /// Get the plain text searching from the start or end of the range.
    /// The resulting string may be shorter than the range given.
    bool GetContiguousPlainText(wxString& text, const wxRichTextRange& range, bool fromStart = true);

    /// Find a suitable wrap position. wrapPosition is the last position in the line to the left
    /// of the split.
    bool FindWrapPosition(const wxRichTextRange& range, wxDC& dc, int availableSpace, long& wrapPosition);

    /// Find the object at the given position
    wxRichTextObject* FindObjectAtPosition(long position);

    /// Get the bullet text for this paragraph.
    wxString GetBulletText();

    /// Allocate or reuse a line object
    wxRichTextLine* AllocateLine(int pos);

    /// Clear remaining unused line objects, if any
    bool ClearUnusedLines(int lineCount);

    /// Get combined attributes of the base style, paragraph style and character style. We use this to dynamically
    /// retrieve the actual style.
    wxTextAttrEx GetCombinedAttributes(const wxTextAttrEx& contentStyle) const;

    /// Get combined attributes of the base style and paragraph style.
    wxTextAttrEx GetCombinedAttributes() const;

    /// Get the first position from pos that has a line break character.
    long GetFirstLineBreakPosition(long pos);

    /// Create default tabstop array
    static void InitDefaultTabs();

    /// Clear default tabstop array
    static void ClearDefaultTabs();

    /// Get default tabstop array
    static const wxArrayInt& GetDefaultTabs() { return sm_defaultTabs; }

protected:
    /// The lines that make up the wrapped paragraph
    wxRichTextLineList m_cachedLines;

    /// Default tabstops
    static wxArrayInt  sm_defaultTabs;
};

/*!
 * wxRichTextPlainText class declaration
 * This object represents a single piece of text.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextPlainText: public wxRichTextObject
{
    DECLARE_DYNAMIC_CLASS(wxRichTextPlainText)
public:
// Constructors

    wxRichTextPlainText(const wxString& text = wxEmptyString, wxRichTextObject* parent = NULL, wxTextAttrEx* style = NULL);
    wxRichTextPlainText(const wxRichTextPlainText& obj): wxRichTextObject() { Copy(obj); }

// Overrideables

    /// Draw the item
    virtual bool Draw(wxDC& dc, const wxRichTextRange& range, const wxRichTextRange& selectionRange, const wxRect& rect, int descent, int style);

    /// Lay the item out
    virtual bool Layout(wxDC& dc, const wxRect& rect, int style);

    /// Get/set the object size for the given range. Returns false if the range
    /// is invalid for this object.
    virtual bool GetRangeSize(const wxRichTextRange& range, wxSize& size, int& descent, wxDC& dc, int flags, wxPoint position/* = wxPoint(0,0)*/) const;

    /// Get any text in this object for the given range
    virtual wxString GetTextForRange(const wxRichTextRange& range) const;

    /// Do a split, returning an object containing the second part, and setting
    /// the first part in 'this'.
    virtual wxRichTextObject* DoSplit(long pos);

    /// Calculate range
    virtual void CalculateRange(long start, long& end);

    /// Delete range
    virtual bool DeleteRange(const wxRichTextRange& range);

    /// Returns true if the object is empty
    virtual bool IsEmpty() const { return m_text.empty(); }

    /// Returns true if this object can merge itself with the given one.
    virtual bool CanMerge(wxRichTextObject* object) const;

    /// Returns true if this object merged itself with the given one.
    /// The calling code will then delete the given object.
    virtual bool Merge(wxRichTextObject* object);

    /// Dump to output stream for debugging
    virtual void Dump(wxTextOutputStream& stream);

    /// Get the first position from pos that has a line break character.
    long GetFirstLineBreakPosition(long pos);

// Accessors

    /// Get the text
    const wxString& GetText() const { return m_text; }

    /// Set the text
    void SetText(const wxString& text) { m_text = text; }

// Operations

    /// Copy
    void Copy(const wxRichTextPlainText& obj);

    /// Clone
    virtual wxRichTextObject* Clone() const { return new wxRichTextPlainText(*this); }
private:
    bool DrawTabbedString(wxDC& dc, const wxTextAttrEx& attr, const wxRect& rect, wxString& str, wxCoord& x, wxCoord& y, bool selected);

protected:
    wxString    m_text;
};

/*!
 * wxRichTextImageBlock stores information about an image, in binary in-memory form
 */

class WXDLLIMPEXP_FWD_BASE wxDataInputStream;
class WXDLLIMPEXP_FWD_BASE wxDataOutputStream;

class WXDLLIMPEXP_RICHTEXT wxRichTextImageBlock: public wxObject
{
public:
    wxRichTextImageBlock();
    wxRichTextImageBlock(const wxRichTextImageBlock& block);
    virtual ~wxRichTextImageBlock();

    void Init();
    void Clear();

    // Load the original image into a memory block.
    // If the image is not a JPEG, we must convert it into a JPEG
    // to conserve space.
    // If it's not a JPEG we can make use of 'image', already scaled, so we don't have to
    // load the image a 2nd time.
    virtual bool MakeImageBlock(const wxString& filename, int imageType, wxImage& image, bool convertToJPEG = true);

    // Make an image block from the wxImage in the given
    // format.
    virtual bool MakeImageBlock(wxImage& image, int imageType, int quality = 80);

    // Write to a file
    bool Write(const wxString& filename);

    // Write data in hex to a stream
    bool WriteHex(wxOutputStream& stream);

    // Read data in hex from a stream
    bool ReadHex(wxInputStream& stream, int length, int imageType);

    // Copy from 'block'
    void Copy(const wxRichTextImageBlock& block);

    // Load a wxImage from the block
    bool Load(wxImage& image);

//// Operators
    void operator=(const wxRichTextImageBlock& block);

//// Accessors

    unsigned char* GetData() const { return m_data; }
    size_t GetDataSize() const { return m_dataSize; }
    int GetImageType() const { return m_imageType; }

    void SetData(unsigned char* image) { m_data = image; }
    void SetDataSize(size_t size) { m_dataSize = size; }
    void SetImageType(int imageType) { m_imageType = imageType; }

    bool Ok() const { return IsOk(); }
    bool IsOk() const { return GetData() != NULL; }

    // Gets the extension for the block's type
    wxString GetExtension() const;

/// Implementation

    // Allocate and read from stream as a block of memory
    static unsigned char* ReadBlock(wxInputStream& stream, size_t size);
    static unsigned char* ReadBlock(const wxString& filename, size_t size);

    // Write memory block to stream
    static bool WriteBlock(wxOutputStream& stream, unsigned char* block, size_t size);

    // Write memory block to file
    static bool WriteBlock(const wxString& filename, unsigned char* block, size_t size);

protected:
    // Size in bytes of the image stored.
    // This is in the raw, original form such as a JPEG file.
    unsigned char*      m_data;
    size_t              m_dataSize;
    int                 m_imageType; // wxWin type id
};


/*!
 * wxRichTextImage class declaration
 * This object represents an image.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextImage: public wxRichTextObject
{
    DECLARE_DYNAMIC_CLASS(wxRichTextImage)
public:
// Constructors

    wxRichTextImage(wxRichTextObject* parent = NULL): wxRichTextObject(parent) { }
    wxRichTextImage(const wxImage& image, wxRichTextObject* parent = NULL, wxTextAttrEx* charStyle = NULL);
    wxRichTextImage(const wxRichTextImageBlock& imageBlock, wxRichTextObject* parent = NULL, wxTextAttrEx* charStyle = NULL);
    wxRichTextImage(const wxRichTextImage& obj): wxRichTextObject() { Copy(obj); }

// Overrideables

    /// Draw the item
    virtual bool Draw(wxDC& dc, const wxRichTextRange& range, const wxRichTextRange& selectionRange, const wxRect& rect, int descent, int style);

    /// Lay the item out
    virtual bool Layout(wxDC& dc, const wxRect& rect, int style);

    /// Get the object size for the given range. Returns false if the range
    /// is invalid for this object.
    virtual bool GetRangeSize(const wxRichTextRange& range, wxSize& size, int& descent, wxDC& dc, int flags, wxPoint position = wxPoint(0,0)) const;

    /// Returns true if the object is empty
    virtual bool IsEmpty() const { return !m_image.Ok(); }

// Accessors

    /// Get the image
    const wxImage& GetImage() const { return m_image; }

    /// Set the image
    void SetImage(const wxImage& image) { m_image = image; }

    /// Get the image block containing the raw data
    wxRichTextImageBlock& GetImageBlock() { return m_imageBlock; }

// Operations

    /// Copy
    void Copy(const wxRichTextImage& obj);

    /// Clone
    virtual wxRichTextObject* Clone() const { return new wxRichTextImage(*this); }

    /// Load wxImage from the block
    virtual bool LoadFromBlock();

    /// Make block from the wxImage
    virtual bool MakeBlock();

protected:
    // TODO: reduce the multiple representations of data
    wxImage                 m_image;
    wxBitmap                m_bitmap;
    wxRichTextImageBlock    m_imageBlock;
};


/*!
 * wxRichTextBuffer class declaration
 * This is a kind of box, used to represent the whole buffer
 */

class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextCommand;
class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextAction;

class WXDLLIMPEXP_RICHTEXT wxRichTextBuffer: public wxRichTextParagraphLayoutBox
{
    DECLARE_DYNAMIC_CLASS(wxRichTextBuffer)
public:
// Constructors

    wxRichTextBuffer() { Init(); }
    wxRichTextBuffer(const wxRichTextBuffer& obj): wxRichTextParagraphLayoutBox() { Init(); Copy(obj); }
    virtual ~wxRichTextBuffer() ;

// Accessors

    /// Gets the command processor
    wxCommandProcessor* GetCommandProcessor() const { return m_commandProcessor; }

    /// Set style sheet, if any.
    void SetStyleSheet(wxRichTextStyleSheet* styleSheet) { m_styleSheet = styleSheet; }
    virtual wxRichTextStyleSheet* GetStyleSheet() const { return m_styleSheet; }

    /// Set style sheet and notify of the change
    bool SetStyleSheetAndNotify(wxRichTextStyleSheet* sheet);

    /// Push style sheet to top of stack
    bool PushStyleSheet(wxRichTextStyleSheet* styleSheet);

    /// Pop style sheet from top of stack
    wxRichTextStyleSheet* PopStyleSheet();

// Operations

    /// Initialisation
    void Init();

    /// Clears the buffer, adds an empty paragraph, and clears the command processor.
    virtual void ResetAndClearCommands();

    /// Load a file
    virtual bool LoadFile(const wxString& filename, int type = wxRICHTEXT_TYPE_ANY);

    /// Save a file
    virtual bool SaveFile(const wxString& filename, int type = wxRICHTEXT_TYPE_ANY);

    /// Load from a stream
    virtual bool LoadFile(wxInputStream& stream, int type = wxRICHTEXT_TYPE_ANY);

    /// Save to a stream
    virtual bool SaveFile(wxOutputStream& stream, int type = wxRICHTEXT_TYPE_ANY);

    /// Set the handler flags, controlling loading and saving
    void SetHandlerFlags(int flags) { m_handlerFlags = flags; }

    /// Get the handler flags, controlling loading and saving
    int GetHandlerFlags() const { return m_handlerFlags; }

    /// Convenience function to add a paragraph of text
    virtual wxRichTextRange AddParagraph(const wxString& text, wxTextAttrEx* paraStyle = NULL) { Modify(); return wxRichTextParagraphLayoutBox::AddParagraph(text, paraStyle); }

    /// Begin collapsing undo/redo commands. Note that this may not work properly
    /// if combining commands that delete or insert content, changing ranges for
    /// subsequent actions.
    virtual bool BeginBatchUndo(const wxString& cmdName);

    /// End collapsing undo/redo commands
    virtual bool EndBatchUndo();

    /// Collapsing commands?
    virtual bool BatchingUndo() const { return m_batchedCommandDepth > 0; }

    /// Submit immediately, or delay according to whether collapsing is on
    virtual bool SubmitAction(wxRichTextAction* action);

    /// Get collapsed command
    virtual wxRichTextCommand* GetBatchedCommand() const { return m_batchedCommand; }

    /// Begin suppressing undo/redo commands. The way undo is suppressed may be implemented
    /// differently by each command. If not dealt with by a command implementation, then
    /// it will be implemented automatically by not storing the command in the undo history
    /// when the action is submitted to the command processor.
    virtual bool BeginSuppressUndo();

    /// End suppressing undo/redo commands.
    virtual bool EndSuppressUndo();

    /// Collapsing commands?
    virtual bool SuppressingUndo() const { return m_suppressUndo > 0; }

    /// Copy the range to the clipboard
    virtual bool CopyToClipboard(const wxRichTextRange& range);

    /// Paste the clipboard content to the buffer
    virtual bool PasteFromClipboard(long position);

    /// Can we paste from the clipboard?
    virtual bool CanPasteFromClipboard() const;

    /// Begin using a style
    virtual bool BeginStyle(const wxTextAttrEx& style);

    /// End the style
    virtual bool EndStyle();

    /// End all styles
    virtual bool EndAllStyles();

    /// Clear the style stack
    virtual void ClearStyleStack();

    /// Get the size of the style stack, for example to check correct nesting
    virtual size_t GetStyleStackSize() const { return m_attributeStack.GetCount(); }

    /// Begin using bold
    bool BeginBold();

    /// End using bold
    bool EndBold() { return EndStyle(); }

    /// Begin using italic
    bool BeginItalic();

    /// End using italic
    bool EndItalic() { return EndStyle(); }

    /// Begin using underline
    bool BeginUnderline();

    /// End using underline
    bool EndUnderline() { return EndStyle(); }

    /// Begin using point size
    bool BeginFontSize(int pointSize);

    /// End using point size
    bool EndFontSize() { return EndStyle(); }

    /// Begin using this font
    bool BeginFont(const wxFont& font);

    /// End using a font
    bool EndFont() { return EndStyle(); }

    /// Begin using this colour
    bool BeginTextColour(const wxColour& colour);

    /// End using a colour
    bool EndTextColour() { return EndStyle(); }

    /// Begin using alignment
    bool BeginAlignment(wxTextAttrAlignment alignment);

    /// End alignment
    bool EndAlignment() { return EndStyle(); }

    /// Begin left indent
    bool BeginLeftIndent(int leftIndent, int leftSubIndent = 0);

    /// End left indent
    bool EndLeftIndent() { return EndStyle(); }

    /// Begin right indent
    bool BeginRightIndent(int rightIndent);

    /// End right indent
    bool EndRightIndent() { return EndStyle(); }

    /// Begin paragraph spacing
    bool BeginParagraphSpacing(int before, int after);

    /// End paragraph spacing
    bool EndParagraphSpacing() { return EndStyle(); }

    /// Begin line spacing
    bool BeginLineSpacing(int lineSpacing);

    /// End line spacing
    bool EndLineSpacing() { return EndStyle(); }

    /// Begin numbered bullet
    bool BeginNumberedBullet(int bulletNumber, int leftIndent, int leftSubIndent, int bulletStyle = wxTEXT_ATTR_BULLET_STYLE_ARABIC|wxTEXT_ATTR_BULLET_STYLE_PERIOD);

    /// End numbered bullet
    bool EndNumberedBullet() { return EndStyle(); }

    /// Begin symbol bullet
    bool BeginSymbolBullet(const wxString& symbol, int leftIndent, int leftSubIndent, int bulletStyle = wxTEXT_ATTR_BULLET_STYLE_SYMBOL);

    /// End symbol bullet
    bool EndSymbolBullet() { return EndStyle(); }

    /// Begin standard bullet
    bool BeginStandardBullet(const wxString& bulletName, int leftIndent, int leftSubIndent, int bulletStyle = wxTEXT_ATTR_BULLET_STYLE_STANDARD);

    /// End standard bullet
    bool EndStandardBullet() { return EndStyle(); }

    /// Begin named character style
    bool BeginCharacterStyle(const wxString& characterStyle);

    /// End named character style
    bool EndCharacterStyle() { return EndStyle(); }

    /// Begin named paragraph style
    bool BeginParagraphStyle(const wxString& paragraphStyle);

    /// End named character style
    bool EndParagraphStyle() { return EndStyle(); }

    /// Begin named list style
    bool BeginListStyle(const wxString& listStyle, int level = 1, int number = 1);

    /// End named character style
    bool EndListStyle() { return EndStyle(); }

    /// Begin URL
    bool BeginURL(const wxString& url, const wxString& characterStyle = wxEmptyString);

    /// End URL
    bool EndURL() { return EndStyle(); }

// Event handling

    /// Add an event handler
    bool AddEventHandler(wxEvtHandler* handler);

    /// Remove an event handler
    bool RemoveEventHandler(wxEvtHandler* handler, bool deleteHandler = false);

    /// Clear event handlers
    void ClearEventHandlers();

    /// Send event to event handlers. If sendToAll is true, will send to all event handlers,
    /// otherwise will stop at the first successful one.
    bool SendEvent(wxEvent& event, bool sendToAll = true);

// Implementation

    /// Copy
    void Copy(const wxRichTextBuffer& obj);

    /// Clone
    virtual wxRichTextObject* Clone() const { return new wxRichTextBuffer(*this); }

    /// Submit command to insert paragraphs
    bool InsertParagraphsWithUndo(long pos, const wxRichTextParagraphLayoutBox& paragraphs, wxRichTextCtrl* ctrl, int flags = 0);

    /// Submit command to insert the given text
    bool InsertTextWithUndo(long pos, const wxString& text, wxRichTextCtrl* ctrl, int flags = 0);

    /// Submit command to insert a newline
    bool InsertNewlineWithUndo(long pos, wxRichTextCtrl* ctrl, int flags = 0);

    /// Submit command to insert the given image
    bool InsertImageWithUndo(long pos, const wxRichTextImageBlock& imageBlock, wxRichTextCtrl* ctrl, int flags = 0);

    /// Submit command to delete this range
    bool DeleteRangeWithUndo(const wxRichTextRange& range, wxRichTextCtrl* ctrl);

    /// Mark modified
    void Modify(bool modify = true) { m_modified = modify; }
    bool IsModified() const { return m_modified; }

    /// Get the style that is appropriate for a new paragraph at this position.
    /// If the previous paragraph has a paragraph style name, look up the next-paragraph
    /// style.
    wxRichTextAttr GetStyleForNewParagraph(long pos, bool caretPosition = false, bool lookUpNewParaStyle=false) const;

    /// Dumps contents of buffer for debugging purposes
    virtual void Dump();
    virtual void Dump(wxTextOutputStream& stream) { wxRichTextParagraphLayoutBox::Dump(stream); }

    /// Returns the file handlers
    static wxList& GetHandlers() { return sm_handlers; }

    /// Adds a handler to the end
    static void AddHandler(wxRichTextFileHandler *handler);

    /// Inserts a handler at the front
    static void InsertHandler(wxRichTextFileHandler *handler);

    /// Removes a handler
    static bool RemoveHandler(const wxString& name);

    /// Finds a handler by name
    static wxRichTextFileHandler *FindHandler(const wxString& name);

    /// Finds a handler by extension and type
    static wxRichTextFileHandler *FindHandler(const wxString& extension, int imageType);

    /// Finds a handler by filename or, if supplied, type
    static wxRichTextFileHandler *FindHandlerFilenameOrType(const wxString& filename, int imageType);

    /// Finds a handler by type
    static wxRichTextFileHandler *FindHandler(int imageType);

    /// Gets a wildcard incorporating all visible handlers. If 'types' is present,
    /// will be filled with the file type corresponding to each filter. This can be
    /// used to determine the type to pass to LoadFile given a selected filter.
    static wxString GetExtWildcard(bool combine = false, bool save = false, wxArrayInt* types = NULL);

    /// Clean up handlers
    static void CleanUpHandlers();

    /// Initialise the standard handlers
    static void InitStandardHandlers();

    /// Get renderer
    static wxRichTextRenderer* GetRenderer() { return sm_renderer; }

    /// Set renderer, deleting old one
    static void SetRenderer(wxRichTextRenderer* renderer);

    /// Minimum margin between bullet and paragraph in 10ths of a mm
    static int GetBulletRightMargin() { return sm_bulletRightMargin; }
    static void SetBulletRightMargin(int margin) { sm_bulletRightMargin = margin; }

    /// Factor to multiply by character height to get a reasonable bullet size
    static float GetBulletProportion() { return sm_bulletProportion; }
    static void SetBulletProportion(float prop) { sm_bulletProportion = prop; }

    /// Scale factor for calculating dimensions
    double GetScale() const { return m_scale; }
    void SetScale(double scale) { m_scale = scale; }

protected:

    /// Command processor
    wxCommandProcessor*     m_commandProcessor;

    /// Has been modified?
    bool                    m_modified;

    /// Collapsed command stack
    int                     m_batchedCommandDepth;

    /// Name for collapsed command
    wxString                m_batchedCommandsName;

    /// Current collapsed command accumulating actions
    wxRichTextCommand*      m_batchedCommand;

    /// Whether to suppress undo
    int                     m_suppressUndo;

    /// Style sheet, if any
    wxRichTextStyleSheet*   m_styleSheet;

    /// List of event handlers that will be notified of events
    wxList                  m_eventHandlers;

    /// Stack of attributes for convenience functions
    wxList                  m_attributeStack;

    /// Flags to be passed to handlers
    int                     m_handlerFlags;

    /// File handlers
    static wxList           sm_handlers;

    /// Renderer
    static wxRichTextRenderer* sm_renderer;

    /// Minimum margin between bullet and paragraph in 10ths of a mm
    static int              sm_bulletRightMargin;

    /// Factor to multiply by character height to get a reasonable bullet size
    static float            sm_bulletProportion;

    /// Scaling factor in use: needed to calculate correct dimensions when printing
    double                  m_scale;
};

/*!
 * The command identifiers
 *
 */

enum wxRichTextCommandId
{
    wxRICHTEXT_INSERT,
    wxRICHTEXT_DELETE,
    wxRICHTEXT_CHANGE_STYLE
};

/*!
 * Command classes for undo/redo
 *
 */

class WXDLLIMPEXP_FWD_RICHTEXT wxRichTextAction;
class WXDLLIMPEXP_RICHTEXT wxRichTextCommand: public wxCommand
{
public:
    // Ctor for one action
    wxRichTextCommand(const wxString& name, wxRichTextCommandId id, wxRichTextBuffer* buffer,
        wxRichTextCtrl* ctrl, bool ignoreFirstTime = false);

    // Ctor for multiple actions
    wxRichTextCommand(const wxString& name);

    virtual ~wxRichTextCommand();

    bool Do();
    bool Undo();

    void AddAction(wxRichTextAction* action);
    void ClearActions();

    wxList& GetActions() { return m_actions; }

protected:

    wxList  m_actions;
};

/*!
 * wxRichTextAction class declaration
 * There can be more than one action in a command.
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextAction: public wxObject
{
public:
    wxRichTextAction(wxRichTextCommand* cmd, const wxString& name, wxRichTextCommandId id, wxRichTextBuffer* buffer,
        wxRichTextCtrl* ctrl, bool ignoreFirstTime = false);

    virtual ~wxRichTextAction();

    bool Do();
    bool Undo();

    /// Update the control appearance
    void UpdateAppearance(long caretPosition, bool sendUpdateEvent = false,
                            wxArrayInt* optimizationLineCharPositions = NULL, wxArrayInt* optimizationLineYPositions = NULL);

    /// Replace the buffer paragraphs with the given fragment.
    void ApplyParagraphs(const wxRichTextParagraphLayoutBox& fragment);

#if wxABI_VERSION >= 20808
    // Create arrays to be used in refresh optimization
    void CalculateRefreshOptimizations(wxArrayInt& optimizationLineCharPositions, wxArrayInt& optimizationLineYPositions);
#endif

    /// Get the fragments
    wxRichTextParagraphLayoutBox& GetNewParagraphs() { return m_newParagraphs; }
    wxRichTextParagraphLayoutBox& GetOldParagraphs() { return m_oldParagraphs; }

    /// Set/get the position used for e.g. insertion
    void SetPosition(long pos) { m_position = pos; }
    long GetPosition() const { return m_position; }

    /// Set/get the range for e.g. deletion
    void SetRange(const wxRichTextRange& range) { m_range = range; }
    const wxRichTextRange& GetRange() const { return m_range; }

    /// Get name
    const wxString& GetName() const { return m_name; }

protected:
    // Action name
    wxString                        m_name;

    // Buffer
    wxRichTextBuffer*               m_buffer;

    // Control
    wxRichTextCtrl*                 m_ctrl;

    // Stores the new paragraphs
    wxRichTextParagraphLayoutBox    m_newParagraphs;

    // Stores the old paragraphs
    wxRichTextParagraphLayoutBox    m_oldParagraphs;

    // The affected range
    wxRichTextRange                 m_range;

    // The insertion point for this command
    long                            m_position;

    // Ignore 1st 'Do' operation because we already did it
    bool                            m_ignoreThis;

    // The command identifier
    wxRichTextCommandId             m_cmdId;
};

/*!
 * Handler flags
 */

// Include style sheet when loading and saving
#define wxRICHTEXT_HANDLER_INCLUDE_STYLESHEET       0x0001

// Save images to memory file system in HTML handler
#define wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_MEMORY    0x0010

// Save images to files in HTML handler
#define wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_FILES     0x0020

// Save images as inline base64 data in HTML handler
#define wxRICHTEXT_HANDLER_SAVE_IMAGES_TO_BASE64    0x0040

// Don't write header and footer (or BODY), so we can include the fragment
// in a larger document
#define wxRICHTEXT_HANDLER_NO_HEADER_FOOTER         0x0080

// Convert the more common face names to names that will work on the current platform
// in a larger document
#define wxRICHTEXT_HANDLER_CONVERT_FACENAMES        0x0100

/*!
 * wxRichTextFileHandler
 * Base class for file handlers
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextFileHandler: public wxObject
{
    DECLARE_CLASS(wxRichTextFileHandler)
public:
    wxRichTextFileHandler(const wxString& name = wxEmptyString, const wxString& ext = wxEmptyString, int type = 0)
        : m_name(name), m_extension(ext), m_type(type), m_flags(0), m_visible(true)
        { }

#if wxUSE_STREAMS
    bool LoadFile(wxRichTextBuffer *buffer, wxInputStream& stream)
    { return DoLoadFile(buffer, stream); }
    bool SaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream)
    { return DoSaveFile(buffer, stream); }
#endif

    bool LoadFile(wxRichTextBuffer *buffer, const wxString& filename);
    bool SaveFile(wxRichTextBuffer *buffer, const wxString& filename);

    /// Can we handle this filename (if using files)? By default, checks the extension.
    virtual bool CanHandle(const wxString& filename) const;

    /// Can we save using this handler?
    virtual bool CanSave() const { return false; }

    /// Can we load using this handler?
    virtual bool CanLoad() const { return false; }

    /// Should this handler be visible to the user?
    virtual bool IsVisible() const { return m_visible; }
    virtual void SetVisible(bool visible) { m_visible = visible; }

    /// The name of the nandler
    void SetName(const wxString& name) { m_name = name; }
    wxString GetName() const { return m_name; }

    /// The default extension to recognise
    void SetExtension(const wxString& ext) { m_extension = ext; }
    wxString GetExtension() const { return m_extension; }

    /// The handler type
    void SetType(int type) { m_type = type; }
    int GetType() const { return m_type; }

    /// Flags controlling how loading and saving is done
    void SetFlags(int flags) { m_flags = flags; }
    int GetFlags() const { return m_flags; }

    /// Encoding to use when saving a file. If empty, a suitable encoding is chosen
    void SetEncoding(const wxString& encoding) { m_encoding = encoding; }
    const wxString& GetEncoding() const { return m_encoding; }

protected:

#if wxUSE_STREAMS
    virtual bool DoLoadFile(wxRichTextBuffer *buffer, wxInputStream& stream) = 0;
    virtual bool DoSaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream) = 0;
#endif

    wxString  m_name;
    wxString  m_encoding;
    wxString  m_extension;
    int       m_type;
    int       m_flags;
    bool      m_visible;
};

/*!
 * wxRichTextPlainTextHandler
 * Plain text handler
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextPlainTextHandler: public wxRichTextFileHandler
{
    DECLARE_CLASS(wxRichTextPlainTextHandler)
public:
    wxRichTextPlainTextHandler(const wxString& name = wxT("Text"), const wxString& ext = wxT("txt"), int type = wxRICHTEXT_TYPE_TEXT)
        : wxRichTextFileHandler(name, ext, type)
        { }

    /// Can we save using this handler?
    virtual bool CanSave() const { return true; }

    /// Can we load using this handler?
    virtual bool CanLoad() const { return true; }

protected:

#if wxUSE_STREAMS
    virtual bool DoLoadFile(wxRichTextBuffer *buffer, wxInputStream& stream);
    virtual bool DoSaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream);
#endif

};

#if wxUSE_DATAOBJ

/*!
 * The data object for a wxRichTextBuffer
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextBufferDataObject: public wxDataObjectSimple
{
public:
    // ctor doesn't copy the pointer, so it shouldn't go away while this object
    // is alive
    wxRichTextBufferDataObject(wxRichTextBuffer* richTextBuffer = (wxRichTextBuffer*) NULL);
    virtual ~wxRichTextBufferDataObject();

    // after a call to this function, the buffer is owned by the caller and it
    // is responsible for deleting it
    wxRichTextBuffer* GetRichTextBuffer();

    // Returns the id for the new data format
    static const wxChar* GetRichTextBufferFormatId() { return ms_richTextBufferFormatId; }

    // base class pure virtuals

    virtual wxDataFormat GetPreferredFormat(Direction dir) const;
    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *pBuf) const;
    virtual bool SetData(size_t len, const void *buf);

    // prevent warnings

    virtual size_t GetDataSize(const wxDataFormat&) const { return GetDataSize(); }
    virtual bool GetDataHere(const wxDataFormat&, void *buf) const { return GetDataHere(buf); }
    virtual bool SetData(const wxDataFormat&, size_t len, const void *buf) { return SetData(len, buf); }

private:
    wxDataFormat            m_formatRichTextBuffer;     // our custom format
    wxRichTextBuffer*       m_richTextBuffer;           // our data
    static const wxChar*    ms_richTextBufferFormatId;  // our format id
};

#endif

/*!
 * wxRichTextRenderer isolates common drawing functionality
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextRenderer: public wxObject
{
public:
    wxRichTextRenderer() {}
    virtual ~wxRichTextRenderer() {}

    /// Draw a standard bullet, as specified by the value of GetBulletName
    virtual bool DrawStandardBullet(wxRichTextParagraph* paragraph, wxDC& dc, const wxTextAttrEx& attr, const wxRect& rect) = 0;

    /// Draw a bullet that can be described by text, such as numbered or symbol bullets
    virtual bool DrawTextBullet(wxRichTextParagraph* paragraph, wxDC& dc, const wxTextAttrEx& attr, const wxRect& rect, const wxString& text) = 0;

    /// Draw a bitmap bullet, where the bullet bitmap is specified by the value of GetBulletName
    virtual bool DrawBitmapBullet(wxRichTextParagraph* paragraph, wxDC& dc, const wxTextAttrEx& attr, const wxRect& rect) = 0;

    /// Enumerate the standard bullet names currently supported
    virtual bool EnumerateStandardBulletNames(wxArrayString& bulletNames) = 0;
};

/*!
 * wxRichTextStdRenderer: standard renderer
 */

class WXDLLIMPEXP_RICHTEXT wxRichTextStdRenderer: public wxRichTextRenderer
{
public:
    wxRichTextStdRenderer() {}

    /// Draw a standard bullet, as specified by the value of GetBulletName
    virtual bool DrawStandardBullet(wxRichTextParagraph* paragraph, wxDC& dc, const wxTextAttrEx& attr, const wxRect& rect);

    /// Draw a bullet that can be described by text, such as numbered or symbol bullets
    virtual bool DrawTextBullet(wxRichTextParagraph* paragraph, wxDC& dc, const wxTextAttrEx& attr, const wxRect& rect, const wxString& text);

    /// Draw a bitmap bullet, where the bullet bitmap is specified by the value of GetBulletName
    virtual bool DrawBitmapBullet(wxRichTextParagraph* paragraph, wxDC& dc, const wxTextAttrEx& attr, const wxRect& rect);

    /// Enumerate the standard bullet names currently supported
    virtual bool EnumerateStandardBulletNames(wxArrayString& bulletNames);
};

/*!
 * Utilities
 *
 */

inline bool wxRichTextHasStyle(int flags, int style)
{
    return ((flags & style) == style);
}

/// Compare two attribute objects
WXDLLIMPEXP_RICHTEXT bool wxTextAttrEq(const wxTextAttrEx& attr1, const wxTextAttrEx& attr2);
WXDLLIMPEXP_RICHTEXT bool wxTextAttrEq(const wxTextAttr& attr1, const wxRichTextAttr& attr2);

/// Compare two attribute objects, but take into account the flags
/// specifying attributes of interest.
WXDLLIMPEXP_RICHTEXT bool wxTextAttrEqPartial(const wxTextAttrEx& attr1, const wxTextAttrEx& attr2, int flags);
WXDLLIMPEXP_RICHTEXT bool wxTextAttrEqPartial(const wxTextAttrEx& attr1, const wxRichTextAttr& attr2, int flags);

/// Apply one style to another
WXDLLIMPEXP_RICHTEXT bool wxRichTextApplyStyle(wxTextAttrEx& destStyle, const wxTextAttrEx& style);
WXDLLIMPEXP_RICHTEXT bool wxRichTextApplyStyle(wxRichTextAttr& destStyle, const wxTextAttrEx& style);
WXDLLIMPEXP_RICHTEXT bool wxRichTextApplyStyle(wxTextAttrEx& destStyle, const wxRichTextAttr& style, wxRichTextAttr* compareWith = NULL);
WXDLLIMPEXP_RICHTEXT bool wxRichTextApplyStyle(wxRichTextAttr& destStyle, const wxRichTextAttr& style, wxRichTextAttr* compareWith = NULL);

// Remove attributes
WXDLLIMPEXP_RICHTEXT bool wxRichTextRemoveStyle(wxTextAttrEx& destStyle, const wxRichTextAttr& style);

/// Combine two bitlists
WXDLLIMPEXP_RICHTEXT bool wxRichTextCombineBitlists(int& valueA, int valueB, int& flagsA, int flagsB);

/// Compare two bitlists
WXDLLIMPEXP_RICHTEXT bool wxRichTextBitlistsEqPartial(int valueA, int valueB, int flags);

/// Split into paragraph and character styles
WXDLLIMPEXP_RICHTEXT bool wxRichTextSplitParaCharStyles(const wxTextAttrEx& style, wxTextAttrEx& parStyle, wxTextAttrEx& charStyle);

/// Compare tabs
WXDLLIMPEXP_RICHTEXT bool wxRichTextTabsEq(const wxArrayInt& tabs1, const wxArrayInt& tabs2);

/// Set the font without changing the font attributes
WXDLLIMPEXP_RICHTEXT void wxSetFontPreservingStyles(wxTextAttr& attr, const wxFont& font);

/// Convert a decimal to Roman numerals
WXDLLIMPEXP_RICHTEXT wxString wxRichTextDecimalToRoman(long n);

WXDLLIMPEXP_RICHTEXT void wxRichTextModuleInit();

#endif
    // wxUSE_RICHTEXT

#endif
    // _WX_RICHTEXTBUFFER_H_

