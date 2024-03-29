# DiscordRPC
add_library(3rdparty_discordRPC INTERFACE)

check_cxx_compiler_flag("-msse -msse2 -mcx16" COMPILER_X86)

# We don't want Discord Rich Presence on the BSDs and other OSes
if (USE_DISCORD_RPC AND (WIN32 OR CMAKE_SYSTEM MATCHES "Linux" OR APPLE) AND COMPILER_X86)
	if (WIN32 AND NOT MSVC)
		ExternalProject_Add(discordRPC
				GIT_REPOSITORY https://github.com/discordapp/discord-rpc
				BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
				INSTALL_COMMAND ""
			)

	endif()

	target_include_directories(3rdparty_discordRPC INTERFACE include)
	target_compile_definitions(3rdparty_discordRPC INTERFACE -DWITH_DISCORD_RPC)

	set(DISCORD_RPC_LIB NOTFOUND)
	if (WIN32)
		if (NOT MSVC)
			set(DISCORD_RPC_LIB ${CMAKE_CURRENT_BINARY_DIR}/src/libdiscord-rpc.a)
		else()
			find_library(DISCORD_RPC_LIB discord-rpc PATHS lib/ NO_DEFAULT_PATH)
		endif()
	elseif(CMAKE_SYSTEM MATCHES "Linux")
		find_library(DISCORD_RPC_LIB discord-rpc-linux PATHS lib/ NO_DEFAULT_PATH)
	elseif(APPLE)
		find_library(DISCORD_RPC_LIB discord-rpc-mac PATHS lib/ NO_DEFAULT_PATH)
	endif()
	target_link_libraries(3rdparty_discordRPC INTERFACE ${DISCORD_RPC_LIB})
	if(APPLE)
		target_link_libraries(3rdparty_discordRPC INTERFACE "objc" "-framework Foundation" "-framework CoreServices")
	endif()
endif()
