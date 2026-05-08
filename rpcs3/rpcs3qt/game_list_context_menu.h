#pragma once

#include "gui_game_info.h"

#include "QMenu"

class game_list_actions;
class game_list_frame;
class gui_settings;
class emu_settings;
class persistent_settings;

class game_list_context_menu : QMenu
{
	Q_OBJECT

public:
	game_list_context_menu(game_list_frame* frame);
	virtual ~game_list_context_menu();

	void show_menu(const std::vector<game_info>& games, const QPoint& global_pos);

private:
	void show_single_selection_context_menu(const game_info& gameinfo, const QPoint& global_pos);
	void show_multi_selection_context_menu(const std::vector<game_info>& games, const QPoint& global_pos);

	game_list_frame* m_game_list_frame = nullptr;
	std::shared_ptr<game_list_actions> m_game_list_actions;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;
};
