#ifdef WITH_DISCORD_RPC
#include "stdafx.h"
#include "_discord_utils.h"
#include "discord.h"

#include <ctime>

LOG_CHANNEL(DISCORD)

namespace discord_sdk
{
	constexpr discord::ClientId client_id = 424004941485572097;

	std::unique_ptr<discord::Core> g_core;

	void initialize()
	{
		DISCORD.notice("Initializing discord core");

		discord::Core* core = nullptr;
		discord::Result result = discord::Core::Create(client_id, DiscordCreateFlags_Default, &core);
		g_core.reset(core);

		if (!g_core)
		{
			DISCORD.error("Failed to instantiate discord core! (error=%d)", static_cast<int>(result));
			return;
		}

		g_core->SetLogHook(discord::LogLevel::Debug, [](discord::LogLevel level, const char* message)
		{
			if (!message)
			{
				return;
			}

			switch (level)
			{
			case discord::LogLevel::Error:
				DISCORD.error("%s", message);
				break;
			case discord::LogLevel::Warn:
				DISCORD.warning("%s", message);
				break;
			case discord::LogLevel::Info:
				DISCORD.notice("%s", message);
				break;
			case discord::LogLevel::Debug:
				DISCORD.trace("%s", message);
				break;
			}
		});
	}

	void shutdown()
	{
		if (g_core)
		{
			g_core->ActivityManager().ClearActivity([](discord::Result result)
			{
				if (result == discord::Result::Ok)
				{
					DISCORD.trace("ClearActivity successful");
				}
				else
				{
					DISCORD.notice("ClearActivity failed (error=%d)", static_cast<int>(result));
				}
			});
			g_core.reset();
		}
	}

	void update_presence(const std::string& state, const std::string& details, bool reset_timer)
	{
		if (!g_core)
		{
			DISCORD.error("Discord core not initialized!");
			return;
		}

		discord::Activity activity{};
		activity.SetType(discord::ActivityType::Playing);
		activity.SetApplicationId(client_id);
		activity.SetState(state.c_str());
		activity.SetDetails(details.c_str());
		activity.SetInstance(true);
		activity.GetAssets().SetLargeImage("rpcs3_logo");
		activity.GetAssets().SetLargeText("RPCS3 is the world's first PlayStation 3 emulator.");

		if (reset_timer)
		{
			activity.GetTimestamps().SetStart(std::time(nullptr));
		}

		g_core->ActivityManager().UpdateActivity(activity, [](discord::Result result)
		{
			if (result == discord::Result::Ok)
			{
				DISCORD.trace("UpdateActivity successful");
			}
			else
			{
				DISCORD.notice("UpdateActivity failed (error=%d)", static_cast<int>(result));
			}
		});
	}

	void run_callbacks()
	{
		if (g_core)
		{
			g_core->RunCallbacks();
		}
	}
}
#endif
