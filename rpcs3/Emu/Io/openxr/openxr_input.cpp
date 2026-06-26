#include "stdafx.h"
#include "openxr_input.h"
#include "openxr_common.h"

namespace openxr
{
static const std::string& get_hand_path(u32 controller_index)
{
	static const std::string left_hand_path = "/user/hand/left";
	static const std::string right_hand_path = "/user/hand/right";
	return controller_index == 0 ? left_hand_path : right_hand_path;
}

struct input_option
{
	std::string name; // Must only contain lowercase letters, digits, '-', '_' and '.'
	std::string localized_name;
	std::string path_suffix;
	XrActionType action_type = XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT;

	std::string get_path(u32 controller_index) const
	{
		return get_hand_path(controller_index) + path_suffix;
	}
};


static const std::map<action_id, input_option> generic_input_options =
{
	{action_id::grip_pose, { "grip_pose", "Grip pose", "/input/grip/pose", XrActionType::XR_ACTION_TYPE_POSE_INPUT }},
	{action_id::aim_pose, { "aim_pose", "Aim pose", "/input/aim/pose", XrActionType::XR_ACTION_TYPE_POSE_INPUT }},
	{action_id::trigger_analog, { "trigger_analog", "Trigger analog", "/input/trigger/value", XrActionType::XR_ACTION_TYPE_FLOAT_INPUT }},
	{action_id::trigger_click, { "trigger_click", "Trigger click", "/input/trigger/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::squeeze_analog, { "squeeze_analog", "Squeeze/grip analog", "/input/squeeze/value", XrActionType::XR_ACTION_TYPE_FLOAT_INPUT }},
	{action_id::squeeze_click, { "squeeze_click", "Squeeze click", "/input/squeeze/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::thumbstick, { "thumbstick", "Thumbstick", "/input/thumbstick", XrActionType::XR_ACTION_TYPE_VECTOR2F_INPUT }},
	{action_id::thumbstick_click, { "thumbstick_click", "Thumbstick click", "/input/thumbstick/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::thumbstick_touch, { "thumbstick_touch", "Thumbstick touch", "/input/thumbstick/touch", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::trackpad, { "trackpad", "Trackpad", "/input/trackpad", XrActionType::XR_ACTION_TYPE_VECTOR2F_INPUT }},
	{action_id::trackpad_click, { "trackpad_click", "Trackpad click", "/input/trackpad/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::trackpad_touch, { "trackpad_touch", "Trackpad touch", "/input/trackpad/touch", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::select_button, { "select_button", "Select button", "/input/select/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::menu_button, { "menu_button", "Menu button", "/input/menu/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::a_button, { "a_button", "A button", "/input/a/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::b_button, { "b_button", "B button", "/input/b/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::x_button, { "x_button", "X button", "/input/x/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::y_button, { "y_button", "Y button", "/input/y/click", XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT }},
	{action_id::haptic, { "haptic", "Vibration", "/output/haptic", XrActionType::XR_ACTION_TYPE_VIBRATION_OUTPUT }},
};

struct interaction_profile
{
	std::string path;
	std::string extension; // Empty if the profile is part of the core specification
	std::vector<action_id> both_hands;
	std::vector<action_id> left_hand;
	std::vector<action_id> right_hand;

	const std::vector<action_id>& get_hand_actions(u32 controller_index) const
	{
		return controller_index == 0 ? left_hand : right_hand;
	}
};

// Inputs shared by the Oculus Touch controller and its successors
static const std::vector<action_id> touch_both_hands =
{
	action_id::grip_pose, action_id::aim_pose, action_id::haptic,
	action_id::trigger_analog, action_id::squeeze_analog,
	action_id::thumbstick, action_id::thumbstick_click, action_id::thumbstick_touch
};
static const std::vector<action_id> touch_left_hand = { action_id::x_button, action_id::y_button, action_id::menu_button };
static const std::vector<action_id> touch_right_hand = { action_id::a_button, action_id::b_button };

// Hand specific inputs of the HTC Vive Cosmos and Focus 3 controllers
static const std::vector<action_id> htc_left_hand = { action_id::x_button, action_id::y_button, action_id::menu_button };
static const std::vector<action_id> htc_right_hand = { action_id::a_button, action_id::b_button };

// Inputs shared by Windows Mixed Reality controllers
static const std::vector<action_id> wmr_both_hands =
{
	action_id::grip_pose, action_id::aim_pose, action_id::haptic,
	action_id::trigger_analog, action_id::squeeze_click,
	action_id::thumbstick, action_id::thumbstick_click,
	action_id::trackpad, action_id::trackpad_click, action_id::trackpad_touch,
	action_id::menu_button
};

// Runtimes reject the whole suggestion if a single path is not supported by the profile.
// Profiles from extensions can only be suggested if the extension was enabled.
static const std::vector<interaction_profile> interaction_profiles =
{
	// Generic fallback that every runtime has to support
	{
		"/interaction_profiles/khr/simple_controller", "",
		{ action_id::grip_pose, action_id::aim_pose, action_id::haptic, action_id::select_button, action_id::menu_button },
		{},
		{}
	},
	// Oculus Rift CV1, Rift S, Quest 1/2 (Quest 3 and Pro are usually mapped to this profile as well)
	{
		"/interaction_profiles/oculus/touch_controller", "",
		touch_both_hands,
		touch_left_hand,
		touch_right_hand
	},
	// Meta Quest 3 (Touch Plus)
	{
		"/interaction_profiles/meta/touch_controller_plus", "XR_META_touch_controller_plus",
		touch_both_hands,
		touch_left_hand,
		touch_right_hand
	},
	// Meta Quest Pro (Touch Pro)
	{
		"/interaction_profiles/facebook/touch_controller_pro", "XR_FB_touch_controller_pro",
		touch_both_hands,
		touch_left_hand,
		touch_right_hand
	},
	// Valve Index (also used by Bigscreen Beyond, Pimax, Varjo etc. with Index controllers)
	{
		"/interaction_profiles/valve/index_controller", "",
		{
			action_id::grip_pose, action_id::aim_pose, action_id::haptic,
			action_id::trigger_analog, action_id::trigger_click, action_id::squeeze_analog,
			action_id::thumbstick, action_id::thumbstick_click, action_id::thumbstick_touch,
			action_id::trackpad, action_id::trackpad_touch,
			action_id::a_button, action_id::b_button
		},
		{},
		{}
	},
	// HTC Vive and Vive Pro wands
	{
		"/interaction_profiles/htc/vive_controller", "",
		{
			action_id::grip_pose, action_id::aim_pose, action_id::haptic,
			action_id::trigger_analog, action_id::trigger_click, action_id::squeeze_click,
			action_id::trackpad, action_id::trackpad_click, action_id::trackpad_touch,
			action_id::menu_button
		},
		{},
		{}
	},
	// HTC Vive Cosmos
	{
		"/interaction_profiles/htc/vive_cosmos_controller", "XR_HTC_vive_cosmos_controller_interaction",
		{
			action_id::grip_pose, action_id::aim_pose, action_id::haptic,
			action_id::trigger_analog, action_id::trigger_click, action_id::squeeze_click,
			action_id::thumbstick, action_id::thumbstick_click, action_id::thumbstick_touch
		},
		htc_left_hand,
		htc_right_hand
	},
	// HTC Vive Focus 3 and XR Elite
	{
		"/interaction_profiles/htc/vive_focus3_controller", "XR_HTC_vive_focus3_controller_interaction",
		{
			action_id::grip_pose, action_id::aim_pose, action_id::haptic,
			action_id::trigger_analog, action_id::trigger_click, action_id::squeeze_click,
			action_id::thumbstick, action_id::thumbstick_click, action_id::thumbstick_touch
		},
		htc_left_hand,
		htc_right_hand
	},
	// Windows Mixed Reality (Lenovo, Dell, Acer, Samsung Odyssey etc.)
	{
		"/interaction_profiles/microsoft/motion_controller", "",
		wmr_both_hands,
		{},
		{}
	},
	// Samsung Odyssey
	{
		"/interaction_profiles/samsung/odyssey_controller", "XR_EXT_samsung_odyssey_controller",
		wmr_both_hands,
		{},
		{}
	},
	// HP Reverb G2
	{
		"/interaction_profiles/hp/mixed_reality_controller", "XR_EXT_hp_mixed_reality_controller",
		{
			action_id::grip_pose, action_id::aim_pose, action_id::haptic,
			action_id::trigger_analog, action_id::squeeze_analog,
			action_id::thumbstick, action_id::thumbstick_click,
			action_id::menu_button
		},
		{ action_id::x_button, action_id::y_button },
		{ action_id::a_button, action_id::b_button }
	},
	// Pico 4
	{
		"/interaction_profiles/bytedance/pico4_controller", "XR_BD_controller_interaction",
		{
			action_id::grip_pose, action_id::aim_pose, action_id::haptic,
			action_id::trigger_analog, action_id::trigger_click,
			action_id::squeeze_analog, action_id::squeeze_click,
			action_id::thumbstick, action_id::thumbstick_click, action_id::thumbstick_touch
		},
		{ action_id::x_button, action_id::y_button, action_id::menu_button },
		{ action_id::a_button, action_id::b_button }
	},
	// Pico Neo 3
	{
		"/interaction_profiles/bytedance/pico_neo3_controller", "XR_BD_controller_interaction",
		{
			action_id::grip_pose, action_id::aim_pose, action_id::haptic,
			action_id::trigger_analog, action_id::trigger_click,
			action_id::squeeze_analog, action_id::squeeze_click,
			action_id::thumbstick, action_id::thumbstick_click, action_id::thumbstick_touch,
			action_id::menu_button
		},
		{ action_id::x_button, action_id::y_button },
		{ action_id::a_button, action_id::b_button }
	},
};

std::set<std::string> input::get_profile_extensions(const std::set<std::string>& supported_extensions)
{
	std::set<std::string> extensions;

	for (const interaction_profile& profile_info : interaction_profiles)
	{
		if (profile_info.extension.empty() || extensions.contains(profile_info.extension))
		{
			continue;
		}

		if (!supported_extensions.contains(profile_info.extension))
		{
			openxr_log.notice("The OpenXR runtime does not support '%s'", profile_info.extension);
			continue;
		}

		extensions.insert(profile_info.extension);
	}

	return extensions;
}

bool input::init(XrInstance instance, XrSession session, const std::set<std::string>& enabled_extensions)
{
	m_session = session;

	// Every step logs its own errors
	return create_actions(instance) &&
		suggest_bindings(instance, enabled_extensions) &&
		attach_actions() &&
		create_spaces();
}

void input::destroy()
{
	for (controller& ctrl : m_controllers)
	{
		for (auto& [id, pose] : ctrl.pose_spaces)
		{
			if (pose.space)
			{
				xrDestroySpace(pose.space);
			}
		}

		ctrl = {};
	}

	// This also destroys its actions, which are shared between both controllers
	if (m_action_set)
	{
		xrDestroyActionSet(m_action_set);
		m_action_set = XR_NULL_HANDLE;
	}

	m_actions = {};
	m_session = XR_NULL_HANDLE;
	m_applied_vibration = {};

	clear_states();
}

void input::update(XrSpace reference_space, XrTime predicted_time)
{
	if (!m_action_set || !sync_actions())
	{
		return;
	}

	for (u32 i = 0; i < m_controllers.size(); i++)
	{
		update_hand(reference_space, predicted_time, m_controllers[i]);
		apply_vibration(i, m_controllers[i]);
	}

	std::lock_guard lock(m_states_mutex);
	for (u32 i = 0; i < m_controllers.size(); i++)
	{
		m_states[i] = m_controllers[i].state;
	}
}

std::array<controller_state, 2> input::get_states() const
{
	std::lock_guard lock(m_states_mutex);
	return m_states;
}

bool input::create_actions(XrInstance instance)
{
	// Action set
	XrActionSetCreateInfo set_info{XR_TYPE_ACTION_SET_CREATE_INFO};
	strcpy_trunc(set_info.actionSetName, "move");
	strcpy_trunc(set_info.localizedActionSetName, "Move");

	if (XR_FAILED(xrCreateActionSet(instance, &set_info, &m_action_set)))
	{
		openxr_log.error("xrCreateActionSet failed");
		return false;
	}

	// Subaction paths
	std::array<XrPath, 2> paths{};
	for (u32 i = 0; i < m_controllers.size(); i++)
	{
		if (XR_FAILED(xrStringToPath(instance, get_hand_path(i).c_str(), &paths[i])))
		{
			openxr_log.error("xrStringToPath failed for '%s'", get_hand_path(i));
			return false;
		}

		m_controllers[i].subaction_path = paths[i];
	}

	// Create actions (each can be used for both hands)
	for (const auto& [id, opt] : generic_input_options)
	{
		XrActionCreateInfo info{};
		info.type = XR_TYPE_ACTION_CREATE_INFO;
		info.actionType = opt.action_type;
		strcpy_trunc(info.actionName, opt.name);
		strcpy_trunc(info.localizedActionName, opt.localized_name);
		info.countSubactionPaths = static_cast<u32>(paths.size());
		info.subactionPaths = paths.data();

		if (XR_FAILED(xrCreateAction(m_action_set, &info, &m_actions[id])))
		{
			openxr_log.error("xrCreateAction failed (name='%s')", opt.name);
			return false;
		}
	}

	return true;
}

bool input::suggest_bindings(XrInstance instance, const std::set<std::string>& enabled_extensions)
{
	bool found_profile = false;

	for (const interaction_profile& profile_info : interaction_profiles)
	{
		if (!profile_info.extension.empty() && !enabled_extensions.contains(profile_info.extension))
		{
			continue;
		}

		XrPath profile;
		if (XR_FAILED(xrStringToPath(instance, profile_info.path.c_str(), &profile)))
		{
			openxr_log.error("xrStringToPath failed for '%s'", profile_info.path);
			continue;
		}

		std::vector<XrActionSuggestedBinding> bindings;

		for (u32 i = 0; i < m_controllers.size(); i++)
		{
			const auto add_binding = [&](action_id id)
			{
				const input_option& opt = ::at32(generic_input_options, id);
				const std::string path = opt.get_path(i);

				XrActionSuggestedBinding binding{};
				binding.action = m_actions[id];

				if (XR_FAILED(xrStringToPath(instance, path.c_str(), &binding.binding)))
				{
					openxr_log.error("xrStringToPath failed for '%s'", path);
					return;
				}

				bindings.push_back(binding);
			};

			for (action_id id : profile_info.both_hands)
			{
				add_binding(id);
			}

			for (action_id id : profile_info.get_hand_actions(i))
			{
				add_binding(id);
			}
		}

		if (bindings.empty())
		{
			continue;
		}

		XrInteractionProfileSuggestedBinding sugg{};
		sugg.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
		sugg.interactionProfile = profile;
		sugg.suggestedBindings = bindings.data();
		sugg.countSuggestedBindings = static_cast<u32>(bindings.size());

		if (XR_FAILED(xrSuggestInteractionProfileBindings(instance, &sugg)))
		{
			openxr_log.error("xrSuggestInteractionProfileBindings failed for '%s'", profile_info.path);
			continue;
		}

		found_profile = true;
	}

	return found_profile;
}

bool input::create_spaces()
{
	bool found_space = false;

	for (controller& ctrl : m_controllers)
	{
		for (const auto& [id, opt] : generic_input_options)
		{
			if (opt.action_type != XR_ACTION_TYPE_POSE_INPUT) continue;

			XrActionSpaceCreateInfo info{};
			info.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
			info.action = m_actions[id];
			info.subactionPath = ctrl.subaction_path;
			info.poseInActionSpace.orientation.w = 1.0f; // Identity pose. A zero quaternion is invalid (XR_ERROR_POSE_INVALID).

			XrSpace space {XR_NULL_HANDLE};

			if (const XrResult result = xrCreateActionSpace(m_session, &info, &space); XR_FAILED(result))
			{
				openxr_log.error("xrCreateActionSpace failed for '%s' (result=%d)", opt.name, static_cast<s32>(result));
				continue;
			}

			ctrl.pose_spaces[id].space = space;
			found_space = true;
		}
	}

	return found_space;
}

bool input::attach_actions()
{
	XrSessionActionSetsAttachInfo info{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
	info.countActionSets = 1;
	info.actionSets = &m_action_set;

	if (XR_FAILED(xrAttachSessionActionSets(m_session, &info)))
	{
		openxr_log.error("xrAttachSessionActionSets failed");
		return false;
	}

	return true;
}

void input::update_hand(XrSpace ref, XrTime time, controller& ctrl)
{
	for (const auto& [id, opt] : generic_input_options)
	{
		XrActionStateGetInfo info{XR_TYPE_ACTION_STATE_GET_INFO};
		info.action = m_actions[id];
		info.subactionPath = ctrl.subaction_path;

		switch (opt.action_type)
		{
		case XrActionType::XR_ACTION_TYPE_BOOLEAN_INPUT:
		{
			XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
			if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &info, &state)))
			{
				ctrl.state.booleans[id] = !!state.currentState;
			}
			break;
		}
		case XrActionType::XR_ACTION_TYPE_FLOAT_INPUT:
		{
			XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
			if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &info, &state)))
			{
				ctrl.state.floats[id] = state.currentState;
			}
			break;
		}
		case XrActionType::XR_ACTION_TYPE_VECTOR2F_INPUT:
		{
			XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};
			if (XR_SUCCEEDED(xrGetActionStateVector2f(m_session, &info, &state)))
			{
				ctrl.state.vectors2f[id] = state.currentState;
			}
			break;
		}
		case XrActionType::XR_ACTION_TYPE_POSE_INPUT:
		{
			const auto it = ctrl.pose_spaces.find(id);
			if (it == ctrl.pose_spaces.end()) break;

			pose_space& space = it->second;
			pose_state& pose = ctrl.state.poses[id];

			XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
			pose.is_active = XR_SUCCEEDED(xrGetActionStatePose(m_session, &info, &state)) && state.isActive;
			pose.is_valid = false;

			if (pose.is_active)
			{
				// The velocities are used to simulate motion sensors
				XrSpaceVelocity velocity{XR_TYPE_SPACE_VELOCITY};
				XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
				loc.next = &velocity;

				if (XR_FAILED(xrLocateSpace(space.space, ref, time, &loc)))
				{
					openxr_log.error("xrLocateSpace failed");
				}
				else
				{
					pose.is_valid = (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) &&
					                (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT);

					if (pose.is_valid)
					{
						pose.pose = loc.pose;
						space.update_motion(pose, velocity, time);
					}
				}
			}

			if (!pose.is_valid)
			{
				pose.has_motion = false;
				pose.accelerometer = {};
				pose.gyro = {};
				space.reset_motion();
			}
			break;
		}
		case XrActionType::XR_ACTION_TYPE_VIBRATION_OUTPUT:
		{
			// Outputs have no state (see apply_vibration)
			break;
		}
		default:
			fmt::throw_exception("Unexpected action type 0x%x", static_cast<u32>(opt.action_type));
		}
	}
}

void input::apply_vibration(u32 hand, const controller& ctrl)
{
	const f32 amplitude = m_vibration[hand].load();
	const auto now = steady_clock::now();

	XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};
	info.action = m_actions[action_id::haptic];
	info.subactionPath = ctrl.subaction_path;

	if (amplitude <= 0.0f)
	{
		if (m_applied_vibration[hand] > 0.0f)
		{
			xrStopHapticFeedback(m_session, &info);
			m_applied_vibration[hand] = 0.0f;
		}
		return;
	}

	// The vibration stops by itself after its duration, so we have to renew it while it's active
	if (amplitude == m_applied_vibration[hand] && now - m_last_vibration_time[hand] < vibration_renew_interval)
	{
		return;
	}

	XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
	vibration.amplitude = std::clamp(amplitude, 0.0f, 1.0f);
	vibration.duration = vibration_duration_ns;
	vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

	if (XR_FAILED(xrApplyHapticFeedback(m_session, &info, reinterpret_cast<const XrHapticBaseHeader*>(&vibration))))
	{
		openxr_log.error("xrApplyHapticFeedback failed");
	}

	m_applied_vibration[hand] = amplitude;
	m_last_vibration_time[hand] = now;
}

void input::set_vibration(u32 hand, f32 amplitude)
{
	ensure(hand < m_vibration.size());
	m_vibration[hand] = amplitude;
}

void input::clear_states()
{
	std::lock_guard lock(m_states_mutex);
	m_states = {};
}

bool input::sync_actions() const
{
	XrActiveActionSet active{};
	active.actionSet = m_action_set;

	XrActionsSyncInfo sync{};
	sync.type = XR_TYPE_ACTIONS_SYNC_INFO;
	sync.countActiveActionSets = 1;
	sync.activeActionSets = &active;

	if (XR_FAILED(xrSyncActions(m_session, &sync)))
	{
		openxr_log.error("xrSyncActions failed");
		return false;
	}

	return true;
}

// Rotates a vector from the reference space into the frame of a pose with this orientation
static XrVector3f rotate_into_pose(const XrQuaternionf& q, const XrVector3f& v)
{
	// Rotate by the conjugate: v' = v + w * t + cross(u, t) with u = -q.xyz and t = 2 * cross(u, v)
	const XrVector3f u{ -q.x, -q.y, -q.z };
	const XrVector3f t{
		2.0f * (u.y * v.z - u.z * v.y),
		2.0f * (u.z * v.x - u.x * v.z),
		2.0f * (u.x * v.y - u.y * v.x)
	};

	return {
		v.x + q.w * t.x + (u.y * t.z - u.z * t.y),
		v.y + q.w * t.y + (u.z * t.x - u.x * t.z),
		v.z + q.w * t.z + (u.x * t.y - u.y * t.x)
	};
}

void input::pose_space::update_motion(pose_state& state, const XrSpaceVelocity& velocity, XrTime time)
{
	constexpr f32 standard_gravity = 9.80665f; // m/s²
	constexpr f32 max_acceleration = 4.0f * standard_gravity; // Roughly what a DS3 can measure
	constexpr f32 acceleration_alpha = 0.5f; // Differentiating the velocity is noisy

	// Linear acceleration from the change of the linear velocity
	if (velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
	{
		if (has_last_velocity && time > last_velocity_time)
		{
			const f32 dt = static_cast<f32>(time - last_velocity_time) / 1'000'000'000.0f;
			const auto get_acceleration = [&](f32 now, f32 last, f32 filtered)
			{
				const f32 value = std::clamp((now - last) / dt, -max_acceleration, max_acceleration);
				return filtered * (1.0f - acceleration_alpha) + value * acceleration_alpha;
			};

			linear_acceleration.x = get_acceleration(velocity.linearVelocity.x, last_linear_velocity.x, linear_acceleration.x);
			linear_acceleration.y = get_acceleration(velocity.linearVelocity.y, last_linear_velocity.y, linear_acceleration.y);
			linear_acceleration.z = get_acceleration(velocity.linearVelocity.z, last_linear_velocity.z, linear_acceleration.z);
		}

		last_linear_velocity = velocity.linearVelocity;
		last_velocity_time = time;
		has_last_velocity = true;
	}
	else
	{
		reset_motion();
	}

	// An accelerometer measures the acceleration minus gravity. The reference space has +Y up, so gravity is -Y.
	const XrVector3f specific_force{
		linear_acceleration.x / standard_gravity,
		linear_acceleration.y / standard_gravity + 1.0f,
		linear_acceleration.z / standard_gravity
	};

	state.accelerometer = rotate_into_pose(state.pose.orientation, specific_force);

	// The runtime reports the angular velocity in the reference space
	if (velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
	{
		state.gyro = rotate_into_pose(state.pose.orientation, velocity.angularVelocity);
	}
	else
	{
		state.gyro = {};
	}

	state.has_motion = true;
}

void input::pose_space::reset_motion()
{
	linear_acceleration = {};
	has_last_velocity = false;
}
}
