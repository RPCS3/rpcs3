#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/SysCalls/lv2/sleep_queue.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_mutex.h"
#include "Emu/SysCalls/lv2/sys_cond.h"
#include "Emu/SysCalls/lv2/sys_semaphore.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/SysCalls/lv2/sys_process.h"

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
	char name[4096];

	m_tree->DeleteAllItems();
	const u32 total_memory_usage = Memory.GetUserMemTotalSize() - Memory.GetUserMemAvailSize();

	const auto& root = m_tree->AddRoot(fmt::Format("Process, ID = 0x00000001, Total Memory Usage = 0x%x (%0.2f MB)", total_memory_usage, (float)total_memory_usage / (1024 * 1024)));

	union name64
	{
		u64 u64_data;
		char string[8];

		name64(u64 data)
			: u64_data(data & 0x00ffffffffffffffull)
		{
		}

		const char* operator &() const
		{
			return string;
		}
	};

	// TODO: FileSystem

	// Semaphores
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_SEMAPHORE_OBJECT))
	{
		sprintf(name, "Semaphores (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto id : Emu.GetIdManager().get_IDs_by_type(SYS_SEMAPHORE_OBJECT))
		{
			const auto sem = Emu.GetIdManager().get<lv2_sema_t>(id);
			sprintf(name, "Semaphore: ID = 0x%x '%s', Count = %d, Max Count = %d, Waiters = %d", id, &name64(sem->name), sem->value.load(), sem->max, sem->waiters.load());
			m_tree->AppendItem(node, name);
		}
	}

	// Mutexes
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_MUTEX_OBJECT))
	{
		sprintf(name, "Mutexes (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto id : Emu.GetIdManager().get_IDs_by_type(SYS_MUTEX_OBJECT))
		{
			const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(id);
			sprintf(name, "Mutex: ID = 0x%x '%s'", id, &name64(mutex->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Mutexes
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_LWMUTEX_OBJECT))
	{
		sprintf(name, "Lightweight Mutexes (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto id : Emu.GetIdManager().get_IDs_by_type(SYS_LWMUTEX_OBJECT))
		{
			const auto lwm = Emu.GetIdManager().get<lv2_lwmutex_t>(id);
			sprintf(name, "Lightweight Mutex: ID = 0x%x '%s'", id, &name64(lwm->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Condition Variables
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_COND_OBJECT))
	{
		sprintf(name, "Condition Variables (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto id : Emu.GetIdManager().get_IDs_by_type(SYS_COND_OBJECT))
		{
			const auto cond = Emu.GetIdManager().get<lv2_cond_t>(id);
			sprintf(name, "Condition Variable: ID = 0x%x '%s'", id, &name64(cond->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Condition Variables
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_LWCOND_OBJECT))
	{
		sprintf(name, "Lightweight Condition Variables (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto id : Emu.GetIdManager().get_IDs_by_type(SYS_LWCOND_OBJECT))
		{
			const auto lwc = Emu.GetIdManager().get<lv2_lwcond_t>(id);
			sprintf(name, "Lightweight Condition Variable: ID = 0x%x '%s'", id, &name64(lwc->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Event Queues
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_EVENT_QUEUE_OBJECT))
	{
		sprintf(name, "Event Queues (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto id : Emu.GetIdManager().get_IDs_by_type(SYS_EVENT_QUEUE_OBJECT))
		{
			const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(id);
			sprintf(name, "Event Queue: ID = 0x%x '%s', Key = %#llx", id, &name64(queue->name), queue->key);
			m_tree->AppendItem(node, name);
		}
	}

	// Event Ports
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_EVENT_PORT_OBJECT))
	{
		sprintf(name, "Event Ports (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto id : Emu.GetIdManager().get_IDs_by_type(SYS_EVENT_PORT_OBJECT))
		{
			const auto port = Emu.GetIdManager().get<lv2_event_port_t>(id);
			sprintf(name, "Event Port: ID = 0x%x, Name = %#llx", id, port->name);
			m_tree->AppendItem(node, name);
		}
	}

	// Modules
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_PRX_OBJECT))
	{
		sprintf(name, "Modules (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		//sprintf(name, "Segment List (%l)", 2 * objects.size()); // TODO: Assuming 2 segments per PRX file is not good
		//m_tree->AppendItem(node, name);
		for (const auto& id : Emu.GetIdManager().get_IDs_by_type(SYS_PRX_OBJECT))
		{
			sprintf(name, "PRX: ID = 0x%x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Memory Containers
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_MEM_OBJECT))
	{
		sprintf(name, "Memory Containers (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().get_IDs_by_type(SYS_MEM_OBJECT))
		{
			sprintf(name, "Memory Container: ID = 0x%x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Event Flags
	if (u32 count = Emu.GetIdManager().get_count_by_type(SYS_EVENT_FLAG_OBJECT))
	{
		sprintf(name, "Event Flags (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		for (const auto& id : Emu.GetIdManager().get_IDs_by_type(SYS_EVENT_FLAG_OBJECT))
		{
			sprintf(name, "Event Flag: ID = 0x%x", id);
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
