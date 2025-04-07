# DiscordRPC

add_library(3rdparty_discordRPC INTERFACE)

if (USE_DISCORD_RPC AND (WIN32 OR CMAKE_SYSTEM MATCHES "Linux" OR APPLE))
	set(BUILD_EXAMPLES FALSE CACHE BOOL "Build example apps")
	set(ENABLE_IO_THREAD TRUE CACHE BOOL "Start up a separate I/O thread, otherwise I'd need to call an update function")
	set(USE_STATIC_CRT FALSE CACHE BOOL "Use /MT[d] for dynamic library")
	set(WARNINGS_AS_ERRORS FALSE CACHE BOOL "When enabled, compiles with `-Werror` (on *nix platforms).")

	add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
	target_include_directories(3rdparty_discordRPC INTERFACE discord-rpc/include)
	target_compile_definitions(3rdparty_discordRPC INTERFACE -DWITH_DISCORD_RPC)
	target_link_libraries(3rdparty_discordRPC INTERFACE discord-rpc)
endif()
