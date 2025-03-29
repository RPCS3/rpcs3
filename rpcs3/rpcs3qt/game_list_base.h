#pragma once

#include "gui_game_info.h"

#include <QIcon>
#include <QWidget>

class game_list_base
{
public:
	game_list_base();

	virtual void clear_list(){};
	virtual void populate(
		[[maybe_unused]] const std::vector<game_info>& game_data,
		[[maybe_unused]] const std::map<QString, QString>& notes_map,
		[[maybe_unused]] const std::map<QString, QString>& title_map,
		[[maybe_unused]] const std::string& selected_item_id,
		[[maybe_unused]] bool play_hover_movies){};

	void set_icon_size(QSize size) { m_icon_size = std::move(size); }
	void set_icon_color(QColor color) { m_icon_color = std::move(color); }
	void set_draw_compat_status_to_grid(bool enabled) { m_draw_compat_status_to_grid = enabled; }

	virtual void repaint_icons(std::vector<game_info>& game_data, const QColor& icon_color, const QSize& icon_size, qreal device_pixel_ratio);

	/** Sets the custom config icon. */
	static QIcon GetCustomConfigIcon(const game_info& game);

protected:
	void IconLoadFunction(game_info game, qreal device_pixel_ratio, std::shared_ptr<atomic_t<bool>> cancel);
	QPixmap PaintedPixmap(const QPixmap& icon, qreal device_pixel_ratio, bool paint_config_icon = false, bool paint_pad_config_icon = false, const QColor& compatibility_color = {}) const;
	QColor GetGridCompatibilityColor(const QString& string) const;

	std::function<void(const movie_item_base*)> m_icon_ready_callback{};
	bool m_draw_compat_status_to_grid{};
	bool m_is_list_layout{};
	QSize m_icon_size{};
	QColor m_icon_color{};
};
