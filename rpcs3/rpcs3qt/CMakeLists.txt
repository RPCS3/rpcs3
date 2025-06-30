add_library(rpcs3_ui STATIC
    about_dialog.cpp
    auto_pause_settings_dialog.cpp
    basic_mouse_settings_dialog.cpp
    breakpoint_handler.cpp
    breakpoint_list.cpp
    call_stack_list.cpp
    camera_settings_dialog.cpp
    cg_disasm_window.cpp
    cheat_manager.cpp
    config_adapter.cpp
    config_checker.cpp
    curl_handle.cpp
    custom_dialog.cpp
    custom_table_widget_item.cpp
    debugger_add_bp_window.cpp
    debugger_frame.cpp
    debugger_list.cpp
    downloader.cpp
    dimensions_dialog.cpp
    _discord_utils.cpp
    emu_settings.cpp
    elf_memory_dumping_dialog.cpp
    emulated_pad_settings_dialog.cpp
    emulated_logitech_g27_settings_dialog.cpp
    fatal_error_dialog.cpp
    find_dialog.cpp
    flow_layout.cpp
    flow_widget.cpp
    flow_widget_item.cpp
    game_compatibility.cpp
    game_list.cpp
    game_list_base.cpp
    game_list_delegate.cpp
    game_list_frame.cpp
    game_list_grid.cpp
    game_list_grid_item.cpp
    game_list_table.cpp
    gui_application.cpp
    gl_gs_frame.cpp
    gs_frame.cpp
    gui_game_info.cpp
    gui_settings.cpp
    infinity_dialog.cpp
    input_dialog.cpp
    instruction_editor_dialog.cpp
    ipc_settings_dialog.cpp
    kernel_explorer.cpp
    localized.cpp
    localized_emu.cpp
    log_frame.cpp
    log_viewer.cpp
    main_window.cpp
    memory_string_searcher.cpp
    memory_viewer_panel.cpp
    microphone_creator.cpp
    midi_creator.cpp
    movie_item.cpp
    movie_item_base.cpp
    msg_dialog_frame.cpp
    music_player_dialog.cpp
    osk_dialog_frame.cpp
    pad_led_settings_dialog.cpp
    pad_motion_settings_dialog.cpp
    pad_settings_dialog.cpp
    patch_creator_dialog.cpp
    patch_manager_dialog.cpp
    permissions.cpp
    persistent_settings.cpp
    pkg_install_dialog.cpp
    progress_dialog.cpp
    progress_indicator.cpp
    ps_move_tracker_dialog.cpp
    qt_camera_handler.cpp
    qt_camera_video_sink.cpp
    qt_music_handler.cpp
    qt_utils.cpp
    qt_video_source.cpp
    raw_mouse_settings_dialog.cpp
    register_editor_dialog.cpp
    recvmessage_dialog_frame.cpp
    render_creator.cpp
    rpcn_settings_dialog.cpp
    rsx_debugger.cpp
    save_data_dialog.cpp
    save_data_info_dialog.cpp
    save_data_list_dialog.cpp
    save_manager_dialog.cpp
    savestate_manager_dialog.cpp
    screenshot_item.cpp
    screenshot_manager_dialog.cpp
    screenshot_preview.cpp
    sendmessage_dialog_frame.cpp
    settings.cpp
    settings_dialog.cpp
    shortcut_utils.cpp
    shortcut_dialog.cpp
    shortcut_handler.cpp
    shortcut_settings.cpp
    skylander_dialog.cpp
    syntax_highlighter.cpp
    system_cmd_dialog.cpp
    table_item_delegate.cpp
    tooltips.cpp
    trophy_manager_dialog.cpp
    trophy_notification_frame.cpp
    trophy_notification_helper.cpp
    update_manager.cpp
    user_account.cpp
    user_manager_dialog.cpp
    uuid.cpp
    vfs_dialog.cpp
    vfs_dialog_path_widget.cpp
    vfs_dialog_tab.cpp
    vfs_dialog_usb_input.cpp
    vfs_dialog_usb_tab.cpp
    vfs_tool_dialog.cpp
    video_label.cpp
    welcome_dialog.cpp

    about_dialog.ui
    camera_settings_dialog.ui
    main_window.ui
    music_player_dialog.ui
    pad_led_settings_dialog.ui
    pad_motion_settings_dialog.ui
    pad_settings_dialog.ui
    patch_creator_dialog.ui
    patch_manager_dialog.ui
    ps_move_tracker_dialog.ui
    settings_dialog.ui
    shortcut_dialog.ui
    welcome_dialog.ui

    ../display_sleep_control.cpp
    ../gamemode_control.cpp
    ../headless_application.cpp
    ../main_application.cpp
    ../module_verifier.cpp
    ../rpcs3.cpp
    ../rpcs3_version.cpp
    ../stdafx.cpp

    ../Input/basic_keyboard_handler.cpp
    ../Input/basic_mouse_handler.cpp
    ../Input/ds3_pad_handler.cpp
    ../Input/ds4_pad_handler.cpp
    ../Input/dualsense_pad_handler.cpp
    ../Input/evdev_joystick_handler.cpp
    ../Input/gui_pad_thread.cpp
    ../Input/hid_pad_handler.cpp
    ../Input/keyboard_pad_handler.cpp
    ../Input/mm_joystick_handler.cpp
    ../Input/pad_thread.cpp
    ../Input/product_info.cpp
    ../Input/ps_move_calibration.cpp
    ../Input/ps_move_config.cpp
    ../Input/ps_move_handler.cpp
    ../Input/ps_move_tracker.cpp
    ../Input/raw_mouse_config.cpp
    ../Input/raw_mouse_handler.cpp
    ../Input/sdl_pad_handler.cpp
    ../Input/sdl_instance.cpp
    ../Input/skateboard_pad_handler.cpp
    ../Input/xinput_pad_handler.cpp

    "../resources.qrc"
)

if(HAS_MEMORY_BREAKPOINTS)
    target_compile_definitions(rpcs3_ui PRIVATE RPCS3_HAS_MEMORY_BREAKPOINTS)
endif()

if(WIN32)
    target_sources(rpcs3_ui PUBLIC "../windows.qrc")
    target_compile_definitions(rpcs3_ui PRIVATE UNICODE _UNICODE)
endif()

set_target_properties(rpcs3_ui
    PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON)

# AUTOMOC brings Windows.h to the sources, which have some definitions conflicting with winsock2.h
# define WIN32_LEAN_AND_MEAN resolve the problem
# https://docs.microsoft.com/en-us/windows/win32/winsock/creating-a-basic-winsock-application
# https://docs.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers#faster-builds-with-smaller-header-files
target_compile_definitions(rpcs3_ui PRIVATE WIN32_LEAN_AND_MEAN)

target_link_libraries(rpcs3_ui
    PUBLIC
        rpcs3_emu
        3rdparty::qt6
        3rdparty::yaml-cpp
        3rdparty::zlib
        3rdparty::discordRPC
        3rdparty::hidapi
        3rdparty::libusb
        3rdparty::wolfssl
        3rdparty::libcurl
        3rdparty::opencv
        3rdparty::fusion

    PRIVATE
        3rdparty::pugixml
        3rdparty::libpng
        3rdparty::7zip
        3rdparty::rtmidi)
