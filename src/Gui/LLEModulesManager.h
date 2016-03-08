#pragma once

#include "Gui/FrameBase.h"

class LLEModulesManagerFrame : public wxDialog
{
	wxCheckListBox *m_check_list;
	std::vector<std::string> m_funcs;

public:
	LLEModulesManagerFrame(wxWindow *parent);
	void Refresh();
	void UpdateSelection(int index);
	void OnSelectAll(wxCommandEvent& WXUNUSED(event), bool is_checked);
};
