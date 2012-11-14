/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/intl.cpp
// Purpose:     Internationalization and localisation for wxWidgets
// Author:      Vadim Zeitlin
// Modified by: Michael N. Filippov <michael@idisys.iae.nsk.su>
//              (2003/09/30 - PluralForms support)
// Created:     29/01/98
// RCS-ID:      $Id: intl.cpp 61340 2009-07-06 21:19:58Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declaration
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifdef __EMX__
// The following define is needed by Innotek's libc to
// make the definition of struct localeconv available.
#define __INTERNAL_DEFS
#endif

#if wxUSE_INTL

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/hashmap.h"
    #include "wx/module.h"
#endif // WX_PRECOMP

#ifndef __WXWINCE__
    #include <locale.h>
#endif

// standard headers
#include <ctype.h>
#include <stdlib.h>
#ifdef HAVE_LANGINFO_H
    #include <langinfo.h>
#endif

#ifdef __WIN32__
    #include "wx/msw/private.h"
#elif defined(__UNIX_LIKE__)
    #include "wx/fontmap.h"         // for CharsetToEncoding()
#endif

#include "wx/file.h"
#include "wx/filename.h"
#include "wx/tokenzr.h"
#include "wx/fontmap.h"
#include "wx/encconv.h"
#include "wx/ptr_scpd.h"
#include "wx/apptrait.h"
#include "wx/stdpaths.h"

#if defined(__WXMAC__)
    #include  "wx/mac/private.h"  // includes mac headers
#endif

#if defined(__DARWIN__)
    #include "wx/mac/corefoundation/cfref.h"
    #include <CoreFoundation/CFLocale.h>
    #include "wx/mac/corefoundation/cfstring.h"
#endif

// ----------------------------------------------------------------------------
// simple types
// ----------------------------------------------------------------------------

// this should *not* be wxChar, this type must have exactly 8 bits!
typedef wxUint8 size_t8;
typedef wxUint32 size_t32;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// magic number identifying the .mo format file
const size_t32 MSGCATALOG_MAGIC    = 0x950412de;
const size_t32 MSGCATALOG_MAGIC_SW = 0xde120495;

// the constants describing the format of lang_LANG locale string
static const size_t LEN_LANG = 2;
static const size_t LEN_SUBLANG = 2;
static const size_t LEN_FULL = LEN_LANG + 1 + LEN_SUBLANG; // 1 for '_'

#define TRACE_I18N _T("i18n")

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

#ifdef __WXDEBUG__

// small class to suppress the translation erros until exit from current scope
class NoTransErr
{
public:
    NoTransErr() { ms_suppressCount++; }
   ~NoTransErr() { ms_suppressCount--;  }

   static bool Suppress() { return ms_suppressCount > 0; }

private:
   static size_t ms_suppressCount;
};

size_t NoTransErr::ms_suppressCount = 0;

#else // !Debug

class NoTransErr
{
public:
    NoTransErr() { }
   ~NoTransErr() { }
};

#endif // Debug/!Debug

static wxLocale *wxSetLocale(wxLocale *pLocale);

// helper functions of GetSystemLanguage()
#ifdef __UNIX__

// get just the language part
static inline wxString ExtractLang(const wxString& langFull)
{
    return langFull.Left(LEN_LANG);
}

// get everything else (including the leading '_')
static inline wxString ExtractNotLang(const wxString& langFull)
{
    return langFull.Mid(LEN_LANG);
}

#endif // __UNIX__


// ----------------------------------------------------------------------------
// Plural forms parser
// ----------------------------------------------------------------------------

/*
                                Simplified Grammar

Expression:
    LogicalOrExpression '?' Expression ':' Expression
    LogicalOrExpression

LogicalOrExpression:
    LogicalAndExpression "||" LogicalOrExpression   // to (a || b) || c
    LogicalAndExpression

LogicalAndExpression:
    EqualityExpression "&&" LogicalAndExpression    // to (a && b) && c
    EqualityExpression

EqualityExpression:
    RelationalExpression "==" RelationalExperession
    RelationalExpression "!=" RelationalExperession
    RelationalExpression

RelationalExpression:
    MultiplicativeExpression '>' MultiplicativeExpression
    MultiplicativeExpression '<' MultiplicativeExpression
    MultiplicativeExpression ">=" MultiplicativeExpression
    MultiplicativeExpression "<=" MultiplicativeExpression
    MultiplicativeExpression

MultiplicativeExpression:
    PmExpression '%' PmExpression
    PmExpression

PmExpression:
    N
    Number
    '(' Expression ')'
*/

class wxPluralFormsToken
{
public:
    enum Type
    {
        T_ERROR, T_EOF, T_NUMBER, T_N, T_PLURAL, T_NPLURALS, T_EQUAL, T_ASSIGN,
        T_GREATER, T_GREATER_OR_EQUAL, T_LESS, T_LESS_OR_EQUAL,
        T_REMINDER, T_NOT_EQUAL,
        T_LOGICAL_AND, T_LOGICAL_OR, T_QUESTION, T_COLON, T_SEMICOLON,
        T_LEFT_BRACKET, T_RIGHT_BRACKET
    };
    Type type() const { return m_type; }
    void setType(Type type) { m_type = type; }
    // for T_NUMBER only
    typedef int Number;
    Number number() const { return m_number; }
    void setNumber(Number num) { m_number = num; }
private:
    Type m_type;
    Number m_number;
};


class wxPluralFormsScanner
{
public:
    wxPluralFormsScanner(const char* s);
    const wxPluralFormsToken& token() const { return m_token; }
    bool nextToken();  // returns false if error
private:
    const char* m_s;
    wxPluralFormsToken m_token;
};

wxPluralFormsScanner::wxPluralFormsScanner(const char* s) : m_s(s)
{
    nextToken();
}

bool wxPluralFormsScanner::nextToken()
{
    wxPluralFormsToken::Type type = wxPluralFormsToken::T_ERROR;
    while (isspace((unsigned char) *m_s))
    {
        ++m_s;
    }
    if (*m_s == 0)
    {
        type = wxPluralFormsToken::T_EOF;
    }
    else if (isdigit((unsigned char) *m_s))
    {
        wxPluralFormsToken::Number number = *m_s++ - '0';
        while (isdigit((unsigned char) *m_s))
        {
            number = number * 10 + (*m_s++ - '0');
        }
        m_token.setNumber(number);
        type = wxPluralFormsToken::T_NUMBER;
    }
    else if (isalpha((unsigned char) *m_s))
    {
        const char* begin = m_s++;
        while (isalnum((unsigned char) *m_s))
        {
            ++m_s;
        }
        size_t size = m_s - begin;
        if (size == 1 && memcmp(begin, "n", size) == 0)
        {
            type = wxPluralFormsToken::T_N;
        }
        else if (size == 6 && memcmp(begin, "plural", size) == 0)
        {
            type = wxPluralFormsToken::T_PLURAL;
        }
        else if (size == 8 && memcmp(begin, "nplurals", size) == 0)
        {
            type = wxPluralFormsToken::T_NPLURALS;
        }
    }
    else if (*m_s == '=')
    {
        ++m_s;
        if (*m_s == '=')
        {
            ++m_s;
            type = wxPluralFormsToken::T_EQUAL;
        }
        else
        {
            type = wxPluralFormsToken::T_ASSIGN;
        }
    }
    else if (*m_s == '>')
    {
        ++m_s;
        if (*m_s == '=')
        {
            ++m_s;
            type = wxPluralFormsToken::T_GREATER_OR_EQUAL;
        }
        else
        {
            type = wxPluralFormsToken::T_GREATER;
        }
    }
    else if (*m_s == '<')
    {
        ++m_s;
        if (*m_s == '=')
        {
            ++m_s;
            type = wxPluralFormsToken::T_LESS_OR_EQUAL;
        }
        else
        {
            type = wxPluralFormsToken::T_LESS;
        }
    }
    else if (*m_s == '%')
    {
        ++m_s;
        type = wxPluralFormsToken::T_REMINDER;
    }
    else if (*m_s == '!' && m_s[1] == '=')
    {
        m_s += 2;
        type = wxPluralFormsToken::T_NOT_EQUAL;
    }
    else if (*m_s == '&' && m_s[1] == '&')
    {
        m_s += 2;
        type = wxPluralFormsToken::T_LOGICAL_AND;
    }
    else if (*m_s == '|' && m_s[1] == '|')
    {
        m_s += 2;
        type = wxPluralFormsToken::T_LOGICAL_OR;
    }
    else if (*m_s == '?')
    {
        ++m_s;
        type = wxPluralFormsToken::T_QUESTION;
    }
    else if (*m_s == ':')
    {
        ++m_s;
        type = wxPluralFormsToken::T_COLON;
    } else if (*m_s == ';') {
        ++m_s;
        type = wxPluralFormsToken::T_SEMICOLON;
    }
    else if (*m_s == '(')
    {
        ++m_s;
        type = wxPluralFormsToken::T_LEFT_BRACKET;
    }
    else if (*m_s == ')')
    {
        ++m_s;
        type = wxPluralFormsToken::T_RIGHT_BRACKET;
    }
    m_token.setType(type);
    return type != wxPluralFormsToken::T_ERROR;
}

class wxPluralFormsNode;

// NB: Can't use wxDEFINE_SCOPED_PTR_TYPE because wxPluralFormsNode is not
//     fully defined yet:
class wxPluralFormsNodePtr
{
public:
    wxPluralFormsNodePtr(wxPluralFormsNode *p = NULL) : m_p(p) {}
    ~wxPluralFormsNodePtr();
    wxPluralFormsNode& operator*() const { return *m_p; }
    wxPluralFormsNode* operator->() const { return m_p; }
    wxPluralFormsNode* get() const { return m_p; }
    wxPluralFormsNode* release();
    void reset(wxPluralFormsNode *p);

private:
    wxPluralFormsNode *m_p;
};

class wxPluralFormsNode
{
public:
    wxPluralFormsNode(const wxPluralFormsToken& token) : m_token(token) {}
    const wxPluralFormsToken& token() const { return m_token; }
    const wxPluralFormsNode* node(size_t i) const
        { return m_nodes[i].get(); }
    void setNode(size_t i, wxPluralFormsNode* n);
    wxPluralFormsNode* releaseNode(size_t i);
    wxPluralFormsToken::Number evaluate(wxPluralFormsToken::Number n) const;

private:
    wxPluralFormsToken m_token;
    wxPluralFormsNodePtr m_nodes[3];
};

wxPluralFormsNodePtr::~wxPluralFormsNodePtr()
{
    delete m_p;
}
wxPluralFormsNode* wxPluralFormsNodePtr::release()
{
    wxPluralFormsNode *p = m_p;
    m_p = NULL;
    return p;
}
void wxPluralFormsNodePtr::reset(wxPluralFormsNode *p)
{
    if (p != m_p)
    {
        delete m_p;
        m_p = p;
    }
}


void wxPluralFormsNode::setNode(size_t i, wxPluralFormsNode* n)
{
    m_nodes[i].reset(n);
}

wxPluralFormsNode*  wxPluralFormsNode::releaseNode(size_t i)
{
    return m_nodes[i].release();
}

wxPluralFormsToken::Number
wxPluralFormsNode::evaluate(wxPluralFormsToken::Number n) const
{
    switch (token().type())
    {
        // leaf
        case wxPluralFormsToken::T_NUMBER:
            return token().number();
        case wxPluralFormsToken::T_N:
            return n;
        // 2 args
        case wxPluralFormsToken::T_EQUAL:
            return node(0)->evaluate(n) == node(1)->evaluate(n);
        case wxPluralFormsToken::T_NOT_EQUAL:
            return node(0)->evaluate(n) != node(1)->evaluate(n);
        case wxPluralFormsToken::T_GREATER:
            return node(0)->evaluate(n) > node(1)->evaluate(n);
        case wxPluralFormsToken::T_GREATER_OR_EQUAL:
            return node(0)->evaluate(n) >= node(1)->evaluate(n);
        case wxPluralFormsToken::T_LESS:
            return node(0)->evaluate(n) < node(1)->evaluate(n);
        case wxPluralFormsToken::T_LESS_OR_EQUAL:
            return node(0)->evaluate(n) <= node(1)->evaluate(n);
        case wxPluralFormsToken::T_REMINDER:
            {
                wxPluralFormsToken::Number number = node(1)->evaluate(n);
                if (number != 0)
                {
                    return node(0)->evaluate(n) % number;
                }
                else
                {
                    return 0;
                }
            }
        case wxPluralFormsToken::T_LOGICAL_AND:
            return node(0)->evaluate(n) && node(1)->evaluate(n);
        case wxPluralFormsToken::T_LOGICAL_OR:
            return node(0)->evaluate(n) || node(1)->evaluate(n);
        // 3 args
        case wxPluralFormsToken::T_QUESTION:
            return node(0)->evaluate(n)
                ? node(1)->evaluate(n)
                : node(2)->evaluate(n);
        default:
            return 0;
    }
}


class wxPluralFormsCalculator
{
public:
    wxPluralFormsCalculator() : m_nplurals(0), m_plural(0) {}

    // input: number, returns msgstr index
    int evaluate(int n) const;

    // input: text after "Plural-Forms:" (e.g. "nplurals=2; plural=(n != 1);"),
    // if s == 0, creates default handler
    // returns 0 if error
    static wxPluralFormsCalculator* make(const char* s = 0);

    ~wxPluralFormsCalculator() {}

    void  init(wxPluralFormsToken::Number nplurals, wxPluralFormsNode* plural);

private:
    wxPluralFormsToken::Number m_nplurals;
    wxPluralFormsNodePtr m_plural;
};

wxDEFINE_SCOPED_PTR_TYPE(wxPluralFormsCalculator)

void wxPluralFormsCalculator::init(wxPluralFormsToken::Number nplurals,
                                   wxPluralFormsNode* plural)
{
    m_nplurals = nplurals;
    m_plural.reset(plural);
}

int wxPluralFormsCalculator::evaluate(int n) const
{
    if (m_plural.get() == 0)
    {
        return 0;
    }
    wxPluralFormsToken::Number number = m_plural->evaluate(n);
    if (number < 0 || number > m_nplurals)
    {
        return 0;
    }
    return number;
}


class wxPluralFormsParser
{
public:
    wxPluralFormsParser(wxPluralFormsScanner& scanner) : m_scanner(scanner) {}
    bool parse(wxPluralFormsCalculator& rCalculator);

private:
    wxPluralFormsNode* parsePlural();
    // stops at T_SEMICOLON, returns 0 if error
    wxPluralFormsScanner& m_scanner;
    const wxPluralFormsToken& token() const;
    bool nextToken();

    wxPluralFormsNode* expression();
    wxPluralFormsNode* logicalOrExpression();
    wxPluralFormsNode* logicalAndExpression();
    wxPluralFormsNode* equalityExpression();
    wxPluralFormsNode* multiplicativeExpression();
    wxPluralFormsNode* relationalExpression();
    wxPluralFormsNode* pmExpression();
};

bool wxPluralFormsParser::parse(wxPluralFormsCalculator& rCalculator)
{
    if (token().type() != wxPluralFormsToken::T_NPLURALS)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != wxPluralFormsToken::T_ASSIGN)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != wxPluralFormsToken::T_NUMBER)
        return false;
    wxPluralFormsToken::Number nplurals = token().number();
    if (!nextToken())
        return false;
    if (token().type() != wxPluralFormsToken::T_SEMICOLON)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != wxPluralFormsToken::T_PLURAL)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != wxPluralFormsToken::T_ASSIGN)
        return false;
    if (!nextToken())
        return false;
    wxPluralFormsNode* plural = parsePlural();
    if (plural == 0)
        return false;
    if (token().type() != wxPluralFormsToken::T_SEMICOLON)
        return false;
    if (!nextToken())
        return false;
    if (token().type() != wxPluralFormsToken::T_EOF)
        return false;
    rCalculator.init(nplurals, plural);
    return true;
}

wxPluralFormsNode* wxPluralFormsParser::parsePlural()
{
    wxPluralFormsNode* p = expression();
    if (p == NULL)
    {
        return NULL;
    }
    wxPluralFormsNodePtr n(p);
    if (token().type() != wxPluralFormsToken::T_SEMICOLON)
    {
        return NULL;
    }
    return n.release();
}

const wxPluralFormsToken& wxPluralFormsParser::token() const
{
    return m_scanner.token();
}

bool wxPluralFormsParser::nextToken()
{
    if (!m_scanner.nextToken())
        return false;
    return true;
}

wxPluralFormsNode* wxPluralFormsParser::expression()
{
    wxPluralFormsNode* p = logicalOrExpression();
    if (p == NULL)
        return NULL;
    wxPluralFormsNodePtr n(p);
    if (token().type() == wxPluralFormsToken::T_QUESTION)
    {
        wxPluralFormsNodePtr qn(new wxPluralFormsNode(token()));
        if (!nextToken())
        {
            return 0;
        }
        p = expression();
        if (p == 0)
        {
            return 0;
        }
        qn->setNode(1, p);
        if (token().type() != wxPluralFormsToken::T_COLON)
        {
            return 0;
        }
        if (!nextToken())
        {
            return 0;
        }
        p = expression();
        if (p == 0)
        {
            return 0;
        }
        qn->setNode(2, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

wxPluralFormsNode*wxPluralFormsParser::logicalOrExpression()
{
    wxPluralFormsNode* p = logicalAndExpression();
    if (p == NULL)
        return NULL;
    wxPluralFormsNodePtr ln(p);
    if (token().type() == wxPluralFormsToken::T_LOGICAL_OR)
    {
        wxPluralFormsNodePtr un(new wxPluralFormsNode(token()));
        if (!nextToken())
        {
            return 0;
        }
        p = logicalOrExpression();
        if (p == 0)
        {
            return 0;
        }
        wxPluralFormsNodePtr rn(p);    // right
        if (rn->token().type() == wxPluralFormsToken::T_LOGICAL_OR)
        {
            // see logicalAndExpression comment
            un->setNode(0, ln.release());
            un->setNode(1, rn->releaseNode(0));
            rn->setNode(0, un.release());
            return rn.release();
        }


        un->setNode(0, ln.release());
        un->setNode(1, rn.release());
        return un.release();
    }
    return ln.release();
}

wxPluralFormsNode* wxPluralFormsParser::logicalAndExpression()
{
    wxPluralFormsNode* p = equalityExpression();
    if (p == NULL)
        return NULL;
    wxPluralFormsNodePtr ln(p);   // left
    if (token().type() == wxPluralFormsToken::T_LOGICAL_AND)
    {
        wxPluralFormsNodePtr un(new wxPluralFormsNode(token()));  // up
        if (!nextToken())
        {
            return NULL;
        }
        p = logicalAndExpression();
        if (p == 0)
        {
            return NULL;
        }
        wxPluralFormsNodePtr rn(p);    // right
        if (rn->token().type() == wxPluralFormsToken::T_LOGICAL_AND)
        {
// transform 1 && (2 && 3) -> (1 && 2) && 3
//     u                  r
// l       r     ->   u      3
//       2   3      l   2
            un->setNode(0, ln.release());
            un->setNode(1, rn->releaseNode(0));
            rn->setNode(0, un.release());
            return rn.release();
        }

        un->setNode(0, ln.release());
        un->setNode(1, rn.release());
        return un.release();
    }
    return ln.release();
}

wxPluralFormsNode* wxPluralFormsParser::equalityExpression()
{
    wxPluralFormsNode* p = relationalExpression();
    if (p == NULL)
        return NULL;
    wxPluralFormsNodePtr n(p);
    if (token().type() == wxPluralFormsToken::T_EQUAL
        || token().type() == wxPluralFormsToken::T_NOT_EQUAL)
    {
        wxPluralFormsNodePtr qn(new wxPluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
        p = relationalExpression();
        if (p == NULL)
        {
            return NULL;
        }
        qn->setNode(1, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

wxPluralFormsNode* wxPluralFormsParser::relationalExpression()
{
    wxPluralFormsNode* p = multiplicativeExpression();
    if (p == NULL)
        return NULL;
    wxPluralFormsNodePtr n(p);
    if (token().type() == wxPluralFormsToken::T_GREATER
            || token().type() == wxPluralFormsToken::T_LESS
            || token().type() == wxPluralFormsToken::T_GREATER_OR_EQUAL
            || token().type() == wxPluralFormsToken::T_LESS_OR_EQUAL)
    {
        wxPluralFormsNodePtr qn(new wxPluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
        p = multiplicativeExpression();
        if (p == NULL)
        {
            return NULL;
        }
        qn->setNode(1, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

wxPluralFormsNode* wxPluralFormsParser::multiplicativeExpression()
{
    wxPluralFormsNode* p = pmExpression();
    if (p == NULL)
        return NULL;
    wxPluralFormsNodePtr n(p);
    if (token().type() == wxPluralFormsToken::T_REMINDER)
    {
        wxPluralFormsNodePtr qn(new wxPluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
        p = pmExpression();
        if (p == NULL)
        {
            return NULL;
        }
        qn->setNode(1, p);
        qn->setNode(0, n.release());
        return qn.release();
    }
    return n.release();
}

wxPluralFormsNode* wxPluralFormsParser::pmExpression()
{
    wxPluralFormsNodePtr n;
    if (token().type() == wxPluralFormsToken::T_N
        || token().type() == wxPluralFormsToken::T_NUMBER)
    {
        n.reset(new wxPluralFormsNode(token()));
        if (!nextToken())
        {
            return NULL;
        }
    }
    else if (token().type() == wxPluralFormsToken::T_LEFT_BRACKET) {
        if (!nextToken())
        {
            return NULL;
        }
        wxPluralFormsNode* p = expression();
        if (p == NULL)
        {
            return NULL;
        }
        n.reset(p);
        if (token().type() != wxPluralFormsToken::T_RIGHT_BRACKET)
        {
            return NULL;
        }
        if (!nextToken())
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
    return n.release();
}

wxPluralFormsCalculator* wxPluralFormsCalculator::make(const char* s)
{
    wxPluralFormsCalculatorPtr calculator(new wxPluralFormsCalculator);
    if (s != NULL)
    {
        wxPluralFormsScanner scanner(s);
        wxPluralFormsParser p(scanner);
        if (!p.parse(*calculator))
        {
            return NULL;
        }
    }
    return calculator.release();
}




// ----------------------------------------------------------------------------
// wxMsgCatalogFile corresponds to one disk-file message catalog.
//
// This is a "low-level" class and is used only by wxMsgCatalog
// ----------------------------------------------------------------------------

WX_DECLARE_EXPORTED_STRING_HASH_MAP(wxString, wxMessagesHash);

class wxMsgCatalogFile
{
public:
    // ctor & dtor
    wxMsgCatalogFile();
   ~wxMsgCatalogFile();

    // load the catalog from disk (szDirPrefix corresponds to language)
    bool Load(const wxChar *szDirPrefix, const wxChar *szName,
              wxPluralFormsCalculatorPtr& rPluralFormsCalculator);

    // fills the hash with string-translation pairs
    void FillHash(wxMessagesHash& hash,
                  const wxString& msgIdCharset,
                  bool convertEncoding) const;

    // return the charset of the strings in this catalog or empty string if
    // none/unknown
    wxString GetCharset() const { return m_charset; }

private:
    // this implementation is binary compatible with GNU gettext() version 0.10

    // an entry in the string table
    struct wxMsgTableEntry
    {
      size_t32   nLen;           // length of the string
      size_t32   ofsString;      // pointer to the string
    };

    // header of a .mo file
    struct wxMsgCatalogHeader
    {
      size_t32  magic,          // offset +00:  magic id
                revision,       //        +04:  revision
                numStrings;     //        +08:  number of strings in the file
      size_t32  ofsOrigTable,   //        +0C:  start of original string table
                ofsTransTable;  //        +10:  start of translated string table
      size_t32  nHashSize,      //        +14:  hash table size
                ofsHashTable;   //        +18:  offset of hash table start
    };

    // all data is stored here, NULL if no data loaded
    size_t8 *m_pData;

    // amount of memory pointed to by m_pData.
    size_t32 m_nSize;

    // data description
    size_t32          m_numStrings;   // number of strings in this domain
    wxMsgTableEntry  *m_pOrigTable,   // pointer to original   strings
                     *m_pTransTable;  //            translated

    wxString m_charset;               // from the message catalog header


    // swap the 2 halves of 32 bit integer if needed
    size_t32 Swap(size_t32 ui) const
    {
          return m_bSwapped ? (ui << 24) | ((ui & 0xff00) << 8) |
                              ((ui >> 8) & 0xff00) | (ui >> 24)
                            : ui;
    }

    const char *StringAtOfs(wxMsgTableEntry *pTable, size_t32 n) const
    {
        const wxMsgTableEntry * const ent = pTable + n;

        // this check could fail for a corrupt message catalog
        size_t32 ofsString = Swap(ent->ofsString);
        if ( ofsString + Swap(ent->nLen) > m_nSize)
        {
            return NULL;
        }

        return (const char *)(m_pData + ofsString);
    }

    bool m_bSwapped;   // wrong endianness?

    DECLARE_NO_COPY_CLASS(wxMsgCatalogFile)
};


// ----------------------------------------------------------------------------
// wxMsgCatalog corresponds to one loaded message catalog.
//
// This is a "low-level" class and is used only by wxLocale (that's why
// it's designed to be stored in a linked list)
// ----------------------------------------------------------------------------

class wxMsgCatalog
{
public:
#if !wxUSE_UNICODE
    wxMsgCatalog() { m_conv = NULL; }
    ~wxMsgCatalog();
#endif

    // load the catalog from disk (szDirPrefix corresponds to language)
    bool Load(const wxChar *szDirPrefix, const wxChar *szName,
              const wxChar *msgIdCharset = NULL, bool bConvertEncoding = false);

    // get name of the catalog
    wxString GetName() const { return m_name; }

    // get the translated string: returns NULL if not found
    const wxChar *GetString(const wxChar *sz, size_t n = size_t(-1)) const;

    // public variable pointing to the next element in a linked list (or NULL)
    wxMsgCatalog *m_pNext;

private:
    wxMessagesHash  m_messages; // all messages in the catalog
    wxString        m_name;     // name of the domain

#if !wxUSE_UNICODE
    // the conversion corresponding to this catalog charset if we installed it
    // as the global one
    wxCSConv *m_conv;
#endif

    wxPluralFormsCalculatorPtr  m_pluralFormsCalculator;
};

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// the list of the directories to search for message catalog files
static wxArrayString gs_searchPrefixes;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMsgCatalogFile class
// ----------------------------------------------------------------------------

wxMsgCatalogFile::wxMsgCatalogFile()
{
    m_pData = NULL;
    m_nSize = 0;
}

wxMsgCatalogFile::~wxMsgCatalogFile()
{
    delete [] m_pData;
}

// return the directories to search for message catalogs under the given
// prefix, separated by wxPATH_SEP
static
wxString GetMsgCatalogSubdirs(const wxChar *prefix, const wxChar *lang)
{
    // Search first in Unix-standard prefix/lang/LC_MESSAGES, then in
    // prefix/lang and finally in just prefix.
    //
    // Note that we use LC_MESSAGES on all platforms and not just Unix, because
    // it doesn't cost much to look into one more directory and doing it this
    // way has two important benefits:
    // a) we don't break compatibility with wx-2.6 and older by stopping to
    //    look in a directory where the catalogs used to be and thus silently
    //    breaking apps after they are recompiled against the latest wx
    // b) it makes it possible to package app's support files in the same
    //    way on all target platforms
    wxString pathPrefix;
    pathPrefix << prefix << wxFILE_SEP_PATH << lang;

    wxString searchPath;
    searchPath.reserve(4*pathPrefix.length());
    searchPath << pathPrefix << wxFILE_SEP_PATH << wxT("LC_MESSAGES") << wxPATH_SEP
               << prefix << wxFILE_SEP_PATH << wxPATH_SEP
               << pathPrefix;

    return searchPath;
}

// construct the search path for the given language
static wxString GetFullSearchPath(const wxChar *lang)
{
    // first take the entries explicitly added by the program
    wxArrayString paths;
    paths.reserve(gs_searchPrefixes.size() + 1);
    size_t n,
           count = gs_searchPrefixes.size();
    for ( n = 0; n < count; n++ )
    {
        paths.Add(GetMsgCatalogSubdirs(gs_searchPrefixes[n], lang));
    }


#if wxUSE_STDPATHS
    // then look in the standard location
    const wxString stdp = wxStandardPaths::Get().
        GetLocalizedResourcesDir(lang, wxStandardPaths::ResourceCat_Messages);

    if ( paths.Index(stdp) == wxNOT_FOUND )
        paths.Add(stdp);
#endif // wxUSE_STDPATHS

    // last look in default locations
#ifdef __UNIX__
    // LC_PATH is a standard env var containing the search path for the .mo
    // files
    const wxChar *pszLcPath = wxGetenv(wxT("LC_PATH"));
    if ( pszLcPath )
    {
        const wxString lcp = GetMsgCatalogSubdirs(pszLcPath, lang);
        if ( paths.Index(lcp) == wxNOT_FOUND )
            paths.Add(lcp);
    }

    // also add the one from where wxWin was installed:
    wxString wxp = wxGetInstallPrefix();
    if ( !wxp.empty() )
    {
        wxp = GetMsgCatalogSubdirs(wxp + _T("/share/locale"), lang);
        if ( paths.Index(wxp) == wxNOT_FOUND )
            paths.Add(wxp);
    }
#endif // __UNIX__


    // finally construct the full search path
    wxString searchPath;
    searchPath.reserve(500);
    count = paths.size();
    for ( n = 0; n < count; n++ )
    {
        searchPath += paths[n];
        if ( n != count - 1 )
            searchPath += wxPATH_SEP;
    }

    return searchPath;
}

// open disk file and read in it's contents
bool wxMsgCatalogFile::Load(const wxChar *szDirPrefix, const wxChar *szName,
                            wxPluralFormsCalculatorPtr& rPluralFormsCalculator)
{
  wxString searchPath;

#if wxUSE_FONTMAP
  // first look for the catalog for this language and the current locale:
  // notice that we don't use the system name for the locale as this would
  // force us to install catalogs in different locations depending on the
  // system but always use the canonical name
  wxFontEncoding encSys = wxLocale::GetSystemEncoding();
  if ( encSys != wxFONTENCODING_SYSTEM )
  {
    wxString fullname(szDirPrefix);
    fullname << _T('.') << wxFontMapperBase::GetEncodingName(encSys);
    searchPath << GetFullSearchPath(fullname) << wxPATH_SEP;
  }
#endif // wxUSE_FONTMAP


  searchPath += GetFullSearchPath(szDirPrefix);
  const wxChar *sublocale = wxStrchr(szDirPrefix, wxT('_'));
  if ( sublocale )
  {
      // also add just base locale name: for things like "fr_BE" (belgium
      // french) we should use "fr" if no belgium specific message catalogs
      // exist
      searchPath << wxPATH_SEP
                 << GetFullSearchPath(wxString(szDirPrefix).
                                      Left((size_t)(sublocale - szDirPrefix)));
  }

  // don't give translation errors here because the wxstd catalog might
  // not yet be loaded (and it's normal)
  //
  // (we're using an object because we have several return paths)

  NoTransErr noTransErr;
  wxLogVerbose(_("looking for catalog '%s' in path '%s'."),
               szName, searchPath.c_str());
  wxLogTrace(TRACE_I18N, _T("Looking for \"%s.mo\" in \"%s\""),
             szName, searchPath.c_str());

  wxFileName fn(szName);
  fn.SetExt(_T("mo"));
  wxString strFullName;
  if ( !wxFindFileInPath(&strFullName, searchPath, fn.GetFullPath()) ) {
    wxLogVerbose(_("catalog file for domain '%s' not found."), szName);
    wxLogTrace(TRACE_I18N, _T("Catalog \"%s.mo\" not found"), szName);
    return false;
  }

  // open file
  wxLogVerbose(_("using catalog '%s' from '%s'."), szName, strFullName.c_str());
  wxLogTrace(TRACE_I18N, _T("Using catalog \"%s\"."), strFullName.c_str());

  wxFile fileMsg(strFullName);
  if ( !fileMsg.IsOpened() )
    return false;

  // get the file size (assume it is less than 4Gb...)
  wxFileOffset lenFile = fileMsg.Length();
  if ( lenFile == wxInvalidOffset )
    return false;

  size_t nSize = wx_truncate_cast(size_t, lenFile);
  wxASSERT_MSG( nSize == lenFile + size_t(0), _T("message catalog bigger than 4GB?") );

  // read the whole file in memory
  m_pData = new size_t8[nSize];
  if ( fileMsg.Read(m_pData, nSize) != lenFile ) {
    wxDELETEA(m_pData);
    return false;
  }

  // examine header
  bool bValid = nSize + (size_t)0 > sizeof(wxMsgCatalogHeader);

  wxMsgCatalogHeader *pHeader = (wxMsgCatalogHeader *)m_pData;
  if ( bValid ) {
    // we'll have to swap all the integers if it's true
    m_bSwapped = pHeader->magic == MSGCATALOG_MAGIC_SW;

    // check the magic number
    bValid = m_bSwapped || pHeader->magic == MSGCATALOG_MAGIC;
  }

  if ( !bValid ) {
    // it's either too short or has incorrect magic number
    wxLogWarning(_("'%s' is not a valid message catalog."), strFullName.c_str());

    wxDELETEA(m_pData);
    return false;
  }

  // initialize
  m_numStrings  = Swap(pHeader->numStrings);
  m_pOrigTable  = (wxMsgTableEntry *)(m_pData +
                   Swap(pHeader->ofsOrigTable));
  m_pTransTable = (wxMsgTableEntry *)(m_pData +
                   Swap(pHeader->ofsTransTable));
  m_nSize = (size_t32)nSize;

  // now parse catalog's header and try to extract catalog charset and
  // plural forms formula from it:

  const char* headerData = StringAtOfs(m_pOrigTable, 0);
  if (headerData && headerData[0] == 0)
  {
      // Extract the charset:
      wxString header = wxString::FromAscii(StringAtOfs(m_pTransTable, 0));
      int begin = header.Find(wxT("Content-Type: text/plain; charset="));
      if (begin != wxNOT_FOUND)
      {
          begin += 34; //strlen("Content-Type: text/plain; charset=")
          size_t end = header.find('\n', begin);
          if (end != size_t(-1))
          {
              m_charset.assign(header, begin, end - begin);
              if (m_charset == wxT("CHARSET"))
              {
                  // "CHARSET" is not valid charset, but lazy translator
                  m_charset.Clear();
              }
          }
      }
      // else: incorrectly filled Content-Type header

      // Extract plural forms:
      begin = header.Find(wxT("Plural-Forms:"));
      if (begin != wxNOT_FOUND)
      {
          begin += 13;
          size_t end = header.find('\n', begin);
          if (end != size_t(-1))
          {
              wxString pfs(header, begin, end - begin);
              wxPluralFormsCalculator* pCalculator = wxPluralFormsCalculator
                  ::make(pfs.ToAscii());
              if (pCalculator != 0)
              {
                  rPluralFormsCalculator.reset(pCalculator);
              }
              else
              {
                   wxLogVerbose(_("Cannot parse Plural-Forms:'%s'"), pfs.c_str());
              }
          }
      }
      if (rPluralFormsCalculator.get() == NULL)
      {
          rPluralFormsCalculator.reset(wxPluralFormsCalculator::make());
      }
  }

  // everything is fine
  return true;
}

void wxMsgCatalogFile::FillHash(wxMessagesHash& hash,
                                const wxString& msgIdCharset,
                                bool convertEncoding) const
{
#if wxUSE_UNICODE
    // this parameter doesn't make sense, we always must convert encoding in
    // Unicode build
    convertEncoding = true;
#elif wxUSE_FONTMAP
    if ( convertEncoding )
    {
        // determine if we need any conversion at all
        wxFontEncoding encCat = wxFontMapperBase::GetEncodingFromName(m_charset);
        if ( encCat == wxLocale::GetSystemEncoding() )
        {
            // no need to convert
            convertEncoding = false;
        }
    }
#endif // wxUSE_UNICODE/wxUSE_FONTMAP

#if wxUSE_WCHAR_T
    // conversion to use to convert catalog strings to the GUI encoding
    wxMBConv *inputConv,
             *inputConvPtr = NULL; // same as inputConv but safely deleteable
    if ( convertEncoding && !m_charset.empty() )
    {
        inputConvPtr =
        inputConv = new wxCSConv(m_charset);
    }
    else // no need or not possible to convert the encoding
    {
#if wxUSE_UNICODE
        // we must somehow convert the narrow strings in the message catalog to
        // wide strings, so use the default conversion if we have no charset
        inputConv = wxConvCurrent;
#else // !wxUSE_UNICODE
        inputConv = NULL;
#endif // wxUSE_UNICODE/!wxUSE_UNICODE
    }

    // conversion to apply to msgid strings before looking them up: we only
    // need it if the msgids are neither in 7 bit ASCII nor in the same
    // encoding as the catalog
    wxCSConv *sourceConv = msgIdCharset.empty() || (msgIdCharset == m_charset)
                            ? NULL
                            : new wxCSConv(msgIdCharset);

#elif wxUSE_FONTMAP
    wxASSERT_MSG( msgIdCharset.empty(),
                  _T("non-ASCII msgid languages only supported if wxUSE_WCHAR_T=1") );

    wxEncodingConverter converter;
    if ( convertEncoding )
    {
        wxFontEncoding targetEnc = wxFONTENCODING_SYSTEM;
        wxFontEncoding enc = wxFontMapperBase::Get()->CharsetToEncoding(m_charset, false);
        if ( enc == wxFONTENCODING_SYSTEM )
        {
            convertEncoding = false; // unknown encoding
        }
        else
        {
            targetEnc = wxLocale::GetSystemEncoding();
            if (targetEnc == wxFONTENCODING_SYSTEM)
            {
                wxFontEncodingArray a = wxEncodingConverter::GetPlatformEquivalents(enc);
                if (a[0] == enc)
                    // no conversion needed, locale uses native encoding
                    convertEncoding = false;
                if (a.GetCount() == 0)
                    // we don't know common equiv. under this platform
                    convertEncoding = false;
                targetEnc = a[0];
            }
        }

        if ( convertEncoding )
        {
            converter.Init(enc, targetEnc);
        }
    }
#endif // wxUSE_WCHAR_T/!wxUSE_WCHAR_T
    (void)convertEncoding; // get rid of warnings about unused parameter

    for (size_t32 i = 0; i < m_numStrings; i++)
    {
        const char *data = StringAtOfs(m_pOrigTable, i);

        wxString msgid;
#if wxUSE_UNICODE
        msgid = wxString(data, *inputConv);
#else // ASCII
        #if wxUSE_WCHAR_T
            if ( inputConv && sourceConv )
                msgid = wxString(inputConv->cMB2WC(data), *sourceConv);
            else
        #endif
                msgid = data;
#endif // wxUSE_UNICODE

        data = StringAtOfs(m_pTransTable, i);
        size_t length = Swap(m_pTransTable[i].nLen);
        size_t offset = 0;
        size_t index = 0;
        while (offset < length)
        {
            const char * const str = data + offset;

            wxString msgstr;
#if wxUSE_UNICODE
            msgstr = wxString(str, *inputConv);
#elif wxUSE_WCHAR_T
            if ( inputConv )
                msgstr = wxString(inputConv->cMB2WC(str), *wxConvUI);
            else
                msgstr = str;
#else // !wxUSE_WCHAR_T
        #if wxUSE_FONTMAP
            if ( convertEncoding )
                msgstr = wxString(converter.Convert(str));
            else
        #endif
                msgstr = str;
#endif // wxUSE_WCHAR_T/!wxUSE_WCHAR_T

            if ( !msgstr.empty() )
            {
                hash[index == 0 ? msgid : msgid + wxChar(index)] = msgstr;
            }

            // skip this string
            offset += strlen(str) + 1;
            ++index;
        }
    }

#if wxUSE_WCHAR_T
    delete sourceConv;
    delete inputConvPtr;
#endif // wxUSE_WCHAR_T
}


// ----------------------------------------------------------------------------
// wxMsgCatalog class
// ----------------------------------------------------------------------------

#if !wxUSE_UNICODE
wxMsgCatalog::~wxMsgCatalog()
{
    if ( m_conv )
    {
        if ( wxConvUI == m_conv )
        {
            // we only change wxConvUI if it points to wxConvLocal so we reset
            // it back to it too
            wxConvUI = &wxConvLocal;
        }

        delete m_conv;
    }
}
#endif // !wxUSE_UNICODE

bool wxMsgCatalog::Load(const wxChar *szDirPrefix, const wxChar *szName,
                        const wxChar *msgIdCharset, bool bConvertEncoding)
{
    wxMsgCatalogFile file;

    m_name = szName;

    if ( !file.Load(szDirPrefix, szName, m_pluralFormsCalculator) )
        return false;

    file.FillHash(m_messages, msgIdCharset, bConvertEncoding);

#if !wxUSE_UNICODE && wxUSE_WCHAR_T
    // we should use a conversion compatible with the message catalog encoding
    // in the GUI if we don't convert the strings to the current conversion but
    // as the encoding is global, only change it once, otherwise we could get
    // into trouble if we use several message catalogs with different encodings
    //
    // this is, of course, a hack but it at least allows the program to use
    // message catalogs in any encodings without asking the user to change his
    // locale
    if ( !bConvertEncoding &&
            !file.GetCharset().empty() &&
                wxConvUI == &wxConvLocal )
    {
        wxConvUI =
        m_conv = new wxCSConv(file.GetCharset());
    }
#endif // !wxUSE_UNICODE && wxUSE_WCHAR_T

    return true;
}

const wxChar *wxMsgCatalog::GetString(const wxChar *sz, size_t n) const
{
    int index = 0;
    if (n != size_t(-1))
    {
        index = m_pluralFormsCalculator->evaluate(n);
    }
    wxMessagesHash::const_iterator i;
    if (index != 0)
    {
        i = m_messages.find(wxString(sz) + wxChar(index));   // plural
    }
    else
    {
        i = m_messages.find(sz);
    }

    if ( i != m_messages.end() )
    {
        return i->second.c_str();
    }
    else
        return NULL;
}

// ----------------------------------------------------------------------------
// wxLocale
// ----------------------------------------------------------------------------

#include "wx/arrimpl.cpp"
WX_DECLARE_EXPORTED_OBJARRAY(wxLanguageInfo, wxLanguageInfoArray);
WX_DEFINE_OBJARRAY(wxLanguageInfoArray)

wxLanguageInfoArray *wxLocale::ms_languagesDB = NULL;

/*static*/ void wxLocale::CreateLanguagesDB()
{
    if (ms_languagesDB == NULL)
    {
        ms_languagesDB = new wxLanguageInfoArray;
        InitLanguagesDB();
    }
}

/*static*/ void wxLocale::DestroyLanguagesDB()
{
    delete ms_languagesDB;
    ms_languagesDB = NULL;
}


void wxLocale::DoCommonInit()
{
  m_pszOldLocale = NULL;

  m_pOldLocale = wxSetLocale(this);

  m_pMsgCat = NULL;
  m_language = wxLANGUAGE_UNKNOWN;
  m_initialized = false;
}

// NB: this function has (desired) side effect of changing current locale
bool wxLocale::Init(const wxChar *szName,
                    const wxChar *szShort,
                    const wxChar *szLocale,
                    bool        bLoadDefault,
                    bool        bConvertEncoding)
{
  wxASSERT_MSG( !m_initialized,
                _T("you can't call wxLocale::Init more than once") );

  m_initialized = true;
  m_strLocale = szName;
  m_strShort = szShort;
  m_bConvertEncoding = bConvertEncoding;
  m_language = wxLANGUAGE_UNKNOWN;

  // change current locale (default: same as long name)
  if ( szLocale == NULL )
  {
    // the argument to setlocale()
    szLocale = szShort;

    wxCHECK_MSG( szLocale, false, _T("no locale to set in wxLocale::Init()") );
  }

#ifdef __WXWINCE__
  // FIXME: I'm guessing here
  wxChar localeName[256];
  int ret = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLANGUAGE, localeName,
      256);
  if (ret != 0)
  {
    m_pszOldLocale = wxStrdup(localeName);
  }
  else
    m_pszOldLocale = NULL;

  // TODO: how to find languageId
  // SetLocaleInfo(languageId, SORT_DEFAULT, localeName);
#else
  wxMB2WXbuf oldLocale = wxSetlocale(LC_ALL, szLocale);
  if ( oldLocale )
      m_pszOldLocale = wxStrdup(oldLocale);
  else
      m_pszOldLocale = NULL;
#endif

  if ( m_pszOldLocale == NULL )
    wxLogError(_("locale '%s' can not be set."), szLocale);

  // the short name will be used to look for catalog files as well,
  // so we need something here
  if ( m_strShort.empty() ) {
    // FIXME I don't know how these 2 letter abbreviations are formed,
    //       this wild guess is surely wrong
    if ( szLocale && szLocale[0] )
    {
        m_strShort += (wxChar)wxTolower(szLocale[0]);
        if ( szLocale[1] )
            m_strShort += (wxChar)wxTolower(szLocale[1]);
    }
  }

  // load the default catalog with wxWidgets standard messages
  m_pMsgCat = NULL;
  bool bOk = true;
  if ( bLoadDefault )
  {
    bOk = AddCatalog(wxT("wxstd"));

    // there may be a catalog with toolkit specific overrides, it is not
    // an error if this does not exist
    if ( bOk )
    {
      wxString port(wxPlatformInfo::Get().GetPortIdName());
      if ( !port.empty() )
      {
        AddCatalog(port.BeforeFirst(wxT('/')).MakeLower());
      }
    }
  }

  return bOk;
}


#if defined(__UNIX__) && wxUSE_UNICODE && !defined(__WXMAC__)
static wxWCharBuffer wxSetlocaleTryUTF(int c, const wxChar *lc)
{
    wxMB2WXbuf l = wxSetlocale(c, lc);
    if ( !l && lc && lc[0] != 0 )
    {
        wxString buf(lc);
        wxString buf2;
        buf2 = buf + wxT(".UTF-8");
        l = wxSetlocale(c, buf2.c_str());
        if ( !l )
        {
            buf2 = buf + wxT(".utf-8");
            l = wxSetlocale(c, buf2.c_str());
        }
        if ( !l )
        {
            buf2 = buf + wxT(".UTF8");
            l = wxSetlocale(c, buf2.c_str());
        }
        if ( !l )
        {
            buf2 = buf + wxT(".utf8");
            l = wxSetlocale(c, buf2.c_str());
        }
    }
    return l;
}
#else
#define wxSetlocaleTryUTF(c, lc)  wxSetlocale(c, lc)
#endif

bool wxLocale::Init(int language, int flags)
{
    int lang = language;
    if (lang == wxLANGUAGE_DEFAULT)
    {
        // auto detect the language
        lang = GetSystemLanguage();
    }

    // We failed to detect system language, so we will use English:
    if (lang == wxLANGUAGE_UNKNOWN)
    {
       return false;
    }

    const wxLanguageInfo *info = GetLanguageInfo(lang);

    // Unknown language:
    if (info == NULL)
    {
        wxLogError(wxT("Unknown language %i."), lang);
        return false;
    }

    wxString name = info->Description;
    wxString canonical = info->CanonicalName;
    wxString locale;

    // Set the locale:
#if defined(__OS2__)
    wxMB2WXbuf retloc = wxSetlocale(LC_ALL , wxEmptyString);
#elif defined(__UNIX__) && !defined(__WXMAC__)
    if (language != wxLANGUAGE_DEFAULT)
        locale = info->CanonicalName;

    wxMB2WXbuf retloc = wxSetlocaleTryUTF(LC_ALL, locale);

    const wxString langOnly = locale.Left(2);
    if ( !retloc )
    {
        // Some C libraries don't like xx_YY form and require xx only
        retloc = wxSetlocaleTryUTF(LC_ALL, langOnly);
    }

#if wxUSE_FONTMAP
    // some systems (e.g. FreeBSD and HP-UX) don't have xx_YY aliases but
    // require the full xx_YY.encoding form, so try using UTF-8 because this is
    // the only thing we can do generically
    //
    // TODO: add encodings applicable to each language to the lang DB and try
    //       them all in turn here
    if ( !retloc )
    {
        const wxChar **names =
            wxFontMapperBase::GetAllEncodingNames(wxFONTENCODING_UTF8);
        while ( *names )
        {
            retloc = wxSetlocale(LC_ALL, locale + _T('.') + *names++);
            if ( retloc )
                break;
        }
    }
#endif // wxUSE_FONTMAP

    if ( !retloc )
    {
        // Some C libraries (namely glibc) still use old ISO 639,
        // so will translate the abbrev for them
        wxString localeAlt;
        if ( langOnly == wxT("he") )
            localeAlt = wxT("iw") + locale.Mid(3);
        else if ( langOnly == wxT("id") )
            localeAlt = wxT("in") + locale.Mid(3);
        else if ( langOnly == wxT("yi") )
            localeAlt = wxT("ji") + locale.Mid(3);
        else if ( langOnly == wxT("nb") )
            localeAlt = wxT("no_NO");
        else if ( langOnly == wxT("nn") )
            localeAlt = wxT("no_NY");

        if ( !localeAlt.empty() )
        {
            retloc = wxSetlocaleTryUTF(LC_ALL, localeAlt);
            if ( !retloc )
                retloc = wxSetlocaleTryUTF(LC_ALL, localeAlt.Left(2));
        }
    }

    if ( !retloc )
    {
        wxLogError(wxT("Cannot set locale to '%s'."), locale.c_str());
        return false;
    }

#ifdef __AIX__
    // at least in AIX 5.2 libc is buggy and the string returned from setlocale(LC_ALL)
    // can't be passed back to it because it returns 6 strings (one for each locale
    // category), i.e. for C locale we get back "C C C C C C"
    //
    // this contradicts IBM own docs but this is not of much help, so just work around
    // it in the crudest possible manner
    wxChar *p = wxStrchr((wxChar *)retloc, _T(' '));
    if ( p )
        *p = _T('\0');
#endif // __AIX__

#elif defined(__WIN32__)

    #if wxUSE_UNICODE && (defined(__VISUALC__) || defined(__MINGW32__))
        // NB: setlocale() from msvcrt.dll (used by VC++ and Mingw)
        //     can't set locale to language that can only be written using
        //     Unicode.  Therefore wxSetlocale call failed, but we don't want
        //     to report it as an error -- so that at least message catalogs
        //     can be used. Watch for code marked with
        //     #ifdef SETLOCALE_FAILS_ON_UNICODE_LANGS bellow.
        #define SETLOCALE_FAILS_ON_UNICODE_LANGS
    #endif

#if !wxUSE_UNICODE
    const
#endif
    wxMB2WXbuf retloc = wxT("C");
    if (language != wxLANGUAGE_DEFAULT)
    {
        if (info->WinLang == 0)
        {
            wxLogWarning(wxT("Locale '%s' not supported by OS."), name.c_str());
            // retloc already set to "C"
        }
        else
        {
            int codepage
                         #ifdef SETLOCALE_FAILS_ON_UNICODE_LANGS
                         = -1
                         #endif
                         ;
            wxUint32 lcid = MAKELCID(MAKELANGID(info->WinLang, info->WinSublang),
                                     SORT_DEFAULT);
            // FIXME
#ifndef __WXWINCE__
            SetThreadLocale(lcid);
#endif
            // NB: we must translate LCID to CRT's setlocale string ourselves,
            //     because SetThreadLocale does not modify change the
            //     interpretation of setlocale(LC_ALL, "") call:
            wxChar buffer[256];
            buffer[0] = wxT('\0');
            GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, buffer, 256);
            locale << buffer;
            if (GetLocaleInfo(lcid, LOCALE_SENGCOUNTRY, buffer, 256) > 0)
                locale << wxT("_") << buffer;
            if (GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, buffer, 256) > 0)
            {
                codepage = wxAtoi(buffer);
                if (codepage != 0)
                    locale << wxT(".") << buffer;
            }
            if (locale.empty())
            {
                wxLogLastError(wxT("SetThreadLocale"));
                wxLogError(wxT("Cannot set locale to language %s."), name.c_str());
                return false;
            }
            else
            {
            // FIXME
#ifndef __WXWINCE__
                retloc = wxSetlocale(LC_ALL, locale);
#endif
#ifdef SETLOCALE_FAILS_ON_UNICODE_LANGS
                if (codepage == 0 && (const wxChar*)retloc == NULL)
                {
                    retloc = wxT("C");
                }
#endif
            }
        }
    }
    else
    {
            // FIXME
#ifndef __WXWINCE__
        retloc = wxSetlocale(LC_ALL, wxEmptyString);
#else
        retloc = NULL;
#endif
#ifdef SETLOCALE_FAILS_ON_UNICODE_LANGS
        if ((const wxChar*)retloc == NULL)
        {
            wxChar buffer[16];
            if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                              LOCALE_IDEFAULTANSICODEPAGE, buffer, 16) > 0 &&
                 wxStrcmp(buffer, wxT("0")) == 0)
            {
                retloc = wxT("C");
            }
        }
#endif
    }

    if ( !retloc )
    {
        wxLogError(wxT("Cannot set locale to language %s."), name.c_str());
        return false;
    }
#elif defined(__WXMAC__)
    if (lang == wxLANGUAGE_DEFAULT)
        locale = wxEmptyString;
    else
        locale = info->CanonicalName;

    wxMB2WXbuf retloc = wxSetlocale(LC_ALL, locale);

    if ( !retloc )
    {
        // Some C libraries don't like xx_YY form and require xx only
        retloc = wxSetlocale(LC_ALL, locale.Mid(0,2));
    }
    if ( !retloc )
    {
        wxLogError(wxT("Cannot set locale to '%s'."), locale.c_str());
        return false;
    }
#else
    wxUnusedVar(flags);
    return false;
    #define WX_NO_LOCALE_SUPPORT
#endif

#ifndef WX_NO_LOCALE_SUPPORT
    wxChar *szLocale = retloc ? wxStrdup(retloc) : NULL;
    bool ret = Init(name, canonical, szLocale,
                    (flags & wxLOCALE_LOAD_DEFAULT) != 0,
                    (flags & wxLOCALE_CONV_ENCODING) != 0);
    free(szLocale);

    if (IsOk()) // setlocale() succeeded
        m_language = lang;

    return ret;
#endif // !WX_NO_LOCALE_SUPPORT
}



void wxLocale::AddCatalogLookupPathPrefix(const wxString& prefix)
{
    if ( gs_searchPrefixes.Index(prefix) == wxNOT_FOUND )
    {
        gs_searchPrefixes.Add(prefix);
    }
    //else: already have it
}

/*static*/ int wxLocale::GetSystemLanguage()
{
    CreateLanguagesDB();

    // init i to avoid compiler warning
    size_t i = 0,
           count = ms_languagesDB->GetCount();

#if defined(__UNIX__) && !defined(__WXMAC__)
    // first get the string identifying the language from the environment
    wxString langFull;
    if (!wxGetEnv(wxT("LC_ALL"), &langFull) &&
        !wxGetEnv(wxT("LC_MESSAGES"), &langFull) &&
        !wxGetEnv(wxT("LANG"), &langFull))
    {
        // no language specified, treat it as English
        return wxLANGUAGE_ENGLISH_US;
    }

    if ( langFull == _T("C") || langFull == _T("POSIX") )
    {
        // default C locale is English too
        return wxLANGUAGE_ENGLISH_US;
    }

    // the language string has the following form
    //
    //      lang[_LANG][.encoding][@modifier]
    //
    // (see environ(5) in the Open Unix specification)
    //
    // where lang is the primary language, LANG is a sublang/territory,
    // encoding is the charset to use and modifier "allows the user to select
    // a specific instance of localization data within a single category"
    //
    // for example, the following strings are valid:
    //      fr
    //      fr_FR
    //      de_DE.iso88591
    //      de_DE@euro
    //      de_DE.iso88591@euro

    // for now we don't use the encoding, although we probably should (doing
    // translations of the msg catalogs on the fly as required) (TODO)
    //
    // we need the modified for languages like Valencian: ca_ES@valencia
    // though, remember it
    wxString modifier;
    size_t posModifier = langFull.find_first_of(_T("@"));
    if ( posModifier != wxString::npos )
        modifier = langFull.Mid(posModifier);

    size_t posEndLang = langFull.find_first_of(_T("@."));
    if ( posEndLang != wxString::npos )
    {
        langFull.Truncate(posEndLang);
    }

    // in addition to the format above, we also can have full language names
    // in LANG env var - for example, SuSE is known to use LANG="german" - so
    // check for this

    // do we have just the language (or sublang too)?
    bool justLang = langFull.length() == LEN_LANG;
    if ( justLang ||
         (langFull.length() == LEN_FULL && langFull[LEN_LANG] == wxT('_')) )
    {
        // 0. Make sure the lang is according to latest ISO 639
        //    (this is necessary because glibc uses iw and in instead
        //    of he and id respectively).

        // the language itself (second part is the dialect/sublang)
        wxString langOrig = ExtractLang(langFull);

        wxString lang;
        if ( langOrig == wxT("iw"))
            lang = _T("he");
        else if (langOrig == wxT("in"))
            lang = wxT("id");
        else if (langOrig == wxT("ji"))
            lang = wxT("yi");
        else if (langOrig == wxT("no_NO"))
            lang = wxT("nb_NO");
        else if (langOrig == wxT("no_NY"))
            lang = wxT("nn_NO");
        else if (langOrig == wxT("no"))
            lang = wxT("nb_NO");
        else
            lang = langOrig;

        // did we change it?
        if ( lang != langOrig )
        {
            langFull = lang + ExtractNotLang(langFull);
        }

        // 1. Try to find the language either as is:
        // a) With modifier if set
        if ( !modifier.empty() )
        {
            wxString langFullWithModifier = langFull + modifier;
            for ( i = 0; i < count; i++ )
            {
                if ( ms_languagesDB->Item(i).CanonicalName == langFullWithModifier )
                    break;
            }
        }

        // b) Without modifier
        if ( modifier.empty() || i == count )
        {
            for ( i = 0; i < count; i++ )
            {
                if ( ms_languagesDB->Item(i).CanonicalName == langFull )
                    break;
            }
        }

        // 2. If langFull is of the form xx_YY, try to find xx:
        if ( i == count && !justLang )
        {
            for ( i = 0; i < count; i++ )
            {
                if ( ms_languagesDB->Item(i).CanonicalName == lang )
                {
                    break;
                }
            }
        }

        // 3. If langFull is of the form xx, try to find any xx_YY record:
        if ( i == count && justLang )
        {
            for ( i = 0; i < count; i++ )
            {
                if ( ExtractLang(ms_languagesDB->Item(i).CanonicalName)
                        == langFull )
                {
                    break;
                }
            }
        }
    }
    else // not standard format
    {
        // try to find the name in verbose description
        for ( i = 0; i < count; i++ )
        {
            if (ms_languagesDB->Item(i).Description.CmpNoCase(langFull) == 0)
            {
                break;
            }
        }
    }
#elif defined(__WXMAC__)
    const wxChar * lc = NULL ;
    long lang = GetScriptVariable( smSystemScript, smScriptLang) ;
    switch( GetScriptManagerVariable( smRegionCode ) ) {
      case verUS :
        lc = wxT("en_US") ;
        break ;
      case verFrance :
        lc = wxT("fr_FR") ;
        break ;
      case verBritain :
        lc = wxT("en_GB") ;
        break ;
      case verGermany :
        lc = wxT("de_DE") ;
        break ;
      case verItaly :
        lc = wxT("it_IT") ;
        break ;
      case verNetherlands :
        lc = wxT("nl_NL") ;
        break ;
      case verFlemish :
        lc = wxT("nl_BE") ;
        break ;
      case verSweden :
        lc = wxT("sv_SE" );
        break ;
      case verSpain :
        lc = wxT("es_ES" );
        break ;
      case verDenmark :
        lc = wxT("da_DK") ;
        break ;
      case verPortugal :
        lc = wxT("pt_PT") ;
        break ;
      case verFrCanada:
        lc = wxT("fr_CA") ;
        break ;
      case verNorway:
        lc = wxT("nb_NO") ;
        break ;
      case verIsrael:
        lc = wxT("iw_IL") ;
        break ;
      case verJapan:
        lc = wxT("ja_JP") ;
        break ;
      case verAustralia:
        lc = wxT("en_AU") ;
        break ;
      case verArabic:
        lc = wxT("ar") ;
        break ;
      case verFinland:
        lc = wxT("fi_FI") ;
        break ;
      case verFrSwiss:
        lc = wxT("fr_CH") ;
        break ;
      case verGrSwiss:
        lc = wxT("de_CH") ;
        break ;
      case verGreece:
        lc = wxT("el_GR") ;
        break ;
      case verIceland:
        lc = wxT("is_IS") ;
        break ;
      case verMalta:
        lc = wxT("mt_MT") ;
        break ;
      case verCyprus:
      // _CY is not part of wx, so we have to translate according to the system language
        if ( lang == langGreek ) {
          lc = wxT("el_GR") ;
        }
        else if ( lang == langTurkish ) {
          lc = wxT("tr_TR") ;
        }
        break ;
      case verTurkey:
        lc = wxT("tr_TR") ;
        break ;
      case verYugoCroatian:
        lc = wxT("hr_HR") ;
        break ;
      case verIndiaHindi:
        lc = wxT("hi_IN") ;
        break ;
      case verPakistanUrdu:
        lc = wxT("ur_PK") ;
        break ;
      case verTurkishModified:
        lc = wxT("tr_TR") ;
        break ;
      case verItalianSwiss:
        lc = wxT("it_CH") ;
        break ;
      case verInternational:
        lc = wxT("en") ;
        break ;
      case verRomania:
        lc = wxT("ro_RO") ;
        break ;
      case verGreecePoly:
        lc = wxT("el_GR") ;
        break ;
      case verLithuania:
        lc = wxT("lt_LT") ;
        break ;
      case verPoland:
        lc = wxT("pl_PL") ;
        break ;
      case verMagyar :
      case verHungary:
        lc = wxT("hu_HU") ;
        break ;
      case verEstonia:
        lc = wxT("et_EE") ;
        break ;
      case verLatvia:
        lc = wxT("lv_LV") ;
        break ;
      case verSami:
        lc = wxT("se_NO") ;
        break ;
      case verFaroeIsl:
        lc = wxT("fo_FO") ;
        break ;
      case verIran:
        lc = wxT("fa_IR") ;
        break ;
      case verRussia:
        lc = wxT("ru_RU") ;
        break ;
       case verIreland:
        lc = wxT("ga_IE") ;
        break ;
      case verKorea:
        lc = wxT("ko_KR") ;
        break ;
      case verChina:
        lc = wxT("zh_CN") ;
        break ;
      case verTaiwan:
        lc = wxT("zh_TW") ;
        break ;
      case verThailand:
        lc = wxT("th_TH") ;
        break ;
      case verCzech:
        lc = wxT("cs_CZ") ;
        break ;
      case verSlovak:
        lc = wxT("sk_SK") ;
        break ;
      case verBengali:
        lc = wxT("bn") ;
        break ;
      case verByeloRussian:
        lc = wxT("be_BY") ;
        break ;
      case verUkraine:
        lc = wxT("uk_UA") ;
        break ;
      case verGreeceAlt:
        lc = wxT("el_GR") ;
        break ;
      case verSerbian:
        lc = wxT("sr_YU") ;
        break ;
      case verSlovenian:
        lc = wxT("sl_SI") ;
        break ;
      case verMacedonian:
        lc = wxT("mk_MK") ;
        break ;
      case verCroatia:
        lc = wxT("hr_HR") ;
        break ;
      case verBrazil:
        lc = wxT("pt_BR ") ;
        break ;
      case verBulgaria:
        lc = wxT("bg_BG") ;
        break ;
      case verCatalonia:
        lc = wxT("ca_ES") ;
        break ;
      case verScottishGaelic:
        lc = wxT("gd") ;
        break ;
      case verManxGaelic:
        lc = wxT("gv") ;
        break ;
      case verBreton:
        lc = wxT("br") ;
        break ;
      case verNunavut:
        lc = wxT("iu_CA") ;
        break ;
      case verWelsh:
        lc = wxT("cy") ;
        break ;
      case verIrishGaelicScript:
        lc = wxT("ga_IE") ;
        break ;
      case verEngCanada:
        lc = wxT("en_CA") ;
        break ;
      case verBhutan:
        lc = wxT("dz_BT") ;
        break ;
      case verArmenian:
        lc = wxT("hy_AM") ;
        break ;
      case verGeorgian:
        lc = wxT("ka_GE") ;
        break ;
      case verSpLatinAmerica:
        lc = wxT("es_AR") ;
        break ;
      case verTonga:
        lc = wxT("to_TO" );
        break ;
      case verFrenchUniversal:
        lc = wxT("fr_FR") ;
        break ;
      case verAustria:
        lc = wxT("de_AT") ;
        break ;
      case verGujarati:
        lc = wxT("gu_IN") ;
        break ;
      case verPunjabi:
        lc = wxT("pa") ;
        break ;
      case verIndiaUrdu:
        lc = wxT("ur_IN") ;
        break ;
      case verVietnam:
        lc = wxT("vi_VN") ;
        break ;
      case verFrBelgium:
        lc = wxT("fr_BE") ;
        break ;
      case verUzbek:
        lc = wxT("uz_UZ") ;
        break ;
      case verSingapore:
        lc = wxT("zh_SG") ;
        break ;
      case verNynorsk:
        lc = wxT("nn_NO") ;
        break ;
      case verAfrikaans:
        lc = wxT("af_ZA") ;
        break ;
      case verEsperanto:
        lc = wxT("eo") ;
        break ;
      case verMarathi:
        lc = wxT("mr_IN") ;
        break ;
      case verTibetan:
        lc = wxT("bo") ;
        break ;
      case verNepal:
        lc = wxT("ne_NP") ;
        break ;
      case verGreenland:
        lc = wxT("kl_GL") ;
        break ;
      default :
        break ;
    }
    if ( !lc )
        return wxLANGUAGE_UNKNOWN;
    for ( i = 0; i < count; i++ )
    {
        if ( ms_languagesDB->Item(i).CanonicalName == lc )
        {
            break;
        }
    }

#elif defined(__WIN32__)
    LCID lcid = GetUserDefaultLCID();
    if ( lcid != 0 )
    {
        wxUint32 lang = PRIMARYLANGID(LANGIDFROMLCID(lcid));
        wxUint32 sublang = SUBLANGID(LANGIDFROMLCID(lcid));

        for ( i = 0; i < count; i++ )
        {
            if (ms_languagesDB->Item(i).WinLang == lang &&
                ms_languagesDB->Item(i).WinSublang == sublang)
            {
                break;
            }
        }
    }
    //else: leave wxlang == wxLANGUAGE_UNKNOWN
#endif // Unix/Win32

    if ( i < count )
    {
        // we did find a matching entry, use it
        return ms_languagesDB->Item(i).Language;
    }

    // no info about this language in the database
    return wxLANGUAGE_UNKNOWN;
}

// ----------------------------------------------------------------------------
// encoding stuff
// ----------------------------------------------------------------------------

// this is a bit strange as under Windows we get the encoding name using its
// numeric value and under Unix we do it the other way round, but this just
// reflects the way different systems provide the encoding info

/* static */
wxString wxLocale::GetSystemEncodingName()
{
    wxString encname;

#if defined(__WIN32__) && !defined(__WXMICROWIN__)
    // FIXME: what is the error return value for GetACP()?
    UINT codepage = ::GetACP();
    encname.Printf(_T("windows-%u"), codepage);
#elif defined(__WXMAC__)
    // default is just empty string, this resolves to the default system
    // encoding later
#elif defined(__UNIX_LIKE__)

#if defined(HAVE_LANGINFO_H) && defined(CODESET)
    // GNU libc provides current character set this way (this conforms
    // to Unix98)
    char *oldLocale = strdup(setlocale(LC_CTYPE, NULL));
    setlocale(LC_CTYPE, "");
    const char *alang = nl_langinfo(CODESET);
    setlocale(LC_CTYPE, oldLocale);
    free(oldLocale);

    if ( alang )
    {
        encname = wxString::FromAscii( alang );
    }
    else // nl_langinfo() failed
#endif // HAVE_LANGINFO_H
    {
        // if we can't get at the character set directly, try to see if it's in
        // the environment variables (in most cases this won't work, but I was
        // out of ideas)
        char *lang = getenv( "LC_ALL");
        char *dot = lang ? strchr(lang, '.') : (char *)NULL;
        if (!dot)
        {
            lang = getenv( "LC_CTYPE" );
            if ( lang )
                dot = strchr(lang, '.' );
        }
        if (!dot)
        {
            lang = getenv( "LANG");
            if ( lang )
                dot = strchr(lang, '.');
        }

        if ( dot )
        {
            encname = wxString::FromAscii( dot+1 );
        }
    }
#endif // Win32/Unix

    return encname;
}

/* static */
wxFontEncoding wxLocale::GetSystemEncoding()
{
#if defined(__WIN32__) && !defined(__WXMICROWIN__)
    UINT codepage = ::GetACP();

    // wxWidgets only knows about CP1250-1257, 874, 932, 936, 949, 950
    if ( codepage >= 1250 && codepage <= 1257 )
    {
        return (wxFontEncoding)(wxFONTENCODING_CP1250 + codepage - 1250);
    }

    if ( codepage == 874 )
    {
        return wxFONTENCODING_CP874;
    }

    if ( codepage == 932 )
    {
        return wxFONTENCODING_CP932;
    }

    if ( codepage == 936 )
    {
        return wxFONTENCODING_CP936;
    }

    if ( codepage == 949 )
    {
        return wxFONTENCODING_CP949;
    }

    if ( codepage == 950 )
    {
        return wxFONTENCODING_CP950;
    }
#elif defined(__WXMAC__)
    TextEncoding encoding = 0 ;
#if TARGET_CARBON
    encoding = CFStringGetSystemEncoding() ;
#else
    UpgradeScriptInfoToTextEncoding ( smSystemScript , kTextLanguageDontCare , kTextRegionDontCare , NULL , &encoding ) ;
#endif
    return wxMacGetFontEncFromSystemEnc( encoding ) ;
#elif defined(__UNIX_LIKE__) && wxUSE_FONTMAP
    const wxString encname = GetSystemEncodingName();
    if ( !encname.empty() )
    {
        wxFontEncoding enc = wxFontMapperBase::GetEncodingFromName(encname);

        // on some modern Linux systems (RedHat 8) the default system locale
        // is UTF8 -- but it isn't supported by wxGTK1 in ANSI build at all so
        // don't even try to use it in this case
#if !wxUSE_UNICODE && \
        ((defined(__WXGTK__) && !defined(__WXGTK20__)) || defined(__WXMOTIF__))
        if ( enc == wxFONTENCODING_UTF8 )
        {
            // the most similar supported encoding...
            enc = wxFONTENCODING_ISO8859_1;
        }
#endif // !wxUSE_UNICODE

        // GetEncodingFromName() returns wxFONTENCODING_DEFAULT for C locale
        // (a.k.a. US-ASCII) which is arguably a bug but keep it like this for
        // backwards compatibility and just take care to not return
        // wxFONTENCODING_DEFAULT from here as this surely doesn't make sense
        if ( enc == wxFONTENCODING_DEFAULT )
        {
            // we don't have wxFONTENCODING_ASCII, so use the closest one
            return wxFONTENCODING_ISO8859_1;
        }

        if ( enc != wxFONTENCODING_MAX )
        {
            return enc;
        }
        //else: return wxFONTENCODING_SYSTEM below
    }
#endif // Win32/Unix

    return wxFONTENCODING_SYSTEM;
}

/* static */
void wxLocale::AddLanguage(const wxLanguageInfo& info)
{
    CreateLanguagesDB();
    ms_languagesDB->Add(info);
}

/* static */
const wxLanguageInfo *wxLocale::GetLanguageInfo(int lang)
{
    CreateLanguagesDB();

    // calling GetLanguageInfo(wxLANGUAGE_DEFAULT) is a natural thing to do, so
    // make it work
    if ( lang == wxLANGUAGE_DEFAULT )
        lang = GetSystemLanguage();

    const size_t count = ms_languagesDB->GetCount();
    for ( size_t i = 0; i < count; i++ )
    {
        if ( ms_languagesDB->Item(i).Language == lang )
        {
            // We need to create a temporary here in order to make this work with BCC in final build mode
            wxLanguageInfo *ptr = &ms_languagesDB->Item(i);
            return ptr;
        }
    }

    return NULL;
}

/* static */
wxString wxLocale::GetLanguageName(int lang)
{
    const wxLanguageInfo *info = GetLanguageInfo(lang);
    if ( !info )
        return wxEmptyString;
    else
        return info->Description;
}

/* static */
const wxLanguageInfo *wxLocale::FindLanguageInfo(const wxString& locale)
{
    CreateLanguagesDB();

    const wxLanguageInfo *infoRet = NULL;

    const size_t count = ms_languagesDB->GetCount();
    for ( size_t i = 0; i < count; i++ )
    {
        const wxLanguageInfo *info = &ms_languagesDB->Item(i);

        if ( wxStricmp(locale, info->CanonicalName) == 0 ||
                wxStricmp(locale, info->Description) == 0 )
        {
            // exact match, stop searching
            infoRet = info;
            break;
        }

        if ( wxStricmp(locale, info->CanonicalName.BeforeFirst(_T('_'))) == 0 )
        {
            // a match -- but maybe we'll find an exact one later, so continue
            // looking
            //
            // OTOH, maybe we had already found a language match and in this
            // case don't overwrite it becauce the entry for the default
            // country always appears first in ms_languagesDB
            if ( !infoRet )
                infoRet = info;
        }
    }

    return infoRet;
}

wxString wxLocale::GetSysName() const
{
            // FIXME
#ifndef __WXWINCE__
    return wxSetlocale(LC_ALL, NULL);
#else
    return wxEmptyString;
#endif
}

// clean up
wxLocale::~wxLocale()
{
    // free memory
    wxMsgCatalog *pTmpCat;
    while ( m_pMsgCat != NULL ) {
        pTmpCat = m_pMsgCat;
        m_pMsgCat = m_pMsgCat->m_pNext;
        delete pTmpCat;
    }

    // restore old locale pointer
    wxSetLocale(m_pOldLocale);

    // FIXME
#ifndef __WXWINCE__
    wxSetlocale(LC_ALL, m_pszOldLocale);
#endif
    free((wxChar *)m_pszOldLocale);     // const_cast
}

// get the translation of given string in current locale
const wxChar *wxLocale::GetString(const wxChar *szOrigString,
                                  const wxChar *szDomain) const
{
    return GetString(szOrigString, szOrigString, size_t(-1), szDomain);
}

const wxChar *wxLocale::GetString(const wxChar *szOrigString,
                                  const wxChar *szOrigString2,
                                  size_t n,
                                  const wxChar *szDomain) const
{
    if ( wxIsEmpty(szOrigString) )
        return wxEmptyString;

    const wxChar *pszTrans = NULL;
    wxMsgCatalog *pMsgCat;

    if ( szDomain != NULL && szDomain[0] )
    {
        pMsgCat = FindCatalog(szDomain);

        // does the catalog exist?
        if ( pMsgCat != NULL )
            pszTrans = pMsgCat->GetString(szOrigString, n);
    }
    else
    {
        // search in all domains
        for ( pMsgCat = m_pMsgCat; pMsgCat != NULL; pMsgCat = pMsgCat->m_pNext )
        {
            pszTrans = pMsgCat->GetString(szOrigString, n);
            if ( pszTrans != NULL )   // take the first found
                break;
        }
    }

    if ( pszTrans == NULL )
    {
#ifdef __WXDEBUG__
        if ( !NoTransErr::Suppress() )
        {
            NoTransErr noTransErr;

            wxLogTrace(TRACE_I18N,
                       _T("string \"%s\"[%ld] not found in %slocale '%s'."),
                       szOrigString, (long)n,
                       szDomain ? wxString::Format(_T("domain '%s' "), szDomain).c_str()
                                : _T(""),
                       m_strLocale.c_str());
        }
#endif // __WXDEBUG__

        if (n == size_t(-1))
            return szOrigString;
        else
            return n == 1 ? szOrigString : szOrigString2;
    }

    return pszTrans;
}

wxString wxLocale::GetHeaderValue( const wxChar* szHeader,
                                   const wxChar* szDomain ) const
{
    if ( wxIsEmpty(szHeader) )
        return wxEmptyString;

    wxChar const * pszTrans = NULL;
    wxMsgCatalog *pMsgCat;

    if ( szDomain != NULL )
    {
        pMsgCat = FindCatalog(szDomain);

        // does the catalog exist?
        if ( pMsgCat == NULL )
            return wxEmptyString;

        pszTrans = pMsgCat->GetString(wxEmptyString, (size_t)-1);
    }
    else
    {
        // search in all domains
        for ( pMsgCat = m_pMsgCat; pMsgCat != NULL; pMsgCat = pMsgCat->m_pNext )
        {
            pszTrans = pMsgCat->GetString(wxEmptyString, (size_t)-1);
            if ( pszTrans != NULL )   // take the first found
                break;
        }
    }

    if ( wxIsEmpty(pszTrans) )
      return wxEmptyString;

    wxChar const * pszFound = wxStrstr(pszTrans, szHeader);
    if ( pszFound == NULL )
      return wxEmptyString;

    pszFound += wxStrlen(szHeader) + 2 /* ': ' */;

    // Every header is separated by \n

    wxChar const * pszEndLine = wxStrchr(pszFound, wxT('\n'));
    if ( pszEndLine == NULL ) pszEndLine = pszFound + wxStrlen(pszFound);


    // wxString( wxChar*, length);
    wxString retVal( pszFound, pszEndLine - pszFound );

    return retVal;
}


// find catalog by name in a linked list, return NULL if !found
wxMsgCatalog *wxLocale::FindCatalog(const wxChar *szDomain) const
{
    // linear search in the linked list
    wxMsgCatalog *pMsgCat;
    for ( pMsgCat = m_pMsgCat; pMsgCat != NULL; pMsgCat = pMsgCat->m_pNext )
    {
        if ( wxStricmp(pMsgCat->GetName(), szDomain) == 0 )
          return pMsgCat;
    }

    return NULL;
}

// check if the given locale is provided by OS and C run time
/* static */
bool wxLocale::IsAvailable(int lang)
{
    const wxLanguageInfo *info = wxLocale::GetLanguageInfo(lang);
    wxCHECK_MSG( info, false, _T("invalid language") );

#if defined(__WIN32__)
    if ( !info->WinLang )
        return false;

    if ( !::IsValidLocale
            (
                MAKELCID(MAKELANGID(info->WinLang, info->WinSublang),
                         SORT_DEFAULT),
                LCID_INSTALLED
            ) )
        return false;

#elif defined(__UNIX__)

    // Test if setting the locale works, then set it back.
    wxMB2WXbuf oldLocale = wxSetlocale(LC_ALL, wxEmptyString);
    wxMB2WXbuf tmp = wxSetlocaleTryUTF(LC_ALL, info->CanonicalName);
    if ( !tmp )
    {
        // Some C libraries don't like xx_YY form and require xx only
        tmp = wxSetlocaleTryUTF(LC_ALL, info->CanonicalName.Left(2));
        if ( !tmp )
            return false;
    }
    // restore the original locale
    wxSetlocale(LC_ALL, oldLocale);
#endif

    return true;
}

// check if the given catalog is loaded
bool wxLocale::IsLoaded(const wxChar *szDomain) const
{
  return FindCatalog(szDomain) != NULL;
}

// add a catalog to our linked list
bool wxLocale::AddCatalog(const wxChar *szDomain)
{
    return AddCatalog(szDomain, wxLANGUAGE_ENGLISH_US, NULL);
}

// add a catalog to our linked list
bool wxLocale::AddCatalog(const wxChar *szDomain,
                          wxLanguage    msgIdLanguage,
                          const wxChar *msgIdCharset)

{
  wxMsgCatalog *pMsgCat = new wxMsgCatalog;

  if ( pMsgCat->Load(m_strShort, szDomain, msgIdCharset, m_bConvertEncoding) ) {
    // add it to the head of the list so that in GetString it will
    // be searched before the catalogs added earlier
    pMsgCat->m_pNext = m_pMsgCat;
    m_pMsgCat = pMsgCat;

    return true;
  }
  else {
    // don't add it because it couldn't be loaded anyway
    delete pMsgCat;

    // It is OK to not load catalog if the msgid language and m_language match,
    // in which case we can directly display the texts embedded in program's
    // source code:
    if (m_language == msgIdLanguage)
        return true;

    // If there's no exact match, we may still get partial match where the
    // (basic) language is same, but the country differs. For example, it's
    // permitted to use en_US strings from sources even if m_language is en_GB:
    const wxLanguageInfo *msgIdLangInfo = GetLanguageInfo(msgIdLanguage);
    if ( msgIdLangInfo &&
         msgIdLangInfo->CanonicalName.Mid(0, 2) == m_strShort.Mid(0, 2) )
    {
        return true;
    }

    return false;
  }
}

// ----------------------------------------------------------------------------
// accessors for locale-dependent data
// ----------------------------------------------------------------------------

#ifdef __WXMSW__

/* static */
wxString wxLocale::GetInfo(wxLocaleInfo index, wxLocaleCategory WXUNUSED(cat))
{
    wxUint32 lcid = LOCALE_USER_DEFAULT;

    if (wxGetLocale())
    {
        const wxLanguageInfo *info = GetLanguageInfo(wxGetLocale()->GetLanguage());
        if (info)
        {                         ;
            lcid = MAKELCID(MAKELANGID(info->WinLang, info->WinSublang),
                                     SORT_DEFAULT);
        }
    }

    wxString str;
    wxChar buffer[256];
    size_t count;
    buffer[0] = wxT('\0');
    switch (index)
    {
        case wxLOCALE_DECIMAL_POINT:
            count = ::GetLocaleInfo(lcid, LOCALE_SDECIMAL, buffer, 256);
            if (!count)
                str << wxT(".");
            else
                str << buffer;
            break;
#if 0
        case wxSYS_LIST_SEPARATOR:
            count = ::GetLocaleInfo(lcid, LOCALE_SLIST, buffer, 256);
            if (!count)
                str << wxT(",");
            else
                str << buffer;
            break;
        case wxSYS_LEADING_ZERO: // 0 means no leading zero, 1 means leading zero
            count = ::GetLocaleInfo(lcid, LOCALE_ILZERO, buffer, 256);
            if (!count)
                str << wxT("0");
            else
                str << buffer;
            break;
#endif
        default:
            wxFAIL_MSG(wxT("Unknown System String !"));
    }
    return str;
}

#elif defined(__DARWIN__)

/* static */
wxString wxLocale::GetInfo(wxLocaleInfo index, wxLocaleCategory WXUNUSED(cat))
{
    CFLocaleRef userLocaleRefRaw;
    if ( wxGetLocale() )
    {
        userLocaleRefRaw = CFLocaleCreate
                           (
                                kCFAllocatorDefault,
                                wxMacCFStringHolder(wxGetLocale()->GetCanonicalName())
                           );
    }
    else // no current locale, use the default one
    {
        userLocaleRefRaw = CFLocaleCopyCurrent();
    }

    wxCFRef<CFLocaleRef> userLocaleRef(userLocaleRefRaw);

    CFTypeRef cfstr;
    switch ( index )
    {
        case wxLOCALE_THOUSANDS_SEP:
            cfstr = CFLocaleGetValue(userLocaleRef, kCFLocaleGroupingSeparator);
            break;

        case wxLOCALE_DECIMAL_POINT:
            cfstr = CFLocaleGetValue(userLocaleRef, kCFLocaleDecimalSeparator);
            break;

        default:
            wxFAIL_MSG( _T("Unknown locale info") );
    }

    wxMacCFStringHolder
        str(CFStringCreateCopy(NULL, static_cast<CFStringRef>(cfstr)));
    return str.AsString();
}

#else // !__WXMSW__ && !__DARWIN__

/* static */
wxString wxLocale::GetInfo(wxLocaleInfo index, wxLocaleCategory cat)
{
    struct lconv *locale_info = localeconv();
    switch (cat)
    {
        case wxLOCALE_CAT_NUMBER:
            switch (index)
            {
                case wxLOCALE_THOUSANDS_SEP:
                    return wxString(locale_info->thousands_sep,
                                    *wxConvCurrent);
                case wxLOCALE_DECIMAL_POINT:
                    return wxString(locale_info->decimal_point,
                                    *wxConvCurrent);
                default:
                    return wxEmptyString;
            }
        case wxLOCALE_CAT_MONEY:
            switch (index)
            {
                case wxLOCALE_THOUSANDS_SEP:
                    return wxString(locale_info->mon_thousands_sep,
                                    *wxConvCurrent);
                case wxLOCALE_DECIMAL_POINT:
                    return wxString(locale_info->mon_decimal_point,
                                    *wxConvCurrent);
                default:
                    return wxEmptyString;
            }
        default:
            return wxEmptyString;
    }
}

#endif // __WXMSW__/!__WXMSW__

// ----------------------------------------------------------------------------
// global functions and variables
// ----------------------------------------------------------------------------

// retrieve/change current locale
// ------------------------------

// the current locale object
static wxLocale *g_pLocale = NULL;

wxLocale *wxGetLocale()
{
  return g_pLocale;
}

wxLocale *wxSetLocale(wxLocale *pLocale)
{
  wxLocale *pOld = g_pLocale;
  g_pLocale = pLocale;
  return pOld;
}



// ----------------------------------------------------------------------------
// wxLocale module (for lazy destruction of languagesDB)
// ----------------------------------------------------------------------------

class wxLocaleModule: public wxModule
{
    DECLARE_DYNAMIC_CLASS(wxLocaleModule)
    public:
        wxLocaleModule() {}
        bool OnInit() { return true; }
        void OnExit() { wxLocale::DestroyLanguagesDB(); }
};

IMPLEMENT_DYNAMIC_CLASS(wxLocaleModule, wxModule)



// ----------------------------------------------------------------------------
// default languages table & initialization
// ----------------------------------------------------------------------------



// --- --- --- generated code begins here --- --- ---

// This table is generated by misc/languages/genlang.py
// When making changes, please put them into misc/languages/langtabl.txt

#if !defined(__WIN32__) || defined(__WXMICROWIN__)

#define SETWINLANG(info,lang,sublang)

#else

#define SETWINLANG(info,lang,sublang) \
    info.WinLang = lang, info.WinSublang = sublang;

#ifndef LANG_AFRIKAANS
#define LANG_AFRIKAANS (0)
#endif
#ifndef LANG_ALBANIAN
#define LANG_ALBANIAN (0)
#endif
#ifndef LANG_ARABIC
#define LANG_ARABIC (0)
#endif
#ifndef LANG_ARMENIAN
#define LANG_ARMENIAN (0)
#endif
#ifndef LANG_ASSAMESE
#define LANG_ASSAMESE (0)
#endif
#ifndef LANG_AZERI
#define LANG_AZERI (0)
#endif
#ifndef LANG_BASQUE
#define LANG_BASQUE (0)
#endif
#ifndef LANG_BELARUSIAN
#define LANG_BELARUSIAN (0)
#endif
#ifndef LANG_BENGALI
#define LANG_BENGALI (0)
#endif
#ifndef LANG_BULGARIAN
#define LANG_BULGARIAN (0)
#endif
#ifndef LANG_CATALAN
#define LANG_CATALAN (0)
#endif
#ifndef LANG_CHINESE
#define LANG_CHINESE (0)
#endif
#ifndef LANG_CROATIAN
#define LANG_CROATIAN (0)
#endif
#ifndef LANG_CZECH
#define LANG_CZECH (0)
#endif
#ifndef LANG_DANISH
#define LANG_DANISH (0)
#endif
#ifndef LANG_DUTCH
#define LANG_DUTCH (0)
#endif
#ifndef LANG_ENGLISH
#define LANG_ENGLISH (0)
#endif
#ifndef LANG_ESTONIAN
#define LANG_ESTONIAN (0)
#endif
#ifndef LANG_FAEROESE
#define LANG_FAEROESE (0)
#endif
#ifndef LANG_FARSI
#define LANG_FARSI (0)
#endif
#ifndef LANG_FINNISH
#define LANG_FINNISH (0)
#endif
#ifndef LANG_FRENCH
#define LANG_FRENCH (0)
#endif
#ifndef LANG_GEORGIAN
#define LANG_GEORGIAN (0)
#endif
#ifndef LANG_GERMAN
#define LANG_GERMAN (0)
#endif
#ifndef LANG_GREEK
#define LANG_GREEK (0)
#endif
#ifndef LANG_GUJARATI
#define LANG_GUJARATI (0)
#endif
#ifndef LANG_HEBREW
#define LANG_HEBREW (0)
#endif
#ifndef LANG_HINDI
#define LANG_HINDI (0)
#endif
#ifndef LANG_HUNGARIAN
#define LANG_HUNGARIAN (0)
#endif
#ifndef LANG_ICELANDIC
#define LANG_ICELANDIC (0)
#endif
#ifndef LANG_INDONESIAN
#define LANG_INDONESIAN (0)
#endif
#ifndef LANG_ITALIAN
#define LANG_ITALIAN (0)
#endif
#ifndef LANG_JAPANESE
#define LANG_JAPANESE (0)
#endif
#ifndef LANG_KANNADA
#define LANG_KANNADA (0)
#endif
#ifndef LANG_KASHMIRI
#define LANG_KASHMIRI (0)
#endif
#ifndef LANG_KAZAK
#define LANG_KAZAK (0)
#endif
#ifndef LANG_KONKANI
#define LANG_KONKANI (0)
#endif
#ifndef LANG_KOREAN
#define LANG_KOREAN (0)
#endif
#ifndef LANG_LATVIAN
#define LANG_LATVIAN (0)
#endif
#ifndef LANG_LITHUANIAN
#define LANG_LITHUANIAN (0)
#endif
#ifndef LANG_MACEDONIAN
#define LANG_MACEDONIAN (0)
#endif
#ifndef LANG_MALAY
#define LANG_MALAY (0)
#endif
#ifndef LANG_MALAYALAM
#define LANG_MALAYALAM (0)
#endif
#ifndef LANG_MANIPURI
#define LANG_MANIPURI (0)
#endif
#ifndef LANG_MARATHI
#define LANG_MARATHI (0)
#endif
#ifndef LANG_NEPALI
#define LANG_NEPALI (0)
#endif
#ifndef LANG_NORWEGIAN
#define LANG_NORWEGIAN (0)
#endif
#ifndef LANG_ORIYA
#define LANG_ORIYA (0)
#endif
#ifndef LANG_POLISH
#define LANG_POLISH (0)
#endif
#ifndef LANG_PORTUGUESE
#define LANG_PORTUGUESE (0)
#endif
#ifndef LANG_PUNJABI
#define LANG_PUNJABI (0)
#endif
#ifndef LANG_ROMANIAN
#define LANG_ROMANIAN (0)
#endif
#ifndef LANG_RUSSIAN
#define LANG_RUSSIAN (0)
#endif
#ifndef LANG_SAMI
#define LANG_SAMI (0)
#endif
#ifndef LANG_SANSKRIT
#define LANG_SANSKRIT (0)
#endif
#ifndef LANG_SERBIAN
#define LANG_SERBIAN (0)
#endif
#ifndef LANG_SINDHI
#define LANG_SINDHI (0)
#endif
#ifndef LANG_SLOVAK
#define LANG_SLOVAK (0)
#endif
#ifndef LANG_SLOVENIAN
#define LANG_SLOVENIAN (0)
#endif
#ifndef LANG_SPANISH
#define LANG_SPANISH (0)
#endif
#ifndef LANG_SWAHILI
#define LANG_SWAHILI (0)
#endif
#ifndef LANG_SWEDISH
#define LANG_SWEDISH (0)
#endif
#ifndef LANG_TAMIL
#define LANG_TAMIL (0)
#endif
#ifndef LANG_TATAR
#define LANG_TATAR (0)
#endif
#ifndef LANG_TELUGU
#define LANG_TELUGU (0)
#endif
#ifndef LANG_THAI
#define LANG_THAI (0)
#endif
#ifndef LANG_TURKISH
#define LANG_TURKISH (0)
#endif
#ifndef LANG_UKRAINIAN
#define LANG_UKRAINIAN (0)
#endif
#ifndef LANG_URDU
#define LANG_URDU (0)
#endif
#ifndef LANG_UZBEK
#define LANG_UZBEK (0)
#endif
#ifndef LANG_VALENCIAN
#define LANG_VALENCIAN (0)
#endif
#ifndef LANG_VIETNAMESE
#define LANG_VIETNAMESE (0)
#endif
#ifndef SUBLANG_ARABIC_ALGERIA
#define SUBLANG_ARABIC_ALGERIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_BAHRAIN
#define SUBLANG_ARABIC_BAHRAIN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_EGYPT
#define SUBLANG_ARABIC_EGYPT SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_IRAQ
#define SUBLANG_ARABIC_IRAQ SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_JORDAN
#define SUBLANG_ARABIC_JORDAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_KUWAIT
#define SUBLANG_ARABIC_KUWAIT SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_LEBANON
#define SUBLANG_ARABIC_LEBANON SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_LIBYA
#define SUBLANG_ARABIC_LIBYA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_MOROCCO
#define SUBLANG_ARABIC_MOROCCO SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_OMAN
#define SUBLANG_ARABIC_OMAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_QATAR
#define SUBLANG_ARABIC_QATAR SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_SAUDI_ARABIA
#define SUBLANG_ARABIC_SAUDI_ARABIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_SYRIA
#define SUBLANG_ARABIC_SYRIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_TUNISIA
#define SUBLANG_ARABIC_TUNISIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_UAE
#define SUBLANG_ARABIC_UAE SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ARABIC_YEMEN
#define SUBLANG_ARABIC_YEMEN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_AZERI_CYRILLIC
#define SUBLANG_AZERI_CYRILLIC SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_AZERI_LATIN
#define SUBLANG_AZERI_LATIN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_CHINESE_SIMPLIFIED
#define SUBLANG_CHINESE_SIMPLIFIED SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_CHINESE_TRADITIONAL
#define SUBLANG_CHINESE_TRADITIONAL SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_CHINESE_HONGKONG
#define SUBLANG_CHINESE_HONGKONG SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_CHINESE_MACAU
#define SUBLANG_CHINESE_MACAU SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_CHINESE_SINGAPORE
#define SUBLANG_CHINESE_SINGAPORE SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_DUTCH
#define SUBLANG_DUTCH SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_DUTCH_BELGIAN
#define SUBLANG_DUTCH_BELGIAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_UK
#define SUBLANG_ENGLISH_UK SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_US
#define SUBLANG_ENGLISH_US SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_AUS
#define SUBLANG_ENGLISH_AUS SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_BELIZE
#define SUBLANG_ENGLISH_BELIZE SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_CAN
#define SUBLANG_ENGLISH_CAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_CARIBBEAN
#define SUBLANG_ENGLISH_CARIBBEAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_EIRE
#define SUBLANG_ENGLISH_EIRE SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_JAMAICA
#define SUBLANG_ENGLISH_JAMAICA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_NZ
#define SUBLANG_ENGLISH_NZ SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_PHILIPPINES
#define SUBLANG_ENGLISH_PHILIPPINES SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_SOUTH_AFRICA
#define SUBLANG_ENGLISH_SOUTH_AFRICA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_TRINIDAD
#define SUBLANG_ENGLISH_TRINIDAD SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ENGLISH_ZIMBABWE
#define SUBLANG_ENGLISH_ZIMBABWE SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_FRENCH
#define SUBLANG_FRENCH SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_FRENCH_BELGIAN
#define SUBLANG_FRENCH_BELGIAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_FRENCH_CANADIAN
#define SUBLANG_FRENCH_CANADIAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_FRENCH_LUXEMBOURG
#define SUBLANG_FRENCH_LUXEMBOURG SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_FRENCH_MONACO
#define SUBLANG_FRENCH_MONACO SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_FRENCH_SWISS
#define SUBLANG_FRENCH_SWISS SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_GERMAN
#define SUBLANG_GERMAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_GERMAN_AUSTRIAN
#define SUBLANG_GERMAN_AUSTRIAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_GERMAN_LIECHTENSTEIN
#define SUBLANG_GERMAN_LIECHTENSTEIN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_GERMAN_LUXEMBOURG
#define SUBLANG_GERMAN_LUXEMBOURG SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_GERMAN_SWISS
#define SUBLANG_GERMAN_SWISS SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ITALIAN
#define SUBLANG_ITALIAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_ITALIAN_SWISS
#define SUBLANG_ITALIAN_SWISS SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_KASHMIRI_INDIA
#define SUBLANG_KASHMIRI_INDIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_KOREAN
#define SUBLANG_KOREAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_LITHUANIAN
#define SUBLANG_LITHUANIAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_MALAY_BRUNEI_DARUSSALAM
#define SUBLANG_MALAY_BRUNEI_DARUSSALAM SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_MALAY_MALAYSIA
#define SUBLANG_MALAY_MALAYSIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_NEPALI_INDIA
#define SUBLANG_NEPALI_INDIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_NORWEGIAN_BOKMAL
#define SUBLANG_NORWEGIAN_BOKMAL SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_NORWEGIAN_NYNORSK
#define SUBLANG_NORWEGIAN_NYNORSK SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_PORTUGUESE
#define SUBLANG_PORTUGUESE SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_PORTUGUESE_BRAZILIAN
#define SUBLANG_PORTUGUESE_BRAZILIAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SERBIAN_CYRILLIC
#define SUBLANG_SERBIAN_CYRILLIC SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SERBIAN_LATIN
#define SUBLANG_SERBIAN_LATIN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH
#define SUBLANG_SPANISH SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_ARGENTINA
#define SUBLANG_SPANISH_ARGENTINA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_BOLIVIA
#define SUBLANG_SPANISH_BOLIVIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_CHILE
#define SUBLANG_SPANISH_CHILE SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_COLOMBIA
#define SUBLANG_SPANISH_COLOMBIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_COSTA_RICA
#define SUBLANG_SPANISH_COSTA_RICA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_DOMINICAN_REPUBLIC
#define SUBLANG_SPANISH_DOMINICAN_REPUBLIC SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_ECUADOR
#define SUBLANG_SPANISH_ECUADOR SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_EL_SALVADOR
#define SUBLANG_SPANISH_EL_SALVADOR SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_GUATEMALA
#define SUBLANG_SPANISH_GUATEMALA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_HONDURAS
#define SUBLANG_SPANISH_HONDURAS SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_MEXICAN
#define SUBLANG_SPANISH_MEXICAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_MODERN
#define SUBLANG_SPANISH_MODERN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_NICARAGUA
#define SUBLANG_SPANISH_NICARAGUA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_PANAMA
#define SUBLANG_SPANISH_PANAMA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_PARAGUAY
#define SUBLANG_SPANISH_PARAGUAY SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_PERU
#define SUBLANG_SPANISH_PERU SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_PUERTO_RICO
#define SUBLANG_SPANISH_PUERTO_RICO SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_URUGUAY
#define SUBLANG_SPANISH_URUGUAY SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SPANISH_VENEZUELA
#define SUBLANG_SPANISH_VENEZUELA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SWEDISH
#define SUBLANG_SWEDISH SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_SWEDISH_FINLAND
#define SUBLANG_SWEDISH_FINLAND SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_URDU_INDIA
#define SUBLANG_URDU_INDIA SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_URDU_PAKISTAN
#define SUBLANG_URDU_PAKISTAN SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_UZBEK_CYRILLIC
#define SUBLANG_UZBEK_CYRILLIC SUBLANG_DEFAULT
#endif
#ifndef SUBLANG_UZBEK_LATIN
#define SUBLANG_UZBEK_LATIN SUBLANG_DEFAULT
#endif


#endif // __WIN32__

#define LNG(wxlang, canonical, winlang, winsublang, layout, desc) \
    info.Language = wxlang;                               \
    info.CanonicalName = wxT(canonical);                  \
    info.LayoutDirection = layout;                        \
    info.Description = wxT(desc);                         \
    SETWINLANG(info, winlang, winsublang)                 \
    AddLanguage(info);

void wxLocale::InitLanguagesDB()
{
   wxLanguageInfo info;
   wxStringTokenizer tkn;

   LNG(wxLANGUAGE_ABKHAZIAN,                  "ab"   , 0              , 0                                 , wxLayout_LeftToRight, "Abkhazian")
   LNG(wxLANGUAGE_AFAR,                       "aa"   , 0              , 0                                 , wxLayout_LeftToRight, "Afar")
   LNG(wxLANGUAGE_AFRIKAANS,                  "af_ZA", LANG_AFRIKAANS , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Afrikaans")
   LNG(wxLANGUAGE_ALBANIAN,                   "sq_AL", LANG_ALBANIAN  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Albanian")
   LNG(wxLANGUAGE_AMHARIC,                    "am"   , 0              , 0                                 , wxLayout_LeftToRight, "Amharic")
   LNG(wxLANGUAGE_ARABIC,                     "ar"   , LANG_ARABIC    , SUBLANG_DEFAULT                   , wxLayout_RightToLeft, "Arabic")
   LNG(wxLANGUAGE_ARABIC_ALGERIA,             "ar_DZ", LANG_ARABIC    , SUBLANG_ARABIC_ALGERIA            , wxLayout_RightToLeft, "Arabic (Algeria)")
   LNG(wxLANGUAGE_ARABIC_BAHRAIN,             "ar_BH", LANG_ARABIC    , SUBLANG_ARABIC_BAHRAIN            , wxLayout_RightToLeft, "Arabic (Bahrain)")
   LNG(wxLANGUAGE_ARABIC_EGYPT,               "ar_EG", LANG_ARABIC    , SUBLANG_ARABIC_EGYPT              , wxLayout_RightToLeft, "Arabic (Egypt)")
   LNG(wxLANGUAGE_ARABIC_IRAQ,                "ar_IQ", LANG_ARABIC    , SUBLANG_ARABIC_IRAQ               , wxLayout_RightToLeft, "Arabic (Iraq)")
   LNG(wxLANGUAGE_ARABIC_JORDAN,              "ar_JO", LANG_ARABIC    , SUBLANG_ARABIC_JORDAN             , wxLayout_RightToLeft, "Arabic (Jordan)")
   LNG(wxLANGUAGE_ARABIC_KUWAIT,              "ar_KW", LANG_ARABIC    , SUBLANG_ARABIC_KUWAIT             , wxLayout_RightToLeft, "Arabic (Kuwait)")
   LNG(wxLANGUAGE_ARABIC_LEBANON,             "ar_LB", LANG_ARABIC    , SUBLANG_ARABIC_LEBANON            , wxLayout_RightToLeft, "Arabic (Lebanon)")
   LNG(wxLANGUAGE_ARABIC_LIBYA,               "ar_LY", LANG_ARABIC    , SUBLANG_ARABIC_LIBYA              , wxLayout_RightToLeft, "Arabic (Libya)")
   LNG(wxLANGUAGE_ARABIC_MOROCCO,             "ar_MA", LANG_ARABIC    , SUBLANG_ARABIC_MOROCCO            , wxLayout_RightToLeft, "Arabic (Morocco)")
   LNG(wxLANGUAGE_ARABIC_OMAN,                "ar_OM", LANG_ARABIC    , SUBLANG_ARABIC_OMAN               , wxLayout_RightToLeft, "Arabic (Oman)")
   LNG(wxLANGUAGE_ARABIC_QATAR,               "ar_QA", LANG_ARABIC    , SUBLANG_ARABIC_QATAR              , wxLayout_RightToLeft, "Arabic (Qatar)")
   LNG(wxLANGUAGE_ARABIC_SAUDI_ARABIA,        "ar_SA", LANG_ARABIC    , SUBLANG_ARABIC_SAUDI_ARABIA       , wxLayout_RightToLeft, "Arabic (Saudi Arabia)")
   LNG(wxLANGUAGE_ARABIC_SUDAN,               "ar_SD", 0              , 0                                 , wxLayout_RightToLeft, "Arabic (Sudan)")
   LNG(wxLANGUAGE_ARABIC_SYRIA,               "ar_SY", LANG_ARABIC    , SUBLANG_ARABIC_SYRIA              , wxLayout_RightToLeft, "Arabic (Syria)")
   LNG(wxLANGUAGE_ARABIC_TUNISIA,             "ar_TN", LANG_ARABIC    , SUBLANG_ARABIC_TUNISIA            , wxLayout_RightToLeft, "Arabic (Tunisia)")
   LNG(wxLANGUAGE_ARABIC_UAE,                 "ar_AE", LANG_ARABIC    , SUBLANG_ARABIC_UAE                , wxLayout_RightToLeft, "Arabic (Uae)")
   LNG(wxLANGUAGE_ARABIC_YEMEN,               "ar_YE", LANG_ARABIC    , SUBLANG_ARABIC_YEMEN              , wxLayout_RightToLeft, "Arabic (Yemen)")
   LNG(wxLANGUAGE_ARMENIAN,                   "hy"   , LANG_ARMENIAN  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Armenian")
   LNG(wxLANGUAGE_ASSAMESE,                   "as"   , LANG_ASSAMESE  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Assamese")
   LNG(wxLANGUAGE_AYMARA,                     "ay"   , 0              , 0                                 , wxLayout_LeftToRight, "Aymara")
   LNG(wxLANGUAGE_AZERI,                      "az"   , LANG_AZERI     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Azeri")
   LNG(wxLANGUAGE_AZERI_CYRILLIC,             "az"   , LANG_AZERI     , SUBLANG_AZERI_CYRILLIC            , wxLayout_LeftToRight, "Azeri (Cyrillic)")
   LNG(wxLANGUAGE_AZERI_LATIN,                "az"   , LANG_AZERI     , SUBLANG_AZERI_LATIN               , wxLayout_LeftToRight, "Azeri (Latin)")
   LNG(wxLANGUAGE_BASHKIR,                    "ba"   , 0              , 0                                 , wxLayout_LeftToRight, "Bashkir")
   LNG(wxLANGUAGE_BASQUE,                     "eu_ES", LANG_BASQUE    , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Basque")
   LNG(wxLANGUAGE_BELARUSIAN,                 "be_BY", LANG_BELARUSIAN, SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Belarusian")
   LNG(wxLANGUAGE_BENGALI,                    "bn"   , LANG_BENGALI   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Bengali")
   LNG(wxLANGUAGE_BHUTANI,                    "dz"   , 0              , 0                                 , wxLayout_LeftToRight, "Bhutani")
   LNG(wxLANGUAGE_BIHARI,                     "bh"   , 0              , 0                                 , wxLayout_LeftToRight, "Bihari")
   LNG(wxLANGUAGE_BISLAMA,                    "bi"   , 0              , 0                                 , wxLayout_LeftToRight, "Bislama")
   LNG(wxLANGUAGE_BRETON,                     "br"   , 0              , 0                                 , wxLayout_LeftToRight, "Breton")
   LNG(wxLANGUAGE_BULGARIAN,                  "bg_BG", LANG_BULGARIAN , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Bulgarian")
   LNG(wxLANGUAGE_BURMESE,                    "my"   , 0              , 0                                 , wxLayout_LeftToRight, "Burmese")
   LNG(wxLANGUAGE_CAMBODIAN,                  "km"   , 0              , 0                                 , wxLayout_LeftToRight, "Cambodian")
   LNG(wxLANGUAGE_CATALAN,                    "ca_ES", LANG_CATALAN   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Catalan")
   LNG(wxLANGUAGE_CHINESE,                    "zh_TW", LANG_CHINESE   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Chinese")
   LNG(wxLANGUAGE_CHINESE_SIMPLIFIED,         "zh_CN", LANG_CHINESE   , SUBLANG_CHINESE_SIMPLIFIED        , wxLayout_LeftToRight, "Chinese (Simplified)")
   LNG(wxLANGUAGE_CHINESE_TRADITIONAL,        "zh_TW", LANG_CHINESE   , SUBLANG_CHINESE_TRADITIONAL       , wxLayout_LeftToRight, "Chinese (Traditional)")
   LNG(wxLANGUAGE_CHINESE_HONGKONG,           "zh_HK", LANG_CHINESE   , SUBLANG_CHINESE_HONGKONG          , wxLayout_LeftToRight, "Chinese (Hongkong)")
   LNG(wxLANGUAGE_CHINESE_MACAU,              "zh_MO", LANG_CHINESE   , SUBLANG_CHINESE_MACAU             , wxLayout_LeftToRight, "Chinese (Macau)")
   LNG(wxLANGUAGE_CHINESE_SINGAPORE,          "zh_SG", LANG_CHINESE   , SUBLANG_CHINESE_SINGAPORE         , wxLayout_LeftToRight, "Chinese (Singapore)")
   LNG(wxLANGUAGE_CHINESE_TAIWAN,             "zh_TW", LANG_CHINESE   , SUBLANG_CHINESE_TRADITIONAL       , wxLayout_LeftToRight, "Chinese (Taiwan)")
   LNG(wxLANGUAGE_CORSICAN,                   "co"   , 0              , 0                                 , wxLayout_LeftToRight, "Corsican")
   LNG(wxLANGUAGE_CROATIAN,                   "hr_HR", LANG_CROATIAN  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Croatian")
   LNG(wxLANGUAGE_CZECH,                      "cs_CZ", LANG_CZECH     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Czech")
   LNG(wxLANGUAGE_DANISH,                     "da_DK", LANG_DANISH    , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Danish")
   LNG(wxLANGUAGE_DUTCH,                      "nl_NL", LANG_DUTCH     , SUBLANG_DUTCH                     , wxLayout_LeftToRight, "Dutch")
   LNG(wxLANGUAGE_DUTCH_BELGIAN,              "nl_BE", LANG_DUTCH     , SUBLANG_DUTCH_BELGIAN             , wxLayout_LeftToRight, "Dutch (Belgian)")
   LNG(wxLANGUAGE_ENGLISH,                    "en_GB", LANG_ENGLISH   , SUBLANG_ENGLISH_UK                , wxLayout_LeftToRight, "English")
   LNG(wxLANGUAGE_ENGLISH_UK,                 "en_GB", LANG_ENGLISH   , SUBLANG_ENGLISH_UK                , wxLayout_LeftToRight, "English (U.K.)")
   LNG(wxLANGUAGE_ENGLISH_US,                 "en_US", LANG_ENGLISH   , SUBLANG_ENGLISH_US                , wxLayout_LeftToRight, "English (U.S.)")
   LNG(wxLANGUAGE_ENGLISH_AUSTRALIA,          "en_AU", LANG_ENGLISH   , SUBLANG_ENGLISH_AUS               , wxLayout_LeftToRight, "English (Australia)")
   LNG(wxLANGUAGE_ENGLISH_BELIZE,             "en_BZ", LANG_ENGLISH   , SUBLANG_ENGLISH_BELIZE            , wxLayout_LeftToRight, "English (Belize)")
   LNG(wxLANGUAGE_ENGLISH_BOTSWANA,           "en_BW", 0              , 0                                 , wxLayout_LeftToRight, "English (Botswana)")
   LNG(wxLANGUAGE_ENGLISH_CANADA,             "en_CA", LANG_ENGLISH   , SUBLANG_ENGLISH_CAN               , wxLayout_LeftToRight, "English (Canada)")
   LNG(wxLANGUAGE_ENGLISH_CARIBBEAN,          "en_CB", LANG_ENGLISH   , SUBLANG_ENGLISH_CARIBBEAN         , wxLayout_LeftToRight, "English (Caribbean)")
   LNG(wxLANGUAGE_ENGLISH_DENMARK,            "en_DK", 0              , 0                                 , wxLayout_LeftToRight, "English (Denmark)")
   LNG(wxLANGUAGE_ENGLISH_EIRE,               "en_IE", LANG_ENGLISH   , SUBLANG_ENGLISH_EIRE              , wxLayout_LeftToRight, "English (Eire)")
   LNG(wxLANGUAGE_ENGLISH_JAMAICA,            "en_JM", LANG_ENGLISH   , SUBLANG_ENGLISH_JAMAICA           , wxLayout_LeftToRight, "English (Jamaica)")
   LNG(wxLANGUAGE_ENGLISH_NEW_ZEALAND,        "en_NZ", LANG_ENGLISH   , SUBLANG_ENGLISH_NZ                , wxLayout_LeftToRight, "English (New Zealand)")
   LNG(wxLANGUAGE_ENGLISH_PHILIPPINES,        "en_PH", LANG_ENGLISH   , SUBLANG_ENGLISH_PHILIPPINES       , wxLayout_LeftToRight, "English (Philippines)")
   LNG(wxLANGUAGE_ENGLISH_SOUTH_AFRICA,       "en_ZA", LANG_ENGLISH   , SUBLANG_ENGLISH_SOUTH_AFRICA      , wxLayout_LeftToRight, "English (South Africa)")
   LNG(wxLANGUAGE_ENGLISH_TRINIDAD,           "en_TT", LANG_ENGLISH   , SUBLANG_ENGLISH_TRINIDAD          , wxLayout_LeftToRight, "English (Trinidad)")
   LNG(wxLANGUAGE_ENGLISH_ZIMBABWE,           "en_ZW", LANG_ENGLISH   , SUBLANG_ENGLISH_ZIMBABWE          , wxLayout_LeftToRight, "English (Zimbabwe)")
   LNG(wxLANGUAGE_ESPERANTO,                  "eo"   , 0              , 0                                 , wxLayout_LeftToRight, "Esperanto")
   LNG(wxLANGUAGE_ESTONIAN,                   "et_EE", LANG_ESTONIAN  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Estonian")
   LNG(wxLANGUAGE_FAEROESE,                   "fo_FO", LANG_FAEROESE  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Faeroese")
   LNG(wxLANGUAGE_FARSI,                      "fa_IR", LANG_FARSI     , SUBLANG_DEFAULT                   , wxLayout_RightToLeft, "Farsi")
   LNG(wxLANGUAGE_FIJI,                       "fj"   , 0              , 0                                 , wxLayout_LeftToRight, "Fiji")
   LNG(wxLANGUAGE_FINNISH,                    "fi_FI", LANG_FINNISH   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Finnish")
   LNG(wxLANGUAGE_FRENCH,                     "fr_FR", LANG_FRENCH    , SUBLANG_FRENCH                    , wxLayout_LeftToRight, "French")
   LNG(wxLANGUAGE_FRENCH_BELGIAN,             "fr_BE", LANG_FRENCH    , SUBLANG_FRENCH_BELGIAN            , wxLayout_LeftToRight, "French (Belgian)")
   LNG(wxLANGUAGE_FRENCH_CANADIAN,            "fr_CA", LANG_FRENCH    , SUBLANG_FRENCH_CANADIAN           , wxLayout_LeftToRight, "French (Canadian)")
   LNG(wxLANGUAGE_FRENCH_LUXEMBOURG,          "fr_LU", LANG_FRENCH    , SUBLANG_FRENCH_LUXEMBOURG         , wxLayout_LeftToRight, "French (Luxembourg)")
   LNG(wxLANGUAGE_FRENCH_MONACO,              "fr_MC", LANG_FRENCH    , SUBLANG_FRENCH_MONACO             , wxLayout_LeftToRight, "French (Monaco)")
   LNG(wxLANGUAGE_FRENCH_SWISS,               "fr_CH", LANG_FRENCH    , SUBLANG_FRENCH_SWISS              , wxLayout_LeftToRight, "French (Swiss)")
   LNG(wxLANGUAGE_FRISIAN,                    "fy"   , 0              , 0                                 , wxLayout_LeftToRight, "Frisian")
   LNG(wxLANGUAGE_GALICIAN,                   "gl_ES", 0              , 0                                 , wxLayout_LeftToRight, "Galician")
   LNG(wxLANGUAGE_GEORGIAN,                   "ka_GE", LANG_GEORGIAN  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Georgian")
   LNG(wxLANGUAGE_GERMAN,                     "de_DE", LANG_GERMAN    , SUBLANG_GERMAN                    , wxLayout_LeftToRight, "German")
   LNG(wxLANGUAGE_GERMAN_AUSTRIAN,            "de_AT", LANG_GERMAN    , SUBLANG_GERMAN_AUSTRIAN           , wxLayout_LeftToRight, "German (Austrian)")
   LNG(wxLANGUAGE_GERMAN_BELGIUM,             "de_BE", 0              , 0                                 , wxLayout_LeftToRight, "German (Belgium)")
   LNG(wxLANGUAGE_GERMAN_LIECHTENSTEIN,       "de_LI", LANG_GERMAN    , SUBLANG_GERMAN_LIECHTENSTEIN      , wxLayout_LeftToRight, "German (Liechtenstein)")
   LNG(wxLANGUAGE_GERMAN_LUXEMBOURG,          "de_LU", LANG_GERMAN    , SUBLANG_GERMAN_LUXEMBOURG         , wxLayout_LeftToRight, "German (Luxembourg)")
   LNG(wxLANGUAGE_GERMAN_SWISS,               "de_CH", LANG_GERMAN    , SUBLANG_GERMAN_SWISS              , wxLayout_LeftToRight, "German (Swiss)")
   LNG(wxLANGUAGE_GREEK,                      "el_GR", LANG_GREEK     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Greek")
   LNG(wxLANGUAGE_GREENLANDIC,                "kl_GL", 0              , 0                                 , wxLayout_LeftToRight, "Greenlandic")
   LNG(wxLANGUAGE_GUARANI,                    "gn"   , 0              , 0                                 , wxLayout_LeftToRight, "Guarani")
   LNG(wxLANGUAGE_GUJARATI,                   "gu"   , LANG_GUJARATI  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Gujarati")
   LNG(wxLANGUAGE_HAUSA,                      "ha"   , 0              , 0                                 , wxLayout_LeftToRight, "Hausa")
   LNG(wxLANGUAGE_HEBREW,                     "he_IL", LANG_HEBREW    , SUBLANG_DEFAULT                   , wxLayout_RightToLeft, "Hebrew")
   LNG(wxLANGUAGE_HINDI,                      "hi_IN", LANG_HINDI     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Hindi")
   LNG(wxLANGUAGE_HUNGARIAN,                  "hu_HU", LANG_HUNGARIAN , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Hungarian")
   LNG(wxLANGUAGE_ICELANDIC,                  "is_IS", LANG_ICELANDIC , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Icelandic")
   LNG(wxLANGUAGE_INDONESIAN,                 "id_ID", LANG_INDONESIAN, SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Indonesian")
   LNG(wxLANGUAGE_INTERLINGUA,                "ia"   , 0              , 0                                 , wxLayout_LeftToRight, "Interlingua")
   LNG(wxLANGUAGE_INTERLINGUE,                "ie"   , 0              , 0                                 , wxLayout_LeftToRight, "Interlingue")
   LNG(wxLANGUAGE_INUKTITUT,                  "iu"   , 0              , 0                                 , wxLayout_LeftToRight, "Inuktitut")
   LNG(wxLANGUAGE_INUPIAK,                    "ik"   , 0              , 0                                 , wxLayout_LeftToRight, "Inupiak")
   LNG(wxLANGUAGE_IRISH,                      "ga_IE", 0              , 0                                 , wxLayout_LeftToRight, "Irish")
   LNG(wxLANGUAGE_ITALIAN,                    "it_IT", LANG_ITALIAN   , SUBLANG_ITALIAN                   , wxLayout_LeftToRight, "Italian")
   LNG(wxLANGUAGE_ITALIAN_SWISS,              "it_CH", LANG_ITALIAN   , SUBLANG_ITALIAN_SWISS             , wxLayout_LeftToRight, "Italian (Swiss)")
   LNG(wxLANGUAGE_JAPANESE,                   "ja_JP", LANG_JAPANESE  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Japanese")
   LNG(wxLANGUAGE_JAVANESE,                   "jw"   , 0              , 0                                 , wxLayout_LeftToRight, "Javanese")
   LNG(wxLANGUAGE_KANNADA,                    "kn"   , LANG_KANNADA   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Kannada")
   LNG(wxLANGUAGE_KASHMIRI,                   "ks"   , LANG_KASHMIRI  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Kashmiri")
   LNG(wxLANGUAGE_KASHMIRI_INDIA,             "ks_IN", LANG_KASHMIRI  , SUBLANG_KASHMIRI_INDIA            , wxLayout_LeftToRight, "Kashmiri (India)")
   LNG(wxLANGUAGE_KAZAKH,                     "kk"   , LANG_KAZAK     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Kazakh")
   LNG(wxLANGUAGE_KERNEWEK,                   "kw_GB", 0              , 0                                 , wxLayout_LeftToRight, "Kernewek")
   LNG(wxLANGUAGE_KINYARWANDA,                "rw"   , 0              , 0                                 , wxLayout_LeftToRight, "Kinyarwanda")
   LNG(wxLANGUAGE_KIRGHIZ,                    "ky"   , 0              , 0                                 , wxLayout_LeftToRight, "Kirghiz")
   LNG(wxLANGUAGE_KIRUNDI,                    "rn"   , 0              , 0                                 , wxLayout_LeftToRight, "Kirundi")
   LNG(wxLANGUAGE_KONKANI,                    ""     , LANG_KONKANI   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Konkani")
   LNG(wxLANGUAGE_KOREAN,                     "ko_KR", LANG_KOREAN    , SUBLANG_KOREAN                    , wxLayout_LeftToRight, "Korean")
   LNG(wxLANGUAGE_KURDISH,                    "ku_TR", 0              , 0                                 , wxLayout_LeftToRight, "Kurdish")
   LNG(wxLANGUAGE_LAOTHIAN,                   "lo"   , 0              , 0                                 , wxLayout_LeftToRight, "Laothian")
   LNG(wxLANGUAGE_LATIN,                      "la"   , 0              , 0                                 , wxLayout_LeftToRight, "Latin")
   LNG(wxLANGUAGE_LATVIAN,                    "lv_LV", LANG_LATVIAN   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Latvian")
   LNG(wxLANGUAGE_LINGALA,                    "ln"   , 0              , 0                                 , wxLayout_LeftToRight, "Lingala")
   LNG(wxLANGUAGE_LITHUANIAN,                 "lt_LT", LANG_LITHUANIAN, SUBLANG_LITHUANIAN                , wxLayout_LeftToRight, "Lithuanian")
   LNG(wxLANGUAGE_MACEDONIAN,                 "mk_MK", LANG_MACEDONIAN, SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Macedonian")
   LNG(wxLANGUAGE_MALAGASY,                   "mg"   , 0              , 0                                 , wxLayout_LeftToRight, "Malagasy")
   LNG(wxLANGUAGE_MALAY,                      "ms_MY", LANG_MALAY     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Malay")
   LNG(wxLANGUAGE_MALAYALAM,                  "ml"   , LANG_MALAYALAM , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Malayalam")
   LNG(wxLANGUAGE_MALAY_BRUNEI_DARUSSALAM,    "ms_BN", LANG_MALAY     , SUBLANG_MALAY_BRUNEI_DARUSSALAM   , wxLayout_LeftToRight, "Malay (Brunei Darussalam)")
   LNG(wxLANGUAGE_MALAY_MALAYSIA,             "ms_MY", LANG_MALAY     , SUBLANG_MALAY_MALAYSIA            , wxLayout_LeftToRight, "Malay (Malaysia)")
   LNG(wxLANGUAGE_MALTESE,                    "mt_MT", 0              , 0                                 , wxLayout_LeftToRight, "Maltese")
   LNG(wxLANGUAGE_MANIPURI,                   ""     , LANG_MANIPURI  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Manipuri")
   LNG(wxLANGUAGE_MAORI,                      "mi"   , 0              , 0                                 , wxLayout_LeftToRight, "Maori")
   LNG(wxLANGUAGE_MARATHI,                    "mr_IN", LANG_MARATHI   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Marathi")
   LNG(wxLANGUAGE_MOLDAVIAN,                  "mo"   , 0              , 0                                 , wxLayout_LeftToRight, "Moldavian")
   LNG(wxLANGUAGE_MONGOLIAN,                  "mn"   , 0              , 0                                 , wxLayout_LeftToRight, "Mongolian")
   LNG(wxLANGUAGE_NAURU,                      "na"   , 0              , 0                                 , wxLayout_LeftToRight, "Nauru")
   LNG(wxLANGUAGE_NEPALI,                     "ne_NP", LANG_NEPALI    , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Nepali")
   LNG(wxLANGUAGE_NEPALI_INDIA,               "ne_IN", LANG_NEPALI    , SUBLANG_NEPALI_INDIA              , wxLayout_LeftToRight, "Nepali (India)")
   LNG(wxLANGUAGE_NORWEGIAN_BOKMAL,           "nb_NO", LANG_NORWEGIAN , SUBLANG_NORWEGIAN_BOKMAL          , wxLayout_LeftToRight, "Norwegian (Bokmal)")
   LNG(wxLANGUAGE_NORWEGIAN_NYNORSK,          "nn_NO", LANG_NORWEGIAN , SUBLANG_NORWEGIAN_NYNORSK         , wxLayout_LeftToRight, "Norwegian (Nynorsk)")
   LNG(wxLANGUAGE_OCCITAN,                    "oc"   , 0              , 0                                 , wxLayout_LeftToRight, "Occitan")
   LNG(wxLANGUAGE_ORIYA,                      "or"   , LANG_ORIYA     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Oriya")
   LNG(wxLANGUAGE_OROMO,                      "om"   , 0              , 0                                 , wxLayout_LeftToRight, "(Afan) Oromo")
   LNG(wxLANGUAGE_PASHTO,                     "ps"   , 0              , 0                                 , wxLayout_LeftToRight, "Pashto, Pushto")
   LNG(wxLANGUAGE_POLISH,                     "pl_PL", LANG_POLISH    , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Polish")
   LNG(wxLANGUAGE_PORTUGUESE,                 "pt_PT", LANG_PORTUGUESE, SUBLANG_PORTUGUESE                , wxLayout_LeftToRight, "Portuguese")
   LNG(wxLANGUAGE_PORTUGUESE_BRAZILIAN,       "pt_BR", LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN      , wxLayout_LeftToRight, "Portuguese (Brazilian)")
   LNG(wxLANGUAGE_PUNJABI,                    "pa"   , LANG_PUNJABI   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Punjabi")
   LNG(wxLANGUAGE_QUECHUA,                    "qu"   , 0              , 0                                 , wxLayout_LeftToRight, "Quechua")
   LNG(wxLANGUAGE_RHAETO_ROMANCE,             "rm"   , 0              , 0                                 , wxLayout_LeftToRight, "Rhaeto-Romance")
   LNG(wxLANGUAGE_ROMANIAN,                   "ro_RO", LANG_ROMANIAN  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Romanian")
   LNG(wxLANGUAGE_RUSSIAN,                    "ru_RU", LANG_RUSSIAN   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Russian")
   LNG(wxLANGUAGE_RUSSIAN_UKRAINE,            "ru_UA", 0              , 0                                 , wxLayout_LeftToRight, "Russian (Ukraine)")
   LNG(wxLANGUAGE_SAMI,                       "se_NO", LANG_SAMI      , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Northern Sami")
   LNG(wxLANGUAGE_SAMOAN,                     "sm"   , 0              , 0                                 , wxLayout_LeftToRight, "Samoan")
   LNG(wxLANGUAGE_SANGHO,                     "sg"   , 0              , 0                                 , wxLayout_LeftToRight, "Sangho")
   LNG(wxLANGUAGE_SANSKRIT,                   "sa"   , LANG_SANSKRIT  , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Sanskrit")
   LNG(wxLANGUAGE_SCOTS_GAELIC,               "gd"   , 0              , 0                                 , wxLayout_LeftToRight, "Scots Gaelic")
   LNG(wxLANGUAGE_SERBIAN,                    "sr_RS", LANG_SERBIAN   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Serbian")
   LNG(wxLANGUAGE_SERBIAN_CYRILLIC,           "sr_RS", LANG_SERBIAN   , SUBLANG_SERBIAN_CYRILLIC          , wxLayout_LeftToRight, "Serbian (Cyrillic)")
   LNG(wxLANGUAGE_SERBIAN_LATIN,              "sr_RS@latin", LANG_SERBIAN   , SUBLANG_SERBIAN_LATIN             , wxLayout_LeftToRight, "Serbian (Latin)")
   LNG(wxLANGUAGE_SERBIAN_CYRILLIC,           "sr_YU", LANG_SERBIAN   , SUBLANG_SERBIAN_CYRILLIC          , wxLayout_LeftToRight, "Serbian (Cyrillic)")
   LNG(wxLANGUAGE_SERBIAN_LATIN,              "sr_YU@latin", LANG_SERBIAN   , SUBLANG_SERBIAN_LATIN             , wxLayout_LeftToRight, "Serbian (Latin)")
   LNG(wxLANGUAGE_SERBO_CROATIAN,             "sh"   , 0              , 0                                 , wxLayout_LeftToRight, "Serbo-Croatian")
   LNG(wxLANGUAGE_SESOTHO,                    "st"   , 0              , 0                                 , wxLayout_LeftToRight, "Sesotho")
   LNG(wxLANGUAGE_SETSWANA,                   "tn"   , 0              , 0                                 , wxLayout_LeftToRight, "Setswana")
   LNG(wxLANGUAGE_SHONA,                      "sn"   , 0              , 0                                 , wxLayout_LeftToRight, "Shona")
   LNG(wxLANGUAGE_SINDHI,                     "sd"   , LANG_SINDHI    , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Sindhi")
   LNG(wxLANGUAGE_SINHALESE,                  "si"   , 0              , 0                                 , wxLayout_LeftToRight, "Sinhalese")
   LNG(wxLANGUAGE_SISWATI,                    "ss"   , 0              , 0                                 , wxLayout_LeftToRight, "Siswati")
   LNG(wxLANGUAGE_SLOVAK,                     "sk_SK", LANG_SLOVAK    , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Slovak")
   LNG(wxLANGUAGE_SLOVENIAN,                  "sl_SI", LANG_SLOVENIAN , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Slovenian")
   LNG(wxLANGUAGE_SOMALI,                     "so"   , 0              , 0                                 , wxLayout_LeftToRight, "Somali")
   LNG(wxLANGUAGE_SPANISH,                    "es_ES", LANG_SPANISH   , SUBLANG_SPANISH                   , wxLayout_LeftToRight, "Spanish")
   LNG(wxLANGUAGE_SPANISH_ARGENTINA,          "es_AR", LANG_SPANISH   , SUBLANG_SPANISH_ARGENTINA         , wxLayout_LeftToRight, "Spanish (Argentina)")
   LNG(wxLANGUAGE_SPANISH_BOLIVIA,            "es_BO", LANG_SPANISH   , SUBLANG_SPANISH_BOLIVIA           , wxLayout_LeftToRight, "Spanish (Bolivia)")
   LNG(wxLANGUAGE_SPANISH_CHILE,              "es_CL", LANG_SPANISH   , SUBLANG_SPANISH_CHILE             , wxLayout_LeftToRight, "Spanish (Chile)")
   LNG(wxLANGUAGE_SPANISH_COLOMBIA,           "es_CO", LANG_SPANISH   , SUBLANG_SPANISH_COLOMBIA          , wxLayout_LeftToRight, "Spanish (Colombia)")
   LNG(wxLANGUAGE_SPANISH_COSTA_RICA,         "es_CR", LANG_SPANISH   , SUBLANG_SPANISH_COSTA_RICA        , wxLayout_LeftToRight, "Spanish (Costa Rica)")
   LNG(wxLANGUAGE_SPANISH_DOMINICAN_REPUBLIC, "es_DO", LANG_SPANISH   , SUBLANG_SPANISH_DOMINICAN_REPUBLIC, wxLayout_LeftToRight, "Spanish (Dominican republic)")
   LNG(wxLANGUAGE_SPANISH_ECUADOR,            "es_EC", LANG_SPANISH   , SUBLANG_SPANISH_ECUADOR           , wxLayout_LeftToRight, "Spanish (Ecuador)")
   LNG(wxLANGUAGE_SPANISH_EL_SALVADOR,        "es_SV", LANG_SPANISH   , SUBLANG_SPANISH_EL_SALVADOR       , wxLayout_LeftToRight, "Spanish (El Salvador)")
   LNG(wxLANGUAGE_SPANISH_GUATEMALA,          "es_GT", LANG_SPANISH   , SUBLANG_SPANISH_GUATEMALA         , wxLayout_LeftToRight, "Spanish (Guatemala)")
   LNG(wxLANGUAGE_SPANISH_HONDURAS,           "es_HN", LANG_SPANISH   , SUBLANG_SPANISH_HONDURAS          , wxLayout_LeftToRight, "Spanish (Honduras)")
   LNG(wxLANGUAGE_SPANISH_MEXICAN,            "es_MX", LANG_SPANISH   , SUBLANG_SPANISH_MEXICAN           , wxLayout_LeftToRight, "Spanish (Mexican)")
   LNG(wxLANGUAGE_SPANISH_MODERN,             "es_ES", LANG_SPANISH   , SUBLANG_SPANISH_MODERN            , wxLayout_LeftToRight, "Spanish (Modern)")
   LNG(wxLANGUAGE_SPANISH_NICARAGUA,          "es_NI", LANG_SPANISH   , SUBLANG_SPANISH_NICARAGUA         , wxLayout_LeftToRight, "Spanish (Nicaragua)")
   LNG(wxLANGUAGE_SPANISH_PANAMA,             "es_PA", LANG_SPANISH   , SUBLANG_SPANISH_PANAMA            , wxLayout_LeftToRight, "Spanish (Panama)")
   LNG(wxLANGUAGE_SPANISH_PARAGUAY,           "es_PY", LANG_SPANISH   , SUBLANG_SPANISH_PARAGUAY          , wxLayout_LeftToRight, "Spanish (Paraguay)")
   LNG(wxLANGUAGE_SPANISH_PERU,               "es_PE", LANG_SPANISH   , SUBLANG_SPANISH_PERU              , wxLayout_LeftToRight, "Spanish (Peru)")
   LNG(wxLANGUAGE_SPANISH_PUERTO_RICO,        "es_PR", LANG_SPANISH   , SUBLANG_SPANISH_PUERTO_RICO       , wxLayout_LeftToRight, "Spanish (Puerto Rico)")
   LNG(wxLANGUAGE_SPANISH_URUGUAY,            "es_UY", LANG_SPANISH   , SUBLANG_SPANISH_URUGUAY           , wxLayout_LeftToRight, "Spanish (Uruguay)")
   LNG(wxLANGUAGE_SPANISH_US,                 "es_US", 0              , 0                                 , wxLayout_LeftToRight, "Spanish (U.S.)")
   LNG(wxLANGUAGE_SPANISH_VENEZUELA,          "es_VE", LANG_SPANISH   , SUBLANG_SPANISH_VENEZUELA         , wxLayout_LeftToRight, "Spanish (Venezuela)")
   LNG(wxLANGUAGE_SUNDANESE,                  "su"   , 0              , 0                                 , wxLayout_LeftToRight, "Sundanese")
   LNG(wxLANGUAGE_SWAHILI,                    "sw_KE", LANG_SWAHILI   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Swahili")
   LNG(wxLANGUAGE_SWEDISH,                    "sv_SE", LANG_SWEDISH   , SUBLANG_SWEDISH                   , wxLayout_LeftToRight, "Swedish")
   LNG(wxLANGUAGE_SWEDISH_FINLAND,            "sv_FI", LANG_SWEDISH   , SUBLANG_SWEDISH_FINLAND           , wxLayout_LeftToRight, "Swedish (Finland)")
   LNG(wxLANGUAGE_TAGALOG,                    "tl_PH", 0              , 0                                 , wxLayout_LeftToRight, "Tagalog")
   LNG(wxLANGUAGE_TAJIK,                      "tg"   , 0              , 0                                 , wxLayout_LeftToRight, "Tajik")
   LNG(wxLANGUAGE_TAMIL,                      "ta"   , LANG_TAMIL     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Tamil")
   LNG(wxLANGUAGE_TATAR,                      "tt"   , LANG_TATAR     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Tatar")
   LNG(wxLANGUAGE_TELUGU,                     "te"   , LANG_TELUGU    , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Telugu")
   LNG(wxLANGUAGE_THAI,                       "th_TH", LANG_THAI      , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Thai")
   LNG(wxLANGUAGE_TIBETAN,                    "bo"   , 0              , 0                                 , wxLayout_LeftToRight, "Tibetan")
   LNG(wxLANGUAGE_TIGRINYA,                   "ti"   , 0              , 0                                 , wxLayout_LeftToRight, "Tigrinya")
   LNG(wxLANGUAGE_TONGA,                      "to"   , 0              , 0                                 , wxLayout_LeftToRight, "Tonga")
   LNG(wxLANGUAGE_TSONGA,                     "ts"   , 0              , 0                                 , wxLayout_LeftToRight, "Tsonga")
   LNG(wxLANGUAGE_TURKISH,                    "tr_TR", LANG_TURKISH   , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Turkish")
   LNG(wxLANGUAGE_TURKMEN,                    "tk"   , 0              , 0                                 , wxLayout_LeftToRight, "Turkmen")
   LNG(wxLANGUAGE_TWI,                        "tw"   , 0              , 0                                 , wxLayout_LeftToRight, "Twi")
   LNG(wxLANGUAGE_UIGHUR,                     "ug"   , 0              , 0                                 , wxLayout_LeftToRight, "Uighur")
   LNG(wxLANGUAGE_UKRAINIAN,                  "uk_UA", LANG_UKRAINIAN , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Ukrainian")
   LNG(wxLANGUAGE_URDU,                       "ur"   , LANG_URDU      , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Urdu")
   LNG(wxLANGUAGE_URDU_INDIA,                 "ur_IN", LANG_URDU      , SUBLANG_URDU_INDIA                , wxLayout_LeftToRight, "Urdu (India)")
   LNG(wxLANGUAGE_URDU_PAKISTAN,              "ur_PK", LANG_URDU      , SUBLANG_URDU_PAKISTAN             , wxLayout_LeftToRight, "Urdu (Pakistan)")
   LNG(wxLANGUAGE_UZBEK,                      "uz"   , LANG_UZBEK     , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Uzbek")
   LNG(wxLANGUAGE_UZBEK_CYRILLIC,             "uz"   , LANG_UZBEK     , SUBLANG_UZBEK_CYRILLIC            , wxLayout_LeftToRight, "Uzbek (Cyrillic)")
   LNG(wxLANGUAGE_UZBEK_LATIN,                "uz"   , LANG_UZBEK     , SUBLANG_UZBEK_LATIN               , wxLayout_LeftToRight, "Uzbek (Latin)")
   LNG(wxLANGUAGE_VALENCIAN,                  "ca_ES@valencia", LANG_VALENCIAN , SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Valencian")
   LNG(wxLANGUAGE_VIETNAMESE,                 "vi_VN", LANG_VIETNAMESE, SUBLANG_DEFAULT                   , wxLayout_LeftToRight, "Vietnamese")
   LNG(wxLANGUAGE_VOLAPUK,                    "vo"   , 0              , 0                                 , wxLayout_LeftToRight, "Volapuk")
   LNG(wxLANGUAGE_WELSH,                      "cy"   , 0              , 0                                 , wxLayout_LeftToRight, "Welsh")
   LNG(wxLANGUAGE_WOLOF,                      "wo"   , 0              , 0                                 , wxLayout_LeftToRight, "Wolof")
   LNG(wxLANGUAGE_XHOSA,                      "xh"   , 0              , 0                                 , wxLayout_LeftToRight, "Xhosa")
   LNG(wxLANGUAGE_YIDDISH,                    "yi"   , 0              , 0                                 , wxLayout_LeftToRight, "Yiddish")
   LNG(wxLANGUAGE_YORUBA,                     "yo"   , 0              , 0                                 , wxLayout_LeftToRight, "Yoruba")
   LNG(wxLANGUAGE_ZHUANG,                     "za"   , 0              , 0                                 , wxLayout_LeftToRight, "Zhuang")
   LNG(wxLANGUAGE_ZULU,                       "zu"   , 0              , 0                                 , wxLayout_LeftToRight, "Zulu")
}
#undef LNG

// --- --- --- generated code ends here --- --- ---

#endif // wxUSE_INTL
