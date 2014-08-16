#pragma once
#include "stdafx.h"
#include <iomanip>
#include <sstream>
#include "Utilities/Log.h"
#include "Utilities/rFile.h"

class AutoPauseManagerDialog : public wxDialog
{
	wxListView *m_list;
	std::vector<u32> m_entries;
	//Used when i want to convert u32 id to string
	std::stringstream m_entry_convert;

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
	u32 *m_presult;
	wxTextCtrl* m_id;
	wxStaticText* m_current_converted;
	std::stringstream m_entry_convert;

public:
	AutoPauseSettingsDialog(wxWindow* parent, u32 *entry);
	void OnOk(wxCommandEvent& event);
	void OnUpdateValue(wxCommandEvent& event);
};