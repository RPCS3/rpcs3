

#include <unordered_map>
#include <chrono>
#include "rpcs3/Input/GameController_pad_handler.h"
#include "Emu/Io/pad_config.h"
#import <GameController/GameController.h>
#import <CoreHaptics/CoreHaptics.h>

class GameController_pad_handler final : public PadHandlerBase
{
	struct GameControllerDevice : public PadDevice
	{
		GCController *deviceObject{ nil };
		bool newVibrateData{ true };
		u16 largeVibrate{ 0 };
		u16 smallVibrate{ 0 };
		steady_clock::time_point last_vibration;
		bool is_sony_device{ false };
// 		DWORD state{ ERROR_NOT_CONNECTED }; // holds internal controller state change
// 		SCP_EXTN state_scp{};
// 		XINPUT_STATE state_base{};
	};
public:
	GameController_pad_handler();
	~GameController_pad_handler();

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	void SetPadData(const std::string& padId, u8 player_id, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(cfg_pad* cfg) override;

	using PadButtonValues = std::unordered_map<u64, u16>;

private:
	bool is_init{ false };
	
	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(u64 keyCode) override;
	bool get_is_right_trigger(u64 keyCode) override;
	bool get_is_left_stick(u64 keyCode) override;
	bool get_is_right_stick(u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	void apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
};

std::shared_ptr<PadHandlerBase> create_GameController_handler()
{
	return std::make_shared<GameController_pad_handler>();
}

void handle_GameController_pad_config(cfg_pad& cfg, std::shared_ptr<PadHandlerBase>& 
									  handler)
{
	static_cast<GameController_pad_handler*>(handler.get())->init_config(&cfg);
}


std::vector<std::string> GameController_pad_handler::ListDevices() 
{
	std::vector<std::string> GC_pads_list;
	auto gcController = [GCController controllers];

	return GC_pads_list;
}

GameController_pad_handler::GameController_pad_handler() : PadHandlerBase(pad_handler::gamecontroller)
{
	init_configs();

}

GameController_pad_handler::~GameController_pad_handler()
{
	
}

void GameController_pad_handler::SetPadData(const std::string& padId, u8 /*player_id*/, u32 largeMotor, u32 smallMotor, s32/* r*/, s32/* g*/, s32/* b*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	
}

std::shared_ptr<PadDevice> GameController_pad_handler::get_device(const std::string& device)
{
	return nullptr;
}

bool GameController_pad_handler::get_is_left_trigger(u64 keyCode)
{
// 	return keyCode == XInputKeyCodes::LT;
	return false;
}

bool GameController_pad_handler::get_is_right_trigger(u64 keyCode)
{
// 	return keyCode == XInputKeyCodes::RT;
	return false;
}

bool GameController_pad_handler::get_is_left_stick(u64 keyCode)
{
// 	switch (keyCode)
// 	{
// 	case XInputKeyCodes::LSXNeg:
// 	case XInputKeyCodes::LSXPos:
// 	case XInputKeyCodes::LSYPos:
// 	case XInputKeyCodes::LSYNeg:
// 		return true;
// 	default:
// 		return false;
// 	}
	return false;
}

bool GameController_pad_handler::get_is_right_stick(u64 keyCode)
{
// 	switch (keyCode)
// 	{
// 	case XInputKeyCodes::RSXNeg:
// 	case XInputKeyCodes::RSXPos:
// 	case XInputKeyCodes::RSYPos:
// 	case XInputKeyCodes::RSYNeg:
// 		return true;
// 	default:
// 		return false;
// 	}
	return false;
}

std::unordered_map<u64, u16> GameController_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	PadButtonValues values;
	GameControllerDevice* dev = static_cast<GameControllerDevice*>(device.get());
	if (!dev /*|| dev->state != ERROR_SUCCESS*/ ) // the state has to be aquired with update_connection before calling this function
		return values;

	// Try SCP first, if it fails for that pad then try normal XInput
// 	if (dev->is_scp_device)
// 	{
// 		return get_button_values_scp(dev->state_scp);
// 	}
// 
// 	return get_button_values_base(dev->state_base);
	return values;
}

u32 GameController_pad_handler::get_battery_level(const std::string& padId)
{
// 	const int device_number = GetDeviceNumber(padId);
// 	if (device_number < 0)
// 		return 0;

	// Receive Battery Info. If device is not on cable, get battery level, else assume full.
// 	XINPUT_BATTERY_INFORMATION battery_info;
// 	(*xinputGetBatteryInformation)(device_number, BATTERY_DEVTYPE_GAMEPAD, &battery_info);
// 
// 	switch (battery_info.BatteryType)
// 	{
// 	case BATTERY_TYPE_DISCONNECTED:
// 		return 0;
// 	case BATTERY_TYPE_WIRED:
// 		return 100;
// 	default:
// 		break;
// 	}
// 
// 	switch (battery_info.BatteryLevel)
// 	{
// 	case BATTERY_LEVEL_EMPTY:
// 		return 0;
// 	case BATTERY_LEVEL_LOW:
// 		return 33;
// 	case BATTERY_LEVEL_MEDIUM:
// 		return 66;
// 	case BATTERY_LEVEL_FULL:
// 		return 100;
// 	default:
// 		return 0;
// 	}
	return 0;
}


void GameController_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
// 	cfg->ls_left.def  = button_list.at(XInputKeyCodes::LSXNeg);
// 	cfg->ls_down.def  = button_list.at(XInputKeyCodes::LSYNeg);
// 	cfg->ls_right.def = button_list.at(XInputKeyCodes::LSXPos);
// 	cfg->ls_up.def    = button_list.at(XInputKeyCodes::LSYPos);
// 	cfg->rs_left.def  = button_list.at(XInputKeyCodes::RSXNeg);
// 	cfg->rs_down.def  = button_list.at(XInputKeyCodes::RSYNeg);
// 	cfg->rs_right.def = button_list.at(XInputKeyCodes::RSXPos);
// 	cfg->rs_up.def    = button_list.at(XInputKeyCodes::RSYPos);
// 	cfg->start.def    = button_list.at(XInputKeyCodes::Start);
// 	cfg->select.def   = button_list.at(XInputKeyCodes::Back);
// 	cfg->ps.def       = button_list.at(XInputKeyCodes::Guide);
// 	cfg->square.def   = button_list.at(XInputKeyCodes::X);
// 	cfg->cross.def    = button_list.at(XInputKeyCodes::A);
// 	cfg->circle.def   = button_list.at(XInputKeyCodes::B);
// 	cfg->triangle.def = button_list.at(XInputKeyCodes::Y);
// 	cfg->left.def     = button_list.at(XInputKeyCodes::Left);
// 	cfg->down.def     = button_list.at(XInputKeyCodes::Down);
// 	cfg->right.def    = button_list.at(XInputKeyCodes::Right);
// 	cfg->up.def       = button_list.at(XInputKeyCodes::Up);
// 	cfg->r1.def       = button_list.at(XInputKeyCodes::RB);
// 	cfg->r2.def       = button_list.at(XInputKeyCodes::RT);
// 	cfg->r3.def       = button_list.at(XInputKeyCodes::RS);
// 	cfg->l1.def       = button_list.at(XInputKeyCodes::LB);
// 	cfg->l2.def       = button_list.at(XInputKeyCodes::LT);
// 	cfg->l3.def       = button_list.at(XInputKeyCodes::LS);
// 
// 	cfg->pressure_intensity_button.def = button_list.at(XInputKeyCodes::None);
// 
// 	// Set default misc variables
// 	cfg->lstickdeadzone.def    = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;  // between 0 and 32767
// 	cfg->rstickdeadzone.def    = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE; // between 0 and 32767
// 	cfg->ltriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
// 	cfg->rtriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
	cfg->lpadsquircling.def    = 8000;
	cfg->rpadsquircling.def    = 8000;

	// apply defaults
	cfg->from_default();
}

pad_preview_values GameController_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
{
	return {
// 		data.at(LT),
// 		data.at(RT),
// 		data.at(LSXPos) - data.at(LSXNeg),
// 		data.at(LSYPos) - data.at(LSYNeg),
// 		data.at(RSXPos) - data.at(RSXNeg),
// 		data.at(RSYPos) - data.at(RSYNeg)
	};
}

bool GameController_pad_handler::Init()
{
	if (is_init)
		return true;

	if (!is_init)
		return false;

	return true;
}

PadHandlerBase::connection GameController_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	GameControllerDevice* dev = static_cast<GameControllerDevice*>(device.get());
	if (!dev)
		return connection::disconnected;

// 	dev->state = ERROR_NOT_CONNECTED;
// 	dev->state_scp = {};
// 	dev->state_base = {};
// 
	// Try SCP first, if it fails for that pad then try normal XInput
// 	if (xinputGetExtended)
// 		dev->state = xinputGetExtended(dev->deviceNumber, &dev->state_scp);
// 
// 	dev->is_scp_device = dev->state == ERROR_SUCCESS;
// 
// 	if (!dev->is_scp_device)
// 		dev->state = xinputGetState(dev->deviceNumber, &dev->state_base);
// 
// 	if (dev->state == ERROR_SUCCESS)
// 		return connection::connected;

	return connection::disconnected;
}

void GameController_pad_handler::get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	GameControllerDevice* dev = static_cast<GameControllerDevice*>(device.get());
	if (!dev || !pad)
		return;

// 	const auto padnum = dev->deviceNumber;

	// Receive Battery Info. If device is not on cable, get battery level, else assume full
// 	XINPUT_BATTERY_INFORMATION battery_info;
// 	(*xinputGetBatteryInformation)(padnum, BATTERY_DEVTYPE_GAMEPAD, &battery_info);
// 	pad->m_cable_state = battery_info.BatteryType == BATTERY_TYPE_WIRED ? 1 : 0;
// 	pad->m_battery_level = pad->m_cable_state ? BATTERY_LEVEL_FULL : battery_info.BatteryLevel;

	if (dev->deviceObject.motion != nil)
	{
		GCMotion *sensors = dev->deviceObject.motion;
		GCAcceleration accel = sensors.acceleration;
		pad->m_sensors[0].m_value = accel.x;
		pad->m_sensors[1].m_value = accel.y;
		pad->m_sensors[2].m_value = accel.z;
// 		pad->m_sensors[3].m_value = sensors.SCP_GYRO;
	}
}

void GameController_pad_handler::apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	GameControllerDevice* dev = static_cast<GameControllerDevice*>(device.get());

}
