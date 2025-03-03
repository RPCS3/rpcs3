#pragma once

#include "hid_pad_handler.h"

#include <array>
#include <unordered_map>

namespace reports
{
	// Descriptor
	// 0x09, 0x05,        // Usage (0x05)
	// 0xA1, 0x01,        // Collection (Application)
	// 0x05, 0x09,        //   Usage Page (Button)
	// 0x19, 0x01,        //   Usage Minimum (0x01)
	// 0x29, 0x0D,        //   Usage Maximum (0x0D)
	// 0x15, 0x00,        //   Logical Minimum (0)
	// 0x25, 0x01,        //   Logical Maximum (1)
	// 0x75, 0x01,        //   Report Size (1)
	// 0x95, 0x0D,        //   Report Count (13)
	// 0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	// 0x75, 0x03,        //   Report Size (3)
	// 0x95, 0x01,        //   Report Count (1)
	// 0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	// 0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
	// 0x09, 0x39,        //   Usage (Hat switch)
	// 0x15, 0x00,        //   Logical Minimum (0)
	// 0x25, 0x07,        //   Logical Maximum (7)
	// 0x35, 0x00,        //   Physical Minimum (0)
	// 0x46, 0x3B, 0x01,  //   Physical Maximum (315)
	// 0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
	// 0x75, 0x04,        //   Report Size (4)
	// 0x95, 0x01,        //   Report Count (1)
	// 0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
	// 0x75, 0x04,        //   Report Size (4)
	// 0x95, 0x01,        //   Report Count (1)
	// 0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	// 0x09, 0x30,        //   Usage (X)
	// 0x09, 0x31,        //   Usage (Y)
	// 0x09, 0x32,        //   Usage (Z)
	// 0x09, 0x35,        //   Usage (Rz)
	// 0x15, 0x00,        //   Logical Minimum (0)
	// 0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	// 0x35, 0x00,        //   Physical Minimum (0)
	// 0x46, 0xFF, 0x00,  //   Physical Maximum (255)
	// 0x65, 0x00,        //   Unit (None)
	// 0x75, 0x08,        //   Report Size (8)
	// 0x95, 0x04,        //   Report Count (4)
	// 0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	// 0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
	// 0x09, 0x20,        //   Usage (0x20)
	// 0x09, 0x21,        //   Usage (0x21)
	// 0x09, 0x22,        //   Usage (0x22)
	// 0x09, 0x23,        //   Usage (0x23)
	// 0x09, 0x24,        //   Usage (0x24)
	// 0x09, 0x25,        //   Usage (0x25)
	// 0x09, 0x26,        //   Usage (0x26)
	// 0x09, 0x27,        //   Usage (0x27)
	// 0x09, 0x28,        //   Usage (0x28)
	// 0x09, 0x29,        //   Usage (0x29)
	// 0x09, 0x2A,        //   Usage (0x2A)
	// 0x09, 0x2B,        //   Usage (0x2B)
	// 0x15, 0x00,        //   Logical Minimum (0)
	// 0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	// 0x75, 0x08,        //   Report Size (8)
	// 0x95, 0x0C,        //   Report Count (12)
	// 0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	// 0x09, 0x2C,        //   Usage (0x2C)
	// 0x09, 0x2D,        //   Usage (0x2D)
	// 0x09, 0x2E,        //   Usage (0x2E)
	// 0x09, 0x2F,        //   Usage (0x2F)
	// 0x15, 0x00,        //   Logical Minimum (0)
	// 0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
	// 0x35, 0x00,        //   Physical Minimum (0)
	// 0x46, 0xFF, 0x03,  //   Physical Maximum (1023)
	// 0x75, 0x10,        //   Report Size (16)
	// 0x95, 0x04,        //   Report Count (4)
	// 0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	// 0x0A, 0x21, 0x26,  //   Usage (0x2621)
	// 0x15, 0x00,        //   Logical Minimum (0)
	// 0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	// 0x35, 0x00,        //   Physical Minimum (0)
	// 0x46, 0xFF, 0x00,  //   Physical Maximum (255)
	// 0x75, 0x08,        //   Report Size (8)
	// 0x95, 0x08,        //   Report Count (8)
	// 0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	// 0x0A, 0x21, 0x26,  //   Usage (0x2621)
	// 0x15, 0x00,        //   Logical Minimum (0)
	// 0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	// 0x75, 0x08,        //   Report Size (8)
	// 0x95, 0x08,        //   Report Count (8)
	// 0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	// 0xC0,              // End Collection

#pragma pack(push, 1)
	struct skateboard_input_report
	{
		// 13 buttons, value range 0 to 1, 13 bits + 3 bits padding (2 bytes total)
		u16 buttons{};

		// d-pad, value range 0 to 7 (8 directions), 4 bits + 4 bits padding (1 byte total)
		u8 d_pad{};

		// 4 axis (X, Y, Z, RZ), value range 0 to 255, 1 byte each (4 bytes total)
		u8 axis_x{};
		u8 axis_y{};
		u8 axis_z{};
		u8 axis_rz{};

		// 12 axis (0x20 - 0x2B), value range 0 to 255, 1 byte each (12 bytes total)
		// These 12 values match the pressure sensitivity values of the buttons (CELL_PAD_BTN_OFFSET_PRESS)
		u8 pressure_right{};    // value for CELL_PAD_BTN_OFFSET_PRESS_RIGHT    -> always 0 ?
		u8 pressure_left{};     // value for CELL_PAD_BTN_OFFSET_PRESS_LEFT     -> always 0 ?
		u8 pressure_up{};       // value for CELL_PAD_BTN_OFFSET_PRESS_UP       -> always 0 ?
		u8 pressure_down{};     // value for CELL_PAD_BTN_OFFSET_PRESS_DOWN     -> always 0 ?
		u8 pressure_triangle{}; // value for CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE -> infrared nose
		u8 pressure_circle{};   // value for CELL_PAD_BTN_OFFSET_PRESS_CIRCLE   -> infrared tail
		u8 pressure_cross{};    // value for CELL_PAD_BTN_OFFSET_PRESS_CROSS    -> infrared left
		u8 pressure_square{};   // value for CELL_PAD_BTN_OFFSET_PRESS_SQUARE   -> infrared right
		u8 pressure_l1{};       // value for CELL_PAD_BTN_OFFSET_PRESS_L1       -> tilt left (probably)
		u8 pressure_r1{};       // value for CELL_PAD_BTN_OFFSET_PRESS_R1       -> tilt right (probably)
		u8 pressure_l2{};       // value for CELL_PAD_BTN_OFFSET_PRESS_L2       -> always 0 ?
		u8 pressure_r2{};       // value for CELL_PAD_BTN_OFFSET_PRESS_R2       -> always 0 ?

		// 4 axis (0x2C - 0x2F), value range 0 to 1023, 2 bytes each (8 bytes total)
		std::array<u16, 4> large_axes{};
	};
#pragma pack(pop)

	struct skateboard_output_report
	{
		// 8 axis (0x2621), value range 0 to 255, 1 byte each (8 bytes total)
		std::array<u8, 8> data{};
	};

	struct skateboard_feature_report
	{
		// 8 axis (0x2621), value range 0 to 255, 1 byte each (8 bytes total)
		std::array<u8, 8> data{};
	};
}

class skateboard_device : public HidDevice
{
public:
	bool skateboard_is_on = false;
	reports::skateboard_input_report report{};
};

class skateboard_pad_handler final : public hid_pad_handler<skateboard_device>
{
	enum skateboard_key_codes
	{
		none = 0,
		left,
		right,
		up,
		down,
		cross,
		square,
		circle,
		triangle,
		start,
		select,
		ps,
		ir_nose,
		ir_tail,
		ir_left,
		ir_right,
		tilt_left,
		tilt_right,
	};

public:
	skateboard_pad_handler();
	~skateboard_pad_handler();

	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	void init_config(cfg_pad* cfg) override;

private:
	DataStatus get_data(skateboard_device* device) override;
	void check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view wide_serial) override;
	int send_output_report(skateboard_device* device) override;

	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
};
