/////////////////////////////////////////////////////////////////////////////
// Name:        joystick.h
// Purpose:     wxJoystick class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: joystick.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_JOYSTICK_H_
#define _WX_JOYSTICK_H_

#include "wx/event.h"

class WXDLLIMPEXP_ADV wxJoystick: public wxObject
{
  DECLARE_DYNAMIC_CLASS(wxJoystick)
 public:
  /*
   * Public interface
   */

  wxJoystick(int joystick = wxJOYSTICK1);

  // Attributes
  ////////////////////////////////////////////////////////////////////////////

  wxPoint GetPosition(void) const;
  int GetZPosition(void) const;
  int GetButtonState(void) const;
  int GetPOVPosition(void) const;
  int GetPOVCTSPosition(void) const;
  int GetRudderPosition(void) const;
  int GetUPosition(void) const;
  int GetVPosition(void) const;
  int GetMovementThreshold(void) const;
  void SetMovementThreshold(int threshold) ;

  // Capabilities
  ////////////////////////////////////////////////////////////////////////////

  static int GetNumberJoysticks(void);

  bool IsOk(void) const; // Checks that the joystick is functioning
  int GetManufacturerId(void) const ;
  int GetProductId(void) const ;
  wxString GetProductName(void) const ;
  int GetXMin(void) const;
  int GetYMin(void) const;
  int GetZMin(void) const;
  int GetXMax(void) const;
  int GetYMax(void) const;
  int GetZMax(void) const;
  int GetNumberButtons(void) const;
  int GetNumberAxes(void) const;
  int GetMaxButtons(void) const;
  int GetMaxAxes(void) const;
  int GetPollingMin(void) const;
  int GetPollingMax(void) const;
  int GetRudderMin(void) const;
  int GetRudderMax(void) const;
  int GetUMin(void) const;
  int GetUMax(void) const;
  int GetVMin(void) const;
  int GetVMax(void) const;

  bool HasRudder(void) const;
  bool HasZ(void) const;
  bool HasU(void) const;
  bool HasV(void) const;
  bool HasPOV(void) const;
  bool HasPOV4Dir(void) const;
  bool HasPOVCTS(void) const;

  // Operations
  ////////////////////////////////////////////////////////////////////////////

  // pollingFreq = 0 means that movement events are sent when above the threshold.
  // If pollingFreq > 0, events are received every this many milliseconds.
  bool SetCapture(wxWindow* win, int pollingFreq = 0);
  bool ReleaseCapture(void);

protected:
  int       m_joystick;
};

#endif
    // _WX_JOYSTICK_H_
