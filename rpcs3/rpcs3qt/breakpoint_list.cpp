#include "breakpoint_list.h"
#include "breakpoint_handler.h"

#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"

#include <QMenu>
#include <QMessageBox>

constexpr auto qstr = QString::fromStdString;

extern bool is_using_interpreter(u32 id_type);

breakpoint_list::breakpoint_list(QWidget* parent, breakpoint_handler* handler) : QListWidget(parent), m_breakpoint_handler(handler)
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
}

/**
* It's unfortunate I need a method like this to sync these.  Should ponder a cleaner way to do this.
*/
void breakpoint_list::UpdateCPUData(cpu_thread* cpu, CPUDisAsm* disasm)
{
	m_cpu = cpu;
	m_disasm = disasm;
}

void breakpoint_list::ClearBreakpoints()
{
	while (count())
	{
		auto* currentItem = takeItem(0);
		const u32 loc = currentItem->data(Qt::UserRole).value<u32>();
		m_breakpoint_handler->RemoveBreakpoint(loc);
		delete currentItem;
	}
}

void breakpoint_list::RemoveBreakpoint(u32 addr)
{
	m_breakpoint_handler->RemoveBreakpoint(addr);

	for (int i = 0; i < count(); i++)
	{
		QListWidgetItem* currentItem = item(i);

		if (currentItem->data(Qt::UserRole).value<u32>() == addr)
		{
			delete takeItem(i);
			break;
		}
	}

	Q_EMIT RequestShowAddress(addr);
}

bool breakpoint_list::AddBreakpoint(u32 pc)
{
	if (!m_breakpoint_handler->AddBreakpoint(pc))
	{
		return false;
	}

	m_disasm->disasm(pc);

	QString text = qstr(m_disasm->last_opcode);
	text.remove(10, 13);

	QListWidgetItem* breakpoint_item = new QListWidgetItem(text);
	breakpoint_item->setForeground(m_text_color_bp);
	breakpoint_item->setBackground(m_color_bp);
	breakpoint_item->setData(Qt::UserRole, pc);
	addItem(breakpoint_item);

	Q_EMIT RequestShowAddress(pc);

	return true;
}

/**
* If breakpoint exists, we remove it, else add new one.  Yeah, it'd be nicer from a code logic to have it be set/reset.  But, that logic has to happen somewhere anyhow.
*/
void breakpoint_list::HandleBreakpointRequest(u32 loc)
{
	if (!m_cpu || m_cpu->state & cpu_flag::exit)
	{
		return;
	}

	if (m_cpu->id_type() != 1)
	{
		// TODO: SPU breakpoints
		QMessageBox::warning(this, tr("Unimplemented Breakpoints For Thread Type!"), tr("Cannot set breakpoints on non-PPU thread currently, sorry."));
		return;
	}

	if (!vm::check_addr(loc, vm::page_executable))
	{
		QMessageBox::warning(this, tr("Invalid Memory For Breakpoints!"), tr("Cannot set breakpoints on non-executable memory!"));
		return;
	}

	if (!is_using_interpreter(m_cpu->id_type()))
	{
		QMessageBox::warning(this, tr("Interpreters-Only Feature!"), tr("Cannot set breakpoints on non-interpreter decoders."));
		return;
	}

	if (m_breakpoint_handler->HasBreakpoint(loc))
	{
		RemoveBreakpoint(loc);
	}
	else
	{
		if (!AddBreakpoint(loc))
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

void breakpoint_list::OnBreakpointListRightClicked(const QPoint &pos)
{
	if (!itemAt(pos))
	{
		return;
	}

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

	m_context_menu->addAction(m_delete_action);
	m_context_menu->exec(viewport()->mapToGlobal(pos));
	m_context_menu->deleteLater();
	m_context_menu = nullptr;
}

void breakpoint_list::OnBreakpointListDelete()
{
	for (int i = selectedItems().count() - 1; i >= 0; i--)
	{
		RemoveBreakpoint(selectedItems().at(i)->data(Qt::UserRole).value<u32>());
	}

	if (m_context_menu)
	{
		m_context_menu->close();
	}
}
