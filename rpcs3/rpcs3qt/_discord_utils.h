#pragma once

#include <string>

namespace discord
{
	// Convenience function for initialization
	void initialize(const std::string& application_id = "424004941485572097");

	// Convenience function for shutdown
	void shutdown();

	// Convenience function for status updates. The default is set to idle.
	void update_presence(const std::string& state = "", const std::string& details = "Idle", bool reset_timer = true);
}
