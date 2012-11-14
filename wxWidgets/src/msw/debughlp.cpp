/////////////////////////////////////////////////////////////////////////////
// Name:        msw/debughlp.cpp
// Purpose:     various Win32 debug helpers
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-08 (extracted from crashrpt.cpp)
// RCS-ID:      $Id: debughlp.cpp 37037 2006-01-21 16:47:30Z JS $
// Copyright:   (c) 2003-2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/msw/debughlp.h"

#if wxUSE_DBGHELP && wxUSE_DYNLIB_CLASS

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// to prevent recursion which could result from corrupted data we limit
// ourselves to that many levels of embedded fields inside structs
static const unsigned MAX_DUMP_DEPTH = 20;

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

// error message from Init()
static wxString gs_errMsg;

// ============================================================================
// wxDbgHelpDLL implementation
// ============================================================================

// ----------------------------------------------------------------------------
// static members
// ----------------------------------------------------------------------------

#define DEFINE_SYM_FUNCTION(func) wxDbgHelpDLL::func ## _t wxDbgHelpDLL::func = 0

wxDO_FOR_ALL_SYM_FUNCS(DEFINE_SYM_FUNCTION);

#undef DEFINE_SYM_FUNCTION

// ----------------------------------------------------------------------------
// initialization methods
// ----------------------------------------------------------------------------

// load all function we need from the DLL

static bool BindDbgHelpFunctions(const wxDynamicLibrary& dllDbgHelp)
{
    #define LOAD_SYM_FUNCTION(name)                                           \
        wxDbgHelpDLL::name = (wxDbgHelpDLL::name ## _t)                       \
                                dllDbgHelp.GetSymbol(_T(#name));              \
        if ( !wxDbgHelpDLL::name )                                            \
        {                                                                     \
            gs_errMsg += _T("Function ") _T(#name) _T("() not found.\n");     \
            return false;                                                     \
        }

    wxDO_FOR_ALL_SYM_FUNCS(LOAD_SYM_FUNCTION);

    #undef LOAD_SYM_FUNCTION

    return true;
}

// called by Init() if we hadn't done this before
static bool DoInit()
{
    wxDynamicLibrary dllDbgHelp(_T("dbghelp.dll"), wxDL_VERBATIM);
    if ( dllDbgHelp.IsLoaded() )
    {
        if ( BindDbgHelpFunctions(dllDbgHelp) )
        {
            // turn on default options
            DWORD options = wxDbgHelpDLL::SymGetOptions();

            options |= SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_DEBUG;

            wxDbgHelpDLL::SymSetOptions(options);

            dllDbgHelp.Detach();
            return true;
        }

        gs_errMsg += _T("\nPlease update your dbghelp.dll version, ")
                     _T("at least version 5.1 is needed!\n")
                     _T("(if you already have a new version, please ")
                     _T("put it in the same directory where the program is.)\n");
    }
    else // failed to load dbghelp.dll
    {
        gs_errMsg += _T("Please install dbghelp.dll available free of charge ")
                     _T("from Microsoft to get more detailed crash information!");
    }

    gs_errMsg += _T("\nLatest dbghelp.dll is available at ")
                 _T("http://www.microsoft.com/whdc/ddk/debugging/\n");

    return false;
}

/* static */
bool wxDbgHelpDLL::Init()
{
    // this flag is -1 until Init() is called for the first time, then it's set
    // to either false or true depending on whether we could load the functions
    static int s_loaded = -1;

    if ( s_loaded == -1 )
    {
        s_loaded = DoInit();
    }

    return s_loaded != 0;
}

// ----------------------------------------------------------------------------
// error handling
// ----------------------------------------------------------------------------

/* static */
const wxString& wxDbgHelpDLL::GetErrorMessage()
{
    return gs_errMsg;
}

/* static */
void wxDbgHelpDLL::LogError(const wxChar *func)
{
    ::OutputDebugString(wxString::Format(_T("dbghelp: %s() failed: %s\r\n"),
                        func, wxSysErrorMsg(::GetLastError())));
}

// ----------------------------------------------------------------------------
// data dumping
// ----------------------------------------------------------------------------

static inline
bool
DoGetTypeInfo(DWORD64 base, ULONG ti, IMAGEHLP_SYMBOL_TYPE_INFO type, void *rc)
{
    static HANDLE s_hProcess = ::GetCurrentProcess();

    return wxDbgHelpDLL::SymGetTypeInfo
                         (
                            s_hProcess,
                            base,
                            ti,
                            type,
                            rc
                         ) != 0;
}

static inline
bool
DoGetTypeInfo(PSYMBOL_INFO pSym, IMAGEHLP_SYMBOL_TYPE_INFO type, void *rc)
{
    return DoGetTypeInfo(pSym->ModBase, pSym->TypeIndex, type, rc);
}

static inline
wxDbgHelpDLL::BasicType GetBasicType(PSYMBOL_INFO pSym)
{
    wxDbgHelpDLL::BasicType bt;
    return DoGetTypeInfo(pSym, TI_GET_BASETYPE, &bt)
            ? bt
            : wxDbgHelpDLL::BASICTYPE_NOTYPE;
}

/* static */
wxString wxDbgHelpDLL::GetSymbolName(PSYMBOL_INFO pSym)
{
    wxString s;

    WCHAR *pwszTypeName;
    if ( SymGetTypeInfo
         (
            GetCurrentProcess(),
            pSym->ModBase,
            pSym->TypeIndex,
            TI_GET_SYMNAME,
            &pwszTypeName
         ) )
    {
        s = wxConvCurrent->cWC2WX(pwszTypeName);

        ::LocalFree(pwszTypeName);
    }

    return s;
}

/* static */ wxString
wxDbgHelpDLL::DumpBaseType(BasicType bt, DWORD64 length, PVOID pAddress)
{
    if ( !pAddress )
    {
        return _T("null");
    }

    if ( ::IsBadReadPtr(pAddress, length) != 0 )
    {
        return _T("BAD");
    }


    wxString s;
    s.reserve(256);

    if ( length == 1 )
    {
        const BYTE b = *(PBYTE)pAddress;

        if ( bt == BASICTYPE_BOOL )
            s = b ? _T("true") : _T("false");
        else
            s.Printf(_T("%#04x"), b);
    }
    else if ( length == 2 )
    {
        s.Printf(bt == BASICTYPE_UINT ? _T("%#06x") : _T("%d"),
                 *(PWORD)pAddress);
    }
    else if ( length == 4 )
    {
        bool handled = false;

        if ( bt == BASICTYPE_FLOAT )
        {
            s.Printf(_T("%f"), *(PFLOAT)pAddress);

            handled = true;
        }
        else if ( bt == BASICTYPE_CHAR )
        {
            // don't take more than 32 characters of a string
            static const size_t NUM_CHARS = 64;

            const char *pc = *(PSTR *)pAddress;
            if ( ::IsBadStringPtrA(pc, NUM_CHARS) == 0 )
            {
                s += _T('"');
                for ( size_t n = 0; n < NUM_CHARS && *pc; n++, pc++ )
                {
                    s += *pc;
                }
                s += _T('"');

                handled = true;
            }
        }

        if ( !handled )
        {
            // treat just as an opaque DWORD
            s.Printf(_T("%#x"), *(PDWORD)pAddress);
        }
    }
    else if ( length == 8 )
    {
        if ( bt == BASICTYPE_FLOAT )
        {
            s.Printf(_T("%lf"), *(double *)pAddress);
        }
        else // opaque 64 bit value
        {
            s.Printf(_T("%#" wxLongLongFmtSpec _T("x")), *(PDWORD *)pAddress);
        }
    }

    return s;
}

wxString
wxDbgHelpDLL::DumpField(PSYMBOL_INFO pSym, void *pVariable, unsigned level)
{
    wxString s;

    // avoid infinite recursion
    if ( level > MAX_DUMP_DEPTH )
    {
        return s;
    }

    SymbolTag tag = SYMBOL_TAG_NULL;
    if ( !DoGetTypeInfo(pSym, TI_GET_SYMTAG, &tag) )
    {
        return s;
    }

    switch ( tag )
    {
        case SYMBOL_TAG_UDT:
        case SYMBOL_TAG_BASE_CLASS:
            s = DumpUDT(pSym, pVariable, level);
            break;

        case SYMBOL_TAG_DATA:
            if ( !pVariable )
            {
                s = _T("NULL");
            }
            else // valid location
            {
                wxDbgHelpDLL::DataKind kind;
                if ( !DoGetTypeInfo(pSym, TI_GET_DATAKIND, &kind) ||
                        kind != DATA_MEMBER )
                {
                    // maybe it's a static member? we're not interested in them...
                    break;
                }

                // get the offset of the child member, relative to its parent
                DWORD ofs = 0;
                if ( !DoGetTypeInfo(pSym, TI_GET_OFFSET, &ofs) )
                    break;

                pVariable = (void *)((DWORD_PTR)pVariable + ofs);


                // now pass to the type representing the type of this member
                SYMBOL_INFO sym = *pSym;
                if ( !DoGetTypeInfo(pSym, TI_GET_TYPEID, &sym.TypeIndex) )
                    break;

                ULONG64 size;
                DoGetTypeInfo(&sym, TI_GET_LENGTH, &size);

                switch ( DereferenceSymbol(&sym, &pVariable) )
                {
                    case SYMBOL_TAG_BASE_TYPE:
                        {
                            BasicType bt = GetBasicType(&sym);
                            if ( bt )
                            {
                                s = DumpBaseType(bt, size, pVariable);
                            }
                        }
                        break;

                    case SYMBOL_TAG_UDT:
                    case SYMBOL_TAG_BASE_CLASS:
                        s = DumpUDT(&sym, pVariable, level);
                        break;
                }
            }

            if ( !s.empty() )
            {
                s = GetSymbolName(pSym) + _T(" = ") + s;
            }
            break;
    }

    if ( !s.empty() )
    {
        s = wxString(_T('\t'), level + 1) + s + _T('\n');
    }

    return s;
}

/* static */ wxString
wxDbgHelpDLL::DumpUDT(PSYMBOL_INFO pSym, void *pVariable, unsigned level)
{
    wxString s;

    // we have to limit the depth of UDT dumping as otherwise we get in
    // infinite loops trying to dump linked lists... 10 levels seems quite
    // reasonable, full information is in minidump file anyhow
    if ( level > 10 )
        return s;

    s.reserve(512);
    s = GetSymbolName(pSym);

#if !wxUSE_STL
    // special handling for ubiquitous wxString: although the code below works
    // for it as well, it shows the wxStringBase class and takes 4 lines
    // instead of only one as this branch
    if ( s == _T("wxString") )
    {
        wxString *ps = (wxString *)pVariable;

        // we can't just dump wxString directly as it could be corrupted or
        // invalid and it could also be locked for writing (i.e. if we're
        // between GetWriteBuf() and UngetWriteBuf() calls) and assert when we
        // try to access it contents using public methods, so instead use our
        // knowledge of its internals
        const wxChar *p = NULL;
        if ( !::IsBadReadPtr(ps, sizeof(wxString)) )
        {
            p = ps->data();
            wxStringData *data = (wxStringData *)p - 1;
            if ( ::IsBadReadPtr(data, sizeof(wxStringData)) ||
                    ::IsBadReadPtr(p, sizeof(wxChar *)*data->nAllocLength) )
            {
                p = NULL; // don't touch this pointer with 10 feet pole
            }
        }

        s << _T("(\"") << (p ? p : _T("???")) << _T(")\"");
    }
    else // any other UDT
#endif // !wxUSE_STL
    {
        // Determine how many children this type has.
        DWORD dwChildrenCount = 0;
        DoGetTypeInfo(pSym, TI_GET_CHILDRENCOUNT, &dwChildrenCount);

        // Prepare to get an array of "TypeIds", representing each of the children.
        TI_FINDCHILDREN_PARAMS *children = (TI_FINDCHILDREN_PARAMS *)
            malloc(sizeof(TI_FINDCHILDREN_PARAMS) +
                        (dwChildrenCount - 1)*sizeof(ULONG));
        if ( !children )
            return s;

        children->Count = dwChildrenCount;
        children->Start = 0;

        // Get the array of TypeIds, one for each child type
        if ( !DoGetTypeInfo(pSym, TI_FINDCHILDREN, children) )
        {
            free(children);
            return s;
        }

        s << _T(" {\n");

        // Iterate through all children
        SYMBOL_INFO sym;
        wxZeroMemory(sym);
        sym.ModBase = pSym->ModBase;
        for ( unsigned i = 0; i < dwChildrenCount; i++ )
        {
            sym.TypeIndex = children->ChildId[i];

            // children here are in lexicographic sense, i.e. we get all our nested
            // classes and not only our member fields, but we can't get the values
            // for the members of the nested classes, of course!
            DWORD nested;
            if ( DoGetTypeInfo(&sym, TI_GET_NESTED, &nested) && nested )
                continue;

            // avoid infinite recursion: this does seem to happen sometimes with
            // complex typedefs...
            if ( sym.TypeIndex == pSym->TypeIndex )
                continue;

            s += DumpField(&sym, pVariable, level + 1);
        }

        free(children);

        s << wxString(_T('\t'), level + 1) << _T('}');
    }

    return s;
}

/* static */
wxDbgHelpDLL::SymbolTag
wxDbgHelpDLL::DereferenceSymbol(PSYMBOL_INFO pSym, void **ppData)
{
    SymbolTag tag = SYMBOL_TAG_NULL;
    for ( ;; )
    {
        if ( !DoGetTypeInfo(pSym, TI_GET_SYMTAG, &tag) )
            break;

        if ( tag != SYMBOL_TAG_POINTER_TYPE )
            break;

        ULONG tiNew;
        if ( !DoGetTypeInfo(pSym, TI_GET_TYPEID, &tiNew) ||
                tiNew == pSym->TypeIndex )
            break;

        pSym->TypeIndex = tiNew;

        // remove one level of indirection except for the char strings: we want
        // to dump "char *" and not a single "char" for them
        if ( ppData && *ppData && GetBasicType(pSym) != BASICTYPE_CHAR )
        {
            DWORD_PTR *pData = (DWORD_PTR *)*ppData;

            if ( ::IsBadReadPtr(pData, sizeof(DWORD_PTR *)) )
            {
                break;
            }

            *ppData = (void *)*pData;
        }
    }

    return tag;
}

/* static */ wxString
wxDbgHelpDLL::DumpSymbol(PSYMBOL_INFO pSym, void *pVariable)
{
    wxString s;
    SYMBOL_INFO symDeref = *pSym;
    switch ( DereferenceSymbol(&symDeref, &pVariable) )
    {
        case SYMBOL_TAG_UDT:
            // show UDT recursively
            s = DumpUDT(&symDeref, pVariable);
            break;

        case SYMBOL_TAG_BASE_TYPE:
            // variable of simple type, show directly
            BasicType bt = GetBasicType(&symDeref);
            if ( bt )
            {
                s = DumpBaseType(bt, pSym->Size, pVariable);
            }
            break;
    }

    return s;
}

// ----------------------------------------------------------------------------
// debugging helpers
// ----------------------------------------------------------------------------

// this code is very useful when debugging debughlp.dll-related code but
// probably not worth having compiled in normally, please do not remove it!
#if 0 // ndef NDEBUG

static wxString TagString(wxDbgHelpDLL::SymbolTag tag)
{
    static const wxChar *tags[] =
    {
        _T("null"),
        _T("exe"),
        _T("compiland"),
        _T("compiland details"),
        _T("compiland env"),
        _T("function"),
        _T("block"),
        _T("data"),
        _T("annotation"),
        _T("label"),
        _T("public symbol"),
        _T("udt"),
        _T("enum"),
        _T("function type"),
        _T("pointer type"),
        _T("array type"),
        _T("base type"),
        _T("typedef"),
        _T("base class"),
        _T("friend"),
        _T("function arg type"),
        _T("func debug start"),
        _T("func debug end"),
        _T("using namespace"),
        _T("vtable shape"),
        _T("vtable"),
        _T("custom"),
        _T("thunk"),
        _T("custom type"),
        _T("managed type"),
        _T("dimension"),
    };

    wxCOMPILE_TIME_ASSERT( WXSIZEOF(tags) == wxDbgHelpDLL::SYMBOL_TAG_MAX,
                                SymbolTagStringMismatch );

    wxString s;
    if ( tag < WXSIZEOF(tags) )
        s = tags[tag];
    else
        s.Printf(_T("unrecognized tag (%d)"), tag);

    return s;
}

static wxString KindString(wxDbgHelpDLL::DataKind kind)
{
    static const wxChar *kinds[] =
    {
         _T("unknown"),
         _T("local"),
         _T("static local"),
         _T("param"),
         _T("object ptr"),
         _T("file static"),
         _T("global"),
         _T("member"),
         _T("static member"),
         _T("constant"),
    };

    wxCOMPILE_TIME_ASSERT( WXSIZEOF(kinds) == wxDbgHelpDLL::DATA_MAX,
                                DataKindStringMismatch );

    wxString s;
    if ( kind < WXSIZEOF(kinds) )
        s = kinds[kind];
    else
        s.Printf(_T("unrecognized kind (%d)"), kind);

    return s;
}

static wxString UdtKindString(wxDbgHelpDLL::UdtKind kind)
{
    static const wxChar *kinds[] =
    {
         _T("struct"),
         _T("class"),
         _T("union"),
    };

    wxCOMPILE_TIME_ASSERT( WXSIZEOF(kinds) == wxDbgHelpDLL::UDT_MAX,
                                UDTKindStringMismatch );

    wxString s;
    if ( kind < WXSIZEOF(kinds) )
        s = kinds[kind];
    else
        s.Printf(_T("unrecognized UDT (%d)"), kind);

    return s;
}

static wxString TypeString(wxDbgHelpDLL::BasicType bt)
{
    static const wxChar *types[] =
    {
        _T("no type"),
        _T("void"),
        _T("char"),
        _T("wchar"),
        _T(""),
        _T(""),
        _T("int"),
        _T("uint"),
        _T("float"),
        _T("bcd"),
        _T("bool"),
        _T(""),
        _T(""),
        _T("long"),
        _T("ulong"),
        _T(""),
        _T(""),
        _T(""),
        _T(""),
        _T(""),
        _T(""),
        _T(""),
        _T(""),
        _T(""),
        _T(""),
        _T("CURRENCY"),
        _T("DATE"),
        _T("VARIANT"),
        _T("complex"),
        _T("bit"),
        _T("BSTR"),
        _T("HRESULT"),
    };

    wxCOMPILE_TIME_ASSERT( WXSIZEOF(types) == wxDbgHelpDLL::BASICTYPE_MAX,
                                BasicTypeStringMismatch );

    wxString s;
    if ( bt < WXSIZEOF(types) )
        s = types[bt];

    if ( s.empty() )
        s.Printf(_T("unrecognized type (%d)"), bt);

    return s;
}

// this function is meant to be called from under debugger to see the
// proprieties of the given type id
extern "C" void DumpTI(ULONG ti)
{
    SYMBOL_INFO sym = { sizeof(SYMBOL_INFO) };
    sym.ModBase = 0x400000; // it's a constant under Win32
    sym.TypeIndex = ti;

    wxDbgHelpDLL::SymbolTag tag = wxDbgHelpDLL::SYMBOL_TAG_NULL;
    DoGetTypeInfo(&sym, TI_GET_SYMTAG, &tag);
    DoGetTypeInfo(&sym, TI_GET_TYPEID, &ti);

    OutputDebugString(wxString::Format(_T("Type 0x%x: "), sym.TypeIndex));
    wxString name = wxDbgHelpDLL::GetSymbolName(&sym);
    if ( !name.empty() )
    {
        OutputDebugString(wxString::Format(_T("name=\"%s\", "), name.c_str()));
    }

    DWORD nested;
    if ( !DoGetTypeInfo(&sym, TI_GET_NESTED, &nested) )
    {
        nested = FALSE;
    }

    OutputDebugString(wxString::Format(_T("tag=%s%s"),
                      nested ? _T("nested ") : wxEmptyString,
                      TagString(tag).c_str()));
    if ( tag == wxDbgHelpDLL::SYMBOL_TAG_UDT )
    {
        wxDbgHelpDLL::UdtKind udtKind;
        if ( DoGetTypeInfo(&sym, TI_GET_UDTKIND, &udtKind) )
        {
            OutputDebugString(_T(" (") + UdtKindString(udtKind) + _T(')'));
        }
    }

    wxDbgHelpDLL::DataKind kind = wxDbgHelpDLL::DATA_UNKNOWN;
    if ( DoGetTypeInfo(&sym, TI_GET_DATAKIND, &kind) )
    {
        OutputDebugString(wxString::Format(
            _T(", kind=%s"), KindString(kind).c_str()));
        if ( kind == wxDbgHelpDLL::DATA_MEMBER )
        {
            DWORD ofs = 0;
            if ( DoGetTypeInfo(&sym, TI_GET_OFFSET, &ofs) )
            {
                OutputDebugString(wxString::Format(_T(" (ofs=0x%x)"), ofs));
            }
        }
    }

    wxDbgHelpDLL::BasicType bt = GetBasicType(&sym);
    if ( bt )
    {
        OutputDebugString(wxString::Format(_T(", type=%s"),
                                TypeString(bt).c_str()));
    }

    if ( ti != sym.TypeIndex )
    {
        OutputDebugString(wxString::Format(_T(", next ti=0x%x"), ti));
    }

    OutputDebugString(_T("\r\n"));
}

#endif // NDEBUG

#endif // wxUSE_DBGHELP
