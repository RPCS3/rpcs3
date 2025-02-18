if(USE_SYSTEM_ZLIB)
    list(REMOVE_ITEM CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
    find_package(ZLIB)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
else()
    add_library(ZLIB::ZLIB INTERFACE IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        INTERFACE_LINK_LIBRARIES zlibstatic
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/3rdparty/zlib/zlib;${CMAKE_BINARY_DIR}/3rdparty/zlib/zlib")
    set(ZLIB_FOUND TRUE)
endif()
