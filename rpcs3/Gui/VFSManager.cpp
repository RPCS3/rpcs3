#include "stdafx.h"
#include "VFSManager.h"

VFSEntrySettingsDialog::VFSEntrySettingsDialog(wxWindow* parent, VFSManagerEntry& entry)
	: wxDialog(parent, wxID_ANY, "Mount configuration")
	, m_entry(entry)
{
	m_tctrl_dev_path      = new wxTextCtrl(this, wxID_ANY);
	m_btn_select_dev_path = new wxButton(this, wxID_ANY, "...");
	m_tctrl_path          = new wxTextCtrl(this, wxID_ANY);
	m_btn_select_path     = new wxButton(this, wxID_ANY, "...");
	m_tctrl_mount         = new wxTextCtrl(this, wxID_ANY);
	m_ch_type             = new wxChoice(this, wxID_ANY);

	wxBoxSizer& s_type(*new wxBoxSizer(wxHORIZONTAL));
	s_type.Add(m_ch_type, 1, wxEXPAND);

	wxBoxSizer& s_dev_path(*new wxBoxSizer(wxHORIZONTAL));
	s_dev_path.Add(m_tctrl_dev_path, 1, wxEXPAND);
	s_dev_path.Add(m_btn_select_dev_path, 0, wxLEFT, 5);

	wxBoxSizer& s_path(*new wxBoxSizer(wxHORIZONTAL));
	s_path.Add(m_tctrl_path, 1, wxEXPAND);
	s_path.Add(m_btn_select_path, 0, wxLEFT, 5);

	wxBoxSizer& s_mount(*new wxBoxSizer(wxHORIZONTAL));
	s_mount.Add(m_tctrl_mount, 1, wxEXPAND);

	wxBoxSizer& s_btns(*new wxBoxSizer(wxHORIZONTAL));
	s_btns.Add(new wxButton(this, wxID_OK));
	s_btns.AddSpacer(30);
	s_btns.Add(new wxButton(this, wxID_CANCEL));

	wxBoxSizer& s_main(*new wxBoxSizer(wxVERTICAL));
	s_main.Add(&s_type,  1, wxEXPAND | wxALL, 10);
	s_main.Add(&s_dev_path,  1, wxEXPAND | wxALL, 10);
	s_main.Add(&s_path,  1, wxEXPAND | wxALL, 10);
	s_main.Add(&s_mount, 1, wxEXPAND | wxALL, 10);
	s_main.AddSpacer(10);
	s_main.Add(&s_btns,  0, wxALL | wxCENTER, 10);

	SetSizerAndFit(&s_main);

	SetSize(350, -1);

	for(const auto i : vfsDeviceTypeNames)
	{
		m_ch_type->Append(i);
	}

	Connect(m_ch_type->GetId(),             wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(VFSEntrySettingsDialog::OnSelectType));
	Connect(m_btn_select_path->GetId(),     wxEVT_COMMAND_BUTTON_CLICKED,  wxCommandEventHandler(VFSEntrySettingsDialog::OnSelectPath));
	Connect(m_btn_select_dev_path->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,  wxCommandEventHandler(VFSEntrySettingsDialog::OnSelectDevPath));
	Connect(wxID_OK,                        wxEVT_COMMAND_BUTTON_CLICKED,  wxCommandEventHandler(VFSEntrySettingsDialog::OnOk));

	m_tctrl_dev_path->SetValue(m_entry.device_path);
	m_tctrl_path->SetValue(m_entry.path);
	m_tctrl_mount->SetValue(m_entry.mount);
	m_ch_type->SetSelection(m_entry.device);

	wxCommandEvent ce;
	OnSelectType(ce);
}

void VFSEntrySettingsDialog::OnSelectType(wxCommandEvent& event)
{
	m_btn_select_path->Enable(m_ch_type->GetSelection() == vfsDevice_LocalFile);
	m_tctrl_dev_path->Enable(m_ch_type->GetSelection() != vfsDevice_LocalFile);
	m_btn_select_dev_path->Enable(m_ch_type->GetSelection() != vfsDevice_LocalFile);
}

void VFSEntrySettingsDialog::OnSelectPath(wxCommandEvent& event)
{
	wxDirDialog ctrl(this, "Select path", wxGetCwd());

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	m_tctrl_path->SetValue(ctrl.GetPath());
}

void VFSEntrySettingsDialog::OnSelectDevPath(wxCommandEvent& event)
{
	wxFileDialog ctrl(this, "Select device");

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	m_tctrl_dev_path->SetValue(ctrl.GetPath());
}

void VFSEntrySettingsDialog::OnOk(wxCommandEvent& event)
{
	m_entry.device_path = strdup( m_tctrl_dev_path->GetValue().c_str());
	m_entry.path = strdup(m_tctrl_path->GetValue().c_str());
	m_entry.mount = strdup(m_tctrl_mount->GetValue().c_str());
	m_entry.device = (vfsDeviceType)m_ch_type->GetSelection();

	EndModal(wxID_OK);
}

enum
{
	id_add,
	id_remove,
	id_config,
};

VFSManagerDialog::VFSManagerDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Virtual File System Manager")
{
	m_list = new wxListView(this);

	wxBoxSizer& s_main(*new wxBoxSizer(wxVERTICAL));
	s_main.Add(m_list, 1, wxEXPAND);

	SetSizerAndFit(&s_main);
	SetSize(800, 600);

	m_list->InsertColumn(0, "Path");
	m_list->InsertColumn(1, "Device path");
	m_list->InsertColumn(2, "Path to Device");
	m_list->InsertColumn(3, "Device");

	Connect(m_list->GetId(), wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxCommandEventHandler(VFSManagerDialog::OnEntryConfig));
	Connect(m_list->GetId(), wxEVT_COMMAND_RIGHT_CLICK,         wxCommandEventHandler(VFSManagerDialog::OnRightClick));
	Connect(id_add,          wxEVT_COMMAND_MENU_SELECTED,       wxCommandEventHandler(VFSManagerDialog::OnAdd));
	Connect(id_remove,       wxEVT_COMMAND_MENU_SELECTED,       wxCommandEventHandler(VFSManagerDialog::OnRemove));
	Connect(id_config,       wxEVT_COMMAND_MENU_SELECTED,       wxCommandEventHandler(VFSManagerDialog::OnEntryConfig));
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(VFSManagerDialog::OnClose));

	LoadEntries();
	UpdateList();
}

void VFSManagerDialog::UpdateList()
{
	m_list->Freeze();
	m_list->DeleteAllItems();
	for(size_t i=0; i<m_entries.size(); ++i)
	{
		m_list->InsertItem(i, m_entries[i].mount);
		m_list->SetItem(i, 1, m_entries[i].path);
		m_list->SetItem(i, 2, m_entries[i].device_path);
		m_list->SetItem(i, 3, vfsDeviceTypeNames[m_entries[i].device]);
	}
	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
	m_list->Thaw();
}

void VFSManagerDialog::OnEntryConfig(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if(idx != wxNOT_FOUND)
	{
		VFSEntrySettingsDialog(this, m_entries[idx]).ShowModal();
		UpdateList();
	}
}

void VFSManagerDialog::OnRightClick(wxCommandEvent& event)
{
	wxMenu* menu = new wxMenu();
	int idx = m_list->GetFirstSelected();

	menu->Append(id_add,	"Add");
	menu->Append(id_remove, "Remove")->Enable(idx != wxNOT_FOUND);
	menu->AppendSeparator();
	menu->Append(id_config, "Config")->Enable(idx != wxNOT_FOUND);

	PopupMenu( menu );
}

void VFSManagerDialog::OnAdd(wxCommandEvent& event)
{
	m_entries.emplace_back(VFSManagerEntry());
	UpdateList();

	u32 idx = m_entries.size() - 1;
	for(int i=0; i<m_list->GetItemCount(); ++i)
	{
		m_list->SetItemState(i, i == idx ? wxLIST_STATE_SELECTED : ~wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	wxCommandEvent ce;
	OnEntryConfig(ce);
}

void VFSManagerDialog::OnRemove(wxCommandEvent& event)
{
	for(int sel = m_list->GetNextSelected(-1), offs = 0; sel != wxNOT_FOUND; sel = m_list->GetNextSelected(sel), --offs)
	{
		m_entries.erase(m_entries.begin() + (sel + offs));
	}

	UpdateList();
}

void VFSManagerDialog::OnClose(wxCloseEvent& event)
{
	SaveEntries();
	event.Skip();
}

void VFSManagerDialog::LoadEntries()
{
	m_entries.clear();

	Emu.GetVFS().SaveLoadDevices(m_entries, true);
}

void VFSManagerDialog::SaveEntries()
{
	Emu.GetVFS().SaveLoadDevices(m_entries, false);
}
