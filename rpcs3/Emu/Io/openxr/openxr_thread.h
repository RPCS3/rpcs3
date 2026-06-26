#pragma once

// Interface to the OpenXR thread that does not depend on the OpenXR SDK.
// Without OpenXR support the functions do nothing, so callers don't need to check HAVE_OPENXR.

namespace openxr
{
	// Everything that needs the OpenXR session. The OpenXR thread only runs while at least one of them is active.
	enum class consumer
	{
		pad_thread,        // The pad thread of a game uses the OpenXR pad handler
		pad_settings,      // The pad settings dialog uses the OpenXR pad handler
		headset_rendering, // A renderer wants to show the game in the headset
	};

#ifdef HAVE_OPENXR
	// Activates or deactivates a consumer. Can be called repeatedly with the same state.
	// The thread is started for the first active consumer and stopped once there are none left.
	// Stopping waits until the session was destroyed, so this can take a moment.
	void set_consumer(consumer c, bool active);

	// Deactivates all consumers and stops the thread. Has to be called before RPCS3 exits.
	void stop_thread();

	// Returns true if an OpenXR runtime is installed
	bool is_openxr_available();
#else
	inline void set_consumer(consumer, bool) {}
	inline void stop_thread() {}
	inline bool is_openxr_available() { return false; }
#endif
}
