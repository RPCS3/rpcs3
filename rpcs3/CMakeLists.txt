
if(USE_PRECOMPILED_HEADERS AND NOT COMMAND target_precompile_headers)
    include(cotire)
endif()

# Generate git-version.h at build time.
include(${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)

# Check for a sufficient compiler and set build options
include(ConfigureCompiler)
include(CheckFunctionExists)

set(CMAKE_CXX_STANDARD 20)

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
    # TODO(cjj19970505@live.cn)
    # this is a single-config generator workaround
    # multi-config generator will fail.

    # TODO(cjj19970505@live.cn)
    # offical Qt binaries are built with -MD(d) only as stated in offical wiki
    # https://wiki.qt.io/Technical_FAQ#Why_does_a_statically_built_Qt_use_the_dynamic_Visual_Studio_runtime_libraries_.3F_Do_I_need_to_deploy_those_with_my_application_.3F
    # If we build our libs with /MT(d), we might encounter some issues.
    # https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-160#what-problems-exist-if-an-application-uses-more-than-one-crt-version

    # Qt installed from Qt installer has following hierarchy:
    # bin/ for release and debug dlls and windeployqt tools
    # lib/cmake/Qt5/ for Qt5_Dir
    # Qt installed from vcpkg has following hierarchy:
    # bin/ for release dlls
    # debug/bin/ for debug dlls
    # tools/qt5/bin/ for tools including windeployqt
    # tools/qt5/debug/bin/ for tools with debug build including windeployqt
    # share/cmake/Qt5/ for Qt5_Dir

    # If Qt5 is installed from official Qt installer
    list(APPEND _QT5_TOOLS_PATHS "${Qt5_DIR}/../../../bin/")

    # If Qt5 is installed from vcpkg
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND _QT5_TOOLS_PATHS "${Qt5_DIR}/../../../tools/qt5/debug/bin/")
    else()
        list(APPEND _QT5_TOOLS_PATHS "${Qt5_DIR}/../../../tools/qt5/bin/")
    endif()
    
    find_program(_WINDEPLOYQT 
        NAMES windeployqt
        PATHS ${_QT5_TOOLS_PATHS}
        REQUIRED)
    message("Use windeployqt in ${_WINDEPLOYQT}")
    
    add_custom_command(TARGET rpcs3 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/bin" "$<TARGET_FILE_DIR:rpcs3>"
        # If Qt5 is installed from vcpkg, add binary path to PATH
        # otherwise windeployqt tool won't be able to locate necessary dlls
        COMMAND set PATH=${Qt5_DIR}/../../../$<$<CONFIG:Debug>:debug/>bin/$<SEMICOLON>%PATH% 
        COMMAND "${_WINDEPLOYQT}" --no-angle --no-compiler-runtime --no-opengl-sw --no-patchqt --no-svg --no-translations --no-quick --plugindir "$<TARGET_FILE_DIR:rpcs3>/qt/plugins" $<$<CONFIG:Debug>:--debug> $<$<CONFIG:Release>:--release> "$<TARGET_FILE:rpcs3>")
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
    install(FILES rpcs3.metainfo.xml
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo)
    # Install other files
    install(DIRECTORY ../bin/Icons
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/rpcs3)
    install(DIRECTORY ../bin/GuiConfigs
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/rpcs3)
    install(DIRECTORY ../bin/git
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/rpcs3)
endif()
