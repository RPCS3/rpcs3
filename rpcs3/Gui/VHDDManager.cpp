#include "stdafx.h"
#include "Utilities/Log.h"
#include "VHDDManager.h"
#include "TextInputDialog.h"
#include <wx/busyinfo.h>

VHDDListDropTarget::VHDDListDropTarget(wxListView* parent) : m_parent(parent)
{
	SetDataObject(new wxDataObjectSimple(wxDF_PRIVATE));
}

wxDragResult VHDDListDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
	int flags = 0;
	int indx = m_parent->HitTest(wxPoint(x, y), flags);
	for(int i=0; i<m_parent->GetItemCount(); ++i)
	{
		m_parent->SetItemState(i, i == indx ? wxLIST_STATE_SELECTED : ~wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	return def;
}

void VHDDListDropTarget::OnLeave()
{
	for(int i=0; i<m_parent->GetItemCount(); ++i)
	{
		m_parent->SetItemState(i, ~wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}
}

wxDragResult VHDDListDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{	
	int flags = 0;
	int dst_indx = m_parent->HitTest(wxPoint(x, y), flags);
	LOG_NOTICE(HLE, "OnData(%d -> %d)", m_src_indx, dst_indx);
	return def;
}

enum
{
	id_open = 0x777,
	id_rename,
	id_remove,
	id_create_dir,
	id_create_file,
	id_import,
	id_export,
	id_create_hdd,
	id_add_hdd
};

VHDDExplorer::VHDDExplorer(wxWindow* parent, const std::string& hdd_path) : wxDialog(parent, wxID_ANY, "Virtual HDD Explorer", wxDefaultPosition)
{
	m_list = new wxListView(this);
	m_drop_target = new VHDDListDropTarget(m_list);

	m_list->SetDropTarget(m_drop_target);
	m_list->DragAcceptFiles(true);

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(m_list, 1, wxEXPAND | wxALL, 5);
	SetSizerAndFit(s_main);

	SetSize(800, 600);
	m_list->InsertColumn(0, "Name");
	m_list->InsertColumn(1, "Type");
	m_list->InsertColumn(2, "Size");
	m_list->InsertColumn(3, "Creation time");

	m_hdd = new vfsHDD(nullptr, hdd_path);
	UpdateList();
	m_list->Bind(wxEVT_LIST_BEGIN_DRAG,     &VHDDExplorer::OnListDrag, this);
	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &VHDDExplorer::DClick, this);
	m_list->Bind(wxEVT_RIGHT_DOWN,          &VHDDExplorer::OnContextMenu, this);
	m_list->Bind(wxEVT_DROP_FILES,          &VHDDExplorer::OnDropFiles, this);

	Bind(wxEVT_MENU, &VHDDExplorer::OnOpen, this, id_open);
	Bind(wxEVT_MENU, &VHDDExplorer::OnRename, this, id_rename);
	Bind(wxEVT_MENU, &VHDDExplorer::OnRemove, this, id_remove);
	Bind(wxEVT_MENU, &VHDDExplorer::OnCreateDir, this, id_create_dir);
	Bind(wxEVT_MENU, &VHDDExplorer::OnCreateFile, this, id_create_file);
	Bind(wxEVT_MENU, &VHDDExplorer::OnImport, this, id_import);
	Bind(wxEVT_MENU, &VHDDExplorer::OnExport, this, id_export);
}

void VHDDExplorer::UpdateList()
{
	m_list->Freeze();
	m_list->DeleteAllItems();
	m_entries.clear();
	m_names.clear();

	u64 block;
	vfsHDD_Entry entry;
	std::string name;

	for(bool is_ok = m_hdd->GetFirstEntry(block, entry, name); is_ok; is_ok = m_hdd->GetNextEntry(block, entry, name))
	{
		int item = m_list->GetItemCount();
		m_list->InsertItem(item, fmt::FromUTF8(name));
		m_list->SetItem(item, 1, entry.type == vfsHDD_Entry_Dir ? "Dir" : "File");
		m_list->SetItem(item, 2, wxString::Format("%lld", entry.size));
		m_list->SetItem(item, 3, wxDateTime().Set(time_t(entry.ctime)).Format());
		m_entries.push_back(entry);
		m_names.push_back(name);
	}

	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
	m_list->Thaw();
}

void VHDDExplorer::Import(const std::string& path, const std::string& to)
{
	if(!m_hdd->Create(vfsHDD_Entry_File, to))
	{
		wxMessageBox("IMPORT ERROR: file creation error.");
		return;
	}

	if(!m_hdd->Open(to, vfsWrite))
	{
		wxMessageBox("IMPORT ERROR: file open error.");
		return;
	}

	wxFile f(fmt::FromUTF8(path));
	char buf[256];

	while(!f.Eof())
	{
		m_hdd->Write(buf, f.Read(buf, 256));
	}

	m_hdd->Close();
}

void VHDDExplorer::Export(const std::string& path, const std::string& to)
{
	if(!m_hdd->Open(path))
	{
		wxMessageBox(wxString::Format("EXPORT ERROR: file open error. (%s)", path.c_str()));
		return;
	}

	wxFile f(fmt::FromUTF8(to), wxFile::write);
	char buf[256];

	while(u64 size = m_hdd->Read(buf, 256))
	{
		f.Write(buf, size);
	}
}

void VHDDExplorer::OpenDir(int sel)
{
	if(sel == wxNOT_FOUND)
	{
		return;
	}

	if(m_entries[sel].type == vfsHDD_Entry_Dir)
	{
		m_hdd->OpenDir(m_names[sel]);
		UpdateList();
	}
}

void VHDDExplorer::OnListDrag(wxListEvent& event)
{
	m_drop_target->SetSrcIndx(event.GetIndex());
	wxDataObjectSimple obj(wxDF_PRIVATE);
	wxDropSource drag(obj, m_list);
	drag.DoDragDrop(wxDrag_AllowMove);
}

void VHDDExplorer::OnDropFiles(wxDropFilesEvent& event)
{
	int count = event.GetNumberOfFiles();
	wxString* dropped = event.GetFiles();

	wxBusyCursor busyCursor;
	wxWindowDisabler disabler;
	wxBusyInfo busyInfo("Adding files, wait please...");


	for(int i=0; i<count; ++i)
	{
		LOG_NOTICE(HLE, "Importing '%s'", dropped[i].wx_str());
		Import(fmt::ToUTF8(dropped[i]), fmt::ToUTF8(wxFileName(dropped[i]).GetFullName()));
	}

	UpdateList();
}

void VHDDExplorer::DClick(wxListEvent& event)
{
	OpenDir(event.GetIndex());
}

void VHDDExplorer::OnContextMenu(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();
	int idx = m_list->GetFirstSelected();

	menu->Append(id_open, "Open")->Enable(idx != wxNOT_FOUND && m_entries[idx].type == vfsHDD_Entry_Dir);
	menu->Append(id_rename, "Rename")->Enable(idx != wxNOT_FOUND && m_names[idx] != "." && m_names[idx] != "..");
	menu->Append(id_remove, "Remove")->Enable(idx != wxNOT_FOUND && m_names[idx] != "." && m_names[idx] != "..");
	menu->AppendSeparator();
	menu->Append(id_create_dir, "Create dir");
	menu->Append(id_create_file, "Create file");
	menu->AppendSeparator();
	menu->Append(id_import, "Import");
	menu->Append(id_export, "Export")->Enable(idx != wxNOT_FOUND);

	PopupMenu(menu);
}

void VHDDExplorer::OnOpen(wxCommandEvent& event)
{
	m_hdd->OpenDir(m_names[m_list->GetFirstSelected()]);
	UpdateList();
}

void VHDDExplorer::OnRename(wxCommandEvent& event)
{
	TextInputDialog dial(this, m_names[m_list->GetFirstSelected()]);
	
	if(dial.ShowModal() == wxID_OK)
	{
		m_hdd->Rename(m_names[m_list->GetFirstSelected()], dial.GetResult());
		UpdateList();
	}
}

void VHDDExplorer::OnRemove(wxCommandEvent& event)
{
	for(int sel = m_list->GetNextSelected(-1); sel != wxNOT_FOUND; sel = m_list->GetNextSelected(sel))
	{
		m_hdd->RemoveEntry(m_names[sel]);
	}

	UpdateList();
}

void VHDDExplorer::OnCreateDir(wxCommandEvent& event)
{
	int i = 1;
	static const std::string& fmt = "New Dir (%d)";
	while(m_hdd->HasEntry(fmt::Format(fmt, i))) i++;

	m_hdd->Create(vfsHDD_Entry_Dir, fmt::Format(fmt, i));
	UpdateList();
}

void VHDDExplorer::OnCreateFile(wxCommandEvent& event)
{
	int i = 1;
	static const std::string& fmt = "New File (%d)";
	while (m_hdd->HasEntry(fmt::Format(fmt, i))) i++;

	m_hdd->Create(vfsHDD_Entry_File, fmt::Format(fmt, i));
	UpdateList();
}

void VHDDExplorer::OnImport(wxCommandEvent& event)
{
	wxFileDialog ctrl(this, "Select import files", wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr,
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	wxArrayString paths;
	ctrl.GetPaths(paths);
	for(size_t i=0; i<paths.GetCount(); ++i)
	{
		if(wxFileExists(paths[i]))
		{
			Import(fmt::ToUTF8(paths[i]), fmt::ToUTF8(wxFileName(paths[i]).GetFullName()));
		}
	}
	UpdateList();
}

void VHDDExplorer::OnExport(wxCommandEvent& event)
{
	if(m_list->GetSelectedItemCount() > 1)
	{
		wxDirDialog ctrl(this, "Select export folder", wxGetCwd());

		if(ctrl.ShowModal() == wxID_CANCEL)
		{
			return;
		}

		for(int sel = m_list->GetNextSelected(-1); sel != wxNOT_FOUND; sel = m_list->GetNextSelected(sel))
		{
			Export(m_names[sel], fmt::ToUTF8(ctrl.GetPath()) + '\\' + m_names[sel]);
		}
	}
	else
	{
		int sel = m_list->GetFirstSelected();
		wxFileDialog ctrl(this, "Select export file", wxEmptyString, fmt::FromUTF8(m_names[sel]), wxFileSelectorDefaultWildcardStr, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		if(ctrl.ShowModal() == wxID_CANCEL)
		{
			return;
		}

		Export(m_names[sel], fmt::ToUTF8(ctrl.GetPath()));
	}

	UpdateList();
}

VHDDSetInfoDialog::VHDDSetInfoDialog(wxWindow* parent) : wxDialog(parent, wxID_ANY, "HDD Settings")
{
	m_spin_size = new wxSpinCtrl(this);
	m_ch_type = new wxChoice(this, wxID_ANY);
	m_spin_block_size = new wxSpinCtrl(this);

	wxBoxSizer* s_sinf = new wxBoxSizer(wxHORIZONTAL);
	s_sinf->Add(m_spin_size, wxSizerFlags().Border(wxALL, 5).Expand());
	s_sinf->Add(m_ch_type, wxSizerFlags().Border(wxALL, 5).Expand());

	wxBoxSizer* s_binf = new wxBoxSizer(wxHORIZONTAL);
	s_binf->Add(m_spin_block_size, wxSizerFlags().Border(wxALL, 5).Expand());

	wxBoxSizer* s_btns = new wxBoxSizer(wxHORIZONTAL);
	s_btns->Add(new wxButton(this, wxID_OK), wxSizerFlags().Align(wxALIGN_LEFT).Border(wxALL, 5));
	s_btns->Add(new wxButton(this, wxID_CANCEL), wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxALL, 5));

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(s_sinf, wxSizerFlags().Align(wxALIGN_TOP).Expand());
	s_main->Add(s_binf, wxSizerFlags().Align(wxALIGN_TOP).Expand());
	s_main->Add(s_btns, wxSizerFlags().Align(wxALIGN_BOTTOM).Expand());
	SetSizerAndFit(s_main);

	m_ch_type->Append("B");
	m_ch_type->Append("KB");
	m_ch_type->Append("MB");
	m_ch_type->Append("GB");

	m_spin_size->SetRange(1, 0x7fffffff);
	m_spin_size->SetValue(64);
	m_ch_type->SetSelection(3);
	m_spin_block_size->SetRange(64, 0x7fffffff);
	m_spin_block_size->SetValue(2048);

	Bind(wxEVT_BUTTON, &VHDDSetInfoDialog::OnOk, this, wxID_OK);
}

void VHDDSetInfoDialog::OnOk(wxCommandEvent& event)
{
	m_res_size = m_spin_size->GetValue();

	for(int i=0; i<m_ch_type->GetSelection(); ++i)
	{
		m_res_size *= 1024;
	}

	m_res_block_size = m_spin_block_size->GetValue();

	EndModal(wxID_OK);
}

void VHDDSetInfoDialog::GetResult(u64& size, u64& block_size)
{
	size = m_res_size;
	block_size = m_res_block_size;
}

VHDDManagerDialog::VHDDManagerDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Virtual HDD Manager")
{
	m_list = new wxListView(this);

	wxBoxSizer* s_main = new wxBoxSizer(wxVERTICAL);
	s_main->Add(m_list, 1, wxEXPAND | wxALL, 5);

	SetSizerAndFit(s_main);
	SetSize(800, 600);

	m_list->InsertColumn(0, "Path");
	//m_list->InsertColumn(1, "Size");
	//m_list->InsertColumn(2, "Block size");

	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &VHDDManagerDialog::DClick, this);
	m_list->Bind(wxEVT_RIGHT_DOWN, &VHDDManagerDialog::OnContextMenu, this);

	Bind(wxEVT_MENU, &VHDDManagerDialog::AddHDD, this, id_add_hdd);
	Bind(wxEVT_MENU, &VHDDManagerDialog::OnOpen, this, id_open);
	Bind(wxEVT_MENU, &VHDDManagerDialog::OnRemove, this, id_remove);
	Bind(wxEVT_MENU, &VHDDManagerDialog::OnCreateHDD, this, id_create_hdd);
	Bind(wxEVT_CLOSE_WINDOW, &VHDDManagerDialog::OnClose, this);
	LoadPaths();
	UpdateList();
}

void VHDDManagerDialog::UpdateList()
{
	m_list->Freeze();
	m_list->DeleteAllItems();

	for(size_t i=0; i<m_paths.size(); ++i)
	{
		m_list->InsertItem(i, m_paths[i]);
	}

	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	//m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	//m_list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);

	m_list->Thaw();
}

void VHDDManagerDialog::Open(int sel)
{
	VHDDExplorer dial(this, m_paths[sel]);
	dial.ShowModal();
}

void VHDDManagerDialog::DClick(wxListEvent& event)
{
	Open(event.GetIndex());
}

void VHDDManagerDialog::AddHDD(wxCommandEvent& event)
{
	wxFileDialog ctrl(this, "Select HDDs", wxEmptyString, wxEmptyString, "Virtual HDD (*.hdd) | *.hdd",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	wxArrayString paths;
	ctrl.GetPaths(paths);
	for(size_t i=0; i<paths.GetCount(); ++i)
	{
		bool skip = false;
		for(size_t j=0; j<m_paths.size(); ++j)
		{
			if(fmt::FromUTF8(m_paths[j]).CmpNoCase(paths[i]) == 0)
			{
				skip = true;
				break;
			}
		}
		
		if(!skip)
		{
			m_paths.emplace_back(fmt::ToUTF8(paths[i]));
		}
	}
	UpdateList();
}

void VHDDManagerDialog::OnContextMenu(wxMouseEvent& event)
{
	wxMenu* menu = new wxMenu();
	int idx = m_list->GetFirstSelected();
	menu->Append(id_open, "Open")->Enable(idx != wxNOT_FOUND);
	menu->Append(id_remove, "Remove")->Enable(idx != wxNOT_FOUND);
	menu->AppendSeparator();
	menu->Append(id_add_hdd, "Add");
	menu->Append(id_create_hdd, "Create");
	PopupMenu(menu);
}

void VHDDManagerDialog::OnOpen(wxCommandEvent& event)
{
	int idx = m_list->GetFirstSelected();
	if(idx >= 0) Open(idx);
}

void VHDDManagerDialog::OnRemove(wxCommandEvent& event)
{
	for(int sel = m_list->GetNextSelected(-1), offs = 0; sel != wxNOT_FOUND; sel = m_list->GetNextSelected(sel), --offs)
	{
		m_paths.erase(m_paths.begin() + (sel + offs));
	}

	UpdateList();
}

void VHDDManagerDialog::OnCreateHDD(wxCommandEvent& event)
{
	wxFileDialog ctrl(this, "Select HDD path", wxEmptyString, "new_hdd.hdd", "Virtual HDD (*.hdd) | *.hdd",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if(ctrl.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	VHDDSetInfoDialog dial(this);
	
	if(dial.ShowModal() == wxID_OK)
	{
		u64 size, bsize;
		dial.GetResult(size, bsize);
		vfsHDDManager::CreateHDD(fmt::ToUTF8(ctrl.GetPath()), size, bsize);
		m_paths.push_back(fmt::ToUTF8(ctrl.GetPath()));
		UpdateList();
	}
}

void VHDDManagerDialog::OnClose(wxCloseEvent& event)
{
	SavePaths();
	event.Skip();
}

void VHDDManagerDialog::LoadPaths()
{
	IniEntry<int> path_count;
	path_count.Init("path_count", "HDDManager");
	size_t count = 0;
	count = path_count.LoadValue(count);

	for(size_t i=0; i<count; ++i)
	{
		IniEntry<std::string> path_entry;
		path_entry.Init(fmt::Format("path[%d]", i), "HDDManager");
		m_paths.emplace_back(path_entry.LoadValue(""));
	}
}

void VHDDManagerDialog::SavePaths()
{
	IniEntry<int> path_count;
	path_count.Init("path_count", "HDDManager");
	path_count.SaveValue(m_paths.size());

	for(size_t i=0; i<m_paths.size(); ++i)
	{
		IniEntry<std::string> path_entry;
		path_entry.Init(fmt::Format("path[%d]", i), "HDDManager");
		path_entry.SaveValue(m_paths[i]);
	}
}
