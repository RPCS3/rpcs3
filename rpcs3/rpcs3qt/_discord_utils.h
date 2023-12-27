#pragma once

#include <string>

namespace discord_sdk
{
	// Convenience function for initialization
	void initialize();

	// Convenience function for shutdown
	void shutdown();

	// Convenience function for status updates. The default is set to idle.
	void update_presence(const std::string& state = "", const std::string& details = "Idle", bool reset_timer = true);

	// Run discord callbacks
	void run_callbacks();
}
