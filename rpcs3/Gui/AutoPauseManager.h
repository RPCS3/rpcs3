#pragma once

class AutoPauseManagerDialog : public wxDialog
{
	enum
	{
		id_add,
		id_remove,
		id_config,
	};

	wxListView* m_list;
	std::vector<u32> m_entries;

public:
	AutoPauseManagerDialog(wxWindow* parent);

	void UpdateList(void);

	void OnEntryConfig(wxCommandEvent& event);
	void OnRightClick(wxMouseEvent& event);
	void OnAdd(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);

	void OnClear(wxCommandEvent& event);
	void OnReload(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	
	void LoadEntries(void);
	void SaveEntries(void);
};

class AutoPauseSettingsDialog : public wxDialog
{
	u32 m_entry;
	u32* m_presult;
	wxTextCtrl* m_id;
	wxStaticText* m_current_converted;

public:
	AutoPauseSettingsDialog(wxWindow* parent, u32* entry);
	void OnOk(wxCommandEvent& event);
	void OnUpdateValue(wxCommandEvent& event);
};
