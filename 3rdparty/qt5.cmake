add_library(3rdparty_qt5 INTERFACE)

find_package(Qt5 5.14 CONFIG COMPONENTS Widgets Qml Concurrent)
if(WIN32)
	find_package(Qt5 5.14 COMPONENTS WinExtras REQUIRED)
	target_link_libraries(3rdparty_qt5 INTERFACE Qt5::Widgets Qt5::WinExtras Qt5::Qml Qt5::Concurrent)
else()
	find_package(Qt5 5.14 COMPONENTS DBus Gui)
	if(Qt5DBus_FOUND)
		target_link_libraries(3rdparty_qt5 INTERFACE Qt5::Widgets Qt5::DBus Qt5::Qml Qt5::Concurrent)
		target_compile_definitions(3rdparty_qt5 INTERFACE -DHAVE_QTDBUS)
	else()
		target_link_libraries(3rdparty_qt5 INTERFACE Qt5::Widgets Qt5::Qml Qt5::Concurrent)
	endif()
	target_include_directories(3rdparty_qt5 INTERFACE ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
endif()

if(NOT Qt5Widgets_FOUND)
	if(Qt5Widgets_VERSION VERSION_LESS 5.14.0)
		message("Minimum supported Qt5 version is 5.14.0! You have version ${Qt5Widgets_VERSION} installed, please upgrade!")
		if(CMAKE_SYSTEM MATCHES "Linux")
			message(FATAL_ERROR "Most distros do not provide an up-to-date version of Qt.
If you're on Ubuntu or Linux Mint, there are PPAs you can use to install one of the latest qt5 versions.
		https://launchpad.net/~beineri/+archive/ubuntu/opt-qt-5.14.1-bionic
		https://launchpad.net/~beineri/+archive/ubuntu/opt-qt-5.14.1-xenial
just make sure to run
	source /opt/qt514/bin/qt514-env.sh
before re-running cmake")
		elseif(WIN32)
			message(FATAL_ERROR "You can download the latest version of Qt5 here: https://www.qt.io/download-open-source/")
		else()
			message(FATAL_ERROR "Look online for instructions on installing an up-to-date Qt5 on ${CMAKE_SYSTEM}.")
		endif()
	endif()

	message("CMake was unable to find Qt5!")
	if(WIN32)
		message(FATAL_ERROR "Make sure the QTDIR env variable has been set properly. (for example C:\\Qt\\5.14.1\\msvc2017_64\\)
You can also try setting the Qt5_DIR preprocessor definiton.")
	elseif(CMAKE_SYSTEM MATCHES "Linux")
		message(FATAL_ERROR "Make sure to install your distro's qt5 package!")
	else()
		message(FATAL_ERROR "You need to have Qt5 installed, look online for instructions on installing Qt5 on ${CMAKE_SYSTEM}.")
	endif()
endif()

add_library(3rdparty::qt5 ALIAS 3rdparty_qt5)
