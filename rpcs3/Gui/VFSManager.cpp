#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/FS/VFS.h"
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

	wxBoxSizer* s_type = new wxBoxSizer(wxHORIZONTAL);
	s_type->Add(m_ch_type, 1, wxEXPAND);

	wxBoxSizer* s_dev_path = new wxBoxSizer(wxHORIZONTAL);
	s_dev_path->Add(m_tctrl_dev_path, 1, wxEXPAND);
	s_dev_path->Add(m_btn_select_dev_path, 0, wxLEFT, 5);

	wxBoxSizer* s_path = new wxBoxSizer(wxHORIZONTAL);
	s_path->Add(m_tctrl_path, 1, wxEXPAND);
	s_path->Add(m_btn_select_path, 0, wxLEFT, 5);

	wxBoxSizer* s_mount = new wxBoxSizer(wxHORIZONTAL);
	s_mount->Add(m_tctrl_mount, 1, wxEXPAND);

	wxBoxSizer* s_btns = new wxBoxSizer(wxHORIZONTAL);
	s_btns->Add(new wxButton(this, wxID_OK));
	s_btns->AddSpacer(30);
	s_btns->Add(new wxButton(this, wxID_CANCEL));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(s_type,  1, wxEXPAND | wxALL, 10);
	s_main->Add(s_dev_path,  1, wxEXPAND | wxALL, 10);
	s_main->Add(s_path,  1, wxEXPAND | wxALL, 10);
	s_main->Add(s_mount, 1, wxEXPAND | wxALL, 10);
	s_main->AddSpacer(10);
	s_main->Add(s_btns,  0, wxALL | wxCENTER, 10);

	SetSizerAndFit(s_main);

	SetSize(350, -1);

	for(const auto i : vfsDeviceTypeNames)
	{
		m_ch_type->Append(i);
	}


	m_ch_type->Bind(wxEVT_CHOICE, &VFSEntrySettingsDialog::OnSelectType, this);
	m_btn_select_path->Bind(wxEVT_BUTTON, &VFSEntrySettingsDialog::OnSelectPath, this);
	m_btn_select_dev_path->Bind(wxEVT_BUTTON, &VFSEntrySettingsDialog::OnSelectDevPath, this);
	Bind(wxEVT_BUTTON,  &VFSEntrySettingsDialog::OnOk, this, wxID_OK);

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
	m_entry.device_path =  m_tctrl_dev_path->GetValue().ToStdString();
	m_entry.path = m_tctrl_path->GetValue().ToStdString();
	m_entry.mount = m_tctrl_mount->GetValue().ToStdString();
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

	wxBoxSizer* s_btns = new wxBoxSizer(wxHORIZONTAL);
	s_btns->Add(new wxButton(this, wxID_OK));
	s_btns->AddSpacer(30);
	s_btns->Add(new wxButton(this, wxID_CANCEL));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(m_list, 1, wxEXPAND);
       s_main->Add(s_btns,  0, wxALL | wxCENTER, 10);

	SetSizerAndFit(s_main);
	SetSize(800, 600);

	m_list->InsertColumn(0, "Path");
	m_list->InsertColumn(1, "Device path");
	m_list->InsertColumn(2, "Path to Device");
	m_list->InsertColumn(3, "Device");

	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &VFSManagerDialog::OnEntryConfig, this);
	m_list->Bind(wxEVT_RIGHT_DOWN, &VFSManagerDialog::OnRightClick, this);
	Bind(wxEVT_MENU,   &VFSManagerDialog::OnAdd, this, id_add);
	Bind(wxEVT_MENU,   &VFSManagerDialog::OnRemove, this, id_remove);
	Bind(wxEVT_MENU,   &VFSManagerDialog::OnEntryConfig, this, id_config);
	Bind(wxEVT_BUTTON, &VFSManagerDialog::OnOK, this, wxID_OK);

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

void VFSManagerDialog::OnRightClick(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();
	int idx = m_list->GetFirstSelected();

	menu->Append(id_add,	"Add");
	menu->Append(id_remove, "Remove")->Enable(idx != wxNOT_FOUND);
	menu->AppendSeparator();
	menu->Append(id_config, "Config")->Enable(idx != wxNOT_FOUND);

	PopupMenu(menu);
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

void VFSManagerDialog::OnOK(wxCommandEvent& event)
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
