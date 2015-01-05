#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Loader/ELF64.h"
#include "Emu/FS/vfsDir.h"
#include "Emu/FS/vfsFile.h"
#include "LLEModulesManager.h"
#include "Emu/System.h"
#include "Emu/FS/VFS.h"

LLEModulesManagerFrame::LLEModulesManagerFrame(wxWindow* parent) : FrameBase(parent, wxID_ANY, "", "LLEModulesManagerFrame", wxSize(800, 600))
{
	wxBoxSizer *s_panel = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *s_p_panel = new wxBoxSizer(wxVERTICAL);
	wxPanel *p_main = new wxPanel(this);
	m_check_list = new wxCheckListBox(p_main, wxID_ANY);

	// select / unselect
	wxStaticBoxSizer* s_selection = new wxStaticBoxSizer(wxHORIZONTAL, p_main);
	wxButton* b_select = new wxButton(p_main, wxID_ANY, "Select All", wxDefaultPosition, wxSize(80, -1));
	wxButton* b_unselect = new wxButton(p_main, wxID_ANY, "Unselect All", wxDefaultPosition, wxSize(80, -1));
	s_selection->Add(b_select);
	s_selection->Add(b_unselect);

	s_p_panel->Add(s_selection);
	s_p_panel->Add(m_check_list, 1, wxEXPAND | wxALL, 5);
	p_main->SetSizerAndFit(s_p_panel);

	s_panel->Add(p_main, 1, wxEXPAND | wxALL, 5);
	SetSizerAndFit(s_panel);
	Refresh();

	b_select->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { OnSelectAll(event, true); event.Skip(); });
	b_unselect->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { OnSelectAll(event, false); event.Skip(); });
	Bind(wxEVT_CHECKLISTBOX, [this](wxCommandEvent& event) { UpdateSelection(event.GetInt()); event.Skip(); });
	Bind(wxEVT_SIZE, [p_main, this](wxSizeEvent& event) { p_main->SetSize(GetClientSize()); m_check_list->SetSize(p_main->GetClientSize() - wxSize(10, 50)); event.Skip(); });
}

void LLEModulesManagerFrame::Refresh()
{
	m_check_list->Clear();
	m_funcs.clear();

	std::string path = "/dev_flash/sys/external/";

	Emu.GetVFS().Init(path);

	vfsDir dir(path);

	loader::handlers::elf64 sprx_loader;
	for (const auto info : dir)
	{
		if (info->flags & DirEntry_TypeFile)
		{
			vfsFile f(path + info->name);
			if (sprx_loader.init(f) != loader::handler::ok)
			{
				continue;
			}

			if (!sprx_loader.is_sprx())
			{
				continue;
			}

			//loader::handlers::elf64::sprx_info info;
			//sprx_loader.load_sprx(info);

			std::string name = sprx_loader.sprx_get_module_name();

			bool is_skip = false;
			for (auto &i : m_funcs)
			{
				if (i == name)
				{
					is_skip = true;
					break;
				}
			}

			if (is_skip)
				continue;

			m_funcs.push_back(name);

			IniEntry<bool> load_lib;
			load_lib.Init(name, "LLE");

			m_check_list->Check(m_check_list->Append(name +
				" v" + std::to_string((int)sprx_loader.m_sprx_module_info.version[0]) +
				"." + std::to_string((int)sprx_loader.m_sprx_module_info.version[1])),
				load_lib.LoadValue(false));
		}
	}

	Emu.GetVFS().UnMountAll();
}

void LLEModulesManagerFrame::UpdateSelection(int index)
{
	if (index < 0)
		return;

	IniEntry<bool> load_lib;
	load_lib.Init(m_funcs[index], "LLE");
	load_lib.SaveValue(m_check_list->IsChecked(index));
}

void LLEModulesManagerFrame::OnSelectAll(wxCommandEvent& WXUNUSED(event), bool is_checked)
{
	for (uint i = 0; i < m_check_list->GetCount(); i++)
	{
		m_check_list->Check(i, is_checked);
		UpdateSelection(i);
	}
}