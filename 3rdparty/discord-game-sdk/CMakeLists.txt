# Discord Game SDK
add_library(3rdparty_discordGameSDK INTERFACE)

# We don't want Discord Rich Presence on the BSDs and other OSes
if (USE_DISCORD_RPC AND (WIN32 OR CMAKE_SYSTEM MATCHES "Linux" OR APPLE) AND COMPILER_X86)

	target_include_directories(3rdparty_discordGameSDK INTERFACE cpp)
	target_compile_definitions(3rdparty_discordGameSDK INTERFACE -DWITH_DISCORD_RPC)

	set(DISCORD_GAME_SDK_LIB NOTFOUND)
	if (WIN32)
		if (NOT MSVC)
			set(USE_DISCORD_RPC OFF CACHE BOOL "Discord RPC is only available in MSVC builds." FORCE)
		else()
			find_library(DISCORD_GAME_SDK_LIB discord-game-sdk.dll.lib PATHS lib/x86_64/ NO_DEFAULT_PATH)
		endif()
	elseif(CMAKE_SYSTEM MATCHES "Linux")
		find_library(DISCORD_GAME_SDK_LIB discord-game-sdk.so PATHS lib/x86_64/ NO_DEFAULT_PATH)
	elseif(APPLE)
		find_library(DISCORD_GAME_SDK_LIB discord-game-sdk.dylib PATHS lib/x86_64/ NO_DEFAULT_PATH)
	endif()
	target_link_libraries(3rdparty_discordGameSDK INTERFACE ${DISCORD_GAME_SDK_LIB})
	if(APPLE)
		target_link_libraries(3rdparty_discordGameSDK INTERFACE "objc" "-framework Foundation" "-framework CoreServices")
	endif()
endif()
