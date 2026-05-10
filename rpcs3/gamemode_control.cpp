#include "gamemode_control.h"

#ifdef GAMEMODE_AVAILABLE
#pragma GCC diagnostic ignored "-Wold-style-cast"
extern "C" {
	#include "3rdparty/feralinteractive/feralinteractive/lib/gamemode_client.h"
}
#endif

// Enables and Disables GameMode based on user settings and system
void enable_gamemode([[maybe_unused]] bool enabled)
{
#if defined(GAMEMODE_AVAILABLE)
	// Enable and Disable Gamemode
	if (enabled)
	{
		gamemode_request_start();
	}
	else
	{
		gamemode_request_end();
	}
#endif
}
