#pragma once

#include "game_list_base.h"
#include "flow_widget.h"

#include <QKeyEvent>

class game_list_grid : public flow_widget, public game_list_base
{
	Q_OBJECT

public:
	explicit game_list_grid();

	void clear_list() override;

	void populate(
		const std::vector<game_info>& game_data,
		const std::map<QString, QString>& notes_map,
		const std::map<QString, QString>& title_map,
		const std::string& selected_item_id,
		bool play_hover_movies) override;

	void repaint_icons(std::vector<game_info>& game_data, const QColor& icon_color, const QSize& icon_size, qreal device_pixel_ratio) override;

	bool eventFilter(QObject* watched, QEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

public Q_SLOTS:
	void FocusAndSelectFirstEntryIfNoneIs();

Q_SIGNALS:
	void FocusToSearchBar();
	void ItemDoubleClicked(const game_info& game);
	void ItemSelectionChanged(const game_info& game);
	void IconReady(const movie_item_base* item);
};
