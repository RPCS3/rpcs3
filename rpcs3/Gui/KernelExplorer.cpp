#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "KernelExplorer.h"

KernelExplorer::KernelExplorer(wxWindow* parent) 
	: wxFrame(parent, wxID_ANY, "Kernel Explorer", wxDefaultPosition, wxSize(700, 450))
{
	this->SetBackgroundColour(wxColour(240,240,240)); //This fix the ugly background color under Windows
	wxBoxSizer* s_panel = new wxBoxSizer(wxVERTICAL);
	
	// Buttons
	wxBoxSizer* box_buttons = new wxBoxSizer(wxHORIZONTAL);
	wxButton* b_refresh = new wxButton(this, wxID_ANY, "Refresh");
	box_buttons->AddSpacer(10);
	box_buttons->Add(b_refresh);
	box_buttons->AddSpacer(10);
	
	wxStaticBoxSizer* box_tree = new wxStaticBoxSizer(wxHORIZONTAL, this, "Kernel");
	m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(600,300));
	box_tree->Add(m_tree);

	// Merge and display everything
	s_panel->AddSpacer(10);
	s_panel->Add(box_buttons);
	s_panel->AddSpacer(10);
	s_panel->Add(box_tree, 0, 0, 100);
	s_panel->AddSpacer(10);
	SetSizerAndFit(s_panel);

	// Events
	b_refresh->Bind(wxEVT_BUTTON, &KernelExplorer::OnRefresh, this);
	
	// Fill the wxTreeCtrl
	Update();
};

void KernelExplorer::Update()
{
	int count;
	char name[4096];

	m_tree->DeleteAllItems();
	auto& root = m_tree->AddRoot("Process, ID = 0x00000001, Total Memory Usage = 0x???????? (???.? MB)");

	// TODO: PPU Threads
	// TODO: SPU/RawSPU Threads
	// TODO: FileSystem

	// Semaphores
	count = Emu.GetIdManager().GetTypeCount(TYPE_SEMAPHORE);
	if (count) {
		sprintf(name, "Semaphores (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_SEMAPHORE);
		for (const auto& id : objects) {
			sprintf(name, "Semaphore: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Mutexes
	count = Emu.GetIdManager().GetTypeCount(TYPE_MUTEX);
	if (count) {
		sprintf(name, "Mutexes (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_MUTEX);
		for (const auto& id : objects) {
			sprintf(name, "Mutex: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Mutexes
	count = Emu.GetIdManager().GetTypeCount(TYPE_LWMUTEX);
	if (count) {
		sprintf(name, "Light Weight Mutexes (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_LWMUTEX);
		for (const auto& id : objects) {
			sprintf(name, "LW Mutex: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Condition Variables
	count = Emu.GetIdManager().GetTypeCount(TYPE_COND);
	if (count) {
		sprintf(name, "Condition Variables (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_COND);
		for (const auto& id : objects) {
			sprintf(name, "Condition Variable: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Condition Variables
	count = Emu.GetIdManager().GetTypeCount(TYPE_LWCOND);
	if (count) {
		sprintf(name, "Light Weight Condition Variables (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_LWCOND);
		for (const auto& id : objects) {
			sprintf(name, "LW Condition Variable: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Event Queues
	count = Emu.GetIdManager().GetTypeCount(TYPE_EVENT_QUEUE);
	if (count) {
		sprintf(name, "Event Queues (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_EVENT_QUEUE);
		for (const auto& id : objects) {
			sprintf(name, "Event Queue: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Modules
	count = Emu.GetIdManager().GetTypeCount(TYPE_PRX);
	if (count) {
		sprintf(name, "Modules (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_PRX);
		sprintf(name, "Segment List (%d)", 2 * objects.size()); // TODO: Assuming 2 segments per PRX file is not good
		m_tree->AppendItem(node, name);
		for (const auto& id : objects) {
			sprintf(name, "PRX: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Memory Containers
	count = Emu.GetIdManager().GetTypeCount(TYPE_MEM);
	if (count) {
		sprintf(name, "Memory Containers (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_MEM);
		for (const auto& id : objects) {
			sprintf(name, "Memory Container: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}
	// Event Flags
	count = Emu.GetIdManager().GetTypeCount(TYPE_EVENT_FLAG);
	if (count) {
		sprintf(name, "Event Flags (%d)", count);
		auto& node = m_tree->AppendItem(root, name);
		auto& objects = Emu.GetIdManager().GetTypeIDs(TYPE_EVENT_FLAG);
		for (const auto& id : objects) {
			sprintf(name, "Event Flag: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	m_tree->Expand(root);
}

void KernelExplorer::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
	Update();
}
