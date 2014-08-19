#pragma once

#include <wx/treectrl.h>

class KernelExplorer : public wxFrame
{
	wxTreeCtrl* m_tree;

public:
	KernelExplorer(wxWindow* parent);
	void Update();

	void OnRefresh(wxCommandEvent& WXUNUSED(event));
};
