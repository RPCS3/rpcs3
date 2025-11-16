# Define GNU standard installation directories
include(GNUInstallDirs)

# Generate git-version.h at build time.
include(${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)

# Check for a sufficient compiler and set build options
include(ConfigureCompiler)
include(CheckFunctionExists)

set(CMAKE_CXX_STANDARD 20)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    add_compile_definitions(DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/rpcs3")
    # Optionally enable X11 for window management
    find_package(X11)
    if(X11_FOUND)
        add_compile_definitions(HAVE_X11)
    endif()
    find_package(Wayland)
    if(WAYLAND_FOUND)
        add_compile_definitions(HAVE_WAYLAND)
    endif()
endif()

if (NOT ANDROID)
    # Qt
    # finds Qt libraries and setups custom commands for MOC and UIC
    # Must be done here because generated MOC and UIC targets cant
    # be found otherwise
    include(${CMAKE_SOURCE_DIR}/3rdparty/qt6.cmake)
endif()

# subdirectories
add_subdirectory(Emu)

if (NOT ANDROID)
    add_subdirectory(rpcs3qt)
endif()

gen_git_version(${CMAKE_CURRENT_SOURCE_DIR})

if (NOT ANDROID)
    # Build rpcs3_lib
    add_library(rpcs3_lib STATIC)

    if(WIN32)
        target_compile_definitions(rpcs3_lib PRIVATE UNICODE _UNICODE)
    endif()

    set_target_properties(rpcs3_lib
        PROPERTIES
            AUTOMOC ON
            AUTOUIC ON)

    target_link_libraries(rpcs3_lib
        PUBLIC
            3rdparty::stblib
            3rdparty::libevdev
            rpcs3_emu
        PRIVATE
            rpcs3_ui
            3rdparty::discordRPC
            3rdparty::qt6
            3rdparty::hidapi
            3rdparty::libusb
            3rdparty::wolfssl
            3rdparty::libcurl
            3rdparty::zlib
            3rdparty::opencv
            3rdparty::fusion)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        #on some Linux distros shm_unlink and similar functions are in librt only
        target_link_libraries(rpcs3_lib PRIVATE rt)
    elseif(NOT WIN32 AND NOT CMAKE_CXX_FLAGS MATCHES "LIBICONV_PLUG")
        #it seems like glibc includes the iconv functions we use but other libc
        #implementations like the one on OSX don't seem implement them
        target_link_libraries(rpcs3_lib PRIVATE iconv)
    endif()

    # Unix display manager
    if(X11_FOUND)
        target_link_libraries(rpcs3_lib PRIVATE X11::X11)
    elseif(USE_VULKAN AND UNIX AND NOT WAYLAND_FOUND AND NOT APPLE AND NOT ANDROID)
        # Wayland has been checked in 3rdparty/CMakeLists.txt already.
        message(FATAL_ERROR "RPCS3 requires either X11 or Wayland (or both) for Vulkan.")
    endif()

    if(UNIX)
        set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
        find_package(Threads REQUIRED)
        target_link_libraries(rpcs3_lib PRIVATE Threads::Threads)
    endif()

    if(WIN32)
        target_link_libraries(rpcs3_lib PRIVATE ws2_32 Iphlpapi Winmm Psapi gdi32 setupapi)
    else()
        target_link_libraries(rpcs3_lib PRIVATE ${CMAKE_DL_LIBS})
    endif()

    if(USE_PRECOMPILED_HEADERS)
        target_precompile_headers(rpcs3_lib PRIVATE stdafx.h)
    endif()

    # Build rpcs3 executable
    add_executable(rpcs3 WIN32 MACOSX_BUNDLE)
    if(WIN32)
        target_sources(rpcs3 PRIVATE rpcs3.rc)
        target_compile_definitions(rpcs3 PRIVATE UNICODE _UNICODE)
    elseif(APPLE)
        target_sources(rpcs3 PRIVATE rpcs3.icns update_helper.sh)
        set_source_files_properties(update_helper.sh PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set_target_properties(rpcs3
            PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/rpcs3.plist.in")
    endif()

    target_sources(rpcs3
        PRIVATE
            main.cpp
    )

    target_link_libraries(rpcs3
        PRIVATE
            rpcs3_lib
    )

    if(USE_PRECOMPILED_HEADERS)
        target_precompile_headers(rpcs3 PRIVATE stdafx.h)
    endif()

    # Copy icons to executable directory
    if(APPLE)
        qt_finalize_target(rpcs3)
        add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/rpcs3.icns $<TARGET_FILE_DIR:rpcs3>/../Resources/rpcs3.icns
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/Icons $<TARGET_FILE_DIR:rpcs3>/../Resources/Icons
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/GuiConfigs $<TARGET_FILE_DIR:rpcs3>/../Resources/GuiConfigs
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "${PROJECT_BINARY_DIR}/bin/rpcs3.app" "$<$<CONFIG:Debug,RelWithDebInfo>:-no-strip>")
    elseif(UNIX)
        add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/Icons $<TARGET_FILE_DIR:rpcs3>/Icons
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/GuiConfigs $<TARGET_FILE_DIR:rpcs3>/GuiConfigs)
    elseif(WIN32)
        add_custom_command(TARGET rpcs3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/Icons $<TARGET_FILE_DIR:rpcs3>/Icons
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/GuiConfigs $<TARGET_FILE_DIR:rpcs3>/GuiConfigs
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/test $<TARGET_FILE_DIR:rpcs3>/test
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-opengl-sw --no-patchqt
                --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-quick-import
                --plugindir "$<IF:$<CXX_COMPILER_ID:MSVC>,$<TARGET_FILE_DIR:rpcs3>/plugins,$<TARGET_FILE_DIR:rpcs3>/share/qt6/plugins>"
                --verbose 0 "$<TARGET_FILE:rpcs3>")
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
        install(DIRECTORY ../bin/test
            DESTINATION ${CMAKE_INSTALL_DATADIR}/rpcs3)
    endif()
endif()

# Unit tests
if(BUILD_RPCS3_TESTS)
    find_package(GTest REQUIRED)

    message(STATUS "Building unit tests...")

    add_executable(rpcs3_test)

    target_sources(rpcs3_test
        PRIVATE
            tests/test.cpp
            tests/test_fmt.cpp
            tests/test_pair.cpp
            tests/test_tuple.cpp
            tests/test_simple_array.cpp
            tests/test_address_range.cpp
    )

    target_link_libraries(rpcs3_test
        PRIVATE
            rpcs3_lib
            rpcs3_emu
            GTest::gtest
    )

    target_include_directories(rpcs3_test
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    include(GoogleTest)
    gtest_discover_tests(rpcs3_test)

    if(RUN_RPCS3_TESTS)
        add_custom_target(run_tests
            ALL
            COMMAND ${CMAKE_CTEST_COMMAND} -j -VV --output-on-failure
            DEPENDS rpcs3_test
     )
    endif()
endif()
