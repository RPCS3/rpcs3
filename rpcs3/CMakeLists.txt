cmake_minimum_required(VERSION 3.8.2)

include(cotire)

# Generate git-version.h at build time.
add_custom_target(GitVersion ALL
	COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/git-version.cmake)

# Check for a sufficient compiler and set build options
include(ConfigureCompiler)

set(ADDITIONAL_LIBS "")
if(CMAKE_SYSTEM MATCHES "Linux")
	#on some Linux distros shm_unlink and similar functions are in librt only
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "rt")
elseif(NOT MSVC AND NOT CMAKE_CXX_FLAGS MATCHES "LIBICONV_PLUG")
	#it seems like glibc includes the iconv functions we use but other libc
	#implementations like the one on OSX don't seem implement them
	set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} "iconv")
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
include(../3rdparty/qt5.cmake)

# subdirectories
add_subdirectory(Emu)
add_subdirectory(rpcs3qt)

file(GLOB RPCS3_SRC "*.cpp")

if(WIN32)
	add_executable(rpcs3 WIN32 ${RPCS3_SRC})
else()
	add_executable(rpcs3 ${RPCS3_SRC})
endif()

add_dependencies(rpcs3 GitVersion)
set_target_properties(rpcs3
	PROPERTIES
		AUTOMOC ON
		AUTOUIC ON)

target_link_libraries(rpcs3 rpcs3_emu rpcs3_ui)
target_link_libraries(rpcs3 3rdparty::discord-rpc 3rdparty::qt5 3rdparty::hidapi)
target_link_libraries(rpcs3 ${ADDITIONAL_LIBS})

# Win resource file
if (WIN32)
	target_sources(rpcs3 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/rpcs3.rc")
endif()

if(UNIX AND NOT APPLE)
	find_package(X11 REQUIRED)
	target_include_directories(rpcs3 PUBLIC ${X11_INCLUDE_DIR})
	target_link_libraries(rpcs3 ${X11_LIBRARIES})
endif()

if(UNIX)
	set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
	find_package(Threads REQUIRED)
	target_link_libraries(rpcs3 Threads::Threads)
endif()

if(WIN32)
	target_link_libraries(rpcs3 ws2_32.lib Winmm.lib Psapi.lib gdi32.lib setupapi.lib)
else()
	target_link_libraries(rpcs3 ${CMAKE_DL_LIBS})
endif()

set_target_properties(rpcs3 PROPERTIES
	COTIRE_CXX_PREFIX_HEADER_INIT "${RPCS3_SRC_DIR}/stdafx.h"
	COTIRE_ADD_UNITY_BUILD OFF)

cotire(rpcs3)

if (UNIX)
	# Copy icons to executable directory
	add_custom_command(TARGET rpcs3 POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/bin/Icons $<TARGET_FILE_DIR:rpcs3>/Icons)
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
endif()
