
#include "PrecompiledHeader.h"

#include <wx/wx.h>
#include "wxHelpers.h"

namespace wxHelpers
{
	wxSizerFlags stdCenteredFlags( wxSizerFlags().Align(wxALIGN_CENTER).DoubleBorder() );
	wxSizerFlags stdSpacingFlags( wxSizerFlags().Border( wxALL, 6 ) );
	wxSizerFlags stdButtonSizerFlags( wxSizerFlags().Align(wxALIGN_RIGHT).Border() );
	wxSizerFlags CheckboxFlags( wxSizerFlags().Border( wxALL, 6 ).Expand() );
	
	void AddCheckBox( wxBoxSizer& sizer, wxWindow* parent, const wxString& label, wxWindowID id )
	{
		sizer.Add( new wxCheckBox( parent, id, label ), CheckboxFlags );
	}
}

void wxDialogWithHelpers::AddCheckBox( wxBoxSizer& sizer, const wxString& label, wxWindowID id )
{
	wxHelpers::AddCheckBox( sizer, this, label, id );
}

wxDialogWithHelpers::wxDialogWithHelpers( wxWindow* parent, int id,  const wxString& title, const wxPoint& pos, const wxSize& size ) :
	wxDialog( parent, id, title, pos, size )
{
}

