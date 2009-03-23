#pragma once

namespace wxHelpers
{
	// Creates a new checkbox and adds it to the specified sizer/parent combo.
	// Uses the default psacer setting for adding checkboxes.
	extern void AddCheckBox( wxBoxSizer& sizer, wxWindow* parent, const wxString& label, wxWindowID id=wxID_ANY );


	extern wxSizerFlags stdCenteredFlags;
	extern wxSizerFlags stdSpacingFlags;

	// This force-aligns the std button sizer to the right, where (at least) us win32 platform
	// users always expect it to be.  Most likely Mac platforms expect it on the left side
	// just because it's *not* where win32 sticks it.  Too bad!
	extern wxSizerFlags stdButtonSizerFlags;
	
	extern wxSizerFlags CheckboxFlags;
}

class wxDialogWithHelpers : public wxDialog
{
public:
	wxDialogWithHelpers(wxWindow* parent, int id,  const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

	void AddCheckBox( wxBoxSizer& sizer, const wxString& label, wxWindowID id=wxID_ANY );
};
