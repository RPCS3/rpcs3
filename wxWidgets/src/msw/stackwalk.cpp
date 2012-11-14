/////////////////////////////////////////////////////////////////////////////
// Name:        msw/stackwalk.cpp
// Purpose:     wxStackWalker implementation for Win32
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-08
// RCS-ID:      $Id: stackwalk.cpp 61341 2009-07-07 09:35:56Z VZ $
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

#if wxUSE_STACKWALKER

#ifndef WX_PRECOMP
    #include "wx/string.h"
#endif

#include "wx/stackwalk.h"

#include "wx/msw/debughlp.h"

#if wxUSE_DBGHELP

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxStackFrame
// ----------------------------------------------------------------------------

void wxStackFrame::OnGetName()
{
    if ( m_hasName )
        return;

    m_hasName = true;

    // get the name of the function for this stack frame entry
    static const size_t MAX_NAME_LEN = 1024;
    BYTE symbolBuffer[sizeof(SYMBOL_INFO) + MAX_NAME_LEN];
    wxZeroMemory(symbolBuffer);

    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_NAME_LEN;

    DWORD64 symDisplacement = 0;
    if ( !wxDbgHelpDLL::SymFromAddr
                        (
                            ::GetCurrentProcess(),
                            GetSymAddr(),
                            &symDisplacement,
                            pSymbol
                        ) )
    {
        wxDbgHelpDLL::LogError(_T("SymFromAddr"));
        return;
    }

    m_name = wxString::FromAscii(pSymbol->Name);
    m_offset = symDisplacement;
}

void wxStackFrame::OnGetLocation()
{
    if ( m_hasLocation )
        return;

    m_hasLocation = true;

    // get the source line for this stack frame entry
    IMAGEHLP_LINE lineInfo = { sizeof(IMAGEHLP_LINE) };
    DWORD dwLineDisplacement;
    if ( !wxDbgHelpDLL::SymGetLineFromAddr
                        (
                            ::GetCurrentProcess(),
                            GetSymAddr(),
                            &dwLineDisplacement,
                            &lineInfo
                        ) )
    {
        // it is normal that we don't have source info for some symbols,
        // notably all the ones from the system DLLs...
        //wxDbgHelpDLL::LogError(_T("SymGetLineFromAddr"));
        return;
    }

    m_filename = wxString::FromAscii(lineInfo.FileName);
    m_line = lineInfo.LineNumber;
}

bool
wxStackFrame::GetParam(size_t n,
                       wxString *type,
                       wxString *name,
                       wxString *value) const
{
    if ( !DoGetParamCount() )
        ConstCast()->OnGetParam();

    if ( n >= DoGetParamCount() )
        return false;

    if ( type )
        *type = m_paramTypes[n];
    if ( name )
        *name = m_paramNames[n];
    if ( value )
        *value = m_paramValues[n];

    return true;
}

void wxStackFrame::OnParam(PSYMBOL_INFO pSymInfo)
{
    m_paramTypes.Add(wxEmptyString);

    m_paramNames.Add(wxString::FromAscii(pSymInfo->Name));

    // if symbol information is corrupted and we crash, the exception is going
    // to be ignored when we're called from WalkFromException() because of the
    // exception handler there returning EXCEPTION_CONTINUE_EXECUTION, but we'd
    // be left in an inconsistent state, so deal with it explicitly here (even
    // if normally we should never crash, of course...)
#ifdef _CPPUNWIND
    try
#else
    __try
#endif
    {
        // as it is a parameter (and not a global var), it is always offset by
        // the frame address
        DWORD_PTR pValue = m_addrFrame + pSymInfo->Address;
        m_paramValues.Add(wxDbgHelpDLL::DumpSymbol(pSymInfo, (void *)pValue));
    }
#ifdef _CPPUNWIND
    catch ( ... )
#else
    __except ( EXCEPTION_EXECUTE_HANDLER )
#endif
    {
        m_paramValues.Add(wxEmptyString);
    }
}

BOOL CALLBACK
EnumSymbolsProc(PSYMBOL_INFO pSymInfo, ULONG WXUNUSED(SymSize), PVOID data)
{
    wxStackFrame *frame = wx_static_cast(wxStackFrame *, data);

    // we're only interested in parameters
    if ( pSymInfo->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER )
    {
        frame->OnParam(pSymInfo);
    }

    // return true to continue enumeration, false would have stopped it
    return TRUE;
}

void wxStackFrame::OnGetParam()
{
    // use SymSetContext to get just the locals/params for this frame
    IMAGEHLP_STACK_FRAME imagehlpStackFrame;
    wxZeroMemory(imagehlpStackFrame);
    imagehlpStackFrame.InstructionOffset = GetSymAddr();
    if ( !wxDbgHelpDLL::SymSetContext
                        (
                            ::GetCurrentProcess(),
                            &imagehlpStackFrame,
                            0           // unused
                        ) )
    {
        // for symbols from kernel DLL we might not have access to their
        // address, this is not a real error
        if ( ::GetLastError() != ERROR_INVALID_ADDRESS )
        {
            wxDbgHelpDLL::LogError(_T("SymSetContext"));
        }

        return;
    }

    if ( !wxDbgHelpDLL::SymEnumSymbols
                        (
                            ::GetCurrentProcess(),
                            NULL,               // DLL base: use current context
                            NULL,               // no mask, get all symbols
                            EnumSymbolsProc,    // callback
                            this                // data to pass to it
                        ) )
    {
        wxDbgHelpDLL::LogError(_T("SymEnumSymbols"));
    }
}


// ----------------------------------------------------------------------------
// wxStackWalker
// ----------------------------------------------------------------------------

void wxStackWalker::WalkFrom(const CONTEXT *pCtx, size_t skip)
{
    if ( !wxDbgHelpDLL::Init() )
    {
        // don't log a user-visible error message here because the stack trace
        // is only needed for debugging/diagnostics anyhow and we shouldn't
        // confuse the user by complaining that we couldn't generate it
        wxLogDebug(_T("Failed to get stack backtrace: %s"),
                   wxDbgHelpDLL::GetErrorMessage().c_str());
        return;
    }

    // according to MSDN, the first parameter should be just a unique value and
    // not process handle (although the parameter is prototyped as "HANDLE
    // hProcess") and actually it advises to use the process id and not handle
    // for Win9x, but then we need to use the same value in StackWalk() call
    // below which should be a real handle... so this is what we use
    const HANDLE hProcess = ::GetCurrentProcess();

    if ( !wxDbgHelpDLL::SymInitialize
                        (
                            hProcess,
                            NULL,   // use default symbol search path
                            TRUE    // load symbols for all loaded modules
                        ) )
    {
        wxDbgHelpDLL::LogError(_T("SymInitialize"));

        return;
    }

    CONTEXT ctx = *pCtx; // will be modified by StackWalk()

    DWORD dwMachineType;

    // initialize the initial frame: currently we can do it for x86 only
    STACKFRAME sf;
    wxZeroMemory(sf);

#ifdef _M_IX86
    sf.AddrPC.Offset       = ctx.Eip;
    sf.AddrPC.Mode         = AddrModeFlat;
    sf.AddrStack.Offset    = ctx.Esp;
    sf.AddrStack.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset    = ctx.Ebp;
    sf.AddrFrame.Mode      = AddrModeFlat;

    dwMachineType = IMAGE_FILE_MACHINE_I386;
#else
    #error "Need to initialize STACKFRAME on non x86"
#endif // _M_IX86

    // iterate over all stack frames (but stop after 200 to avoid entering
    // infinite loop if the stack is corrupted)
    for ( size_t nLevel = 0; nLevel < 200; nLevel++ )
    {
        // get the next stack frame
        if ( !wxDbgHelpDLL::StackWalk
                            (
                                dwMachineType,
                                hProcess,
                                ::GetCurrentThread(),
                                &sf,
                                &ctx,
                                NULL,       // read memory function (default)
                                wxDbgHelpDLL::SymFunctionTableAccess,
                                wxDbgHelpDLL::SymGetModuleBase,
                                NULL        // address translator for 16 bit
                            ) )
        {
            if ( ::GetLastError() )
                wxDbgHelpDLL::LogError(_T("StackWalk"));

            break;
        }

        // don't show this frame itself in the output
        if ( nLevel >= skip )
        {
            wxStackFrame frame(nLevel - skip,
                               (void *)sf.AddrPC.Offset,
                               sf.AddrFrame.Offset);

            OnStackFrame(frame);
        }
    }

    // this results in crashes inside ntdll.dll when called from
    // exception handler ...
#if 0
    if ( !wxDbgHelpDLL::SymCleanup(hProcess) )
    {
        wxDbgHelpDLL::LogError(_T("SymCleanup"));
    }
#endif
}

void wxStackWalker::WalkFrom(const _EXCEPTION_POINTERS *ep, size_t skip)
{
    WalkFrom(ep->ContextRecord, skip);
}

void wxStackWalker::WalkFromException()
{
    // wxGlobalSEInformation is unavailable if wxUSE_ON_FATAL_EXCEPTION==0
#if wxUSE_ON_FATAL_EXCEPTION
    extern EXCEPTION_POINTERS *wxGlobalSEInformation;

    wxCHECK_RET( wxGlobalSEInformation,
                 _T("wxStackWalker::WalkFromException() can only be called from wxApp::OnFatalException()") );

    // don't skip any frames, the first one is where we crashed
    WalkFrom(wxGlobalSEInformation, 0);
#endif // wxUSE_ON_FATAL_EXCEPTION
}

void wxStackWalker::Walk(size_t skip, size_t WXUNUSED(maxDepth))
{
    // to get a CONTEXT for the current location, simply force an exception and
    // get EXCEPTION_POINTERS from it
    //
    // note:
    //  1. we additionally skip RaiseException() and WalkFromException() frames
    //  2. explicit cast to EXCEPTION_POINTERS is needed with VC7.1 even if it
    //     shouldn't have been according to the docs
    __try
    {
        RaiseException(0x1976, 0, 0, NULL);
    }
    __except( WalkFrom((EXCEPTION_POINTERS *)GetExceptionInformation(),
                       skip + 2), EXCEPTION_CONTINUE_EXECUTION )
    {
        // never executed because of WalkFromException() return value
    }
}

#else // !wxUSE_DBGHELP

// ============================================================================
// stubs
// ============================================================================

// ----------------------------------------------------------------------------
// wxStackFrame
// ----------------------------------------------------------------------------

void wxStackFrame::OnGetName()
{
}

void wxStackFrame::OnGetLocation()
{
}

bool
wxStackFrame::GetParam(size_t WXUNUSED(n),
                       wxString * WXUNUSED(type),
                       wxString * WXUNUSED(name),
                       wxString * WXUNUSED(value)) const
{
    return false;
}

void wxStackFrame::OnParam(_SYMBOL_INFO * WXUNUSED(pSymInfo))
{
}

void wxStackFrame::OnGetParam()
{
}

// ----------------------------------------------------------------------------
// wxStackWalker
// ----------------------------------------------------------------------------

void
wxStackWalker::WalkFrom(const CONTEXT * WXUNUSED(pCtx), size_t WXUNUSED(skip))
{
}

void
wxStackWalker::WalkFrom(const _EXCEPTION_POINTERS * WXUNUSED(ep),
                        size_t WXUNUSED(skip))
{
}

void wxStackWalker::WalkFromException()
{
}

void wxStackWalker::Walk(size_t WXUNUSED(skip), size_t WXUNUSED(maxDepth))
{
}

#endif // wxUSE_DBGHELP/!wxUSE_DBGHELP

#endif // wxUSE_STACKWALKER

