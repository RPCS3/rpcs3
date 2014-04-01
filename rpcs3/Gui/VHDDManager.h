#pragma once
#include <vector>
#include <wx/dnd.h>
#include "Emu/HDD/HDD.h"

class VHDDListDropTarget : public wxDropTarget
{
	wxListView* m_parent;
	int m_src_indx;

public:
	VHDDListDropTarget(wxListView* parent);
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
	virtual void OnLeave();
	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
	void SetSrcIndx(int src_indx)
	{
		m_src_indx = src_indx;
	}
};

class VHDDExplorer : public wxDialog
{
	std::vector<vfsHDD_Entry> m_entries;
	std::vector<std::string> m_names;
	wxListView* m_list;
	vfsHDD* m_hdd;
	VHDDListDropTarget* m_drop_target;

public:
	VHDDExplorer(wxWindow* parent, const std::string& hdd_path);

	void UpdateList();
	void Import(const std::string& path, const std::string& to);
	void Export(const std::string& path, const std::string& to);

	void OnListDrag(wxListEvent& event);
	void OnDropFiles(wxDropFilesEvent& event);
	void OpenDir(int sel);
	void DClick(wxListEvent& event);
	void OnContextMenu(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnRename(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);
	void OnCreateDir(wxCommandEvent& event);
	void OnCreateFile(wxCommandEvent& event);
	void OnImport(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
};

class VHDDSetInfoDialog : public wxDialog
{
	wxSpinCtrl* m_spin_size;
	wxChoice* m_ch_type;
	wxSpinCtrl* m_spin_block_size;

	u64 m_res_size;
	u64 m_res_block_size;
public:
	VHDDSetInfoDialog(wxWindow* parent);

	void OnOk(wxCommandEvent& event);

	void GetResult(u64& size, u64& block_size);
};

class VHDDManagerDialog : public wxDialog
{
	std::vector<std::string> m_paths;
	wxListView* m_list;

public:
	VHDDManagerDialog(wxWindow* parent);

	void UpdateList();

	void Open(int sel);
	void DClick(wxListEvent& event);
	void AddHDD(wxCommandEvent& event);
	void OnContextMenu(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnRemove(wxCommandEvent& event);
	void OnCreateHDD(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);
	void LoadPaths();
	void SavePaths();
};
