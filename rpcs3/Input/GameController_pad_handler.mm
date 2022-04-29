

#include <unordered_map>
#include <chrono>
#include "xinput_pad_handler.h"
#include "Emu/Io/pad_config.h"
#import <GameController/GameController.h>

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
