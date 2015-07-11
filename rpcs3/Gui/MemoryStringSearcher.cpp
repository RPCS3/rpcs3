#include "stdafx_gui.h"
#include "rpcs3/Ini.h"
#include "Utilities/rPlatform.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "MemoryStringSearcher.h"
#include "Emu/RSX/sysutil_video.h"
#include "Emu/RSX/GSManager.h"
//#include "Emu/RSX/GCM.h"

#include <wx/notebook.h>

MemoryStringSearcher::MemoryStringSearcher(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, "String Searcher", wxDefaultPosition, wxSize(545, 64))
	, exit(false)
{
	this->SetBackgroundColour(wxColour(240,240,240));
	//wxBoxSizer* s_panel = new wxBoxSizer(wxHORIZONTAL);

	//Tools
	//wxBoxSizer* s_tools = new wxBoxSizer(wxVERTICAL);

	//Tabs
	//wxNotebook* nb_rsx = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(482,475));

	s_panel = new wxBoxSizer(wxHORIZONTAL);
	t_addr = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(482, -1));
	b_search = new wxButton(this, wxID_ANY, "Search", wxPoint(482, 0), wxSize(40, -1));
	b_search->Bind(wxEVT_BUTTON, &MemoryStringSearcher::Search, this);
	s_panel->Add(t_addr);
	s_panel->Add(b_search);
};

void MemoryStringSearcher::Search(wxCommandEvent& event)
{
	const wxString wstr = t_addr->GetValue();
	const char *str = wstr.c_str();
	const u32 len = wstr.length();

	LOG_NOTICE(GENERAL, "Searching for string %s", str);

	// Search the address space for the string
	u32 strIndex = 0;
	u32 numFound = 0;
	const auto area = vm::get(vm::main);
	for (u32 addr = area->addr; addr < area->addr + area->size; addr++) {
		if (!vm::check_addr(addr)) {
			strIndex = 0;
			continue;
		}

		u8 byte = vm::read8(addr);
		if (byte == str[strIndex]) {
			if (strIndex == len) {
				// Found it
				LOG_NOTICE(GENERAL, "Found @ %04x", addr - len);
				numFound++;
				strIndex = 0;
				continue;
			}

			strIndex++;
		}
		else
			strIndex = 0;

		if (addr % (1024 * 1024 * 64) == 0) { // Log every 64mb
			LOG_NOTICE(GENERAL, "Searching %04x ...", addr);
		}
	}

	LOG_NOTICE(GENERAL, "Search completed (found %d matches)", numFound);
}