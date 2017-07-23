#include "stdafx.h"
#include "save_data_list_dialog.h"
#include "save_data_info_dialog.h"
#include "gui_settings.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

//Show up the savedata list, either to choose one to save/load or to manage saves.
//I suggest to use function callbacks to give save data list or get save data entry. (Not implemented or stubbed)
save_data_list_dialog::save_data_list_dialog(const std::vector<SaveDataEntry>& entries, s32 focusedEntry, bool is_saving, QWidget* parent)
	: QDialog(parent), m_save_entries(entries), m_selectedEntry(-1), selectedEntryLabel(nullptr)
{
	setWindowTitle(tr("Save Data Interface"));
	setMinimumSize(QSize(400, 400));

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

		selectedEntryLabel = new QLabel(this);
		UpdateSelectionLabel();
	}

	if (is_saving)
	{
		QPushButton *saveNewEntry = new QPushButton(tr("Save New Entry"), this);
		connect(saveNewEntry, &QAbstractButton::clicked, this, [&]() {
			m_selectedEntry = -1; // Set the return properly.
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
	connect(m_list, &QTableWidget::currentCellChanged, this, [&](int cr, int cc, int pr, int pc) {
		m_selectedEntry = cr;
		UpdateSelectionLabel();
	});

	// TODO: Unstub functions inside of this context menu so it makes sense to show this menu
	//connect(m_list, &QTableWidget::customContextMenuRequested, this, &save_data_list_dialog::ShowContextMenu);
	connect(m_list->horizontalHeader(), &QHeaderView::sectionClicked, [=](int col) {
		OnSort(col);
	});

	// main layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addWidget(m_list);
	if (selectedEntryLabel != nullptr)
	{
		vbox_main->addWidget(selectedEntryLabel);
	}
	vbox_main->addLayout(hbox_action);
	setLayout(vbox_main);

	LoadEntries();
	UpdateList();

	connect(m_list, &QTableWidget::cellChanged, [&](int row, int col) {
		int originalIndex = m_list->item(row, 0)->data(Qt::UserRole).toInt();
		SaveDataEntry originalEntry = m_save_entries[originalIndex];
		QString originalDirName = qstr(originalEntry.dirName);
		gui_settings settings(this);
		QVariantMap currNotes = settings.GetValue(GUI::m_saveNotes).toMap();
		currNotes[originalDirName] = m_list->item(row, col)->text();
		settings.SetValue(GUI::m_saveNotes, currNotes);
	});

	m_list->setCurrentCell(focusedEntry, 0);
}

void save_data_list_dialog::UpdateSelectionLabel()
{
	if (selectedEntryLabel != nullptr)
	{
		if (m_list->currentRow() == -1)
		{
			selectedEntryLabel->setText(tr("Currently Selected: None"));
		}
		else
		{
			int entry = m_list->item(m_list->currentRow(), 0)->data(Qt::UserRole).toInt();
			selectedEntryLabel->setText(tr("Currently Selected: ") + qstr(m_save_entries[entry].dirName));
		}
	}
}

s32 save_data_list_dialog::GetSelection()
{
	int res = result();
	if (res == QDialog::Accepted)
	{
		if (m_selectedEntry == -1)
		{ // Save new entry
			return -1;
		}
		return m_list->item(m_selectedEntry, 0)->data(Qt::UserRole).toInt();
	}

	// Cancel is pressed. May promote to enum or figure out proper cellsavedata code to use later.
	return -2;
}

void save_data_list_dialog::OnSort(int idx)
{
	if (idx >= 0)
	{
		if (idx == m_sortColumn)
		{
			m_sortAscending ^= true;
		}
		else
		{
			m_sortAscending = true;
		}
		Qt::SortOrder colSortOrder = m_sortAscending ? Qt::AscendingOrder : Qt::DescendingOrder;
		m_list->sortByColumn(m_sortColumn, colSortOrder);
		m_sortColumn = idx;
	}
}

//Copy a existing save, need to get more arguments. maybe a new dialog.
void save_data_list_dialog::OnEntryCopy()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnEntryCopy called.");
		//Some Operations?
		UpdateList();
	}
}

//Remove a save file, need to be confirmed.
void save_data_list_dialog::OnEntryRemove()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		LOG_WARNING(HLE, "Stub - save_data_utility: save_data_list_dialog: OnEntryRemove called.");
		//Some Operations?
		UpdateList();
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

//Pop-up a small context-menu, being a replacement for save_data_manage_dialog
void save_data_list_dialog::ShowContextMenu(const QPoint &pos)
{
	QPoint globalPos = m_list->mapToGlobal(pos);
	QMenu* menu = new QMenu();
	int idx = m_list->currentRow();

	saveIDAct = new QAction(tr("SaveID"), this);
	titleAct = new QAction(tr("Title"), this);
	subtitleAct = new QAction(tr("Subtitle"), this);
	copyAct = new QAction(tr("&Copy"), this);
	removeAct = new QAction(tr("&Remove"), this);
	infoAct = new QAction(tr("&Info"), this);

	//This is also a stub for the sort setting. Ids is set according to their sort-type integer.
	m_sort_options = new QMenu(tr("&Sort"));
	m_sort_options->addAction(titleAct);
	m_sort_options->addAction(subtitleAct);
	m_sort_options->addAction(saveIDAct);

	menu->addMenu(m_sort_options);
	menu->addSeparator();
	menu->addAction(copyAct);
	menu->addAction(removeAct);
	menu->addSeparator();
	menu->addAction(infoAct);

	copyAct->setEnabled(idx != -1);
	removeAct->setEnabled(idx != -1);

	//Events
	connect(copyAct, &QAction::triggered, this, &save_data_list_dialog::OnEntryCopy);
	connect(removeAct, &QAction::triggered, this, &save_data_list_dialog::OnEntryRemove);
	connect(infoAct, &QAction::triggered, this, &save_data_list_dialog::OnEntryInfo);

	connect(titleAct, &QAction::triggered, this, [=] {OnSort(0); });
	connect(subtitleAct, &QAction::triggered, this, [=] {OnSort(1); });
	connect(saveIDAct, &QAction::triggered, this, [=] {OnSort(2); });

	menu->exec(globalPos);
}

//This is intended to load the save data list from a way. However that is not certain for a stub. Does nothing now.
void save_data_list_dialog::LoadEntries(void)
{

}

void save_data_list_dialog::UpdateList(void)
{
	m_list->clearContents();
	m_list->setRowCount(m_save_entries.size());
	gui_settings settings(this);

	int row = 0;
	for (SaveDataEntry entry: m_save_entries)
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

		QVariantMap currNotes = settings.GetValue(GUI::m_saveNotes).toMap();
		QTableWidgetItem* noteItem = new QTableWidgetItem();
		noteItem->setFlags(noteItem->flags() | Qt::ItemIsEditable);
		if (currNotes.contains(dirName))
		{
			noteItem->setText(currNotes[dirName].toString());
		}
		else
		{
			currNotes[dirName] = "";
			settings.SetValue(GUI::m_saveNotes, currNotes);
		}
		m_list->setItem(row, 3, noteItem);
		++row;
	}

	m_list->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_list->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	QSize tableSize = QSize(
		m_list->verticalHeader()->width() + m_list->horizontalHeader()->length() + m_list->frameWidth() * 2,
		m_list->horizontalHeader()->height() + m_list->verticalHeader()->length() + m_list->frameWidth() * 2);

	resize(minimumSize().expandedTo(sizeHint() - m_list->sizeHint() + tableSize));

}
