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
#include "Emu/Cell/lv2/sys_interrupt.h"
#include "Emu/Cell/lv2/sys_timer.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_fs.h"

#include "KernelExplorer.h"

KernelExplorer::KernelExplorer(wxWindow* parent) 
	: wxDialog(parent, wxID_ANY, "Kernel explorer", wxDefaultPosition, wxSize(700, 450))
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

	const auto vm_block = vm::get(vm::user_space);

	if (!vm_block)
	{
		return;
	}

	const u32 total_memory_usage = vm_block->used();

	const auto& root = m_tree->AddRoot(fmt::format("Process, ID = 0x00000001, Total memory usage = 0x%x (%0.2f MB)", total_memory_usage, (float)total_memory_usage / (1024 * 1024)));

	union name64
	{
		u64 u64_data;
		char string[8];

		name64(u64 data)
		    : u64_data(data & 0x00ffffffffffffffull)
		{
		}

		const char* operator+() const
		{
			return string;
		}
	};

	// TODO: FileSystem

	struct lv2_obj_rec
	{
		wxTreeItemId node;
		u32 count{0};

		lv2_obj_rec() = default;
		lv2_obj_rec(wxTreeItemId node)
			: node(node)
		{
		}
	};

	std::vector<lv2_obj_rec> lv2_types(256);
	lv2_types[SYS_MEM_OBJECT] = m_tree->AppendItem(root, "Memory");
	lv2_types[SYS_MUTEX_OBJECT] = m_tree->AppendItem(root, "Mutexes");
	lv2_types[SYS_COND_OBJECT] = m_tree->AppendItem(root, "Condition variables");
	lv2_types[SYS_RWLOCK_OBJECT] = m_tree->AppendItem(root, "Reader writer locks");
	lv2_types[SYS_INTR_TAG_OBJECT] = m_tree->AppendItem(root, "Interrupt tags");
	lv2_types[SYS_INTR_SERVICE_HANDLE_OBJECT] = m_tree->AppendItem(root, "Interrupt service handles");
	lv2_types[SYS_EVENT_QUEUE_OBJECT] = m_tree->AppendItem(root, "Event queues");
	lv2_types[SYS_EVENT_PORT_OBJECT] = m_tree->AppendItem(root, "Event ports");
	lv2_types[SYS_TRACE_OBJECT] = m_tree->AppendItem(root, "Traces");
	lv2_types[SYS_SPUIMAGE_OBJECT] = m_tree->AppendItem(root, "SPU Images");
	lv2_types[SYS_PRX_OBJECT] = m_tree->AppendItem(root, "Modules");
	lv2_types[SYS_SPUPORT_OBJECT] = m_tree->AppendItem(root, "SPU Ports");
	lv2_types[SYS_LWMUTEX_OBJECT] = m_tree->AppendItem(root, "Light weight mutexes");
	lv2_types[SYS_TIMER_OBJECT] = m_tree->AppendItem(root, "Timers");
	lv2_types[SYS_SEMAPHORE_OBJECT] = m_tree->AppendItem(root, "Semaphores");
	lv2_types[SYS_LWCOND_OBJECT] = m_tree->AppendItem(root, "Light weight condition variables");
	lv2_types[SYS_EVENT_FLAG_OBJECT] = m_tree->AppendItem(root, "Event flags");

	idm::select<lv2_obj>([&](u32 id, lv2_obj& obj)
	{
		lv2_types[id >> 24].count++;

		if (auto& node = lv2_types[id >> 24].node) switch (id >> 24)
		{
		case SYS_MEM_OBJECT:
		{
			auto& mem = static_cast<lv2_memory&>(obj);
			m_tree->AppendItem(node, fmt::format("Memory: ID = 0x%08x", id));
			break;
		}
		case SYS_MUTEX_OBJECT:
		{
			auto& mutex = static_cast<lv2_mutex&>(obj);
			m_tree->AppendItem(node, fmt::format("Mutex: ID = 0x%08x \"%s\",%s Owner = 0x%x, Locks = %u, Conds = %u, Wq = %zu", id, +name64(mutex.name),
				mutex.recursive == SYS_SYNC_RECURSIVE ? " Recursive," : "", mutex.owner >> 1, +mutex.lock_count, +mutex.cond_count, mutex.sq.size()));
			break;
		}
		case SYS_COND_OBJECT:
		{
			auto& cond = static_cast<lv2_cond&>(obj);
			m_tree->AppendItem(node, fmt::format("Cond: ID = 0x%08x \"%s\", Waiters = %u", id, +name64(cond.name), +cond.waiters));
			break;
		}
		case SYS_RWLOCK_OBJECT:
		{
			auto& rw = static_cast<lv2_rwlock&>(obj);
			const s64 val = rw.owner;
			m_tree->AppendItem(node, fmt::format("RW Lock: ID = 0x%08x \"%s\", Owner = 0x%x(%d), Rq = %zu, Wq = %zu", id, +name64(rw.name),
				std::max<s64>(0, val >> 1), -std::min<s64>(0, val >> 1), rw.rq.size(), rw.wq.size()));
			break;
		}
		case SYS_INTR_TAG_OBJECT:
		{
			auto& tag = static_cast<lv2_int_tag&>(obj);
			m_tree->AppendItem(node, fmt::format("Intr tag: ID = 0x%08x", id));
			break;
		}
		case SYS_INTR_SERVICE_HANDLE_OBJECT:
		{
			auto& serv = static_cast<lv2_int_serv&>(obj);
			m_tree->AppendItem(node, fmt::format("Intr Svc: ID = 0x%08x", id));
			break;
		}
		case SYS_EVENT_QUEUE_OBJECT:
		{
			auto& eq = static_cast<lv2_event_queue&>(obj);
			m_tree->AppendItem(node, fmt::format("Event queue: ID = 0x%08x \"%s\", %s, Key = %#llx, Events = %zu/%d, Waiters = %zu", id, +name64(eq.name),
				eq.type == SYS_SPU_QUEUE ? "SPU" : "PPU", eq.key, eq.events.size(), eq.size, eq.sq.size()));
			break;
		}
		case SYS_EVENT_PORT_OBJECT:
		{
			auto& ep = static_cast<lv2_event_port&>(obj);
			m_tree->AppendItem(node, fmt::format("Event port: ID = 0x%08x, Name = %#llx", id, ep.name));
			break;
		}
		case SYS_TRACE_OBJECT:
		{
			m_tree->AppendItem(node, fmt::format("Trace: ID = 0x%08x", id));
			break;
		}
		case SYS_SPUIMAGE_OBJECT:
		{
			m_tree->AppendItem(node, fmt::format("SPU Image: ID = 0x%08x", id));
			break;
		}
		case SYS_PRX_OBJECT:
		{
			auto& prx = static_cast<lv2_prx&>(obj);
			m_tree->AppendItem(node, fmt::format("PRX: ID = 0x%08x", id));
			break;
		}
		case SYS_SPUPORT_OBJECT:
		{
			m_tree->AppendItem(node, fmt::format("SPU Port: ID = 0x%08x", id));
			break;
		}
		case SYS_LWMUTEX_OBJECT:
		{
			auto& lwm = static_cast<lv2_lwmutex&>(obj);
			m_tree->AppendItem(node, fmt::format("LWMutex: ID = 0x%08x \"%s\", Wq = %zu", id, +name64(lwm.name), lwm.sq.size()));
			break;
		}
		case SYS_TIMER_OBJECT:
		{
			auto& timer = static_cast<lv2_timer&>(obj);
			m_tree->AppendItem(node, fmt::format("Timer: ID = 0x%08x", id));
			break;
		}
		case SYS_SEMAPHORE_OBJECT:
		{
			auto& sema = static_cast<lv2_sema&>(obj);
			m_tree->AppendItem(node, fmt::format("Semaphore: ID = 0x%08x \"%s\", Count = %d, Max count = %d, Waiters = %#zu", id, +name64(sema.name),
				sema.val.load(), sema.max, sema.sq.size()));
			break;
		}
		case SYS_LWCOND_OBJECT:
		{
			auto& lwc = static_cast<lv2_cond&>(obj);
			m_tree->AppendItem(node, fmt::format("LWCond: ID = 0x%08x \"%s\", Waiters = %zu", id, +name64(lwc.name), +lwc.waiters));
			break;
		}
		case SYS_EVENT_FLAG_OBJECT:
		{
			auto& ef = static_cast<lv2_event_flag&>(obj);
			m_tree->AppendItem(node, fmt::format("Event flag: ID = 0x%08x \"%s\", Type = 0x%x, Pattern = 0x%llx, Wq = %zu", id, +name64(ef.name),
				ef.type, ef.pattern.load(), +ef.waiters));
			break;
		}
		default:
		{
			m_tree->AppendItem(node, fmt::format("Unknown object: ID = 0x%08x", id));
		}
		}
	});

	lv2_types.emplace_back(m_tree->AppendItem(root, "Memory containers"));

	idm::select<lv2_memory_container>([&](u32 id, lv2_memory_container&)
	{
		lv2_types.back().count++;
		m_tree->AppendItem(lv2_types.back().node, fmt::format("Memory container: ID = 0x%08x", id));
	});

	lv2_types.emplace_back(m_tree->AppendItem(root, "PPU Threads"));

	idm::select<ppu_thread>([&](u32 id, ppu_thread& ppu)
	{
		lv2_types.back().count++;
		m_tree->AppendItem(lv2_types.back().node, fmt::format("PPU Thread: ID = 0x%08x '%s'", id, ppu.get_name()));
	});

	lv2_types.emplace_back(m_tree->AppendItem(root, "SPU Threads"));

	idm::select<SPUThread>([&](u32 id, SPUThread& spu)
	{
		lv2_types.back().count++;
		m_tree->AppendItem(lv2_types.back().node, fmt::format("SPU Thread: ID = 0x%08x '%s'", id, spu.get_name()));
	});

	lv2_types.emplace_back(m_tree->AppendItem(root, "SPU Thread groups"));

	idm::select<lv2_spu_group>([&](u32 id, lv2_spu_group& tg)
	{
		lv2_types.back().count++;
		m_tree->AppendItem(lv2_types.back().node, fmt::format("SPU Thread group: ID = 0x%08x '%s'", id, tg.name.c_str()));
	});

	lv2_types.emplace_back(m_tree->AppendItem(root, "File descriptors"));

	idm::select<lv2_fs_object>([&](u32 id, lv2_fs_object& fo)
	{
		lv2_types.back().count++;
		m_tree->AppendItem(lv2_types.back().node, fmt::format("FD: ID = 0x%08x '%s'", id));
	});

	for (auto&& entry : lv2_types)
	{
		if (entry.node && entry.count)
		{
			// Append object count
			m_tree->SetItemText(entry.node, m_tree->GetItemText(entry.node) + fmt::format(" (%zu)", entry.count));
		}
		else if (entry.node)
		{
			// Delete node otherwise
			m_tree->Delete(entry.node);
		}
	}

	// RawSPU Threads (TODO)

	m_tree->Expand(root);
}

void KernelExplorer::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
	Update();
}
