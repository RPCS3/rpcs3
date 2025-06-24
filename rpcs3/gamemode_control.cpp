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

// Enables GameMode
//		Returns -2 if Gamemode isn't supported.
//		Returns the value of gamemode_request_start() otherwise.
//		This is either 0 or -1 depending on whether gamemode was successfully enabled or not respectively.
int enable_gamemode(bool enabled) {
	if (!gamemode_supported()) {
		return -2;
	}

	// Enable Gamemode
	if (enabled) {
		u8 s_gamemode_start = gamemode_request_start();
		return s_gamemode_start;
	}
}
