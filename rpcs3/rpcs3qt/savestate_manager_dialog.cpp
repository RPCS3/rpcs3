#include "stdafx.h"
#include "savestate_manager_dialog.h"
#include "custom_table_widget_item.h"
#include "game_list_delegate.h"
#include "qt_utils.h"
#include "game_list.h"
#include "gui_settings.h"
#include "progress_dialog.h"

#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QMenu>
#include <QtConcurrent>

LOG_CHANNEL(gui_log, "GUI");

enum GameUserRole
{
	GameIndex = Qt::UserRole,
	GamePixmapLoaded,
	GamePixmap
};

savestate_manager_dialog::savestate_manager_dialog(std::shared_ptr<gui_settings> gui_settings, const std::vector<game_info>& games)
	: QWidget()
	, m_gui_settings(std::move(gui_settings))
	, m_game_info(games)
{
	// Nonspecific widget settings
	setWindowTitle(tr("Savestate Manager"));
	setObjectName("savestate_manager");
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);

	m_game_icon_size_index = m_gui_settings->GetValue(gui::ss_game_icon_size).toInt();

	// Game chooser combo box
	m_game_combo = new QComboBox();
	m_game_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

	// Games Table
	m_game_table = new game_list();
	m_game_table->setObjectName("savestate_manager_game_table");
	m_game_table->setShowGrid(false);
	m_game_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_table->verticalScrollBar()->installEventFilter(this);
	m_game_table->verticalScrollBar()->setSingleStep(20);
	m_game_table->horizontalScrollBar()->setSingleStep(20);
	m_game_table->setItemDelegate(new game_list_delegate(m_game_table));
	m_game_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_game_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_game_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_game_table->setColumnCount(static_cast<int>(gui::savestate_game_list_columns::count));
	m_game_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_game_table->horizontalHeader()->setStretchLastSection(true);
	m_game_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_game_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_game_table->verticalHeader()->setVisible(false);
	m_game_table->setAlternatingRowColors(true);
	m_game_table->installEventFilter(this);

	auto add_game_column = [this](gui::savestate_game_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_game_table->setHorizontalHeaderItem(static_cast<int>(col), new QTableWidgetItem(header_text));
		m_game_column_acts.append(new QAction(action_text, this));
	};

	add_game_column(gui::savestate_game_list_columns::icon,       tr("Icon"),       tr("Show Icons"));
	add_game_column(gui::savestate_game_list_columns::name,       tr("Game"),       tr("Show Games"));
	add_game_column(gui::savestate_game_list_columns::savestates, tr("Savestates"), tr("Show Savestates"));

	// Savestate Table
	m_savestate_table = new game_list();
	m_savestate_table->setObjectName("savestate_manager_savestate_table");
	m_savestate_table->setShowGrid(false);
	m_savestate_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_savestate_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_savestate_table->verticalScrollBar()->installEventFilter(this);
	m_savestate_table->verticalScrollBar()->setSingleStep(20);
	m_savestate_table->horizontalScrollBar()->setSingleStep(20);
	m_savestate_table->setItemDelegate(new game_list_delegate(m_savestate_table));
	m_savestate_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_savestate_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_savestate_table->setColumnCount(static_cast<int>(gui::savestate_list_columns::count));
	m_savestate_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_savestate_table->horizontalHeader()->setStretchLastSection(true);
	m_savestate_table->verticalHeader()->setVisible(false);
	m_savestate_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_savestate_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_savestate_table->setAlternatingRowColors(true);
	m_savestate_table->installEventFilter(this);

	auto add_savestate_column = [this](gui::savestate_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_savestate_table->setHorizontalHeaderItem(static_cast<int>(col), new QTableWidgetItem(header_text));
		m_savestate_column_acts.append(new QAction(action_text, this));
	};

	add_savestate_column(gui::savestate_list_columns::name, tr("Name"), tr("Show Names"));
	add_savestate_column(gui::savestate_list_columns::compatible, tr("Compatible"), tr("Show Compatible"));
	add_savestate_column(gui::savestate_list_columns::date, tr("Created"), tr("Show Created"));
	add_savestate_column(gui::savestate_list_columns::path, tr("Path"), tr("Show Paths"));

	m_splitter = new QSplitter();
	m_splitter->addWidget(m_game_table);
	m_splitter->addWidget(m_savestate_table);

	m_game_icon_size = gui_settings::SizeFromSlider(m_game_icon_size_index);

	QLabel* game_slider_label = new QLabel();
	game_slider_label->setText(tr("Game Icon Size: %0x%1").arg(m_game_icon_size.width()).arg(m_game_icon_size.height()));

	m_game_icon_slider = new QSlider(Qt::Horizontal);
	m_game_icon_slider->setRange(0, gui::gl_max_slider_pos);
	m_game_icon_slider->setValue(m_game_icon_size_index);

	// LAYOUTS
	QGroupBox* choose_game = new QGroupBox(tr("Choose Game"));
	QVBoxLayout* choose_layout = new QVBoxLayout();
	choose_layout->addWidget(m_game_combo);
	choose_game->setLayout(choose_layout);

	QGroupBox* icon_settings = new QGroupBox(tr("Icon Options"));
	QVBoxLayout* slider_layout = new QVBoxLayout();
	slider_layout->addWidget(game_slider_label);
	slider_layout->addWidget(m_game_icon_slider);
	icon_settings->setLayout(slider_layout);

	QVBoxLayout* options_layout = new QVBoxLayout();
	options_layout->addWidget(choose_game);
	options_layout->addWidget(icon_settings);
	options_layout->addStretch();

	QHBoxLayout* all_layout = new QHBoxLayout(this);
	all_layout->addLayout(options_layout);
	all_layout->addWidget(m_splitter);
	all_layout->setStretch(1, 1);
	setLayout(all_layout);

	// Make connects
	connect(m_game_icon_slider, &QSlider::valueChanged, this, [this, game_slider_label](int val)
	{
		m_game_icon_size_index = val;
		m_game_icon_size = gui_settings::SizeFromSlider(val);
		if (game_slider_label)
		{
			game_slider_label->setText(tr("Game Icon Size: %0x%1").arg(m_game_icon_size.width()).arg(m_game_icon_size.height()));
		}
		ResizeGameIcons();
		if (m_save_game_icon_size)
		{
			m_save_game_icon_size = false;
			m_gui_settings->SetValue(gui::ss_game_icon_size, val);
		}
	});

	connect(m_game_icon_slider, &QSlider::sliderReleased, this, [this]()
	{
		m_gui_settings->SetValue(gui::ss_game_icon_size, m_game_icon_slider->value());
	});

	connect(m_game_icon_slider, &QSlider::actionTriggered, this, [this](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_game_icon_size = true; // actionTriggered happens before the value was changed
		}
	});

	connect(m_savestate_table, &QTableWidget::customContextMenuRequested, this, &savestate_manager_dialog::ShowSavestateTableContextMenu);

	connect(m_game_combo, &QComboBox::currentTextChanged, this, [this]
	{
		PopulateSavestateTable();
	});

	connect(m_game_table, &QTableWidget::customContextMenuRequested, this, &savestate_manager_dialog::ShowGameTableContextMenu);

	connect(m_game_table, &QTableWidget::itemSelectionChanged, this, [this]
	{
		if (m_game_table->selectedItems().isEmpty())
		{
			return;
		}
		QTableWidgetItem* item = m_game_table->item(m_game_table->selectedItems().first()->row(), static_cast<int>(gui::savestate_game_list_columns::name));
		if (!item)
		{
			return;
		}
		m_game_combo->setCurrentText(item->text());
	});

	connect(this, &savestate_manager_dialog::GameIconReady, this, [this](int index, const QPixmap& pixmap)
	{
		if (QTableWidgetItem* icon_item = m_game_table->item(index, static_cast<int>(gui::savestate_game_list_columns::icon)))
		{
			icon_item->setData(Qt::DecorationRole, pixmap);
		}
	});

	m_savestate_table->create_header_actions(m_savestate_column_acts,
		[this](int col) { return m_gui_settings->GetSavestateListColVisibility(static_cast<gui::savestate_list_columns>(col)); },
		[this](int col, bool visible) { m_gui_settings->SetSavestateListColVisibility(static_cast<gui::savestate_list_columns>(col), visible); });
	
	m_game_table->create_header_actions(m_game_column_acts,
		[this](int col) { return m_gui_settings->GetSavestateGamelistColVisibility(static_cast<gui::savestate_game_list_columns>(col)); },
		[this](int col, bool visible) { m_gui_settings->SetSavestateGamelistColVisibility(static_cast<gui::savestate_game_list_columns>(col), visible); });

	RepaintUI(true);

	StartSavestateLoadThreads();
}

savestate_manager_dialog::~savestate_manager_dialog()
{
	WaitAndAbortGameRepaintThreads();
}

bool savestate_manager_dialog::LoadSavestateFolderToDB(std::unique_ptr<game_savestates_data>&& game_savestates)
{
	ensure(!!game_savestates);

	if (game_savestates->title_id.empty())
	{
		gui_log.error("Failed to load savestates. Path empty!");
		return false;
	}

	const std::string dir_path = fs::get_config_dir() + "savestates/" + game_savestates->title_id + "/";

	const QDir savestate_dir(QString::fromStdString(dir_path));
	const QFileInfoList file_list = savestate_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

	if (file_list.isEmpty())
	{
		return false;
	}

	// Populate game_savestates_data
	game_savestates->savestates.resize(file_list.size());
	game_savestates->dir_path = dir_path;

	extern bool is_savestate_compatible(const std::string& filepath);

	for (usz id = 0; id < game_savestates->savestates.size(); ++id)
	{
		game_savestates->savestates[id].name = file_list[id].baseName();
		game_savestates->savestates[id].path = file_list[id].absoluteFilePath();
		game_savestates->savestates[id].date = file_list[id].birthTime();
		game_savestates->savestates[id].is_compatible = is_savestate_compatible(game_savestates->savestates[id].path.toStdString());
	}

	{
		std::scoped_lock lock(m_savestate_db_mtx);
		m_savestate_db.push_back(std::move(game_savestates));
	}

	return true;
}

void savestate_manager_dialog::RepaintUI(bool restore_layout)
{
	if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
	{
		m_game_icon_color = m_gui_settings->GetValue(gui::ss_icon_color).value<QColor>();
	}
	else
	{
		m_game_icon_color = gui::utils::get_label_color("savestate_manager_icon_background_color", Qt::transparent, Qt::transparent);
	}

	PopulateGameTable();

	if (restore_layout && !restoreGeometry(m_gui_settings->GetValue(gui::ss_geometry).toByteArray()))
	{
		resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
	}

	if (restore_layout && !m_splitter->restoreState(m_gui_settings->GetValue(gui::ss_splitterState).toByteArray()))
	{
		const int width_left = m_splitter->width() * 0.4;
		const int width_right = m_splitter->width() - width_left;
		m_splitter->setSizes({ width_left, width_right });
	}

	PopulateSavestateTable();

	const QByteArray game_table_state = m_gui_settings->GetValue(gui::ss_games_state).toByteArray();
	if (restore_layout && !m_game_table->horizontalHeader()->restoreState(game_table_state) && m_game_table->rowCount())
	{
		// If no settings exist, resize to contents. (disabled)
		//m_game_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	}

	const QByteArray savestate_table_state = m_gui_settings->GetValue(gui::ss_savestate_state).toByteArray();
	if (restore_layout && !m_savestate_table->horizontalHeader()->restoreState(savestate_table_state) && m_savestate_table->rowCount())
	{
		// If no settings exist, resize to contents. (disabled)
		//m_savestate_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	}

	if (restore_layout)
	{
		// Make sure the actions and the headers are synced
		m_game_table->sync_header_actions(m_game_column_acts, [this](int col) { return m_gui_settings->GetSavestateGamelistColVisibility(static_cast<gui::savestate_game_list_columns>(col)); });
		m_savestate_table->sync_header_actions(m_savestate_column_acts, [this](int col) { return m_gui_settings->GetSavestateListColVisibility(static_cast<gui::savestate_list_columns>(col)); });
	}

	// Show dialog and then paint gui in order to adjust headers correctly
	show();
	ReadjustGameTable();
	ReadjustSavestateTable();
}

void savestate_manager_dialog::HandleRepaintUiRequest()
{
	const QSize window_size = size();
	const QByteArray splitter_state = m_splitter->saveState();
	const QByteArray game_table_state = m_game_table->horizontalHeader()->saveState();
	const QByteArray savestate_table_state = m_savestate_table->horizontalHeader()->saveState();

	RepaintUI(false);

	m_splitter->restoreState(splitter_state);
	m_game_table->horizontalHeader()->restoreState(game_table_state);
	m_savestate_table->horizontalHeader()->restoreState(savestate_table_state);

	// Make sure the actions and the headers are synced
	m_game_table->sync_header_actions(m_game_column_acts, [this](int col) { return m_gui_settings->GetSavestateGamelistColVisibility(static_cast<gui::savestate_game_list_columns>(col)); });
	m_savestate_table->sync_header_actions(m_savestate_column_acts, [this](int col) { return m_gui_settings->GetSavestateListColVisibility(static_cast<gui::savestate_list_columns>(col)); });

	resize(window_size);
}

void savestate_manager_dialog::ResizeGameIcons()
{
	if (m_game_combo->count() <= 0)
		return;

	WaitAndAbortGameRepaintThreads();

	QPixmap placeholder(m_game_icon_size);
	placeholder.fill(Qt::transparent);

	qRegisterMetaType<QVector<int>>("QVector<int>");
	for (int i = 0; i < m_game_table->rowCount(); ++i)
	{
		if (QTableWidgetItem* icon_item = m_game_table->item(i, static_cast<int>(gui::savestate_game_list_columns::icon)))
		{
			icon_item->setData(Qt::DecorationRole, placeholder);
		}
	}

	ReadjustGameTable();

	for (int i = 0; i < m_game_table->rowCount(); ++i)
	{
		if (movie_item* item = static_cast<movie_item*>(m_game_table->item(i, static_cast<int>(gui::savestate_game_list_columns::icon))))
		{
			const qreal dpr = devicePixelRatioF();
			const int savestate_index = item->data(GameUserRole::GameIndex).toInt();
			const std::string icon_path = m_savestate_db[savestate_index]->game_icon_path;

			item->set_icon_load_func([this, icon_path, savestate_index, cancel = item->icon_loading_aborted(), dpr](int index)
			{
				if (cancel && cancel->load())
				{
					return;
				}

				QPixmap icon;

				if (movie_item* item = static_cast<movie_item*>(m_game_table->item(index, static_cast<int>(gui::savestate_game_list_columns::icon))))
				{
					if (!item->data(GameUserRole::GamePixmapLoaded).toBool())
					{
						// Load game icon
						if (!icon.load(QString::fromStdString(icon_path)))
						{
							gui_log.warning("Could not load savestate game icon from path %s", icon_path);
						}
						item->setData(GameUserRole::GamePixmapLoaded, true);
						item->setData(GameUserRole::GamePixmap, icon);
					}
					else
					{
						icon = item->data(GameUserRole::GamePixmap).value<QPixmap>();
					}
				}

				if (cancel && cancel->load())
				{
					return;
				}

				QPixmap new_icon(icon.size() * dpr);
				new_icon.setDevicePixelRatio(dpr);
				new_icon.fill(m_game_icon_color);

				if (!icon.isNull())
				{
					QPainter painter(&new_icon);
					painter.setRenderHint(QPainter::SmoothPixmapTransform);
					painter.drawPixmap(QPoint(0, 0), icon);
					painter.end();
				}

				new_icon = new_icon.scaled(m_game_icon_size * dpr, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

				if (!cancel || !cancel->load())
				{
					Q_EMIT GameIconReady(index, new_icon);
				}
			});
		}
	}
}

void savestate_manager_dialog::ShowSavestateTableContextMenu(const QPoint& pos)
{
	const int row = m_savestate_table->currentRow();
	const QTableWidgetItem* path_item = m_savestate_table->item(row, static_cast<int>(gui::savestate_list_columns::path));

	if (!path_item)
	{
		return;
	}

	QMenu* menu = new QMenu();
	QAction* show_savestate_dir = new QAction(tr("&Open Savestate Directory"), menu);
	QAction* boot_savestate = new QAction(tr("&Boot Savestate"), menu);
	QAction* delete_savestate = new QAction(tr("&Delete Savestate"), menu);

	if (const QTableWidgetItem* comp_item = m_savestate_table->item(row, static_cast<int>(gui::savestate_list_columns::compatible)))
	{
		boot_savestate->setEnabled(comp_item->data(Qt::UserRole).toBool());
	}

	const int db_ind = m_game_combo->currentData().toInt();
	const std::string path = path_item->text().toStdString();

	connect(show_savestate_dir, &QAction::triggered, this, [this, db_ind]()
	{
		gui::utils::open_dir(QString::fromStdString(m_savestate_db[db_ind]->dir_path));
	});

	connect(boot_savestate, &QAction::triggered, this, [this, path]()
	{
		gui_log.notice("Booting savestate from savestate manager...");
		Q_EMIT RequestBoot(path);
	});

	connect(delete_savestate, &QAction::triggered, this, [this, path]()
	{
		gui_log.notice("Removing savestate '%s' from savestate manager...", path);

		if (QMessageBox::question(this, tr("Confirm Deletion"), tr("Delete savestate '%0'?").arg(QString::fromStdString(path))) != QMessageBox::Yes)
			return;

		if (fs::remove_file(path))
		{
			gui_log.success("Removed savestate '%s'", path);
			StartSavestateLoadThreads(); // Reload the savestate list
		}
		else
		{
			gui_log.error("Failed to remove file '%s' (%s)", path, fs::g_tls_error);
			QMessageBox::warning(this, tr("Deletion Failed!"), tr("Failed to delete savestate '%0'!").arg(QString::fromStdString(path)));
		}
	});

	menu->addAction(boot_savestate);
	menu->addAction(show_savestate_dir);
	menu->addSeparator();
	menu->addAction(delete_savestate);

	menu->exec(m_savestate_table->viewport()->mapToGlobal(pos));
}

void savestate_manager_dialog::ShowGameTableContextMenu(const QPoint& pos)
{
	const int row = m_game_table->currentRow();

	if (!m_game_table->item(row, static_cast<int>(gui::savestate_game_list_columns::icon)))
	{
		return;
	}

	QMenu* menu = new QMenu();
	QAction* remove_savestate_dir = new QAction(tr("&Remove All Savestates"), this);
	QAction* show_savestate_dir = new QAction(tr("&Open Savestate Directory"), menu);

	const int db_ind = m_game_combo->currentData().toInt();

	const QTableWidgetItem* name_item = m_game_table->item(row, static_cast<int>(gui::savestate_game_list_columns::name));
	const QString name = name_item ? name_item->text() : "";

	connect(remove_savestate_dir, &QAction::triggered, this, [this, name, db_ind]()
	{
		if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete the savestates for:\n%0?").arg(name), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			const std::string path = m_savestate_db[db_ind]->dir_path;
			ensure(path != (fs::get_config_dir() + "savestates/")); // Make sure we aren't deleting the root path by accident
			fs::remove_all(path); // Remove the game's savestate folder
			StartSavestateLoadThreads(); // Reload the savestate list
		}
	});
	connect(show_savestate_dir, &QAction::triggered, this, [this, db_ind]()
	{
		gui::utils::open_dir(QString::fromStdString(m_savestate_db[db_ind]->dir_path));
	});

	menu->addAction(show_savestate_dir);

	if (!name.isEmpty())
	{
		QAction* copy_name = new QAction(tr("&Copy Name"), menu);
		connect(copy_name, &QAction::triggered, this, [this, name]()
		{
			QApplication::clipboard()->setText(name);
		});
		menu->addAction(copy_name);
	}

	menu->addSeparator();
	menu->addAction(remove_savestate_dir);

	menu->exec(m_game_table->viewport()->mapToGlobal(pos));
}

void savestate_manager_dialog::StartSavestateLoadThreads()
{
	WaitAndAbortGameRepaintThreads();

	m_savestate_db.clear();

	const QString savestate_path = QString::fromStdString(fs::get_config_dir() + "savestates/");
	const QDir savestate_dir(savestate_path);
	const QStringList folder_list = savestate_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	const int count = folder_list.count();

	if (count <= 0)
	{
		RepaintUI(true);
		return;
	}

	std::vector<std::unique_ptr<game_savestates_data>> game_data(count);

	qRegisterMetaType<QVector<int>>("QVector<int>");
	QList<int> indices;
	for (int i = 0; i < count; ++i)
	{
		indices.append(i);

		game_data[i] = std::make_unique<game_savestates_data>();
		game_data[i]->title_id = folder_list[i].toStdString();

		for (const game_info& gameinfo : m_game_info)
		{
			if (gameinfo && gameinfo->info.serial == game_data[i]->title_id)
			{
				game_data[i]->game_name = gameinfo->info.name;
				game_data[i]->game_icon_path = gameinfo->info.icon_path;
				break;
			}
		}

		if (game_data[i]->game_name.empty())
		{
			game_data[i]->game_name = game_data[i]->title_id;
		}
	}

	QFutureWatcher<void> future_watcher;

	progress_dialog progress_dialog(tr("Loading savestates"), tr("Loading savestates, please wait..."), tr("Cancel"), 0, 1, false, this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

	connect(&future_watcher, &QFutureWatcher<void>::progressRangeChanged, &progress_dialog, &QProgressDialog::setRange);
	connect(&future_watcher, &QFutureWatcher<void>::progressValueChanged, &progress_dialog, &QProgressDialog::setValue);
	connect(&future_watcher, &QFutureWatcher<void>::finished, this, [this]() { RepaintUI(true); });
	connect(&progress_dialog, &QProgressDialog::canceled, this, [this, &future_watcher]()
	{
		future_watcher.cancel();
		close(); // It's pointless to show an empty window
	});

	atomic_t<usz> error_count{};
	future_watcher.setFuture(QtConcurrent::map(indices, [this, &error_count, &game_data](const int& i)
	{
		gui_log.trace("Loading savestate dir: %s", game_data[i]->title_id);

		if (!LoadSavestateFolderToDB(std::move(game_data[i])))
		{
			// TODO: add a way of showing the number of corrupted/invalid savestates in UI somewhere.
			gui_log.error("Error occurred while parsing folder %s for savestates.", game_data[i]->title_id);
			error_count++;
		}
	}));

	progress_dialog.exec();

	future_watcher.waitForFinished();

	if (error_count != 0)
	{
		gui_log.error("Failed to load %d of %d savestates!", error_count.load(), count);
	}
}

void savestate_manager_dialog::PopulateGameTable()
{
	WaitAndAbortGameRepaintThreads();

	m_game_table->setSortingEnabled(false); // Disable sorting before using setItem calls
	m_game_table->clearContents();
	m_game_table->setRowCount(static_cast<int>(m_savestate_db.size()));

	m_game_combo->clear();
	m_game_combo->blockSignals(true);

	qRegisterMetaType<QVector<int>>("QVector<int>");
	QList<int> indices;
	for (usz i = 0; i < m_savestate_db.size(); ++i)
		indices.append(static_cast<int>(i));

	QPixmap placeholder(m_game_icon_size);
	placeholder.fill(Qt::transparent);

	for (int i = 0; i < indices.count(); ++i)
	{
		const QString name = QString::fromStdString(m_savestate_db[i]->game_name).simplified();
		const quint64 savestate_count = m_savestate_db[i]->savestates.size();

		custom_table_widget_item* icon_item = new custom_table_widget_item;
		icon_item->setData(Qt::DecorationRole, placeholder);
		icon_item->setData(GameUserRole::GameIndex, i);
		icon_item->setData(GameUserRole::GamePixmapLoaded, false);
		icon_item->setData(GameUserRole::GamePixmap, QPixmap());

		m_game_table->setItem(i, static_cast<int>(gui::savestate_game_list_columns::icon), icon_item);
		m_game_table->setItem(i, static_cast<int>(gui::savestate_game_list_columns::name), new custom_table_widget_item(name));
		m_game_table->setItem(i, static_cast<int>(gui::savestate_game_list_columns::savestates), new custom_table_widget_item(QString::number(savestate_count), Qt::UserRole, savestate_count));

		m_game_combo->addItem(name, i);
	}

	m_game_combo->model()->sort(0, Qt::AscendingOrder);
	m_game_combo->blockSignals(false);
	m_game_combo->setCurrentIndex(0);

	m_game_table->setSortingEnabled(true); // Enable sorting only after using setItem calls

	ResizeGameIcons();

	gui::utils::resize_combo_box_view(m_game_combo);
}

void savestate_manager_dialog::PopulateSavestateTable()
{
	if (m_game_combo->count() <= 0)
		return;

	const auto& data = m_savestate_db[m_game_combo->currentData().toInt()];
	ensure(!!data);

	gui_log.trace("Populating Savestate Manager UI with %s %s", data->game_name, data->dir_path);

	const std::vector<savestate_data>& savestates = data->savestates;

	m_savestate_table->clear_list();
	m_savestate_table->setRowCount(static_cast<int>(savestates.size()));
	m_savestate_table->setSortingEnabled(false); // Disable sorting before using setItem calls

	const QLocale locale{};

	for (int i = 0; i < static_cast<int>(savestates.size()); i++)
	{
		const savestate_data& savestate = savestates[i];
		m_savestate_table->setItem(i, static_cast<int>(gui::savestate_list_columns::name), new custom_table_widget_item(savestate.name));
		m_savestate_table->setItem(i, static_cast<int>(gui::savestate_list_columns::compatible), new custom_table_widget_item(savestate.is_compatible ? tr("Compatible") : tr("Not compatible"), Qt::UserRole, savestate.is_compatible));
		m_savestate_table->setItem(i, static_cast<int>(gui::savestate_list_columns::date), new custom_table_widget_item(savestate.date.toString(), Qt::UserRole, savestate.date));
		m_savestate_table->setItem(i, static_cast<int>(gui::savestate_list_columns::path), new custom_table_widget_item(savestate.path));
	}

	m_savestate_table->setSortingEnabled(true); // Re-enable sorting after using setItem calls
}

void savestate_manager_dialog::ReadjustGameTable() const
{
	// Fixate vertical header and row height
	m_game_table->verticalHeader()->setDefaultSectionSize(m_game_icon_size.height());
	m_game_table->verticalHeader()->setMinimumSectionSize(m_game_icon_size.height());
	m_game_table->verticalHeader()->setMaximumSectionSize(m_game_icon_size.height());

	// Resize and fixate icon column
	m_game_table->resizeColumnToContents(static_cast<int>(gui::savestate_game_list_columns::icon));
	m_game_table->horizontalHeader()->setSectionResizeMode(static_cast<int>(gui::savestate_game_list_columns::icon), QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_game_table->resizeColumnToContents(static_cast<int>(gui::savestate_game_list_columns::count) - 1);
}

void savestate_manager_dialog::ReadjustSavestateTable() const
{
	// Fixate vertical header and row height
	m_savestate_table->resizeRowsToContents();

	// Shorten the last section to remove horizontal scrollbar if possible
	m_savestate_table->resizeColumnToContents(static_cast<int>(gui::savestate_list_columns::count) - 1);
}

bool savestate_manager_dialog::eventFilter(QObject *object, QEvent *event)
{
	const bool is_savestate_scroll = object == m_savestate_table->verticalScrollBar();
	const bool is_savestate_table  = object == m_savestate_table;
	const bool is_game_scroll      = object == m_game_table->verticalScrollBar();
	const bool is_game_table       = object == m_game_table;
	int zoom_val = 0;

	switch (event->type())
	{
	case QEvent::Wheel:
	{
		QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);

		if (wheelEvent->modifiers() & Qt::ControlModifier && (is_savestate_scroll || is_game_scroll))
		{
			const QPoint numSteps = wheelEvent->angleDelta() / 8 / 15; // http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
			zoom_val = numSteps.y();
		}
		break;
	}
	case QEvent::KeyPress:
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent && keyEvent->modifiers() == Qt::ControlModifier && (is_savestate_table || is_game_table))
		{
			if (keyEvent->key() == Qt::Key_Plus)
			{
				zoom_val = 1;
			}
			else if (keyEvent->key() == Qt::Key_Minus)
			{
				zoom_val = -1;
			}
		}
		break;
	}
	default:
		break;
	}

	if (zoom_val != 0)
	{
		if (m_game_icon_slider && (is_game_table || is_game_scroll))
		{
			m_save_game_icon_size = true;
			m_game_icon_slider->setSliderPosition(zoom_val + m_game_icon_slider->value());
		}
		return true;
	}

	return QWidget::eventFilter(object, event);
}

void savestate_manager_dialog::closeEvent(QCloseEvent *event)
{
	// Save gui settings
	m_gui_settings->SetValue(gui::ss_geometry, saveGeometry(), false);
	m_gui_settings->SetValue(gui::ss_splitterState, m_splitter->saveState(), false);
	m_gui_settings->SetValue(gui::ss_games_state, m_game_table->horizontalHeader()->saveState(), false);
	m_gui_settings->SetValue(gui::ss_savestate_state, m_savestate_table->horizontalHeader()->saveState(), true);

	QWidget::closeEvent(event);
}

void savestate_manager_dialog::WaitAndAbortGameRepaintThreads()
{
	for (int i = 0; i < m_game_table->rowCount(); i++)
	{
		if (movie_item* item = static_cast<movie_item*>(m_game_table->item(i, static_cast<int>(gui::savestate_game_list_columns::icon))))
		{
			item->wait_for_icon_loading(true);
		}
	}
}
