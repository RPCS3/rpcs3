#include "trophy_manager_dialog.h"
#include "custom_table_widget_item.h"
#include "table_item_delegate.h"
#include "qt_utils.h"
#include "game_list.h"

#include "stdafx.h"

#include "Utilities/Log.h"
#include "Utilities/StrUtil.h"
#include "rpcs3/Emu/VFS.h"
#include "Emu/System.h"

#include "Emu/Memory/vm.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include "yaml-cpp/yaml.h"

#include <QtConcurrent>
#include <QFutureWatcher>
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
#include <QProgressDialog>
#include <QGuiApplication>
#include <QScreen>

LOG_CHANNEL(gui_log, "GUI");

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
	vfs::mount("/dev_hdd0", Emulator::GetHddDir());

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
	m_game_table->setItemDelegate(new table_item_delegate(this, true));
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
	m_trophy_table = new game_list();
	m_trophy_table->setObjectName("trophy_manager_trophy_table");
	m_trophy_table->setShowGrid(false);
	m_trophy_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_trophy_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_trophy_table->verticalScrollBar()->installEventFilter(this);
	m_trophy_table->verticalScrollBar()->setSingleStep(20);
	m_trophy_table->horizontalScrollBar()->setSingleStep(20);
	m_trophy_table->setItemDelegate(new table_item_delegate(this, true));
	m_trophy_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_trophy_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_trophy_table->setColumnCount(TrophyColumns::Count);
	m_trophy_table->setHorizontalHeaderLabels(QStringList{ tr("Icon"), tr("Name"), tr("Description"), tr("Type"), tr("Status"), tr("ID"), tr("Platinum Relevant") });
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
		if (m_game_table->selectedItems().isEmpty())
		{
			return;
		}
		QTableWidgetItem* item = m_game_table->item(m_game_table->selectedItems().first()->row(), GameColumns::GameName);
		if (!item)
		{
			return;
		}
		m_game_combo->setCurrentText(item->text());
	});

	RepaintUI(true);

	StartTrophyLoadThreads();
}

trophy_manager_dialog::~trophy_manager_dialog()
{
}

bool trophy_manager_dialog::LoadTrophyFolderToDB(const std::string& trop_name)
{
	const std::string trophyPath = m_trophy_dir + trop_name;

	// Populate GameTrophiesData
	std::unique_ptr<GameTrophiesData> game_trophy_data = std::make_unique<GameTrophiesData>();

	game_trophy_data->path = vfs::get(trophyPath + "/");
	game_trophy_data->trop_usr.reset(new TROPUSRLoader());
	const std::string trophyUsrPath = trophyPath + "/TROPUSR.DAT";
	const std::string trophyConfPath = trophyPath + "/TROPCONF.SFM";
	const bool success = game_trophy_data->trop_usr->Load(trophyUsrPath, trophyConfPath);

	fs::file config(vfs::get(trophyConfPath));

	if (!success || !config)
	{
		gui_log.error("Failed to load trophy database for %s", trop_name);
		return false;
	}

	const u32 trophy_count = game_trophy_data->trop_usr->GetTrophiesCount();

	if (trophy_count == 0)
	{
		gui_log.error("Warning game %s in trophy folder %s usr file reports zero trophies.  Cannot load in trophy manager.", game_trophy_data->game_name, game_trophy_data->path);
		return false;
	}

	for (u32 trophy_id = 0; trophy_id < trophy_count; ++trophy_id)
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
		const QString path = qstr(game_trophy_data->path) + "TROP" + padding + QString::number(trophy_id) + ".PNG";
		if (!trophy_icon.load(path))
		{
			gui_log.error("Failed to load trophy icon for trophy %d %s", trophy_id, game_trophy_data->path);
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

void trophy_manager_dialog::RepaintUI(bool restore_layout)
{
	if (m_gui_settings->GetValue(gui::m_enableUIColors).toBool())
	{
		m_game_icon_color = m_gui_settings->GetValue(gui::tr_icon_color).value<QColor>();
	}
	else
	{
		m_game_icon_color = gui::utils::get_label_color("trophy_manager_icon_background_color");
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

	QByteArray game_table_state = m_gui_settings->GetValue(gui::tr_games_state).toByteArray();
	if (restore_layout && !m_game_table->horizontalHeader()->restoreState(game_table_state) && m_game_table->rowCount())
	{
		// If no settings exist, resize to contents. (disabled)
		//m_game_table->verticalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
		//m_game_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
	}

	QByteArray trophy_table_state = m_gui_settings->GetValue(gui::tr_trophy_state).toByteArray();
	if (restore_layout && !m_trophy_table->horizontalHeader()->restoreState(trophy_table_state) && m_trophy_table->rowCount())
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
	const QSize window_size = size();
	const QByteArray splitter_state = m_splitter->saveState();
	const QByteArray game_table_state = m_game_table->horizontalHeader()->saveState();
	const QByteArray trophy_table_state = m_trophy_table->horizontalHeader()->saveState();

	RepaintUI(false);

	m_splitter->restoreState(splitter_state);
	m_game_table->horizontalHeader()->restoreState(game_table_state);
	m_trophy_table->horizontalHeader()->restoreState(trophy_table_state);

	resize(window_size);
}

QPixmap trophy_manager_dialog::GetResizedGameIcon(int index)
{
	QTableWidgetItem* item = m_game_table->item(index, GameColumns::GameIcon);
	if (!item)
	{
		return QPixmap();
	}
	const QPixmap icon = item->data(Qt::UserRole).value<QPixmap>();
	const qreal dpr = devicePixelRatioF();

	QPixmap new_icon = QPixmap(icon.size() * dpr);
	new_icon.setDevicePixelRatio(dpr);
	new_icon.fill(m_game_icon_color);

	if (!icon.isNull())
	{
		QPainter painter(&new_icon);
		painter.drawPixmap(QPoint(0, 0), icon);
		painter.end();
	}

	return new_icon.scaled(m_game_icon_size * dpr, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
}

void trophy_manager_dialog::ResizeGameIcons()
{
	if (m_game_combo->count() <= 0)
		return;

	QList<int> indices;
	for (int i = 0; i < m_game_table->rowCount(); ++i)
		indices.append(i);

	std::function<QPixmap(const int&)> get_scaled = [this](const int& i)
	{
		return GetResizedGameIcon(i);
	};

	QList<QPixmap> scaled = QtConcurrent::blockingMapped<QList<QPixmap>>(indices, get_scaled);

	for (int i = 0; i < m_game_table->rowCount() && i < scaled.count(); ++i)
	{
		QTableWidgetItem* icon_item = m_game_table->item(i, TrophyColumns::Icon);
		if (icon_item)
			icon_item->setData(Qt::DecorationRole, scaled[i]);
	}

	ReadjustGameTable();
}

void trophy_manager_dialog::ResizeTrophyIcons()
{
	if (m_game_combo->count() <= 0)
		return;

	const int db_pos = m_game_combo->currentData().toInt();
	const qreal dpr = devicePixelRatioF();
	const int new_height = m_icon_height * dpr;

	QList<int> indices;
	for (int i = 0; i < m_trophy_table->rowCount(); ++i)
		indices.append(i);

	std::function<QPixmap(const int&)> get_scaled = [this, db_pos, dpr, new_height](const int& i)
	{
		QTableWidgetItem* item = m_trophy_table->item(i, TrophyColumns::Id);
		QTableWidgetItem* icon_item = m_trophy_table->item(i, TrophyColumns::Icon);
		if (!item || !icon_item)
		{
			return QPixmap();
		}
		const int trophy_id = item->text().toInt();
		const QPixmap icon = m_trophies_db[db_pos]->trophy_images[trophy_id];

		QPixmap new_icon = QPixmap(icon.size() * dpr);
		new_icon.setDevicePixelRatio(dpr);
		new_icon.fill(m_game_icon_color);

		if (!icon.isNull())
		{
			QPainter painter(&new_icon);
			painter.drawPixmap(QPoint(0, 0), icon);
			painter.end();
		}

		return new_icon.scaledToHeight(new_height, Qt::SmoothTransformation);
	};

	QList<QPixmap> scaled = QtConcurrent::blockingMapped<QList<QPixmap>>(indices, get_scaled);

	for (int i = 0; i < m_trophy_table->rowCount() && i < scaled.count(); ++i)
	{
		QTableWidgetItem* icon_item = m_trophy_table->item(i, TrophyColumns::Icon);
		if (icon_item)
			icon_item->setData(Qt::DecorationRole, scaled[i]);
	}

	ReadjustTrophyTable();
}

void trophy_manager_dialog::ApplyFilter()
{
	if (!m_game_combo || m_game_combo->count() <= 0)
		return;

	const int db_pos = m_game_combo->currentData().toInt();
	if (db_pos >= m_trophies_db.size() || !m_trophies_db[db_pos])
		return;

	const auto trop_usr = m_trophies_db[db_pos]->trop_usr.get();
	if (!trop_usr)
		return;

	for (int i = 0; i < m_trophy_table->rowCount(); ++i)
	{
		QTableWidgetItem* item = m_trophy_table->item(i, TrophyColumns::Id);
		QTableWidgetItem* type_item = m_trophy_table->item(i, TrophyColumns::Type);
		QTableWidgetItem* icon_item = m_trophy_table->item(i, TrophyColumns::Icon);

		if (!item || !type_item || !icon_item)
		{
			continue;
		}

		const int trophy_id = item->text().toInt();
		const QString trophy_type = type_item->text();

		// I could use boolean logic and reduce this to something much shorter and also much more confusing...
		const bool hidden = icon_item->data(Qt::UserRole).toBool();
		const bool trophy_unlocked = trop_usr->GetTrophyUnlockState(trophy_id);

		bool hide = false;

		// Special override to show *just* hidden trophies.
		if (!m_show_unlocked_trophies && !m_show_locked_trophies && m_show_hidden_trophies)
		{
			hide = !hidden;
		}
		else if (trophy_unlocked && !m_show_unlocked_trophies)
		{
			hide = true;
		}
		else if (!trophy_unlocked && !m_show_locked_trophies)
		{
			hide = true;
		}
		else if (hidden && !trophy_unlocked && !m_show_hidden_trophies)
		{
			hide = true;
		}
		else if ((trophy_type == Bronze   && !m_show_bronze_trophies)
		 || (trophy_type == Silver   && !m_show_silver_trophies)
		 || (trophy_type == Gold     && !m_show_gold_trophies)
		 || (trophy_type == Platinum && !m_show_platinum_trophies))
		{
			hide = true;
		}

		m_trophy_table->setRowHidden(i, hide);
	}

	ReadjustTrophyTable();
}

void trophy_manager_dialog::ShowContextMenu(const QPoint& pos)
{
	QTableWidgetItem* item = m_trophy_table->item(m_trophy_table->currentRow(), TrophyColumns::Icon);
	if (!item)
	{
		return;
	}

	QMenu* menu = new QMenu();
	QAction* show_trophy_dir = new QAction(tr("Open Trophy Dir"), menu);

	const int db_ind = m_game_combo->currentData().toInt();

	connect(show_trophy_dir, &QAction::triggered, [=]()
	{
		QString path = qstr(m_trophies_db[db_ind]->path);
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});

	menu->addAction(show_trophy_dir);
	menu->exec(m_trophy_table->viewport()->mapToGlobal(pos));
}

void trophy_manager_dialog::StartTrophyLoadThreads()
{
	m_trophies_db.clear();

	QDir trophy_dir(qstr(vfs::get(m_trophy_dir)));
	const auto folder_list = trophy_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	const int count = folder_list.count();

	if (count <= 0)
	{
		RepaintUI(true);
		return;
	}

	QList<int> indices;
	for (int i = 0; i < count; ++i)
		indices.append(i);

	QFutureWatcher<void> futureWatcher;

	QProgressDialog progressDialog(tr("Loading trophy data, please wait..."), tr("Cancel"), 0, 1, this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	progressDialog.setWindowTitle(tr("Loading trophies"));

	connect(&futureWatcher, &QFutureWatcher<void>::progressRangeChanged, &progressDialog, &QProgressDialog::setRange);
	connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged, &progressDialog, &QProgressDialog::setValue);
	connect(&futureWatcher, &QFutureWatcher<void>::finished, [this]() { RepaintUI(true); });
	connect(&progressDialog, &QProgressDialog::canceled, [this, &futureWatcher]()
	{
		futureWatcher.cancel();
		this->close(); // It's pointless to show an empty window
	});

	futureWatcher.setFuture(QtConcurrent::map(indices, [this, folder_list, &progressDialog](const int& i)
	{
		const std::string dir_name = sstr(folder_list.value(i));
		gui_log.trace("Loading trophy dir: %s", dir_name);
		try
		{
			LoadTrophyFolderToDB(dir_name);
		}
		catch (const std::exception& e)
		{
			// TODO: Add error checks & throws to LoadTrophyFolderToDB so that they can be caught here.
			// Also add a way of showing the number of corrupted/invalid folders in UI somewhere.
			gui_log.error("Exception occurred while parsing folder %s for trophies: %s", dir_name, e.what());
		}
	}));

	progressDialog.exec();

	futureWatcher.waitForFinished();
}

void trophy_manager_dialog::PopulateGameTable()
{
	m_game_table->setSortingEnabled(false); // Disable sorting before using setItem calls

	m_game_table->clearContents();
	m_game_table->setRowCount(static_cast<int>(m_trophies_db.size()));

	m_game_combo->clear();

	QList<int> indices;
	for (size_t i = 0; i < m_trophies_db.size(); ++i)
		indices.append(static_cast<int>(i));

	std::function<QPixmap(const int&)> get_icon = [this](const int& i)
	{
		// Load game icon
		QPixmap icon;
		const std::string icon_path = m_trophies_db[i]->path + "ICON0.PNG";
		if (!icon.load(qstr(icon_path)))
		{
			gui_log.warning("Could not load trophy game icon from path %s", icon_path);
		}
		return icon;
	};

	QList<QPixmap> icons = QtConcurrent::blockingMapped<QList<QPixmap>>(indices, get_icon);

	for (int i = 0; i < indices.count(); ++i)
	{
		const int all_trophies = m_trophies_db[i]->trop_usr->GetTrophiesCount();
		const int unlocked_trophies = m_trophies_db[i]->trop_usr->GetUnlockedTrophiesCount();
		const int percentage = 100 * unlocked_trophies / all_trophies;
		const QString progress = QString("%0% (%1/%2)").arg(percentage).arg(unlocked_trophies).arg(all_trophies);
		const QString name = qstr(m_trophies_db[i]->game_name).simplified();

		custom_table_widget_item* icon_item = new custom_table_widget_item;
		if (icons.count() > i)
			icon_item->setData(Qt::UserRole, icons[i]);

		m_game_table->setItem(i, GameColumns::GameIcon, icon_item);
		m_game_table->setItem(i, GameColumns::GameName, new custom_table_widget_item(name));
		m_game_table->setItem(i, GameColumns::GameProgress, new custom_table_widget_item(progress, Qt::UserRole, percentage));

		m_game_combo->addItem(name, i);
	}

	ResizeGameIcons();

	m_game_table->setSortingEnabled(true); // Enable sorting only after using setItem calls

	gui::utils::resize_combo_box_view(m_game_combo);
}

void trophy_manager_dialog::PopulateTrophyTable()
{
	if (m_game_combo->count() <= 0)
		return;

	auto& data = m_trophies_db[m_game_combo->currentData().toInt()];
	gui_log.trace("Populating Trophy Manager UI with %s %s", data->game_name, data->path);

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
		gui_log.error("Root name does not match trophyconf in trophy. Name received: %s", trophy_base->GetChildren()->GetName());
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

		// Get data (stolen graciously from sceNpTrophy.cpp)
		SceNpTrophyDetails details;

		// Get trophy id
		const s32 trophy_id = atoi(n->GetAttribute("id").c_str());
		details.trophyId = trophy_id;

		// Get platinum link id (we assume there only exists one platinum trophy per game for now)
		const s32 platinum_link_id = atoi(n->GetAttribute("pid").c_str());
		const QString platinum_relevant = platinum_link_id < 0 ? tr("No") : tr("Yes");

		// Get trophy type
		QString trophy_type = "";

		switch (n->GetAttribute("ttype")[0])
		{
		case 'B': details.trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   trophy_type = Bronze;   break;
		case 'S': details.trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   trophy_type = Silver;   break;
		case 'G': details.trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     trophy_type = Gold;     break;
		case 'P': details.trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; trophy_type = Platinum; break;
		}

		// Get hidden state
		const bool hidden = n->GetAttribute("hidden")[0] == 'y';
		details.hidden = hidden;

		// Get name and detail
		for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext())
		{
			if (n2->GetName() == "name")
			{
				std::string name = n2->GetNodeContent();
				strcpy_trunc(details.name, name);
			}
			if (n2->GetName() == "detail")
			{
				std::string detail = n2->GetNodeContent();
				strcpy_trunc(details.description, detail);
			}
		}

		const QString unlockstate = data->trop_usr->GetTrophyUnlockState(trophy_id) ? tr("Unlocked") : tr("Locked");

		custom_table_widget_item* icon_item = new custom_table_widget_item();
		icon_item->setData(Qt::UserRole, hidden, true);

		custom_table_widget_item* type_item = new custom_table_widget_item(trophy_type);
		type_item->setData(Qt::UserRole, uint(details.trophyGrade), true);

		m_trophy_table->setItem(i, TrophyColumns::Icon, icon_item);
		m_trophy_table->setItem(i, TrophyColumns::Name, new custom_table_widget_item(qstr(details.name)));
		m_trophy_table->setItem(i, TrophyColumns::Description, new custom_table_widget_item(qstr(details.description)));
		m_trophy_table->setItem(i, TrophyColumns::Type, type_item);
		m_trophy_table->setItem(i, TrophyColumns::IsUnlocked, new custom_table_widget_item(unlockstate));
		m_trophy_table->setItem(i, TrophyColumns::Id, new custom_table_widget_item(QString::number(trophy_id), Qt::UserRole, trophy_id));
		m_trophy_table->setItem(i, TrophyColumns::PlatinumLink, new custom_table_widget_item(platinum_relevant, Qt::UserRole, platinum_link_id));

		++i;
	}

	m_trophy_table->setSortingEnabled(true); // Re-enable sorting after using setItem calls

	ResizeTrophyIcons();
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
			const QPoint numSteps = wheelEvent->angleDelta() / 8 / 15;	// http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
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
