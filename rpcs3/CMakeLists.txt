cmake_minimum_required(VERSION 2.8)

project(rpcs3)

if (CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=gnu++11)
    add_definitions(-D__WXGTK__)
    #add_definitions(-Wfatal-errors)
    add_definitions(-w) # TODO: remove me
    add_definitions(-DwxUSE_UNICODE=0)
    add_definitions(-fpermissive) # TODO: remove me
endif()

find_package(wxWidgets)

include("${wxWidgets_USE_FILE}")

include_directories(
${wxWidgets_INCLUDE_DIRS}
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/Emu
${CMAKE_SOURCE_DIR}/Gui
${CMAKE_SOURCE_DIR}/Loader
${CMAKE_SOURCE_DIR}/..
)

file(
GLOB_RECURSE
RPCS3_SRC
${CMAKE_SOURCE_DIR}/Emu/*
${CMAKE_SOURCE_DIR}/Gui/*
${CMAKE_SOURCE_DIR}/Loader/*
${CMAKE_SOURCE_DIR}/../Utilities/*
)

add_executable(rpcs3 ${RPCS3_SRC})
target_link_libraries(rpcs3 ${wxWidgets_LIBRARIES})
