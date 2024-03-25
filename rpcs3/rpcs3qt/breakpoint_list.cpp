#include "breakpoint_list.h"
#include "breakpoint_handler.h"

#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"

#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>


constexpr auto qstr = QString::fromStdString;

extern bool is_using_interpreter(u32 id_type);

breakpoint_list::breakpoint_list(QWidget* parent, breakpoint_handler* handler) : QListWidget(parent), m_ppu_breakpoint_handler(handler)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	connect(this, &QListWidget::itemDoubleClicked, this, &breakpoint_list::OnBreakpointListDoubleClicked);
	connect(this, &QListWidget::customContextMenuRequested, this, &breakpoint_list::OnBreakpointListRightClicked);

	m_delete_action = new QAction(tr("&Delete"), this);
	m_delete_action->setShortcut(Qt::Key_Delete);
	m_delete_action->setShortcutContext(Qt::WidgetShortcut);
	connect(m_delete_action, &QAction::triggered, this, &breakpoint_list::OnBreakpointListDelete);
	addAction(m_delete_action);

	// Hide until used in order to allow as much space for registers panel as possible
	// hide();
}

/**
 * It's unfortunate I need a method like this to sync these.  Should ponder a cleaner way to do this.
 */
void breakpoint_list::UpdateCPUData(cpu_thread* cpu, CPUDisAsm* disasm)
{
	m_cpu    = cpu;
	m_disasm = disasm;
}

void breakpoint_list::ClearBreakpoints()
{
	while (count())
	{
		auto* currentItem = takeItem(0);
		const u32 loc     = currentItem->data(Qt::UserRole).value<u32>();
		m_ppu_breakpoint_handler->RemoveBreakpoint(loc);
		delete currentItem;
	}

	// hide();
}

void breakpoint_list::RemoveBreakpoint(u32 addr)
{
	m_ppu_breakpoint_handler->RemoveBreakpoint(addr);

	for (int i = 0; i < count(); i++)
	{
		QListWidgetItem* currentItem = item(i);

		if (currentItem->data(Qt::UserRole).value<u32>() == addr)
		{
			delete takeItem(i);
			break;
		}
	}

	// if (!count())
	// {
	// 	hide();
	// }
}

bool breakpoint_list::AddBreakpoint(u32 pc, bs_t<breakpoint_types> type)
{
	if (!m_ppu_breakpoint_handler->AddBreakpoint(pc, type))
	{
		return false;
	}

	QString breakpoint_item_text;
	if (type == breakpoint_types::bp_exec)
	{
		m_disasm->disasm(m_disasm->dump_pc = pc);
		breakpoint_item_text = qstr(m_disasm->last_opcode);
		breakpoint_item_text.remove(10, 13);
	}
	else if (type == breakpoint_types::bp_read)
	{
		breakpoint_item_text = QString("BPMR:  0x%1").arg(pc, 8, 16, QChar('0'));
	}
	else if (type == breakpoint_types::bp_write)
	{
		breakpoint_item_text = QString("BPMW:  0x%1").arg(pc, 8, 16, QChar('0'));
	}
	else if (type == (breakpoint_types::bp_read + breakpoint_types::bp_write))
	{
		breakpoint_item_text = QString("BPMRW: 0x%1").arg(pc, 8, 16, QChar('0'));
	}

	QListWidgetItem* breakpoint_item = new QListWidgetItem(breakpoint_item_text);
	breakpoint_item->setForeground(m_text_color_bp);
	breakpoint_item->setBackground(m_color_bp);
	breakpoint_item->setData(Qt::UserRole, pc);
	addItem(breakpoint_item);

	show();

	return true;
}

/**
 * If breakpoint exists, we remove it, else add new one.  Yeah, it'd be nicer from a code logic to have it be set/reset.  But, that logic has to happen somewhere anyhow.
 */
void breakpoint_list::HandleBreakpointRequest(u32 loc, bool only_add)
{
	if (!m_cpu || m_cpu->state & cpu_flag::exit)
	{
		return;
	}

	if (!is_using_interpreter(m_cpu->id_type()))
	{
		QMessageBox::warning(this, tr("Interpreters-Only Feature!"), tr("Cannot set breakpoints on non-interpreter decoders."));
		return;
	}

	switch (m_cpu->id_type())
	{
	case 2:
	{
		if (loc >= SPU_LS_SIZE || loc % 4)
		{
			QMessageBox::warning(this, tr("Invalid Memory For Breakpoints!"), tr("Cannot set breakpoints on non-SPU executable memory!"));
			return;
		}

		const auto spu = static_cast<spu_thread*>(m_cpu);
		auto& list = spu->local_breakpoints;

		if (list[loc / 4].test_and_invert())
		{
			if (std::none_of(list.begin(), list.end(), [](auto& val){ return val.load(); }))
			{
				spu->has_active_local_bps = false;
			}
		}
		else
		{
			if (!spu->has_active_local_bps.exchange(true))
			{
				spu->state.atomic_op([](bs_t<cpu_flag>& flags)
				{
					if (flags & cpu_flag::pending)
					{
						flags += cpu_flag::pending_recheck;
					}
					else
					{
						flags += cpu_flag::pending;
					}
				});
			}
		}

		return;
	}
	case 1: break;
	default:
		QMessageBox::warning(this, tr("Unimplemented Breakpoints For Thread Type!"), tr("Cannot set breakpoints on a thread not an PPU/SPU currently, sorry."));
		return;
	}

	if (!vm::check_addr(loc, vm::page_executable))
	{
		QMessageBox::warning(this, tr("Invalid Memory For Breakpoints!"), tr("Cannot set breakpoints on non-executable memory!"));
		return;
	}

	if (m_ppu_breakpoint_handler->HasBreakpoint(loc, breakpoint_types::bp_exec))
	{
		if (!only_add)
		{
			RemoveBreakpoint(loc);
		}
	}
	else
	{
		if (!AddBreakpoint(loc, breakpoint_types::bp_exec))
		{
			QMessageBox::warning(this, tr("Unknown error while setting breakpoint!"), tr("Failed to set breakpoints."));
			return;
		}
	}
}

void breakpoint_list::OnBreakpointListDoubleClicked()
{
	const u32 address = currentItem()->data(Qt::UserRole).value<u32>();
	Q_EMIT RequestShowAddress(address);
}

void breakpoint_list::OnBreakpointListRightClicked(const QPoint& pos)
{
	m_context_menu = new QMenu();

	if (selectedItems().count() == 1)
	{
		QAction* rename_action = m_context_menu->addAction(tr("&Rename"));
		connect(rename_action, &QAction::triggered, this, [this]()
			{
				QListWidgetItem* current_item = selectedItems().first();
				current_item->setFlags(current_item->flags() | Qt::ItemIsEditable);
				editItem(current_item);
			});
		m_context_menu->addSeparator();
	}

	if (selectedItems().count() >= 1)
	{
		m_context_menu->addAction(m_delete_action);
	}

	QAction* m_addbp = new QAction(tr("Add Breakpoint"), this);
	connect(m_addbp, &QAction::triggered, this, &breakpoint_list::ShowAddBreakpointWindow);
	m_context_menu->addAction(m_addbp);

	QAction* m_tglbpmbreak = new QAction(m_ppu_breakpoint_handler->IsBreakOnBPM() ? tr("Disable BPM") : tr("Enable BPM"), this);
	connect(m_tglbpmbreak, &QAction::triggered, [&]
		{
			m_ppu_breakpoint_handler->SetBreakOnBPM(!m_ppu_breakpoint_handler->IsBreakOnBPM());
		});
	m_context_menu->addAction(m_tglbpmbreak);

	m_context_menu->exec(viewport()->mapToGlobal(pos));
	m_context_menu->deleteLater();
	m_context_menu = nullptr;
}

void breakpoint_list::OnBreakpointListDelete()
{
	for (int i = selectedItems().count() - 1; i >= 0; i--)
	{
		RemoveBreakpoint(::at32(selectedItems(), i)->data(Qt::UserRole).value<u32>());
	}

	if (m_context_menu)
	{
		m_context_menu->close();
	}
}

void breakpoint_list::ShowAddBreakpointWindow()
{
	QDialog* dlg = new QDialog(this);

	dlg->setWindowTitle(tr("Add a breakpoint"));
	dlg->setModal(true);

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QHBoxLayout* hbox_top = new QHBoxLayout();
	QLabel* l_address     = new QLabel(tr("Address"));
	QLineEdit* t_address  = new QLineEdit();
	t_address->setPlaceholderText(tr("Address here"));
	t_address->setFocus();

	hbox_top->addWidget(l_address);
	hbox_top->addWidget(t_address);
	vbox_panel->addLayout(hbox_top);

	QHBoxLayout* hbox_bot = new QHBoxLayout();
	QComboBox* co_bptype  = new QComboBox(this);
	QStringList qstr_breakpoint_types;
	qstr_breakpoint_types << tr("Memory Read")
						  << tr("Memory Write")
						  << tr("Memory Read&Write")
						  << tr("Execution");
	co_bptype->addItems(qstr_breakpoint_types);

	hbox_bot->addWidget(co_bptype);
	vbox_panel->addLayout(hbox_bot);

	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	button_box->button(QDialogButtonBox::Ok)->setText(tr("Add"));

	hbox_buttons->addWidget(button_box);
	vbox_panel->addLayout(hbox_buttons);

	dlg->setLayout(vbox_panel);

	connect(button_box, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

	dlg->move(QCursor::pos());

	if (dlg->exec() == QDialog::Accepted)
	{
		if (!t_address->text().isEmpty())
		{
			u32 address = std::stoul(t_address->text().toStdString(), nullptr, 16);
			bs_t<breakpoint_types> bp_t;
			switch (co_bptype->currentIndex())
			{
			case 0:
				bp_t = breakpoint_types::bp_read;
				break;
			case 1:
				bp_t = breakpoint_types::bp_write;
				break;
			case 2:
				bp_t = breakpoint_types::bp_read + breakpoint_types::bp_write;
				break;
			case 3:
				bp_t = breakpoint_types::bp_exec;
				break;
			default:
				bp_t = {};
				break;
			}

			if (bp_t)
				AddBreakpoint(address, bp_t);
		}
	}

	dlg->deleteLater();
}
