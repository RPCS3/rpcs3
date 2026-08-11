# DiscordRPC

add_library(3rdparty_discordRPC INTERFACE)

if (USE_DISCORD_RPC AND (WIN32 OR CMAKE_SYSTEM MATCHES "Linux" OR APPLE))
	set(BUILD_EXAMPLES FALSE CACHE BOOL "Build example apps")
	set(ENABLE_IO_THREAD TRUE CACHE BOOL "Start up a separate I/O thread, otherwise I'd need to call an update function")
	set(USE_STATIC_CRT FALSE CACHE BOOL "Use /MT[d] for dynamic library")
	set(WARNINGS_AS_ERRORS FALSE CACHE BOOL "When enabled, compiles with `-Werror` (on *nix platforms).")

	add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)

	if (APPLE)
		foreach(property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
			get_target_property(discord_rpc_links discord-rpc ${property})

			if (discord_rpc_links)
				string(REPLACE
					", -mmacosx-version-min=10.10"
					""
					discord_rpc_links
					"${discord_rpc_links}"
				)

				set_property(TARGET discord-rpc PROPERTY ${property} "${discord_rpc_links}")
			endif()
		endforeach()
	endif()
	target_include_directories(3rdparty_discordRPC SYSTEM INTERFACE discord-rpc/include)
	target_compile_definitions(3rdparty_discordRPC INTERFACE -DWITH_DISCORD_RPC)
	target_link_libraries(3rdparty_discordRPC INTERFACE discord-rpc)
endif()
