#pragma once

class VFSEntrySettingsDialog : public wxDialog
{
	wxTextCtrl* m_tctrl_dev_path;
	wxButton* m_btn_select_dev_path;
	wxTextCtrl* m_tctrl_path;
	wxButton* m_btn_select_path;
	wxTextCtrl* m_tctrl_mount;
	wxChoice* m_ch_type;
	VFSManagerEntry& m_entry;

public:
	VFSEntrySettingsDialog(wxWindow* parent, VFSManagerEntry& entry);
	void OnSelectType(wxCommandEvent& event);
	void OnSelectPath(wxCommandEvent& event);
	void OnSelectDevPath(wxCommandEvent& event);
	void OnOk(wxCommandEvent& event);
};

class VFSManagerDialog : public wxDialog
{
	wxListView* m_list;
	std::vector<VFSManagerEntry> m_entries;

public:
	VFSManagerDialog(wxWindow* parent);

	void UpdateList();

	void OnEntryConfig(wxCommandEvent& event);
	void OnRightClick(wxCommandEvent& event);
	void OnAdd(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);
	void LoadEntries();
	void SaveEntries();
};