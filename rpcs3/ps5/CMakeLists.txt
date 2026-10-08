# PS5: RPCS3's frontend on the console (ps5_frontend.h), linked by the title.
# With it, what the desktop builds into its Qt side and the emulator needs
# without Qt: the pad thread and its tables, the PS Move configuration and
# tracker (without OpenCV), and the version strings.
add_library(rpcs3_ps5 STATIC
    ps5_frontend.cpp
    ps5_gs_frame.cpp
    ps5_pad_handler.cpp
    ps5_firmware.cpp
    ps5_terminate.cpp
    ps5_audio_backend.cpp
    ps5_launcher.cpp
    ps5_ui.cpp
    ps5_pause_menu.cpp
    ps5_patches.cpp
    ps5_fdtrack.cpp
    ps5_sound.cpp
    ../rpcs3qt/emu_settings_type.cpp
    ../Input/pad_thread.cpp
    ../Input/product_info.cpp
    ../Input/ps_move_config.cpp
    ../Input/ps_move_tracker.cpp
    ../rpcs3_version.cpp
)
# RPCS3's overlay images (bin/Icons/ui), built in for the frontend to write to
# /app0/rpcs3/Icons/ui/ (embed_files.py)
file(GLOB_RECURSE ps5_icon_files CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/bin/Icons/ui/*.png)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ps5_icons.cpp
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/embed_files.py ${CMAKE_CURRENT_BINARY_DIR}/ps5_icons.cpp ${CMAKE_SOURCE_DIR}/bin/Icons/ui ${ps5_icon_files}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embed_files.py ${ps5_icon_files}
    VERBATIM)
target_sources(rpcs3_ps5 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/ps5_icons.cpp)

# RPCS3's English strings for its native dialogs, read from the desktop's
# rpcs3qt/localized_emu.h without Qt (localized_strings.py)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ps5_localized.cpp
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/localized_strings.py ${CMAKE_CURRENT_SOURCE_DIR}/../rpcs3qt/localized_emu.h ${CMAKE_CURRENT_BINARY_DIR}/ps5_localized.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/localized_strings.py ${CMAKE_CURRENT_SOURCE_DIR}/../rpcs3qt/localized_emu.h
    VERBATIM)
target_sources(rpcs3_ps5 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/ps5_localized.cpp)
target_include_directories(rpcs3_ps5 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(rpcs3_ps5 PUBLIC rpcs3_emu 3rdparty::fusion 3rdparty::zlib 3rdparty::yaml-cpp) # Fusion: the pad thread's motion filter; zlib: the SCE decrypter (firmware); yaml-cpp: the patch engine's header (ps5_patches.h)
target_compile_options(rpcs3_ps5 PRIVATE "SHELL:-include ${PS5_VOLK_DIR}/volk.h")
# std::terminate's report rethrows the exception for its message: the one unit
# built with exceptions (ps5_terminate.cpp)
set_source_files_properties(ps5_terminate.cpp PROPERTIES COMPILE_OPTIONS "-fexceptions")
