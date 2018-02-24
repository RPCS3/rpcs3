#include "trophy_manager_dialog.h"
#include "trophy_tree_widget_item.h"

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
#include <QPushButton>
#include <QGroupBox>
#include <QPixmap>
#include <QTableWidget>
#include <QDesktopWidget>
#include <QLabel>
#include <QDir>
#include <QMenu>
#include <QDirIterator>
#include <QDesktopServices>
#include <QUrl>

static const char* m_TROPHY_DIR = "/dev_hdd0/home/00000001/trophy/";

namespace
{
	constexpr auto qstr = QString::fromStdString;
	inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }
}

trophy_manager_dialog::trophy_manager_dialog(std::shared_ptr<gui_settings> gui_settings)
	: QWidget(), m_sort_column(0), m_col_sort_order(Qt::AscendingOrder), m_gui_settings(gui_settings)
{
	// Nonspecific widget settings
	setWindowTitle(tr("Trophy Manager"));

	m_icon_height            = m_gui_settings->GetValue(gui::tr_icon_height).toInt();
	m_show_locked_trophies   = m_gui_settings->GetValue(gui::tr_show_locked).toBool();
	m_show_unlocked_trophies = m_gui_settings->GetValue(gui::tr_show_unlocked).toBool();
	m_show_hidden_trophies   = m_gui_settings->GetValue(gui::tr_show_hidden).toBool();
	m_show_bronze_trophies   = m_gui_settings->GetValue(gui::tr_show_bronze).toBool();
	m_show_silver_trophies   = m_gui_settings->GetValue(gui::tr_show_silver).toBool();
	m_show_gold_trophies     = m_gui_settings->GetValue(gui::tr_show_gold).toBool();
	m_show_platinum_trophies = m_gui_settings->GetValue(gui::tr_show_platinum).toBool();

	// HACK: dev_hdd0 must be mounted for vfs to work for loading trophies.
	const std::string emu_dir_ = g_cfg.vfs.emulator_dir;
	const std::string emu_dir = emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
	vfs::mount("dev_hdd0", fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir));

	// Trophy Tree
	m_trophy_tree = new QTreeWidget();
	m_trophy_tree->setColumnCount(6);

	QStringList column_names;
	column_names << tr("Icon") << tr("Name") << tr("Description") << tr("Type") << tr("Status") << tr("ID");
	m_trophy_tree->setHeaderLabels(column_names);
	m_trophy_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_trophy_tree->header()->setStretchLastSection(false);
	m_trophy_tree->setSortingEnabled(true);
	m_trophy_tree->setContextMenuPolicy(Qt::CustomContextMenu);

	// Populate the trophy database
	QDirIterator dir_iter(qstr(vfs::get(m_TROPHY_DIR)));
	while (dir_iter.hasNext()) 
	{
		dir_iter.next();
		if (dir_iter.fileName() == "." || dir_iter.fileName() == ".." || dir_iter.fileName() == ".gitignore")
		{
			continue;
		}
		std::string dirName = sstr(dir_iter.fileName());
		LOG_TRACE(GENERAL, "Loading trophy dir: %s", dirName);
		LoadTrophyFolderToDB(dirName);
	}

	PopulateUI();
	ApplyFilter();

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

	QLabel* slider_label = new QLabel();
	slider_label->setText(tr("Icon Size: %0").arg(m_icon_height));

	m_icon_slider = new QSlider(Qt::Horizontal);
	m_icon_slider->setRange(25, 225);
	m_icon_slider->setValue(m_icon_height);

	// LAYOUTS
	QGroupBox* show_settings = new QGroupBox(tr("Trophy View Options"));
	show_settings->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	QVBoxLayout* settings_layout = new QVBoxLayout();
	settings_layout->addWidget(check_lock_trophy);
	settings_layout->addWidget(check_unlock_trophy);
	settings_layout->addWidget(check_hidden_trophy);
	settings_layout->addWidget(check_bronze_trophy);
	settings_layout->addWidget(check_silver_trophy);
	settings_layout->addWidget(check_gold_trophy);
	settings_layout->addWidget(check_platinum_trophy);
	show_settings->setLayout(settings_layout);

	QGroupBox* icon_settings = new QGroupBox(tr("Trophy Icon Options"));
	QVBoxLayout* slider_layout = new QVBoxLayout();
	slider_layout->addWidget(slider_label);
	slider_layout->addWidget(m_icon_slider);
	icon_settings->setLayout(slider_layout);

	QVBoxLayout* options_layout = new QVBoxLayout();
	options_layout->addStretch();
	options_layout->addWidget(show_settings);
	options_layout->addWidget(icon_settings);
	options_layout->addStretch();

	QHBoxLayout* all_layout = new QHBoxLayout(this);
	all_layout->addLayout(options_layout);
	all_layout->addWidget(m_trophy_tree);
	all_layout->setStretch(1, 1);
	setLayout(all_layout);

	QByteArray geometry = m_gui_settings->GetValue(gui::tr_geometry).toByteArray();
	if (geometry.isEmpty() == false)
	{
		restoreGeometry(geometry);
	}
	else
	{
		resize(QDesktopWidget().availableGeometry().size() * 0.7);
	}

	// Make connects
	connect(m_icon_slider, &QSlider::valueChanged, this, [=](int val)
	{
		slider_label->setText(tr("Icon Size: %0").arg(val));
		ResizeTrophyIcons(val);
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

	connect(m_trophy_tree->header(), &QHeaderView::sectionClicked, this, &trophy_manager_dialog::OnColClicked);

	connect(m_trophy_tree, &QTableWidget::customContextMenuRequested, this, &trophy_manager_dialog::ShowContextMenu);
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

void trophy_manager_dialog::OnColClicked(int col)
{
	if (col == 0) return; // Don't "sort" icons.

	if (col == m_sort_column)
	{
		m_col_sort_order = (m_col_sort_order == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
	}
	else
	{
		m_col_sort_order = Qt::AscendingOrder;
	}
	m_sort_column = col;

	m_trophy_tree->sortByColumn(m_sort_column, m_col_sort_order);
}

void trophy_manager_dialog::ResizeTrophyIcons(int size)
{
	for (int i = 0; i < m_trophy_tree->topLevelItemCount(); ++i)
	{
		auto* game = m_trophy_tree->topLevelItem(i);
		int db_pos = game->data(1, Qt::UserRole).toInt();

		for (int j = 0; j < game->childCount(); ++j)
		{
			auto* node = game->child(j);
			int trophy_id = node->text(TrophyColumns::Id).toInt();
			node->setData(TrophyColumns::Icon, Qt::DecorationRole, m_trophies_db[db_pos]->trophy_images[trophy_id].scaledToHeight(size, Qt::SmoothTransformation));
			node->setSizeHint(TrophyColumns::Icon, QSize(-1, size));
		}
	}
}

void trophy_manager_dialog::ApplyFilter()
{
	for (int i = 0; i < m_trophy_tree->topLevelItemCount(); ++i)
	{
		auto* game = m_trophy_tree->topLevelItem(i);
		int db_pos = game->data(1, Qt::UserRole).toInt();

		for (int j = 0; j < game->childCount(); ++j)
		{
			auto* node = game->child(j);
			int trophy_id = node->text(TrophyColumns::Id).toInt();
			QString trophy_type = node->text(TrophyColumns::Type);

			// I could use boolean logic and reduce this to something much shorter and also much more confusing...
			bool hidden = node->data(TrophyColumns::Hidden, Qt::UserRole).toBool();
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

			node->setHidden(hide);
		}
	}
}

void trophy_manager_dialog::ShowContextMenu(const QPoint& loc)
{
	QPoint globalPos = m_trophy_tree->mapToGlobal(loc);
	QMenu* menu = new QMenu();
	QTreeWidgetItem* item = m_trophy_tree->currentItem();
	if (!item)
	{
		return;
	}

	QAction* show_trophy_dir = new QAction(tr("Open Trophy Dir"), menu);

	// Only two levels in this tree (ignoring root). So getting the index as such works.
	int db_ind;
	bool is_game_node = m_trophy_tree->indexOfTopLevelItem(item) != -1;
	if (is_game_node)
	{
		db_ind = item->data(1, Qt::UserRole).toInt();
	}
	else
	{
		db_ind = item->parent()->data(1, Qt::UserRole).toInt();
	}

	connect(show_trophy_dir, &QAction::triggered, [=]()
	{
		QString path = qstr(m_trophies_db[db_ind]->path);
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});
	
	menu->addAction(show_trophy_dir);
	menu->exec(globalPos);
}

void trophy_manager_dialog::PopulateUI()
{
	for (int i = 0; i < m_trophies_db.size(); ++i)
	{
		auto& data = m_trophies_db[i];

		LOG_TRACE(GENERAL, "Populating Trophy Manager UI with %s %s", data->game_name, data->path);
		std::shared_ptr<rXmlNode> trophy_base = data->trop_config.GetRoot();
		if (trophy_base->GetChildren()->GetName() == "trophyconf")
		{
			trophy_base = trophy_base->GetChildren();
		}
		else
		{
			LOG_ERROR(GENERAL, "Root name does not match trophyconf in trophy. Name received: %s", trophy_base->GetChildren()->GetName());
			continue;
		}

		QTreeWidgetItem* game_root = new QTreeWidgetItem(m_trophy_tree);
		// Name is set later to include the trophy locked/unlocked count.
		game_root->setData(1, Qt::UserRole, i);
		m_trophy_tree->addTopLevelItem(game_root);

		int unlocked_trophies = 0;

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

			bool unlocked = data->trop_usr->GetTrophyUnlockState(trophy_id);
			if (unlocked)
			{
				++unlocked_trophies;
			}

			trophy_tree_widget_item* trophy_item = new trophy_tree_widget_item(game_root);
			trophy_item->setData(TrophyColumns::Icon, Qt::DecorationRole, data->trophy_images[trophy_id].scaledToHeight(m_icon_height, Qt::SmoothTransformation));
			trophy_item->setSizeHint(TrophyColumns::Icon, QSize(-1, m_icon_height));
			trophy_item->setText(TrophyColumns::Name, qstr(details.name));
			trophy_item->setText(TrophyColumns::Description, qstr(details.description));
			trophy_item->setText(TrophyColumns::Type, trophy_type);
			trophy_item->setText(TrophyColumns::IsUnlocked, unlocked ? "Unlocked" : "Locked");
			trophy_item->setText(TrophyColumns::Id, QString::number(trophy_id));
			trophy_item->setData(TrophyColumns::Hidden, Qt::UserRole, hidden);

			game_root->addChild(trophy_item);
		}

		int all_trophies = data->trop_usr->GetTrophiesCount();
		int percentage = 100 * unlocked_trophies / all_trophies;
		game_root->setText(0, qstr(data->game_name) + QString(" : %1% (%2/%3)").arg(percentage).arg(unlocked_trophies).arg(all_trophies));
	}
}

void trophy_manager_dialog::closeEvent(QCloseEvent * event)
{
	// Save gui settings
	m_gui_settings->SetValue(gui::tr_geometry, saveGeometry());
}
