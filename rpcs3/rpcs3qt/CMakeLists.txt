set(SRC_FILES
	about_dialog.cpp
	auto_pause_settings_dialog.cpp
	breakpoint_handler.cpp
	breakpoint_list.cpp
	cg_disasm_window.cpp
	cheat_manager.cpp
	custom_dialog.cpp
	debugger_frame.cpp
	debugger_list.cpp
	_discord_utils.cpp
	emu_settings.cpp
	find_dialog.cpp
	game_compatibility.cpp
	game_list_frame.cpp
	game_list_grid.cpp
	game_list_grid_delegate.cpp
	gui_application.cpp
	gl_gs_frame.cpp
	gs_frame.cpp
	gui_settings.cpp
	input_dialog.cpp
	instruction_editor_dialog.cpp
	kernel_explorer.cpp
	localized.cpp
	log_frame.cpp
	main_window.cpp
	memory_string_searcher.cpp
	memory_viewer_panel.cpp
	msg_dialog_frame.cpp
	osk_dialog_frame.cpp
	pad_led_settings_dialog.cpp
	pad_settings_dialog.cpp
	persistent_settings.cpp
	pkg_install_dialog.cpp
	progress_dialog.cpp
	qt_utils.cpp
	register_editor_dialog.cpp
	rsx_debugger.cpp
	save_data_dialog.cpp
	save_data_info_dialog.cpp
	save_data_list_dialog.cpp
	save_manager_dialog.cpp
	settings.cpp
	settings_dialog.cpp
	skylander_dialog.cpp
	syntax_highlighter.cpp
	tooltips.cpp
	trophy_manager_dialog.cpp
	trophy_notification_frame.cpp
	trophy_notification_helper.cpp
	update_manager.cpp
	user_account.cpp
	user_manager_dialog.cpp
	vfs_dialog.cpp
	vfs_dialog_tab.cpp
	welcome_dialog.cpp
)

set(UI_FILES
	about_dialog.ui
	main_window.ui
	pad_led_settings_dialog.ui
	pad_settings_dialog.ui
	settings_dialog.ui
	welcome_dialog.ui
)

set(RES_FILES "../resources.qrc")

if(WIN32)
	list(APPEND RES_FILES "../windows.qrc")
endif()

add_library(rpcs3_ui ${SRC_FILES} ${UI_FILES} ${RES_FILES})

set_target_properties(rpcs3_ui
	PROPERTIES
		AUTOMOC ON
		AUTOUIC ON
		AUTORCC ON)

target_link_libraries(rpcs3_ui
	PUBLIC
		3rdparty::qt5 3rdparty::yaml-cpp

	PRIVATE
		rpcs3_emu
		3rdparty::zlib 3rdparty::pugixml
		3rdparty::discord-rpc
		3rdparty::hidapi
		3rdparty::libusb
		3rdparty::libpng
		3rdparty::7z
		3rdparty::wolfssl
		3rdparty::libcurl)
