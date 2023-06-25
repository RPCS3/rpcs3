#pragma once

#include "gui_settings.h"

#include <QKeySequence>

namespace gui
{
	namespace shortcuts
	{
		enum class shortcut_handler_id : int
		{
			main_window,
			game_window,
		};

		enum class shortcut : int
		{
			mw_start,
			mw_stop,
			mw_pause,
			mw_restart,
			mw_toggle_fullscreen,
			mw_exit_fullscreen,
			mw_refresh,
			mw_welcome_dialog,

			gw_toggle_fullscreen,
			gw_exit_fullscreen,
			gw_log_mark,
			gw_mouse_lock,
			gw_screenshot,
			gw_toggle_recording,
			gw_pause_play,
			gw_savestate,
			gw_restart,
			gw_rsx_capture,
			gw_frame_limit,

			count
		};
	}
}

struct shortcut_info
{
	QString name;
	QString localized_name;
	QString key_sequence;
	gui::shortcuts::shortcut_handler_id handler_id{};
};

class shortcut_settings : public QObject
{
	Q_OBJECT

public:
	shortcut_settings();
	~shortcut_settings();

	const std::map<const gui::shortcuts::shortcut, const shortcut_info> shortcut_map;

	gui_save get_shortcut_gui_save(const QString& shortcut_name);

	QKeySequence get_key_sequence(const shortcut_info& entry, const std::shared_ptr<gui_settings>& gui_settings);
};
