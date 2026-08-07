add_library(rcheevos STATIC
    # Core client
    rcheevos/src/rc_client.c
    rcheevos/src/rc_client_external.c
    rcheevos/src/rc_compat.c
    rcheevos/src/rc_util.c
    rcheevos/src/rc_version.c

    # REST API
    rcheevos/src/rapi/rc_api_common.c
    rcheevos/src/rapi/rc_api_editor.c
    rcheevos/src/rapi/rc_api_info.c
    rcheevos/src/rapi/rc_api_runtime.c
    rcheevos/src/rapi/rc_api_user.c

    # Achievement engine
    rcheevos/src/rcheevos/alloc.c
    rcheevos/src/rcheevos/condition.c
    rcheevos/src/rcheevos/condset.c
    rcheevos/src/rcheevos/consoleinfo.c
    rcheevos/src/rcheevos/format.c
    rcheevos/src/rcheevos/lboard.c
    rcheevos/src/rcheevos/memref.c
    rcheevos/src/rcheevos/operand.c
    rcheevos/src/rcheevos/rc_validate.c
    rcheevos/src/rcheevos/richpresence.c
    rcheevos/src/rcheevos/runtime.c
    rcheevos/src/rcheevos/runtime_progress.c
    rcheevos/src/rcheevos/trigger.c
    rcheevos/src/rcheevos/value.c

    # Media hashing
    rcheevos/src/rhash/cdreader.c
    rcheevos/src/rhash/aes.c
    rcheevos/src/rhash/hash.c
    rcheevos/src/rhash/hash_disc.c
    rcheevos/src/rhash/hash_encrypted.c
    rcheevos/src/rhash/hash_rom.c
    rcheevos/src/rhash/hash_zip.c
    rcheevos/src/rhash/md5.c
)

if(WIN32)
    target_sources(rcheevos PRIVATE rcheevos/src/rc_client_raintegration.c)
    target_compile_definitions(rcheevos PRIVATE RC_CLIENT_SUPPORTS_RAINTEGRATION)
endif()

target_include_directories(rcheevos PUBLIC rcheevos/include)

target_compile_definitions(rcheevos PUBLIC
    RC_CLIENT_SUPPORTS_HASH
)

target_compile_definitions(rcheevos PRIVATE
    RC_DISABLE_LUA=1
)

add_library(3rdparty_rcheevos INTERFACE)
target_link_libraries(3rdparty_rcheevos INTERFACE rcheevos)
