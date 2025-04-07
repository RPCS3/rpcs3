#include "stdafx.h"
#include "game_list_base.h"

#include <QDir>
#include <QPainter>

#include <cmath>
#include <unordered_set>

LOG_CHANNEL(game_list_log, "GameList");

game_list_base::game_list_base()
{
}

void game_list_base::repaint_icons(std::vector<game_info>& game_data, const QColor& icon_color, const QSize& icon_size, qreal device_pixel_ratio)
{
	m_icon_size = icon_size;
	m_icon_color = icon_color;

	QPixmap placeholder(icon_size * device_pixel_ratio);
	placeholder.setDevicePixelRatio(device_pixel_ratio);
	placeholder.fill(Qt::transparent);

	for (game_info& game : game_data)
	{
		game->pxmap = placeholder;

		if (movie_item_base* item = game->item)
		{
			item->set_icon_load_func([this, game, device_pixel_ratio, cancel = item->icon_loading_aborted()](int)
			{
				IconLoadFunction(game, device_pixel_ratio, cancel);
			});

			item->image_change_callback();
		}
	}
}

void game_list_base::IconLoadFunction(game_info game, qreal device_pixel_ratio, std::shared_ptr<atomic_t<bool>> cancel)
{
	if (cancel && cancel->load())
	{
		return;
	}

	static std::unordered_set<std::string> warn_once_list;
	static shared_mutex s_mtx;

	if (game->icon.isNull() && (game->info.icon_path.empty() || !game->icon.load(QString::fromStdString(game->info.icon_path))))
	{
		if (game_list_log.warning)
		{
			bool logged = false;
			{
				std::lock_guard lock(s_mtx);
				logged = !warn_once_list.emplace(game->info.icon_path).second;
			}

			if (!logged)
			{
				game_list_log.warning("Could not load image from path %s", QDir(QString::fromStdString(game->info.icon_path)).absolutePath());
			}
		}
	}

	if (!game->item || (cancel && cancel->load()))
	{
		return;
	}

	const QColor color = GetGridCompatibilityColor(game->compat.color);
	{
		std::lock_guard lock(game->item->pixmap_mutex);
		game->pxmap = PaintedPixmap(game->icon, device_pixel_ratio, game->has_custom_config, game->has_custom_pad_config, color);
	}

	if (!cancel || !cancel->load())
	{
		if (m_icon_ready_callback)
			m_icon_ready_callback(game->item);
	}
}

QPixmap game_list_base::PaintedPixmap(const QPixmap& icon, qreal device_pixel_ratio, bool paint_config_icon, bool paint_pad_config_icon, const QColor& compatibility_color) const
{
	QSize canvas_size(320, 176);
	QSize icon_size(icon.size());
	QPoint target_pos;

	if (!icon.isNull())
	{
		// Let's upscale the original icon to at least fit into the outer rect of the size of PS3's ICON0.PNG
		if (icon_size.width() < 320 || icon_size.height() < 176)
		{
			icon_size.scale(320, 176, Qt::KeepAspectRatio);
		}

		canvas_size = icon_size;

		// Calculate the centered size and position of the icon on our canvas.
		if (icon_size.width() != 320 || icon_size.height() != 176)
		{
			ensure(icon_size.height() > 0);
			constexpr double target_ratio = 320.0 / 176.0; // aspect ratio 20:11

			if ((icon_size.width() / static_cast<double>(icon_size.height())) > target_ratio)
			{
				canvas_size.setHeight(std::ceil(icon_size.width() / target_ratio));
			}
			else
			{
				canvas_size.setWidth(std::ceil(icon_size.height() * target_ratio));
			}

			target_pos.setX(std::max<int>(0, (canvas_size.width() - icon_size.width()) / 2.0));
			target_pos.setY(std::max<int>(0, (canvas_size.height() - icon_size.height()) / 2.0));
		}
	}

	// Create a canvas large enough to fit our entire scaled icon
	QPixmap canvas(canvas_size * device_pixel_ratio);
	canvas.setDevicePixelRatio(device_pixel_ratio);
	canvas.fill(m_icon_color);

	// Create a painter for our canvas
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// Draw the icon onto our canvas
	if (!icon.isNull())
	{
		painter.drawPixmap(target_pos.x(), target_pos.y(), icon_size.width(), icon_size.height(), icon);
	}

	// Draw config icons if necessary
	if (!m_is_list_layout && (paint_config_icon || paint_pad_config_icon))
	{
		const int width = canvas_size.width() * 0.2;
		const QPoint origin = QPoint(canvas_size.width() - width, 0);
		QString icon_path;

		if (paint_config_icon && paint_pad_config_icon)
		{
			icon_path = ":/Icons/combo_config_bordered.png";
		}
		else if (paint_config_icon)
		{
			icon_path = ":/Icons/custom_config.png";
		}
		else if (paint_pad_config_icon)
		{
			icon_path = ":/Icons/controllers.png";
		}

		QPixmap custom_config_icon(icon_path);
		custom_config_icon.setDevicePixelRatio(device_pixel_ratio);
		painter.drawPixmap(origin, custom_config_icon.scaled(QSize(width, width) * device_pixel_ratio, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
	}

	// Draw game compatibility icons if necessary
	if (compatibility_color.isValid())
	{
		const int size = canvas_size.height() * 0.2;
		const int spacing = canvas_size.height() * 0.05;
		QColor copyColor = QColor(compatibility_color);
		copyColor.setAlpha(215); // ~85% opacity
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(QBrush(copyColor));
		painter.setPen(QPen(Qt::black, std::max(canvas_size.width() / 320, canvas_size.height() / 176)));
		painter.drawEllipse(spacing, spacing, size, size);
	}

	// Finish the painting
	painter.end();

	// Scale and return our final image
	return canvas.scaled(m_icon_size * device_pixel_ratio, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
}

QColor game_list_base::GetGridCompatibilityColor(const QString& string) const
{
	if (m_draw_compat_status_to_grid && !m_is_list_layout)
	{
		return QColor(string);
	}
	return QColor();
}

QIcon game_list_base::GetCustomConfigIcon(const game_info& game)
{
	if (!game)
		return {};

	static const QIcon icon_combo_config_bordered(":/Icons/combo_config_bordered.png");
	static const QIcon icon_custom_config(":/Icons/custom_config.png");
	static const QIcon icon_controllers(":/Icons/controllers.png");

	if (game->has_custom_config && game->has_custom_pad_config)
	{
		return icon_combo_config_bordered;
	}

	if (game->has_custom_config)
	{
		return icon_custom_config;
	}

	if (game->has_custom_pad_config)
	{
		return icon_controllers;
	}

	return {};
}
