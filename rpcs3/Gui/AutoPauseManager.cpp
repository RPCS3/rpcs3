#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/System.h"
#include "AutoPauseManager.h"

//TODO::Get the enable configuration from ini.
AutoPauseManagerDialog::AutoPauseManagerDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Automatic pause settings")
{
	SetMinSize(wxSize(400, 360));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);

	wxStaticText* s_description = new wxStaticText(this, wxID_ANY, "To use auto pause: enter the ID(s) of a function or a system call. You must restart your emulated game or application for changed settings to take effect. You can enable/disable this in the settings.", wxDefaultPosition, wxDefaultSize, 0);
	s_description->Wrap(400);
	s_main->Add(s_description, 0, wxALL, 5);

	m_list = new wxListView(this);
	m_list->InsertColumn(0, "Call ID");
	m_list->InsertColumn(1, "Type");

	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &AutoPauseManagerDialog::OnEntryConfig, this);
	m_list->Bind(wxEVT_RIGHT_DOWN, &AutoPauseManagerDialog::OnRightClick, this);

	s_main->Add(m_list, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* s_action = new wxBoxSizer(wxHORIZONTAL);

	s_action->Add(new wxButton(this, wxID_CLEAR, "Cl&ear", wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_action->Add(new wxButton(this, wxID_REFRESH, "&Reload", wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_action->Add(new wxButton(this, wxID_SAVE, "&Save", wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_action->Add(new wxButton(this, wxID_CANCEL, "&Close", wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	s_main->Add(s_action, 0, wxALL, 5);

	Bind(wxEVT_MENU, &AutoPauseManagerDialog::OnAdd, this, id_add);
	Bind(wxEVT_MENU, &AutoPauseManagerDialog::OnRemove, this, id_remove);
	Bind(wxEVT_MENU, &AutoPauseManagerDialog::OnEntryConfig, this, id_config);

	Bind(wxEVT_BUTTON, &AutoPauseManagerDialog::OnClear, this, wxID_CLEAR);
	Bind(wxEVT_BUTTON, &AutoPauseManagerDialog::OnReload, this, wxID_REFRESH);
	Bind(wxEVT_BUTTON, &AutoPauseManagerDialog::OnSave, this, wxID_SAVE);

	Emu.Stop();

	LoadEntries();
	UpdateList();

	SetSizerAndFit(s_main);
	Layout();
	Centre(wxBOTH);
}

//Copied some from AutoPause.
void AutoPauseManagerDialog::LoadEntries(void)
{
	m_entries.clear();
	m_entries.reserve(16);

	fs::file list(fs::get_config_dir() + "pause.bin");

	if (list)
	{
		//System calls ID and Function calls ID are all u32 iirc.
		u32 num;
		size_t fmax = list.size();
		size_t fcur = 0;
		list.seek(0);
		while (fcur <= fmax - sizeof(u32))
		{
			list.read(&num, sizeof(u32));
			fcur += sizeof(u32);
			if (num == 0xFFFFFFFF) break;

			m_entries.emplace_back(num);
		}
	}
}

//Copied some from AutoPause.
//Tip: This one doesn't check for the file is being read or not.
//This would always use a 0xFFFFFFFF as end of the pause.bin
void AutoPauseManagerDialog::SaveEntries(void)
{
	fs::file list(fs::get_config_dir() + "pause.bin", fs::rewrite);
	//System calls ID and Function calls ID are all u32 iirc.
	u32 num = 0;
	list.seek(0);
	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		if (num == 0xFFFFFFFF) continue;
		num = m_entries[i];
		list.write(&num, sizeof(u32));
	}
	num = 0xFFFFFFFF;
	list.write(&num, sizeof(u32));
}

void AutoPauseManagerDialog::UpdateList(void)
{
	m_list->Freeze();
	m_list->DeleteAllItems();
	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		m_list->InsertItem(i, i);
		if (m_entries[i] != 0xFFFFFFFF)
		{
			m_list->SetItem(i, 0, fmt::format("%08x", m_entries[i]));
		}
		else
		{
			m_list->SetItem(i, 0, "Unset");
		}
		
		if (m_entries[i] < 1024)
		{
			m_list->SetItem(i, 1, "System call");
		}
		else
		{
			m_list->SetItem(i, 1, "Function call");
		}
	}
	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	m_list->Thaw();
}

void AutoPauseManagerDialog::OnEntryConfig(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if (idx != wxNOT_FOUND)
	{
		AutoPauseSettingsDialog(this, &m_entries[idx]).ShowModal();
		UpdateList();
	}
}

void AutoPauseManagerDialog::OnRightClick(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();
	int idx = m_list->GetFirstSelected();

	menu->Append(id_add, "&Add");
	menu->Append(id_remove, "&Remove")->Enable(idx != wxNOT_FOUND);
	menu->AppendSeparator();
	menu->Append(id_config, "&Config")->Enable(idx != wxNOT_FOUND);

	PopupMenu(menu);
}

void AutoPauseManagerDialog::OnAdd(wxCommandEvent& event)
{
	m_entries.emplace_back(0xFFFFFFFF);
	UpdateList();

	u32 idx = m_entries.size() - 1;
	for (int i = 0; i < m_list->GetItemCount(); ++i)
	{
		m_list->SetItemState(i, i == idx ? wxLIST_STATE_SELECTED : ~wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	wxCommandEvent ce;
	OnEntryConfig(ce);
}

void AutoPauseManagerDialog::OnRemove(wxCommandEvent& event)
{
	for (int sel = m_list->GetNextSelected(-1), offs = 0; sel != wxNOT_FOUND; sel = m_list->GetNextSelected(sel), --offs)
	{
		m_entries.erase(m_entries.begin() + (sel + offs));
	}
	UpdateList();
}

void AutoPauseManagerDialog::OnSave(wxCommandEvent& event)
{
	SaveEntries();
	LOG_SUCCESS(HLE,"Automatic pause: file pause.bin was updated.");
	//event.Skip();
}

void AutoPauseManagerDialog::OnClear(wxCommandEvent& event)
{
	m_entries.clear();
	UpdateList();
}

void AutoPauseManagerDialog::OnReload(wxCommandEvent& event)
{
	LoadEntries();
	UpdateList();
}

AutoPauseSettingsDialog::AutoPauseSettingsDialog(wxWindow* parent, u32 *entry)
	: wxDialog(parent, wxID_ANY, "Automatic pause Setting")
	, m_presult(entry)
{
	m_entry = *m_presult;
	//SetSizeHints(wxSize(400, -1), wxDefaultSize);
	SetMinSize(wxSize(400, -1));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);

	wxStaticText* s_description = new wxStaticText(this, wxID_ANY, "Specify ID of system call or function call below. You need to use a hexadecimal ID.", wxDefaultPosition, wxDefaultSize, 0);
	s_description->Wrap(400);
	s_main->Add(s_description, 0, wxALL, 5);

	wxBoxSizer* s_config = new wxBoxSizer(wxHORIZONTAL);

	m_id = new wxTextCtrl(this, wxID_STATIC, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	s_config->Add(m_id, 1, wxALL | wxEXPAND, 5);
	s_config->Add(new wxButton(this, wxID_OK, "&OK", wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_config->Add(new wxButton(this, wxID_CANCEL, "&Cancel", wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	s_main->Add(s_config, 0, wxEXPAND, 5);

	m_current_converted = new wxStaticText(this, wxID_ANY, "Currently it gets an id of \"Unset\".", wxDefaultPosition, wxDefaultSize, 0);
	s_main->Add(m_current_converted, 0, wxALL, 5);

	m_id->SetValue(fmt::format("%08x", m_entry));

	SetTitle("Automatic pause setting: " + m_id->GetValue());

	Bind(wxEVT_BUTTON, &AutoPauseSettingsDialog::OnOk, this, wxID_OK);
	Bind(wxEVT_TEXT, &AutoPauseSettingsDialog::OnUpdateValue, this, wxID_STATIC);

	SetSizerAndFit(s_main);
	//SetSize(wxSize(400, -1));
	Layout();
	Centre(wxBOTH);

	wxCommandEvent ce;
	OnUpdateValue(ce);
}

void AutoPauseSettingsDialog::OnOk(wxCommandEvent& event)
{
	ullong value = 0;
	m_id->GetValue().ToULongLong(&value, 16);

	m_entry = value;
	*m_presult = m_entry;

	EndModal(wxID_OK);
}

void AutoPauseSettingsDialog::OnUpdateValue(wxCommandEvent& event)
{
	ullong value;
	const bool is_ok = m_id->GetValue().ToULongLong(&value, 16) && value <= UINT32_MAX;

	m_current_converted->SetLabelText(fmt::format("Current value: %08x (%s)", u32(value), is_ok ? "OK" : "conversion failed"));
	event.Skip();
}
