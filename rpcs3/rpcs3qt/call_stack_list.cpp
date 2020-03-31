﻿#include "call_stack_list.h"

constexpr auto qstr = QString::fromStdString;

call_stack_list::call_stack_list(QWidget* parent) : QListWidget(parent)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// connects
	connect(this, &QListWidget::itemDoubleClicked, this, &call_stack_list::OnCallStackListDoubleClicked);
}

void call_stack_list::UpdateCPUData(std::weak_ptr<cpu_thread> cpu, std::shared_ptr<CPUDisAsm> disasm)
{
	this->cpu = cpu;
}

void call_stack_list::HandleUpdate(std::vector<u32> call_stack)
{
	clear();

	for (auto addr : call_stack)
	{
		const QString call_stack_item_text = qstr(fmt::format("0x%08llx", addr));
		QListWidgetItem* callStackItem = new QListWidgetItem(call_stack_item_text);
		callStackItem->setData(Qt::UserRole, { addr });
		addItem(callStackItem);
	}
}

void call_stack_list::OnCallStackListDoubleClicked()
{
	const u32 address = currentItem()->data(Qt::UserRole).value<u32>();
	Q_EMIT RequestShowAddress(address);
}
