#include "breakpoint_list.h"
#include "breakpoint_handler.h"

#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"

#include <QMenu>

constexpr auto qstr = QString::fromStdString;

breakpoint_list::breakpoint_list(QWidget* parent, breakpoint_handler* handler) : QListWidget(parent), m_breakpoint_handler(handler)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// connects
	connect(this, &QListWidget::itemDoubleClicked, this, &breakpoint_list::OnBreakpointListDoubleClicked);
	connect(this, &QListWidget::customContextMenuRequested, this, &breakpoint_list::OnBreakpointListRightClicked);
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
		u32 loc = currentItem->data(Qt::UserRole).value<u32>();
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

void breakpoint_list::AddBreakpoint(u32 pc)
{
	m_breakpoint_handler->AddBreakpoint(pc);

	m_disasm->disasm(pc);

	QString text = qstr(m_disasm->last_opcode);
	text.remove(10, 13);

	QListWidgetItem* breakpoint_item = new QListWidgetItem(text);
	breakpoint_item->setForeground(m_text_color_bp);
	breakpoint_item->setBackground(m_color_bp);
	breakpoint_item->setData(Qt::UserRole, pc);
	addItem(breakpoint_item);

	Q_EMIT RequestShowAddress(pc);
}

/**
* If breakpoint exists, we remove it, else add new one.  Yeah, it'd be nicer from a code logic to have it be set/reset.  But, that logic has to happen somewhere anyhow.
*/
void breakpoint_list::HandleBreakpointRequest(u32 loc)
{
	if (!m_cpu || m_cpu->id_type() != 1 || !vm::check_addr(loc, vm::page_allocated | vm::page_executable))
	{
		// TODO: SPU breakpoints
		return;
	}

	if (m_breakpoint_handler->HasBreakpoint(loc))
	{
		RemoveBreakpoint(loc);
	}
	else
	{
		AddBreakpoint(loc);
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

	QMenu* menu = new QMenu();

	if (selectedItems().count() == 1)
	{
		QAction* rename_action = menu->addAction(tr("&Rename"));
		connect(rename_action, &QAction::triggered, this, [this]()
		{
			QListWidgetItem* current_item = selectedItems().first();
			current_item->setFlags(current_item->flags() | Qt::ItemIsEditable);
			editItem(current_item);
		});
		menu->addSeparator();
	}

	QAction* delete_action = new QAction(tr("&Delete"), this);
	delete_action->setShortcut(Qt::Key_Delete);
	delete_action->setShortcutContext(Qt::WidgetShortcut);
	connect(delete_action, &QAction::triggered, this, &breakpoint_list::OnBreakpointListDelete);
	menu->addAction(delete_action);

	menu->exec(viewport()->mapToGlobal(pos));
}

void breakpoint_list::OnBreakpointListDelete()
{
	for (int i = selectedItems().count() - 1; i >= 0; i--)
	{
		RemoveBreakpoint(item(i)->data(Qt::UserRole).value<u32>());
	}
}
