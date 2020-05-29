#include "stdafx.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_cond.h"
#include "Emu/Cell/lv2/sys_semaphore.h"
#include "Emu/Cell/lv2/sys_event_flag.h"
#include "Emu/Cell/lv2/sys_rwlock.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_overlay.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_mmapper.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/Cell/lv2/sys_fs.h"

#include "Emu/RSX/RSXThread.h"

#include "kernel_explorer.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>

kernel_explorer::kernel_explorer(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("Kernel Explorer"));
	setObjectName("kernel_explorer");
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(QSize(700, 450));

	QVBoxLayout* vbox_panel = new QVBoxLayout();
	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	QPushButton* button_refresh = new QPushButton(tr("Refresh"), this);
	hbox_buttons->addWidget(button_refresh);
	hbox_buttons->addStretch();

	m_tree = new QTreeWidget(this);
	m_tree->setBaseSize(QSize(600, 300));
	m_tree->setWindowTitle(tr("Kernel"));
	m_tree->header()->close();

	// Merge and display everything
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_buttons);
	vbox_panel->addSpacing(10);
	vbox_panel->addWidget(m_tree);
	vbox_panel->addSpacing(10);
	setLayout(vbox_panel);

	// Events
	connect(button_refresh, &QAbstractButton::clicked, this, &kernel_explorer::Update);

	Update();
}

constexpr auto qstr = QString::fromStdString;

void kernel_explorer::Update()
{
	m_tree->clear();

	const auto dct = g_fxo->get<lv2_memory_container>();

	if (!dct)
	{
		return;
	}

	const u32 total_memory_usage = dct->used;

	QTreeWidgetItem* root = new QTreeWidgetItem();
	root->setText(0, qstr(fmt::format("Process 0x%08x: Total Memory Usage: 0x%x/0x%x (%0.2f/%0.2f MB)", process_getpid(), total_memory_usage, dct->size, 1. * total_memory_usage / (1024 * 1024)
		, 1. * dct->size / (1024 * 1024))));
	m_tree->addTopLevelItem(root);

	// TODO: FileSystem

	struct lv2_obj_rec
	{
		QTreeWidgetItem* node;
		u32 count{ 0 };

		lv2_obj_rec() = default;
		lv2_obj_rec(QTreeWidgetItem* node)
			: node(node)
		{
		}
	};

	auto l_addTreeChild = [this](QTreeWidgetItem *parent, QString text)
	{
		QTreeWidgetItem *treeItem = new QTreeWidgetItem();
		treeItem->setText(0, text);
		parent->addChild(treeItem);
		return treeItem;
	};

	std::vector<lv2_obj_rec> lv2_types(256);
	lv2_types[SYS_MEM_OBJECT] =                 l_addTreeChild(root, "Memory");
	lv2_types[SYS_MUTEX_OBJECT] =               l_addTreeChild(root, "Mutexes");
	lv2_types[SYS_COND_OBJECT] =                l_addTreeChild(root, "Condition Variables");
	lv2_types[SYS_RWLOCK_OBJECT] =              l_addTreeChild(root, "Reader Writer Locks");
	lv2_types[SYS_INTR_TAG_OBJECT] =            l_addTreeChild(root, "Interrupt Tags");
	lv2_types[SYS_INTR_SERVICE_HANDLE_OBJECT] = l_addTreeChild(root, "Interrupt Service Handles");
	lv2_types[SYS_EVENT_QUEUE_OBJECT] =         l_addTreeChild(root, "Event Queues");
	lv2_types[SYS_EVENT_PORT_OBJECT] =          l_addTreeChild(root, "Event Ports");
	lv2_types[SYS_TRACE_OBJECT] =               l_addTreeChild(root, "Traces");
	lv2_types[SYS_SPUIMAGE_OBJECT] =            l_addTreeChild(root, "SPU Images");
	lv2_types[SYS_PRX_OBJECT] =                 l_addTreeChild(root, "PRX Modules");
	lv2_types[SYS_SPUPORT_OBJECT] =             l_addTreeChild(root, "SPU Ports");
	lv2_types[SYS_OVERLAY_OBJECT] =             l_addTreeChild(root, "Overlay Modules");
	lv2_types[SYS_LWMUTEX_OBJECT] =             l_addTreeChild(root, "Light Weight Mutexes");
	lv2_types[SYS_TIMER_OBJECT] =               l_addTreeChild(root, "Timers");
	lv2_types[SYS_SEMAPHORE_OBJECT] =           l_addTreeChild(root, "Semaphores");
	lv2_types[SYS_LWCOND_OBJECT] =              l_addTreeChild(root, "Light Weight Condition Variables");
	lv2_types[SYS_EVENT_FLAG_OBJECT] =          l_addTreeChild(root, "Event Flags");

	idm::select<lv2_obj>([&](u32 id, lv2_obj& obj)
	{
		lv2_types[id >> 24].count++;

		if (auto& node = lv2_types[id >> 24].node) switch (id >> 24)
		{
		case SYS_MEM_OBJECT:
		{
			auto& mem = static_cast<lv2_memory&>(obj);
			l_addTreeChild(node, qstr(fmt::format("Memory 0x%08x: Size: 0x%x (%0.2f MB), Granularity: %s, Mappings: %u", id, mem.size, mem.size * 1. / (1024 * 1024), mem.align == 0x10000u ? "64K" : "1MB", +mem.counter)));
			break;
		}
		case SYS_MUTEX_OBJECT:
		{
			auto& mutex = static_cast<lv2_mutex&>(obj);
			l_addTreeChild(node, qstr(fmt::format(u8"Mutex 0x%08x: “%s”,%s Owner: %#x, Locks: %u, Key: %#llx, Conds: %u, Wq: %zu", id, lv2_obj::name64(mutex.name),
				mutex.recursive == SYS_SYNC_RECURSIVE ? " Recursive," : "", mutex.owner >> 1, +mutex.lock_count, mutex.key, +mutex.cond_count, mutex.sq.size())));
			break;
		}
		case SYS_COND_OBJECT:
		{
			auto& cond = static_cast<lv2_cond&>(obj);
			l_addTreeChild(node, qstr(fmt::format(u8"Cond 0x%08x: “%s”, Key: %#llx, Wq: %u", id, lv2_obj::name64(cond.name), cond.key, +cond.waiters)));
			break;
		}
		case SYS_RWLOCK_OBJECT:
		{
			auto& rw = static_cast<lv2_rwlock&>(obj);
			const s64 val = rw.owner;
			l_addTreeChild(node, qstr(fmt::format(u8"RW Lock 0x%08x: “%s”, Owner: %#x(%d), Key: %#llx, Rq: %zu, Wq: %zu", id, lv2_obj::name64(rw.name),
				std::max<s64>(0, val >> 1), -std::min<s64>(0, val >> 1), rw.key, rw.rq.size(), rw.wq.size())));
			break;
		}
		case SYS_INTR_TAG_OBJECT:
		{
			// auto& tag = static_cast<lv2_int_tag&>(obj);
			l_addTreeChild(node, qstr(fmt::format("Intr Tag 0x%08x", id)));
			break;
		}
		case SYS_INTR_SERVICE_HANDLE_OBJECT:
		{
			// auto& serv = static_cast<lv2_int_serv&>(obj);
			l_addTreeChild(node, qstr(fmt::format("Intr Svc 0x%08x", id)));
			break;
		}
		case SYS_EVENT_QUEUE_OBJECT:
		{
			auto& eq = static_cast<lv2_event_queue&>(obj);
			l_addTreeChild(node, qstr(fmt::format(u8"Event Queue 0x%08x: “%s”, %s, Key: %#llx, Events: %zu/%d, Wq: %zu", id, lv2_obj::name64(eq.name),
				eq.type == SYS_SPU_QUEUE ? "SPU" : "PPU", eq.key, eq.events.size(), eq.size, eq.sq.size())));
			break;
		}
		case SYS_EVENT_PORT_OBJECT:
		{
			auto& ep = static_cast<lv2_event_port&>(obj);
			l_addTreeChild(node, qstr(fmt::format("Event Port 0x%08x: Name: %#llx, Bound: %s", id, ep.name, lv2_event_queue::check(ep.queue))));
			break;
		}
		case SYS_TRACE_OBJECT:
		{
			l_addTreeChild(node, qstr(fmt::format("Trace 0x%08x", id)));
			break;
		}
		case SYS_SPUIMAGE_OBJECT:
		{
			auto& spi = static_cast<lv2_spu_image&>(obj);
			l_addTreeChild(node, qstr(fmt::format("SPU Image 0x%08x, Entry: 0x%x, Segs: *0x%x, Num Segs: %d", id, spi.e_entry, spi.segs, spi.nsegs)));
			break;
		}
		case SYS_PRX_OBJECT:
		{
			auto& prx = static_cast<lv2_prx&>(obj);
			const u32 addr0 = !prx.segs.empty() ? prx.segs[0].addr : 0;

			if (!addr0)
			{
				l_addTreeChild(node, qstr(fmt::format("PRX 0x%08x: '%s' (HLE)", id, prx.name)));
				break;
			}

			const u32 end0 = addr0 + prx.segs[0].size - 1;
			l_addTreeChild(node, qstr(fmt::format("PRX 0x%08x: '%s', Seg0: (0x%x...0x%x)", id, prx.name, addr0, end0)));
			break;
		}
		case SYS_SPUPORT_OBJECT:
		{
			l_addTreeChild(node, qstr(fmt::format("SPU Port 0x%08x", id)));
			break;
		}
		case SYS_OVERLAY_OBJECT:
		{
			auto& ovl = static_cast<lv2_overlay&>(obj);
			l_addTreeChild(node, qstr(fmt::format("OVL 0x%08x: '%s', Seg0: (0x%x...0x%x)", id, ovl.name, ovl.segs[0].addr, ovl.segs[0].addr + ovl.segs[0].size - 1)));
			break;
		}
		case SYS_LWMUTEX_OBJECT:
		{
			auto& lwm = static_cast<lv2_lwmutex&>(obj);
			std::string owner_str = "unknown"; // Either invalid state or the lwmutex control data was moved from
			sys_lwmutex_t lwm_data{};

			if (lwm.control.try_read(lwm_data))
			{
				switch (const u32 owner = lwm_data.vars.owner)
				{
				case lwmutex_free: owner_str = "free"; break;
				//case lwmutex_dead: owner_str = "dead"; break;
				case lwmutex_reserved: owner_str = "reserved"; break;
				default:
				{
					if (owner >= ppu_thread::id_base && owner <= ppu_thread::id_base + ppu_thread::id_step - 1)
					{
						owner_str = fmt::format("0x%x", owner);
					}

					break;
				}
				}
			}
			else
			{
				l_addTreeChild(node, qstr(fmt::format(u8"LWMutex 0x%08x: “%s”, Wq: %zu (Couldn't extract control data)", id, lv2_obj::name64(lwm.name), lwm.sq.size())));
				break;
			}

			l_addTreeChild(node, qstr(fmt::format(u8"LWMutex 0x%08x: “%s”,%s Owner: %s, Locks: %u, Wq: %zu", id, lv2_obj::name64(lwm.name),
					(lwm_data.attribute & SYS_SYNC_RECURSIVE) ? " Recursive," : "", owner_str, lwm_data.recursive_count, lwm.sq.size())));
			break;
		}
		case SYS_TIMER_OBJECT:
		{
			// auto& timer = static_cast<lv2_timer&>(obj);
			l_addTreeChild(node, qstr(fmt::format("Timer 0x%08x", id)));
			break;
		}
		case SYS_SEMAPHORE_OBJECT:
		{
			auto& sema = static_cast<lv2_sema&>(obj);
			const auto val = +sema.val;
			l_addTreeChild(node, qstr(fmt::format(u8"Sema 0x%08x: “%s”, Count: %d/%d, Wq: %zu", id, lv2_obj::name64(sema.name),
				std::max<s32>(val, 0), sema.max, -std::min<s32>(val, 0))));
			break;
		}
		case SYS_LWCOND_OBJECT:
		{
			auto& lwc = static_cast<lv2_lwcond&>(obj);
			l_addTreeChild(node, qstr(fmt::format(u8"LWCond 0x%08x: “%s”, Wq: %zu", id, lv2_obj::name64(lwc.name), +lwc.waiters)));
			break;
		}
		case SYS_EVENT_FLAG_OBJECT:
		{
			auto& ef = static_cast<lv2_event_flag&>(obj);
			l_addTreeChild(node, qstr(fmt::format(u8"Event Flag 0x%08x: “%s”, Type: 0x%x, Key: %#llx, Pattern: 0x%llx, Wq: %zu", id, lv2_obj::name64(ef.name),
				ef.type, ef.key, ef.pattern.load(), +ef.waiters)));
			break;
		}
		default:
		{
			l_addTreeChild(node, qstr(fmt::format("Unknown object 0x%08x", id)));
		}
		}
	});

	lv2_types.emplace_back(l_addTreeChild(root, "Memory Containers"));

	idm::select<lv2_memory_container>([&](u32 id, lv2_memory_container& container)
	{
		lv2_types.back().count++;
		const u32 used = container.used;
		l_addTreeChild(lv2_types.back().node, qstr(fmt::format("Memory Container 0x%08x: Used: 0x%x/0x%x (%0.2f/%0.2f MB)", id, used, container.size, used * 1. / (1024 * 1024), container.size * 1. / (1024 * 1024))));
	});

	lv2_types.emplace_back(l_addTreeChild(root, "PPU Threads"));

	idm::select<named_thread<ppu_thread>>([&](u32 id, ppu_thread& ppu)
	{
		lv2_types.back().count++;

		const auto func = ppu.last_function;
		l_addTreeChild(lv2_types.back().node, qstr(fmt::format(u8"PPU 0x%07x: “%s”, PRIO: %d, Joiner: %s, State: %s, %s func: “%s”", id, *ppu.ppu_tname.load(), +ppu.prio, ppu.joiner.load(), ppu.state.load()
			, ppu.current_function ? "In" : "Last", func ? func : "")));
	});

	lv2_types.emplace_back(l_addTreeChild(root, "SPU Threads"));

	idm::select<named_thread<spu_thread>>([&](u32 /*id*/, spu_thread& spu)
	{
		lv2_types.back().count++;

		std::string_view type = "threaded";

		if (spu.is_isolated)
		{
			type = "isolated";
		}
		else if (spu.offset >= RAW_SPU_BASE_ADDR)
		{
			type = "raw";
		}

		l_addTreeChild(lv2_types.back().node, qstr(fmt::format(u8"SPU 0x%07x: “%s”, State: %s, Type: %s", spu.lv2_id, *spu.spu_tname.load(), spu.state.load(), type)));
	});

	lv2_types.emplace_back(l_addTreeChild(root, "SPU Thread Groups"));

	idm::select<lv2_spu_group>([&](u32 id, lv2_spu_group& tg)
	{
		lv2_types.back().count++;
		l_addTreeChild(lv2_types.back().node, qstr(fmt::format(u8"SPU Group 0x%07x: “%s”, Status = %s, Priority = %d, Type = 0x%x", id, tg.name, tg.run_state.load(), +tg.prio, tg.type)));
	});

	lv2_types.emplace_back(l_addTreeChild(root, "RSX Contexts"));

	do
	{
		// Currently a single context is supported at a time
		const auto rsx = rsx::get_current_renderer();
		const auto context_info = g_fxo->get<lv2_rsx_config>();

		if (!rsx || !context_info)
		{
			break;
		}

		const auto base = context_info->context_base;

		if (!base)
		{
			break;
		}

		lv2_types.back().count++;
		const auto tree = l_addTreeChild(lv2_types.back().node, qstr(fmt::format(u8"RSX Context 0x55555555, Local Size: %u MB, Base Addr: 0x%x, Device Addr: 0x%x", context_info->memory_size >> 20, base, context_info->device_addr)));

		lv2_obj_rec io_tree = l_addTreeChild(tree, "IO-EA Table");
		lv2_obj_rec zc_tree = l_addTreeChild(tree, "Zcull Bindings");
		lv2_obj_rec db_tree = l_addTreeChild(tree, "Display Buffers");

		decltype(rsx->iomap_table) table;
		decltype(rsx->display_buffers) dbs;
		decltype(rsx->zculls) zcs;
		{
			std::lock_guard lock(context_info->mutex);
			std::memcpy(&table, &rsx->iomap_table, sizeof(table));
			std::memcpy(&dbs, rsx->display_buffers, sizeof(dbs));
			std::memcpy(&zcs, &rsx->zculls, sizeof(zcs));
		}

		for (u32 i = 0, size_block = 0, first_ea = 0, first_io = 0;;)
		{
			const auto addr = table.get_addr(i << 20);

			if (addr == umax)
			{
				if (size_block)
				{
					// Print block
					l_addTreeChild(io_tree.node, qstr(fmt::format("IO: %08x, EA: %08x, Size: %uMB", first_io, first_ea, size_block)));
					io_tree.count++;
				}

				if (i >= 512u)
				{
					break;
				}

				size_block = 0;
				i++;
				continue;
			}

			const auto old_block_size = size_block++;

			i++;

			if (old_block_size)
			{
				if (first_ea + (old_block_size << 20) == addr)
				{
					continue;
				}
				else
				{
					// Print last block before we continue to a new one
					l_addTreeChild(io_tree.node, qstr(fmt::format("IO: %08x, EA: %08x, Size: %uMB", first_io, first_ea, old_block_size)));
					size_block = 1;
					first_ea = addr;
					first_io = (i - 1) << 20;
					io_tree.count++;
					continue;
				}
			}
			else
			{
				first_ea = addr;
				first_io = (i - 1) << 20;
			}
		}

		for (const auto& zc : zcs)
		{
			if (zc.bound)
			{
				l_addTreeChild(zc_tree.node, qstr(fmt::format("O: %07x, W: %u, H: %u, Zformat: 0x%x, AAformat: 0x%x, Dir: 0x%x, sFunc: 0x%x, sRef: %02x, sMask: %02x"
					, zc.offset, zc.height, zc.width, zc.zFormat, zc.aaFormat, zc.zcullDir, zc.sFunc, zc.sRef, zc.sMask)));
				zc_tree.count++;
			}
		}

		for (const auto& db : dbs)
		{
			if (db.valid())
			{
				l_addTreeChild(db_tree.node, qstr(fmt::format("Offset: %07x, Width: %u, Height: %u, Pitch: %u"
					, db.offset, db.height, db.width, db.pitch)));
				db_tree.count++;
			}
		}

		for (auto&& entry : {&io_tree, &zc_tree, &db_tree})
		{
			if (entry->node)
			{
				if (entry->count)
				{
					// Append object count
					entry->node->setText(0, entry->node->text(0) + qstr(fmt::format(" (%zu)", entry->count)));
				}
				else
				{
					// Delete node otherwise
					delete entry->node;
				}
			}
		}
	}
	while (0);

	lv2_types.emplace_back(l_addTreeChild(root, "File Descriptors"));

	idm::select<lv2_fs_object>([&](u32 id, lv2_fs_object& fo)
	{
		lv2_types.back().count++;
		l_addTreeChild(lv2_types.back().node, qstr(fmt::format(u8"FD %u: “%s”", id, fo.name.data())));
	});

	for (auto&& entry : lv2_types)
	{
		if (entry.node)
		{
			if (entry.count)
			{
				// Append object count
				entry.node->setText(0, entry.node->text(0) + qstr(fmt::format(" (%zu)", entry.count)));
			}
			else
			{
				// Delete node otherwise
				delete entry.node;
			}
		}
	}

	// RawSPU Threads (TODO)

	root->setExpanded(true);
}
