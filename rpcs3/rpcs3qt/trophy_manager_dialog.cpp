#include "trophy_manager_dialog.h"
#include "custom_table_widget_item.h"
#include "table_item_delegate.h"
#include "qt_utils.h"

#include "stdafx.h"

#include "Utilities/Log.h"
#include "Utilities/StrUtil.h"
#include "rpcs3/Emu/VFS.h"
#include "Emu/System.h"

#include "Emu/Memory/Memory.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include "yaml-cpp/yaml.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QPixmap>
#include <QDesktopWidget>
#include <QDir>
#include <QMenu>
#include <QDirIterator>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>
#include <QWheelEvent>

static const char* m_TROPHY_DIR = "/dev_hdd0/home/00000001/trophy/";

namespace
{
	constexpr auto qstr = QString::fromStdString;
	inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }
}

trophy_manager_dialog::trophy_manager_dialog(std::shared_ptr<gui_settings> gui_settings)
	: QWidget(), m_gui_settings(gui_settings)
{
	// Nonspecific widget settings
	setWindowTitle(tr("Trophy Manager"));
	setObjectName("trophy_manager");
	setAttribute(Qt::WA_DeleteOnClose);

	m_game_icon_size_index   = m_gui_settings->GetValue(gui::tr_game_iconSize).toInt();
	m_icon_height            = m_gui_settings->GetValue(gui::tr_icon_height).toInt();
	m_show_locked_trophies   = m_gui_settings->GetValue(gui::tr_show_locked).toBool();
	m_show_unlocked_trophies = m_gui_settings->GetValue(gui::tr_show_unlocked).toBool();
	m_show_hidden_trophies   = m_gui_settings->GetValue(gui::tr_show_hidden).toBool();
	m_show_bronze_trophies   = m_gui_settings->GetValue(gui::tr_show_bronze).toBool();
	m_show_silver_trophies   = m_gui_settings->GetValue(gui::tr_show_silver).toBool();
	m_show_gold_trophies     = m_gui_settings->GetValue(gui::tr_show_gold).toBool();
	m_show_platinum_trophies = m_gui_settings->GetValue(gui::tr_show_platinum).toBool();

	// HACK: dev_hdd0 must be mounted for vfs to work for loading trophies.
	vfs::mount("dev_hdd0", Emu.GetHddDir());

	// Game chooser combo box
	m_game_combo = new QComboBox();
	m_game_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

	// Game progression label
	m_game_progress = new QLabel(tr("Progress: %1% (%2/%3)").arg(0).arg(0).arg(0));

	// Games Table
	m_game_table = new QTableWidget();
	m_game_table->setObjectName("trophy_manager_game_table");
	m_game_table->setShowGrid(false);
	m_game_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_game_table->verticalScrollBar()->installEventFilter(this);
	m_game_table->verticalScrollBar()->setSingleStep(20);
	m_game_table->horizontalScrollBar()->setSingleStep(20);
	m_game_table->setItemDelegate(new table_item_delegate(this));
	m_game_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_game_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_game_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_game_table->setColumnCount(GameColumns::GameColumnsCount);
	m_game_table->setHorizontalHeaderLabels(QStringList{ tr("Icon"), tr("Game"), tr("Progress") });
	m_game_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_game_table->horizontalHeader()->setStretchLastSection(true);
	m_game_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_game_table->verticalHeader()->setVisible(false);
	m_game_table->setAlternatingRowColors(true);
	m_game_table->installEventFilter(this);

	// Trophy Table
	m_trophy_table = new QTableWidget();
	m_trophy_table->setObjectName("trophy_manager_trophy_table");
	m_trophy_table->setShowGrid(false);
	m_trophy_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_trophy_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_trophy_table->verticalScrollBar()->installEventFilter(this);
	m_trophy_table->verticalScrollBar()->setSingleStep(20);
	m_trophy_table->horizontalScrollBar()->setSingleStep(20);
	m_trophy_table->setItemDelegate(new table_item_delegate(this));
	m_trophy_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_trophy_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_trophy_table->setColumnCount(TrophyColumns::Count);
	m_trophy_table->setHorizontalHeaderLabels(QStringList{ tr("Icon"), tr("Name"), tr("Description"), tr("Type"), tr("Status"), tr("ID") });
	m_trophy_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_trophy_table->horizontalHeader()->setStretchLastSection(true);
	m_trophy_table->verticalHeader()->setVisible(false);
	m_trophy_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_trophy_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_trophy_table->setAlternatingRowColors(true);
	m_trophy_table->installEventFilter(this);

	m_splitter = new QSplitter();
	m_splitter->addWidget(m_game_table);
	m_splitter->addWidget(m_trophy_table);

	m_game_icon_size = gui_settings::SizeFromSlider(m_game_icon_size_index);

	// Checkboxes to control dialog
	QCheckBox* check_lock_trophy = new QCheckBox(tr("Show Locked Trophies"));
	check_lock_trophy->setCheckable(true);
	check_lock_trophy->setChecked(m_show_locked_trophies);

	QCheckBox* check_unlock_trophy = new QCheckBox(tr("Show Unlocked Trophies"));
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
	connect(m_icon_slider, &QSlider::valueChanged, this, [=](int val)
	{
		m_icon_height = val;
		trophy_slider_label->setText(tr("Trophy Icon Size: %0x%1").arg(val).arg(val));
		ResizeTrophyIcons();
		if (m_save_icon_height)
		{
			m_save_icon_height = false;
			m_gui_settings->SetValue(gui::tr_icon_height, val);
		}
	});

	connect(m_icon_slider, &QSlider::sliderReleased, this, [&]()
	{
		m_gui_settings->SetValue(gui::tr_icon_height, m_icon_slider->value());
	});

	connect(m_icon_slider, &QSlider::actionTriggered, [&](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_icon_height = true; // actionTriggered happens before the value was changed
		}
	});

	connect(m_game_icon_slider, &QSlider::valueChanged, this, [=](int val)
	{
		m_game_icon_size_index = val;
		m_game_icon_size = gui_settings::SizeFromSlider(val);;
		game_slider_label->setText(tr("Game Icon Size: %0x%1").arg(m_game_icon_size.width()).arg(m_game_icon_size.height()));
		ResizeGameIcons();
		if (m_save_game_icon_size)
		{
			m_save_game_icon_size = false;
			m_gui_settings->SetValue(gui::tr_game_iconSize, val);
		}
	});

	connect(m_game_icon_slider, &QSlider::sliderReleased, this, [&]()
	{
		m_gui_settings->SetValue(gui::tr_game_iconSize, m_game_icon_slider->value());
	});

	connect(m_game_icon_slider, &QSlider::actionTriggered, [&](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_game_icon_size = true; // actionTriggered happens before the value was changed
		}
	});

	connect(check_lock_trophy, &QCheckBox::clicked, [this](bool checked)
	{
		m_show_locked_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_locked, checked);
	});

	connect(check_unlock_trophy, &QCheckBox::clicked, [this](bool checked)
	{
		m_show_unlocked_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_unlocked, checked);
	});

	connect(check_hidden_trophy, &QCheckBox::clicked, [this](bool checked)
	{
		m_show_hidden_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_hidden, checked);
	});

	connect(check_bronze_trophy, &QCheckBox::clicked, [this](bool checked)
	{
		m_show_bronze_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_bronze, checked);
	});

	connect(check_silver_trophy, &QCheckBox::clicked, [this](bool checked)
	{
		m_show_silver_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_silver, checked);
	});

	connect(check_gold_trophy, &QCheckBox::clicked, [this](bool checked)
	{
		m_show_gold_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_gold, checked);
	});

	connect(check_platinum_trophy, &QCheckBox::clicked, [this](bool checked)
	{
		m_show_platinum_trophies = checked;
		ApplyFilter();
		m_gui_settings->SetValue(gui::tr_show_platinum, checked);
	});

	connect(m_trophy_table, &QTableWidget::customContextMenuRequested, this, &trophy_manager_dialog::ShowContextMenu);

	connect(m_game_combo, &QComboBox::currentTextChanged, [this]
	{
		PopulateTrophyTable();
		ApplyFilter();
	});

	connect(m_game_table, &QTableWidget::itemSelectionChanged, [this]
	{
		m_game_combo->setCurrentText(m_game_table->item(m_game_table->currentRow(), GameColumns::GameName)->text());
	});

	RepaintUI(true);
}

bool trophy_manager_dialog::LoadTrophyFolderToDB(const std::string& trop_name)
{
	std::string trophyPath = m_TROPHY_DIR + trop_name;

	// Populate GameTrophiesData
	std::unique_ptr<GameTrophiesData> game_trophy_data = std::make_unique<GameTrophiesData>();

	game_trophy_data->path = vfs::get(trophyPath + "/");
	game_trophy_data->trop_usr.reset(new TROPUSRLoader());
	std::string trophyUsrPath = trophyPath + "/TROPUSR.DAT";
	std::string trophyConfPath = trophyPath + "/TROPCONF.SFM";
	bool success = game_trophy_data->trop_usr->Load(trophyUsrPath, trophyConfPath);

	fs::file config(vfs::get(trophyConfPath));

	if (!success || !config)
	{
		LOG_ERROR(GENERAL, "Failed to load trophy database for %s", trop_name);
		return false;
	}

	if (game_trophy_data->trop_usr->GetTrophiesCount() == 0)
	{
		LOG_ERROR(GENERAL, "Warning game %s in trophy folder %s usr file reports zero trophies.  Cannot load in trophy manager.", game_trophy_data->game_name, game_trophy_data->path);
		return false;
	}

	for (u32 trophy_id = 0; trophy_id < game_trophy_data->trop_usr->GetTrophiesCount(); ++trophy_id)
	{
		// Figure out how many zeros are needed for padding.  (either 0, 1, or 2)
		QString padding = "";
		if (trophy_id < 10)
		{
			padding = "00";
		}
		else if (trophy_id < 100)
		{
			padding = "0";
		}

		QPixmap trophy_icon;
		QString path = qstr(game_trophy_data->path) + "TROP" + padding + QString::number(trophy_id) + ".PNG";
		if (!trophy_icon.load(path))
		{
			LOG_ERROR(GENERAL, "Failed to load trophy icon for trophy %n %s", trophy_id, game_trophy_data->path);
		}
		game_trophy_data->trophy_images.emplace_back(std::move(trophy_icon));
	}

	// Get game name
	game_trophy_data->trop_config.Read(config.to_string());
	std::shared_ptr<rXmlNode> trophy_base = game_trophy_data->trop_config.GetRoot();
	if (trophy_base->GetChildren()->GetName() == "trophyconf")
	{
		trophy_base = trophy_base->GetChildren();
	}
	for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
	{
		if (n->GetName() == "title-name")
		{
			game_trophy_data->game_name = n->GetNodeContent();
			break;
		}
	}

	m_trophies_db.push_back(std::move(game_trophy_data));

	config.release();
	return true;
}

void trophy_manager_dialog::RepaintUI(bool refresh_trophies)
{
	if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
	{
		m_game_icon_color = m_gui_settings->GetValue(gui::gl_iconColor).value<QColor>();
	}
	else
	{
		m_game_icon_color = gui::utils::get_label_color("gamelist_icon_background_color");
	}

	if (refresh_trophies)
	{
		PopulateTrophyDB();
	}

	PopulateGameTable();

	if (!restoreGeometry(m_gui_settings->GetValue(gui::tr_geometry).toByteArray()))
	{
		resize(QDesktopWidget().availableGeometry().size() * 0.7);
	}

	if (!m_splitter->restoreState(m_gui_settings->GetValue(gui::tr_splitterState).toByteArray()))
	{
		const int width_left = m_splitter->width() * 0.4;
		const int width_right = m_splitter->width() - width_left;
		m_splitter->setSizes({ width_left, width_right });
	}

	PopulateTrophyTable();

	QByteArray game_table_state = m_gui_settings->GetValue(gui::tr_games_state).toByteArray();
	if (!m_game_table->horizontalHeader()->restoreState(game_table_state) && m_game_table->rowCount())
	{
		// If no settings exist, resize to contents. (disabled)
		//m_game_table->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
		//m_game_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	}

	QByteArray trophy_table_state = m_gui_settings->GetValue(gui::tr_trophy_state).toByteArray();
	if (!m_trophy_table->horizontalHeader()->restoreState(trophy_table_state) && m_trophy_table->rowCount())
	{
		// If no settings exist, resize to contents. (disabled)
		//m_trophy_table->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
		//m_trophy_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	}

	ApplyFilter();

	// Show dialog and then paint gui in order to adjust headers correctly
	show();
	ReadjustGameTable();
	ReadjustTrophyTable();
}

void trophy_manager_dialog::HandleRepaintUiRequest()
{
	RepaintUI();
}

void trophy_manager_dialog::ResizeGameIcon(int index)
{
	QTableWidgetItem* item = m_game_table->item(index, GameColumns::GameIcon);
	const QPixmap pixmap = item->data(Qt::UserRole).value<QPixmap>();
	const QSize original_size = pixmap.size();

	QPixmap new_pixmap = QPixmap(original_size);
	new_pixmap.fill(m_game_icon_color);

	QPainter painter(&new_pixmap);

	if (!pixmap.isNull())
	{
		painter.drawPixmap(QPoint(0, 0), pixmap);
	}

	painter.end();

	const QPixmap scaled = new_pixmap.scaled(m_game_icon_size, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
	item->setData(Qt::DecorationRole, scaled);
}

void trophy_manager_dialog::ResizeGameIcons()
{
	if (m_game_combo->count() <= 0)
		return;

	for (int i = 0; i < m_game_table->rowCount(); ++i)
	{
		ResizeGameIcon(i);
	}

	ReadjustGameTable();
}

void trophy_manager_dialog::ResizeTrophyIcons()
{
	if (m_game_combo->count() <= 0)
		return;

	int db_pos = m_game_combo->currentData().toInt();

	for (int i = 0; i < m_trophy_table->rowCount(); ++i)
	{
		int trophy_id = m_trophy_table->item(i, TrophyColumns::Id)->text().toInt();
		QPixmap scaled = m_trophies_db[db_pos]->trophy_images[trophy_id].scaledToHeight(m_icon_height, Qt::SmoothTransformation);
		m_trophy_table->item(i, TrophyColumns::Icon)->setData(Qt::DecorationRole, scaled);
	}

	ReadjustTrophyTable();
}

void trophy_manager_dialog::ApplyFilter()
{
	if (m_game_combo->count() <= 0)
		return;

	int db_pos = m_game_combo->currentData().toInt();

	for (int i = 0; i < m_trophy_table->rowCount(); ++i)
	{
		int trophy_id = m_trophy_table->item(i, TrophyColumns::Id)->text().toInt();
		QString trophy_type = m_trophy_table->item(i, TrophyColumns::Type)->text();

		// I could use boolean logic and reduce this to something much shorter and also much more confusing...
		bool hidden = m_trophy_table->item(i, TrophyColumns::Icon)->data(Qt::UserRole).toBool();
		bool trophy_unlocked = m_trophies_db[db_pos]->trop_usr->GetTrophyUnlockState(trophy_id);

		bool hide = false;
		if (trophy_unlocked && !m_show_unlocked_trophies)
		{
			hide = true;
		}
		if (!trophy_unlocked && !m_show_locked_trophies)
		{
			hide = true;
		}
		if (hidden && !trophy_unlocked && !m_show_hidden_trophies)
		{
			hide = true;
		}
		if ((trophy_type == Bronze   && !m_show_bronze_trophies)
		 || (trophy_type == Silver   && !m_show_silver_trophies)
		 || (trophy_type == Gold     && !m_show_gold_trophies)
		 || (trophy_type == Platinum && !m_show_platinum_trophies))
		{
			hide = true;
		}

		// Special override to show *just* hidden trophies.
		if (!m_show_unlocked_trophies && !m_show_locked_trophies && m_show_hidden_trophies)
		{
			hide = !hidden;
		}

		m_trophy_table->setRowHidden(i, hide);
	}

	ReadjustTrophyTable();
}

void trophy_manager_dialog::ShowContextMenu(const QPoint& loc)
{
	QPoint globalPos = m_trophy_table->mapToGlobal(loc);
	QMenu* menu = new QMenu();
	QTableWidgetItem* item = m_trophy_table->item(m_trophy_table->currentRow(), TrophyColumns::Icon);
	if (!item)
	{
		return;
	}

	QAction* show_trophy_dir = new QAction(tr("Open Trophy Dir"), menu);

	int db_ind = m_game_combo->currentData().toInt();

	connect(show_trophy_dir, &QAction::triggered, [=]()
	{
		QString path = qstr(m_trophies_db[db_ind]->path);
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});
	
	menu->addAction(show_trophy_dir);
	menu->exec(globalPos);
}

void trophy_manager_dialog::PopulateTrophyDB()
{
	m_trophies_db.clear();

	QDirIterator dir_iter(qstr(vfs::get(m_TROPHY_DIR)), QDir::Dirs | QDir::NoDotAndDotDot);
	while (dir_iter.hasNext())
	{
		dir_iter.next();
		std::string dirName = sstr(dir_iter.fileName());
		LOG_TRACE(GENERAL, "Loading trophy dir: %s", dirName);
		LoadTrophyFolderToDB(dirName);
	}
}

void trophy_manager_dialog::PopulateGameTable()
{
	m_game_table->setSortingEnabled(false); // Disable sorting before using setItem calls

	m_game_table->clearContents();
	m_game_table->setRowCount(m_trophies_db.size());

	m_game_combo->clear();

	for (int i = 0; i < m_trophies_db.size(); ++i)
	{
		const int all_trophies = m_trophies_db[i]->trop_usr->GetTrophiesCount();
		const int unlocked_trophies = m_trophies_db[i]->trop_usr->GetUnlockedTrophiesCount();
		const int percentage = 100 * unlocked_trophies / all_trophies;
		const std::string icon_path = m_trophies_db[i]->path + "/ICON0.PNG";
		const QString name = qstr(m_trophies_db[i]->game_name).simplified();
		const QString progress = QString("%0% (%1/%2)").arg(percentage).arg(unlocked_trophies).arg(all_trophies);

		m_game_combo->addItem(name, i);

		// Load game icon
		QPixmap icon;
		if (!icon.load(qstr(icon_path)))
		{
			LOG_WARNING(GENERAL, "Could not load trophy game icon from path %s", icon_path);
		}

		custom_table_widget_item* icon_item = new custom_table_widget_item;
		icon_item->setData(Qt::UserRole, icon);

		m_game_table->setItem(i, GameColumns::GameIcon, icon_item);
		m_game_table->setItem(i, GameColumns::GameName, new custom_table_widget_item(name));
		m_game_table->setItem(i, GameColumns::GameProgress, new custom_table_widget_item(progress, Qt::UserRole, percentage));

		ResizeGameIcon(i);
	}

	m_game_table->setSortingEnabled(true); // Enable sorting only after using setItem calls

	gui::utils::resize_combo_box_view(m_game_combo);
}

void trophy_manager_dialog::PopulateTrophyTable()
{
	if (m_game_combo->count() <= 0)
		return;

	auto& data = m_trophies_db[m_game_combo->currentData().toInt()];
	LOG_TRACE(GENERAL, "Populating Trophy Manager UI with %s %s", data->game_name, data->path);

	const int all_trophies = data->trop_usr->GetTrophiesCount();
	const int unlocked_trophies = data->trop_usr->GetUnlockedTrophiesCount();
	const int percentage = 100 * unlocked_trophies / all_trophies;

	m_game_progress->setText(QString("Progress: %1% (%2/%3)").arg(percentage).arg(unlocked_trophies).arg(all_trophies));

	m_trophy_table->clearContents();
	m_trophy_table->setRowCount(all_trophies);
	m_trophy_table->setSortingEnabled(false); // Disable sorting before using setItem calls

	std::shared_ptr<rXmlNode> trophy_base = data->trop_config.GetRoot();
	if (trophy_base->GetChildren()->GetName() == "trophyconf")
	{
		trophy_base = trophy_base->GetChildren();
	}
	else
	{
		LOG_ERROR(GENERAL, "Root name does not match trophyconf in trophy. Name received: %s", trophy_base->GetChildren()->GetName());
		return;
	}

	int i = 0;
	for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
	{
		// Only show trophies.
		if (n->GetName() != "trophy")
		{
			continue;
		}

		s32 trophy_id = atoi(n->GetAttribute("id").c_str());

		// Don't show hidden trophies
		bool hidden = n->GetAttribute("hidden")[0] == 'y';

		// Get data (stolen graciously from sceNpTrophy.cpp)
		SceNpTrophyDetails details;
		details.trophyId = trophy_id;
		QString trophy_type = "";

		switch (n->GetAttribute("ttype")[0])
		{
		case 'B': details.trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   trophy_type = Bronze;   break;
		case 'S': details.trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   trophy_type = Silver;   break;
		case 'G': details.trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     trophy_type = Gold;     break;
		case 'P': details.trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; trophy_type = Platinum; break;
		}

		switch (n->GetAttribute("hidden")[0])
		{
		case 'y': details.hidden = true;  break;
		case 'n': details.hidden = false; break;
		}

		for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext())
		{
			if (n2->GetName() == "name")
			{
				std::string name = n2->GetNodeContent();
				memcpy(details.name, name.c_str(), std::min((size_t)SCE_NP_TROPHY_NAME_MAX_SIZE, name.length() + 1));
			}
			if (n2->GetName() == "detail")
			{
				std::string detail = n2->GetNodeContent();
				memcpy(details.description, detail.c_str(), std::min((size_t)SCE_NP_TROPHY_DESCR_MAX_SIZE, detail.length() + 1));
			}
		}

		QString unlockstate = data->trop_usr->GetTrophyUnlockState(trophy_id) ? tr("Unlocked") : tr("Locked");

		custom_table_widget_item* icon_item = new custom_table_widget_item();
		icon_item->setData(Qt::DecorationRole, data->trophy_images[trophy_id].scaledToHeight(m_icon_height, Qt::SmoothTransformation));
		icon_item->setData(Qt::UserRole, hidden, true);

		custom_table_widget_item* type_item = new custom_table_widget_item(trophy_type);
		type_item->setData(Qt::UserRole, uint(details.trophyGrade), true);

		m_trophy_table->setItem(i, TrophyColumns::Icon, icon_item);
		m_trophy_table->setItem(i, TrophyColumns::Name, new custom_table_widget_item(qstr(details.name)));
		m_trophy_table->setItem(i, TrophyColumns::Description, new custom_table_widget_item(qstr(details.description)));
		m_trophy_table->setItem(i, TrophyColumns::Type, type_item);
		m_trophy_table->setItem(i, TrophyColumns::IsUnlocked, new custom_table_widget_item(unlockstate));
		m_trophy_table->setItem(i, TrophyColumns::Id, new custom_table_widget_item(QString::number(trophy_id), Qt::UserRole, trophy_id));

		++i;
	}

	m_trophy_table->setSortingEnabled(true); // Re-enable sorting after using setItem calls

	ReadjustTrophyTable();
}

void trophy_manager_dialog::ReadjustGameTable()
{
	// Fixate vertical header and row height
	m_game_table->verticalHeader()->setMinimumSectionSize(m_game_icon_size.height());
	m_game_table->verticalHeader()->setMaximumSectionSize(m_game_icon_size.height());
	m_game_table->resizeRowsToContents();

	// Resize and fixate icon column
	m_game_table->resizeColumnToContents(GameColumns::GameIcon);
	m_game_table->horizontalHeader()->setSectionResizeMode(GameColumns::GameIcon, QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_game_table->resizeColumnToContents(GameColumns::GameColumnsCount - 1);
}

void trophy_manager_dialog::ReadjustTrophyTable()
{
	// Fixate vertical header and row height
	m_trophy_table->verticalHeader()->setMinimumSectionSize(m_icon_height);
	m_trophy_table->verticalHeader()->setMaximumSectionSize(m_icon_height);
	m_trophy_table->resizeRowsToContents();

	// Resize and fixate icon column
	m_trophy_table->resizeColumnToContents(TrophyColumns::Icon);
	m_trophy_table->horizontalHeader()->setSectionResizeMode(TrophyColumns::Icon, QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	m_trophy_table->resizeColumnToContents(TrophyColumns::Count - 1);
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
			QPoint numSteps = wheelEvent->angleDelta() / 8 / 15;	// http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
			zoom_val = numSteps.y();
		}
		break;
	}
	case QEvent::KeyPress:
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->modifiers() & Qt::ControlModifier && (is_trophy_table || is_game_table))
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
	m_gui_settings->SetValue(gui::tr_geometry, saveGeometry());
	m_gui_settings->SetValue(gui::tr_splitterState, m_splitter->saveState());
	m_gui_settings->SetValue(gui::tr_games_state,  m_game_table->horizontalHeader()->saveState());
	m_gui_settings->SetValue(gui::tr_trophy_state, m_trophy_table->horizontalHeader()->saveState());

	QWidget::closeEvent(event);
}
