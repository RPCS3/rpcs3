#include "stdafx.h"
#include "game_list_table.h"
#include "game_list_delegate.h"
#include "game_list_frame.h"
#include "gui_settings.h"
#include "localized.h"
#include "custom_table_widget_item.h"
#include "persistent_settings.h"
#include "qt_utils.h"

#include "Emu/vfs_config.h"
#include "Utilities/StrUtil.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QStringBuilder>

game_list_table::game_list_table(game_list_frame* frame, std::shared_ptr<persistent_settings> persistent_settings)
	: game_list(), m_game_list_frame(frame), m_persistent_settings(std::move(persistent_settings))
{
	m_is_list_layout = true;

	setShowGrid(false);
	setItemDelegate(new game_list_delegate(this));
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	verticalScrollBar()->setSingleStep(20);
	horizontalScrollBar()->setSingleStep(20);
	verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	verticalHeader()->setVisible(false);
	horizontalHeader()->setHighlightSections(false);
	horizontalHeader()->setSortIndicatorShown(true);
	horizontalHeader()->setStretchLastSection(true);
	horizontalHeader()->setDefaultSectionSize(150);
	horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setAlternatingRowColors(true);
	setColumnCount(static_cast<int>(gui::game_list_columns::count));
	setMouseTracking(true);

	connect(this, &game_list_table::size_on_disk_ready, this, [this](const game_info& game)
	{
		if (!game || !game->item) return;
		if (QTableWidgetItem* size_item = item(static_cast<movie_item*>(game->item)->row(), static_cast<int>(gui::game_list_columns::dir_size)))
		{
			const u64& game_size = game->info.size_on_disk;
			size_item->setText(game_size != umax ? gui::utils::format_byte_size(game_size) : tr("Unknown"));
			size_item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(game_size));
		}
	});

	connect(this, &game_list::IconReady, this, [this](const movie_item_base* item)
	{
		if (item) item->image_change_callback();
	});
}

void game_list_table::restore_layout(const QByteArray& state)
{
	// Resize to fit and get the ideal icon column width
	resize_columns_to_contents();
	const int icon_column_width = columnWidth(static_cast<int>(gui::game_list_columns::icon));

	// Restore header layout from last session
	if (!horizontalHeader()->restoreState(state) && rowCount())
	{
		// Nothing to do
	}

	// Make sure no columns are squished
	fix_narrow_columns();

	// Make sure that the icon column is large enough for the actual items.
	// This is important if the list appeared as empty when closing the software before.
	horizontalHeader()->resizeSection(static_cast<int>(gui::game_list_columns::icon), icon_column_width);

	// Save new header state
	horizontalHeader()->restoreState(horizontalHeader()->saveState());
}

void game_list_table::resize_columns_to_contents(int spacing)
{
	horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);

	// Make non-icon columns slightly bigger for better visuals
	for (int i = 1; i < columnCount(); i++)
	{
		if (isColumnHidden(i))
		{
			continue;
		}

		const int size = horizontalHeader()->sectionSize(i) + spacing;
		horizontalHeader()->resizeSection(i, size);
	}
}

void game_list_table::adjust_icon_column()
{
	// Fixate vertical header and row height
	verticalHeader()->setDefaultSectionSize(m_icon_size.height());
	verticalHeader()->setMinimumSectionSize(m_icon_size.height());
	verticalHeader()->setMaximumSectionSize(m_icon_size.height());

	// Resize the icon column
	resizeColumnToContents(static_cast<int>(gui::game_list_columns::icon));

	// Shorten the last section to remove horizontal scrollbar if possible
	resizeColumnToContents(static_cast<int>(gui::game_list_columns::count) - 1);
}

void game_list_table::sort(usz game_count, int sort_column, Qt::SortOrder col_sort_order)
{
	// Back-up old header sizes to handle unwanted column resize in case of zero search results
	const int old_row_count = rowCount();
	const usz old_game_count = game_count;

	std::vector<int> column_widths(columnCount());
	for (int i = 0; i < columnCount(); i++)
	{
		column_widths[i] = columnWidth(i);
	}

	// Sorting resizes hidden columns, so unhide them as a workaround
	std::vector<int> columns_to_hide;

	for (int i = 0; i < columnCount(); i++)
	{
		if (isColumnHidden(i))
		{
			setColumnHidden(i, false);
			columns_to_hide.push_back(i);
		}
	}

	// Sort the list by column and sort order
	sortByColumn(sort_column, col_sort_order);

	// Hide columns again
	for (int col : columns_to_hide)
	{
		setColumnHidden(col, true);
	}

	// Don't resize the columns if no game is shown to preserve the header settings
	if (!rowCount())
	{
		for (int i = 0; i < columnCount(); i++)
		{
			setColumnWidth(i, column_widths[i]);
		}

		horizontalHeader()->setSectionResizeMode(static_cast<int>(gui::game_list_columns::icon), QHeaderView::Fixed);
		return;
	}

	// Fixate vertical header and row height
	verticalHeader()->setDefaultSectionSize(m_icon_size.height());
	verticalHeader()->setMinimumSectionSize(m_icon_size.height());
	verticalHeader()->setMaximumSectionSize(m_icon_size.height());

	// Resize columns if the game list was empty before
	if (!old_row_count && !old_game_count)
	{
		resize_columns_to_contents();
	}
	else
	{
		resizeColumnToContents(static_cast<int>(gui::game_list_columns::icon));
	}

	// Fixate icon column
	horizontalHeader()->setSectionResizeMode(static_cast<int>(gui::game_list_columns::icon), QHeaderView::Fixed);

	// Shorten the last section to remove horizontal scrollbar if possible
	resizeColumnToContents(static_cast<int>(gui::game_list_columns::count) - 1);
}

void game_list_table::set_custom_config_icon(const game_info& game)
{
	if (!game)
	{
		return;
	}

	const QString serial = QString::fromStdString(game->info.serial);

	for (int row = 0; row < rowCount(); ++row)
	{
		if (QTableWidgetItem* title_item = item(row, static_cast<int>(gui::game_list_columns::name)))
		{
			if (const QTableWidgetItem* serial_item = item(row, static_cast<int>(gui::game_list_columns::serial)); serial_item && serial_item->text() == serial)
			{
				title_item->setIcon(game_list_base::GetCustomConfigIcon(game));
			}
		}
	}
}

void game_list_table::populate(
	const std::vector<game_info>& game_data,
	const std::map<QString, QString>& notes_map,
	const std::map<QString, QString>& title_map,
	const std::string& selected_item_id,
	bool play_hover_movies)
{
	clear_list();

	setRowCount(::narrow<int>(game_data.size()));

	// Default locale. Uses current Qt application language.
	const QLocale locale{};
	const Localized localized;

	const std::string dev_flash = g_cfg_vfs.get_dev_flash();

	int row = 0;
	int index = -1;
	int selected_row = -1;

	const auto get_title = [&title_map](const QString& serial, const std::string& name) -> QString
	{
		if (const auto it = title_map.find(serial); it != title_map.cend())
		{
			return it->second;
		}

		return QString::fromStdString(name);
	};

	for (const auto& game : game_data)
	{
		index++;

		const QString serial = QString::fromStdString(game->info.serial);
		const QString title = get_title(serial, game->info.name);

		// Icon
		custom_table_widget_item* icon_item = new custom_table_widget_item;
		game->item = icon_item;

		icon_item->set_image_change_callback([this, icon_item, game](const QVideoFrame& frame)
		{
			if (!icon_item || !game)
			{
				return;
			}

			if (const QPixmap pixmap = icon_item->get_movie_image(frame); icon_item->get_active() && !pixmap.isNull())
			{
				icon_item->setData(Qt::DecorationRole, pixmap.scaled(m_icon_size, Qt::KeepAspectRatio));
				return;
			}

			std::lock_guard lock(icon_item->pixmap_mutex);

			if (!game->pxmap.isNull())
			{
				icon_item->setData(Qt::DecorationRole, game->pxmap);

				if (!game->has_hover_gif && !game->has_hover_pam)
				{
					game->pxmap = {};
				}
			}
		});

		icon_item->set_size_calc_func([this, game, cancel = icon_item->size_on_disk_loading_aborted(), dev_flash]()
		{
			if (game && game->info.size_on_disk == umax && (!cancel || !cancel->load()))
			{
				if (game->info.path.starts_with(dev_flash))
				{
					// Do not report size of apps inside /dev_flash (it does not make sense to do so)
					game->info.size_on_disk = 0;
				}
				else
				{
					game->info.size_on_disk = fs::get_dir_size(game->info.path, 1, cancel.get());
				}

				if (!cancel || !cancel->load())
				{
					Q_EMIT size_on_disk_ready(game);
					return;
				}
			}
		});

		if (play_hover_movies && (game->has_hover_gif || game->has_hover_pam))
		{
			icon_item->set_video_path(game->info.movie_path);
		}

		icon_item->setData(Qt::UserRole, index, true);
		icon_item->setData(gui::custom_roles::game_role, QVariant::fromValue(game));

		// Title
		custom_table_widget_item* title_item = new custom_table_widget_item(title);
		title_item->setIcon(game_list_base::GetCustomConfigIcon(game));

		// Serial
		custom_table_widget_item* serial_item = new custom_table_widget_item(game->info.serial);

		if (const auto it = notes_map.find(serial); it != notes_map.cend() && !it->second.isEmpty())
		{
			const QString tool_tip = tr("%0 [%1]\n\nNotes:\n%2").arg(title).arg(serial).arg(it->second);
			title_item->setToolTip(tool_tip);
			serial_item->setToolTip(tool_tip);
		}

		// Move Support (http://www.psdevwiki.com/ps3/PARAM.SFO#ATTRIBUTE)
		const bool supports_move = game->info.attr & 0x800000;

		// Compatibility
		custom_table_widget_item* compat_item = new custom_table_widget_item;
		compat_item->setText(game->compat.text % (game->compat.date.isEmpty() ? QStringLiteral("") : " (" % game->compat.date % ")"));
		compat_item->setData(Qt::UserRole, game->compat.index, true);
		compat_item->setToolTip(game->compat.tooltip);
		if (!game->compat.color.isEmpty())
		{
			compat_item->setData(Qt::DecorationRole, gui::utils::circle_pixmap(game->compat.color, devicePixelRatioF() * 2));
		}

		// Version
		QString app_version = QString::fromStdString(game->GetGameVersion());

		if (game->info.bootable && !game->compat.latest_version.isEmpty())
		{
			f64 top_ver = 0.0, app_ver = 0.0;
			const bool unknown = app_version == localized.category.unknown;
			const bool ok_app = !unknown && try_to_float(&app_ver, app_version.toStdString(), ::std::numeric_limits<s32>::min(), ::std::numeric_limits<s32>::max());
			const bool ok_top = !unknown && try_to_float(&top_ver, game->compat.latest_version.toStdString(), ::std::numeric_limits<s32>::min(), ::std::numeric_limits<s32>::max());

			// If the app is bootable and the compat database contains info about the latest patch version:
			// add a hint for available software updates if the app version is unknown or lower than the latest version.
			if (unknown || (ok_top && ok_app && top_ver > app_ver))
			{
				app_version = tr("%0 (Update available: %1)").arg(app_version, game->compat.latest_version);
			}
		}

		// Playtimes
		const quint64 elapsed_ms = m_persistent_settings->GetPlaytime(serial);

		// Last played (support outdated values)
		QDateTime last_played;
		const QString last_played_str = m_persistent_settings->GetLastPlayed(serial);

		if (!last_played_str.isEmpty())
		{
			last_played = QDateTime::fromString(last_played_str, gui::persistent::last_played_date_format);

			if (!last_played.isValid())
			{
				last_played = QDateTime::fromString(last_played_str, gui::persistent::last_played_date_format_old);
			}
		}

		const u64 game_size = game->info.size_on_disk;

		setItem(row, static_cast<int>(gui::game_list_columns::icon),       icon_item);
		setItem(row, static_cast<int>(gui::game_list_columns::name),       title_item);
		setItem(row, static_cast<int>(gui::game_list_columns::serial),     serial_item);
		setItem(row, static_cast<int>(gui::game_list_columns::firmware),   new custom_table_widget_item(game->info.fw));
		setItem(row, static_cast<int>(gui::game_list_columns::version),    new custom_table_widget_item(app_version));
		setItem(row, static_cast<int>(gui::game_list_columns::category),   new custom_table_widget_item(game->localized_category));
		setItem(row, static_cast<int>(gui::game_list_columns::path),       new custom_table_widget_item(game->info.path));
		setItem(row, static_cast<int>(gui::game_list_columns::move),       new custom_table_widget_item((supports_move ? tr("Supported") : tr("Not Supported")).toStdString(), Qt::UserRole, !supports_move));
		setItem(row, static_cast<int>(gui::game_list_columns::resolution), new custom_table_widget_item(Localized::GetStringFromU32(game->info.resolution, localized.resolution.mode, true)));
		setItem(row, static_cast<int>(gui::game_list_columns::sound),      new custom_table_widget_item(Localized::GetStringFromU32(game->info.sound_format, localized.sound.format, true)));
		setItem(row, static_cast<int>(gui::game_list_columns::parental),   new custom_table_widget_item(Localized::GetStringFromU32(game->info.parental_lvl, localized.parental.level), Qt::UserRole, game->info.parental_lvl));
		setItem(row, static_cast<int>(gui::game_list_columns::last_play),  new custom_table_widget_item(locale.toString(last_played, last_played >= QDateTime::currentDateTime().addDays(-7) ? gui::persistent::last_played_date_with_time_of_day_format : gui::persistent::last_played_date_format_new), Qt::UserRole, last_played));
		setItem(row, static_cast<int>(gui::game_list_columns::playtime),   new custom_table_widget_item(elapsed_ms == 0 ? tr("Never played") : localized.GetVerboseTimeByMs(elapsed_ms), Qt::UserRole, elapsed_ms));
		setItem(row, static_cast<int>(gui::game_list_columns::compat),     compat_item);
		setItem(row, static_cast<int>(gui::game_list_columns::dir_size),   new custom_table_widget_item(game_size != umax ? gui::utils::format_byte_size(game_size) : tr("Unknown"), Qt::UserRole, QVariant::fromValue<qulonglong>(game_size)));

		if (selected_item_id == game->info.path + game->info.icon_path)
		{
			selected_row = row;
		}

		row++;
	}

	selectRow(selected_row);
}

void game_list_table::repaint_icons(std::vector<game_info>& game_data, const QColor& icon_color, const QSize& icon_size, qreal device_pixel_ratio)
{
	game_list_base::repaint_icons(game_data, icon_color, icon_size, device_pixel_ratio);
	adjust_icon_column();
}
