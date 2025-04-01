#include "save_manager_dialog.h"

#include "custom_table_widget_item.h"
#include "qt_utils.h"
#include "qt_video_source.h"
#include "gui_application.h"
#include "gui_settings.h"
#include "persistent_settings.h"
#include "game_list_delegate.h"
#include "progress_dialog.h"
#include "video_label.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Loader/PSF.h"

#include <QtConcurrent>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QPainter>
#include <QScreen>
#include <QScrollBar>

#include "Utilities/File.h"
#include "Utilities/mutex.h"

LOG_CHANNEL(gui_log, "GUI");

namespace
{
	// Helper converters
	QString FormatTimestamp(s64 time)
	{
		QDateTime dateTime;
		dateTime.setSecsSinceEpoch(time);
		return dateTime.toString("yyyy-MM-dd HH:mm:ss");
	}
}

enum SaveColumns
{
	Icon = 0,
	Name = 1,
	Time = 2,
	Dir = 3,
	Note = 4,

	Count
};

enum SaveUserRole
{
	Pixmap = Qt::UserRole,
	PixmapScaled,
	PixmapLoaded
};

save_manager_dialog::save_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<persistent_settings> persistent_settings, std::string dir, QWidget* parent)
	: QDialog(parent)
	, m_dir(std::move(dir))
	, m_gui_settings(std::move(gui_settings))
	, m_persistent_settings(std::move(persistent_settings))
{
	setWindowTitle(tr("Save Manager"));
	setMinimumSize(QSize(400, 400));
	setAttribute(Qt::WA_DeleteOnClose);

	Init();
}

/*
 * Future proofing.  Makes it easier in future if I add ability to change directories
 */
void save_manager_dialog::Init()
{
	// Table
	m_list = new game_list();
	m_list->setItemDelegate(new game_list_delegate(m_list));
	m_list->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_list->setColumnCount(SaveColumns::Count);
	m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_list->verticalScrollBar()->setSingleStep(20);
	m_list->horizontalScrollBar()->setSingleStep(10);
	m_list->setHorizontalHeaderLabels(QStringList() << tr("Icon") << tr("Title & Subtitle") << tr("Last Modified") << tr("Save ID") << tr("Notes"));
	m_list->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
	m_list->horizontalHeader()->setStretchLastSection(true);
	m_list->setMouseTracking(true);

	// Bottom bar
	const int icon_size = m_gui_settings->GetValue(gui::sd_icon_size).toInt();
	m_icon_size = QSize(icon_size, icon_size * 176 / 320);
	QLabel* label_icon_size = new QLabel(tr("Icon size:"), this);
	QSlider* slider_icon_size = new QSlider(Qt::Horizontal, this);
	slider_icon_size->setMinimum(60);
	slider_icon_size->setMaximum(225);
	slider_icon_size->setValue(icon_size);
	QLabel* label_search_bar = new QLabel(tr("Search:"), this);
	QLineEdit* search_bar = new QLineEdit(this);
	QPushButton* push_close = new QPushButton(tr("&Close"), this);
	push_close->setAutoDefault(true);

	// Details
	m_details_icon = new video_label(this);
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
	hbox_buttons->addWidget(label_search_bar);
	hbox_buttons->addWidget(search_bar);
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
	connect(m_button_folder, &QAbstractButton::clicked, [this]()
	{
		const int idx = m_list->currentRow();
		QTableWidgetItem* item = m_list->item(idx, SaveColumns::Name);
		if (!item)
		{
			return;
		}
		const int idx_real = item->data(Qt::UserRole).toInt();
		const QString path = QString::fromStdString(m_dir + ::at32(m_save_entries, idx_real).dirName + "/");
		gui::utils::open_dir(path);
	});
	connect(slider_icon_size, &QAbstractSlider::valueChanged, this, &save_manager_dialog::SetIconSize);
	connect(m_list->horizontalHeader(), &QHeaderView::sectionClicked, this, &save_manager_dialog::OnSort);
	connect(m_list, &QTableWidget::customContextMenuRequested, this, &save_manager_dialog::ShowContextMenu);
	connect(m_list, &QTableWidget::cellChanged, [&](int row, int col)
	{
		if (col != SaveColumns::Note)
		{
			return;
		}
		QTableWidgetItem* user_item = m_list->item(row, SaveColumns::Name);
		QTableWidgetItem* text_item = m_list->item(row, SaveColumns::Note);
		if (!user_item || !text_item)
		{
			return;
		}
		const int original_index = user_item->data(Qt::UserRole).toInt();
		const SaveDataEntry originalEntry = ::at32(m_save_entries, original_index);
		const QString original_dir_name = QString::fromStdString(originalEntry.dirName);
		QVariantMap notes = m_persistent_settings->GetValue(gui::persistent::save_notes).toMap();
		notes[original_dir_name] = text_item->text();
		m_persistent_settings->SetValue(gui::persistent::save_notes, notes);
	});
	connect(m_list, &QTableWidget::itemSelectionChanged, this, &save_manager_dialog::UpdateDetails);
	connect(this, &save_manager_dialog::IconReady, this, [this](int index, const QPixmap& pixmap)
	{
		if (movie_item* item = static_cast<movie_item*>(m_list->item(index, SaveColumns::Icon)))
		{
			item->setData(SaveUserRole::PixmapScaled, pixmap);
			item->image_change_callback();
		}
	});
	connect(search_bar, &QLineEdit::textChanged, this, &save_manager_dialog::text_changed);
}

/**
* This certainly isn't ideal for this code, as it essentially copies cellSaveData.  But, I have no other choice without adding public methods to cellSaveData.
*/
std::vector<SaveDataEntry> save_manager_dialog::GetSaveEntries(const std::string& base_dir)
{
	std::vector<SaveDataEntry> save_entries;
	std::vector<fs::dir_entry> dir_list;

	qRegisterMetaType<QVector<int>>("QVector<int>");
	QList<int> indices;

	for (const auto& entry : fs::dir(base_dir))
	{
		if (!entry.is_directory || entry.name == "." || entry.name == "..")
		{
			continue;
		}
		indices.append(static_cast<int>(dir_list.size()));
		dir_list.emplace_back(entry);
	}

	if (dir_list.empty())
	{
		return save_entries;
	}

	QFutureWatcher<void> future_watcher;

	progress_dialog progress_dialog(tr("Loading save data"), tr("Loading save data, please wait..."), tr("Cancel"), 0, 1, false, this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

	connect(&future_watcher, &QFutureWatcher<void>::progressRangeChanged, &progress_dialog, &QProgressDialog::setRange);
	connect(&future_watcher, &QFutureWatcher<void>::progressValueChanged, &progress_dialog, &QProgressDialog::setValue);
	connect(&progress_dialog, &QProgressDialog::canceled, this, [this, &future_watcher]()
	{
		future_watcher.cancel();
		close(); // It's pointless to show an empty window
	});

	shared_mutex mutex;

	future_watcher.setFuture(QtConcurrent::map(indices, [&](int index)
	{
		const fs::dir_entry& entry = ::at32(dir_list, index);
		gui_log.trace("Loading trophy dir: %s", entry.name);

		// PSF parameters
		const auto [psf, errc] = psf::load(base_dir + entry.name + "/PARAM.SFO");

		if (psf.empty())
		{
			gui_log.error("Failed to load savedata: %s (%s)", base_dir + "/" + entry.name, errc);
			return;
		}

		SaveDataEntry save_entry2 {};
		save_entry2.dirName = psf::get_string(psf, "SAVEDATA_DIRECTORY");
		save_entry2.listParam = psf::get_string(psf, "SAVEDATA_LIST_PARAM");
		save_entry2.title = psf::get_string(psf, "TITLE");
		save_entry2.subtitle = psf::get_string(psf, "SUB_TITLE");
		save_entry2.details = psf::get_string(psf, "DETAIL");

		for (const auto& entry2 : fs::dir(base_dir + entry.name))
		{
			save_entry2.size += entry2.size;
		}

		save_entry2.atime = entry.atime;
		save_entry2.mtime = entry.mtime;
		save_entry2.ctime = entry.ctime;
		save_entry2.isNew = false;

		std::scoped_lock lock(mutex);
		save_entries.emplace_back(save_entry2);
	}));

	progress_dialog.exec();
	future_watcher.waitForFinished();
	return save_entries;
}

void save_manager_dialog::UpdateList()
{
	WaitForRepaintThreads(true);

	if (m_dir.empty())
	{
		m_dir = rpcs3::utils::get_hdd0_dir() + "home/" + Emu.GetUsr() + "/savedata/";
	}

	m_save_entries = GetSaveEntries(m_dir);

	m_list->setSortingEnabled(false); // Disable sorting before using setItem calls
	m_list->clearContents();
	m_list->setRowCount(static_cast<int>(m_save_entries.size()));

	const QVariantMap notes = m_persistent_settings->GetValue(gui::persistent::save_notes).toMap();

	if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
	{
		m_icon_color = m_gui_settings->GetValue(gui::sd_icon_color).value<QColor>();
	}
	else
	{
		m_icon_color = gui::utils::get_label_color("save_manager_icon_background_color", Qt::transparent, Qt::transparent);
	}

	QPixmap placeholder(320, 176);
	placeholder.fill(Qt::transparent);

	const s32 language_index = gui_application::get_language_id();
	const std::string localized_movie = fmt::format("ICON1_%02d.PAM", language_index);

	for (int i = 0; i < static_cast<int>(m_save_entries.size()); ++i)
	{
		const SaveDataEntry& entry = ::at32(m_save_entries, i);

		const QString title = QString::fromStdString(entry.title) + QStringLiteral("\n") + QString::fromStdString(entry.subtitle);
		const QString dir_name = QString::fromStdString(entry.dirName);
		const std::string dir_path = m_dir + entry.dirName + "/";

		custom_table_widget_item* icon_item = new custom_table_widget_item;
		icon_item->setData(Qt::DecorationRole, placeholder);
		icon_item->setData(SaveUserRole::Pixmap, placeholder);
		icon_item->setData(SaveUserRole::PixmapScaled, placeholder);
		icon_item->setData(SaveUserRole::PixmapLoaded, false);
		icon_item->setFlags(icon_item->flags() & ~Qt::ItemIsEditable);

		if (const std::string movie_path = dir_path + localized_movie; fs::is_file(movie_path))
		{
			icon_item->set_video_path(movie_path);
		}
		else if (const std::string movie_path = dir_path + "ICON1.PAM"; fs::is_file(movie_path))
		{
			icon_item->set_video_path(movie_path);
		}

		icon_item->set_image_change_callback([this, icon_item](const QVideoFrame& frame)
		{
			if (!icon_item)
			{
				return;
			}

			if (const QPixmap pixmap = icon_item->get_movie_image(frame); icon_item->get_active() && !pixmap.isNull())
			{
				icon_item->setData(Qt::DecorationRole, pixmap.scaled(m_icon_size, Qt::KeepAspectRatio));
			}
			else
			{
				icon_item->setData(Qt::DecorationRole, icon_item->data(SaveUserRole::PixmapScaled).value<QPixmap>());
				icon_item->stop_movie();
			}
		});
		m_list->setItem(i, SaveColumns::Icon, icon_item);

		custom_table_widget_item* titleItem = new custom_table_widget_item(title);
		titleItem->setData(Qt::UserRole, i); // For sorting to work properly
		titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(i, SaveColumns::Name, titleItem);

		custom_table_widget_item* timeItem = new custom_table_widget_item(FormatTimestamp(entry.mtime));
		timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(i, SaveColumns::Time, timeItem);

		custom_table_widget_item* dirNameItem = new custom_table_widget_item(dir_name);
		dirNameItem->setFlags(dirNameItem->flags() & ~Qt::ItemIsEditable);
		m_list->setItem(i, SaveColumns::Dir, dirNameItem);

		custom_table_widget_item* noteItem = new custom_table_widget_item();
		noteItem->setFlags(noteItem->flags() | Qt::ItemIsEditable);
		if (notes.contains(dir_name))
		{
			noteItem->setText(notes[dir_name].toString());
		}
		m_list->setItem(i, SaveColumns::Note, noteItem);
	}

	m_list->setSortingEnabled(true); // Enable sorting only after using setItem calls

	m_list->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_list->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	const QSize tableSize(
		m_list->verticalHeader()->width() + m_list->horizontalHeader()->length() + m_list->frameWidth() * 2,
		m_list->horizontalHeader()->height() + m_list->verticalHeader()->length() + m_list->frameWidth() * 2);

	const QSize preferredSize = minimumSize().expandedTo(sizeHint() - m_list->sizeHint() + tableSize);

	const QSize maxSize(preferredSize.width(), static_cast<int>(QGuiApplication::primaryScreen()->geometry().height() * 0.6));

	resize(preferredSize.boundedTo(maxSize));

	UpdateIcons();
}

void save_manager_dialog::HandleRepaintUiRequest()
{
	const QSize window_size = size();
	const Qt::SortOrder sort_order = m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder;

	UpdateList();

	m_list->sortByColumn(m_sort_column, sort_order);
	resize(window_size);
}

void save_manager_dialog::UpdateIcons()
{
	WaitForRepaintThreads(true);

	const qreal dpr = devicePixelRatioF();

	QPixmap placeholder(m_icon_size);
	placeholder.fill(Qt::transparent);

	for (int i = 0; i < m_list->rowCount(); ++i)
	{
		if (movie_item* icon_item = static_cast<movie_item*>(m_list->item(i, SaveColumns::Icon)))
		{
			icon_item->setData(SaveUserRole::PixmapScaled, placeholder);
			icon_item->setData(Qt::DecorationRole, placeholder);
		}
	}

	m_list->resizeRowsToContents();
	m_list->resizeColumnToContents(SaveColumns::Icon);

	const s32 language_index = gui_application::get_language_id();
	const std::string localized_icon = fmt::format("ICON0_%02d.PNG", language_index);

	for (int i = 0; i < m_list->rowCount(); ++i)
	{
		if (movie_item* icon_item = static_cast<movie_item*>(m_list->item(i, SaveColumns::Icon)))
		{
			icon_item->set_icon_load_func([this, cancel = icon_item->icon_loading_aborted(), dpr, localized_icon](int index)
			{
				if (cancel && cancel->load())
				{
					return;
				}

				QPixmap icon;

				if (movie_item* item = static_cast<movie_item*>(m_list->item(index, SaveColumns::Icon)))
				{
					if (!item->data(SaveUserRole::PixmapLoaded).toBool())
					{
						// Load game icon
						if (QTableWidgetItem* user_item = m_list->item(index, SaveColumns::Name))
						{
							const int idx_real = user_item->data(Qt::UserRole).toInt();
							const SaveDataEntry& entry = ::at32(m_save_entries, idx_real);

							if (!icon.load(QString::fromStdString(m_dir + entry.dirName + "/" + localized_icon)) &&
								!icon.load(QString::fromStdString(m_dir + entry.dirName + "/ICON0.PNG")))
							{
								gui_log.warning("Loading icon for save %s failed", entry.dirName);
								icon = QPixmap(320, 176);
								icon.fill(m_icon_color);
							}

							item->setData(SaveUserRole::PixmapLoaded, true);
							item->setData(SaveUserRole::Pixmap, icon);
						}
						else
						{
							gui_log.error("Loading icon for save failed (table item is null)");
						}
					}
					else
					{
						icon = item->data(SaveUserRole::Pixmap).value<QPixmap>();
					}
				}

				if (cancel && cancel->load())
				{
					return;
				}

				QPixmap new_icon(icon.size() * dpr);
				new_icon.setDevicePixelRatio(dpr);
				new_icon.fill(m_icon_color);

				if (!icon.isNull())
				{
					QPainter painter(&new_icon);
					painter.setRenderHint(QPainter::SmoothPixmapTransform);
					painter.drawPixmap(QPoint(0, 0), icon);
					painter.end();
				}

				new_icon = new_icon.scaled(m_icon_size * dpr, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

				if (!cancel || !cancel->load())
				{
					Q_EMIT IconReady(index, new_icon);
				}
			});
		}
	}
}

/**
* Copied method to do sort from save_data_list_dialog
*/
void save_manager_dialog::OnSort(int logicalIndex)
{
	if (logicalIndex >= 0)
	{
		WaitForRepaintThreads(false);

		if (logicalIndex == m_sort_column)
		{
			m_sort_ascending ^= true;
		}
		else
		{
			m_sort_ascending = true;
		}
		m_sort_column = logicalIndex;
		const Qt::SortOrder sort_order = m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
		m_list->sortByColumn(m_sort_column, sort_order);
	}
}

// Remove a save file, need to be confirmed.
void save_manager_dialog::OnEntryRemove(int row, bool user_interaction)
{
	if (QTableWidgetItem* item = m_list->item(row, SaveColumns::Name))
	{
		const int idx_real = item->data(Qt::UserRole).toInt();
		const SaveDataEntry& entry = ::at32(m_save_entries, idx_real);

		if (!user_interaction || QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete:\n%1?").arg(QString::fromStdString(entry.title)), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			fs::remove_all(m_dir + entry.dirName + "/");
			m_list->removeRow(row);
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

	WaitForRepaintThreads(false);

	if (selection.size() == 1)
	{
		OnEntryRemove(selection.first().row(), true);
		return;
	}

	if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete these %n items?", "", selection.size()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		std::sort(selection.rbegin(), selection.rend());
		for (const QModelIndex& index : selection)
		{
			OnEntryRemove(index.row(), false);
		}
	}
}

// Pop-up a small context-menu, being a replacement for save_data_manage_dialog
void save_manager_dialog::ShowContextMenu(const QPoint& pos)
{
	const int idx = m_list->currentRow();
	if (idx == -1)
	{
		return;
	}

	WaitForRepaintThreads(false);

	const bool selectedItems = m_list->selectionModel()->selectedRows().size() > 1;

	QAction* removeAct = new QAction(tr("&Remove"), this);
	QAction* showDirAct = new QAction(tr("&Open Save Directory"), this);

	QMenu* menu = new QMenu();
	menu->addAction(removeAct);
	menu->addAction(showDirAct);

	showDirAct->setEnabled(!selectedItems);

	// Events
	connect(removeAct, &QAction::triggered, this, &save_manager_dialog::OnEntriesRemove); // entriesremove handles case of one as well
	connect(showDirAct, &QAction::triggered, [this, idx]()
	{
		QTableWidgetItem* item = m_list->item(idx, SaveColumns::Name);
		if (!item)
		{
			return;
		}
		const int idx_real = item->data(Qt::UserRole).toInt();
		const QString path = QString::fromStdString(m_dir + ::at32(m_save_entries, idx_real).dirName + "/");
		gui::utils::open_dir(path);
	});

	menu->exec(m_list->viewport()->mapToGlobal(pos));
}

void save_manager_dialog::SetIconSize(int size)
{
	m_icon_size = QSize(size, size * 176 / 320);
	UpdateIcons();
	m_gui_settings->SetValue(gui::sd_icon_size, size, false); // Don't sync while sliding
}

void save_manager_dialog::closeEvent(QCloseEvent *event)
{
	m_gui_settings->SetValue(gui::sd_geometry, saveGeometry());

	QDialog::closeEvent(event);
}

void save_manager_dialog::UpdateDetails()
{
	if (const int selected = m_list->selectionModel()->selectedRows().size(); selected != 1)
	{
		m_details_icon->set_thumbnail({});
		m_details_icon->set_active(false);

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
		WaitForRepaintThreads(false);

		const int row = m_list->currentRow();
		QTableWidgetItem* item = m_list->item(row, SaveColumns::Name);
		movie_item* icon_item = static_cast<movie_item*>(m_list->item(row, SaveColumns::Icon));

		if (!item || !icon_item)
		{
			return;
		}

		const int idx = item->data(Qt::UserRole).toInt();
		const SaveDataEntry& save = ::at32(m_save_entries, idx);

		m_details_icon->set_video_path(icon_item->video_path().toStdString());
		m_details_icon->set_thumbnail(icon_item->data(SaveUserRole::Pixmap).value<QPixmap>());
		m_details_icon->set_active(false);

		m_details_title->setText(QString::fromStdString(save.title));
		m_details_subtitle->setText(QString::fromStdString(save.subtitle));
		m_details_modified->setText(tr("Last modified: %1").arg(FormatTimestamp(save.mtime)));
		m_details_details->setText(tr("Details:\n").append(QString::fromStdString(save.details)));
		QString note = tr("Note:\n");
		const QString dir_name = QString::fromStdString(save.dirName);
		if (const QVariantMap map = m_persistent_settings->GetValue(gui::persistent::save_notes).toMap();
			map.contains(dir_name))
		{
			note.append(map[dir_name].toString());
		}
		m_details_note->setText(note);

		m_button_delete->setDisabled(false);
		m_button_folder->setDisabled(false);
	}
}

void save_manager_dialog::WaitForRepaintThreads(bool abort)
{
	for (int i = 0; i < m_list->rowCount(); i++)
	{
		if (movie_item* item = static_cast<movie_item*>(m_list->item(i, SaveColumns::Icon)))
		{
			item->wait_for_icon_loading(abort);
		}
	}
}

void save_manager_dialog::text_changed(const QString& text)
{
	const auto check_text = [&](int row)
	{
		if (text.isEmpty())
			return true;

		for (int col = SaveColumns::Name; col < SaveColumns::Count; col++)
		{
			const QTableWidgetItem* item = m_list->item(row, col);

			if (item && item->text().contains(text, Qt::CaseInsensitive))
				return true;
		}

		return false;
	};

	bool new_row_visible = false;

	for (int i = 0; i < m_list->rowCount(); i++)
	{
		// only show items filtered for search text
		const bool is_hidden = m_list->isRowHidden(i);
		const bool hide = !check_text(i);

		if (is_hidden != hide)
		{
			m_list->setRowHidden(i, hide);
			new_row_visible = !hide;
		}
	}

	if (new_row_visible)
	{
		UpdateIcons();
	}
}
