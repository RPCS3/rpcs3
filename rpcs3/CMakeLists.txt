
if(USE_PRECOMPILED_HEADERS AND NOT COMMAND target_precompile_headers)
    include(cotire)
endif()

# Generate git-version.h at build time.
include(${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)

# Check for a sufficient compiler and set build options
include(ConfigureCompiler)
include(CheckFunctionExists)

set(ADDITIONAL_LIBS "")
if(CMAKE_SYSTEM MATCHES "Linux")
    #on some Linux distros shm_unlink and similar functions are in librt only
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "rt")
elseif(NOT MSVC AND NOT CMAKE_CXX_FLAGS MATCHES "LIBICONV_PLUG")
    #it seems like glibc includes the iconv functions we use but other libc
    #implementations like the one on OSX don't seem implement them
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "iconv")
endif()

if(UNIX AND NOT APPLE)
    # Optionally enable X11 for window management
    find_package(X11)
    if(X11_FOUND)
        add_definitions(-DHAVE_X11)
    endif()
endif()

if(NOT RPCS3_SRC_DIR)
    set(RPCS3_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
    message("-- Initializing RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
else()
    message("-- Using Custom RPCS3_SRC_DIR=${RPCS3_SRC_DIR}")
endif()

# Qt5
# finds Qt libraries and setups custom commands for MOC and UIC
# Must be done here because generated MOC and UIC targets cant
# be found otherwise
include(${CMAKE_SOURCE_DIR}/3rdparty/qt5.cmake)

# subdirectories
add_subdirectory(Emu)
add_subdirectory(rpcs3qt)

set(RPCS3_SRC
    display_sleep_control.cpp
    headless_application.cpp
    main.cpp
    main_application.cpp
    rpcs3_version.cpp
    stb_image.cpp
    stdafx.cpp

    Input/basic_keyboard_handler.cpp
    Input/basic_mouse_handler.cpp
    Input/ds3_pad_handler.cpp
    Input/ds4_pad_handler.cpp
    Input/dualsense_pad_handler.cpp
    Input/evdev_joystick_handler.cpp
    Input/hid_pad_handler.cpp
    Input/keyboard_pad_handler.cpp
    Input/mm_joystick_handler.cpp
    Input/pad_thread.cpp
    Input/xinput_pad_handler.cpp
)

if(WIN32)
    add_executable(rpcs3 WIN32 ${RPCS3_SRC})
elseif(APPLE)
    add_executable(rpcs3 MACOSX_BUNDLE ${RPCS3_SRC} "${RPCS3_SRC_DIR}/rpcs3.icns")
    set_target_properties(rpcs3
        PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${RPCS3_SRC_DIR}/rpcs3.plist.in")
else()
    add_executable(rpcs3 ${RPCS3_SRC})
endif()

gen_git_version(${RPCS3_SRC_DIR})
set_target_properties(rpcs3
    PROPERTIES
        AUTOMOC ON
        AUTOUIC ON)

target_link_libraries(rpcs3 rpcs3_emu rpcs3_ui)
target_link_libraries(rpcs3 3rdparty::discordRPC 3rdparty::qt5 3rdparty::hidapi 3rdparty::libusb 3rdparty::wolfssl 3rdparty::libcurl)
target_link_libraries(rpcs3 ${ADDITIONAL_LIBS})

# Win resource file
if(WIN32)
    target_sources(rpcs3 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/rpcs3.rc")
endif()

# Unix display manager
if(X11_FOUND)
    target_include_directories(rpcs3 PUBLIC ${X11_INCLUDE_DIR})
    target_link_libraries(rpcs3 ${X11_LIBRARIES})
elseif(USE_VULKAN AND UNIX AND NOT WAYLAND_FOUND AND NOT APPLE)
    # Wayland has been checked in 3rdparty/CMakeLists.txt already.
    message(FATAL_ERROR "RPCS3 requires either X11 or Wayland (or both) for Vulkan.")
endif()

if(UNIX)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    find_package(Threads REQUIRED)
    target_link_libraries(rpcs3 Threads::Threads)
endif()

if(WIN32)
    target_link_libraries(rpcs3 ws2_32.lib Iphlpapi.lib Winmm.lib Psapi.lib gdi32.lib setupapi.lib)
else()
    target_link_libraries(rpcs3 ${CMAKE_DL_LIBS})
endif()

if(USE_PRECOMPILED_HEADERS)
    if(COMMAND target_precompile_headers)
        target_precompile_headers(rpcs3 PRIVATE "${RPCS3_SRC_DIR}/stdafx.h")
    else()
        set_target_properties(rpcs3 PROPERTIES
            COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h"
            COTIRE_ADD_UNITY_BUILD OFF)

        cotire(rpcs3)
    endif()
endif()

# Copy icons to executable directory
if(APPLE)
    add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${RPCS3_SRC_DIR}/rpcs3.icns $<TARGET_FILE_DIR:rpcs3>/../Resources/rpcs3.icns
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/Icons $<TARGET_FILE_DIR:rpcs3>/../Resources/Icons
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/GuiConfigs $<TARGET_FILE_DIR:rpcs3>/../Resources/GuiConfigs
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/git $<TARGET_FILE_DIR:rpcs3>/../Resources/git
            COMMAND "${Qt5_DIR}/../../../bin/macdeployqt" "${PROJECT_BINARY_DIR}/bin/rpcs3.app")
elseif(UNIX)
    add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/Icons $<TARGET_FILE_DIR:rpcs3>/Icons)
    add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/GuiConfigs $<TARGET_FILE_DIR:rpcs3>/GuiConfigs)
    add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/git $<TARGET_FILE_DIR:rpcs3>/git)
elseif(WIN32)
    add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/bin" "$<TARGET_FILE_DIR:rpcs3>"
            COMMAND "${Qt5_DIR}/../../../bin/windeployqt" --no-angle --no-compiler-runtime --no-opengl-sw --no-patchqt --no-svg --no-translations --no-quick --plugindir "$<TARGET_FILE_DIR:rpcs3>/qt/plugins" --release "$<TARGET_FILE:rpcs3>")
endif()

# Unix installation
if(UNIX AND NOT APPLE)
    # Install the binary
    install(TARGETS rpcs3 RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
    # Install the application icon and menu item
    install(FILES rpcs3.svg
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
    install(FILES rpcs3.png
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps)
    install(FILES rpcs3.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
    install(FILES rpcs3.appdata.xml
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo)
    # Install other files
    install(DIRECTORY ../bin/Icons
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/rpcs3)
    install(DIRECTORY ../bin/GuiConfigs
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/rpcs3)
    install(DIRECTORY ../bin/git
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/rpcs3)
endif()
