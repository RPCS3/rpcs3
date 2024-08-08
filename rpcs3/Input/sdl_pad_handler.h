#pragma once

#ifdef HAVE_SDL2

#include "Emu/Io/PadHandler.h"
#include "SDL.h"

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
		SDL_GameController* game_controller = nullptr;
		SDL_GameControllerType type = SDL_GameControllerType::SDL_CONTROLLER_TYPE_UNKNOWN;
		SDL_Joystick* joystick = nullptr;
		SDL_JoystickPowerLevel power_level = SDL_JoystickPowerLevel::SDL_JOYSTICK_POWER_UNKNOWN;
		SDL_JoystickPowerLevel last_power_level = SDL_JoystickPowerLevel::SDL_JOYSTICK_POWER_UNKNOWN;

		std::string name;
		std::string path;
		std::string serial;
		u16 vid = 0;
		u16 pid = 0;
		u16 product_version = 0;
		u16 firmware_version = 0;

		bool is_virtual_device = false;

		bool has_led = false;
		bool has_rumble = false;
		bool has_rumble_triggers = false;
		bool has_accel = false;
		bool has_gyro = false;

		float data_rate_accel = 0.0f;
		float data_rate_gyro = 0.0f;

		std::set<SDL_GameControllerButton> button_ids;
		std::set<SDL_GameControllerAxis> axis_ids;

		std::vector<touchpad> touchpads;
	};

	sdl_info sdl{};

	std::array<float, 3> values_accel{};
	std::array<float, 3> values_gyro{};

	bool led_needs_update = true;
	bool led_is_on = true;
	bool led_is_blinking = false;
	steady_clock::time_point led_timestamp{};

	bool has_new_rumble_data = true;
	steady_clock::time_point last_vibration{};
};

class sdl_pad_handler : public PadHandlerBase
{
	enum SDLKeyCodes
	{
		None = 0,

		A,
		B,
		X,
		Y,
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
		Paddle1,
		Paddle2,
		Paddle3,
		Paddle4,
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
		RSYPos
	};

public:
	sdl_pad_handler();
	~sdl_pad_handler();

	SDLDevice::sdl_info get_sdl_info(int i);

	bool Init() override;
	void process() override;
	void init_config(cfg_pad* cfg) override;
	std::vector<pad_list_entry> list_devices() override;
	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void get_motion_sensors(const std::string& pad_id, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors) override;
	connection get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, bool first_call, bool get_blacklist, const std::vector<std::string>& buttons) override;

private:
	// pseudo 'controller id' to keep track of unique controllers
	std::map<std::string, std::shared_ptr<SDLDevice>> m_controllers;

	void enumerate_devices();
	std::shared_ptr<SDLDevice> get_device_by_game_controller(SDL_GameController* game_controller) const;

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

	u32 get_battery_color(SDL_JoystickPowerLevel power_level, u32 brightness) const;
	void set_rumble(SDLDevice* dev, u8 speed_large, u8 speed_small);

	static std::string button_to_string(SDL_GameControllerButton button);
	static std::string axis_to_string(SDL_GameControllerAxis axis);

	static SDLKeyCodes get_button_code(SDL_GameControllerButton button);
};

#endif
