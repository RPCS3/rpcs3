#include "save_manager_dialog.h"

#include "save_data_info_dialog.h"
#include "gui_settings.h"

#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Loader/PSF.h"

#include <QIcon>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

namespace
{
	// Helper converters
	inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }
	inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

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

			for (const auto entry2 : fs::dir(base_dir + entry.name))
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

save_manager_dialog::save_manager_dialog(std::string dir, QWidget* parent) : QDialog(parent),
	m_save_entries(), m_dir(dir), m_sortColumn(1), m_sortAscending(true)
{
	setWindowTitle(tr("Save Manager"));
	setWindowIcon(QIcon(":/rpcs3.ico"));
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
	m_list->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_list->setColumnCount(4);
	m_list->setHorizontalHeaderLabels(QStringList() << tr("Title") << tr("Subtitle") << tr("Save ID") << tr("Entry Notes"));

	// Button Layout
	QHBoxLayout* hbox_buttons = new QHBoxLayout();

	QPushButton* push_close = new QPushButton(tr("&Close"), this);
	push_close->setAutoDefault(true);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(push_close);

	// main layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addWidget(m_list);
	vbox_main->addLayout(hbox_buttons);

	setLayout(vbox_main);

	UpdateList();

	
	// Connects and events
	connect(push_close, &QAbstractButton::clicked, this, &save_manager_dialog::close);
	connect(m_list, &QTableWidget::itemDoubleClicked, this, &save_manager_dialog::OnEntryInfo);

	connect(m_list, &QTableWidget::customContextMenuRequested, this, &save_manager_dialog::ShowContextMenu);
	connect(m_list->horizontalHeader(), &QHeaderView::sectionClicked, [=](int col) {
		OnSort(col);
	});

	connect(m_list, &QTableWidget::cellChanged, [&](int row, int col) {
		int originalIndex = m_list->item(row, 0)->data(Qt::UserRole).toInt();
		SaveDataEntry originalEntry = m_save_entries[originalIndex];
		QString originalDirName = qstr(originalEntry.dirName);
		gui_settings settings(this);
		QVariantMap currNotes = settings.GetValue(GUI::m_saveNotes).toMap();
		currNotes[originalDirName] = m_list->item(row, col)->text();
		settings.SetValue(GUI::m_saveNotes, currNotes);
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
	m_list->setRowCount(m_save_entries.size());
	gui_settings settings(this);

	int row = 0;
	for (SaveDataEntry entry : m_save_entries)
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

/**
* Copied method to do sort from save_data_list_dialog
*/
void save_manager_dialog::OnSort(int idx)
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

//Pop-up a small context-menu, being a replacement for save_data_manage_dialog
void save_manager_dialog::ShowContextMenu(const QPoint &pos)
{
	QPoint globalPos = m_list->mapToGlobal(pos);
	QMenu* menu = new QMenu();
	int idx = m_list->currentRow();

	saveIDAct = new QAction(tr("SaveID"), this);
	titleAct = new QAction(tr("Title"), this);
	subtitleAct = new QAction(tr("Subtitle"), this);
	removeAct = new QAction(tr("&Remove"), this);
	infoAct = new QAction(tr("&Info"), this);

	//This is also a stub for the sort setting. Ids is set according to their sort-type integer.
	m_sort_options = new QMenu(tr("&Sort"));
	m_sort_options->addAction(titleAct);
	m_sort_options->addAction(subtitleAct);
	m_sort_options->addAction(saveIDAct);

	menu->addMenu(m_sort_options);
	menu->addSeparator();
	menu->addAction(removeAct);
	menu->addSeparator();
	menu->addAction(infoAct);

	removeAct->setEnabled(idx != -1);

	//Events
	connect(removeAct, &QAction::triggered, this, &save_manager_dialog::OnEntryRemove);
	connect(infoAct, &QAction::triggered, this, &save_manager_dialog::OnEntryInfo);

	connect(titleAct, &QAction::triggered, this, [=] {OnSort(0); });
	connect(subtitleAct, &QAction::triggered, this, [=] {OnSort(1); });
	connect(saveIDAct, &QAction::triggered, this, [=] {OnSort(2); });

	menu->exec(globalPos);
}
