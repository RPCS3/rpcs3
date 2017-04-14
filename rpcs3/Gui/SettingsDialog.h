#pragma once

class SettingsDialog : public wxDialog
{
public:
	SettingsDialog(wxWindow* parent, const std::string& path);

private:
	wxTextCtrl* s_module_search_box;
	wxCheckListBox* chbox_list_core_lle;
	wxRadioBox* rbox_lib_loader;

	void OnModuleListItemToggled(wxCommandEvent& event);
	void OnSearchBoxTextChanged(wxCommandEvent& event);
	void OnLibLoaderToggled(wxCommandEvent& event);
	void EnableModuleList(int selection);
	std::map<std::string, bool> lle_module_list;
};
