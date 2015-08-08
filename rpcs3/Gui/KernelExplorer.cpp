#include "stdafx_gui.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_lwcond.h"
#include "Emu/SysCalls/lv2/sys_mutex.h"
#include "Emu/SysCalls/lv2/sys_cond.h"
#include "Emu/SysCalls/lv2/sys_semaphore.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/SysCalls/lv2/sys_event_flag.h"
#include "Emu/SysCalls/lv2/sys_rwlock.h"
#include "Emu/SysCalls/lv2/sys_prx.h"
#include "Emu/SysCalls/lv2/sys_memory.h"
#include "Emu/SysCalls/lv2/sys_mmapper.h"
#include "Emu/SysCalls/lv2/sys_spu.h"

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
	m_tree->DeleteAllItems();
	const u32 total_memory_usage = vm::get(vm::user_space)->used.load();

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
	const auto sema_map = idm::get_map<lv2_sema_t>();

	if (sema_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Semaphores (%zu)", sema_map.size()));

		for (const auto& data : sema_map)
		{
			const auto& sema = *data.second;

			m_tree->AppendItem(node, fmt::format("Semaphore: ID = 0x%08x '%s', Count = %d, Max Count = %d, Waiters = %#zu", data.first,
				&name64(sema.name), sema.value.load(), sema.max, sema.sq.size()));
		}
	}

	// Mutexes
	const auto mutex_map = idm::get_map<lv2_mutex_t>();

	if (mutex_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Mutexes (%zu)", mutex_map.size()));

		for (const auto& data : mutex_map)
		{
			const auto& mutex = *data.second;

			m_tree->AppendItem(node, fmt::format("Mutex: ID = 0x%08x '%s'", data.first,
				&name64(mutex.name)));
		}
	}

	// Lightweight Mutexes
	const auto lwm_map = idm::get_map<lv2_lwmutex_t>();

	if (lwm_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Lightweight Mutexes (%zu)", lwm_map.size()));

		for (const auto& data : lwm_map)
		{
			const auto& lwm = *data.second;

			m_tree->AppendItem(node, fmt::format("LWMutex: ID = 0x%08x '%s'", data.first,
				&name64(lwm.name)));
		}
	}

	// Condition Variables
	const auto cond_map = idm::get_map<lv2_cond_t>();

	if (cond_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Condition Variables (%zu)", cond_map.size()));

		for (const auto& data : cond_map)
		{
			const auto& cond = *data.second;

			m_tree->AppendItem(node, fmt::format("Cond: ID = 0x%08x '%s'", data.first,
				&name64(cond.name)));
		}
	}

	// Lightweight Condition Variables
	const auto lwc_map = idm::get_map<lv2_lwcond_t>();

	if (lwc_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Lightweight Condition Variables (%zu)", lwc_map.size()));

		for (const auto& data : lwc_map)
		{
			const auto& lwc = *data.second;

			m_tree->AppendItem(node, fmt::format("LWCond: ID = 0x%08x '%s'", data.first,
				&name64(lwc.name)));
		}
	}

	// Event Queues
	const auto eq_map = idm::get_map<lv2_event_queue_t>();

	if (eq_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Event Queues (%zu)", eq_map.size()));

		for (const auto& data : eq_map)
		{
			const auto& eq = *data.second;

			m_tree->AppendItem(node, fmt::format("Event Queue: ID = 0x%08x '%s', %s, Key = %#llx, Events = %zu/%d, Waiters = %zu", data.first,
				&name64(eq.name), eq.type == SYS_SPU_QUEUE ? "SPU" : "PPU", eq.key, eq.events.size(), eq.size, eq.sq.size()));
		}
	}

	// Event Ports
	const auto ep_map = idm::get_map<lv2_event_port_t>();

	if (ep_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Event Ports (%zu)", ep_map.size()));

		for (const auto& data : ep_map)
		{
			const auto& ep = *data.second;

			m_tree->AppendItem(node, fmt::format("Event Port: ID = 0x%08x, Name = %#llx", data.first,
				ep.name));
		}
	}

	// Event Flags
	const auto ef_map = idm::get_map<lv2_event_flag_t>();

	if (ef_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Event Flags (%zu)", ef_map.size()));

		for (const auto& data : ef_map)
		{
			const auto& ef = *data.second;

			m_tree->AppendItem(node, fmt::format("Event Flag: ID = 0x%08x", data.first));
		}
	}

	// Reader/writer Locks
	const auto rwlock_map = idm::get_map<lv2_rwlock_t>();

	if (rwlock_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Reader/writer Locks (%zu)", rwlock_map.size()));

		for (const auto& data : rwlock_map)
		{
			const auto& rwlock = *data.second;

			m_tree->AppendItem(node, fmt::format("RWLock: ID = 0x%08x", data.first));
		}
	}

	// PRX Libraries
	const auto prx_map = idm::get_map<lv2_prx_t>();

	if (prx_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("PRX Libraries (%zu)", prx_map.size()));

		for (const auto& data : prx_map)
		{
			const auto& prx = *data.second;

			m_tree->AppendItem(node, fmt::format("PRX: ID = 0x%08x", data.first));
		}
	}

	// Memory Containers
	const auto ct_map = idm::get_map<lv2_memory_container_t>();

	if (ct_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Memory Containers (%zu)", ct_map.size()));

		for (const auto& data : ct_map)
		{
			const auto& ct = *data.second;

			m_tree->AppendItem(node, fmt::format("Memory Container: ID = 0x%08x", data.first));
		}
	}

	// Memory Objects
	const auto mem_map = idm::get_map<lv2_memory_t>();

	if (mem_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("Memory Objects (%zu)", mem_map.size()));

		for (const auto& data : mem_map)
		{
			const auto& mem = *data.second;

			m_tree->AppendItem(node, fmt::format("Memory Object: ID = 0x%08x", data.first));
		}
	}

	// PPU Threads
	const auto ppu_map = idm::get_map<PPUThread>();

	if (ppu_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("PPU Threads (%zu)", ppu_map.size()));

		for (const auto& data : ppu_map)
		{
			const auto& ppu = *data.second;

			m_tree->AppendItem(node, fmt::format("PPU Thread: ID = 0x%08x '%s', - %s", data.first,
				ppu.get_name().c_str(), ppu.ThreadStatusToString()));
		}
	}

	// SPU Thread Groups
	const auto spu_map = idm::get_map<lv2_spu_group_t>();

	if (spu_map.size())
	{
		const auto& node = m_tree->AppendItem(root, fmt::format("SPU Thread Groups (%d)", spu_map.size()));

		for (const auto& data : spu_map)
		{
			const auto& tg = *data.second;

			m_tree->AppendItem(node, fmt::format("SPU Thread Group: ID = 0x%08x '%s'", data.first,
				tg.name.c_str()));
		}
	}

	// RawSPU Threads (TODO)

	m_tree->Expand(root);
}

void KernelExplorer::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
	Update();
}
