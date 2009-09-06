/////////////////////////////////////////////////////////////////////////////
// Name:        valgen.h
// Purpose:     wxGenericValidator class
// Author:      Kevin Smith
// Modified by:
// Created:     Jan 22 1999
// RCS-ID:
// Copyright:   (c) 1999 Julian Smart (assigned from Kevin)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VALGENH__
#define _WX_VALGENH__

#include "wx/validate.h"

#if wxUSE_VALIDATORS

class WXDLLEXPORT wxGenericValidator: public wxValidator
{
DECLARE_CLASS(wxGenericValidator)
public:
  wxGenericValidator(bool* val);
  wxGenericValidator(int* val);
  wxGenericValidator(wxString* val);
  wxGenericValidator(wxArrayInt* val);
  wxGenericValidator(const wxGenericValidator& copyFrom);

  virtual ~wxGenericValidator(){}

  // Make a clone of this validator (or return NULL) - currently necessary
  // if you're passing a reference to a validator.
  // Another possibility is to always pass a pointer to a new validator
  // (so the calling code can use a copy constructor of the relevant class).
  virtual wxObject *Clone() const { return new wxGenericValidator(*this); }
  bool Copy(const wxGenericValidator& val);

  // Called when the value in the window must be validated.
  // This function can pop up an error message.
  virtual bool Validate(wxWindow * WXUNUSED(parent)) { return true; }

  // Called to transfer data to the window
  virtual bool TransferToWindow();

  // Called to transfer data to the window
  virtual bool TransferFromWindow();

protected:
  void Initialize();

  bool*       m_pBool;
  int*        m_pInt;
  wxString*   m_pString;
  wxArrayInt* m_pArrayInt;

private:
// Cannot use
//  DECLARE_NO_COPY_CLASS(wxGenericValidator)
// because copy constructor is explicitly declared above;
// but no copy assignment operator is defined, so declare
// it private to prevent the compiler from defining it:
    wxGenericValidator& operator=(const wxGenericValidator&);
};

#endif
  // wxUSE_VALIDATORS

#endif
  // _WX_VALGENH__
