#include "call_stack_list.h"

#include "Utilities/StrFmt.h"

constexpr auto qstr = QString::fromStdString;

call_stack_list::call_stack_list(QWidget* parent) : QListWidget(parent)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// connects
	connect(this, &QListWidget::itemDoubleClicked, this, &call_stack_list::OnCallStackListDoubleClicked);
}

void call_stack_list::HandleUpdate(const std::vector<std::pair<u32, u32>>& call_stack)
{
	clear();

	for (const auto& addr : call_stack)
	{
		const QString text = qstr(fmt::format("0x%08llx (sp=0x%08llx)", addr.first, addr.second));
		QListWidgetItem* call_stack_item = new QListWidgetItem(text);
		call_stack_item->setData(Qt::UserRole, { addr.first });
		addItem(call_stack_item);
	}
}

void call_stack_list::OnCallStackListDoubleClicked()
{
	const u32 address = currentItem()->data(Qt::UserRole).value<u32>();
	Q_EMIT RequestShowAddress(address);
}
