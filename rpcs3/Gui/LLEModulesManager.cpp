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
	s_p_panel->Add(m_check_list, 0, wxEXPAND | wxALL, 5);
	p_main->SetSizerAndFit(s_p_panel);
	s_panel->Add(p_main, 0, wxEXPAND | wxALL, 5);
	SetSizerAndFit(s_panel);

	Refresh();
	//Bind(wxEVT_CHECKLISTBOX, [this](wxCommandEvent& event) { UpdateSelection(); });
	Bind(wxEVT_SIZE, [p_main, this](wxSizeEvent& event) { p_main->SetSize(GetClientSize()); m_check_list->SetSize(p_main->GetClientSize() - wxSize(10, 10)); });
}

void LLEModulesManagerFrame::Refresh()
{
	m_check_list->Clear();

	std::string path = "/dev_flash/sys/external/";

	Emu.GetVFS().Init(path);

	vfsDir dir(path);

	loader::handlers::elf64 sprx_loader;
	for (const DirEntryInfo* info = dir.Read(); info; info = dir.Read())
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

			sprx_loader.load();

			m_check_list->Check(m_check_list->Append(sprx_loader.sprx_get_module_name() + 
				" v" + std::to_string((int)sprx_loader.m_sprx_module_info.version[0]) + "." + std::to_string((int)sprx_loader.m_sprx_module_info.version[1])));
		}
	}

	Emu.GetVFS().UnMountAll();
}

void LLEModulesManagerFrame::UpdateSelection()
{

}