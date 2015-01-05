#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/SysCalls/SyncPrimitivesManager.h"
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
	const u32 total_memory_usage = Memory.GetUserMemTotalSize() - Memory.GetUserMemAvailSize();

	const auto& root = m_tree->AddRoot(fmt::Format("Process, ID = 0x00000001, Total Memory Usage = 0x%x (%0.2f MB)", total_memory_usage, (float)total_memory_usage / (1024 * 1024)));

	// TODO: FileSystem

	// Semaphores
	count = Emu.GetIdManager().GetTypeCount(TYPE_SEMAPHORE);
	if (count)
	{
		sprintf(name, "Semaphores (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_SEMAPHORE))
		{
			auto sem = Emu.GetSyncPrimManager().GetSemaphoreData(id);
			sprintf(name, "Semaphore: ID = 0x%08x '%s', Count = %d, Max Count = %d", id, sem.name.c_str(), sem.count, sem.max_count);
			m_tree->AppendItem(node, name);
		}
	}

	// Mutexes
	count = Emu.GetIdManager().GetTypeCount(TYPE_MUTEX);
	if (count)
	{
		sprintf(name, "Mutexes (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_MUTEX))
		{
			sprintf(name, "Mutex: ID = 0x%08x '%s'", id, Emu.GetSyncPrimManager().GetSyncPrimName(id, TYPE_MUTEX).c_str());
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Mutexes
	count = Emu.GetIdManager().GetTypeCount(TYPE_LWMUTEX);
	if (count)
	{
		sprintf(name, "Light Weight Mutexes (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_LWMUTEX))
		{
			auto lwm = Emu.GetSyncPrimManager().GetLwMutexData(id);
			sprintf(name, "LW Mutex: ID = 0x%08x '%s'", id, lwm.name.c_str());
			m_tree->AppendItem(node, name);
		}
	}

	// Condition Variables
	count = Emu.GetIdManager().GetTypeCount(TYPE_COND);
	if (count)
	{
		sprintf(name, "Condition Variables (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_COND))
		{
			sprintf(name, "Condition Variable: ID = 0x%08x '%s'", id, Emu.GetSyncPrimManager().GetSyncPrimName(id, TYPE_COND).c_str());
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Condition Variables
	count = Emu.GetIdManager().GetTypeCount(TYPE_LWCOND);
	if (count)
	{
		sprintf(name, "Light Weight Condition Variables (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_LWCOND))
		{
			sprintf(name, "LW Condition Variable: ID = 0x%08x '%s'", id, Emu.GetSyncPrimManager().GetSyncPrimName(id, TYPE_LWCOND).c_str());
			m_tree->AppendItem(node, name);
		}
	}

	// Event Queues
	count = Emu.GetIdManager().GetTypeCount(TYPE_EVENT_QUEUE);
	if (count)
	{
		sprintf(name, "Event Queues (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_EVENT_QUEUE))
		{
			sprintf(name, "Event Queue: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Modules
	count = Emu.GetIdManager().GetTypeCount(TYPE_PRX);
	if (count)
	{
		sprintf(name, "Modules (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		//sprintf(name, "Segment List (%l)", 2 * objects.size()); // TODO: Assuming 2 segments per PRX file is not good
		//m_tree->AppendItem(node, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_PRX))
		{
			sprintf(name, "PRX: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Memory Containers
	count = Emu.GetIdManager().GetTypeCount(TYPE_MEM);
	if (count)
	{
		sprintf(name, "Memory Containers (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_MEM))
		{
			sprintf(name, "Memory Container: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Event Flags
	count = Emu.GetIdManager().GetTypeCount(TYPE_EVENT_FLAG);
	if (count)
	{
		sprintf(name, "Event Flags (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().GetTypeIDs(TYPE_EVENT_FLAG))
		{
			sprintf(name, "Event Flag: ID = 0x%08x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// PPU / SPU / RawSPU threads
	{
		// TODO: add mutexes owners

		//const auto& objects = Emu.GetCPU().GetThreads();
		u32 ppu_threads_count = 0;
		u32 spu_threads_count = 0;
		u32 raw_spu_threads_count = 0;
		//for (const auto& thread : objects)
		//{
		//	if (thread->GetType() == CPU_THREAD_PPU)
		//		ppu_threads_count++;

		//	if (thread->GetType() == CPU_THREAD_SPU)
		//		spu_threads_count++;

		//	if (thread->GetType() == CPU_THREAD_RAW_SPU)
		//		raw_spu_threads_count++;
		//}

		//if (ppu_threads_count)
		//{
		//	sprintf(name, "PPU Threads (%d)", ppu_threads_count);
		//	const auto& node = m_tree->AppendItem(root, name);

		//	for (const auto& thread : objects)
		//	{
		//		if (thread->GetType() == CPU_THREAD_PPU)
		//		{
		//			sprintf(name, "Thread: ID = 0x%08x '%s', - %s", thread->GetId(), thread->GetName().c_str(), thread->ThreadStatusToString().c_str());
		//			m_tree->AppendItem(node, name);
		//		}
		//	}
		//}

		//if (spu_threads_count)
		//{
		//	sprintf(name, "SPU Threads (%d)", spu_threads_count);
		//	const auto& node = m_tree->AppendItem(root, name);

		//	for (const auto& thread : objects)
		//	{
		//		if (thread->GetType() == CPU_THREAD_SPU)
		//		{
		//			sprintf(name, "Thread: ID = 0x%08x '%s', - %s", thread->GetId(), thread->GetName().c_str(), thread->ThreadStatusToString().c_str());
		//			m_tree->AppendItem(node, name);
		//		}
		//	}
		//}

		//if (raw_spu_threads_count)
		//{
		//	sprintf(name, "RawSPU Threads (%d)", raw_spu_threads_count);
		//	const auto& node = m_tree->AppendItem(root, name);

		//	for (const auto& thread : objects)
		//	{
		//		if (thread->GetType() == CPU_THREAD_RAW_SPU)
		//		{
		//			sprintf(name, "Thread: ID = 0x%08x '%s', - %s", thread->GetId(), thread->GetName().c_str(), thread->ThreadStatusToString().c_str());
		//			m_tree->AppendItem(node, name);
		//		}
		//	}
		//}

	}

	m_tree->Expand(root);
}

void KernelExplorer::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
	Update();
}
