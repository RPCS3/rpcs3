/////////////////////////////////////////////////////////////////////////////
// Name:        wx/cppunit.h
// Purpose:     wrapper header for CppUnit headers
// Author:      Vadim Zeitlin
// Created:     15.02.04
// RCS-ID:      $Id: cppunit.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2004 Vadim Zeitlin
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CPPUNIT_H_
#define _WX_CPPUNIT_H_

///////////////////////////////////////////////////////////////////////////////
// using CPPUNIT_TEST() macro results in this warning, disable it as there is
// no other way to get rid of it and it's not very useful anyhow
#ifdef __VISUALC__
    // typedef-name 'foo' used as synonym for class-name 'bar'
    #pragma warning(disable:4097)

    // unreachable code: we don't care about warnings in CppUnit headers
    #pragma warning(disable:4702)

    // 'id': identifier was truncated to 'num' characters in the debug info
    #pragma warning(disable:4786)
#endif // __VISUALC__

#ifdef __BORLANDC__
    #pragma warn -8022
#endif

#ifndef CPPUNIT_STD_NEED_ALLOCATOR
    #define CPPUNIT_STD_NEED_ALLOCATOR 0
#endif

///////////////////////////////////////////////////////////////////////////////
// Set the default format for the errors, which can be used by an IDE to jump
// to the error location. This default gets overridden by the cppunit headers
// for some compilers (e.g. VC++).

#ifndef CPPUNIT_COMPILER_LOCATION_FORMAT
    #define CPPUNIT_COMPILER_LOCATION_FORMAT "%p:%l:"
#endif


///////////////////////////////////////////////////////////////////////////////
// Include all needed cppunit headers.
//

#include "wx/beforestd.h"
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/CompilerOutputter.h>
#include "wx/afterstd.h"


///////////////////////////////////////////////////////////////////////////////
// Set of helpful test macros.
//

// Base macro for wrapping CPPUNIT_TEST macros and so making them conditional
// tests, meaning that the test only get registered and thus run when a given
// runtime condition is true.
// In case the condition is evaluated as false a skip message is logged
// (the message will only be shown in verbose mode).
#define WXTEST_ANY_WITH_CONDITION(suiteName, Condition, testMethod, anyTest) \
    if (Condition) \
        { anyTest; } \
    else \
        wxLogInfo(wxString::Format(wxT("skipping: %s.%s\n  reason: %s equals false\n"), \
                                    wxString(suiteName, wxConvUTF8).c_str(), \
                                    wxString(#testMethod, wxConvUTF8).c_str(), \
                                    wxString(#Condition, wxConvUTF8).c_str()))

// Conditional CPPUNIT_TEST macro.
#define WXTEST_WITH_CONDITION(suiteName, Condition, testMethod) \
    WXTEST_ANY_WITH_CONDITION(suiteName, Condition, testMethod, CPPUNIT_TEST(testMethod))
// Conditional CPPUNIT_TEST_FAIL macro.
#define WXTEST_FAIL_WITH_CONDITION(suiteName, Condition, testMethod) \
    WXTEST_ANY_WITH_CONDITION(suiteName, Condition, testMethod, CPPUNIT_TEST_FAIL(testMethod))

// Use this macro to compare a wxString with a literal string.
#define WX_ASSERT_STR_EQUAL(p, s) CPPUNIT_ASSERT_EQUAL(wxString(p), s)

// Use this macro to compare a size_t with a literal integer
#define WX_ASSERT_SIZET_EQUAL(n, m) CPPUNIT_ASSERT_EQUAL(((size_t)n), m)

// Use this macro to compare the expected time_t value with the result of not
// necessarily time_t type
#define WX_ASSERT_TIME_T_EQUAL(t, n) CPPUNIT_ASSERT_EQUAL((t), (time_t)(n))


///////////////////////////////////////////////////////////////////////////////
// stream inserter for wxString
//

#include "wx/string.h"

inline std::ostream& operator<<(std::ostream& o, const wxString& s)
{
    return o << s.mb_str();
}


///////////////////////////////////////////////////////////////////////////////
// Some more compiler warning tweaking and auto linking.
//

#ifdef __BORLANDC__
    #pragma warn .8022
#endif

#ifdef _MSC_VER
  #pragma warning(default:4702)
#endif // _MSC_VER

// for VC++ automatically link in cppunit library
#ifdef __VISUALC__
  #ifdef NDEBUG
    #pragma comment(lib, "cppunit.lib")
  #else // Debug
    #pragma comment(lib, "cppunitd.lib")
  #endif // Release/Debug
#endif

#endif // _WX_CPPUNIT_H_

