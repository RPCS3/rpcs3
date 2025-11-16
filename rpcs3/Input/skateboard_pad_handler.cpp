#include "stdafx.h"
#include "skateboard_pad_handler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(skateboard_log, "Skateboard");

using namespace reports;

namespace
{
	constexpr id_pair SKATEBOARD_ID_0 = {0x12BA, 0x0400}; // Tony Hawk RIDE Skateboard
	constexpr id_pair SKATEBOARD_ID_1 = {0x1430, 0x0100}; // Tony Hawk SHRED Skateboard

	enum button_flags : u16
	{
		square   = 0x0001,
		cross    = 0x0002,
		circle   = 0x0004,
		triangle = 0x0008,
		select   = 0x0100,
		start    = 0x0200,
		ps       = 0x1000,
	};

	enum dpad_states : u8
	{
		up         = 0x00,
		up_right   = 0x01,
		left       = 0x02,
		down_right = 0x03,
		down       = 0x04,
		down_left  = 0x05,
		right      = 0x06,
		up_left    = 0x07,
		none       = 0x0F,
	};

	// Default data if dongle is connected but the skateboard is turned off:
	static constexpr std::array<u8, sizeof(skateboard_input_report)> not_connected_state = { 0x00, 0x00, 0x0F, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02 };
	static constexpr std::array<u8, sizeof(skateboard_input_report)> disconnected_state  = { 0x00, 0x00, 0x0F, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
}

skateboard_pad_handler::skateboard_pad_handler()
    : hid_pad_handler<skateboard_device>(pad_handler::skateboard, {SKATEBOARD_ID_0, SKATEBOARD_ID_1})
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
		{ skateboard_key_codes::none,       "" },
		{ skateboard_key_codes::left,       "Left" },
		{ skateboard_key_codes::right,      "Right" },
		{ skateboard_key_codes::up,         "Up" },
		{ skateboard_key_codes::down,       "Down" },
		{ skateboard_key_codes::cross,      "Cross" },
		{ skateboard_key_codes::square,     "Square" },
		{ skateboard_key_codes::circle,     "Circle" },
		{ skateboard_key_codes::triangle,   "Triangle" },
		{ skateboard_key_codes::start,      "Start" },
		{ skateboard_key_codes::select,     "Select" },
		{ skateboard_key_codes::ps,         "PS" },
		{ skateboard_key_codes::ir_left,    "IR Left" },
		{ skateboard_key_codes::ir_right,   "IR Right" },
		{ skateboard_key_codes::ir_nose,    "IR Nose" },
		{ skateboard_key_codes::ir_tail,    "IR Tail" },
		{ skateboard_key_codes::tilt_left,  "Tilt Left" },
		{ skateboard_key_codes::tilt_right, "Tilt Right" }
	};

	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;

	// Set capabilities
	b_has_config = true;
	b_has_rumble = false;
	b_has_motion = true;
	b_has_deadzones = false;
	b_has_led = false;
	b_has_rgb = false;
	b_has_player_led = false;
	b_has_battery = false;
	b_has_battery_led = false;
	b_has_pressure_intensity_button = false;
	b_has_orientation = true;

	m_name_string = "Skateboard #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;
}

skateboard_pad_handler::~skateboard_pad_handler()
{
}

void skateboard_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->ls_down.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->ls_right.def = ::at32(button_list, skateboard_key_codes::none);
	cfg->ls_up.def    = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_left.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_down.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_right.def = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_up.def    = ::at32(button_list, skateboard_key_codes::none);
	cfg->start.def    = ::at32(button_list, skateboard_key_codes::start);
	cfg->select.def   = ::at32(button_list, skateboard_key_codes::select);
	cfg->ps.def       = ::at32(button_list, skateboard_key_codes::ps);
	cfg->square.def   = ::at32(button_list, skateboard_key_codes::square);
	cfg->cross.def    = ::at32(button_list, skateboard_key_codes::cross);
	cfg->circle.def   = ::at32(button_list, skateboard_key_codes::circle);
	cfg->triangle.def = ::at32(button_list, skateboard_key_codes::triangle);
	cfg->left.def     = ::at32(button_list, skateboard_key_codes::left);
	cfg->down.def     = ::at32(button_list, skateboard_key_codes::down);
	cfg->right.def    = ::at32(button_list, skateboard_key_codes::right);
	cfg->up.def       = ::at32(button_list, skateboard_key_codes::up);
	cfg->r1.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->r2.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->r3.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->l1.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->l2.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->l3.def       = ::at32(button_list, skateboard_key_codes::none);

	cfg->ir_nose.def    = ::at32(button_list, skateboard_key_codes::ir_nose);
	cfg->ir_tail.def    = ::at32(button_list, skateboard_key_codes::ir_tail);
	cfg->ir_left.def    = ::at32(button_list, skateboard_key_codes::ir_left);
	cfg->ir_right.def   = ::at32(button_list, skateboard_key_codes::ir_right);
	cfg->tilt_left.def  = ::at32(button_list, skateboard_key_codes::tilt_left);
	cfg->tilt_right.def = ::at32(button_list, skateboard_key_codes::tilt_right);

	cfg->orientation_reset_button.def = ::at32(button_list, skateboard_key_codes::none);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = 0;
	cfg->rstick_anti_deadzone.def = 0;
	cfg->lstickdeadzone.def    = 40; // between 0 and 255
	cfg->rstickdeadzone.def    = 40; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255

	// apply defaults
	cfg->from_default();
}

void skateboard_pad_handler::check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view wide_serial)
{
	if (!hidDevice)
	{
		return;
	}

	skateboard_device* device = nullptr;

	for (auto& controller : m_controllers)
	{
		ensure(controller.second);

		if (!controller.second->hidDevice)
		{
			device = controller.second.get();
			break;
		}
	}

	if (!device)
	{
		return;
	}

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		skateboard_log.error("check_add_device: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		HidDevice::close(hidDevice);
		return;
	}

	device->hidDevice = hidDevice;
	device->path = path;

	// Activate
	if (send_output_report(device) == -1)
	{
		skateboard_log.error("check_add_device: send_output_report failed! Reason: %s", hid_error(hidDevice));
	}

	std::string serial;
	for (wchar_t ch : wide_serial)
		serial += static_cast<uchar>(ch);

	skateboard_log.notice("Added device: serial='%s', path='%s'", serial, device->path);
}

skateboard_pad_handler::DataStatus skateboard_pad_handler::get_data(skateboard_device* device)
{
	if (!device)
		return DataStatus::ReadError;

	std::array<u8, sizeof(skateboard_input_report)> buf{};

	int res = hid_read(device->hidDevice, buf.data(), buf.size());

	if (res == -1)
	{
		// looks like controller disconnected or read error
		skateboard_log.error("get_data ReadError: %s", hid_error(device->hidDevice));
		return DataStatus::ReadError;
	}

	if (res != static_cast<int>(sizeof(skateboard_input_report)))
		return DataStatus::NoNewData;

	if (std::memcmp(&device->report, buf.data(), sizeof(skateboard_input_report)) == 0)
		return DataStatus::NoNewData;

	// Get the new data
	std::memcpy(&device->report, buf.data(), sizeof(skateboard_input_report));

	// Check the skateboard's power state based on the input report
	device->skateboard_is_on =
		(std::memcmp(not_connected_state.data(), buf.data(), not_connected_state.size()) != 0 && // This usually means that the device hasn't been connected to the dongle yet.
		 std::memcmp(disconnected_state.data(), buf.data(), disconnected_state.size()) != 0);    // This usually means that the device was disconnected from the dongle.

	return DataStatus::NewData;
}

PadHandlerBase::connection skateboard_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	skateboard_device* dev = static_cast<skateboard_device*>(device.get());
	if (!dev || dev->path == hid_enumerated_device_default)
		return connection::disconnected;

	if (dev->hidDevice == nullptr)
	{
		// try to reconnect
		if (hid_device* hid_dev = dev->open())
		{
			if (hid_set_nonblocking(hid_dev, 1) == -1)
			{
				skateboard_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", dev->path, hid_error(hid_dev));
			}
		}
		else
		{
			// nope, not there
			return connection::disconnected;
		}
	}

	if (get_data(dev) == DataStatus::ReadError)
	{
		// this also can mean disconnected, either way deal with it on next loop and reconnect
		dev->close();

		return connection::no_data;
	}

	if (!dev->skateboard_is_on)
	{
		// This means that the dongle is still connected, but the skateboard is turned off.
		// There is no need to reconnect the hid device again, we just have to check the input report for proper data.
		// The game should get the proper disconnected state anyway.
		return connection::disconnected;
	}

	return connection::connected;
}

std::unordered_map<u64, u16> skateboard_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> key_buf;
	skateboard_device* dev = static_cast<skateboard_device*>(device.get());
	if (!dev)
		return key_buf;

	const skateboard_input_report& input = dev->report;

	// D-Pad
	key_buf[skateboard_key_codes::left]  = (input.d_pad == dpad_states::left || input.d_pad == dpad_states::up_left || input.d_pad == dpad_states::down_left) ? 255 : 0;
	key_buf[skateboard_key_codes::right] = (input.d_pad == dpad_states::right || input.d_pad == dpad_states::up_right || input.d_pad == dpad_states::down_right) ? 255 : 0;
	key_buf[skateboard_key_codes::up]    = (input.d_pad == dpad_states::up || input.d_pad == dpad_states::up_left || input.d_pad == dpad_states::up_right) ? 255 : 0;
	key_buf[skateboard_key_codes::down]  = (input.d_pad == dpad_states::down || input.d_pad == dpad_states::down_left || input.d_pad == dpad_states::down_right) ? 255 : 0;

	// Face buttons
	key_buf[skateboard_key_codes::cross]    = (input.buttons & button_flags::cross)    ? 255 : 0;
	key_buf[skateboard_key_codes::square]   = (input.buttons & button_flags::square)   ? 255 : 0;
	key_buf[skateboard_key_codes::circle]   = (input.buttons & button_flags::circle)   ? 255 : 0;
	key_buf[skateboard_key_codes::triangle] = (input.buttons & button_flags::triangle) ? 255 : 0;
	key_buf[skateboard_key_codes::start]    = (input.buttons & button_flags::start)    ? 255 : 0;
	key_buf[skateboard_key_codes::select]   = (input.buttons & button_flags::select)   ? 255 : 0;
	key_buf[skateboard_key_codes::ps]       = (input.buttons & button_flags::ps)       ? 255 : 0;

	// Infrared
	key_buf[skateboard_key_codes::ir_nose]    = input.pressure_triangle;
	key_buf[skateboard_key_codes::ir_tail]    = input.pressure_circle;
	key_buf[skateboard_key_codes::ir_left]    = input.pressure_cross;
	key_buf[skateboard_key_codes::ir_right]   = input.pressure_square;
	key_buf[skateboard_key_codes::tilt_left]  = input.pressure_l1;
	key_buf[skateboard_key_codes::tilt_right] = input.pressure_r1;

	// NOTE: Axes X, Y, Z and RZ are always 128, which is the default anyway, so setting the values is omitted.

	return key_buf;
}

void skateboard_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	skateboard_device* dev = static_cast<skateboard_device*>(device.get());
	if (!dev || !pad)
		return;

	const skateboard_input_report& input = dev->report;

	pad->m_sensors[0].m_value = Clamp0To1023(input.large_axes[0]);
	pad->m_sensors[1].m_value = Clamp0To1023(input.large_axes[1]);
	pad->m_sensors[2].m_value = Clamp0To1023(input.large_axes[2]);
	pad->m_sensors[3].m_value = Clamp0To1023(input.large_axes[3]);

	set_raw_orientation(*pad);
}

pad_preview_values skateboard_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& /*data*/)
{
	// There is no proper user interface for skateboard values yet
	return {};
}

int skateboard_pad_handler::send_output_report(skateboard_device* device)
{
	if (!device || !device->hidDevice)
		return -2;

	const cfg_pad* config = device->config;
	if (config == nullptr)
		return -2; // hid_write returns -1 on error

	// The output report contents are still unknown
	skateboard_output_report report{};
	return hid_write(device->hidDevice, report.data.data(), report.data.size());
}

void skateboard_pad_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	skateboard_device* dev = static_cast<skateboard_device*>(device.get());
	if (!dev || !dev->hidDevice || !dev->config || !pad)
		return;

	dev->new_output_data = false;

	// Disabled until needed
	//const auto now = steady_clock::now();
	//const auto elapsed = now - dev->last_output;

	//if (dev->new_output_data || elapsed > min_output_interval)
	//{
	//	if (const int res = send_output_report(dev); res >= 0)
	//	{
	//		dev->new_output_data = false;
	//		dev->last_output = now;
	//	}
	//	else if (res == -1)
	//	{
	//		skateboard_log.error("apply_pad_data: send_output_report failed! error=%s", hid_error(dev->hidDevice));
	//	}
	//}
}

void skateboard_pad_handler::SetPadData(const std::string& padId, u8 player_id, u8 /*large_motor*/, u8 /*small_motor*/, s32 /*r*/, s32 /*g*/, s32 /*b*/, bool /*player_led*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	std::shared_ptr<skateboard_device> device = get_hid_device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
		return;

	device->player_id = player_id;
	device->config = get_config(padId);

	ensure(device->config);

	// Disabled until needed
	//if (send_output_report(device.get()) == -1)
	//{
	//	skateboard_log.error("SetPadData: send_output_report failed! Reason: %s", hid_error(device->hidDevice));
	//}
}
