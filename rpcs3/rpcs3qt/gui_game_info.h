#pragma once

#include "movie_item_base.h"
#include "game_compatibility.h"

#include "Emu/GameInfo.h"

#include <QPixmap>

/* Having the icons associated with the game info simplifies logic internally */
struct gui_game_info
{
	GameInfo info{};
	QString localized_category;
	compat::status compat;
	QPixmap icon;
	QPixmap pxmap;
	bool has_custom_config = false;
	bool has_custom_pad_config = false;
	bool has_custom_icon = false;
	bool has_hover_gif = false;
	bool has_hover_pam = false;
	movie_item_base* item = nullptr;

	// Returns the visible version string in the game list
	std::string GetGameVersion() const;
};

typedef std::shared_ptr<gui_game_info> game_info;
Q_DECLARE_METATYPE(game_info)
