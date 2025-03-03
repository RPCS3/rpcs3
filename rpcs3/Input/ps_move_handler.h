#pragma once

#include "hid_pad_handler.h"

#include <unordered_map>

namespace reports
{
	// NOTE: The 1st half-frame contains slightly older data than the 2nd half-frame
#pragma pack(push, 1)
	struct ps_move_input_report_common
	{
		//                                      ID    Size   Description
		u8 report_id{};                      // 0x00    1    HID Report ID (always 0x01)
		u8 buttons_1{};                      // 0x01    1    Buttons 1 (Start, Select)
		u8 buttons_2{};                      // 0x02    1    Buttons 2 (X, Square, Circle, Triangle)
		u8 buttons_3{};                      // 0x03    1+   Buttons 3 (PS, Move, T) and EXT
		u8 sequence_number{};                // 0x04    1-   Sequence number
		u8 trigger_1{};                      // 0x05    1    T button values (1st half-frame)
		u8 trigger_2{};                      // 0x06    1    T button values (2nd half-frame)
		u32 magic{};                         // 0x07    4    always 0x7F7F7F7F
		u8 timestamp_upper{};                // 0x0B    1    Timestamp (upper byte)
		u8 battery_level{};                  // 0x0C    1    Battery level. 0x05 = max, 0xEE = USB charging
		s16 accel_x_1{};                     // 0x0D    2    X-axis accelerometer (1st half-frame)
		s16 accel_y_1{};                     // 0x0F    2    Z-axis accelerometer (1st half-frame)
		s16 accel_z_1{};                     // 0x11    2    Y-axis accelerometer (1st half-frame)
		s16 accel_x_2{};                     // 0x13    2    X-axis accelerometer (2nd half-frame)
		s16 accel_y_2{};                     // 0x15    2    Z-axis accelerometer (2nd half-frame)
		s16 accel_z_2{};                     // 0x17    2    Y-axis accelerometer (2nd half-frame)
		s16 gyro_x_1{};                      // 0x19    2    X-axis gyroscope (1st half-frame)
		s16 gyro_y_1{};                      // 0x1B    2    Z-axis gyroscope (1st half-frame)
		s16 gyro_z_1{};                      // 0x1D    2    Y-axis gyroscope (1st half-frame)
		s16 gyro_x_2{};                      // 0x1F    2    X-axis gyroscope (2nd half-frame)
		s16 gyro_y_2{};                      // 0x21    2    Z-axis gyroscope (2nd half-frame)
		s16 gyro_z_2{};                      // 0x23    2    Y-axis gyroscope (2nd half-frame)
		u8 temperature{};                    // 0x25    1+   Temperature
		u8 magnetometer_x{};                 // 0x26    1+   Temperature + X-axis magnetometer
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct ps_move_input_report_ZCM1
	{
		ps_move_input_report_common common{};

		//                                      ID    Size   Description
		u8 magnetometer_x2{};                // 0x27    1-   X-axis magnetometer
		u8 magnetometer_y{};                 // 0x28    1+   Y-axis magnetometer
		u8 magnetometer_yz{};                // 0x29    1    YZ-axis magnetometer
		u8 magnetometer_z{};                 // 0x2A    1-   Z-axis magnetometer
		u8 timestamp_lower{};                // 0x2B    1    Timestamp (lower byte)
		std::array<u8, 5> ext_device_data{}; // 0x2C    5    External device data
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct ps_move_input_report_ZCM2
	{
		ps_move_input_report_common common{};

		//                                      ID    Size   Description
		u16 timestamp2;                      // 0x27    2    same as common timestamp
		u16 unk;                             // 0x29    2    Unknown
		u8 timestamp_lower;                  // 0x2B    1    Timestamp (lower byte)
	};
#pragma pack(pop)

	struct ps_move_output_report
	{
		u8 type{};
		u8 zero{};
		u8 r{};
		u8 g{};
		u8 b{};
		u8 zero2{};
		u8 rumble{};
		u8 padding[2];
	};

	struct ps_move_feature_report
	{
		std::array<u8, 4> data{}; // TODO
	};

	// Buffer size for calibration data
	constexpr u32 PSMOVE_CALIBRATION_SIZE = 49;

	// Three blocks, minus header (2 bytes) for blocks 2,3
	constexpr u32 PSMOVE_ZCM1_CALIBRATION_BLOB_SIZE = PSMOVE_CALIBRATION_SIZE * 3 - 2 * 2;

	// Three blocks, minus header (2 bytes) for block 2
	constexpr u32 PSMOVE_ZCM2_CALIBRATION_BLOB_SIZE = PSMOVE_CALIBRATION_SIZE * 2 - 2 * 1;

	struct ps_move_calibration_blob
	{
		std::array<u8, std::max(PSMOVE_ZCM1_CALIBRATION_BLOB_SIZE, PSMOVE_ZCM2_CALIBRATION_BLOB_SIZE)> data{};
	};
}

enum
{
	zero_shift = 0x8000,
};

enum class ps_move_model
{
	ZCM1, // PS3
	ZCM2, // PS4
};

struct ps_move_calibration
{
	bool is_valid = false;
	f32 accel_x_factor = 1.0f;
	f32 accel_y_factor = 1.0f;
	f32 accel_z_factor = 1.0f;
	f32 accel_x_offset = 0.0f;
	f32 accel_y_offset = 0.0f;
	f32 accel_z_offset = 0.0f;
	f32 gyro_x_gain = 1.0f;
	f32 gyro_y_gain = 1.0f;
	f32 gyro_z_gain = 1.0f;
	f32 gyro_x_offset = 0.0f;
	f32 gyro_y_offset = 0.0f;
	f32 gyro_z_offset = 0.0f;
};

class ps_move_device : public HidDevice
{
public:
	ps_move_model model = ps_move_model::ZCM1;
	reports::ps_move_input_report_ZCM1 input_report_ZCM1{};
	reports::ps_move_input_report_ZCM2 input_report_ZCM2{};
	reports::ps_move_output_report output_report{};
	reports::ps_move_output_report last_output_report{};
	steady_clock::time_point last_output_report_time;
	u32 external_device_id = 0;
	ps_move_calibration calibration{};

	const reports::ps_move_input_report_common& input_report_common() const;
};

class ps_move_handler final : public hid_pad_handler<ps_move_device>
{
	enum ps_move_key_codes
	{
		none = 0,
		cross,
		square,
		circle,
		triangle,
		start,
		select,
		ps,
		move,
		t,

		// Available through external sharpshooter
		firing_mode_1,
		firing_mode_2,
		firing_mode_3,
		reload,

		// Available through external racing wheel
		dpad_up,
		dpad_down,
		dpad_left,
		dpad_right,
		L1,
		R1,
		L2,
		R2,
		throttle,
		paddle_left,
		paddle_right,
	};

public:
	ps_move_handler();
	~ps_move_handler();

	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(cfg_pad* cfg) override;

private:
#ifdef _WIN32
	hid_device* connect_move_device(ps_move_device* device, std::string_view path);
#endif

	DataStatus get_data(ps_move_device* device) override;
	void check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view wide_serial) override;
	int send_output_report(ps_move_device* device) override;

	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;

	void handle_external_device(const pad_ensemble& binding);
};
