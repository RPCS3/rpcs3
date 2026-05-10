#include "call_stack_list.h"

#include "Utilities/StrFmt.h"

#include <QKeyEvent>
#include <QMouseEvent>

call_stack_list::call_stack_list(QWidget* parent) : QListWidget(parent)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// connects
	connect(this, &QListWidget::itemDoubleClicked, this, &call_stack_list::ShowItemAddress);

	// Hide until used in order to allow as much space for registers panel as possible
	hide();
}

void call_stack_list::keyPressEvent(QKeyEvent* event)
{
	QListWidget::keyPressEvent(event);
	event->ignore(); // Propagate the event to debugger_frame

	if (!event->modifiers() && event->key() == Qt::Key_Return)
	{
		ShowItemAddress();
	}
}

void call_stack_list::HandleUpdate(const std::vector<std::pair<u32, u32>>& call_stack)
{
	clear();

	for (const auto& addr : call_stack)
	{
		const QString text = QString::fromStdString(fmt::format("0x%08llx (sp=0x%08llx)", addr.first, addr.second));
		QListWidgetItem* call_stack_item = new QListWidgetItem(text);
		call_stack_item->setData(Qt::UserRole, { addr.first });
		addItem(call_stack_item);
	}

	setVisible(!call_stack.empty());
}

void call_stack_list::ShowItemAddress()
{
	if (QListWidgetItem* call_stack_item = currentItem())
	{
		const u32 address = call_stack_item->data(Qt::UserRole).value<u32>();
		Q_EMIT RequestShowAddress(address);
	}
}

void call_stack_list::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (!ev) return;

	// Qt's itemDoubleClicked signal doesn't distinguish between mouse buttons and there is no simple way to get the pressed button.
	// So we have to ignore this event when another button is pressed.
	if (ev->button() != Qt::LeftButton)
	{
		ev->ignore();
		return;
	}

	QListWidget::mouseDoubleClickEvent(ev);
}
