#pragma once

#include <wx/treectrl.h>

class KernelExplorer : public wxDialog
{
	wxTreeCtrl* m_tree;

public:
	KernelExplorer(wxWindow* parent);
	void Update();

	void OnRefresh(wxCommandEvent& WXUNUSED(event));
};
