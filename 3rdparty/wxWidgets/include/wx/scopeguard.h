///////////////////////////////////////////////////////////////////////////////
// Name:        wx/scopeguard.h
// Purpose:     declares wxwxScopeGuard and related macros
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.07.2003
// RCS-ID:      $Id: scopeguard.h 44111 2007-01-07 13:28:16Z SN $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

/*
    Acknowledgements: this header is heavily based on (well, almost the exact
    copy of) ScopeGuard.h by Andrei Alexandrescu and Petru Marginean published
    in December 2000 issue of C/C++ Users Journal.
    http://www.cuj.com/documents/cujcexp1812alexandr/
 */

#ifndef _WX_SCOPEGUARD_H_
#define _WX_SCOPEGUARD_H_

#include "wx/defs.h"

#include "wx/except.h"

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

#ifdef __WATCOMC__

// WATCOM-FIXME: C++ of Open Watcom 1.3 doesn't like OnScopeExit() created
// through template so it must be workarounded with dedicated inlined macro.
// For compatibility with Watcom compilers wxPrivate::OnScopeExit must be
// replaced with wxPrivateOnScopeExit but in user code (for everyone who
// doesn't care about OW compatibility) wxPrivate::OnScopeExit still works.

#define wxPrivateOnScopeExit(guard)          \
    {                                        \
        if ( !(guard).WasDismissed() )       \
        {                                    \
            wxTRY                            \
            {                                \
                (guard).Execute();           \
            }                                \
            wxCATCH_ALL(;)                   \
        }                                    \
    }

#define wxPrivateUse(n) wxUnusedVar(n)

#else

#if !defined(__GNUC__) || wxCHECK_GCC_VERSION(2, 95)
// namespace support was first implemented in gcc-2.95,
// so avoid using it for older versions.
namespace wxPrivate
{
#else
#define wxPrivate
#endif
    // in the original implementation this was a member template function of
    // ScopeGuardImplBase but gcc 2.8 which is still used for OS/2 doesn't
    // support member templates and so we must make it global
    template <class ScopeGuardImpl>
    void OnScopeExit(ScopeGuardImpl& guard)
    {
        if ( !guard.WasDismissed() )
        {
            // we're called from ScopeGuardImpl dtor and so we must not throw
            wxTRY
            {
                guard.Execute();
            }
            wxCATCH_ALL(;) // do nothing, just eat the exception
        }
    }

    // just to avoid the warning about unused variables
    template <class T>
    void Use(const T& WXUNUSED(t))
    {
    }
#if !defined(__GNUC__) || wxCHECK_GCC_VERSION(2, 95)
} // namespace wxPrivate
#endif

#define wxPrivateOnScopeExit(n) wxPrivate::OnScopeExit(n)
#define wxPrivateUse(n) wxPrivate::Use(n)

#endif

// ============================================================================
// wxScopeGuard for functions and functors
// ============================================================================

// ----------------------------------------------------------------------------
// wxScopeGuardImplBase: used by wxScopeGuardImpl[0..N] below
// ----------------------------------------------------------------------------

class wxScopeGuardImplBase
{
public:
    wxScopeGuardImplBase() : m_wasDismissed(false) { }

    void Dismiss() const { m_wasDismissed = true; }

    // for OnScopeExit() only (we can't make it friend, unfortunately)!
    bool WasDismissed() const { return m_wasDismissed; }

protected:
    ~wxScopeGuardImplBase() { }

    wxScopeGuardImplBase(const wxScopeGuardImplBase& other)
        : m_wasDismissed(other.m_wasDismissed)
    {
        other.Dismiss();
    }

    // must be mutable for copy ctor to work
    mutable bool m_wasDismissed;

private:
    wxScopeGuardImplBase& operator=(const wxScopeGuardImplBase&);
};

// ----------------------------------------------------------------------------
// wxScopeGuardImpl0: scope guard for actions without parameters
// ----------------------------------------------------------------------------

template <class F>
class wxScopeGuardImpl0 : public wxScopeGuardImplBase
{
public:
    static wxScopeGuardImpl0<F> MakeGuard(F fun)
    {
        return wxScopeGuardImpl0<F>(fun);
    }

    ~wxScopeGuardImpl0() { wxPrivateOnScopeExit(*this); }

    void Execute() { m_fun(); }

protected:
    wxScopeGuardImpl0(F fun) : m_fun(fun) { }

    F m_fun;

    wxScopeGuardImpl0& operator=(const wxScopeGuardImpl0&);
};

template <class F>
inline wxScopeGuardImpl0<F> wxMakeGuard(F fun)
{
    return wxScopeGuardImpl0<F>::MakeGuard(fun);
}

// ----------------------------------------------------------------------------
// wxScopeGuardImpl1: scope guard for actions with 1 parameter
// ----------------------------------------------------------------------------

template <class F, class P1>
class wxScopeGuardImpl1 : public wxScopeGuardImplBase
{
public:
    static wxScopeGuardImpl1<F, P1> MakeGuard(F fun, P1 p1)
    {
        return wxScopeGuardImpl1<F, P1>(fun, p1);
    }

    ~wxScopeGuardImpl1() { wxPrivateOnScopeExit(* this); }

    void Execute() { m_fun(m_p1); }

protected:
    wxScopeGuardImpl1(F fun, P1 p1) : m_fun(fun), m_p1(p1) { }

    F m_fun;
    const P1 m_p1;

    wxScopeGuardImpl1& operator=(const wxScopeGuardImpl1&);
};

template <class F, class P1>
inline wxScopeGuardImpl1<F, P1> wxMakeGuard(F fun, P1 p1)
{
    return wxScopeGuardImpl1<F, P1>::MakeGuard(fun, p1);
}

// ----------------------------------------------------------------------------
// wxScopeGuardImpl2: scope guard for actions with 2 parameters
// ----------------------------------------------------------------------------

template <class F, class P1, class P2>
class wxScopeGuardImpl2 : public wxScopeGuardImplBase
{
public:
    static wxScopeGuardImpl2<F, P1, P2> MakeGuard(F fun, P1 p1, P2 p2)
    {
        return wxScopeGuardImpl2<F, P1, P2>(fun, p1, p2);
    }

    ~wxScopeGuardImpl2() { wxPrivateOnScopeExit(*this); }

    void Execute() { m_fun(m_p1, m_p2); }

protected:
    wxScopeGuardImpl2(F fun, P1 p1, P2 p2) : m_fun(fun), m_p1(p1), m_p2(p2) { }

    F m_fun;
    const P1 m_p1;
    const P2 m_p2;

    wxScopeGuardImpl2& operator=(const wxScopeGuardImpl2&);
};

template <class F, class P1, class P2>
inline wxScopeGuardImpl2<F, P1, P2> wxMakeGuard(F fun, P1 p1, P2 p2)
{
    return wxScopeGuardImpl2<F, P1, P2>::MakeGuard(fun, p1, p2);
}

// ============================================================================
// wxScopeGuards for object methods
// ============================================================================

// ----------------------------------------------------------------------------
// wxObjScopeGuardImpl0
// ----------------------------------------------------------------------------

template <class Obj, class MemFun>
class wxObjScopeGuardImpl0 : public wxScopeGuardImplBase
{
public:
    static wxObjScopeGuardImpl0<Obj, MemFun>
        MakeObjGuard(Obj& obj, MemFun memFun)
    {
        return wxObjScopeGuardImpl0<Obj, MemFun>(obj, memFun);
    }

    ~wxObjScopeGuardImpl0() { wxPrivateOnScopeExit(*this); }

    void Execute() { (m_obj.*m_memfun)(); }

protected:
    wxObjScopeGuardImpl0(Obj& obj, MemFun memFun)
        : m_obj(obj), m_memfun(memFun) { }

    Obj& m_obj;
    MemFun m_memfun;
};

template <class Obj, class MemFun>
inline wxObjScopeGuardImpl0<Obj, MemFun> wxMakeObjGuard(Obj& obj, MemFun memFun)
{
    return wxObjScopeGuardImpl0<Obj, MemFun>::MakeObjGuard(obj, memFun);
}

template <class Obj, class MemFun, class P1>
class wxObjScopeGuardImpl1 : public wxScopeGuardImplBase
{
public:
    static wxObjScopeGuardImpl1<Obj, MemFun, P1>
        MakeObjGuard(Obj& obj, MemFun memFun, P1 p1)
    {
        return wxObjScopeGuardImpl1<Obj, MemFun, P1>(obj, memFun, p1);
    }

    ~wxObjScopeGuardImpl1() { wxPrivateOnScopeExit(*this); }

    void Execute() { (m_obj.*m_memfun)(m_p1); }

protected:
    wxObjScopeGuardImpl1(Obj& obj, MemFun memFun, P1 p1)
        : m_obj(obj), m_memfun(memFun), m_p1(p1) { }

    Obj& m_obj;
    MemFun m_memfun;
    const P1 m_p1;
};

template <class Obj, class MemFun, class P1>
inline wxObjScopeGuardImpl1<Obj, MemFun, P1>
wxMakeObjGuard(Obj& obj, MemFun memFun, P1 p1)
{
    return wxObjScopeGuardImpl1<Obj, MemFun, P1>::MakeObjGuard(obj, memFun, p1);
}

template <class Obj, class MemFun, class P1, class P2>
class wxObjScopeGuardImpl2 : public wxScopeGuardImplBase
{
public:
    static wxObjScopeGuardImpl2<Obj, MemFun, P1, P2>
        MakeObjGuard(Obj& obj, MemFun memFun, P1 p1, P2 p2)
    {
        return wxObjScopeGuardImpl2<Obj, MemFun, P1, P2>(obj, memFun, p1, p2);
    }

    ~wxObjScopeGuardImpl2() { wxPrivateOnScopeExit(*this); }

    void Execute() { (m_obj.*m_memfun)(m_p1, m_p2); }

protected:
    wxObjScopeGuardImpl2(Obj& obj, MemFun memFun, P1 p1, P2 p2)
        : m_obj(obj), m_memfun(memFun), m_p1(p1), m_p2(p2) { }

    Obj& m_obj;
    MemFun m_memfun;
    const P1 m_p1;
    const P2 m_p2;
};

template <class Obj, class MemFun, class P1, class P2>
inline wxObjScopeGuardImpl2<Obj, MemFun, P1, P2>
wxMakeObjGuard(Obj& obj, MemFun memFun, P1 p1, P2 p2)
{
    return wxObjScopeGuardImpl2<Obj, MemFun, P1, P2>::
                                            MakeObjGuard(obj, memFun, p1, p2);
}

// ============================================================================
// public stuff
// ============================================================================

// wxScopeGuard is just a reference, see the explanation in CUJ article
typedef const wxScopeGuardImplBase& wxScopeGuard;

// when an unnamed scope guard is needed, the macros below may be used
//
// NB: the original code has a single (and much nicer) ON_BLOCK_EXIT macro
//     but this results in compiler warnings about unused variables and I
//     didn't find a way to work around this other than by having different
//     macros with different names

#define wxGuardName    wxMAKE_UNIQUE_NAME(scopeGuard)

#define wxON_BLOCK_EXIT0_IMPL(n, f) \
    wxScopeGuard n = wxMakeGuard(f); \
    wxPrivateUse(n)
#define wxON_BLOCK_EXIT0(f) \
    wxON_BLOCK_EXIT0_IMPL(wxGuardName, f)

#define wxON_BLOCK_EXIT_OBJ0_IMPL(n, o, m) \
    wxScopeGuard n = wxMakeObjGuard(o, m); \
    wxPrivateUse(n)
#define wxON_BLOCK_EXIT_OBJ0(o, m) \
    wxON_BLOCK_EXIT_OBJ0_IMPL(wxGuardName, o, &m)

#define wxON_BLOCK_EXIT1_IMPL(n, f, p1) \
    wxScopeGuard n = wxMakeGuard(f, p1); \
    wxPrivateUse(n)
#define wxON_BLOCK_EXIT1(f, p1) \
    wxON_BLOCK_EXIT1_IMPL(wxGuardName, f, p1)

#define wxON_BLOCK_EXIT_OBJ1_IMPL(n, o, m, p1) \
    wxScopeGuard n = wxMakeObjGuard(o, m, p1); \
    wxPrivateUse(n)
#define wxON_BLOCK_EXIT_OBJ1(o, m, p1) \
    wxON_BLOCK_EXIT_OBJ1_IMPL(wxGuardName, o, &m, p1)

#define wxON_BLOCK_EXIT2_IMPL(n, f, p1, p2) \
    wxScopeGuard n = wxMakeGuard(f, p1, p2); \
    wxPrivateUse(n)
#define wxON_BLOCK_EXIT2(f, p1, p2) \
    wxON_BLOCK_EXIT2_IMPL(wxGuardName, f, p1, p2)

#define wxON_BLOCK_EXIT_OBJ2_IMPL(n, o, m, p1, p2) \
    wxScopeGuard n = wxMakeObjGuard(o, m, p1, p2); \
    wxPrivateUse(n)
#define wxON_BLOCK_EXIT_OBJ2(o, m, p1, p2) \
    wxON_BLOCK_EXIT_OBJ2_IMPL(wxGuardName, o, &m, p1, p2)

#endif // _WX_SCOPEGUARD_H_
