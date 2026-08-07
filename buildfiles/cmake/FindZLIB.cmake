if(USE_SYSTEM_ZLIB)
    list(REMOVE_ITEM CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
    find_package(ZLIB)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
else()
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB STATIC IMPORTED GLOBAL)
        if(MSVC)
            # On MSVC, zlib builds as zs.lib (Release) / zsd.lib (Debug) in per-config subdirectories
            set_target_properties(ZLIB::ZLIB PROPERTIES
                IMPORTED_LOCATION_RELEASE         "${rpcs3_BINARY_DIR}/3rdparty/zlib/zlib/Release/zs.lib"
                IMPORTED_LOCATION_DEBUG           "${rpcs3_BINARY_DIR}/3rdparty/zlib/zlib/Debug/zsd.lib"
                IMPORTED_LOCATION_RELWITHDEBINFO  "${rpcs3_BINARY_DIR}/3rdparty/zlib/zlib/RelWithDebInfo/zs.lib"
                IMPORTED_LOCATION_MINSIZEREL      "${rpcs3_BINARY_DIR}/3rdparty/zlib/zlib/MinSizeRel/zs.lib")
        else()
            # On non-MSVC, zlib OUTPUT_NAME is "z" → libz.a
            set_target_properties(ZLIB::ZLIB PROPERTIES
                IMPORTED_LOCATION "${rpcs3_BINARY_DIR}/3rdparty/zlib/zlib/libz.a")
        endif()
        set_target_properties(ZLIB::ZLIB PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${rpcs3_SOURCE_DIR}/3rdparty/zlib/zlib;${rpcs3_BINARY_DIR}/3rdparty/zlib/zlib")
    endif()
    set(ZLIB_FOUND TRUE)
endif()
