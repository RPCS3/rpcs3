#include "save_manager_dialog.h"

#include "save_data_info_dialog.h"
#include "custom_table_widget_item.h"
#include "qt_utils.h"

#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Loader/PSF.h"

#include <QtConcurrent>
#include <QDateTime>
#include <QIcon>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QApplication>
#include <QUrl>
#include <QDesktopServices>
#include <QPainter>
#include <QScreen>

LOG_CHANNEL(gui_log, "GUI");

namespace
{
	// Helper converters
	constexpr auto qstr = QString::fromStdString;
	inline std::string sstr(const QString& _in) { return _in.toStdString(); }

	QString FormatTimestamp(u64 time)
	{
		QDateTime dateTime;
		dateTime.setTime_t(time);
		return dateTime.toString("yyyy-MM-dd HH:mm:ss");
	}

	/**
	* This certainly isn't ideal for this code, as it essentially copies cellSaveData.  But, I have no other choice without adding public methods to cellSaveData.
	*/
	std::vector<SaveDataEntry> GetSaveEntries(const std::string& base_dir)
	{
		std::vector<SaveDataEntry> save_entries;

		// get the saves matching the supplied prefix
		for (const auto& entry : fs::dir(base_dir))
		{
			if (!entry.is_directory || entry.name == "." || entry.name == "..")
			{
				continue;
			}

			// PSF parameters
			const auto& psf = psf::load_object(fs::file(base_dir + entry.name + "/PARAM.SFO"));

			if (psf.empty())
			{
				gui_log.error("Failed to load savedata: %s", base_dir + "/" + entry.name);
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
	m_list->setColumnCount(5);
	m_list->setHorizontalHeaderLabels(QStringList() << tr("Icon") << tr("Title & Subtitle") << tr("Last Modified") << tr("Save ID") << tr("Notes"));
	m_list->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
	m_list->horizontalHeader()->setStretchLastSection(true);

	// Bottom bar
	int icon_size = m_gui_settings->GetValue(gui::sd_icon_size).toInt();
	m_icon_size = QSize(icon_size, icon_size);
	QLabel* label_icon_size = new QLabel("Icon size:", this);
	QSlider* slider_icon_size = new QSlider(Qt::Horizontal, this);
	slider_icon_size->setMinimum(60);
	slider_icon_size->setMaximum(225);
	slider_icon_size->setValue(m_icon_size.height());
	QPushButton* push_close = new QPushButton(tr("&Close"), this);
	push_close->setAutoDefault(true);

	// Details
	m_details_icon = new QLabel(this);
	m_details_icon->setMinimumSize(320, 176);
	m_details_title = new QLabel(tr("Select an item to view details"), this);
	m_details_title->setWordWrap(true);
	m_details_subtitle = new QLabel(this);
	m_details_subtitle->setWordWrap(true);
	m_details_modified = new QLabel(this);
	m_details_modified->setWordWrap(true);
	m_details_details = new QLabel(this);
	m_details_details->setWordWrap(true);
	m_details_note = new QLabel(this);
	m_details_note->setWordWrap(true);
	m_button_delete = new QPushButton(tr("Delete Selection"), this);
	m_button_delete->setDisabled(true);
	m_button_folder = new QPushButton(tr("View Folder"), this);
	m_button_delete->setDisabled(true);

	// Details layout
	QVBoxLayout *vbox_details = new QVBoxLayout();
	vbox_details->addWidget(m_details_icon);
	vbox_details->addWidget(m_details_title);
	vbox_details->addWidget(m_details_subtitle);
	vbox_details->addWidget(m_details_modified);
	vbox_details->addWidget(m_details_details);
	vbox_details->addWidget(m_details_note);
	vbox_details->addStretch();
	vbox_details->addWidget(m_button_delete);
	vbox_details->setAlignment(m_button_delete, Qt::AlignHCenter);
	vbox_details->addWidget(m_button_folder);
	vbox_details->setAlignment(m_button_folder, Qt::AlignHCenter);

	// List + Details
	QHBoxLayout *hbox_content = new QHBoxLayout();
	hbox_content->addWidget(m_list);
	hbox_content->addLayout(vbox_details);

	// Items below list
	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	hbox_buttons->addWidget(label_icon_size);
	hbox_buttons->addWidget(slider_icon_size);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(push_close);

	// main layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addLayout(hbox_content);
	vbox_main->addLayout(hbox_buttons);
	setLayout(vbox_main);

	UpdateList();
	m_list->sortByColumn(1, Qt::AscendingOrder);

	if (restoreGeometry(m_gui_settings->GetValue(gui::sd_geometry).toByteArray()))
		resize(size().expandedTo(QGuiApplication::primaryScreen()->availableSize() * 0.5));

	// Connects and events
	connect(push_close, &QAbstractButton::clicked, this, &save_manager_dialog::close);
	connect(m_button_delete, &QAbstractButton::clicked, this, &save_manager_dialog::OnEntriesRemove);
	connect(m_button_folder, &QAbstractButton::clicked, [=]()
	{
		const int idx = m_list->currentRow();
		QTableWidgetItem* item = m_list->item(idx, 1);
		if (!item)
		{
			return;
		}
		const int idx_real = item->data(Qt::UserRole).toInt();
		const QString path = qstr(m_dir + m_save_entries[idx_real].dirName + "/");
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});
	connect(slider_icon_size, &QAbstractSlider::valueChanged, this, &save_manager_dialog::SetIconSize);
	connect(m_list->horizontalHeader(), &QHeaderView::sectionClicked, this, &save_manager_dialog::OnSort);
	connect(m_list, &QTableWidget::customContextMenuRequested, this, &save_manager_dialog::ShowContextMenu);
	connect(m_list, &QTableWidget::cellChanged, [&](int row, int col)
	{
		QTableWidgetItem* user_item = m_list->item(row, 1);
		QTableWidgetItem* text_item = m_list->item(row, col);
		if (!user_item || !text_item)
		{
			return;
		}
		const int originalIndex = user_item->data(Qt::UserRole).toInt();
		const SaveDataEntry originalEntry = m_save_entries[originalIndex];
		const QString originalDirName = qstr(originalEntry.dirName);
		QVariantMap currNotes = m_gui_settings->GetValue(gui::m_saveNotes).toMap();
		currNotes[originalDirName] = text_item->text();
		m_gui_settings->SetValue(gui::m_saveNotes, currNotes);
	});
	connect(m_list, &QTableWidget::itemSelectionChanged, this, &save_manager_dialog::UpdateDetails);
}

void save_manager_dialog::UpdateList()
{
	if (m_dir == "")
	{
		m_dir = Emulator::GetHddDir() + "home/" + Emu.GetUsr() + "/savedata/";
	}

	m_save_entries = GetSaveEntries(m_dir);

	m_list->clearContents();
	m_list->setRowCount(static_cast<int>(m_save_entries.size()));

	QVariantMap currNotes = m_gui_settings->GetValue(gui::m_saveNotes).toMap();

	if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
	{
		m_icon_color = m_gui_settings->GetValue(gui::sd_icon_color).value<QColor>();
	}
	else
	{
		m_icon_color = gui::utils::get_label_color("save_manager_icon_background_color");
	}

	QList<int> indices;
	for (size_t i = 0; i < m_save_entries.size(); ++i)
		indices.append(static_cast<int>(i));

	std::function<QPixmap(const int&)> get_icon = [this](const int& row)
	{
		const auto& entry = m_save_entries[row];
		QPixmap icon = QPixmap(320, 176);
		if (!icon.loadFromData(entry.iconBuf.data(), static_cast<uint>(entry.iconBuf.size())))
		{
			gui_log.warning("Loading icon for save %s failed", entry.dirName);
			icon = QPixmap(320, 176);
			icon.fill(m_icon_color);
		}
		return icon;
	};

	QList<QPixmap> icons = QtConcurrent::blockingMapped<QList<QPixmap>>(indices, get_icon);

	for (int i = 0; i < icons.count(); ++i)
	{
		const auto& entry = m_save_entries[i];

		QString title = qstr(entry.title) + QStringLiteral("\n") + qstr(entry.subtitle);
		QString dirName = qstr(entry.dirName);

		custom_table_widget_item* iconItem = new custom_table_widget_item;
		iconItem->setData(Qt::UserRole, icons[i]);
		iconItem->setFlags(iconItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(i, 0, iconItem);

		QTableWidgetItem* titleItem = new QTableWidgetItem(title);
		titleItem->setData(Qt::UserRole, i); // For sorting to work properly
		titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(i, 1, titleItem);

		QTableWidgetItem* timeItem = new QTableWidgetItem(FormatTimestamp(entry.mtime));
		timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(i, 2, timeItem);

		QTableWidgetItem* dirNameItem = new QTableWidgetItem(dirName);
		dirNameItem->setFlags(dirNameItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(i, 3, dirNameItem);

		QTableWidgetItem* noteItem = new QTableWidgetItem();
		noteItem->setFlags(noteItem->flags() | Qt::ItemIsEditable);
		if (currNotes.contains(dirName))
		{
			noteItem->setText(currNotes[dirName].toString());
		}
		m_list->setItem(i, 4, noteItem);
	}

	UpdateIcons();

	m_list->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_list->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	QSize tableSize = QSize(
		m_list->verticalHeader()->width() + m_list->horizontalHeader()->length() + m_list->frameWidth() * 2,
		m_list->horizontalHeader()->height() + m_list->verticalHeader()->length() + m_list->frameWidth() * 2);

	QSize preferredSize = minimumSize().expandedTo(sizeHint() - m_list->sizeHint() + tableSize);

	QSize maxSize = QSize(preferredSize.width(), static_cast<int>(QGuiApplication::primaryScreen()->geometry().height() * 0.6));

	resize(preferredSize.boundedTo(maxSize));
}

void save_manager_dialog::HandleRepaintUiRequest()
{
	const QSize window_size = size();
	const Qt::SortOrder sort_order = m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder;

	UpdateList();

	m_list->sortByColumn(m_sort_column, sort_order);
	resize(window_size);
}

QPixmap save_manager_dialog::GetResizedIcon(int i)
{
	const qreal dpr = devicePixelRatioF();
	const int width = m_icon_size.width() * dpr;
	const int height = m_icon_size.height() * dpr * 176 / 320;

	QTableWidgetItem* item = m_list->item(i, 0);
	if (!item)
	{
		return QPixmap();
	}
	QPixmap data = item->data(Qt::UserRole).value<QPixmap>();

	QPixmap icon = QPixmap(data.size() * dpr);
	icon.setDevicePixelRatio(dpr);
	icon.fill(m_icon_color);

	QPainter painter(&icon);
	painter.drawPixmap(0, 0, data);
	return icon.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void save_manager_dialog::UpdateIcons()
{
	QList<int> indices;
	for (int i = 0; i < m_list->rowCount(); ++i)
		indices.append(i);

	std::function<QPixmap(const int&)> get_scaled = [this](const int& i)
	{
		return GetResizedIcon(i);
	};

	QList<QPixmap> scaled = QtConcurrent::blockingMapped<QList<QPixmap>>(indices, get_scaled);

	for (int i = 0; i < m_list->rowCount() && i < scaled.count(); ++i)
	{
		QTableWidgetItem* icon_item = m_list->item(i, 0);
		if (icon_item)
			icon_item->setData(Qt::DecorationRole, scaled[i]);
	}

	m_list->resizeRowsToContents();
	m_list->resizeColumnToContents(0);
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
		m_sort_column = logicalIndex;
		Qt::SortOrder sort_order = m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
		m_list->sortByColumn(m_sort_column, sort_order);
	}
}

// Remove a save file, need to be confirmed.
void save_manager_dialog::OnEntryRemove()
{
	int idx = m_list->currentRow();
	if (idx != -1)
	{
		QTableWidgetItem* item = m_list->item(idx, 1);
		if (!item)
		{
			return;
		}
		const int idx_real = item->data(Qt::UserRole).toInt();
		if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete:\n%1?").arg(qstr(m_save_entries[idx_real].title)), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			fs::remove_all(m_dir + m_save_entries[idx_real].dirName + "/");
			m_list->removeRow(idx);
		}
	}
}

void save_manager_dialog::OnEntriesRemove()
{
	QModelIndexList selection(m_list->selectionModel()->selectedRows());
	if (selection.empty())
	{
		return;
	}

	if (selection.size() == 1)
	{
		OnEntryRemove();
		return;
	}

	if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete these %1 items?").arg(selection.size()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		std::sort(selection.rbegin(), selection.rend());
		for (QModelIndex index : selection)
		{
			QTableWidgetItem* item = m_list->item(index.row(), 1);
			if (!item)
			{
				continue;
			}
			const int idx_real = item->data(Qt::UserRole).toInt();
			fs::remove_all(m_dir + m_save_entries[idx_real].dirName + "/");
			m_list->removeRow(index.row());
		}
	}
}

// Pop-up a small context-menu, being a replacement for save_data_manage_dialog
void save_manager_dialog::ShowContextMenu(const QPoint &pos)
{
	int idx = m_list->currentRow();
	if (idx == -1)
	{
		return;
	}

	const bool selectedItems = m_list->selectionModel()->selectedRows().size() > 1;

	QAction* removeAct = new QAction(tr("&Remove"), this);
	QAction* showDirAct = new QAction(tr("&Open Save Directory"), this);

	QMenu* menu = new QMenu();
	menu->addAction(removeAct);
	menu->addAction(showDirAct);

	showDirAct->setEnabled(!selectedItems);
	removeAct->setEnabled(idx != -1);

	// Events
	connect(removeAct, &QAction::triggered, this, &save_manager_dialog::OnEntriesRemove); // entriesremove handles case of one as well
	connect(showDirAct, &QAction::triggered, [=]()
	{
		QTableWidgetItem* item = m_list->item(idx, 1);
		if (!item)
		{
			return;
		}
		const int idx_real = item->data(Qt::UserRole).toInt();
		const QString path = qstr(m_dir + m_save_entries[idx_real].dirName + "/");
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});

	menu->exec(m_list->viewport()->mapToGlobal(pos));
}

void save_manager_dialog::SetIconSize(int size)
{
	m_icon_size = QSize(size, size);
	UpdateIcons();
	m_gui_settings->SetValue(gui::sd_icon_size, size);
}

void save_manager_dialog::closeEvent(QCloseEvent *event)
{
	m_gui_settings->SetValue(gui::sd_geometry, saveGeometry());

	QDialog::closeEvent(event);
}

void save_manager_dialog::UpdateDetails()
{
	int selected = m_list->selectionModel()->selectedRows().size();

	if (selected != 1)
	{
		m_details_icon->setPixmap(QPixmap());
		m_details_subtitle->setText("");
		m_details_modified->setText("");
		m_details_details->setText("");
		m_details_note->setText("");

		if (selected > 1)
		{
			m_button_delete->setDisabled(false);
			m_details_title->setText(tr("%1 items selected").arg(selected));
		}
		else
		{
			m_button_delete->setDisabled(true);
			m_details_title->setText(tr("Select an item to view details"));
		}
		m_button_folder->setDisabled(true);
	}
	else
	{
		const int row = m_list->currentRow();
		QTableWidgetItem* item = m_list->item(row, 1);
		QTableWidgetItem* icon_item = m_list->item(row, 0);

		if (!item || !icon_item)
		{
			return;
		}

		const int idx = item->data(Qt::UserRole).toInt();
		const SaveDataEntry& save = m_save_entries[idx];

		m_details_icon->setPixmap(icon_item->data(Qt::UserRole).value<QPixmap>());
		m_details_title->setText(qstr(save.title));
		m_details_subtitle->setText(qstr(save.subtitle));
		m_details_modified->setText(tr("Last modified: %1").arg(FormatTimestamp(save.mtime)));
		m_details_details->setText(tr("Details:\n").append(qstr(save.details)));
		QString note = tr("Note:\n");
		QVariantMap map = m_gui_settings->GetValue(gui::m_saveNotes).toMap();
		if (map.contains(qstr(save.dirName)))
			note.append(map[qstr(save.dirName)].toString());
		m_details_note->setText(note);

		m_button_delete->setDisabled(false);
		m_button_folder->setDisabled(false);
	}
}
