#pragma once

#include "openxr_common.h"
#include "openxr_input.h"
#include "openxr_renderer.h"

#include "util/atomic.hpp"

#include <chrono>
#include <string_view>

namespace openxr
{
	// Owns the OpenXR instance, the session and its own Vulkan device, and runs the frame loop.
	// It stays idle until OpenXR is needed for input or headset rendering, and retries if no headset is available.
	class session_thread
	{
	public:
		static constexpr std::string_view thread_name = "OpenXR Thread";

		session_thread() = default;
		~session_thread();

		session_thread(const session_thread&) = delete;
		session_thread& operator=(const session_thread&) = delete;

		void operator()();

		bool is_input_active() const { return m_input_active; }
		input& get_input() { return m_input; }

	private:
		static bool is_rendering_wanted();

		bool init();
		void shutdown();

		// Polls runtime events, runs one frame and updates the input.
		// Blocks in xrWaitFrame until the runtime's next frame while the session is running.
		// Returns false if the session has to be recreated.
		bool update();

		bool create_instance();
		bool get_system_info();
		bool create_vulkan_device();
		bool create_session();
		bool create_reference_space();
		bool begin_session();
		void poll_events();
		void handle_session_state_change(XrSessionState state);
		bool run_frame(XrTime& predicted_time);

		XrInstance m_instance{XR_NULL_HANDLE};
		XrSystemId m_system{};
		std::set<std::string> m_enabled_extensions;
		XrSession m_session{XR_NULL_HANDLE};
		XrSessionState m_session_state = XR_SESSION_STATE_UNKNOWN;
		XrSpace m_reference_space{XR_NULL_HANDLE};
		XrEnvironmentBlendMode m_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		bool m_session_running = false;
		bool m_restart_required = false;
		std::chrono::steady_clock::time_point m_next_init_attempt{};

		// The session is visible but has no input focus since this time (see handle_session_state_change)
		std::chrono::steady_clock::time_point m_unfocused_since{};
		bool m_unfocused_warning_shown = false;

		input m_input;
		atomic_t<bool> m_input_active = false;

		// We only create one queue
		static constexpr u32 vk_queue_index = 0;

		VkInstance m_vk_instance{};
		VkPhysicalDevice m_vk_physical{};
		VkDevice m_vk_device{};
		u32 m_vk_queue_family{};
		bool m_rendering_supported = false; // The device supports headset rendering
		bool m_rendering_requested = false; // Headset rendering is enabled in the config (see update_renderer)

		void update_renderer();

		renderer m_renderer;
		placeholder_layer m_placeholder;
	};
}
