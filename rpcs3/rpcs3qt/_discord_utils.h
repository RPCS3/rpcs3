#pragma once

#include <string>

namespace discord
{
	// Convenience function for initialization
	void initialize(const std::string& application_id = "424004941485572097");

	// Convenience function for shutdown
	void shutdown();

	// Convenience function for status updates. The default is set to idle.
	// If start_timestamp != 0, it is used as the elapsed timer origin (overrides reset_timer).
	void update_presence(const std::string& state = "", const std::string& details = "Idle", bool reset_timer = true, int64_t start_timestamp = 0);
}
