#pragma once
#include <wx/listctrl.h>

class MemoryStringSearcher : public wxFrame
{
	wxTextCtrl* t_addr;
	wxBoxSizer* s_panel;
	wxButton* b_search;

public:
	bool exit;
	MemoryStringSearcher(wxWindow* parent);
	~MemoryStringSearcher()
	{
		exit = true;
	}

	void Search(wxCommandEvent& event);
};
