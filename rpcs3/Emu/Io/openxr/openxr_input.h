#pragma once

#include "openxr/openxr.h"

#include "util/types.hpp"
#include "util/atomic.hpp"

#include <array>
#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <string>

namespace openxr
{
	enum class action_id
	{
		grip_pose,
		aim_pose,
		trigger_analog,
		trigger_click,
		squeeze_analog,
		squeeze_click,
		thumbstick,
		thumbstick_click,
		thumbstick_touch,
		trackpad,
		trackpad_click,
		trackpad_touch,
		select_button,
		menu_button,
		a_button,
		b_button,
		x_button,
		y_button,
		haptic, // Vibration output

		count
	};

	// One value per action
	template <typename T>
	struct action_array : std::array<T, static_cast<usz>(action_id::count)>
	{
		using base = std::array<T, static_cast<usz>(action_id::count)>;

		T& operator[](action_id id) { return base::operator[](static_cast<usz>(id)); }
		const T& operator[](action_id id) const { return base::operator[](static_cast<usz>(id)); }
	};

	struct pose_state
	{
		XrPosef pose{};
		bool is_active = false;
		bool is_valid = false;

		// What motion sensors in the controller would measure, in the frame of the pose (+X right, +Y up, +Z back).
		// The accelerometer includes gravity, so it reads +1 on Y while the controller rests flat.
		bool has_motion = false;
		XrVector3f accelerometer{}; // In G
		XrVector3f gyro{};          // In rad/s (right-handed, so +Y is a yaw to the left)
	};

	// Snapshot of one controller (0 = left hand, 1 = right hand)
	struct controller_state
	{
		action_array<pose_state> poses{};
		action_array<bool> booleans{};
		action_array<f32> floats{};
		action_array<XrVector2f> vectors2f{};
	};

	// Returns false if the OpenXR thread does not run or has no input
	bool get_controller_states(std::array<controller_state, 2>& states);

	// Sets the vibration strength (0 to 1) of a controller (0 = left hand, 1 = right hand)
	void set_controller_vibration(u32 hand, f32 amplitude);

	// Controller actions and their bindings for all known interaction profiles
	class input
	{
	public:
		// Returns the extensions of all controller profiles that the runtime supports
		static std::set<std::string> get_profile_extensions(const std::set<std::string>& supported_extensions);

		// Has to be called after the session was created and before it begins
		bool init(XrInstance instance, XrSession session, const std::set<std::string>& enabled_extensions);

		// Has to be called before the session is destroyed
		void destroy();

		// Syncs the actions and updates the controller states. Only valid while the session is focused.
		void update(XrSpace reference_space, XrTime predicted_time);

		std::array<controller_state, 2> get_states() const;

		// Resets the states, for example when the session loses focus and we don't get any input
		void clear_states();

		// Thread safe. Applied in update.
		void set_vibration(u32 hand, f32 amplitude);

	private:
		// The space of a pose action and the state that is needed to simulate motion sensors
		struct pose_space
		{
			void update_motion(pose_state& state, const XrSpaceVelocity& velocity, XrTime time);
			void reset_motion();

			XrSpace space {XR_NULL_HANDLE};
			XrVector3f linear_acceleration{}; // Smoothed, in m/s² in the reference space
			XrVector3f last_linear_velocity{};
			XrTime last_velocity_time = 0;
			bool has_last_velocity = false;
		};

		struct controller
		{
			XrPath subaction_path {XR_NULL_PATH};
			std::map<action_id, pose_space> pose_spaces;
			controller_state state;
		};

		bool create_actions(XrInstance instance);
		bool suggest_bindings(XrInstance instance, const std::set<std::string>& enabled_extensions);
		bool create_spaces();
		bool attach_actions();
		void update_hand(XrSpace ref, XrTime time, controller& ctrl);
		void apply_vibration(u32 hand, const controller& ctrl);
		bool sync_actions() const;

		// Each vibration lasts this long, and is renewed while the game keeps it active
		static constexpr XrDuration vibration_duration_ns = 100'000'000;
		static constexpr auto vibration_renew_interval = std::chrono::milliseconds(50);

		XrSession m_session{XR_NULL_HANDLE};
		XrActionSet m_action_set{XR_NULL_HANDLE};
		action_array<XrAction> m_actions{}; // Shared between both controllers
		std::array<controller, 2> m_controllers;

		// The states that other threads can read
		mutable std::mutex m_states_mutex;
		std::array<controller_state, 2> m_states;

		// Requested by other threads
		std::array<atomic_t<f32>, 2> m_vibration{};

		// Only used by the OpenXR thread
		std::array<f32, 2> m_applied_vibration{};
		std::array<steady_clock::time_point, 2> m_last_vibration_time{};
	};
}
