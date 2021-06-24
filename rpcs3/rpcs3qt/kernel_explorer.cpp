#include <map>

#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
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
#include "Emu/Cell/lv2/sys_timer.h"
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/Cell/lv2/sys_vm.h"
#include "Emu/Cell/lv2/sys_net.h"
#include "Emu/Cell/lv2/sys_fs.h"
#include "Emu/Cell/lv2/sys_interrupt.h"
#include "Emu/Cell/Modules/cellSpurs.h"

#include "Emu/RSX/RSXThread.h"

#include "kernel_explorer.h"
#include "qt_utils.h"

constexpr auto qstr = QString::fromStdString;

enum kernel_item_role
{
	name_role       = Qt::UserRole + 0,
	expanded_role   = Qt::UserRole + 1,
	type_role       = Qt::UserRole + 2,
	id_role         = Qt::UserRole + 3,
};

enum kernel_item_type : int
{
	root,
	node,
	volatile_node,
	solid_node,
	leaf
};

static QTreeWidgetItem* add_child(QTreeWidgetItem *parent, const QString& text, int column, kernel_item_type type)
{
	if (parent)
	{
		for (int i = 0; i < parent->childCount(); i++)
		{
			if (parent->child(i)->data(0, kernel_item_role::name_role).toString() == text)
			{
				return parent->child(i);
			}
		}
	}
	QTreeWidgetItem* item = gui::utils::add_child(parent, text, column);
	if (item)
	{
		item->setData(0, kernel_item_role::name_role, text);
		item->setData(0, kernel_item_role::type_role, type);
	}
	return item;
}

static QTreeWidgetItem* add_leaf(QTreeWidgetItem *parent, const QString& text, int column = 0)
{
	return add_child(parent, text, column, kernel_item_type::leaf);
}

static QTreeWidgetItem* add_node(u32 id, QTreeWidgetItem *parent, const QString& text, int column = 0)
{
	QTreeWidgetItem* node = add_child(parent, text, column, kernel_item_type::node);
	if (node)
	{
		node->setData(0, kernel_item_role::id_role, id);
	}
	return node;
}

static QTreeWidgetItem* find_first_node(QTreeWidget* tree, QTreeWidgetItem *parent, const QString& regexp)
{
	if (tree && parent)
	{
		for (auto item : tree->findItems(regexp, Qt::MatchFlag::MatchRegularExpression | Qt::MatchFlag::MatchRecursive))
		{
			if (item->parent() == parent && item->data(0, kernel_item_role::type_role).toInt() != kernel_item_type::leaf)
			{
				return item;
			}
		}
	}
	return nullptr;
}

static QTreeWidgetItem* find_node(QTreeWidget* tree, u32 id)
{
	if (tree)
	{
		for (auto item : tree->findItems(".*", Qt::MatchFlag::MatchRegularExpression | Qt::MatchFlag::MatchRecursive))
		{
			if (item->data(0, kernel_item_role::type_role).toInt() == kernel_item_type::node &&
				item->data(0, kernel_item_role::id_role).toUInt() == id)
			{
				return item;
			}
		}
	}
	return nullptr;
}

// Find node with ID in selected node children (optimization)
static QTreeWidgetItem* find_node(QTreeWidgetItem* root, u32 id)
{
	if (root)
	{
		for (int i = 0; i < root->childCount(); i++)
		{
			if (auto item = root->child(i); item->data(0, kernel_item_role::type_role).toInt() == kernel_item_type::node &&
				item->data(0, kernel_item_role::id_role).toUInt() == id)
			{
				return item;
			}
		}
	}

	return nullptr;
}

static QTreeWidgetItem* add_volatile_node(QTreeWidget* tree, QTreeWidgetItem *parent, const QString& base_text, const QString& text = "", int column = 0)
{
	QTreeWidgetItem* node = find_first_node(tree, parent, base_text + ".*");
	if (!node)
	{
		node = add_child(parent, base_text, column, kernel_item_type::volatile_node);
	}
	if (node)
	{
		node->setText(0, text.isEmpty() ? base_text : text);
	}
	return node;
}

static QTreeWidgetItem* add_solid_node(QTreeWidget* tree, QTreeWidgetItem *parent, const QString& base_text, const QString& text = "", int column = 0)
{
	QTreeWidgetItem* node = find_first_node(tree, parent, base_text + ".*");
	if (!node)
	{
		node = add_child(parent, base_text, column, kernel_item_type::solid_node);
	}
	if (node)
	{
		node->setText(0, text.isEmpty() ? base_text : text);
	}
	return node;
}

kernel_explorer::kernel_explorer(QWidget* parent)
	: QDialog(parent)
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
	vbox_panel->addSpacing(8);
	vbox_panel->addLayout(hbox_buttons);
	vbox_panel->addSpacing(8);
	vbox_panel->addWidget(m_tree);
	setLayout(vbox_panel);

	// Events
	connect(button_refresh, &QAbstractButton::clicked, this, &kernel_explorer::Update);

	Update();
}

void kernel_explorer::Update()
{
	const auto dct = g_fxo->try_get<lv2_memory_container>();

	if (!dct)
	{
		m_tree->clear();
		return;
	}

	//static const usz additional_size = 6;

	const std::unordered_map<u32, QString> tree_item_names =
	{
		{ process_info                   , tr("Process Info")},

		{ SYS_MEM_OBJECT                 , tr("Shared Memory")},
		{ virtual_memory                 , tr("Virtual Memory")},
		{ SYS_MUTEX_OBJECT               , tr("Mutexes")},
		{ SYS_COND_OBJECT                , tr("Condition Variables")},
		{ SYS_RWLOCK_OBJECT              , tr("Reader Writer Locks")},
		{ SYS_INTR_TAG_OBJECT            , tr("Interrupt Tags")},
		{ SYS_INTR_SERVICE_HANDLE_OBJECT , tr("Interrupt Service Handles")},
		{ SYS_EVENT_QUEUE_OBJECT         , tr("Event Queues")},
		{ SYS_EVENT_PORT_OBJECT          , tr("Event Ports")},
		{ SYS_TRACE_OBJECT               , tr("Traces")},
		{ SYS_SPUIMAGE_OBJECT            , tr("SPU Images")},
		{ SYS_PRX_OBJECT                 , tr("PRX Modules")},
		{ SYS_SPUPORT_OBJECT             , tr("SPU Ports")},
		{ SYS_OVERLAY_OBJECT             , tr("Overlay Modules")},
		{ SYS_LWMUTEX_OBJECT             , tr("Light Weight Mutexes")},
		{ SYS_TIMER_OBJECT               , tr("Timers")},
		{ SYS_SEMAPHORE_OBJECT           , tr("Semaphores")},
		{ SYS_FS_FD_OBJECT               , tr("File Descriptors ?")},
		{ SYS_LWCOND_OBJECT              , tr("Light Weight Condition Variables")},
		{ SYS_EVENT_FLAG_OBJECT          , tr("Event Flags")},

		{ memory_containers              , tr("Memory Containers")},
		{ ppu_threads                    , tr("PPU Threads")},
		{ spu_threads                    , tr("SPU Threads")},
		{ spu_thread_groups              , tr("SPU Thread Groups")},
		{ rsx_contexts                   , tr("RSX Contexts")},
		{ sockets                        , tr("Sockets")},
		{ file_descriptors               , tr("File Descriptors")},
	};

	QTreeWidgetItem* root = m_tree->topLevelItem(0);
	if (!root)
	{
		root = new QTreeWidgetItem();
		root->setData(0, kernel_item_role::type_role, kernel_item_type::root);
		m_tree->addTopLevelItem(root);

		for (const auto& [key, name] : tree_item_names)
		{
			add_node(key, root, name);
		}
	}
	else
	{
		std::function<void(QTreeWidgetItem*)> clean_up_tree;
		clean_up_tree = [&clean_up_tree](QTreeWidgetItem* item)
		{
			if (item && item->data(0, kernel_item_role::type_role).toInt() != kernel_item_type::leaf)
			{
				item->setText(0, item->data(0, kernel_item_role::name_role).toString());
				item->setData(0, kernel_item_role::expanded_role, item->isExpanded());

				for (int i = item->childCount() - 1; i >= 0; i--)
				{
					switch (item->child(i)->data(0, kernel_item_role::type_role).toInt())
					{
					case kernel_item_type::leaf:
					{
						delete item->takeChild(i);
						break;
					}
					case kernel_item_type::volatile_node:
					{
						if (item->child(i)->childCount() <= 0)
						{
							// Cleanup volatile node if it has no children
							delete item->takeChild(i);
							break;
						}
						[[fallthrough]];
					}
					case kernel_item_type::solid_node:
					case kernel_item_type::node:
					case kernel_item_type::root:
					default:
					{
						clean_up_tree(item->child(i));
						break;
					}
					}
				}
			}
		};
		clean_up_tree(root);
	}

	const u32 total_memory_usage = dct->used;

	root->setText(0, qstr(fmt::format("Process 0x%08x: Total Memory Usage: 0x%x/0x%x (%0.2f/%0.2f MB)", process_getpid(), total_memory_usage, dct->size, 1. * total_memory_usage / (1024 * 1024)
		, 1. * dct->size / (1024 * 1024))));

	add_solid_node(m_tree, find_node(root, additional_nodes::process_info), qstr(fmt::format("Process Info, Sdk Version: 0x%08x, PPC SEG: %#x, SFO Category: %s", g_ps3_process_info.sdk_ver, g_ps3_process_info.ppc_seg, Emu.GetCat())));

	auto display_program_segments = [this](QTreeWidgetItem* tree, const ppu_module& m)
	{
		for (usz i = 0; i < m.segs.size(); i++)
		{
			const u32 addr = m.segs[i].addr;
			const u32 size = m.segs[i].size;

			add_leaf(tree, qstr(fmt::format("Segment %u: (0x%08x...0x%08x), Flags: 0x%x"
				, i, addr, addr + std::max<u32>(size, 1) - 1, m.segs[i].flags)));
		}
	};

	idm::select<lv2_obj>([&](u32 id, lv2_obj& obj)
	{
		const auto node = find_node(root, id >> 24);
		if (!node)
		{
			return;
		}

		switch (id >> 24)
		{
		case SYS_MEM_OBJECT:
		{
			auto& mem = static_cast<lv2_memory&>(obj);
			const f64 size_mb = mem.size * 1. / (1024 * 1024);

			if (mem.pshared)
			{
				add_leaf(node, qstr(fmt::format("Shared Mem 0x%08x: Size: 0x%x (%0.2f MB), Chunk: %s, Mappings: %u, Mem Container: %s, Key: %#llx", id, mem.size, size_mb, mem.align == 0x10000u ? "64K" : "1MB", +mem.counter, mem.ct->id, mem.key)));
				break;
			}

			add_leaf(node, qstr(fmt::format("Shared Mem 0x%08x: Size: 0x%x (%0.2f MB), Chunk: %s, Mem Container: %s, Mappings: %u", id, mem.size, size_mb, mem.align == 0x10000u ? "64K" : "1MB", mem.ct->id, +mem.counter)));
			break;
		}
		case SYS_MUTEX_OBJECT:
		{
			auto& mutex = static_cast<lv2_mutex&>(obj);
			add_leaf(node, qstr(fmt::format(u8"Mutex 0x%08x: “%s”, %s,%s Owner: %#x, Locks: %u, Key: %#llx, Conds: %u, Wq: %zu", id, lv2_obj::name64(mutex.name), mutex.protocol,
				mutex.recursive == SYS_SYNC_RECURSIVE ? " Recursive," : "", mutex.owner >> 1, +mutex.lock_count, mutex.key, mutex.cond_count, mutex.sq.size())));
			break;
		}
		case SYS_COND_OBJECT:
		{
			auto& cond = static_cast<lv2_cond&>(obj);
			add_leaf(node, qstr(fmt::format(u8"Cond 0x%08x: “%s”, %s, Mutex: 0x%08x, Key: %#llx, Wq: %u", id, lv2_obj::name64(cond.name), cond.mutex->protocol, cond.mtx_id, cond.key, +cond.waiters)));
			break;
		}
		case SYS_RWLOCK_OBJECT:
		{
			auto& rw = static_cast<lv2_rwlock&>(obj);
			const s64 val = rw.owner;
			add_leaf(node, qstr(fmt::format(u8"RW Lock 0x%08x: “%s”, %s, Owner: %#x(%d), Key: %#llx, Rq: %zu, Wq: %zu", id, lv2_obj::name64(rw.name), rw.protocol,
				std::max<s64>(0, val >> 1), -std::min<s64>(0, val >> 1), rw.key, rw.rq.size(), rw.wq.size())));
			break;
		}
		case SYS_INTR_TAG_OBJECT:
		{
			auto& tag = static_cast<lv2_int_tag&>(obj);
			const auto handler = tag.handler.get();

			if (lv2_obj::check(handler))
			{
				add_leaf(node, qstr(fmt::format("Intr Tag 0x%08x, Handler: 0x%08x", id, handler->id)));
				break;
			}

			add_leaf(node, qstr(fmt::format("Intr Tag 0x%08x, Handler: Unbound", id)));
			break;
		}
		case SYS_INTR_SERVICE_HANDLE_OBJECT:
		{
			auto& serv = static_cast<lv2_int_serv&>(obj);
			add_leaf(node, qstr(fmt::format("Intr Svc 0x%08x, PPU: 0x%07x, arg1: 0x%x, arg2: 0x%x", id, serv.thread->id, serv.arg1, serv.arg2)));
			break;
		}
		case SYS_EVENT_QUEUE_OBJECT:
		{
			auto& eq = static_cast<lv2_event_queue&>(obj);
			add_leaf(node, qstr(fmt::format(u8"Event Queue 0x%08x: “%s”, %s, %s, Key: %#llx, Events: %zu/%d, Wq: %zu", id, lv2_obj::name64(eq.name), eq.protocol,
				eq.type == SYS_SPU_QUEUE ? "SPU" : "PPU", eq.key, eq.events.size(), eq.size, eq.sq.size())));
			break;
		}
		case SYS_EVENT_PORT_OBJECT:
		{
			auto& ep = static_cast<lv2_event_port&>(obj);
			const auto type = ep.type == SYS_EVENT_PORT_LOCAL ? "LOCAL"sv : "IPC"sv;

			if (const auto queue = ep.queue.get(); lv2_obj::check(queue))
			{
				if (queue == idm::check_unlocked<lv2_obj, lv2_event_queue>(queue->id))
				{
					add_leaf(node, qstr(fmt::format("Event Port 0x%08x: %s, Name: %#llx, Event Queue (ID): 0x%08x", id, type, ep.name, queue->id)));
					break;
				}

				// This code is unused until multi-process is implemented
				if (queue == lv2_event_queue::find(queue->key).get())
				{
					add_leaf(node, qstr(fmt::format("Event Port 0x%08x: %s, Name: %#llx, Event Queue (IPC): %s", id, type, ep.name, queue->key)));
					break;
				}
			}

			add_leaf(node, qstr(fmt::format("Event Port 0x%08x: %s, Name: %#llx, Unbound", id, type, ep.name)));
			break;
		}
		case SYS_TRACE_OBJECT:
		{
			add_leaf(node, qstr(fmt::format("Trace 0x%08x", id)));
			break;
		}
		case SYS_SPUIMAGE_OBJECT:
		{
			auto& spi = static_cast<lv2_spu_image&>(obj);
			add_leaf(node, qstr(fmt::format("SPU Image 0x%08x, Entry: 0x%x, Segs: *0x%x, Num Segs: %d", id, spi.e_entry, spi.segs, spi.nsegs)));
			break;
		}
		case SYS_PRX_OBJECT:
		{
			auto& prx = static_cast<lv2_prx&>(obj);

			if (prx.segs.empty())
			{
				add_leaf(node, qstr(fmt::format("PRX 0x%08x: '%s' (HLE)", id, prx.name)));
				break;
			}

			const QString text = qstr(fmt::format("PRX 0x%08x: '%s'", id, prx.name));
			QTreeWidgetItem* prx_tree = add_solid_node(m_tree, node, text, text);
			display_program_segments(prx_tree, prx);
			break;
		}
		case SYS_SPUPORT_OBJECT:
		{
			add_leaf(node, qstr(fmt::format("SPU Port 0x%08x", id)));
			break;
		}
		case SYS_OVERLAY_OBJECT:
		{
			auto& ovl = static_cast<lv2_overlay&>(obj);
			const QString text = qstr(fmt::format("OVL 0x%08x: '%s'", id, ovl.name));
			QTreeWidgetItem* ovl_tree = add_solid_node(m_tree, node, text, text);
			display_program_segments(ovl_tree, ovl);
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
				case lwmutex_dead: owner_str = "dead"; break;
				case lwmutex_reserved: owner_str = "reserved"; break;
				default:
				{
					if (idm::check_unlocked<named_thread<ppu_thread>>(owner))
					{
						owner_str = fmt::format("0x%x", owner);
					}
					else
					{
						fmt::append(owner_str, " (0x%x)", owner);
					}

					break;
				}
				}
			}
			else
			{
				add_leaf(node, qstr(fmt::format(u8"LWMutex 0x%08x: “%s”, %s, Signal: %#x, Wq: %zu (Couldn't extract control data)", id, lv2_obj::name64(lwm.name), lwm.protocol, +lwm.signaled, lwm.sq.size())));
				break;
			}

			add_leaf(node, qstr(fmt::format(u8"LWMutex 0x%08x: “%s”, %s,%s Owner: %s, Locks: %u, Signal: %#x, Wq: %zu", id, lv2_obj::name64(lwm.name), lwm.protocol,
					(lwm_data.attribute & SYS_SYNC_RECURSIVE) ? " Recursive," : "", owner_str, lwm_data.recursive_count, +lwm.signaled, lwm.sq.size())));
			break;
		}
		case SYS_TIMER_OBJECT:
		{
			auto& timer = static_cast<lv2_timer&>(obj);

			sys_timer_information_t info;
			timer.get_information(info);

			add_leaf(node, qstr(fmt::format("Timer 0x%08x: State: %s, Period: 0x%llx, Next Expire: 0x%llx", id, info.timer_state ? "Running" : "Stopped"
				, info.period, info.next_expire)));
			break;
		}
		case SYS_SEMAPHORE_OBJECT:
		{
			auto& sema = static_cast<lv2_sema&>(obj);
			const auto val = +sema.val;
			add_leaf(node, qstr(fmt::format(u8"Sema 0x%08x: “%s”, %s, Count: %d/%d, Key: %#llx, Wq: %zu", id, lv2_obj::name64(sema.name), sema.protocol,
				std::max<s32>(val, 0), sema.max, sema.key, -std::min<s32>(val, 0))));
			break;
		}
		case SYS_LWCOND_OBJECT:
		{
			auto& lwc = static_cast<lv2_lwcond&>(obj);
			add_leaf(node, qstr(fmt::format(u8"LWCond 0x%08x: “%s”, %s, OG LWMutex: 0x%08x, Wq: %zu", id, lv2_obj::name64(lwc.name), lwc.protocol, lwc.lwid, +lwc.waiters)));
			break;
		}
		case SYS_EVENT_FLAG_OBJECT:
		{
			auto& ef = static_cast<lv2_event_flag&>(obj);
			add_leaf(node, qstr(fmt::format(u8"Event Flag 0x%08x: “%s”, %s, Type: 0x%x, Key: %#llx, Pattern: 0x%llx, Wq: %zu", id, lv2_obj::name64(ef.name), ef.protocol,
				ef.type, ef.key, ef.pattern.load(), +ef.waiters)));
			break;
		}
		default:
		{
			add_leaf(node, qstr(fmt::format("Unknown object 0x%08x", id)));
		}
		}
	});

	idm::select<sys_vm_t>([&](u32 /*id*/, sys_vm_t& vmo)
	{
		const u32 psize = vmo.psize;
		add_leaf(find_node(root, additional_nodes::virtual_memory), qstr(fmt::format("Virtual Mem 0x%08x: Virtual Size: 0x%x (%0.2f MB), Physical Size: 0x%x (%0.2f MB), Mem Container: %s", vmo.addr
			, vmo.size, vmo.size * 1. / (1024 * 1024), psize, psize * 1. / (1024 * 1024))), vmo.ct->id);
	});

	idm::select<lv2_socket>([&](u32 id, lv2_socket& sock)
	{
		add_leaf(find_node(root, additional_nodes::sockets), qstr(fmt::format("Socket %u: Type: %s, Family: %s, Wq: %zu", id, sock.type, sock.family, sock.queue.size())));
	});

	idm::select<lv2_memory_container>([&](u32 id, lv2_memory_container& container)
	{
		const u32 used = container.used;
		add_leaf(find_node(root, additional_nodes::memory_containers), qstr(fmt::format("Memory Container 0x%08x: Used: 0x%x/0x%x (%0.2f/%0.2f MB)", id, used, container.size, used * 1. / (1024 * 1024), container.size * 1. / (1024 * 1024))));
	});

	idm::select<named_thread<ppu_thread>>([&](u32 id, ppu_thread& ppu)
	{
		const auto func = ppu.last_function;
		const ppu_thread_status status = lv2_obj::ppu_state(&ppu, false);

		add_leaf(find_node(root, additional_nodes::ppu_threads), qstr(fmt::format(u8"PPU 0x%07x: “%s”, PRIO: %d, Joiner: %s, Status: %s, State: %s, %s func: “%s”", id, *ppu.ppu_tname.load(), +ppu.prio, ppu.joiner.load(), status, ppu.state.load()
			, ppu.current_function ? "In" : "Last", func ? func : "")));
	});

	idm::select<named_thread<spu_thread>>([&](u32 /*id*/, spu_thread& spu)
	{
		QTreeWidgetItem* spu_thread_tree = add_solid_node(m_tree, find_node(root, additional_nodes::spu_threads), qstr(fmt::format(u8"SPU 0x%07x: “%s”, State: %s, Type: %s", spu.lv2_id, *spu.spu_tname.load(), spu.state.load(), spu.get_type())));

		if (spu.get_type() == spu_type::threaded)
		{
			reader_lock lock(spu.group->mutex);

			bool has_connected_ports = false;
			const auto first_spu = spu.group->threads[0].get();

			// Always show information of the first thread in group
			// Or if information differs from that thread
			if (&spu == first_spu || std::any_of(std::begin(spu.spup), std::end(spu.spup), [&](const auto& port)
			{
				// Flag to avoid reporting information if no SPU ports are connected
				has_connected_ports |= lv2_obj::check(port);

				// Check if ports do not match with the first thread
				return port != first_spu->spup[&port - spu.spup];
			}))
			{
				for (const auto& port : spu.spup)
				{
					if (lv2_obj::check(port))
					{
						add_leaf(spu_thread_tree, qstr(fmt::format("SPU Port %u: Queue ID: 0x%08x", &port - spu.spup, port->id)));
					}
				}
			}
			else if (has_connected_ports)
			{
				// Avoid duplication of information between threads which is common
				add_leaf(spu_thread_tree, qstr(fmt::format("SPU Ports: As SPU 0x%07x", first_spu->lv2_id)));
			}

			for (const auto& [key, queue] : spu.spuq)
			{
				if (lv2_obj::check(queue))
				{
					add_leaf(spu_thread_tree, qstr(fmt::format("SPU Queue: Queue ID: 0x%08x, Key: 0x%x", queue->id, key)));
				}
			}
		}
		else
		{
			for (const auto& ctrl : spu.int_ctrl)
			{
				if (lv2_obj::check(ctrl.tag))
				{
					add_leaf(spu_thread_tree, qstr(fmt::format("Interrupt Tag %u: ID: 0x%x, Mask: 0x%x, Status: 0x%x", &ctrl - spu.int_ctrl.data(), ctrl.tag->id, +ctrl.mask, +ctrl.stat)));
				}
			}
		}
	});

	idm::select<lv2_spu_group>([&](u32 id, lv2_spu_group& tg)
	{
		QTreeWidgetItem* spu_tree = add_solid_node(m_tree, find_node(root, additional_nodes::spu_thread_groups), qstr(fmt::format(u8"SPU Group 0x%07x: “%s”, Status = %s, Priority = %d, Type = 0x%x", id, tg.name, tg.run_state.load(), +tg.prio, tg.type)));

		if (tg.name.ends_with("CellSpursKernelGroup"sv))
		{
			vm::ptr<CellSpurs> pspurs{};

			for (const auto& thread : tg.threads)
			{
				if (thread)
				{
					// Find SPURS structure address
					const u64 arg = tg.args[thread->index][1];

					if (!pspurs)
					{
						if (arg < u32{umax} && arg % 0x80 == 0 && vm::check_addr(arg, vm::page_readable, pspurs.size()))
						{
							pspurs.set(static_cast<u32>(arg));
						}
						else
						{
							break;
						}
					}
					else if (pspurs.addr() != arg)
					{
						pspurs = {};
						break;
					}
				}
			}

			CellSpurs spurs{};

			if (pspurs && pspurs.try_read(spurs))
			{
				const QString branch_name = tr("SPURS %1").arg(pspurs.addr());
				const u32 wklEnabled = spurs.wklEnabled;
				QTreeWidgetItem* spurs_tree = add_solid_node(m_tree, spu_tree, branch_name, qstr(fmt::format("SPURS, Instance: *0x%x, LWMutex: 0x%x, LWCond: 0x%x, wklEnabled: 0x%x"
					, pspurs, spurs.mutex.sleep_queue, spurs.cond.lwcond_queue, wklEnabled)));

				const u32 signal_mask = u32{spurs.wklSignal1} << 16 | spurs.wklSignal2;

				for (u32 wid = 0; wid < CELL_SPURS_MAX_WORKLOAD2; wid++)
				{
					if (!(wklEnabled & (0x80000000u >> wid)))
					{
						continue;
					}

					const auto state = spurs.wklState(wid).raw();

					if (state == SPURS_WKL_STATE_NON_EXISTENT)
					{
						continue;
					}

					const u32 ready_count = spurs.readyCount(wid);
					const auto& name = spurs.wklName(wid);
					const u8 evt = spurs.wklEvent(wid);
					const u8 status = spurs.wklStatus(wid);
					const auto has_signal = (signal_mask & (0x80000000u >> wid)) ? "Signalled"sv : "No Signal"sv;

					QTreeWidgetItem* wkl_tree = add_solid_node(m_tree, spurs_tree, branch_name + qstr(fmt::format(" Work.%u", wid)), qstr(fmt::format("Work.%u, class: %s, %s, %s, Status: %#x, Event: %#x, %s, ReadyCnt: %u", wid, +name.nameClass, +name.nameInstance, state, status, evt, has_signal, ready_count)));

					auto contention = [&](u8 v)
					{
						if (wid >= CELL_SPURS_MAX_WORKLOAD)
							return (v >> 4);
						else
							return v & 0xf;
					};

					const auto& winfo = spurs.wklInfo(wid);
					add_leaf(wkl_tree, qstr(fmt::format("Contention: %u/%u (pending: %u), Image: *0x%x (size: 0x%x, arg: 0x%x), Priority (BE64): %016x", contention(spurs.wklCurrentContention[wid % 16])
						, contention(spurs.wklMaxContention[wid % 16]), contention(spurs.wklPendingContention[wid % 16]), +winfo.addr, winfo.size, winfo.arg, std::bit_cast<be_t<u64>>(winfo.priority))));
				}

				add_leaf(spurs_tree, qstr(fmt::format("Handler Info: PPU0: 0x%x, PPU1: 0x%x, DirtyState: %u, Waiting: %u, Exiting: %u", spurs.ppu0, spurs.ppu1
					, +spurs.handlerDirty, +spurs.handlerWaiting, +spurs.handlerExiting)));
			}
			else
			{
				// TODO: Might be old CellSpurs structure which is smaller
			}
		}
	});

	QTreeWidgetItem* rsx_context_node = find_node(root, additional_nodes::rsx_contexts);

	do
	{
		// Currently a single context is supported at a time
		const auto rsx = rsx::get_current_renderer();

		if (!rsx)
		{
			break;
		}

		const auto base = rsx->dma_address;

		if (!base)
		{
			break;
		}

		const QString branch_name = "RSX Context 0x55555555";
		QTreeWidgetItem* rsx_tree = add_solid_node(m_tree, rsx_context_node, branch_name,
			branch_name + qstr(fmt::format(u8", Local Size: %u MB, Base Addr: 0x%x, Device Addr: 0x%x, Handlers: 0x%x", rsx->local_mem_size >> 20, base, rsx->device_addr, +vm::_ref<RsxDriverInfo>(rsx->driver_info).handlers)));

		QTreeWidgetItem* io_tree = add_volatile_node(m_tree, rsx_tree, tr("IO-EA Table"));
		QTreeWidgetItem* zc_tree = add_volatile_node(m_tree, rsx_tree, tr("Zcull Bindings"));
		QTreeWidgetItem* db_tree = add_volatile_node(m_tree, rsx_tree, tr("Display Buffers"));

		decltype(rsx->iomap_table) table;
		decltype(rsx->display_buffers) dbs;
		decltype(rsx->zculls) zcs;
		{
			std::lock_guard lock(rsx->sys_rsx_mtx);
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
					add_leaf(io_tree, qstr(fmt::format("IO: %08x, EA: %08x, Size: %uMB", first_io, first_ea, size_block)));
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
					add_leaf(io_tree, qstr(fmt::format("IO: %08x, EA: %08x, Size: %uMB", first_io, first_ea, old_block_size)));
					size_block = 1;
					first_ea = addr;
					first_io = (i - 1) << 20;
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
				add_leaf(zc_tree, qstr(fmt::format("O: %07x, W: %u, H: %u, Zformat: 0x%x, AAformat: 0x%x, Dir: 0x%x, sFunc: 0x%x, sRef: %02x, sMask: %02x"
					, zc.offset, zc.height, zc.width, zc.zFormat, zc.aaFormat, zc.zcullDir, zc.sFunc, zc.sRef, zc.sMask)));
			}
		}

		for (const auto& db : dbs)
		{
			if (db.valid())
			{
				add_leaf(db_tree, qstr(fmt::format("Offset: %07x, Width: %u, Height: %u, Pitch: %u"
					, db.offset, db.height, db.width, db.pitch)));
			}
		}
	}
	while (false);

	idm::select<lv2_fs_object>([&](u32 id, lv2_fs_object& fo)
	{
		const std::string str = fmt::format("FD %u: %s", id, [&]() -> std::string
		{
			if (idm::check_unlocked<lv2_fs_object, lv2_file>(id))
			{
				return fmt::format("%s", static_cast<lv2_file&>(fo));
			}

			if (idm::check_unlocked<lv2_fs_object, lv2_dir>(id))
			{
				return fmt::format("%s", static_cast<lv2_dir&>(fo));
			}

			return "Unknown object!";
		}());

		add_leaf(find_node(root, additional_nodes::file_descriptors), qstr(str));
	});

	std::function<int(QTreeWidgetItem*)> final_touches;
	final_touches = [&final_touches](QTreeWidgetItem* item) -> int
	{
		int visible_children = 0;

		for (int i = 0; i < item->childCount(); i++)
		{
			auto node = item->child(i);

			if (!node)
			{
				continue;
			}

			switch (const int type = node->data(0, kernel_item_role::type_role).toInt())
			{
			case kernel_item_type::leaf:
			{
				node->setHidden(false);
				break;
			}
			case kernel_item_type::node:
			case kernel_item_type::solid_node:
			case kernel_item_type::volatile_node:
			{
				const int count = final_touches(node);

				if (count > 0)
				{
					// Append count
					node->setText(0, node->text(0) + qstr(fmt::format(" (%zu)", count)));

					// Expand if necessary
					node->setExpanded(node->data(0, kernel_item_role::expanded_role).toBool());
				}

				// Hide node if it has no children
				node->setHidden(type != kernel_item_type::solid_node && count <= 0);
				break;
			}
			case kernel_item_type::root:
			default:
			{
				break;
			}
			}

			if (!node->isHidden())
			{
				visible_children++;
			}
		}

		return visible_children;
	};
	final_touches(root);
	root->setExpanded(true);
}
