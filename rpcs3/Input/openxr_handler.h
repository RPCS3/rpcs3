#pragma once

#include "Emu/Io/PadHandler.h"
#include "Emu/Io/openxr/openxr_input.h"

// Uses VR controllers as a gamepad.
// Device #1 combines both controllers, #2 is the left controller and #3 is the right controller.
class openxr_handler final : public PadHandlerBase
{
	// The inputs of one controller. Each controller has its own key codes (see get_key_code).
	enum class key_id : u32
	{
		trigger,
		trigger_click,
		grip,
		grip_click,
		stick_left,
		stick_right,
		stick_down,
		stick_up,
		stick_click,
		stick_touch,
		trackpad_left,
		trackpad_right,
		trackpad_down,
		trackpad_up,
		trackpad_click,
		trackpad_touch,
		a,
		b,
		x,
		y,
		menu,
		select,

		count
	};

	enum hand : u32
	{
		left_hand = 0,
		right_hand = 1,
	};

	struct openxr_device : public PadDevice
	{
		std::array<bool, 2> uses_hand{}; // Which controllers belong to this device
		std::array<openxr::controller_state, 2> states{};

		// Applied to the PS Move orientation. Set when a game resets the orientation (see cellGemSetYaw).
		ps_move_data::vect<4> orientation_offset = ps_move_data::default_quaternion;
	};

	void update_move_data(openxr_device& dev, const openxr::pose_state& pose, ps_move_data& move_data) const;

	static constexpr u32 key_none = 0;

	static constexpr u32 get_key_code(hand h, key_id key)
	{
		return 1 + static_cast<u32>(h) * static_cast<u32>(key_id::count) + static_cast<u32>(key);
	}

public:
	openxr_handler();

	std::vector<pad_list_entry> list_devices() override;
	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	void init_config(cfg_pad* cfg) override;

private:
	int get_device_number(const std::string& pad_id) const;
	static std::array<bool, 2> get_uses_hand(int device_number);
	static bool is_stick(hand h, u32 keyCode);
	void set_vibration(const std::array<bool, 2>& uses_hand, u8 large_motor, u8 small_motor) const;

	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
	std::unordered_map<u32, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u32, u16>& data, const std::vector<std::string>& buttons) override;
};
