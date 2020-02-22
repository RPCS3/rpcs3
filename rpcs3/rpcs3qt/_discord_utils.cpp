#ifdef WITH_DISCORD_RPC
#include "stdafx.h"
#include "_discord_utils.h"
#include "discord_rpc.h"
#include "discord_register.h"

namespace discord
{
	void initialize(const std::string& application_id)
	{
		DiscordEventHandlers handlers = {};
		Discord_Initialize(application_id.c_str(), &handlers, 1, NULL);
	}

	void shutdown()
	{
		Discord_Shutdown();
	}

	void update_presence(const std::string& state, const std::string& details, bool reset_timer)
	{
		DiscordRichPresence discordPresence = {};
		discordPresence.details = details.c_str();
		discordPresence.state = state.c_str();
		discordPresence.largeImageKey = "rpcs3_logo";
		discordPresence.largeImageText = "RPCS3 is the world's first PlayStation 3 emulator.";

		if (reset_timer)
		{
			discordPresence.startTimestamp = time(0);
		}

		Discord_UpdatePresence(&discordPresence);
	}
}
#endif
