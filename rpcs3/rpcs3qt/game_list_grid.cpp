#include "stdafx.h"
#include "game_list_grid.h"
#include "game_list_grid_item.h"
#include "movie_item.h"
#include "gui_settings.h"
#include "qt_utils.h"
#include "Utilities/File.h"

#include <QApplication>
#include <QStringBuilder>

game_list_grid::game_list_grid()
	: flow_widget(nullptr), game_list_base()
{
	setObjectName("game_list_grid");
	setContextMenuPolicy(Qt::CustomContextMenu);

	m_icon_ready_callback = [this](const game_info& game)
	{
		Q_EMIT IconReady(game);
	};

	connect(this, &game_list_grid::IconReady, this, [this](const game_info& game)
	{
		if (!game || !game->item) return;
		game->item->call_icon_func();
	}, Qt::QueuedConnection); // The default 'AutoConnection' doesn't seem to work in this specific case...

	connect(this, &flow_widget::ItemSelectionChanged, this, [this](int index)
	{
		if (game_list_grid_item* item = static_cast<game_list_grid_item*>(::at32(items(), index)))
		{
			Q_EMIT ItemSelectionChanged(item->game());
		}
	});
}

void game_list_grid::clear_list()
{
	clear();
}

void game_list_grid::populate(
	const std::vector<game_info>& game_data,
	const std::map<QString, QString>& notes_map,
	const std::map<QString, QString>& title_map,
	const std::string& selected_item_id,
	bool play_hover_movies)
{
	clear_list();

	const QString game_icon_path = play_hover_movies ? QString::fromStdString(fs::get_config_dir() + "/Icons/game_icons/") : "";
	game_list_grid_item* selected_item = nullptr;

	blockSignals(true);

	const auto get_title = [&title_map](const QString& serial, const std::string& name) -> QString
	{
		if (const auto it = title_map.find(serial); it != title_map.cend())
		{
			return it->second.simplified();
		}

		return QString::fromStdString(name).simplified();
	};

	for (const auto& game : game_data)
	{
		const QString serial = QString::fromStdString(game->info.serial);
		const QString title = get_title(serial, game->info.name);

		game_list_grid_item* item = new game_list_grid_item(this, game, title);
		item->installEventFilter(this);
		item->setFocusPolicy(Qt::StrongFocus);

		game->item = item;

		if (const auto it = notes_map.find(serial); it != notes_map.cend() && !it->second.isEmpty())
		{
			item->setToolTip(tr("%0 [%1]\n\nNotes:\n%2").arg(title).arg(serial).arg(it->second));
		}
		else
		{
			item->setToolTip(tr("%0 [%1]").arg(title).arg(serial));
		}

		item->set_icon_func([this, item, game](const QVideoFrame& frame)
		{
			if (!item || !game)
			{
				return;
			}

			if (const QPixmap pixmap = item->get_movie_image(frame); item->get_active() && !pixmap.isNull())
			{
				item->set_icon(gui::utils::get_centered_pixmap(pixmap, m_icon_size, 0, 0, 1.0, Qt::FastTransformation));
			}
			else
			{
				std::lock_guard lock(item->pixmap_mutex);

				item->set_icon(game->pxmap);

				if (!game->has_hover_gif && !game->has_hover_pam)
				{
					game->pxmap = {};
				}

				item->stop_movie();
			}
		});

		if (play_hover_movies && game->has_hover_gif)
		{
			item->set_movie_path(game_icon_path % serial % "/hover.gif");
		}
		else if (play_hover_movies && game->has_hover_pam)
		{
			item->set_movie_path(QString::fromStdString(game->info.movie_path));
		}

		if (selected_item_id == game->info.path + game->info.icon_path)
		{
			selected_item = item;
		}

		add_widget(item);
	}

	blockSignals(false);

	// Update layout before setting focus on the selected item
	show();

	QApplication::processEvents();

	select_item(selected_item);
}

void game_list_grid::repaint_icons(QList<game_info>& game_data, const QColor& icon_color, const QSize& icon_size, qreal device_pixel_ratio)
{
	m_icon_size = icon_size;
	m_icon_color = icon_color;

	QPixmap placeholder(icon_size * device_pixel_ratio);
	placeholder.setDevicePixelRatio(device_pixel_ratio);
	placeholder.fill(Qt::transparent);

	const bool show_title = m_icon_size.width() > (gui::gl_icon_size_medium.width() + gui::gl_icon_size_small.width()) / 2;

	for (game_info& game : game_data)
	{
		if (game_list_grid_item* item = static_cast<game_list_grid_item*>(game->item))
		{
			if (item->icon_loading())
			{
				// We already have an icon. Simply set the icon size to let the label scale itself in a quick and dirty fashion.
				item->set_icon_size(m_icon_size);
			}
			else
			{
				// We don't have an icon. Set a placeholder to initialize the layout.
				game->pxmap = placeholder;
				item->call_icon_func();
			}

			item->set_icon_load_func([this, game, device_pixel_ratio, cancel = item->icon_loading_aborted()](int)
			{
				IconLoadFunction(game, device_pixel_ratio, cancel);
			});

			item->adjust_size();
			item->show_title(show_title);
			item->got_visible = false;
		}
	}
}

void game_list_grid::FocusAndSelectFirstEntryIfNoneIs()
{
	if (items().empty() == false)
	{
		items().first()->setFocus();
	}
}

bool game_list_grid::eventFilter(QObject* watched, QEvent* event)
{
	if (!event)
	{
		return false;
	}

	if (event->type() == QEvent::MouseButtonDblClick && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
	{
		if (game_list_grid_item* item = static_cast<game_list_grid_item*>(watched))
		{
			Q_EMIT ItemDoubleClicked(item->game());
			return true;
		}
	}

	return false;
}

void game_list_grid::keyPressEvent(QKeyEvent* event)
{
	if (!event)
	{
		return;
	}

	const auto modifiers = event->modifiers();

	if (modifiers == Qt::ControlModifier && event->key() == Qt::Key_F && !event->isAutoRepeat())
	{
		Q_EMIT FocusToSearchBar();
		return;
	}

	flow_widget::keyPressEvent(event);
}
