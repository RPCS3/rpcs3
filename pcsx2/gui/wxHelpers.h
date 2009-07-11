#pragma once

#include <wx/wx.h>
#include <wx/filepicker.h>


namespace wxHelpers
{
	extern wxCheckBox& AddCheckBoxTo( wxWindow* parent, wxBoxSizer& sizer, const wxString& label, wxWindowID id=wxID_ANY );
	extern wxRadioButton& AddRadioButtonTo( wxWindow* parent, wxBoxSizer& sizer, const wxString& label, wxWindowID id=wxID_ANY, bool isFirst = false );
	extern wxStaticText& AddStaticTextTo(wxWindow* parent, wxBoxSizer& sizer, const wxString& label, int size=0, int alignFlags=wxALIGN_LEFT );

	namespace SizerFlags
	{
		extern wxSizerFlags StdSpace();
		extern wxSizerFlags StdCenter();
		extern wxSizerFlags StdExpand();
		extern wxSizerFlags StdGroupie();
		extern wxSizerFlags StdButton();
		extern wxSizerFlags Checkbox();
	};
}

//////////////////////////////////////////////////////////////////////////////////////////
//
class wxDialogWithHelpers : public wxDialog
{
protected:
	bool m_hasContextHelp;

public:
	wxDialogWithHelpers(wxWindow* parent, int id, const wxString& title, bool hasContextHelp, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

protected:
	wxCheckBox& AddCheckBox( wxBoxSizer& sizer, const wxString& label, wxWindowID id=wxID_ANY );
	wxStaticText& AddStaticText(wxBoxSizer& sizer, const wxString& label, int size=0 );
    void AddOkCancel( wxBoxSizer& sizer, bool hasApply=false );
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class wxPanelWithHelpers : public wxPanel
{
protected:
	bool m_StartNewRadioGroup;

public:
	wxPanelWithHelpers( wxWindow* parent, int id, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

protected:
	wxCheckBox& AddCheckBox( wxBoxSizer& sizer, const wxString& label, wxWindowID id=wxID_ANY );
	wxRadioButton& AddRadioButton( wxBoxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, wxWindowID id=wxID_ANY );
	wxStaticText& AddStaticText(wxBoxSizer& sizer, const wxString& label, int size=0, int alignFlags=wxALIGN_LEFT );
	
	void StartRadioGroup()
	{
		m_StartNewRadioGroup = true;
	}
};
