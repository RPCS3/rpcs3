#pragma once

#include <QString>

namespace gui
{
	namespace stylesheets
	{
		const QString default_style_sheet
		(
			// main window toolbar search
			"QLineEdit#mw_searchbar { padding: 0 1em; background: #fdfdfd; selection-background-color: #148aff; margin: .8em; color:#000000; }"

			// main window toolbar slider
			"QSlider#sizeSlider { color: #505050; background: #F0F0F0; }"
			"QSlider#sizeSlider::handle:horizontal { border: 0em smooth rgba(227, 227, 227, 255); border-radius: .58em; background: #404040; width: 1.2em; margin: -.5em 0; }"
			"QSlider#sizeSlider::groove:horizontal { border-radius: .15em; background: #5b5b5b; height: .3em; }"

			// main window toolbar
			"QToolBar#mw_toolbar { background-color: #F0F0F0; border: none; }"
			"QToolBar#mw_toolbar::separator { background-color: rgba(207, 207, 207, 235); width: 0.125em; margin-top: 0.250em; margin-bottom: 0.250em; }"
			"QToolButton:disabled { color: #787878; }"

			// main window toolbar icon color
			"QLabel#toolbar_icon_color { color: #5b5b5b; }"

			// thumbnail icon color
			"QLabel#thumbnail_icon_color { color: rgba(0, 100, 231, 255); }"

			// game list icon color
			"QLabel#gamelist_icon_background_color { color: rgba(240, 240, 240, 255); }"

			// game grid
			"#game_list_grid #flow_widget_content { background: #fff; }"
			"#game_list_grid_item { background: #fff; }"
			"#game_list_grid_item[selected=\"true\"] { background: #148aff; }"
			"#game_list_grid_item:focus { background: #148aff; }"
			"#game_list_grid_item:hover { background: #94c9ff; }"
			"#game_list_grid_item:hover:focus { background: #007fff; }"
			"#game_list_grid_item #game_list_grid_item_title_label { color: rgba(51, 51, 51, 255); font-weight: 600; font-size: 8pt; font-family: Lucida Grande; border: 0em solid white; }"

			// game grid hover and focus: we need to handle properties differently when using descendants
			"#game_list_grid_item[selected=\"true\"] #game_list_grid_item_title_label { color: #fff; }"
			"#game_list_grid_item[hover=\"true\"] #game_list_grid_item_title_label { color: #fff; }"
			"#game_list_grid_item[focus=\"true\"] #game_list_grid_item_title_label { color: #fff; }"

			// save manager icon color
			"QLabel#save_manager_icon_background_color { color: rgba(240, 240, 240, 255); }"

			// trophy manager icon color
			"QLabel#trophy_manager_icon_background_color { color: rgba(240, 240, 240, 255); }"

			// tables
			"QTableWidget { alternate-background-color: #f2f2f2; background-color: #fff; border: none; }"
			"QTableView::item { border-left: 0.063em solid white; border-right: 0.063em solid white; padding-left:0.313em; }"
			"QTableView::item:selected { background-color: #148aff; color: #fff; }"

			// table headers
			"QHeaderView::section { padding-left: .5em; padding-right: .5em; padding-top: .4em; padding-bottom: -.1em; border: 0.063em solid #ffffff; }"
			"QHeaderView::section:hover { background: #e3e3e3; padding-left: .5em; padding-right: .5em; padding-top: .4em; padding-bottom: -.1em; border: 0.063em solid #ffffff; }"

			// dock widget
			"QDockWidget{ background: transparent; color: black; }"
			"[floating=\"true\"]{ background: white; }"
			"QDockWidget::title{ background: #e3e3e3; border: none; padding-top: 0.2em; padding-left: 0.2em; }"
			"QDockWidget::close-button, QDockWidget::float-button{ background-color: #e3e3e3; }"

			// log frame tty
			"QPlainTextEdit#tty_frame { background-color: #ffffff; }"
			"QLabel#tty_text { color: #000000; }"

			// log frame log
			"QPlainTextEdit#log_frame { background-color: #ffffff; }"
			"QLabel#log_level_always { color: #107896; }"
			"QLabel#log_level_fatal { color: #ff00ff; }"
			"QLabel#log_level_error { color: #C02F1D; }"
			"QLabel#log_level_todo { color: #ff6000; }"
			"QLabel#log_level_success { color: #008000; }"
			"QLabel#log_level_warning { color: #BA8745; }"
			"QLabel#log_level_notice { color: #000000; }"
			"QLabel#log_level_trace { color: #808080; }"
			"QLabel#log_stack { color: #000000; }"

			// about dialog
			"QWidget#header_section { background-color: #ffffff; }"

			// kernel explorer
			"QDialog#kernel_explorer { background-color: rgba(240, 240, 240, 255); }"

			// memory viewer
			"QDialog#memory_viewer { background-color: rgba(240, 240, 240, 255); }"
			"QLabel#memory_viewer_address_panel { color: rgba(75, 135, 150, 255); background-color: rgba(240, 240, 240, 255); }"
			"QLabel#memory_viewer_hex_panel { color: #000000; background-color: rgba(240, 240, 240, 255); }"
			"QLabel#memory_viewer_ascii_panel { color: #000000; background-color: rgba(240, 240, 240, 255); }"

			// debugger frame
			"QLabel#debugger_frame_breakpoint { color: #000000; background-color: #ffff00; }"
			"QLabel#debugger_frame_pc { color: #000000; background-color: #00ff00; }"

			// rsx debugger
			"QLabel#rsx_debugger_display_buffer { background-color: rgba(240, 240, 240, 255); }"

			// pad settings
			"QLabel#l_controller { color: #434343; }"

			// Top menu bar (Workaround for transparent menus in Qt 6.7.3)
			"QMenu { color: #000; background-color: #F0F0F0; alternate-background-color: #f2f2f2; }"
			"QMenu::item:selected { background: #90C8F6; }"
			"QMenu::item:disabled { color: #787878; }"
		);
	}
}
