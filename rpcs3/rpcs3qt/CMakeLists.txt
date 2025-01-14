function(copy_base_translations target)
  get_target_property(MOC_EXECUTABLE_LOCATION Qt6::moc IMPORTED_LOCATION)
  get_filename_component(QT_BINARY_DIRECTORY "${MOC_EXECUTABLE_LOCATION}" DIRECTORY)
  find_program(LCONVERT_EXE lconvert HINTS "${QT_BINARY_DIRECTORY}")
  set(BASE_TRANSLATIONS_DIR "${QT_BINARY_DIRECTORY}/../translations")

  if(NOT APPLE)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${target}>/translations")
  endif()

  file(GLOB qmFiles "${BASE_TRANSLATIONS_DIR}/qt_*.qm")
  foreach(path IN LISTS qmFiles)
    get_filename_component(file ${path} NAME)

    # qt_help_<lang> just has to ruin everything.
    if(file MATCHES "qt_help_" OR NOT file MATCHES "qt_([^.]+).qm")
      continue()
    endif()

    # If qtbase_<lang>.qm exists, merge all qms for that language into a single qm.
    set(lang "${CMAKE_MATCH_1}")
    set(baseQmPath "${BASE_TRANSLATIONS_DIR}/qtbase_${lang}.qm")
    if(EXISTS "${baseQmPath}")
      set(outPath "${CMAKE_CURRENT_BINARY_DIR}/qt_${lang}.qm")
      set(srcQmFiles)
      file(GLOB langQmFiles "${BASE_TRANSLATIONS_DIR}/qt*${lang}.qm")
      foreach(qmFile IN LISTS langQmFiles)
        get_filename_component(file ${qmFile} NAME)
        if(file STREQUAL "qt_${lang}.qm")
          continue()
        endif()
        LIST(APPEND srcQmFiles "${qmFile}")
      endforeach()
      add_custom_command(OUTPUT ${outPath}
        COMMAND "${LCONVERT_EXE}" -verbose -of qm -o "${outPath}" ${srcQmFiles}
        DEPENDS ${srcQmFiles}
      )
      set(path "${outPath}")
    endif()

    target_sources(${target} PRIVATE ${path})
    if(APPLE)
      set_source_files_properties(${path} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/translations)
    elseif(ALLOW_INSTALL)
      install(FILES "${path}" DESTINATION "${CMAKE_INSTALL_BINDIR}/translations")
    else()
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${path}" "$<TARGET_FILE_DIR:${target}>/translations")
    endif()
  endforeach()
endfunction()


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
    debugger_frame.cpp
    debugger_list.cpp
    downloader.cpp
    dimensions_dialog.cpp
    _discord_utils.cpp
    emu_settings.cpp
    elf_memory_dumping_dialog.cpp
    emulated_pad_settings_dialog.cpp
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
    osk_dialog_frame.cpp
    pad_led_settings_dialog.cpp
    pad_motion_settings_dialog.cpp
    pad_settings_dialog.cpp
    patch_creator_dialog.cpp
    patch_manager_dialog.cpp
    persistent_settings.cpp
    pkg_install_dialog.cpp
    progress_dialog.cpp
    progress_indicator.cpp
    ps_move_tracker_dialog.cpp
    qt_camera_handler.cpp
    qt_camera_video_sink.cpp
    qt_music_handler.cpp
    qt_utils.cpp
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
    welcome_dialog.cpp

    about_dialog.ui
    camera_settings_dialog.ui
    main_window.ui
    pad_led_settings_dialog.ui
    pad_motion_settings_dialog.ui
    pad_settings_dialog.ui
    patch_creator_dialog.ui
    patch_manager_dialog.ui
    ps_move_tracker_dialog.ui
    settings_dialog.ui
    shortcut_dialog.ui
    welcome_dialog.ui

    "../resources.qrc"
)

set(TS_FILES
  translations/rpcs3_sv.ts
  translations/rpcs3_pl.ts
)

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
        3rdparty::qt6
        3rdparty::yaml-cpp

    PRIVATE
        rpcs3_emu
        3rdparty::zlib
        3rdparty::pugixml
        3rdparty::discordRPC
        3rdparty::hidapi
        3rdparty::libusb
        3rdparty::libpng
        3rdparty::7zip
        3rdparty::wolfssl
        3rdparty::libcurl
        3rdparty::opencv
        3rdparty::fusion
        3rdparty::rtmidi)

# Translation setup.
qt_add_lrelease(rpcs3 TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
if(NOT APPLE)
  set(QM_OUTPUT_DIR "$<TARGET_FILE_DIR:rpcs3>/translations")
  add_custom_command(TARGET rpcs3 POST_BUILD COMMAND "${CMAKE_COMMAND}" -E make_directory "${QM_OUTPUT_DIR}")
  foreach (QM_FILE IN LISTS QM_FILES)
    get_filename_component(QM_FILE_NAME ${QM_FILE} NAME)
    add_custom_command(TARGET rpcs3 POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${QM_FILE}" "${QM_OUTPUT_DIR}/${QM_FILE_NAME}")
    if(ALLOW_INSTALL)
      install(FILES "${QM_FILE}" DESTINATION "${CMAKE_INSTALL_BINDIR}/translations")
    endif()
  endforeach()
else()
  foreach (QM_FILE IN LISTS QM_FILES)
    target_sources(rpcs3 PRIVATE ${QM_FILE})
    set_source_files_properties(${QM_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/translations)
  endforeach()
endif()
copy_base_translations(rpcs3)

