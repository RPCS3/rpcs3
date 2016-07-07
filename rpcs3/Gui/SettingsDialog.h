#pragma once

class SettingsDialog : public wxDialog
{
public:
	SettingsDialog(wxWindow* parent);

private:
	wxCheckListBox* chbox_list_core_lle;

	void OnModuleListItemToggled(wxCommandEvent& event);
	void OnSearchBoxTextChanged(wxCommandEvent& event);
	std::map<std::string, bool> lle_module_list;
};
