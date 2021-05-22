# Define GNU standard installation directories
include(GNUInstallDirs)

# Generate git-version.h at build time.
include(${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)

# Check for a sufficient compiler and set build options
include(ConfigureCompiler)
include(CheckFunctionExists)

set(CMAKE_CXX_STANDARD 20)

set(ADDITIONAL_LIBS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    #on some Linux distros shm_unlink and similar functions are in librt only
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "rt")
elseif(NOT WIN32 AND NOT CMAKE_CXX_FLAGS MATCHES "LIBICONV_PLUG")
    #it seems like glibc includes the iconv functions we use but other libc
    #implementations like the one on OSX don't seem implement them
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "iconv")
endif()

if(UNIX AND NOT APPLE)
    add_definitions(-DDATADIR="${CMAKE_INSTALL_FULL_DATADIR}/rpcs3")
    # Optionally enable X11 for window management
    find_package(X11)
    if(X11_FOUND)
        add_definitions(-DHAVE_X11)
    endif()
endif()

# Qt
# finds Qt libraries and setups custom commands for MOC and UIC
# Must be done here because generated MOC and UIC targets cant
# be found otherwise
include(${CMAKE_SOURCE_DIR}/3rdparty/qt6.cmake)

# subdirectories
add_subdirectory(Emu)
add_subdirectory(rpcs3qt)

if(WIN32)
    add_executable(rpcs3 WIN32)
    target_sources(rpcs3 PRIVATE rpcs3.rc)
elseif(APPLE)
    add_executable(rpcs3 MACOSX_BUNDLE)
    target_sources(rpcs3 PRIVATE rpcs3.icns)
    set_target_properties(rpcs3
        PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/rpcs3.plist.in")
else()
    add_executable(rpcs3)
endif()

target_sources(rpcs3
    PRIVATE
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
    Input/evdev_gun_handler.cpp
    Input/hid_pad_handler.cpp
    Input/keyboard_pad_handler.cpp
    Input/mm_joystick_handler.cpp
    Input/pad_thread.cpp
    Input/sdl_pad_handler.cpp
    Input/xinput_pad_handler.cpp
)

gen_git_version(${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(rpcs3
    PROPERTIES
        AUTOMOC ON
        AUTOUIC ON)

target_link_libraries(rpcs3 PRIVATE rpcs3_emu rpcs3_ui)
target_link_libraries(rpcs3 PRIVATE 3rdparty::discordRPC 3rdparty::qt6 3rdparty::hidapi 3rdparty::libusb 3rdparty::wolfssl 3rdparty::libcurl 3rdparty::zlib)
target_link_libraries(rpcs3 PRIVATE ${ADDITIONAL_LIBS})

# Unix display manager
if(X11_FOUND)
    target_include_directories(rpcs3 PUBLIC ${X11_INCLUDE_DIR})
    target_link_libraries(rpcs3 PRIVATE ${X11_LIBRARIES})
elseif(USE_VULKAN AND UNIX AND NOT WAYLAND_FOUND AND NOT APPLE)
    # Wayland has been checked in 3rdparty/CMakeLists.txt already.
    message(FATAL_ERROR "RPCS3 requires either X11 or Wayland (or both) for Vulkan.")
endif()

if(UNIX)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    find_package(Threads REQUIRED)
    target_link_libraries(rpcs3 PRIVATE Threads::Threads)
endif()

if(WIN32)
    target_link_libraries(rpcs3 PRIVATE ws2_32 Iphlpapi Winmm Psapi gdi32 setupapi pdh)
else()
    target_link_libraries(rpcs3 PRIVATE ${CMAKE_DL_LIBS})
endif()

if(USE_PRECOMPILED_HEADERS)
    target_precompile_headers(rpcs3 PRIVATE stdafx.h)
endif()

get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
if(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
elseif(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
endif()

# Copy icons to executable directory
if(APPLE)
    if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        set(QT_DEPLOY_FLAGS "-no-strip")
    else()
        set(QT_DEPLOY_FLAGS "")
    endif()
    add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/rpcs3.icns $<TARGET_FILE_DIR:rpcs3>/../Resources/rpcs3.icns
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/Icons $<TARGET_FILE_DIR:rpcs3>/../Resources/Icons
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/GuiConfigs $<TARGET_FILE_DIR:rpcs3>/../Resources/GuiConfigs
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/bin/git $<TARGET_FILE_DIR:rpcs3>/../Resources/git
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "${PROJECT_BINARY_DIR}/bin/rpcs3.app" "${QT_DEPLOY_FLAGS}")
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
    # offical Qt binaries are built with -MD(d) only as stated in offical wiki
    # https://wiki.qt.io/Technical_FAQ#Why_does_a_statically_built_Qt_use_the_dynamic_Visual_Studio_runtime_libraries_.3F_Do_I_need_to_deploy_those_with_my_application_.3F
    # If we build our libs with /MT(d), we might encounter some issues.
    # https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-160#what-problems-exist-if-an-application-uses-more-than-one-crt-version

    # Qt installed from Qt installer has following hierarchy:
    # bin/ for release and debug dlls and windeployqt tools
    # lib/cmake/Qt6/ for Qt6_Dir
    # Qt installed from vcpkg has following hierarchy:
    # bin/ for release dlls
    # debug/bin/ for debug dlls
    # tools/qt6/bin/ for tools including windeployqt
    # tools/qt6/debug/bin/ for tools with debug build including windeployqt
    # share/cmake/Qt6/ for Qt6_Dir

    # If Qt is installed from official Qt installer
    # list(APPEND _QT6_TOOLS_PATHS "${Qt6_DIR}/../../../bin/")

    # If Qt is installed from vcpkg
    # list(APPEND _QT6_TOOLS_PATHS "${Qt6_DIR}/../../../tools/qt6$<$<CONFIG:Debug>:/debug>/bin/")

    add_custom_command(TARGET rpcs3 POST_BUILD
        # COMMAND set PATH=${_QT6_TOOLS_PATHS}$<SEMICOLON>%PATH%
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/bin" "$<TARGET_FILE_DIR:rpcs3>"
        # If Qt is installed from vcpkg, add binary path to PATH
        # otherwise windeployqt tool won't be able to locate necessary dlls
        # COMMAND set PATH=${Qt6_DIR}/../../../$<$<CONFIG:Debug>:debug/>bin/$<SEMICOLON>%PATH% 
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-opengl-sw --no-patchqt --no-translations --no-quick --no-system-d3d-compiler --no-quick-import --plugindir "$<TARGET_FILE_DIR:rpcs3>/qt6/plugins" --verbose 0 "$<TARGET_FILE:rpcs3>")
endif()

# Unix installation
if(UNIX AND NOT APPLE)
    # Install the binary
    install(TARGETS rpcs3 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    # Install the application icon and menu item
    install(FILES rpcs3.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
    install(FILES rpcs3.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps)
    install(FILES rpcs3.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES rpcs3.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
    # Install other files
    install(DIRECTORY ../bin/Icons
        DESTINATION ${CMAKE_INSTALL_DATADIR}/rpcs3)
    install(DIRECTORY ../bin/GuiConfigs
        DESTINATION ${CMAKE_INSTALL_DATADIR}/rpcs3)
    install(DIRECTORY ../bin/git
        DESTINATION ${CMAKE_INSTALL_DATADIR}/rpcs3)
    install(DIRECTORY ../bin/test
        DESTINATION ${CMAKE_INSTALL_DATADIR}/rpcs3)
endif()
