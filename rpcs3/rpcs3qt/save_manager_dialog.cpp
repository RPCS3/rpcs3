#include "save_manager_dialog.h"

#include "save_data_info_dialog.h"

#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Loader/PSF.h"

#include <QIcon>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QApplication>
#include <QUrl>
#include <QDesktopServices>

namespace
{
	// Helper converters
	constexpr auto qstr = QString::fromStdString;
	inline std::string sstr(const QString& _in) { return _in.toStdString(); }

	/**
	* This certainly isn't ideal for this code, as it essentially copies cellSaveData.  But, I have no other choice without adding public methods to cellSaveData.
	*/
	std::vector<SaveDataEntry> GetSaveEntries(const std::string& base_dir)
	{
		std::vector<SaveDataEntry> save_entries;

		// get the saves matching the supplied prefix
		for (const auto& entry : fs::dir(base_dir))
		{
			if (!entry.is_directory)
			{
				continue;
			}

			// PSF parameters
			const auto& psf = psf::load_object(fs::file(base_dir + entry.name + "/PARAM.SFO"));

			if (psf.empty())
			{
				continue;
			}

			SaveDataEntry save_entry2;
			save_entry2.dirName = psf.at("SAVEDATA_DIRECTORY").as_string();
			save_entry2.listParam = psf.at("SAVEDATA_LIST_PARAM").as_string();
			save_entry2.title = psf.at("TITLE").as_string();
			save_entry2.subtitle = psf.at("SUB_TITLE").as_string();
			save_entry2.details = psf.at("DETAIL").as_string();

			save_entry2.size = 0;

			for (const auto& entry2 : fs::dir(base_dir + entry.name))
			{
				save_entry2.size += entry2.size;
			}

			save_entry2.atime = entry.atime;
			save_entry2.mtime = entry.mtime;
			save_entry2.ctime = entry.ctime;
			if (fs::is_file(base_dir + entry.name + "/ICON0.PNG"))
			{
				fs::file icon = fs::file(base_dir + entry.name + "/ICON0.PNG");
				u32 iconSize = icon.size();
				std::vector<uchar> iconData;
				icon.read(iconData, iconSize);
				save_entry2.iconBuf = iconData;
			}
			save_entry2.isNew = false;
			save_entries.emplace_back(save_entry2);
		}
		return save_entries;
	}
}

save_manager_dialog::save_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::string dir, QWidget* parent)
	: QDialog(parent), m_save_entries(), m_dir(dir), m_sort_column(1), m_sort_ascending(true), m_gui_settings(gui_settings)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setWindowTitle(tr("Save Manager"));
	setMinimumSize(QSize(400, 400));

	Init(dir);
}

/*
 * Future proofing.  Makes it easier in future if I add ability to change directories
 */
void save_manager_dialog::Init(std::string dir)
{
	// Table
	m_list = new QTableWidget(this);

	//m_list->setItemDelegate(new table_item_delegate(this)); // to get rid of cell selection rectangles include "table_item_delegate.h"
	m_list->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_list->setColumnCount(4);
	m_list->setHorizontalHeaderLabels(QStringList() << tr("Title") << tr("Subtitle") << tr("Save ID") << tr("Entry Notes"));

	QPushButton* push_remove_entries = new QPushButton(tr("Delete Selection"), this);

	QPushButton* push_close = new QPushButton(tr("&Close"), this);
	push_close->setAutoDefault(true);

	// Button Layout
	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	hbox_buttons->addWidget(push_remove_entries);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(push_close);

	// main layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addWidget(m_list);
	vbox_main->addLayout(hbox_buttons);
	setLayout(vbox_main);

	UpdateList();

	QByteArray geometry = m_gui_settings->GetValue(gui::sd_geometry).toByteArray();
	if (geometry.isEmpty() == false)
	{
		restoreGeometry(geometry);
	}

	// Connects and events
	connect(push_close, &QAbstractButton::clicked, this, &save_manager_dialog::close);
	connect(push_remove_entries, &QAbstractButton::clicked, this, &save_manager_dialog::OnEntriesRemove);
	connect(m_list->horizontalHeader(), &QHeaderView::sectionClicked, this, &save_manager_dialog::OnSort);
	connect(m_list, &QTableWidget::itemDoubleClicked, this, &save_manager_dialog::OnEntryInfo);
	connect(m_list, &QTableWidget::customContextMenuRequested, this, &save_manager_dialog::ShowContextMenu);
	connect(m_list, &QTableWidget::cellChanged, [&](int row, int col)
	{
		int originalIndex = m_list->item(row, 0)->data(Qt::UserRole).toInt();
		SaveDataEntry originalEntry = m_save_entries[originalIndex];
		QString originalDirName = qstr(originalEntry.dirName);
		QVariantMap currNotes = m_gui_settings->GetValue(gui::m_saveNotes).toMap();
		currNotes[originalDirName] = m_list->item(row, col)->text();
		m_gui_settings->SetValue(gui::m_saveNotes, currNotes);
	});
}

void save_manager_dialog::UpdateList()
{
	if (m_dir == "")
	{
		m_dir = Emu.GetHddDir() + "home/00000001/savedata/";
	}

	m_save_entries = GetSaveEntries(m_dir);

	m_list->clearContents();
	m_list->setRowCount(static_cast<int>(m_save_entries.size()));

	QVariantMap currNotes = m_gui_settings->GetValue(gui::m_saveNotes).toMap();

	int row = 0;
	for (const SaveDataEntry& entry : m_save_entries)
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

	QSize tableSize = QSize(
		m_list->verticalHeader()->width() + m_list->horizontalHeader()->length() + m_list->frameWidth() * 2,
		m_list->horizontalHeader()->height() + m_list->verticalHeader()->length() + m_list->frameWidth() * 2);

	QSize preferredSize = minimumSize().expandedTo(sizeHint() - m_list->sizeHint() + tableSize);

	QSize maxSize = QSize(preferredSize.width(), static_cast<int>(QApplication::desktop()->screenGeometry().height()*.6));

	resize(preferredSize.boundedTo(maxSize));
}

/**
* Copied method to do sort from save_data_list_dialog
*/
void save_manager_dialog::OnSort(int logicalIndex)
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

/**
 *Display info dialog directly. Copied from save_data_list_dialog
 */
void save_manager_dialog::OnEntryInfo()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		idx = m_list->item(idx, 0)->data(Qt::UserRole).toInt();
		save_data_info_dialog* infoDialog = new save_data_info_dialog(m_save_entries[idx], this);
		infoDialog->setModal(true);
		infoDialog->show();
	}
}

//Remove a save file, need to be confirmed.
void save_manager_dialog::OnEntryRemove()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		int idx_real = m_list->item(idx, 0)->data(Qt::UserRole).toInt();
		if (QMessageBox::question(this, "Delete Confirmation", "Are you sure you want to delete:\n" + qstr(m_save_entries[idx_real].title) + "?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			fs::remove_all(m_dir + m_save_entries[idx_real].dirName + "/");
			m_list->removeRow(idx);
		}
	}
}

void save_manager_dialog::OnEntriesRemove()
{
	QModelIndexList selection(m_list->selectionModel()->selectedRows());
	if (selection.size() == 0)
	{
		return;
	}

	if (selection.size() == 1)
	{
		OnEntryRemove();
		return;
	}

	if (QMessageBox::question(this, "Delete Confirmation", QString("Are you sure you want to delete these %1 items?").arg(selection.size())
		, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		qSort(selection.begin(), selection.end(), qGreater<QModelIndex>());
		for (QModelIndex index : selection)
		{
			int idx_real = m_list->item(index.row(), 0)->data(Qt::UserRole).toInt();
			fs::remove_all(m_dir + m_save_entries[idx_real].dirName + "/");
			m_list->removeRow(index.row());
		}
	}
}

//Pop-up a small context-menu, being a replacement for save_data_manage_dialog
void save_manager_dialog::ShowContextMenu(const QPoint &pos)
{
	bool selectedItems = m_list->selectionModel()->selectedRows().size() > 1;

	QPoint globalPos = m_list->mapToGlobal(pos);
	QMenu* menu = new QMenu();
	int idx = m_list->currentRow();
	if (idx == -1)
	{
		return;
	}

	QAction* saveIDAct = new QAction(tr("SaveID"), this);
	QAction* titleAct = new QAction(tr("Title"), this);
	QAction* subtitleAct = new QAction(tr("Subtitle"), this);

	QAction* removeAct = new QAction(tr("&Remove"), this);
	QAction* infoAct = new QAction(tr("&Info"), this);
	QAction* showDirAct = new QAction(tr("&Open Save Directory"), this);

	//This is also a stub for the sort setting. Ids are set accordingly to their sort-type integer.
	// TODO: add more sorting types.
	m_sort_options = new QMenu(tr("&Sort By"));
	m_sort_options->addAction(titleAct);
	m_sort_options->addAction(subtitleAct);
	m_sort_options->addAction(saveIDAct);

	menu->addMenu(m_sort_options);
	menu->addSeparator();
	menu->addAction(removeAct);
	menu->addAction(infoAct);
	menu->addAction(showDirAct);

	infoAct->setEnabled(!selectedItems);
	showDirAct->setEnabled(!selectedItems);
	removeAct->setEnabled(idx != -1);

	// Events
	connect(removeAct, &QAction::triggered, this, &save_manager_dialog::OnEntriesRemove); // entriesremove handles case of one as well
	connect(infoAct, &QAction::triggered, this, &save_manager_dialog::OnEntryInfo);
	connect(showDirAct, &QAction::triggered, [=]() {
		int idx_real = m_list->item(idx, 0)->data(Qt::UserRole).toInt();
		QString path = qstr(m_dir + m_save_entries[idx_real].dirName + "/");
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});

	connect(titleAct, &QAction::triggered, this, [=] {OnSort(0); });
	connect(subtitleAct, &QAction::triggered, this, [=] {OnSort(1); });
	connect(saveIDAct, &QAction::triggered, this, [=] {OnSort(2); });

	menu->exec(globalPos);
}

void save_manager_dialog::closeEvent(QCloseEvent * event)
{
	m_gui_settings->SetValue(gui::sd_geometry, saveGeometry());
}
