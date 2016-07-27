#include "stdafx.h"
#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_cond.h"
#include "Emu/Cell/lv2/sys_semaphore.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_event_flag.h"
#include "Emu/Cell/lv2/sys_rwlock.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_mmapper.h"
#include "Emu/Cell/lv2/sys_spu.h"

#include "KernelExplorer.h"

KernelExplorer::KernelExplorer(wxWindow* parent) 
	: wxDialog(parent, wxID_ANY, "Kernel Explorer", wxDefaultPosition, wxSize(700, 450))
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
	m_tree->DeleteAllItems();
	const u32 total_memory_usage = vm::get(vm::user_space)->used();

	const auto& root = m_tree->AddRoot(fmt::format("Process, ID = 0x00000001, Total Memory Usage = 0x%x (%0.2f MB)", total_memory_usage, (float)total_memory_usage / (1024 * 1024)));

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
	if (const u32 count = idm::get_count<lv2_sema_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Semaphores (%zu)", count));

		idm::select<lv2_sema_t>([&](u32 id, lv2_sema_t& sema)
		{
			m_tree->AppendItem(node, fmt::format("Semaphore: ID = 0x%08x '%s', Count = %d, Max Count = %d, Waiters = %#zu", id,
				&name64(sema.name), sema.value.load(), sema.max, sema.sq.size()));
		});
	}

	// Mutexes
	if (const u32 count = idm::get_count<lv2_mutex_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Mutexes (%zu)", count));

		idm::select<lv2_mutex_t>([&](u32 id, lv2_mutex_t& mutex)
		{
			m_tree->AppendItem(node, fmt::format("Mutex: ID = 0x%08x '%s'", id,
				&name64(mutex.name)));
		});
	}

	// Lightweight Mutexes
	if (const u32 count = idm::get_count<lv2_lwmutex_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Lightweight Mutexes (%zu)", count));

		idm::select<lv2_lwmutex_t>([&](u32 id, lv2_lwmutex_t& lwm)
		{
			m_tree->AppendItem(node, fmt::format("LWMutex: ID = 0x%08x '%s'", id,
				&name64(lwm.name)));
		});
	}

	// Condition Variables
	if (const u32 count = idm::get_count<lv2_cond_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Condition Variables (%zu)", count));

		idm::select<lv2_cond_t>([&](u32 id, lv2_cond_t& cond)
		{
			m_tree->AppendItem(node, fmt::format("Cond: ID = 0x%08x '%s'", id,
				&name64(cond.name)));
		});
	}

	// Lightweight Condition Variables
	if (const u32 count = idm::get_count<lv2_lwcond_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Lightweight Condition Variables (%zu)", count));

		idm::select<lv2_lwcond_t>([&](u32 id, lv2_lwcond_t& lwc)
		{
			m_tree->AppendItem(node, fmt::format("LWCond: ID = 0x%08x '%s'", id,
				&name64(lwc.name)));
		});
	}

	// Event Queues
	if (const u32 count = idm::get_count<lv2_event_queue_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Event Queues (%zu)", count));

		idm::select<lv2_event_queue_t>([&](u32 id, lv2_event_queue_t& eq)
		{
			m_tree->AppendItem(node, fmt::format("Event Queue: ID = 0x%08x '%s', %s, Key = %#llx, Events = %zu/%d, Waiters = %zu", id,
				&name64(eq.name), eq.type == SYS_SPU_QUEUE ? "SPU" : "PPU", eq.ipc_key, eq.events(), eq.size, eq.waiters()));
		});
	}

	// Event Ports
	if (const u32 count = idm::get_count<lv2_event_port_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Event Ports (%zu)", count));

		idm::select<lv2_event_port_t>([&](u32 id, lv2_event_port_t& ep)
		{
			m_tree->AppendItem(node, fmt::format("Event Port: ID = 0x%08x, Name = %#llx", id,
				ep.name));
		});
	}

	// Event Flags
	if (const u32 count = idm::get_count<lv2_event_flag_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Event Flags (%zu)", count));

		idm::select<lv2_event_flag_t>([&](u32 id, lv2_event_flag_t& ef)
		{
			m_tree->AppendItem(node, fmt::format("Event Flag: ID = 0x%08x '%s', Type = 0x%x, Pattern = 0x%llx", id,
				&name64(ef.name), ef.type, ef.pattern.load()));
		});
	}

	// Reader/writer Locks
	if (const u32 count = idm::get_count<lv2_rwlock_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Reader/writer Locks (%zu)", count));

		idm::select<lv2_rwlock_t>([&](u32 id, lv2_rwlock_t&)
		{
			m_tree->AppendItem(node, fmt::format("RWLock: ID = 0x%08x", id));
		});
	}

	// PRX Libraries
	if (const u32 count = idm::get_count<lv2_prx_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("PRX Libraries (%zu)", count));

		idm::select<lv2_prx_t>([&](u32 id, lv2_prx_t&)
		{
			m_tree->AppendItem(node, fmt::format("PRX: ID = 0x%08x", id));
		});
	}

	// Memory Containers
	if (const u32 count = idm::get_count<lv2_memory_container>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Memory Containers (%zu)", count));

		idm::select<lv2_memory_container>([&](u32 id, lv2_memory_container&)
		{
			m_tree->AppendItem(node, fmt::format("Memory Container: ID = 0x%08x", id));
		});
	}

	// Memory Objects
	if (const u32 count = idm::get_count<lv2_memory>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Memory Objects (%zu)", count));

		idm::select<lv2_memory>([&](u32 id, lv2_memory&)
		{
			m_tree->AppendItem(node, fmt::format("Memory Object: ID = 0x%08x", id));
		});
	}

	// PPU Threads
	if (const u32 count = idm::get_count<ppu_thread>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("PPU Threads (%zu)", count));

		idm::select<ppu_thread>([&](u32 id, ppu_thread& ppu)
		{
			m_tree->AppendItem(node, fmt::format("PPU Thread: ID = 0x%08x '%s'", id, ppu.get_name()));
		});
	}

	// SPU Thread Groups
	if (const u32 count = idm::get_count<lv2_spu_group_t>())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("SPU Thread Groups (%d)", count));

		idm::select<lv2_spu_group_t>([&](u32 id, lv2_spu_group_t& tg)
		{
			m_tree->AppendItem(node, fmt::format("SPU Thread Group: ID = 0x%08x '%s'", id,
				tg.name.c_str()));
		});
	}

	// RawSPU Threads (TODO)

	m_tree->Expand(root);
}

void KernelExplorer::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
	Update();
}
