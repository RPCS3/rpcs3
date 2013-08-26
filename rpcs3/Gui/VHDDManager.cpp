#include "stdafx.h"
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
	ConLog.Write("OnData(%d -> %d)", m_src_indx, dst_indx);
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

VHDDExplorer::VHDDExplorer(wxWindow* parent, const wxString& hdd_path) : wxDialog(parent, wxID_ANY, "Virtual HDD Explorer", wxDefaultPosition)
{
	m_list = new wxListView(this);
	m_drop_target = new VHDDListDropTarget(m_list);

	m_list->SetDropTarget(m_drop_target);
	m_list->DragAcceptFiles(true);

	wxBoxSizer& s_main(*new wxBoxSizer(wxVERTICAL));
	s_main.Add(m_list, 1, wxEXPAND | wxALL, 5);
	SetSizerAndFit(&s_main);

	SetSize(800, 600);
	m_list->InsertColumn(0, "Name");
	m_list->InsertColumn(1, "Type");
	m_list->InsertColumn(2, "Size");
	m_list->InsertColumn(3, "Creation time");

	m_hdd = new vfsHDD(hdd_path);
	UpdateList();
	Connect(m_list->GetId(),	wxEVT_COMMAND_LIST_BEGIN_DRAG,		wxListEventHandler(VHDDExplorer::OnListDrag));
	Connect(m_list->GetId(),	wxEVT_COMMAND_LIST_ITEM_ACTIVATED,	wxListEventHandler(VHDDExplorer::DClick));
	Connect(m_list->GetId(),	wxEVT_COMMAND_RIGHT_CLICK,			wxCommandEventHandler(VHDDExplorer::OnContextMenu));
	m_list->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(VHDDExplorer::OnDropFiles), (wxObject*)0, this);

	Connect(id_open,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDExplorer::OnOpen));
	Connect(id_rename,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDExplorer::OnRename));
	Connect(id_remove,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDExplorer::OnRemove));
	Connect(id_create_dir,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDExplorer::OnCreateDir));
	Connect(id_create_file,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDExplorer::OnCreateFile));
	Connect(id_import,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDExplorer::OnImport));
	Connect(id_export,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDExplorer::OnExport));
}

void VHDDExplorer::UpdateList()
{
	m_list->Freeze();
	m_list->DeleteAllItems();
	m_entries.Clear();
	m_names.Clear();

	u64 block;
	vfsHDD_Entry entry;
	wxString name;

	for(bool is_ok = m_hdd->GetFirstEntry(block, entry, name); is_ok; is_ok = m_hdd->GetNextEntry(block, entry, name))
	{
		int item = m_list->GetItemCount();
		m_list->InsertItem(item, name);
		m_list->SetItem(item, 1, entry.type == vfsHDD_Entry_Dir ? "Dir" : "File");
		m_list->SetItem(item, 2, wxString::Format("%lld", entry.size));
		m_list->SetItem(item, 3, wxDateTime().Set(time_t(entry.ctime)).Format());
		m_entries.AddCpy(entry);
		m_names.Add(name);
	}

	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
	m_list->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
	m_list->Thaw();
}

void VHDDExplorer::Import(const wxString& path, const wxString& to)
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

	wxFile f(path);
	char buf[256];

	while(!f.Eof())
	{
		m_hdd->Write(buf, f.Read(buf, 256));
	}

	m_hdd->Close();
}

void VHDDExplorer::Export(const wxString& path, const wxString& to)
{
	if(!m_hdd->Open(path))
	{
		wxMessageBox(wxString::Format("EXPORT ERROR: file open error. (%s)", path));
		return;
	}

	wxFile f(to, wxFile::write);
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
		ConLog.Write("Importing '%s'", dropped[i]);
		Import(dropped[i], wxFileName(dropped[i]).GetFullName());
	}

	UpdateList();
}

void VHDDExplorer::DClick(wxListEvent& event)
{
	OpenDir(event.GetIndex());
}

void VHDDExplorer::OnContextMenu(wxCommandEvent& event)
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

	PopupMenu( menu );
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
	static const wxString& fmt = "New Dir (%d)";
	while(m_hdd->HasEntry(wxString::Format(fmt, i))) i++;

	m_hdd->Create(vfsHDD_Entry_Dir, wxString::Format(fmt, i));
	UpdateList();
}

void VHDDExplorer::OnCreateFile(wxCommandEvent& event)
{
	int i = 1;
	static const wxString& fmt = "New File (%d)";
	while(m_hdd->HasEntry(wxString::Format(fmt, i))) i++;

	m_hdd->Create(vfsHDD_Entry_File, wxString::Format(fmt, i));
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
			Import(paths[i], wxFileName(paths[i]).GetFullName());
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
			Export(m_names[sel], ctrl.GetPath() + '\\' + m_names[sel]);
		}
	}
	else
	{
		int sel = m_list->GetFirstSelected();
		wxFileDialog ctrl(this, "Select export file", wxEmptyString, m_names[sel], wxFileSelectorDefaultWildcardStr, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		if(ctrl.ShowModal() == wxID_CANCEL)
		{
			return;
		}

		Export(m_names[sel], ctrl.GetPath());
	}

	UpdateList();
}

VHDDSetInfoDialog::VHDDSetInfoDialog(wxWindow* parent) : wxDialog(parent, wxID_ANY, "HDD Settings", wxDefaultPosition)
{
	m_spin_size = new wxSpinCtrl(this);
	m_ch_type = new wxChoice(this, wxID_ANY);
	m_spin_block_size = new wxSpinCtrl(this);

	wxBoxSizer& s_sinf(*new wxBoxSizer(wxHORIZONTAL));
	s_sinf.Add(m_spin_size, wxSizerFlags().Border(wxALL, 5).Expand());
	s_sinf.Add(m_ch_type, wxSizerFlags().Border(wxALL, 5).Expand());

	wxBoxSizer& s_binf(*new wxBoxSizer(wxHORIZONTAL));
	s_binf.Add(m_spin_block_size, wxSizerFlags().Border(wxALL, 5).Expand());

	wxBoxSizer& s_btns(*new wxBoxSizer(wxHORIZONTAL));
	s_btns.Add(new wxButton(this, wxID_OK), wxSizerFlags().Align(wxALIGN_LEFT).Border(wxALL, 5));
	s_btns.Add(new wxButton(this, wxID_CANCEL), wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxALL, 5));

	wxBoxSizer& s_main(*new wxBoxSizer(wxVERTICAL));
	s_main.Add(&s_sinf, wxSizerFlags().Align(wxALIGN_TOP).Expand());
	s_main.Add(&s_binf, wxSizerFlags().Align(wxALIGN_TOP).Expand());
	s_main.Add(&s_btns, wxSizerFlags().Align(wxALIGN_BOTTOM).Expand());
	SetSizerAndFit(&s_main);

	m_ch_type->Append("B");
	m_ch_type->Append("KB");
	m_ch_type->Append("MB");
	m_ch_type->Append("GB");

	m_spin_size->SetMin(1);
	m_spin_size->SetMax(0x7fffffff);
	m_spin_size->SetValue(64);
	m_ch_type->SetSelection(3);
	m_spin_block_size->SetMin(64);
	m_spin_block_size->SetMax(0x7fffffff);
	m_spin_block_size->SetValue(2048);
	Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(VHDDSetInfoDialog::OnOk));
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
	: wxDialog(parent, wxID_ANY, "Virtual HDD Manager", wxDefaultPosition)
{
	m_list = new wxListView(this);

	wxBoxSizer& s_main(*new wxBoxSizer(wxVERTICAL));
	s_main.Add(m_list, 1, wxEXPAND | wxALL, 5);

	SetSizerAndFit(&s_main);
	SetSize(800, 600);

	m_list->InsertColumn(0, "Path");
	//m_list->InsertColumn(1, "Size");
	//m_list->InsertColumn(2, "Block size");
	Connect(m_list->GetId(),	wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler(VHDDManagerDialog::DClick));
	Connect(m_list->GetId(),	wxEVT_COMMAND_RIGHT_CLICK,	wxCommandEventHandler(VHDDManagerDialog::OnContextMenu));

	Connect(id_add_hdd,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDManagerDialog::AddHDD));
	Connect(id_open,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDManagerDialog::OnOpen));
	Connect(id_remove,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDManagerDialog::OnRemove));
	Connect(id_create_hdd,		wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(VHDDManagerDialog::OnCreateHDD));
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(VHDDManagerDialog::OnClose));
	LoadPathes();
	UpdateList();
}

void VHDDManagerDialog::UpdateList()
{
	m_list->Freeze();
	m_list->DeleteAllItems();

	for(size_t i=0; i<m_pathes.GetCount(); ++i)
	{
		m_list->InsertItem(i, m_pathes[i]);
	}

	m_list->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
	//m_list->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
	//m_list->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);

	m_list->Thaw();
}

void VHDDManagerDialog::Open(int sel)
{
	VHDDExplorer dial(this, m_pathes[sel]);
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

	wxArrayString pathes;
	ctrl.GetPaths(pathes);
	for(size_t i=0; i<pathes.GetCount(); ++i)
	{
		bool skip = false;
		for(size_t j=0; j<m_pathes.GetCount(); ++j)
		{
			if(m_pathes[j].CmpNoCase(pathes[i]) == 0)
			{
				skip = true;
				break;
			}
		}
		
		if(!skip)
		{
			m_pathes.Move(new wxString(pathes[i].c_str()));
		}
	}
	UpdateList();
}

void VHDDManagerDialog::OnContextMenu(wxCommandEvent& event)
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
		m_pathes.RemoveAt(sel + offs);
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
		vfsHDDManager::CreateHDD(ctrl.GetPath(), size, bsize);
		m_pathes.AddCpy(ctrl.GetPath());
		UpdateList();
	}
}

void VHDDManagerDialog::OnClose(wxCloseEvent& event)
{
	SavePathes();
	event.Skip();
}

void VHDDManagerDialog::LoadPathes()
{
	IniEntry<int> path_count;
	path_count.Init("path_count", "HDDManager");
	int count = 0;
	count = path_count.LoadValue(count);

	m_pathes.SetCount(count);

	for(size_t i=0; i<m_pathes.GetCount(); ++i)
	{
		IniEntry<wxString> path_entry;
		path_entry.Init(wxString::Format("path[%d]", i), "HDDManager");
		new (m_pathes + i) wxString(path_entry.LoadValue(wxEmptyString));
	}
}

void VHDDManagerDialog::SavePathes()
{
	IniEntry<int> path_count;
	path_count.Init("path_count", "HDDManager");
	path_count.SaveValue(m_pathes.GetCount());

	for(size_t i=0; i<m_pathes.GetCount(); ++i)
	{
		IniEntry<wxString> path_entry;
		path_entry.Init(wxString::Format("path[%d]", i), "HDDManager");
		path_entry.SaveValue(m_pathes[i]);
	}
}