#include "shortcut_settings.h"
#include "gui_settings.h"

using namespace gui::shortcuts;

template <>
void fmt_class_string<shortcut>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](shortcut value)
	{
		switch (value)
		{
		case shortcut::mw_start: return "mw_start";
		case shortcut::mw_stop: return "mw_stop";
		case shortcut::mw_pause: return "mw_pause";
		case shortcut::mw_restart: return "mw_restart";
		case shortcut::mw_toggle_fullscreen: return "mw_toggle_fullscreen";
		case shortcut::mw_exit_fullscreen: return "mw_exit_fullscreen";
		case shortcut::mw_refresh: return "mw_refresh";
		case shortcut::gw_toggle_fullscreen: return "gw_toggle_fullscreen";
		case shortcut::gw_exit_fullscreen: return "gw_exit_fullscreen";
		case shortcut::gw_log_mark: return "gw_log_mark";
		case shortcut::gw_mouse_lock: return "gw_mouse_lock";
		case shortcut::gw_screenshot: return "gw_screenshot";
		case shortcut::gw_toggle_recording: return "gw_toggle_recording";
		case shortcut::gw_pause_play: return "gw_pause_play";
		case shortcut::gw_savestate: return "gw_savestate";
		case shortcut::gw_savestate_1: return "gw_savestate1";
		case shortcut::gw_savestate_2: return "gw_savestate2";
		case shortcut::gw_savestate_3: return "gw_savestate3";
		case shortcut::gw_savestate_4: return "gw_savestate4";
		case shortcut::gw_restart: return "gw_restart";
		case shortcut::gw_rsx_capture: return "gw_rsx_capture";
		case shortcut::gw_frame_limit: return "gw_frame_limit";
		case shortcut::gw_toggle_mouse_and_keyboard: return "gw_toggle_mouse_and_keyboard";
		case shortcut::gw_home_menu: return "gw_home_menu";
		case shortcut::gw_mute_unmute: return "gw_mute_unmute";
		case shortcut::gw_volume_up: return "gw_volume_up";
		case shortcut::gw_volume_down: return "gw_volume_down";
		case shortcut::count: return "count";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<shortcut_handler_id>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](gui::shortcuts::shortcut_handler_id value)
	{
		switch (value)
		{
			case shortcut_handler_id::main_window: return "main_window";
			case shortcut_handler_id::game_window: return "game_window";
		}

		return unknown;
	});
}

shortcut_settings::shortcut_settings()
	: shortcut_map({
		{ shortcut::mw_start, shortcut_info{ "main_window_start", tr("Start"), "Ctrl+E", shortcut_handler_id::main_window, false } },
		{ shortcut::mw_stop, shortcut_info{ "main_window_stop", tr("Stop"), "Ctrl+S", shortcut_handler_id::main_window, false } },
		{ shortcut::mw_pause, shortcut_info{ "main_window_pause", tr("Pause"), "Ctrl+P", shortcut_handler_id::main_window, false } },
		{ shortcut::mw_restart, shortcut_info{ "main_window_restart", tr("Restart"), "Ctrl+R", shortcut_handler_id::main_window, false } },
		{ shortcut::mw_toggle_fullscreen, shortcut_info{ "main_window_toggle_fullscreen", tr("Toggle Fullscreen"), "Alt+Return", shortcut_handler_id::main_window, false } },
		{ shortcut::mw_exit_fullscreen, shortcut_info{ "main_window_exit_fullscreen", tr("Exit Fullscreen"), "Esc", shortcut_handler_id::main_window, false } },
		{ shortcut::mw_refresh, shortcut_info{ "main_window_refresh", tr("Refresh"), "Ctrl+F5", shortcut_handler_id::main_window, false } },
		{ shortcut::gw_toggle_fullscreen, shortcut_info{ "game_window_toggle_fullscreen", tr("Toggle Fullscreen"), "Alt+Return", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_exit_fullscreen, shortcut_info{ "game_window_exit_fullscreen", tr("Exit Fullscreen"), "Esc", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_log_mark, shortcut_info{ "game_window_log_mark", tr("Add Log Mark"), "Alt+L", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_mouse_lock, shortcut_info{ "game_window_mouse_lock", tr("Mouse lock"), "Ctrl+L", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_toggle_recording, shortcut_info{ "game_window_toggle_recording", tr("Start/Stop Recording"), "F11", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_screenshot, shortcut_info{ "game_window_screenshot", tr("Screenshot"), "F12", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_pause_play, shortcut_info{ "game_window_pause_play", tr("Pause/Play"), "Ctrl+P", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_savestate, shortcut_info{ "game_window_savestate", tr("Savestate"), "Ctrl+S", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_savestate_1, shortcut_info{ "game_window_savestate_1", tr("Savestate"), "Alt+Ctrl+1", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_savestate_2, shortcut_info{ "game_window_savestate_2", tr("Savestate"), "Alt+Ctrl+2", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_savestate_3, shortcut_info{ "game_window_savestate_3", tr("Savestate"), "Alt+Ctrl+3", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_savestate_4, shortcut_info{ "game_window_savestate_4", tr("Savestate"), "Alt+Ctrl+4", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_restart, shortcut_info{ "game_window_restart", tr("Restart"), "Ctrl+R", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_rsx_capture, shortcut_info{ "game_window_rsx_capture", tr("RSX Capture"), "Alt+C", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_frame_limit, shortcut_info{ "game_window_frame_limit", tr("Toggle Framelimit"), "Ctrl+F10", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_toggle_mouse_and_keyboard, shortcut_info{ "game_window_toggle_mouse_and_keyboard", tr("Toggle Keyboard"), "Ctrl+F11", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_home_menu, shortcut_info{ "gw_home_menu", tr("Open Home Menu"), "Shift+F10", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_mute_unmute, shortcut_info{ "gw_mute_unmute", tr("Mute/Unmute Audio"), "Ctrl+Shift+M", shortcut_handler_id::game_window, false } },
		{ shortcut::gw_volume_up, shortcut_info{ "gw_volume_up", tr("Volume Up"), "Ctrl+Shift++", shortcut_handler_id::game_window, true } },
		{ shortcut::gw_volume_down, shortcut_info{ "gw_volume_down", tr("Volume Down"), "Ctrl+Shift+-", shortcut_handler_id::game_window, true } },
	})
{
}

shortcut_settings::~shortcut_settings()
{
}

gui_save shortcut_settings::get_shortcut_gui_save(const QString& shortcut_name)
{
	const auto it = std::find_if(shortcut_map.begin(), shortcut_map.end(), [&](const auto& entry) { return entry.second.name == shortcut_name; });

	if (it != shortcut_map.cend())
	{
		return gui_save(gui::sc, it->second.name, it->second.key_sequence);
	}

	return gui_save();
}

QKeySequence shortcut_settings::get_key_sequence(const shortcut_info& entry, const std::shared_ptr<gui_settings>& gui_settings)
{
	if (!gui_settings)
		return {};

	const QString saved_value = gui_settings->GetValue(get_shortcut_gui_save(entry.name)).toString();

	return QKeySequence::fromString(saved_value);
}
