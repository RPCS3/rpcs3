#include "stdafx.h"
#include "Emu/System.h"
#include "AutoPauseManager.h"
#include "stdafx.h"
#include <iomanip>
#include <sstream>
#include "Utilities/Log.h"
#include "Utilities/rFile.h"

enum
{
	id_add,
	id_remove,
	id_config
};

//TODO::Get the enable configuration from ini.
AutoPauseManagerDialog::AutoPauseManagerDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Auto Pause Manager")
{
	SetMinSize(wxSize(400, 360));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);

	m_entry_convert.clear();
	m_entry_convert.str("");
	m_entry_convert << "To use auto pause: enter the ID(s) of a function or a system call. Restart of the game is required to apply. You can enable/disable this in the settings.";

	wxStaticText* s_description = new wxStaticText(this, wxID_ANY, m_entry_convert.str(),wxDefaultPosition, wxDefaultSize, 0);
	s_description->Wrap(400);
	s_main->Add(s_description, 0, wxALL, 5);

	m_list = new wxListView(this);
	m_list->InsertColumn(0, "Call ID");
	m_list->InsertColumn(1, "Type");

	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &AutoPauseManagerDialog::OnEntryConfig, this);
	m_list->Bind(wxEVT_RIGHT_DOWN, &AutoPauseManagerDialog::OnRightClick, this);

	s_main->Add(m_list, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* s_action = new wxBoxSizer(wxHORIZONTAL);

	s_action->Add(new wxButton(this, wxID_CLEAR, wxT("Cl&ear"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_action->Add(new wxButton(this, wxID_REFRESH, wxT("&Reload"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_action->Add(new wxButton(this, wxID_SAVE, wxT("&Save"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_action->Add(new wxButton(this, wxID_CANCEL, wxT("&Close"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

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

	if (rExists("pause.bin"))
	{
		rFile list;
		list.Open("pause.bin", rFile::read);
		//System calls ID and Function calls ID are all u32 iirc.
		u32 num;
		size_t fmax = list.Length();
		size_t fcur = 0;
		list.Seek(0);
		while (fcur <= fmax - sizeof(u32))
		{
			list.Read(&num, sizeof(u32));
			fcur += sizeof(u32);
			if (num == 0xFFFFFFFF) break;

			m_entries.emplace_back(num);
		}
		list.Close();
	}
}

//Copied some from AutoPause.
//Tip: This one doesn't check for the file is being read or not.
//This would always use a 0xFFFFFFFF as end of the pause.bin
void AutoPauseManagerDialog::SaveEntries(void)
{
	if (rExists("pause.bin"))
	{
		rRemoveFile("pause.bin");
	}
	rFile list;
	list.Open("pause.bin", rFile::write);
	//System calls ID and Function calls ID are all u32 iirc.
	u32 num = 0;
	list.Seek(0);
	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		if (num == 0xFFFFFFFF) continue;
		num = m_entries[i];
		list.Write(&num, sizeof(u32));
	}
	num = 0xFFFFFFFF;
	list.Write(&num, sizeof(u32));
	list.Close();
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
			m_entry_convert.clear();
			m_entry_convert.str("");
			m_entry_convert << std::hex << std::setw(8) << std::setfill('0') << m_entries[i];
			m_list->SetItem(i, 0, m_entry_convert.str());
		}
		else
		{
			m_list->SetItem(i, 0, "Unset");
		}
		
		if (m_entries[i] < 1024)
		{
			m_list->SetItem(i, 1, "System Call");
		}
		else
		{
			m_list->SetItem(i, 1, "Function Call");
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
	LOG_SUCCESS(HLE,"Auto Pause: File pause.bin was updated.");
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
	: wxDialog(parent, wxID_ANY, "Auto Pause Setting")
	, m_presult(entry)
{
	m_entry = *m_presult;
	//SetSizeHints(wxSize(400, -1), wxDefaultSize);
	SetMinSize(wxSize(400, -1));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);

	m_entry_convert.clear();
	m_entry_convert.str("");
	m_entry_convert << "Specify ID of System Call or Function Call below. You need to use a Hexadecimal ID.";

	wxStaticText* s_description = new wxStaticText(this, wxID_ANY, m_entry_convert.str(), wxDefaultPosition, wxDefaultSize, 0);
	s_description->Wrap(400);
	s_main->Add(s_description, 0, wxALL, 5);

	wxBoxSizer* s_config = new wxBoxSizer(wxHORIZONTAL);

	m_id = new wxTextCtrl(this, wxID_STATIC, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	s_config->Add(m_id, 1, wxALL | wxEXPAND, 5);
	s_config->Add(new wxButton(this, wxID_OK, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_config->Add(new wxButton(this, wxID_CANCEL, wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	s_main->Add(s_config, 0, wxEXPAND, 5);

	m_current_converted = new wxStaticText(this, wxID_ANY, wxT("Currently it gets an id of \"Unset\"."), wxDefaultPosition, wxDefaultSize, 0);
	s_main->Add(m_current_converted, 0, wxALL, 5);

	m_entry_convert.clear();
	m_entry_convert.str("");
	m_entry_convert << std::hex << std::setw(8) << std::setfill('0') << m_entry;
	m_id->SetValue(m_entry_convert.str());

	SetTitle("Auto Pause Setting: "+m_entry_convert.str());

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
	//Luckily StringStream omit those not hex.
	m_entry_convert.clear();
	m_entry_convert.str("");
	m_entry_convert << std::hex << m_id->GetValue();
	m_entry_convert >> m_entry;

	*m_presult = m_entry;

	EndModal(wxID_OK);
}

void AutoPauseSettingsDialog::OnUpdateValue(wxCommandEvent& event)
{
	//Luckily StringStream omit those not hex.
	m_entry_convert.clear();
	m_entry_convert.str("");
	m_entry_convert << std::hex << m_id->GetValue();
	m_entry_convert >> m_entry;

	m_entry_convert.clear();
	m_entry_convert.str("");
	m_entry_convert << std::hex << /*std::setw(8) << std::setfill('0') <<*/ m_entry;
	m_entry_convert.clear();
	if (m_entry_convert.str() == m_id->GetValue())
	{
		m_current_converted->SetLabelText("Currently it gets an id of \"" + m_entry_convert.str() + "\" - SAME.");
	}
	else {
		m_current_converted->SetLabelText("Currently it gets an id of \"" + m_entry_convert.str() + "\".");
	}
	event.Skip();
}