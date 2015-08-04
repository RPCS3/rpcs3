#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/SysCalls/lv2/sleep_queue.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_mutex.h"
#include "Emu/SysCalls/lv2/sys_cond.h"
#include "Emu/SysCalls/lv2/sys_semaphore.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/SysCalls/lv2/sys_event_flag.h"
#include "Emu/SysCalls/lv2/sys_prx.h"
#include "Emu/SysCalls/lv2/sys_memory.h"

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
	const u32 total_memory_usage = vm::get(vm::user_space)->used.load();

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
	if (u32 count = Emu.GetIdManager().get_count<lv2_sema_t>())
	{
		sprintf(name, "Semaphores (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto id : Emu.GetIdManager().get_set<lv2_sema_t>())
		{
			const auto sem = Emu.GetIdManager().get<lv2_sema_t>(id);
			sprintf(name, "Semaphore: ID = 0x%x '%s', Count = %d, Max Count = %d, Waiters = %#llx", id, &name64(sem->name), sem->value.load(), sem->max, sem->sq.size());
			m_tree->AppendItem(node, name);
		}
	}

	// Mutexes
	if (u32 count = Emu.GetIdManager().get_count<lv2_mutex_t>())
	{
		sprintf(name, "Mutexes (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto id : Emu.GetIdManager().get_set<lv2_mutex_t>())
		{
			const auto mutex = Emu.GetIdManager().get<lv2_mutex_t>(id);
			sprintf(name, "Mutex: ID = 0x%x '%s'", id, &name64(mutex->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Mutexes
	if (u32 count = Emu.GetIdManager().get_count<lv2_lwmutex_t>())
	{
		sprintf(name, "Lightweight Mutexes (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto id : Emu.GetIdManager().get_set<lv2_lwmutex_t>())
		{
			const auto lwm = Emu.GetIdManager().get<lv2_lwmutex_t>(id);
			sprintf(name, "Lightweight Mutex: ID = 0x%x '%s'", id, &name64(lwm->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Condition Variables
	if (u32 count = Emu.GetIdManager().get_count<lv2_cond_t>())
	{
		sprintf(name, "Condition Variables (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto id : Emu.GetIdManager().get_set<lv2_cond_t>())
		{
			const auto cond = Emu.GetIdManager().get<lv2_cond_t>(id);
			sprintf(name, "Condition Variable: ID = 0x%x '%s'", id, &name64(cond->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Light Weight Condition Variables
	if (u32 count = Emu.GetIdManager().get_count<lv2_lwcond_t>())
	{
		sprintf(name, "Lightweight Condition Variables (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto id : Emu.GetIdManager().get_set<lv2_lwcond_t>())
		{
			const auto lwc = Emu.GetIdManager().get<lv2_lwcond_t>(id);
			sprintf(name, "Lightweight Condition Variable: ID = 0x%x '%s'", id, &name64(lwc->name));
			m_tree->AppendItem(node, name);
		}
	}

	// Event Queues
	if (u32 count = Emu.GetIdManager().get_count<lv2_event_queue_t>())
	{
		sprintf(name, "Event Queues (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto id : Emu.GetIdManager().get_set<lv2_event_queue_t>())
		{
			const auto queue = Emu.GetIdManager().get<lv2_event_queue_t>(id);
			sprintf(name, "Event Queue: ID = 0x%x '%s', Key = %#llx", id, &name64(queue->name), queue->key);
			m_tree->AppendItem(node, name);
		}
	}

	// Event Ports
	if (u32 count = Emu.GetIdManager().get_count<lv2_event_port_t>())
	{
		sprintf(name, "Event Ports (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto id : Emu.GetIdManager().get_set<lv2_event_port_t>())
		{
			const auto port = Emu.GetIdManager().get<lv2_event_port_t>(id);
			sprintf(name, "Event Port: ID = 0x%x, Name = %#llx", id, port->name);
			m_tree->AppendItem(node, name);
		}
	}

	// Modules
	if (u32 count = Emu.GetIdManager().get_count<lv2_prx_t>())
	{
		sprintf(name, "Modules (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);
		//sprintf(name, "Segment List (%l)", 2 * objects.size()); // TODO: Assuming 2 segments per PRX file is not good
		//m_tree->AppendItem(node, name);

		for (const auto& id : Emu.GetIdManager().get_set<lv2_prx_t>())
		{
			sprintf(name, "PRX: ID = 0x%x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Memory Containers
	if (u32 count = Emu.GetIdManager().get_count<lv2_memory_container_t>())
	{
		sprintf(name, "Memory Containers (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto& id : Emu.GetIdManager().get_set<lv2_memory_container_t>())
		{
			sprintf(name, "Memory Container: ID = 0x%x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// Event Flags
	if (u32 count = Emu.GetIdManager().get_count<lv2_event_flag_t>())
	{
		sprintf(name, "Event Flags (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto& id : Emu.GetIdManager().get_set<lv2_event_flag_t>())
		{
			sprintf(name, "Event Flag: ID = 0x%x", id);
			m_tree->AppendItem(node, name);
		}
	}

	// PPU Threads
	if (u32 count = Emu.GetIdManager().get_count<PPUThread>())
	{
		sprintf(name, "PPU Threads (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto& thread : Emu.GetIdManager().get_all<PPUThread>())
		{
			sprintf(name, "Thread: ID = 0x%08x '%s', - %s", thread->get_id(), thread->get_name().c_str(), thread->ThreadStatusToString());
			m_tree->AppendItem(node, name);
		}
	}

	if (u32 count = Emu.GetIdManager().get_count<SPUThread>())
	{
		sprintf(name, "SPU Threads (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto& thread : Emu.GetIdManager().get_all<SPUThread>())
		{
			if (thread->get_type() == CPU_THREAD_SPU)
			{
				sprintf(name, "Thread: ID = 0x%08x '%s', - %s", thread->get_id(), thread->get_name().c_str(), thread->ThreadStatusToString());
				m_tree->AppendItem(node, name);
			}
		}
	}

	if (u32 count = Emu.GetIdManager().get_count<RawSPUThread>())
	{
		sprintf(name, "RawSPU Threads (%d)", count);
		const auto& node = m_tree->AppendItem(root, name);

		for (const auto& thread : Emu.GetIdManager().get_all<RawSPUThread>())
		{
			if (thread->get_type() == CPU_THREAD_RAW_SPU)
			{
				sprintf(name, "Thread: ID = 0x%08x '%s', - %s", thread->get_id(), thread->get_name().c_str(), thread->ThreadStatusToString());
				m_tree->AppendItem(node, name);
			}
		}
	}

	m_tree->Expand(root);
}

void KernelExplorer::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
	Update();
}
