#include "stdafx.h"
#include "SaveDataUtility.h"

//Cause i can not decide what struct to be used to fill those. Just use no real data now.
//Currently variable info isn't used. it supposed to be a container for the information passed by other.
SaveDataInfoDialog::SaveDataInfoDialog(wxWindow* parent, const SaveDataInformation& info)
	: wxDialog(parent, wxID_ANY, "Save Data Information")
{
	SetMinSize(wxSize(400, 300));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);

	m_list = new wxListView(this);
	m_list->InsertColumn(0, "Name");
	m_list->InsertColumn(1, "Detail");
	s_main->Add(m_list, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* s_actions = new wxBoxSizer(wxHORIZONTAL);
	s_actions->Add(0, 0, 1, wxEXPAND, 5);	//Add a spacer to make Close on the Right-Down corner of this dialog.
	s_actions->Add(new wxButton(this, wxID_CANCEL, wxT("&Close"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);
	s_main->Add(s_actions, 0, wxEXPAND, 5);

	SetSizerAndFit(s_main);
	Layout();
	Centre(wxBOTH);

	SetSize(wxSize(400, 300));

	UpdateData();
}
//This is intended to write the information of save data to wxListView.
//However been not able to decide which data struct i should use, i use static content for this to make it stub.
void SaveDataInfoDialog::UpdateData()
{
	m_list->Freeze();
	m_list->DeleteAllItems();

	m_list->InsertItem(0, 0);
	m_list->SetItem(0, 0, "User ID");
	m_list->SetItem(0, 1, "00000000 (None)");

	m_list->InsertItem(1, 1);
	m_list->SetItem(1, 0, "Game Title");
	m_list->SetItem(1, 1, "Happy with rpcs3 (free)");

	m_list->InsertItem(2, 2);
	m_list->SetItem(2, 0, "Subtitle");
	m_list->SetItem(2, 1, "You devs are great");

	m_list->InsertItem(3, 3);
	m_list->SetItem(3, 0, "Detail");
	m_list->SetItem(3, 1, "Stub it first");

	m_list->InsertItem(4, 4);
	m_list->SetItem(4, 0, "Copy-Able?");
	m_list->SetItem(4, 1, "1 (Not allowed)");

	m_list->InsertItem(5, 5);
	m_list->SetItem(5, 0, "Play Time");
	m_list->SetItem(5, 1, "00:00:00");
	//Maybe there should be more details of save data.
	//But i'm getting bored for assign it one by one.

	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	m_list->Thaw();
}

//This dialog represents the Menu of Save Data Utility - which pop up after when you roll to a save and press triangle.
//I've ever thought of make it a right-click menu or a show-hide panel of the main dialog.
//Well only when those function calls related get implemented we can tell what this GUI should be, seriously.
SaveDataManageDialog::SaveDataManageDialog(wxWindow* parent, unsigned int* sort_type, SaveDataEntry& save)
	: wxDialog(parent, wxID_ANY, "Save Data Pop-up Menu")
{
	SetMinSize(wxSize(400, 110));

	wxBoxSizer* s_manage = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* s_sort = new wxBoxSizer(wxHORIZONTAL);
	s_sort->Add(new wxStaticText(this, wxID_ANY, wxT("Sort By"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL | wxEXPAND, 5);

	m_sort_options = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
	//You might change this - of corse we should know what to been set - maybe after functions related been implemented.
	m_sort_options->Append(wxT("User Id"));
	m_sort_options->Append(wxT("Game Title"));
	m_sort_options->Append(wxT("Game Subtitle"));
	m_sort_options->Append(wxT("Play Time"));
	m_sort_options->Append(wxT("Data Size"));
	m_sort_options->Append(wxT("Last Modified"));
	m_sort_options->Append(wxT("Created Time"));
	m_sort_options->Append(wxT("Accessed Time"));
	m_sort_options->Append(wxT("Modified Time"));
	m_sort_options->Append(wxT("Modify Time"));

	m_sort_type = sort_type;
	if (m_sort_type != nullptr)
	{
		//Check sort type and set it to combo box
		if ((*m_sort_type >= m_sort_options->GetCount())
			|| (*m_sort_type < 0))
		{
			*m_sort_type = 0;
		}
	}
	
	m_sort_options->SetSelection(*m_sort_type);
	s_sort->Add(m_sort_options, 1, wxALL | wxEXPAND, 5);

	wxButton* s_sort_action = new wxButton(this, wxID_ANY, wxT("&Apply!"), wxDefaultPosition, wxDefaultSize, 0);
	s_sort_action->Bind(wxEVT_BUTTON, &SaveDataManageDialog::OnApplySort, this);
	s_sort->Add(s_sort_action, 0, wxALL, 5);
	
	s_manage->Add(s_sort, 1, wxEXPAND, 5);

	wxBoxSizer* s_actions = new wxBoxSizer(wxHORIZONTAL);

	wxButton* s_copy = new wxButton(this, wxID_ANY, wxT("&Copy"), wxDefaultPosition, wxDefaultSize, 0);
	s_copy->Bind(wxEVT_BUTTON, &SaveDataManageDialog::OnCopy, this);
	s_actions->Add(s_copy, 0, wxALL, 5);

	wxButton* s_delete = new wxButton(this, wxID_ANY, wxT("&Delete"), wxDefaultPosition, wxDefaultSize, 0);
	s_delete->Bind(wxEVT_BUTTON, &SaveDataManageDialog::OnDelete, this);
	s_actions->Add(s_delete, 0, wxALL, 5);

	wxButton* s_info = new wxButton(this, wxID_ANY, wxT("&Info"), wxDefaultPosition, wxDefaultSize, 0);
	s_info->Bind(wxEVT_BUTTON, &SaveDataManageDialog::OnInfo, this);
	s_actions->Add(s_info, 0, wxALL, 5);

	s_actions->Add(new wxButton(this, wxID_CANCEL, wxT("&Close"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	s_manage->Add(s_actions, 1, wxEXPAND, 5);

	SetSizerAndFit(s_manage);
	Layout();
	Center(wxBOTH);
}
//Display information about the current selected save data.
//If selected is "New Save Data" or other invalid, this dialog would be initialized with "Info" disabled or not visible.
void SaveDataManageDialog::OnInfo(wxCommandEvent& event)
{
	LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataManageDialog: OnInfo called.");
	SaveDataInformation info;	//It should get a real one for information.. finally
	SaveDataInfoDialog(this, info).ShowModal();
}
//Copy selected save data to another. Might need a dialog but i just leave it as this. Or Modal Dialog.
void SaveDataManageDialog::OnCopy(wxCommandEvent& event)
{
	LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataManageDialog: OnCopy called.");
	event.Skip();
}
//Delete selected save data, need confirm. just a stub now.
void SaveDataManageDialog::OnDelete(wxCommandEvent& event)
{
	LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataManageDialog: OnDelete called.");
	event.Skip();
}
//This should return the sort setting of the save data list. Also not implemented really.
void SaveDataManageDialog::OnApplySort(wxCommandEvent& event)
{	
	*m_sort_type = m_sort_options->GetSelection();
	LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataManageDialog: OnApplySort called. NAME=%s",
		m_sort_options->GetStringSelection().ToStdString().c_str());
	//event.Skip();
}

//Show up the savedata list, either to choose one to save/load or to manage saves.
//I suggest to use function callbacks to give save data list or get save data entry. (Not implemented or stubbed)
SaveDataListDialog::SaveDataListDialog(wxWindow* parent, bool enable_manage)
	: wxDialog(parent, wxID_ANY, "Save Data Utility")
{
	SetMinSize(wxSize(400, 400));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);

	m_entry_convert.clear();
	m_entry_convert.str("");
	m_entry_convert << "This is only a stub now. Don't expect real effect from this."
		<< "Cause related functions hasn't been implemented yet.";
	wxStaticText* s_description = new wxStaticText(this, wxID_ANY, m_entry_convert.str(), wxDefaultPosition, wxDefaultSize, 0);
	s_description->Wrap(400);
	s_main->Add(s_description, 0, wxALL, 5);

	m_list = new wxListView(this);
	m_list->InsertColumn(0, "Game ID");
	m_list->InsertColumn(1, "Save ID");
	m_list->InsertColumn(2, "Detail");

	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SaveDataListDialog::OnEntryInfo, this);
	m_list->Bind(wxEVT_RIGHT_DOWN, &SaveDataListDialog::OnRightClick, this);

	s_main->Add(m_list, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* s_action = new wxBoxSizer(wxHORIZONTAL);

	//If do not need manage, hide it, like just a save data picker.
	if (!enable_manage)
	{
		wxButton *m_select = new wxButton(this, wxID_OK, wxT("&Select"), wxDefaultPosition, wxDefaultSize, 0);
		m_select->Bind(wxEVT_BUTTON, &SaveDataListDialog::OnSelect, this);
		s_action->Add(m_select, 0, wxALL, 5);
		SetTitle("Save Data Chooser");
	}
	else {
		wxButton *m_manage = new wxButton(this, wxID_ANY, wxT("&Manage"), wxDefaultPosition, wxDefaultSize, 0);
		m_manage->Bind(wxEVT_BUTTON, &SaveDataListDialog::OnManage, this);
		s_action->Add(m_manage, 0, wxALL, 5);
	}

	s_action->Add(0, 0, 1, wxEXPAND, 5);

	s_action->Add(new wxButton(this, wxID_CANCEL, wxT("&Close"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	s_main->Add(s_action, 0, wxEXPAND, 5);

	Bind(wxEVT_MENU, &SaveDataListDialog::OnEntryCopy, this, id_copy);
	Bind(wxEVT_MENU, &SaveDataListDialog::OnEntryRemove, this, id_remove);
	Bind(wxEVT_MENU, &SaveDataListDialog::OnEntryInfo, this, id_info);

	SetSizerAndFit(s_main);
	Layout();
	Centre(wxBOTH);

	LoadEntries();
	UpdateList();
}
//After you pick a menu item from the sort sub-menu
void SaveDataListDialog::OnSort(wxCommandEvent& event)
{
	LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataListDialog: OnSort called.");
	int idx = event.GetSelection();
	if ((idx < m_sort_options->GetMenuItemCount())
		&& (idx >= 0))
	{
		m_sort_type = idx;
		LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataListDialog: OnSort called. Type Value:%d",m_sort_type);
	}
}
//Copy a existing save, need to get more arguments. maybe a new dialog.
void SaveDataListDialog::OnEntryCopy(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if (idx != wxNOT_FOUND)
	{
		LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataListDialog: OnEntryCopy called.");
		//Some Operations?
		UpdateList();
	}
}
//Remove a save file, need to be confirmed.
void SaveDataListDialog::OnEntryRemove(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if (idx != wxNOT_FOUND)
	{
		LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataListDialog: OnEntryRemove called.");
		//Some Operations?
		UpdateList();
	}
}
//Display info dialog directly.
void SaveDataListDialog::OnEntryInfo(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if (idx != wxNOT_FOUND)
	{
		LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataListDialog: OnEntryInfo called.");
		SaveDataInformation info;	//Only a stub now.
		SaveDataInfoDialog(this, info).ShowModal();
	}
}
//Display info dialog directly.
void SaveDataListDialog::OnManage(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if (idx != wxNOT_FOUND)
	{
		LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataListDialog: OnManage called.");
		SaveDataEntry save;	//Only a stub now.
		SaveDataManageDialog(this, &m_sort_type, save).ShowModal();
	}
}
//When you press that select button in the Chooser mode.
void SaveDataListDialog::OnSelect(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if (idx != wxNOT_FOUND)
	{
		LOG_WARNING(HLE, "Stub - SaveDataUtility: SaveDataListDialog: OnSelect called.");
		EndModal(wxID_OK);
	}
}
//Pop-up a small context-menu, being a replacement for SaveDataManageDialog
void SaveDataListDialog::OnRightClick(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();
	int idx = m_list->GetFirstSelected();

	//This is also a stub for the sort setting. Ids is set according to their sort-type integer.
	m_sort_options = new wxMenu();
	m_sort_options->Append(0, "UserID");
	m_sort_options->Append(1, "Title");
	m_sort_options->Append(2, "Subtitle");
	//Well you can not use direct index cause you can not get its index through wxCommandEvent
	m_sort_options->Bind(wxEVT_MENU, &SaveDataListDialog::OnSort, this);

	menu->AppendSubMenu(m_sort_options, "&Sort");
	menu->AppendSeparator();
	menu->Append(id_copy, "&Copy")->Enable(idx != wxNOT_FOUND);
	menu->Append(id_remove, "&Remove")->Enable(idx != wxNOT_FOUND);
	menu->AppendSeparator();
	menu->Append(id_info, "&Info");

	PopupMenu(menu);
}
//This is intended to load the save data list from a way. However that is not certain for a stub. Does nothing now.
void SaveDataListDialog::LoadEntries(void)
{

}
//Setup some static items just for display.
void SaveDataListDialog::UpdateList(void)
{
	m_list->Freeze();
	m_list->DeleteAllItems();

	m_list->InsertItem(0, 0);
	m_list->SetItem(0, 0, "TEST00000");
	m_list->SetItem(0, 1, "00");
	m_list->SetItem(0, 2, "Final battle");

	m_list->InsertItem(0, 0);
	m_list->SetItem(0, 0, "XXXX99876");
	m_list->SetItem(0, 1, "30");
	m_list->SetItem(0, 2, "This is a fake game");

	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	m_list->Thaw();
}