#pragma once

#include "game_list.h"

class persistent_settings;
class game_list_frame;

class game_list_table : public game_list
{
	Q_OBJECT

public:
	game_list_table(game_list_frame* frame, std::shared_ptr<persistent_settings> persistent_settings);

	/** Restores the initial layout of the table */
	void restore_layout(const QByteArray& state);

	/** Resizes the columns to their contents and adds a small spacing */
	void resize_columns_to_contents(int spacing = 20);

	void adjust_icon_column();

	void sort(usz game_count, int sort_column, Qt::SortOrder col_sort_order);

	void set_custom_config_icon(const game_info& game);

	void populate(
		const std::vector<game_info>& game_data,
		const std::map<QString, QString>& notes_map,
		const std::map<QString, QString>& title_map,
		const std::string& selected_item_id,
		bool play_hover_movies) override;

	void repaint_icons(std::vector<game_info>& game_data, const QColor& icon_color, const QSize& icon_size, qreal device_pixel_ratio) override;

Q_SIGNALS:
	void size_on_disk_ready(const game_info& game);

private:
	game_list_frame* m_game_list_frame{};
	std::shared_ptr<persistent_settings> m_persistent_settings;
};
