#include "stdafx.h"
#include "trophy_manager_dialog.h"
#include "custom_table_widget_item.h"
#include "game_list_delegate.h"
#include "qt_utils.h"
#include "game_list.h"
#include "gui_application.h"
#include "gui_settings.h"
#include "progress_dialog.h"
#include "persistent_settings.h"

#include "util/logs.hpp"
#include "Utilities/StrUtil.h"
#include "Utilities/File.h"
#include "Emu/VFS.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Emu/Cell/Modules/cellRtc.h"

#include <QApplication>
#include <QClipboard>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QPixmap>
#include <QDir>
#include <QMenu>
#include <QDesktopServices>
#include <QScrollBar>
#include <QWheelEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QTimeZone>

LOG_CHANNEL(gui_log, "GUI");

enum GameUserRole
{
	GameIndex = Qt::UserRole,
	GamePixmapLoaded,
	GamePixmap
};

trophy_manager_dialog::trophy_manager_dialog(std::shared_ptr<gui_settings> gui_settings)
	: QWidget()
	, m_gui_settings(std::move(gui_settings))
{
	// Nonspecific widget settings
	setWindowTitle(tr("Trophy Manager"));
	setObjectName("trophy_manager");
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);

	m_game_icon_size_index   = m_gui_settings->GetValue(gui::tr_game_iconSize).toInt();
	m_icon_height            = m_gui_settings->GetValue(gui::tr_icon_height).toInt();
	m_show_locked_trophies   = m_gui_settings->GetValue(gui::tr_show_locked).toBool();
	m_show_unlocked_trophies = m_gui_settings->GetValue(gui::tr_show_unlocked).toBool();
	m_show_hidden_trophies   = m_gui_settings->GetValue(gui::tr_show_hidden).toBool();
	m_show_bronze_trophies   = m_gui_settings->GetValue(gui::tr_show_bronze).toBool();
	m_show_silver_trophies   = m_gui_settings->GetValue(gui::tr_show_silver).toBool();
	m_show_gold_trophies     = m_gui_settings->GetValue(gui::tr_show_gold).toBool();
	m_show_platinum_trophies = m_gui_settings->GetValue(gui::tr_show_platinum).toBool();

	// Make sure the directory is mounted
	vfs::mount("/dev_hdd0", rpcs3::utils::get_hdd0_dir());

	// Get the currently selected user's trophy path.
	m_trophy_dir = "/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/";

	// Game chooser combo box
	m_game_combo = new QComboBox();
	m_game_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

	// Game progression label
	m_game_progress = new QLabel(tr("Progress: %1% (%2/%3)").arg(0).arg(0).arg(0));

	// Games Table
	m_game_table = new game_list();
	m_game_table->setObjectName("trophy_manager_game_table");
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
	m_game_table->setColumnCount(static_cast<int>(gui::trophy_game_list_columns::count));
	m_game_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_game_table->horizontalHeader()->setStretchLastSection(true);
	m_game_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_game_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_game_table->verticalHeader()->setVisible(false);
	m_game_table->setAlternatingRowColors(true);
	m_game_table->installEventFilter(this);

	auto add_game_column = [this](gui::trophy_game_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_game_table->setHorizontalHeaderItem(static_cast<int>(col), new QTableWidgetItem(header_text));
		m_game_column_acts.append(new QAction(action_text, this));
	};

	add_game_column(gui::trophy_game_list_columns::icon,     tr("Icon"),     tr("Show Icons"));
	add_game_column(gui::trophy_game_list_columns::name,     tr("Game"),     tr("Show Games"));
	add_game_column(gui::trophy_game_list_columns::progress, tr("Progress"), tr("Show Progress"));
	add_game_column(gui::trophy_game_list_columns::trophies, tr("Trophies"), tr("Show Trophies"));

	// Trophy Table
	m_trophy_table = new game_list();
	m_trophy_table->setObjectName("trophy_manager_trophy_table");
	m_trophy_table->setShowGrid(false);
	m_trophy_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_trophy_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_trophy_table->verticalScrollBar()->installEventFilter(this);
	m_trophy_table->verticalScrollBar()->setSingleStep(20);
	m_trophy_table->horizontalScrollBar()->setSingleStep(20);
	m_trophy_table->setItemDelegate(new game_list_delegate(m_trophy_table));
	m_trophy_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_trophy_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_trophy_table->setColumnCount(static_cast<int>(gui::trophy_list_columns::count));
	m_trophy_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_trophy_table->horizontalHeader()->setStretchLastSection(true);
	m_trophy_table->horizontalHeader()->setSectionResizeMode(static_cast<int>(gui::trophy_list_columns::icon), QHeaderView::Fixed);
	m_trophy_table->verticalHeader()->setVisible(false);
	m_trophy_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_trophy_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_trophy_table->setAlternatingRowColors(true);
	m_trophy_table->installEventFilter(this);

	auto add_trophy_column = [this](gui::trophy_list_columns col, const QString& header_text, const QString& action_text)
	{
		m_trophy_table->setHorizontalHeaderItem(static_cast<int>(col), new QTableWidgetItem(header_text));
		m_trophy_column_acts.append(new QAction(action_text, this));
	};

	add_trophy_column(gui::trophy_list_columns::icon,          tr("Icon"),              tr("Show Icons"));
	add_trophy_column(gui::trophy_list_columns::name,          tr("Name"),              tr("Show Names"));
	add_trophy_column(gui::trophy_list_columns::description,   tr("Description"),       tr("Show Descriptions"));
	add_trophy_column(gui::trophy_list_columns::type,          tr("Type"),              tr("Show Types"));
	add_trophy_column(gui::trophy_list_columns::is_unlocked,   tr("Status"),            tr("Show Status"));
	add_trophy_column(gui::trophy_list_columns::id,            tr("ID"),                tr("Show IDs"));
	add_trophy_column(gui::trophy_list_columns::platinum_link, tr("Platinum Relevant"), tr("Show Platinum Relevant"));
	add_trophy_column(gui::trophy_list_columns::time_unlocked, tr("Time Unlocked"),     tr("Show Time Unlocked"));

	m_splitter = new QSplitter();
	m_splitter->addWidget(m_game_table);
	m_splitter->addWidget(m_trophy_table);

	m_game_icon_size = gui_settings::SizeFromSlider(m_game_icon_size_index);

	// Checkboxes to control dialog
	QCheckBox* check_lock_trophy = new QCheckBox(tr("Show Not Earned Trophies"));
	check_lock_trophy->setCheckable(true);
	check_lock_trophy->setChecked(m_show_locked_trophies);

	QCheckBox* check_unlock_trophy = new QCheckBox(tr("Show Earned Trophies"));
	check_unlock_trophy->setCheckable(true);
	check_unlock_trophy->setChecked(m_show_unlocked_trophies);

	QCheckBox* check_hidden_trophy = new QCheckBox(tr("Show Hidden Trophies"));
	check_hidden_trophy->setCheckable(true);
	check_hidden_trophy->setChecked(m_show_hidden_trophies);

	QCheckBox* check_bronze_trophy = new QCheckBox(tr("Show Bronze Trophies"));
	check_bronze_trophy->setCheckable(true);
	check_bronze_trophy->setChecked(m_show_bronze_trophies);

	QCheckBox* check_silver_trophy = new QCheckBox(tr("Show Silver Trophies"));
	check_silver_trophy->setCheckable(true);
	check_silver_trophy->setChecked(m_show_silver_trophies);

	QCheckBox* check_gold_trophy = new QCheckBox(tr("Show Gold Trophies"));
	check_gold_trophy->setCheckable(true);
	check_gold_trophy->setChecked(m_show_gold_trophies);

	QCheckBox* check_platinum_trophy = new QCheckBox(tr("Show Platinum Trophies"));
	check_platinum_trophy->setCheckable(true);
	check_platinum_trophy->setChecked(m_show_platinum_trophies);

	QLabel* trophy_slider_label = new QLabel();
	trophy_slider_label->setText(tr("Trophy Icon Size: %0x%1").arg(m_icon_height).arg(m_icon_height));

	QLabel* game_slider_label = new QLabel();
	game_slider_label->setText(tr("Game Icon Size: %0x%1").arg(m_game_icon_size.width()).arg(m_game_icon_size.height()));

	m_icon_slider = new QSlider(Qt::Horizontal);
	m_icon_slider->setRange(25, 225);
	m_icon_slider->setValue(m_icon_height);

	m_game_icon_slider = new QSlider(Qt::Horizontal);
	m_game_icon_slider->setRange(0, gui::gl_max_slider_pos);
	m_game_icon_slider->setValue(m_game_icon_size_index);

	// LAYOUTS
	QGroupBox* choose_game = new QGroupBox(tr("Choose Game"));
	QVBoxLayout* choose_layout = new QVBoxLayout();
	choose_layout->addWidget(m_game_combo);
	choose_game->setLayout(choose_layout);

	QGroupBox* trophy_info = new QGroupBox(tr("Trophy Info"));
	QVBoxLayout* info_layout = new QVBoxLayout();
	info_layout->addWidget(m_game_progress);
	trophy_info->setLayout(info_layout);

	QGroupBox* show_settings = new QGroupBox(tr("Trophy View Options"));
	QVBoxLayout* settings_layout = new QVBoxLayout();
	settings_layout->addWidget(check_lock_trophy);
	settings_layout->addWidget(check_unlock_trophy);
	settings_layout->addWidget(check_hidden_trophy);
	settings_layout->addWidget(check_bronze_trophy);
	settings_layout->addWidget(check_silver_trophy);
	settings_layout->addWidget(check_gold_trophy);
	settings_layout->addWidget(check_platinum_trophy);
	show_settings->setLayout(settings_layout);

	QGroupBox* icon_settings = new QGroupBox(tr("Icon Options"));
	QVBoxLayout* slider_layout = new QVBoxLayout();
	slider_layout->addWidget(trophy_slider_label);
	slider_layout->addWidget(m_icon_slider);
	slider_layout->addWidget(game_slider_label);
	slider_layout->addWidget(m_game_icon_slider);
	icon_settings->setLayout(slider_layout);

	QVBoxLayout* options_layout = new QVBoxLayout();
	options_layout->addWidget(choose_game);
	options_layout->addWidget(trophy_info);
	options_layout->addWidget(show_settings);
	options_layout->addWidget(icon_settings);
	options_layout->addStretch();

	QHBoxLayout* all_layout = new QHBoxLayout(this);
	all_layout->addLayout(options_layout);
	all_layout->addWidget(m_splitter);
	all_layout->setStretch(1, 1);
	setLayout(all_layout);

	// Make connects
	connect(m_icon_slider, &QSlider::valueChanged, this, [this, trophy_slider_label](int val)
	{
		m_icon_height = val;
		if (trophy_slider_label)
		{
			trophy_slider_label->setText(tr("Trophy Icon Size: %0x%1").arg(val).arg(val));
		}
		ResizeTrophyIcons();
		if (m_save_icon_height)
		{
			m_save_icon_height = false;
			m_gui_settings->SetValue(gui::tr_icon_height, val);
		}
	});

	connect(m_icon_slider, &QSlider::sliderReleased, this, [this]()
	{
		m_gui_settings->SetValue(gui::tr_icon_height, m_icon_slider->value());
	});

	connect(m_icon_slider, &QSlider::actionTriggered, this, [this](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_icon_height = true; // actionTriggered happens before the value was changed
		}
	});

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
			m_gui_settings->SetValue(gui::tr_game_iconSize, val);
		}
	});

	connect(m_game_icon_slider, &QSlider::sliderReleased, this, [this]()
	{
		m_gui_settings->SetValue(gui::tr_game_iconSize, m_game_icon_slider->value());
	});

	connect(m_game_icon_slider, &QSlider::actionTriggered, this, [this](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_game_icon_size = true; // actionTriggered happens before the value was changed
		}
	});

	connect(check_lock_trophy, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_show_locked_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_locked, checked);
	});

	connect(check_unlock_trophy, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_show_unlocked_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_unlocked, checked);
	});

	connect(check_hidden_trophy, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_show_hidden_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_hidden, checked);
	});

	connect(check_bronze_trophy, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_show_bronze_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_bronze, checked);
	});

	connect(check_silver_trophy, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_show_silver_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_silver, checked);
	});

	connect(check_gold_trophy, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_show_gold_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_gold, checked);
	});

	connect(check_platinum_trophy, &QCheckBox::clicked, this, [this](bool checked)
	{
		m_show_platinum_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_platinum, checked);
	});

	connect(m_trophy_table, &QTableWidget::customContextMenuRequested, this, &trophy_manager_dialog::ShowTrophyTableContextMenu);

	connect(m_game_combo, &QComboBox::currentTextChanged, this, [this]
	{
		PopulateTrophyTable();
		ApplyFilter();
	});

	connect(m_game_table, &QTableWidget::customContextMenuRequested, this, &trophy_manager_dialog::ShowGameTableContextMenu);

	connect(m_game_table, &QTableWidget::itemSelectionChanged, this, [this]
	{
		if (m_game_table->selectedItems().isEmpty())
		{
			return;
		}
		QTableWidgetItem* item = m_game_table->item(m_game_table->selectedItems().first()->row(), static_cast<int>(gui::trophy_game_list_columns::name));
		if (!item)
		{
			return;
		}
		m_game_combo->setCurrentText(item->text());
	});

	connect(this, &trophy_manager_dialog::TrophyIconReady, this, [this](int index, const QPixmap& pixmap)
	{
		if (QTableWidgetItem* icon_item = m_trophy_table->item(index, static_cast<int>(gui::trophy_list_columns::icon)))
		{
			icon_item->setData(Qt::DecorationRole, pixmap);
		}
	});

	connect(this, &trophy_manager_dialog::GameIconReady, this, [this](int index, const QPixmap& pixmap)
	{
		if (QTableWidgetItem* icon_item = m_game_table->item(index, static_cast<int>(gui::trophy_game_list_columns::icon)))
		{
			icon_item->setData(Qt::DecorationRole, pixmap);
		}
	});

	m_trophy_table->create_header_actions(m_trophy_column_acts,
		[this](int col) { return m_gui_settings->GetTrophylistColVisibility(static_cast<gui::trophy_list_columns>(col)); },
		[this](int col, bool visible) { m_gui_settings->SetTrophylistColVisibility(static_cast<gui::trophy_list_columns>(col), visible); });
	
	m_game_table->create_header_actions(m_game_column_acts,
		[this](int col) { return m_gui_settings->GetTrophyGamelistColVisibility(static_cast<gui::trophy_game_list_columns>(col)); },
		[this](int col, bool visible) { m_gui_settings->SetTrophyGamelistColVisibility(static_cast<gui::trophy_game_list_columns>(col), visible); });

	RepaintUI(true);

	StartTrophyLoadThreads();
}

trophy_manager_dialog::~trophy_manager_dialog()
{
	WaitAndAbortGameRepaintThreads();
	WaitAndAbortTrophyRepaintThreads();
}

bool trophy_manager_dialog::LoadTrophyFolderToDB(const std::string& trop_name)
{
	const std::string trophy_path = m_trophy_dir + trop_name;
	const std::string vfs_path = vfs::get(trophy_path + "/");

	if (vfs_path.empty())
	{
		gui_log.error("Failed to load trophy database for %s. Path empty!", trop_name);
		return false;
	}

	// Populate GameTrophiesData
	std::unique_ptr<GameTrophiesData> game_trophy_data = std::make_unique<GameTrophiesData>();

	game_trophy_data->path = vfs_path;
	game_trophy_data->trop_usr = std::make_unique<TROPUSRLoader>();
	const std::string tropusr_path = trophy_path + "/TROPUSR.DAT";
	const std::string tropconf_path = trophy_path + "/TROPCONF.SFM";
	const bool success = game_trophy_data->trop_usr->Load(tropusr_path, tropconf_path).success;

	fs::file config(vfs::get(tropconf_path));

	if (!success || !config)
	{
		gui_log.error("Failed to load trophy database for %s", trop_name);
		return false;
	}

	const u32 trophy_count = game_trophy_data->trop_usr->GetTrophiesCount();

	if (trophy_count == 0)
	{
		gui_log.error("Warning game %s in trophy folder %s usr file reports zero trophies. Cannot load in trophy manager.", game_trophy_data->game_name, game_trophy_data->path);
		return false;
	}

	for (u32 trophy_id = 0; trophy_id < trophy_count; ++trophy_id)
	{
		// A trophy icon has 3 digits from 000 to 999, for example TROP001.PNG
		game_trophy_data->trophy_image_paths[trophy_id] = QString::fromStdString(fmt::format("%sTROP%03d.PNG", game_trophy_data->path, trophy_id));
	}

	// Get game name
	pugi::xml_parse_result res = game_trophy_data->trop_config.Read(config.to_string());
	if (!res)
	{
		gui_log.error("Failed to read trophy xml: %s", tropconf_path);
		return false;
	}

	std::shared_ptr<rXmlNode> trophy_base = game_trophy_data->trop_config.GetRoot();
	if (!trophy_base)
	{
		gui_log.error("Failed to read trophy xml (root is null): %s", tropconf_path);
		return false;
	}

	for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
	{
		if (n->GetName() == "title-name")
		{
			game_trophy_data->game_name = n->GetNodeContent();
			break;
		}
	}

	{
		std::scoped_lock lock(m_trophies_db_mtx);
		m_trophies_db.push_back(std::move(game_trophy_data));
	}

	config.release();
	return true;
}

void trophy_manager_dialog::RepaintUI(bool restore_layout)
{
	if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
	{
		m_game_icon_color = m_gui_settings->GetValue(gui::tr_icon_color).value<QColor>();
	}
	else
	{
		m_game_icon_color = gui::utils::get_label_color("trophy_manager_icon_background_color", Qt::transparent, Qt::transparent);
	}

	PopulateGameTable();

	if (restore_layout && !restoreGeometry(m_gui_settings->GetValue(gui::tr_geometry).toByteArray()))
	{
		resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
	}

	if (restore_layout && !m_splitter->restoreState(m_gui_settings->GetValue(gui::tr_splitterState).toByteArray()))
	{
		const int width_left = m_splitter->width() * 0.4;
		const int width_right = m_splitter->width() - width_left;
		m_splitter->setSizes({ width_left, width_right });
	}

	PopulateTrophyTable();

	const QByteArray game_table_state = m_gui_settings->GetValue(gui::tr_games_state).toByteArray();
	if (restore_layout && !m_game_table->horizontalHeader()->restoreState(game_table_state) && m_game_table->rowCount())
	{
		// If no settings exist, resize to contents. (disabled)
		//m_game_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	}

	const QByteArray trophy_table_state = m_gui_settings->GetValue(gui::tr_trophy_state).toByteArray();
	if (restore_layout && !m_trophy_table->horizontalHeader()->restoreState(trophy_table_state) && m_trophy_table->rowCount())
	{
		// If no settings exist, resize to contents. (disabled)
		//m_trophy_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	}

	if (restore_layout)
	{
		// Make sure the actions and the headers are synced
		m_game_table->sync_header_actions(m_game_column_acts, [this](int col) { return m_gui_settings->GetTrophyGamelistColVisibility(static_cast<gui::trophy_game_list_columns>(col)); });
		m_trophy_table->sync_header_actions(m_trophy_column_acts, [this](int col) { return m_gui_settings->GetTrophylistColVisibility(static_cast<gui::trophy_list_columns>(col)); });
	}

	ApplyFilter();

	// Show dialog and then paint gui in order to adjust headers correctly
	show();
	ReadjustGameTable();
	ReadjustTrophyTable();
}

void trophy_manager_dialog::HandleRepaintUiRequest()
{
	const QSize window_size = size();
	const QByteArray splitter_state = m_splitter->saveState();
	const QByteArray game_table_state = m_game_table->horizontalHeader()->saveState();
	const QByteArray trophy_table_state = m_trophy_table->horizontalHeader()->saveState();

	RepaintUI(false);

	m_splitter->restoreState(splitter_state);
	m_game_table->horizontalHeader()->restoreState(game_table_state);
	m_trophy_table->horizontalHeader()->restoreState(trophy_table_state);

	// Make sure the actions and the headers are synced
	m_game_table->sync_header_actions(m_game_column_acts, [this](int col) { return m_gui_settings->GetTrophyGamelistColVisibility(static_cast<gui::trophy_game_list_columns>(col)); });
	m_trophy_table->sync_header_actions(m_trophy_column_acts, [this](int col) { return m_gui_settings->GetTrophylistColVisibility(static_cast<gui::trophy_list_columns>(col)); });

	resize(window_size);
}

void trophy_manager_dialog::ResizeGameIcons()
{
	if (m_game_combo->count() <= 0)
		return;

	WaitAndAbortGameRepaintThreads();

	QPixmap placeholder(m_game_icon_size);
	placeholder.fill(Qt::transparent);

	qRegisterMetaType<QVector<int>>("QVector<int>");
	for (int i = 0; i < m_game_table->rowCount(); ++i)
	{
		if (QTableWidgetItem* icon_item = m_game_table->item(i, static_cast<int>(gui::trophy_game_list_columns::icon)))
		{
			icon_item->setData(Qt::DecorationRole, placeholder);
		}
	}

	ReadjustGameTable();

	const s32 language_index = gui_application::get_language_id();
	const QString localized_icon = QString::fromStdString(fmt::format("ICON0_%02d.PNG", language_index));

	for (int i = 0; i < m_game_table->rowCount(); ++i)
	{
		if (movie_item* item = static_cast<movie_item*>(m_game_table->item(i, static_cast<int>(gui::trophy_game_list_columns::icon))))
		{
			const qreal dpr = devicePixelRatioF();
			const int trophy_index = item->data(GameUserRole::GameIndex).toInt();
			const QString icon_path = QString::fromStdString(m_trophies_db[trophy_index]->path);

			item->set_icon_load_func([this, icon_path, localized_icon, trophy_index, cancel = item->icon_loading_aborted(), dpr](int index)
			{
				if (cancel && cancel->load())
				{
					return;
				}

				QPixmap icon;

				if (movie_item* item = static_cast<movie_item*>(m_game_table->item(index, static_cast<int>(gui::trophy_game_list_columns::icon))))
				{
					if (!item->data(GameUserRole::GamePixmapLoaded).toBool())
					{
						// Load game icon
						if (!icon.load(icon_path + localized_icon) &&
							!icon.load(icon_path + "ICON0.PNG"))
						{
							gui_log.warning("Could not load trophy game icon from path %s", icon_path);
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

void trophy_manager_dialog::ResizeTrophyIcons()
{
	if (m_game_combo->count() <= 0)
		return;

	WaitAndAbortTrophyRepaintThreads();

	const int db_pos = m_game_combo->currentData().toInt();
	const qreal dpr = devicePixelRatioF();
	const int new_height = m_icon_height * dpr;

	QPixmap placeholder(m_icon_height, m_icon_height);
	placeholder.fill(Qt::transparent);

	for (int i = 0; i < m_trophy_table->rowCount(); ++i)
	{
		if (QTableWidgetItem* icon_item = m_trophy_table->item(i, static_cast<int>(gui::trophy_list_columns::icon)))
		{
			icon_item->setData(Qt::DecorationRole, placeholder);
		}
	}

	ReadjustTrophyTable();

	for (int i = 0; i < m_trophy_table->rowCount(); ++i)
	{
		if (QTableWidgetItem* id_item = m_trophy_table->item(i, static_cast<int>(gui::trophy_list_columns::id)))
		{
			if (movie_item* item = static_cast<movie_item*>(m_trophy_table->item(i, static_cast<int>(gui::trophy_list_columns::icon))))
			{
				item->set_icon_load_func([this, data = ::at32(m_trophies_db, db_pos).get(), trophy_id = id_item->text().toInt(), cancel = item->icon_loading_aborted(), dpr, new_height](int index)
				{
					if (cancel && cancel->load())
					{
						return;
					}

					QPixmap icon;

					if (data)
					{
						bool found_icon{};
						QString path;
						{
							std::scoped_lock lock(m_trophies_db_mtx);
							found_icon = data->trophy_images.contains(trophy_id);

							if (found_icon)
							{
								icon = data->trophy_images[trophy_id];
							}
							else
							{
								path = data->trophy_image_paths[trophy_id];
							}
						}

						if (!found_icon)
						{
							if (icon.load(path))
							{
								std::scoped_lock lock(m_trophies_db_mtx);
								data->trophy_images[trophy_id] = icon;
							}
							else
							{
								gui_log.error("Failed to load trophy icon for trophy %d (icon='%s')", trophy_id, path);
							}
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

					new_icon = new_icon.scaledToHeight(new_height, Qt::SmoothTransformation);

					if (!cancel || !cancel->load())
					{
						Q_EMIT TrophyIconReady(index, new_icon);
					}
				});
			}
		}
	}
}

void trophy_manager_dialog::ApplyFilter()
{
	if (!m_game_combo || m_game_combo->count() <= 0)
		return;

	const int db_pos = m_game_combo->currentData().toInt();
	if (db_pos < 0 || static_cast<usz>(db_pos) >= m_trophies_db.size() || !m_trophies_db[db_pos])
		return;

	const TROPUSRLoader* trop_usr = m_trophies_db[db_pos]->trop_usr.get();
	if (!trop_usr)
		return;

	for (int i = 0; i < m_trophy_table->rowCount(); ++i)
	{
		QTableWidgetItem* item = m_trophy_table->item(i, static_cast<int>(gui::trophy_list_columns::id));
		QTableWidgetItem* type_item = m_trophy_table->item(i, static_cast<int>(gui::trophy_list_columns::type));
		QTableWidgetItem* icon_item = m_trophy_table->item(i, static_cast<int>(gui::trophy_list_columns::icon));

		if (!item || !type_item || !icon_item)
		{
			continue;
		}

		const int trophy_id   = item->text().toInt();
		const int trophy_type = type_item->data(Qt::UserRole).toInt();

		// I could use boolean logic and reduce this to something much shorter and also much more confusing...
		const bool hidden = icon_item->data(Qt::UserRole).toBool();
		const bool trophy_unlocked = trop_usr->GetTrophyUnlockState(trophy_id);

		bool hide = false;

		// Special override to show *just* hidden trophies.
		if (!m_show_unlocked_trophies && !m_show_locked_trophies && m_show_hidden_trophies)
		{
			hide = !hidden;
		}
		else if ((trophy_unlocked && !m_show_unlocked_trophies)
		     || (!trophy_unlocked && !m_show_locked_trophies)
		     || (hidden && !trophy_unlocked && !m_show_hidden_trophies)
		     || (trophy_type == SCE_NP_TROPHY_GRADE_BRONZE && !m_show_bronze_trophies)
		     || (trophy_type == SCE_NP_TROPHY_GRADE_SILVER && !m_show_silver_trophies)
		     || (trophy_type == SCE_NP_TROPHY_GRADE_GOLD && !m_show_gold_trophies)
		     || (trophy_type == SCE_NP_TROPHY_GRADE_PLATINUM && !m_show_platinum_trophies))
		{
			hide = true;
		}

		m_trophy_table->setRowHidden(i, hide);
	}

	ReadjustTrophyTable();
}

void trophy_manager_dialog::ShowTrophyTableContextMenu(const QPoint& pos)
{
	const int row = m_trophy_table->currentRow();

	if (!m_trophy_table->item(row, static_cast<int>(gui::trophy_list_columns::icon)))
	{
		return;
	}

	QMenu* menu = new QMenu();
	QAction* show_trophy_dir = new QAction(tr("&Open Trophy Directory"), menu);

	const int db_ind = m_game_combo->currentData().toInt();

	connect(show_trophy_dir, &QAction::triggered, this, [this, db_ind]()
	{
		const QString path = QString::fromStdString(m_trophies_db[db_ind]->path);
		gui::utils::open_dir(path);
	});

	menu->addAction(show_trophy_dir);

	const QTableWidgetItem* name_item = m_trophy_table->item(row, static_cast<int>(gui::trophy_list_columns::name));
	const QTableWidgetItem* desc_item = m_trophy_table->item(row, static_cast<int>(gui::trophy_list_columns::description));

	const QString name = name_item ? name_item->text() : "";
	const QString desc = desc_item ? desc_item->text() : "";

	if (!name.isEmpty() || !desc.isEmpty())
	{
		QMenu* copy_menu = new QMenu(tr("&Copy Info"), menu);

		if (!name.isEmpty() && !desc.isEmpty())
		{
			QAction* copy_both = new QAction(tr("&Copy Name + Description"), copy_menu);
			connect(copy_both, &QAction::triggered, this, [this, name, desc]()
			{
				QApplication::clipboard()->setText(name % QStringLiteral("\n\n") % desc);
			});
			copy_menu->addAction(copy_both);
		}

		if (!name.isEmpty())
		{
			QAction* copy_name = new QAction(tr("&Copy Name"), copy_menu);
			connect(copy_name, &QAction::triggered, this, [this, name]()
			{
				QApplication::clipboard()->setText(name);
			});
			copy_menu->addAction(copy_name);
		}

		if (!desc.isEmpty())
		{
			QAction* copy_desc = new QAction(tr("&Copy Description"), copy_menu);
			connect(copy_desc, &QAction::triggered, this, [this, desc]()
			{
				QApplication::clipboard()->setText(desc);
			});
			copy_menu->addAction(copy_desc);
		}

		menu->addMenu(copy_menu);
	}

	const QTableWidgetItem* id_item = m_trophy_table->item(row, static_cast<int>(gui::trophy_list_columns::id));
	const QTableWidgetItem* type_item = m_trophy_table->item(row, static_cast<int>(gui::trophy_list_columns::type));
	if (id_item && type_item && !Emu.IsRunning())
	{
		const int type = type_item->data(Qt::UserRole).toInt();
		const int trophy_id = id_item->text().toInt();
		const bool is_unlocked = m_trophies_db[db_ind]->trop_usr->GetTrophyUnlockState(trophy_id);

		QAction* lock_unlock_trophy = new QAction(is_unlocked ? tr("&Lock Trophy") : tr("&Unlock Trophy"), menu);
		connect(lock_unlock_trophy, &QAction::triggered, this, [this, db_ind, trophy_id, is_unlocked, row, type]()
		{
			if (type == SCE_NP_TROPHY_GRADE_PLATINUM && !is_unlocked)
			{
				QMessageBox::information(this, tr("Action not permitted."), tr("Platinum trophies can only be unlocked ingame."), QMessageBox::Ok);
				return;
			}

			auto& db = m_trophies_db[db_ind];
			const std::string path = vfs::retrieve(db->path);
			const std::string tropusr_path = path + "/TROPUSR.DAT";
			const std::string tropconf_path = path + "/TROPCONF.SFM";

			// Reload trophy file just make sure it hasn't changed
			if (!db->trop_usr->Load(tropusr_path, tropconf_path).success)
			{
				gui_log.error("Failed to load trophy file");
				return;
			}

			u64 tick = 0;

			if (is_unlocked)
			{
				if (!db->trop_usr->LockTrophy(trophy_id))
				{
					gui_log.error("Failed to lock trophy %d", trophy_id);
					return;
				}
			}
			else
			{
				tick = DateTimeToTick(QDateTime::currentDateTime());

				if (!db->trop_usr->UnlockTrophy(trophy_id, tick, tick))
				{
					gui_log.error("Failed to unlock trophy %d", trophy_id);
					return;
				}
			}
			if (!db->trop_usr->Save(tropusr_path))
			{
				gui_log.error("Failed to save '%s': error=%s", path, fs::g_tls_error);
				return;
			}
			if (QTableWidgetItem* lock_item = m_trophy_table->item(row, static_cast<int>(gui::trophy_list_columns::is_unlocked)))
			{
				lock_item->setText(db->trop_usr->GetTrophyUnlockState(trophy_id) ? tr("Earned") : tr("Not Earned"));
			}
			if (QTableWidgetItem* date_item = m_trophy_table->item(row, static_cast<int>(gui::trophy_list_columns::time_unlocked)))
			{
				date_item->setText(tick ? QLocale().toString(TickToDateTime(tick), gui::persistent::last_played_date_with_time_of_day_format) : tr("Unknown"));
				date_item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(tick));
			}
		});
		menu->addSeparator();
		menu->addAction(lock_unlock_trophy);
	}

	menu->exec(m_trophy_table->viewport()->mapToGlobal(pos));
}

void trophy_manager_dialog::ShowGameTableContextMenu(const QPoint& pos)
{
	const int row = m_game_table->currentRow();

	if (!m_game_table->item(row, static_cast<int>(gui::trophy_game_list_columns::icon)))
	{
		return;
	}

	QMenu* menu = new QMenu();
	QAction* remove_trophy_dir = new QAction(tr("&Remove"), this);
	QAction* show_trophy_dir = new QAction(tr("&Open Trophy Directory"), menu);

	const int db_ind = m_game_combo->currentData().toInt();

	const QTableWidgetItem* name_item = m_game_table->item(row, static_cast<int>(gui::trophy_game_list_columns::name));
	const QString name = name_item ? name_item->text() : "";

	connect(remove_trophy_dir, &QAction::triggered, this, [this, name, db_ind]()
	{
		if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete the trophies for:\n%1?").arg(name), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			const std::string path = m_trophies_db[db_ind]->path;
			ensure(path != vfs::get(m_trophy_dir)); // Make sure we aren't deleting the root path by accident
			fs::remove_all(path + "/"); // Remove the game's trophy folder
			StartTrophyLoadThreads(); // Reload the trophy list
		}
	});
	connect(show_trophy_dir, &QAction::triggered, this, [this, db_ind]()
	{
		const QString path = QString::fromStdString(m_trophies_db[db_ind]->path);
		gui::utils::open_dir(path);
	});

	menu->addAction(remove_trophy_dir);
	menu->addAction(show_trophy_dir);

	if (!name.isEmpty())
	{
		QAction* copy_name = new QAction(tr("&Copy Name"), menu);
		connect(copy_name, &QAction::triggered, this, [this, name]()
		{
			QApplication::clipboard()->setText(name);
		});
		menu->addAction(copy_name);
	}

	menu->exec(m_game_table->viewport()->mapToGlobal(pos));
}

void trophy_manager_dialog::StartTrophyLoadThreads()
{
	WaitAndAbortGameRepaintThreads();
	WaitAndAbortTrophyRepaintThreads();

	m_trophies_db.clear();

	const QString trophy_path = QString::fromStdString(vfs::get(m_trophy_dir));

	if (trophy_path.isEmpty())
	{
		gui_log.error("Cannot load trophy dir. Path empty!");
		RepaintUI(true);
		return;
	}

	const QDir trophy_dir(trophy_path);
	const QStringList folder_list = trophy_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	const int count = folder_list.count();

	if (count <= 0)
	{
		RepaintUI(true);
		return;
	}

	qRegisterMetaType<QVector<int>>("QVector<int>");
	QList<int> indices;
	for (int i = 0; i < count; ++i)
		indices.append(i);

	QFutureWatcher<void> futureWatcher;

	progress_dialog progressDialog(tr("Loading trophies"), tr("Loading trophy data, please wait..."), tr("Cancel"), 0, 1, false, this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

	connect(&futureWatcher, &QFutureWatcher<void>::progressRangeChanged, &progressDialog, &QProgressDialog::setRange);
	connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged, &progressDialog, &QProgressDialog::setValue);
	connect(&futureWatcher, &QFutureWatcher<void>::finished, this, [this]() { RepaintUI(true); });
	connect(&progressDialog, &QProgressDialog::canceled, this, [this, &futureWatcher]()
	{
		futureWatcher.cancel();
		close(); // It's pointless to show an empty window
	});

	atomic_t<usz> error_count{};
	futureWatcher.setFuture(QtConcurrent::map(indices, [this, &error_count, &folder_list](const int& i)
	{
		const std::string dir_name = folder_list.value(i).toStdString();
		gui_log.trace("Loading trophy dir: %s", dir_name);

		if (!LoadTrophyFolderToDB(dir_name))
		{
			// TODO: add a way of showing the number of corrupted/invalid folders in UI somewhere.
			gui_log.error("Error occurred while parsing folder %s for trophies.", dir_name);
			error_count++;
		}
	}));

	progressDialog.exec();

	futureWatcher.waitForFinished();

	if (error_count != 0)
	{
		gui_log.error("Failed to load %d of %d trophy folders!", error_count.load(), count);
	}
}

void trophy_manager_dialog::PopulateGameTable()
{
	WaitAndAbortGameRepaintThreads();

	m_game_table->setSortingEnabled(false); // Disable sorting before using setItem calls
	m_game_table->clearContents();
	m_game_table->setRowCount(static_cast<int>(m_trophies_db.size()));

	m_game_combo->clear();
	m_game_combo->blockSignals(true);

	qRegisterMetaType<QVector<int>>("QVector<int>");
	QList<int> indices;
	for (usz i = 0; i < m_trophies_db.size(); ++i)
		indices.append(static_cast<int>(i));

	QPixmap placeholder(m_game_icon_size);
	placeholder.fill(Qt::transparent);

	for (int i = 0; i < indices.count(); ++i)
	{
		const int all_trophies = m_trophies_db[i]->trop_usr->GetTrophiesCount();
		const int unlocked_trophies = m_trophies_db[i]->trop_usr->GetUnlockedTrophiesCount();
		const int percentage = 100 * unlocked_trophies / all_trophies;
		const QString progress = tr("%0% (%1/%2)").arg(percentage).arg(unlocked_trophies).arg(all_trophies);
		const QString name = QString::fromStdString(m_trophies_db[i]->game_name).simplified();

		custom_table_widget_item* icon_item = new custom_table_widget_item;
		icon_item->setData(Qt::DecorationRole, placeholder);
		icon_item->setData(GameUserRole::GameIndex, i);
		icon_item->setData(GameUserRole::GamePixmapLoaded, false);
		icon_item->setData(GameUserRole::GamePixmap, QPixmap());

		m_game_table->setItem(i, static_cast<int>(gui::trophy_game_list_columns::icon), icon_item);
		m_game_table->setItem(i, static_cast<int>(gui::trophy_game_list_columns::name), new custom_table_widget_item(name));
		m_game_table->setItem(i, static_cast<int>(gui::trophy_game_list_columns::progress), new custom_table_widget_item(progress, Qt::UserRole, percentage));
		m_game_table->setItem(i, static_cast<int>(gui::trophy_game_list_columns::trophies), new custom_table_widget_item(QString::number(all_trophies), Qt::UserRole, all_trophies));

		m_game_combo->addItem(name, i);
	}

	m_game_combo->model()->sort(0, Qt::AscendingOrder);
	m_game_combo->blockSignals(false);
	m_game_combo->setCurrentIndex(0);

	m_game_table->setSortingEnabled(true); // Enable sorting only after using setItem calls

	ResizeGameIcons();

	gui::utils::resize_combo_box_view(m_game_combo);
}

void trophy_manager_dialog::PopulateTrophyTable()
{
	if (m_game_combo->count() <= 0)
		return;

	auto& data = m_trophies_db[m_game_combo->currentData().toInt()];
	ensure(!!data);

	gui_log.trace("Populating Trophy Manager UI with %s %s", data->game_name, data->path);

	const int all_trophies = data->trop_usr->GetTrophiesCount();
	const int unlocked_trophies = data->trop_usr->GetUnlockedTrophiesCount();
	const int percentage = (all_trophies > 0) ? (100 * unlocked_trophies / all_trophies) : 0;

	m_game_progress->setText(tr("Progress: %1% (%2/%3)").arg(percentage).arg(unlocked_trophies).arg(all_trophies));

	m_trophy_table->clear_list();
	m_trophy_table->setRowCount(all_trophies);
	m_trophy_table->setSortingEnabled(false); // Disable sorting before using setItem calls

	QPixmap placeholder(m_icon_height, m_icon_height);
	placeholder.fill(Qt::transparent);

	const QLocale locale{};

	std::shared_ptr<rXmlNode> trophy_base = data->trop_config.GetRoot();
	if (!trophy_base)
	{
		gui_log.error("Populating Trophy Manager UI failed (root is null): %s %s", data->game_name, data->path);
	}

	int i = 0;
	for (std::shared_ptr<rXmlNode> n = trophy_base ? trophy_base->GetChildren() : nullptr; n; n = n->GetNext())
	{
		// Only show trophies.
		if (n->GetName() != "trophy")
		{
			continue;
		}

		// Get data (stolen graciously from sceNpTrophy.cpp)
		SceNpTrophyDetails details{};

		// Get trophy id
		const s32 trophy_id = atoi(n->GetAttribute("id").c_str());
		details.trophyId = trophy_id;

		// Get platinum link id (we assume there only exists one platinum trophy per game for now)
		const s32 platinum_link_id = atoi(n->GetAttribute("pid").c_str());
		const QString platinum_relevant = platinum_link_id < 0 ? tr("No") : tr("Yes");

		// Get trophy type
		QString trophy_type;

		switch (n->GetAttribute("ttype")[0])
		{
		case 'B': details.trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   trophy_type = tr("Bronze", "Trophy type");   break;
		case 'S': details.trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   trophy_type = tr("Silver", "Trophy type");   break;
		case 'G': details.trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     trophy_type = tr("Gold", "Trophy type");     break;
		case 'P': details.trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; trophy_type = tr("Platinum", "Trophy type"); break;
		default: gui_log.warning("Unknown trophy grade %s", n->GetAttribute("ttype")); break;
		}

		// Get hidden state
		const bool hidden = n->GetAttribute("hidden")[0] == 'y';
		details.hidden = hidden;

		// Get name and detail
		for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext())
		{
			const std::string name = n2->GetName();
			if (name == "name")
			{
				strcpy_trunc(details.name, n2->GetNodeContent());
			}
			else if (name == "detail")
			{
				strcpy_trunc(details.description, n2->GetNodeContent());
			}
		}

		// Get timestamp
		const u64 tick = data->trop_usr->GetTrophyTimestamp(trophy_id);
		const QString datetime = tick ? locale.toString(TickToDateTime(tick), gui::persistent::last_played_date_with_time_of_day_format) : tr("Unknown");

		const QString unlockstate = data->trop_usr->GetTrophyUnlockState(trophy_id) ? tr("Earned") : tr("Not Earned");

		custom_table_widget_item* icon_item = new custom_table_widget_item();
		icon_item->setData(Qt::UserRole, hidden, true);
		icon_item->setData(Qt::DecorationRole, placeholder);

		custom_table_widget_item* type_item = new custom_table_widget_item(trophy_type);
		type_item->setData(Qt::UserRole, static_cast<uint>(details.trophyGrade), true);

		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::icon), icon_item);
		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::name), new custom_table_widget_item(QString::fromStdString(details.name)));
		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::description), new custom_table_widget_item(QString::fromStdString(details.description)));
		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::type), type_item);
		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::is_unlocked), new custom_table_widget_item(unlockstate));
		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::id), new custom_table_widget_item(QString::number(trophy_id), Qt::UserRole, trophy_id));
		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::platinum_link), new custom_table_widget_item(platinum_relevant, Qt::UserRole, platinum_link_id));
		m_trophy_table->setItem(i, static_cast<int>(gui::trophy_list_columns::time_unlocked), new custom_table_widget_item(datetime, Qt::UserRole, QVariant::fromValue<qulonglong>(tick)));

		++i;
	}

	m_trophy_table->setSortingEnabled(true); // Re-enable sorting after using setItem calls

	ResizeTrophyIcons();
}

void trophy_manager_dialog::ReadjustGameTable() const
{
	// Fixate vertical header and row height
	m_game_table->verticalHeader()->setDefaultSectionSize(m_game_icon_size.height());
	m_game_table->verticalHeader()->setMinimumSectionSize(m_game_icon_size.height());
	m_game_table->verticalHeader()->setMaximumSectionSize(m_game_icon_size.height());

	// Resize and fixate icon column
	m_game_table->resizeColumnToContents(static_cast<int>(gui::trophy_game_list_columns::icon));
	m_game_table->horizontalHeader()->setSectionResizeMode(static_cast<int>(gui::trophy_game_list_columns::icon), QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_game_table->resizeColumnToContents(static_cast<int>(gui::trophy_game_list_columns::count) - 1);
}

void trophy_manager_dialog::ReadjustTrophyTable() const
{
	// Fixate vertical header and row height
	m_trophy_table->verticalHeader()->setDefaultSectionSize(m_icon_height);
	m_trophy_table->verticalHeader()->setMinimumSectionSize(m_icon_height);
	m_trophy_table->verticalHeader()->setMaximumSectionSize(m_icon_height);

	// Resize and fixate icon column
	m_trophy_table->resizeColumnToContents(static_cast<int>(gui::trophy_list_columns::icon));

	// Shorten the last section to remove horizontal scrollbar if possible
	m_trophy_table->resizeColumnToContents(static_cast<int>(gui::trophy_list_columns::count) - 1);
}

bool trophy_manager_dialog::eventFilter(QObject *object, QEvent *event)
{
	const bool is_trophy_scroll = object == m_trophy_table->verticalScrollBar();
	const bool is_trophy_table  = object == m_trophy_table;
	const bool is_game_scroll   = object == m_game_table->verticalScrollBar();
	const bool is_game_table    = object == m_game_table;
	int zoom_val = 0;

	switch (event->type())
	{
	case QEvent::Wheel:
	{
		QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);

		if (wheelEvent->modifiers() & Qt::ControlModifier && (is_trophy_scroll || is_game_scroll))
		{
			const QPoint numSteps = wheelEvent->angleDelta() / 8 / 15;	// http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
			zoom_val = numSteps.y();
		}
		break;
	}
	case QEvent::KeyPress:
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent && keyEvent->modifiers() == Qt::ControlModifier && (is_trophy_table || is_game_table))
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
		if (m_icon_slider && (is_trophy_table || is_trophy_scroll))
		{
			m_save_icon_height = true;
			m_icon_slider->setSliderPosition(zoom_val + m_icon_slider->value());
		}
		else if (m_game_icon_slider && (is_game_table || is_game_scroll))
		{
			m_save_game_icon_size = true;
			m_game_icon_slider->setSliderPosition(zoom_val + m_game_icon_slider->value());
		}
		return true;
	}

	return QWidget::eventFilter(object, event);
}

void trophy_manager_dialog::closeEvent(QCloseEvent *event)
{
	// Save gui settings
	m_gui_settings->SetValue(gui::tr_geometry, saveGeometry(), false);
	m_gui_settings->SetValue(gui::tr_splitterState, m_splitter->saveState(), false);
	m_gui_settings->SetValue(gui::tr_games_state,  m_game_table->horizontalHeader()->saveState(), false);
	m_gui_settings->SetValue(gui::tr_trophy_state, m_trophy_table->horizontalHeader()->saveState(), true);

	QWidget::closeEvent(event);
}

void trophy_manager_dialog::WaitAndAbortGameRepaintThreads()
{
	for (int i = 0; i < m_game_table->rowCount(); i++)
	{
		if (movie_item* item = static_cast<movie_item*>(m_game_table->item(i, static_cast<int>(gui::trophy_game_list_columns::icon))))
		{
			item->wait_for_icon_loading(true);
		}
	}
}

void trophy_manager_dialog::WaitAndAbortTrophyRepaintThreads()
{
	for (int i = 0; i < m_trophy_table->rowCount(); i++)
	{
		if (movie_item* item = static_cast<movie_item*>(m_trophy_table->item(i, static_cast<int>(gui::trophy_list_columns::icon))))
		{
			item->wait_for_icon_loading(true);
		}
	}
}

QDateTime trophy_manager_dialog::TickToDateTime(u64 tick)
{
	const CellRtcDateTime rtc_date = tick_to_date_time(tick);
	const QDateTime datetime(
		QDate(rtc_date.year, rtc_date.month, rtc_date.day),
		QTime(rtc_date.hour, rtc_date.minute, rtc_date.second, rtc_date.microsecond / 1000),
		QTimeZone::UTC);
	return datetime.toLocalTime();
}

u64 trophy_manager_dialog::DateTimeToTick(QDateTime date_time)
{
	const QDateTime utc = date_time.toUTC();
	const QDate date = utc.date();
	const QTime time = utc.time();
	const CellRtcDateTime rtc_date = {
		.year = static_cast<u16>(date.year()),
		.month = static_cast<u16>(date.month()),
		.day = static_cast<u16>(date.day()),
		.hour = static_cast<u16>(time.hour()),
		.minute = static_cast<u16>(time.minute()),
		.second = static_cast<u16>(time.second()),
		.microsecond = static_cast<u32>(time.msec() * 1000),
	};
	return date_time_to_tick(rtc_date);
}
