#include "stdafx.h"
#include "save_data_list_dialog.h"
#include "save_data_info_dialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QDesktopWidget>
#include <QApplication>

constexpr auto qstr = QString::fromStdString;

//Show up the savedata list, either to choose one to save/load or to manage saves.
//I suggest to use function callbacks to give save data list or get save data entry. (Not implemented or stubbed)
save_data_list_dialog::save_data_list_dialog(const std::vector<SaveDataEntry>& entries, s32 focusedEntry, u32 op, vm::ptr<CellSaveDataListSet> listSet, QWidget* parent)
	: QDialog(parent), m_save_entries(entries), m_entry(selection_code::new_save), m_entry_label(nullptr)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	if (op >= 8)
	{
		setWindowTitle(tr("Save Data Interface (Delete)"));
	}
	else if (op & 1)
	{
		setWindowTitle(tr("Save Data Interface (Load)"));
	}
	else
	{
		setWindowTitle(tr("Save Data Interface (Save)"));
	}

	setMinimumSize(QSize(400, 400));

	m_gui_settings.reset(new gui_settings());

	// Table
	m_list = new QTableWidget(this);

	//m_list->setItemDelegate(new table_item_delegate(this)); // to get rid of cell selection rectangles include "table_item_delegate.h"
	m_list->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_list->setColumnCount(4);
	m_list->setHorizontalHeaderLabels(QStringList() << tr("Title") << tr("Subtitle") << tr("Save ID") << tr("Entry Notes"));

	// Button Layout
	QHBoxLayout* hbox_action = new QHBoxLayout();

	if (entries.size() > 0)
	{ // If there are no entries, don't add the selection widget or the selection label to the UI.
		QPushButton *push_select = new QPushButton(tr("&Select Entry"), this);
		connect(push_select, &QAbstractButton::clicked, this, &save_data_list_dialog::accept);
		push_select->setAutoDefault(true);
		push_select->setDefault(true);
		hbox_action->addWidget(push_select);

		m_entry_label = new QLabel(this);
		UpdateSelectionLabel();
	}

	if (listSet->newData)
	{
		QPushButton *saveNewEntry = new QPushButton(tr("Save New Entry"), this);
		connect(saveNewEntry, &QAbstractButton::clicked, this, [&]()
		{
			m_entry = selection_code::new_save;
			accept();
		});
		hbox_action->addWidget(saveNewEntry);
	}

	hbox_action->addStretch();

	QPushButton *push_cancel = new QPushButton(tr("&Cancel"), this);
	push_cancel->setAutoDefault(false);
	hbox_action->addWidget(push_cancel);

	// events
	connect(push_cancel, &QAbstractButton::clicked, this, &save_data_list_dialog::close);
	connect(m_list, &QTableWidget::itemDoubleClicked, this, &save_data_list_dialog::OnEntryInfo);
	connect(m_list, &QTableWidget::currentCellChanged, this, [&](int cr, int cc, int pr, int pc)
	{
		m_entry = cr;
		UpdateSelectionLabel();
		Q_UNUSED(cr); Q_UNUSED(pr); Q_UNUSED(pc);
	});

	connect(m_list->horizontalHeader(), &QHeaderView::sectionClicked, this, &save_data_list_dialog::OnSort);

	// main layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addWidget(m_list);
	if (m_entry_label != nullptr)
	{
		vbox_main->addWidget(m_entry_label);
	}
	vbox_main->addLayout(hbox_action);
	setLayout(vbox_main);

	UpdateList();

	connect(m_list, &QTableWidget::cellChanged, [&](int row, int col)
	{
		int originalIndex = m_list->item(row, 0)->data(Qt::UserRole).toInt();
		SaveDataEntry originalEntry = m_save_entries[originalIndex];
		QString originalDirName = qstr(originalEntry.dirName);
		QVariantMap currNotes = m_gui_settings->GetValue(gui::m_saveNotes).toMap();
		currNotes[originalDirName] = m_list->item(row, col)->text();
		m_gui_settings->SetValue(gui::m_saveNotes, currNotes);
	});

	m_list->setCurrentCell(focusedEntry, 0);
}

void save_data_list_dialog::UpdateSelectionLabel()
{
	if (m_entry_label != nullptr)
	{
		if (m_list->currentRow() == -1)
		{
			m_entry_label->setText(tr("Currently Selected: None"));
		}
		else
		{
			int entry = m_list->item(m_list->currentRow(), 0)->data(Qt::UserRole).toInt();
			m_entry_label->setText(tr("Currently Selected: ") + qstr(m_save_entries[entry].dirName));
		}
	}
}

s32 save_data_list_dialog::GetSelection()
{
	int res = result();
	if (res == QDialog::Accepted)
	{
		if (m_entry == selection_code::new_save)
		{ // Save new entry
			return selection_code::new_save;
		}
		return m_list->item(m_entry, 0)->data(Qt::UserRole).toInt();
	}

	// Cancel is pressed. May figure out proper cellsavedata code to use later.
	return selection_code::canceled;
}

void save_data_list_dialog::OnSort(int logicalIndex)
{
	if (logicalIndex >= 0)
	{
		if (logicalIndex == m_sort_column)
		{
			m_sort_ascending ^= true;
		}
		else
		{
			m_sort_ascending = true;
		}
		Qt::SortOrder sort_order = m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
		m_list->sortByColumn(m_sort_column, sort_order);
		m_sort_column = logicalIndex;
	}
}

//Display info dialog directly.
void save_data_list_dialog::OnEntryInfo()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		save_data_info_dialog* infoDialog = new save_data_info_dialog(m_save_entries[idx], this);
		infoDialog->setModal(true);
		infoDialog->show();
	}
}

void save_data_list_dialog::UpdateList()
{
	m_list->clearContents();
	m_list->setRowCount((int)m_save_entries.size());

	QVariantMap currNotes = m_gui_settings->GetValue(gui::m_saveNotes).toMap();

	int row = 0;
	for (const SaveDataEntry& entry: m_save_entries)
	{
		QString title = qstr(entry.title);
		QString subtitle = qstr(entry.subtitle);
		QString dirName = qstr(entry.dirName);

		QTableWidgetItem* titleItem = new QTableWidgetItem(title);
		titleItem->setData(Qt::UserRole, row); // For sorting to work properly
		titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsEditable);

		m_list->setItem(row, 0, titleItem);
		QTableWidgetItem* subtitleItem = new QTableWidgetItem(subtitle);
		subtitleItem->setFlags(subtitleItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(row, 1, subtitleItem);

		QTableWidgetItem* dirNameItem = new QTableWidgetItem(dirName);
		dirNameItem->setFlags(dirNameItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(row, 2, dirNameItem);

		QTableWidgetItem* noteItem = new QTableWidgetItem();
		noteItem->setFlags(noteItem->flags() | Qt::ItemIsEditable);

		if (currNotes.contains(dirName))
		{
			noteItem->setText(currNotes[dirName].toString());
		}

		m_list->setItem(row, 3, noteItem);
		++row;
	}

	m_list->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_list->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	QSize tableSize = QSize
	(
		m_list->verticalHeader()->width() + m_list->horizontalHeader()->length() + m_list->frameWidth() * 2,
		m_list->horizontalHeader()->height() + m_list->verticalHeader()->length() + m_list->frameWidth() * 2
	);

	QSize preferredSize = minimumSize().expandedTo(sizeHint() - m_list->sizeHint() + tableSize);

	QSize maxSize = QSize(preferredSize.width(), static_cast<int>(QApplication::desktop()->screenGeometry().height()*.6));

	resize(preferredSize.boundedTo(maxSize));
}
