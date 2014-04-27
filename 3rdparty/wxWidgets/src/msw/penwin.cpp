/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/penwin.cpp
// Purpose:     PenWindows code
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: penwin.cpp 37162 2006-01-26 16:50:23Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/window.h"
#endif

#include "wx/msw/private.h"

#if wxUSE_PENWINDOWS

#ifdef __BORLANDC__
#define RPA_DEFAULT 1
#else
#include <penwin.h>
#endif

HANDLE g_hPenWin = (HANDLE)NULL;
typedef void (CALLBACK * PENREGPROC)(WORD,BOOL);

// The routine below allows Windows applications (binaries) to
// support Pen input when running under Microsoft Windows for
// Pen Computing 1.0 without need of the PenPalete.
//
// Should masked edit functions be added to wxWidgets we would
// be a new class of functions to support BEDIT controls.
//
// (The function is a NOOP for native Windows-NT)
#ifndef __WIN32__
static  void (CALLBACK * RegPenApp) (WORD, BOOL) = NULL;
#endif

// Where is this called??
void wxEnablePenAppHooks (bool hook)
{
#ifndef __WIN32__
  if (hook)
    {
      if (g_hPenWin)
      return;

      ///////////////////////////////////////////////////////////////////////
      // If running on a Pen Windows system, register this app so all
      // EDIT controls in dialogs are replaced by HEDIT controls.
      if ((g_hPenWin = (HANDLE)::GetSystemMetrics (SM_PENWINDOWS)) != (HANDLE) NULL)
      {
        // We do this fancy GetProcAddress simply because we don't
        // know if we're running Pen Windows.
        if ((RegPenApp = (PENREGPROC)GetProcAddress (g_hPenWin, "RegisterPenApp")) != NULL)
          (*RegPenApp) (RPA_DEFAULT, TRUE);
      }
    }
  else
    {
      ///////////////////////////////////////////////////////////////////////
      // If running on a Pen Windows system, unregister
      if (g_hPenWin)
      {
        // Unregister this app
        if (RegPenApp != NULL)
          (*RegPenApp) (RPA_DEFAULT, FALSE);
        g_hPenWin = (HANDLE) NULL;
      }
    }
#endif /* ! Windows-NT */
}

#endif
  // End wxUSE_PENWINDOWS

void wxRegisterPenWin(void)
{
#if wxUSE_PENWINDOWS
///////////////////////////////////////////////////////////////////////
// If running on a Pen Windows system, register this app so all
// EDIT controls in dialogs are replaced by HEDIT controls.
// (Notice the CONTROL statement in the RC file is "EDIT",
// RegisterPenApp will automatically change that control to
// an HEDIT.
  if ((g_hPenWin = (HANDLE)::GetSystemMetrics(SM_PENWINDOWS)) != (HANDLE)NULL) {
    // We do this fancy GetProcAddress simply because we don't
    // know if we're running Pen Windows.
   if ( (RegPenApp = (void (CALLBACK *)(WORD, BOOL))GetProcAddress(g_hPenWin, "RegisterPenApp"))!= NULL)
     (*RegPenApp)(RPA_DEFAULT, TRUE);
  }
///////////////////////////////////////////////////////////////////////
#endif
}

void wxCleanUpPenWin(void)
{
#if wxUSE_PENWINDOWS
  if (g_hPenWin) {
    // Unregister this app
    if (RegPenApp != NULL)
      (*RegPenApp)(RPA_DEFAULT, FALSE);
  }
#endif
}
