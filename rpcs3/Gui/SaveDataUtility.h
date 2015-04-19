#pragma once
#include <sstream>

//TODO: Implement function calls related to Save Data List.
//Those function calls may be needed to use this GUI.
//Currently this is only a stub.

//A stub for the struct sent to SaveDataInfoDialog.
struct SaveDataInformation
{

};
//A stub for the sorting.
enum
{
	SAVE_DATA_LIST_SORT_BY_USERID
};
//A stub for a single entry of save data. used to make a save data list or do management.
struct SaveDataEntry
{

};

enum
{
	//Reserved some Ids for Sort-By Submenu.
	id_copy = 64,
	id_remove,
	id_info
};

//Used to display the information of a savedata.
//Not sure about what information should be displayed.
class SaveDataInfoDialog :public wxDialog
{
	wxListView* m_list;

	void UpdateData();
public:
	SaveDataInfoDialog(wxWindow* parent, const SaveDataInformation& info);
};

//Simple way to show up the sort menu and other operations
//Like what you get when press Triangle on SaveData.
class SaveDataManageDialog :public wxDialog
{
	wxComboBox* m_sort_options;
	unsigned int* m_sort_type;
	
	void OnInfo(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);
	void OnApplySort(wxCommandEvent& event);
public:
	SaveDataManageDialog(wxWindow* parent, unsigned int* sort_type, SaveDataEntry& save);
};

//Display a list of SaveData. Would need to be initialized.
//Can also be used as a Save Data Chooser.
class SaveDataListDialog : public wxDialog
{
	wxListView* m_list;
	wxMenu* m_sort_options;
	unsigned int m_sort_type;
	std::stringstream m_entry_convert;

	void OnSelect(wxCommandEvent& event);
	void OnManage(wxCommandEvent& event);

	void OnRightClick(wxMouseEvent& event);
	void OnSort(wxCommandEvent& event);
	void OnEntryCopy(wxCommandEvent& event);
	void OnEntryRemove(wxCommandEvent& event);
	void OnEntryInfo(wxCommandEvent& event);

	void LoadEntries(void);
	void UpdateList(void);
public:
	SaveDataListDialog(wxWindow* parent, bool enable_manage);
};
