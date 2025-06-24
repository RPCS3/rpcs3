#include "gamemode_control.h"

#ifdef GAMEMODE_AVAILABLE
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C" {
	#include "3rdparty/feralinteractive/feralinteractive/lib/gamemode_client.h"
}

// Checks if Gamemode is supported on system
bool gamemode_supported()
{
#if defined(GAMEMODE_AVAILABLE)
	return true
#else
	return false;
#endif
}

// TODO: Might add later
/*
static int gamemode_status() {
	return gamemode_query_status();
}
*/

// Enables and Disables GameMode based on user settings and system
void enable_gamemode(bool enabled) {
	if (!gamemode_supported()) {
		return;
	}

	// Enable and Disable Gamemode
	if (enabled) {
		gamemode_request_start();
	} else {
		gamemode_request_end();
	}
}
