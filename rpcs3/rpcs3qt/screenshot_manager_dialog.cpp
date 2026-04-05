#include "stdafx.h"
#include "screenshot_manager_dialog.h"
#include "screenshot_preview.h"
#include "screenshot_item.h"
#include "flow_widget.h"
#include "qt_utils.h"
#include "Utilities/File.h"
#include "Emu/system_utils.hpp"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QScreen>
#include <QVBoxLayout>
#include <QtConcurrent>

LOG_CHANNEL(gui_log, "GUI");

screenshot_manager_dialog::screenshot_manager_dialog(const std::vector<game_info>& games, QWidget* parent)
	: QDialog(parent), m_games(games)
{
	setWindowTitle(tr("Screenshots"));
	setAttribute(Qt::WA_DeleteOnClose);

	m_icon_size = QSize(160, 90);
	m_flow_widget = new flow_widget(this);
	m_flow_widget->setObjectName("flow_widget");

	m_placeholder = QPixmap(m_icon_size);
	m_placeholder.fill(Qt::gray);

	connect(this, &screenshot_manager_dialog::signal_entry_parsed, this, &screenshot_manager_dialog::add_entry, Qt::ConnectionType::QueuedConnection);

	m_combo_sort_filter = new QComboBox();
	m_combo_sort_filter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_combo_sort_filter->addItem(tr("Sort by Game"), static_cast<int>(sort_filter::game));
	m_combo_sort_filter->addItem(tr("Sort by Date"), static_cast<int>(sort_filter::date));
	connect(m_combo_sort_filter, &QComboBox::currentIndexChanged, this, [this](int /*index*/){ reload(); });

	m_combo_type_filter = new QComboBox();
	m_combo_type_filter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_combo_type_filter->addItem(tr("All Screenshots"), static_cast<int>(type_filter::all));
	m_combo_type_filter->addItem(tr("RPCS3 Screenshots"), static_cast<int>(type_filter::rpcs3));
	m_combo_type_filter->addItem(tr("Cell Screenshots"), static_cast<int>(type_filter::cell));
	connect(m_combo_type_filter, &QComboBox::currentIndexChanged, this, [this](int /*index*/){ reload(); });

	m_combo_game_filter = new QComboBox();
	m_combo_game_filter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_combo_game_filter->addItem(tr("All Games"), QString());
	connect(m_combo_game_filter, &QComboBox::currentIndexChanged, this, [this](int /*index*/){ reload(); });

	QHBoxLayout* sort_layout = new QHBoxLayout();
	sort_layout->addWidget(m_combo_sort_filter);
	QGroupBox* gb_sort = new QGroupBox(tr("Sort"));
	gb_sort->setLayout(sort_layout);

	QHBoxLayout* type_layout = new QHBoxLayout();
	type_layout->addWidget(m_combo_type_filter);
	QGroupBox* gb_type = new QGroupBox(tr("Filter Type"));
	gb_type->setLayout(type_layout);

	QHBoxLayout* game_layout = new QHBoxLayout();
	game_layout->addWidget(m_combo_game_filter);
	QGroupBox* gb_game = new QGroupBox(tr("Filter Game"));
	gb_game->setLayout(game_layout);

	QHBoxLayout* top_layout = new QHBoxLayout();
	top_layout->addWidget(gb_sort);
	top_layout->addWidget(gb_type);
	top_layout->addWidget(gb_game);
	top_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(top_layout);
	layout->addWidget(m_flow_widget);
	setLayout(layout);

	resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

screenshot_manager_dialog::~screenshot_manager_dialog()
{
	m_abort_parsing = true;
	gui::utils::stop_future_watcher(m_parsing_watcher, true);
}

void screenshot_manager_dialog::add_entry(const QString& path)
{
	screenshot_item* item = new screenshot_item(m_flow_widget, m_icon_size, path, m_placeholder);
	connect(item, &screenshot_item::signal_icon_preview, this, &screenshot_manager_dialog::show_preview);

	m_flow_widget->add_widget(item);
}

void screenshot_manager_dialog::show_preview(const QString& path)
{
	screenshot_preview* preview = new screenshot_preview(path);
	preview->show();
}

void screenshot_manager_dialog::reload()
{
	m_abort_parsing = true;
	m_parsing_watcher.disconnect();
	gui::utils::stop_future_watcher(m_parsing_watcher, true);

	const type_filter t_filter = static_cast<type_filter>(m_combo_type_filter->currentData().toInt());
	const sort_filter s_filter = static_cast<sort_filter>(m_combo_sort_filter->currentData().toInt());
	const QString game_filter = m_combo_game_filter->currentData().toString();

	const std::string screenshot_path_rpcs3 = fs::get_config_dir() + "screenshots/";
	const std::string screenshot_path_cell  = rpcs3::utils::get_hdd0_dir() + "/photo/";

	std::vector<std::string> folders;
	switch (t_filter)
	{
	case type_filter::all:
		folders.push_back(screenshot_path_rpcs3);
		folders.push_back(screenshot_path_cell);
		break;
	case type_filter::rpcs3:
		folders.push_back(screenshot_path_rpcs3);
		break;
	case type_filter::cell:
		folders.push_back(screenshot_path_cell);
		break;
	}

	m_flow_widget->clear();
	m_game_folders.clear();
	m_abort_parsing = false;

	connect(&m_parsing_watcher, &QFutureWatcher<void>::finished, this, [this]()
	{
		std::vector<std::pair<QString, QString>> games;
		for (const auto& [dirname, paths] : m_game_folders)
		{
			const std::string serial = dirname.toStdString();
			std::string text = serial;
			for (const auto& game : m_games)
			{
				if (game && game->info.serial == serial)
				{
					text = fmt::format("%s (%s)", game->info.name, serial);
					break;
				}
			}
			games.push_back(std::pair(dirname, QString::fromStdString(text)));
		}

		std::sort(games.begin(), games.end(), [](const std::pair<QString, QString>& l, const std::pair<QString, QString>& r)
		{
			return l.second < r.second;
		});

		const QString old_filter = m_combo_game_filter->currentData().toString();
		m_combo_game_filter->blockSignals(true);
		m_combo_game_filter->clear();
		m_combo_game_filter->addItem(tr("All Games"), QString());
		for (const auto& [dirname, text] : games)
		{
			m_combo_game_filter->addItem(text, dirname);
		}
		m_combo_game_filter->setCurrentIndex(m_combo_game_filter->findData(old_filter));
		m_combo_game_filter->blockSignals(false);
	});

	m_parsing_watcher.setFuture(QtConcurrent::map(m_parsing_threads, [this, folders, game_filter, s_filter](int index)
	{
		if (index != 0)
		{
			return;
		}

		const QStringList filter{ QStringLiteral("*.png") };

		for (const std::string& folder : folders)
		{
			if (m_abort_parsing)
			{
				return;
			}

			if (folder.empty())
			{
				gui_log.error("Screenshot manager: Trying to load screenshots from empty path!");
				continue;
			}

			QDirIterator dir_iter(QString::fromStdString(folder), filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

			while (dir_iter.hasNext() && !m_abort_parsing)
			{
				QFileInfo info(dir_iter.next());
				const QString dirname = info.dir().dirName();
				m_game_folders[dirname].push_back(std::move(info));
			}
		}

		switch (s_filter)
		{
		case sort_filter::game:
		{
			for (const auto& [dirname, infos] : m_game_folders)
			{
				if (game_filter.isEmpty() || game_filter == dirname)
				{
					for (const QFileInfo& info : infos)
					{
						Q_EMIT signal_entry_parsed(info.filePath());
					}
				}
			}
			break;
		}
		case sort_filter::date:
		{
			std::vector<QFileInfo> sorted_infos;
			for (const auto& [dirname, infos] : m_game_folders)
			{
				if (game_filter.isEmpty() || game_filter == dirname)
				{
					sorted_infos.insert(sorted_infos.end(), infos.begin(), infos.end());
				}
			}

			std::sort(sorted_infos.begin(), sorted_infos.end(), [](const QFileInfo& a, const QFileInfo& b)
			{
				return a.lastModified() < b.lastModified();
			});

			for (const QFileInfo& info : sorted_infos)
			{
				Q_EMIT signal_entry_parsed(info.filePath());
			}
			break;
		}
		}
	}));
}

void screenshot_manager_dialog::showEvent(QShowEvent* event)
{
	QDialog::showEvent(event);
	reload();
}
