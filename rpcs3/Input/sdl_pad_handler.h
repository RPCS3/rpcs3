#pragma once

#ifdef HAVE_SDL3

#include "Emu/Io/PadHandler.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif


class SDLDevice : public PadDevice
{
public:

	struct touch_point
	{
		int index = 0;
		int x = 0;
		int y = 0;
	};

	struct touchpad
	{
		int index = 0;
		std::vector<touch_point> fingers;
	};

	struct sdl_info
	{
		SDL_Gamepad* gamepad = nullptr;
		SDL_GamepadType type = SDL_GamepadType::SDL_GAMEPAD_TYPE_UNKNOWN;
		SDL_GamepadType real_type = SDL_GamepadType::SDL_GAMEPAD_TYPE_UNKNOWN;
		SDL_GUID guid {};
		int power_level = 0;
		int last_power_level = 0;

		std::string name;
		std::string path;
		std::string serial;
		u16 vid = 0;
		u16 pid = 0;
		u16 product_version = 0;
		u16 firmware_version = 0;

		bool is_virtual_device = false;
		bool is_ds3_with_pressure_buttons = false;

		bool has_led = false;
		bool has_mono_led = false;
		bool has_player_led = false;
		bool has_rumble = false;
		bool has_rumble_triggers = false;
		bool has_accel = false;
		bool has_gyro = false;

		f32 data_rate_accel = 0.0f;
		f32 data_rate_gyro = 0.0f;

		std::set<SDL_GamepadButton> button_ids;
		std::set<SDL_GamepadAxis> axis_ids;

		std::vector<touchpad> touchpads;
	};

	sdl_info sdl{};

	std::array<f32, 3> values_accel{};
	std::array<f32, 3> values_gyro{};

	bool led_needs_update = true;
	bool led_is_on = true;
	bool led_is_blinking = false;
	steady_clock::time_point led_timestamp{};
};

class sdl_pad_handler : public PadHandlerBase
{
	enum SDLKeyCodes
	{
		None = 0,

		South,
		East,
		West,
		North,
		Left,
		Right,
		Up,
		Down,
		LB,
		RB,
		LS,
		RS,
		Start,
		Back,
		Guide,
		Misc1,
		Misc2,
		Misc3,
		Misc4,
		Misc5,
		Misc6,
		RPaddle1,
		LPaddle1,
		RPaddle2,
		LPaddle2,
		Touchpad,

		Touch_L,
		Touch_R,
		Touch_U,
		Touch_D,

		LT,
		RT,

		LSXNeg,
		LSXPos,
		LSYNeg,
		LSYPos,
		RSXNeg,
		RSXPos,
		RSYNeg,
		RSYPos,

		// DS3 Pressure sensitive buttons (reported as axis)
		PressureBegin,
		PressureCross,    // Cross        axis 6
		PressureCircle,   // Circle       axis 7
		PressureSquare,   // Square       axis 8
		PressureTriangle, // Triangle     axis 9
		PressureL1,       // L1           axis 10
		PressureR1,       // R1           axis 11
		PressureUp,       // D-Pad Up     axis 12
		PressureDown,     // D-Pad Down   axis 13
		PressureLeft,     // D-Pad Left   axis 14
		PressureRight,    // D-Pad Right  axis 15
		PressureEnd,
	};

public:
	sdl_pad_handler();
	~sdl_pad_handler();

	SDLDevice::sdl_info get_sdl_info(SDL_JoystickID id);

	bool Init() override;
	void process() override;
	void init_config(cfg_pad* cfg) override;
	std::vector<pad_list_entry> list_devices() override;
	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void get_motion_sensors(const std::string& pad_id, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors) override;
	connection get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, gui_call_type call_type, const std::vector<std::string>& buttons) override;

private:
	// pseudo 'controller id' to keep track of unique controllers
	std::map<std::string, std::shared_ptr<SDLDevice>> m_controllers;

	void enumerate_devices();
	std::shared_ptr<SDLDevice> get_device_by_gamepad(SDL_Gamepad* gamepad) const;

	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_touch_pad_motion(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;

	u32 get_battery_color(int power_level, u32 brightness) const;
	void set_rumble(SDLDevice* dev, u8 speed_large, u8 speed_small);

	static std::string button_to_string(SDL_GamepadButton button);
	static std::string axis_to_string(SDL_GamepadAxis axis);

	static SDLKeyCodes get_button_code(SDL_GamepadButton button);
};

#endif
