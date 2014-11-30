#pragma once
#include "Gui/FrameBase.h"
#include <wx/checklst.h>

class LLEModulesManagerFrame : public FrameBase
{
	wxCheckListBox *m_check_list;
	std::vector<std::string> m_funcs;

public:
	LLEModulesManagerFrame(wxWindow *parent);
	void Refresh();
	void UpdateSelection(int index);
};