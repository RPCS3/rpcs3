#include "stdafx.h"
#include "ps_move_handler.h"
#include "ps_move_calibration.h"
#include "Emu/Io/pad_config.h"
#include "Emu/Cell/Modules/cellGem.h"

LOG_CHANNEL(move_log, "Move");

using namespace reports;

namespace
{
	constexpr id_pair MOVE_ID_ZCM1 = {0x054C, 0x03D5};
	constexpr id_pair MOVE_ID_ZCM2 = {0x054C, 0x0c5e};

	enum button_flags : u16
	{
		select   = 0x01,
		start    = 0x08,
		triangle = 0x10,
		circle   = 0x20,
		cross    = 0x40,
		square   = 0x80,
		ps       = 0x0001,
		move     = 0x4008,
		t        = 0x8010,
		ext_dev  = 0x1000,

		// Sharpshooter
		ss_firing_mode_1 = 0x01,
		ss_firing_mode_2 = 0x02,
		ss_firing_mode_3 = 0x04,
		ss_trigger       = 0x40,
		ss_reload        = 0x80,

		// Racing Wheel
		rw_d_pad_up    = 0x10,
		rw_d_pad_right = 0x20,
		rw_d_pad_down  = 0x40,
		rw_d_pad_left  = 0x80,
		rw_l1          = 0x04,
		rw_r1          = 0x08,
		rw_paddle_l    = 0x01,
		rw_paddle_r    = 0x02,
	};

	enum battery_status : u8
	{
		charge_empty = 0x00,
		charge_1     = 0x01,
		charge_2     = 0x02,
		charge_3     = 0x03,
		charge_4     = 0x04,
		charge_full  = 0x05,
		usb_charging = 0xEE,
		usb_charged  = 0xEF,
	};
}

const ps_move_input_report_common& ps_move_device::input_report_common() const
{
	switch (model)
	{
	default:
	case ps_move_model::ZCM1:
	{
		return input_report_ZCM1.common;
	}
	case ps_move_model::ZCM2:
	{
		return input_report_ZCM2.common;
	}
	}
}

ps_move_handler::ps_move_handler()
    : hid_pad_handler<ps_move_device>(pad_handler::move, { MOVE_ID_ZCM1, MOVE_ID_ZCM2 })
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
		{ ps_move_key_codes::none,          "" },
		{ ps_move_key_codes::cross,         "Cross" },
		{ ps_move_key_codes::square,        "Square" },
		{ ps_move_key_codes::circle,        "Circle" },
		{ ps_move_key_codes::triangle,      "Triangle" },
		{ ps_move_key_codes::start,         "Start" },
		{ ps_move_key_codes::select,        "Select" },
		{ ps_move_key_codes::ps,            "PS" },
		{ ps_move_key_codes::move,          "Move" },
		{ ps_move_key_codes::t,             "T" },
		{ ps_move_key_codes::firing_mode_1, "Firing Mode 1" },
		{ ps_move_key_codes::firing_mode_2, "Firing Mode 2" },
		{ ps_move_key_codes::firing_mode_3, "Firing Mode 3" },
		{ ps_move_key_codes::reload,        "Reload" },
		{ ps_move_key_codes::dpad_up,       "D-Pad Up" },
		{ ps_move_key_codes::dpad_down,     "D-Pad Down" },
		{ ps_move_key_codes::dpad_left,     "D-Pad Left" },
		{ ps_move_key_codes::dpad_right,    "D-Pad Right" },
		{ ps_move_key_codes::L1,            "L1" },
		{ ps_move_key_codes::R1,            "R1" },
		{ ps_move_key_codes::L2,            "L2" },
		{ ps_move_key_codes::R2,            "R2" },
		{ ps_move_key_codes::throttle,      "Throttle" },
		{ ps_move_key_codes::paddle_left,   "Paddle Left" },
		{ ps_move_key_codes::paddle_right,  "Paddle Right" },
	};

	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;

	// Set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_motion = true;
	b_has_deadzones = true;
	b_has_led = true;
	b_has_rgb = true;
	b_has_player_led = false;
	b_has_battery = true;
	b_has_battery_led = false;
	b_has_pressure_intensity_button = false;
	b_has_orientation = true;

	m_name_string = "PS Move #";
	m_max_devices = 4; // CELL_GEM_MAX_NUM

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;
}

ps_move_handler::~ps_move_handler()
{
}

void ps_move_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(button_list, ps_move_key_codes::none);
	cfg->ls_down.def  = ::at32(button_list, ps_move_key_codes::none);
	cfg->ls_right.def = ::at32(button_list, ps_move_key_codes::none);
	cfg->ls_up.def    = ::at32(button_list, ps_move_key_codes::none);
	cfg->rs_left.def  = ::at32(button_list, ps_move_key_codes::none);
	cfg->rs_down.def  = ::at32(button_list, ps_move_key_codes::none);
	cfg->rs_right.def = ::at32(button_list, ps_move_key_codes::none);
	cfg->rs_up.def    = ::at32(button_list, ps_move_key_codes::none);
	cfg->start.def    = ::at32(button_list, ps_move_key_codes::start);
	cfg->select.def   = ::at32(button_list, ps_move_key_codes::select);
	cfg->ps.def       = ::at32(button_list, ps_move_key_codes::ps);
	cfg->square.def   = ::at32(button_list, ps_move_key_codes::square);
	cfg->cross.def    = ::at32(button_list, ps_move_key_codes::cross);
	cfg->circle.def   = ::at32(button_list, ps_move_key_codes::circle);
	cfg->triangle.def = ::at32(button_list, ps_move_key_codes::triangle);
	cfg->left.def     = ::at32(button_list, ps_move_key_codes::none);
	cfg->down.def     = ::at32(button_list, ps_move_key_codes::none);
	cfg->right.def    = ::at32(button_list, ps_move_key_codes::none);
	cfg->up.def       = ::at32(button_list, ps_move_key_codes::none);
	cfg->r1.def       = ::at32(button_list, ps_move_key_codes::move);
	cfg->r2.def       = ::at32(button_list, ps_move_key_codes::t);
	cfg->r3.def       = ::at32(button_list, ps_move_key_codes::none);
	cfg->l1.def       = ::at32(button_list, ps_move_key_codes::none);
	cfg->l2.def       = ::at32(button_list, ps_move_key_codes::none);
	cfg->l3.def       = ::at32(button_list, ps_move_key_codes::none);

	cfg->orientation_reset_button.def = ::at32(button_list, ps_move_key_codes::none);

	// Set default misc variables
	cfg->lstickdeadzone.def    = 40; // between 0 and 255
	cfg->rstickdeadzone.def    = 40; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255

	// apply defaults
	cfg->from_default();
}

#ifdef _WIN32
hid_device* ps_move_handler::connect_move_device(ps_move_device* device, std::string_view path)
{
	if (!device)
	{
		return nullptr;
	}

	// Windows enumerates 3 ps move devices: Col01, Col02, and Col03.
	// We use Col01 for data and Col02 for bluetooth.
	// Our enumerated paths are filtered and only contain Col01.
	// We open Col02 first, and then Col01. Col02 is unused for now.

	static const std::string col01 = "&Col01#";
	static const std::string number = "&0000#";

	std::string col02_path { path };
	col02_path.replace(path.find(col01), col01.size(), "&Col02#");
	col02_path.replace(path.find(number), number.size(), "&0001#");

	// Open Col02
	device->bt_device = hid_open_path(col02_path.c_str());
	if (!device->bt_device)
	{
		move_log.error("%s hid_open_path failed! error='%s', path='%s'", m_type, hid_error(device->bt_device), col02_path);
		return nullptr;
	}

	if (const hid_device_info* info = hid_get_device_info(device->bt_device))
	{
		move_log.notice("%s adding bt device: vid=0x%x, pid=0x%x, path='%s'", m_type, info->vendor_id, info->product_id, col02_path);
	}
	else
	{
		move_log.warning("%s adding bt device: vid=N/A, pid=N/A, path='%s', error='%s'", m_type, col02_path, hid_error(device->bt_device));
	}

	if (hid_set_nonblocking(device->bt_device, 1) == -1)
	{
		move_log.error("connect_move_device: hid_set_nonblocking failed! Reason: %s", hid_error(device->bt_device));
		device->close();
		return nullptr;
	}

	// Open Col01
	device->hidDevice = hid_open_path(path.data());
	if (!device->hidDevice)
	{
		move_log.error("%s hid_open_path failed! error='%s', path='%s'", m_type, hid_error(device->bt_device), path);
		device->close();
		return nullptr;
	}

	if (hid_set_nonblocking(device->hidDevice, 1) == -1)
	{
		move_log.error("connect_move_device: hid_set_nonblocking failed! Reason: %s", hid_error(device->hidDevice));
		device->close();
		return nullptr;
	}

	if (const hid_device_info* info = hid_get_device_info(device->hidDevice))
	{
		move_log.notice("%s adding device: vid=0x%x, pid=0x%x, path='%s'", m_type, info->vendor_id, info->product_id, col02_path);

		switch (info->product_id)
		{
		default:
		case MOVE_ID_ZCM1.m_pid:
			device->model = ps_move_model::ZCM1;
			break;
		case MOVE_ID_ZCM2.m_pid:
			device->model = ps_move_model::ZCM2;
			break;
		}
	}
	else
	{
		move_log.warning("%s adding device: vid=N/A, pid=N/A, path='%s', error='%s'", m_type, col02_path, hid_error(device->hidDevice));
		device->model = ps_move_model::ZCM1;
	}

	return device->hidDevice;
}
#endif

void ps_move_handler::check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view wide_serial)
{
#ifndef _WIN32
	if (!hidDevice)
	{
		return;
	}
#endif

	ps_move_device* device = nullptr;

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

#ifdef _WIN32
	hidDevice = connect_move_device(device, path);
	if (!hidDevice)
	{
		device->close();
		return;
	}
#else
	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		move_log.error("check_add_device: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		HidDevice::close(hidDevice);
		return;
	}
#endif

	device->hidDevice = hidDevice;
	device->path = path;

	// Get calibration
	device->calibration.is_valid = true;

	ps_move_calibration_blob calibration {};

	for (int i = 0; i < 2; i++)
	{
		std::array<u8, PSMOVE_CALIBRATION_SIZE> cal {};
		cal[0] = 0x10;
		const int res = hid_get_feature_report(device->hidDevice, cal.data(), cal.size());
		if (res < 0)
		{
			move_log.error("connect_move_device: hid_get_feature_report 0x10 (calibration) failed! result=%d, error=%s", res, hid_error(device->hidDevice));
			device->calibration.is_valid = false;
			break;
		}

		int src_offset = 0;
		int dest_offset = 0;

		if ((cal[1] == 0x01 && device->model == ps_move_model::ZCM1) ||
			(cal[1] == 0x81 && device->model == ps_move_model::ZCM2))
		{
			// This is the second block
			dest_offset = PSMOVE_CALIBRATION_SIZE;
			src_offset = 2;
		}
		else if (cal[1] == 0x82 && device->model == ps_move_model::ZCM1)
		{
			// This is the third block
			dest_offset = 2 * PSMOVE_CALIBRATION_SIZE - 2;
			src_offset = 2;
		}
		else if (cal[1] != 0x00) // Check if this is the first block (offsets stay 0)
		{
			move_log.error("connect_move_device: Failed to read calibration: cal=0x%x'", cal[1]);
			device->calibration.is_valid = false;
			break;
		}

		std::memcpy(&calibration.data[dest_offset], &cal[src_offset], cal.size() - src_offset);
	}

	if (device->calibration.is_valid)
	{
		psmove_parse_calibration(calibration, *device);
	}

	// Activate
	if (send_output_report(device) == -1)
	{
		move_log.error("check_add_device: send_output_report failed! Reason: %s", hid_error(hidDevice));
	}

	std::string serial;
	for (wchar_t ch : wide_serial)
		serial += static_cast<uchar>(ch);

	move_log.success("Added device: serial='%s', path='%s'", serial, device->path);
}

ps_move_handler::DataStatus ps_move_handler::get_data(ps_move_device* device)
{
	if (!device)
		return DataStatus::ReadError;

	constexpr u8 reportId = 0x01;
	void* report = nullptr;
	usz report_size = 0;

	switch (device->model)
	{
	case ps_move_model::ZCM1:
		report = &device->input_report_ZCM1;
		device->input_report_ZCM1.common.report_id = reportId;
		report_size = sizeof(ps_move_input_report_ZCM1);
		break;
	case ps_move_model::ZCM2:
		report = &device->input_report_ZCM2;
		device->input_report_ZCM2.common.report_id = reportId;
		report_size = sizeof(ps_move_input_report_ZCM2);
		break;
	}

	std::vector<u8> buf(report_size);

	int res = hid_read(device->hidDevice, buf.data(), report_size);
	if (res < 0)
	{
		// looks like controller disconnected or read error
		move_log.error("get_data: hid_read 0x%02x failed! result=%d, buf[0]=0x%x, error=%s", reportId, res, buf[0], hid_error(device->hidDevice));
		return DataStatus::ReadError;
	}

	if (res != static_cast<int>(report_size))
		return DataStatus::NoNewData;

	if (std::memcmp(report, buf.data(), report_size) == 0)
		return DataStatus::NoNewData;

	// Get the new data
	std::memcpy(report, buf.data(), report_size);

	//move_log.error("%s", fmt::buf_to_hexstring(buf.data(), buf.size(), 64));

	return DataStatus::NewData;
}

PadHandlerBase::connection ps_move_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	ps_move_device* move_device = static_cast<ps_move_device*>(device.get());
	if (!move_device || move_device->path == hid_enumerated_device_default)
		return connection::disconnected;

	if (move_device->hidDevice == nullptr)
	{
		// try to reconnect
#ifdef _WIN32
		if (hid_device* dev = connect_move_device(move_device, move_device->path))
		{
			move_device->hidDevice = dev;
		}
#else
		if (hid_device* dev = move_device->open())
		{
			if (hid_set_nonblocking(dev, 1) == -1)
			{
				move_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", move_device->path, hid_error(dev));
			}
		}
#endif
		else
		{
			// nope, not there
			move_log.error("Device %s: disconnected", move_device->path);
			return connection::disconnected;
		}
	}

	if (get_data(move_device) == DataStatus::ReadError)
	{
		// this also can mean disconnected, either way deal with it on next loop and reconnect
		move_device->close();

		return connection::no_data;
	}

	return connection::connected;
}

void ps_move_handler::handle_external_device(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	ps_move_device* dev = static_cast<ps_move_device*>(device.get());
	if (!dev || !pad)
		return;

	auto& move_data = pad->move_data;

	if (dev->model != ps_move_model::ZCM1)
	{
		move_data.external_device_read_requested = false;
		move_data.external_device_write_requested = false;
		return;
	}

	const ps_move_input_report_common& input = dev->input_report_common();
	const u16 extra_buttons = input.sequence_number << 8 | input.buttons_3;

	move_data.external_device_connected = !!(extra_buttons & button_flags::ext_dev);

	if (!move_data.external_device_connected)
	{
		dev->external_device_id = move_data.external_device_id = 0;
		std::memset(move_data.external_device_data.data(), 0, move_data.external_device_data.size());
		move_data.external_device_read_requested = false;
		move_data.external_device_write_requested = false;
		return;
	}

	std::memcpy(move_data.external_device_data.data(), dev->input_report_ZCM1.ext_device_data.data(), dev->input_report_ZCM1.ext_device_data.size());

	if (move_data.external_device_read_requested || move_data.external_device_id == 0)
	{
		bool success = false;

		std::array<u8, 49> ext_buf{};
		ext_buf[0x00] = 0xE0; // Report ID
		ext_buf[0x01] = 0x01; // Read flag
		ext_buf[0x02] = 0xA0; // Target extension device's I²C slave address
		ext_buf[0x03] = 0x00; // Offset
		ext_buf[0x04] = 0xFF; // Length

		if (int res = hid_send_feature_report(dev->hidDevice, ext_buf.data(), ext_buf.size()); res != static_cast<int>(ext_buf.size()))
		{
			move_log.error("get_extended_info: hid_send_feature_report 0xE0 (external_device) failed! result=%d, ext_buf[0]=0x%x, error=%s", res, ext_buf[0], hid_error(dev->hidDevice));
		}
		else if (res = hid_get_feature_report(dev->hidDevice, ext_buf.data(), ext_buf.size()); res < 0)
		{
			move_log.error("get_extended_info: hid_get_feature_report 0xE0 (external_device) failed! result=%d, ext_buf[0]=0x%x, error=%s", res, ext_buf[0], hid_error(dev->hidDevice));
		}
		else if (ext_buf[0x01] != 0) // The result will hold an error flag at pos 0x01
		{
			move_log.error("get_extended_info: hid_get_feature_report 0xE0 (external_device) returned error: ext_buf[0x01]=0x%x, error=%s", ext_buf[0x01], hid_error(dev->hidDevice));
		}
		else
		{
			move_log.trace("get_extended_info: hid_get_feature_report 0xE0 got result: %s", fmt::buf_to_hexstring(ext_buf.data(), ext_buf.size(), 64));
			success = true;
		}

		// Get device ID
		const u32 old_id = dev->external_device_id;

		// The result will be stored starting at pos 0x09
		dev->external_device_id = move_data.external_device_id = (ext_buf[0x09] << 8) | ext_buf[0x0A];

		if (dev->external_device_id != 0 && dev->external_device_id != old_id)
		{
			move_log.notice("get_extended_info: external device with ID 0x%x found", dev->external_device_id);
		}

		if (move_data.external_device_read_requested)
		{
			auto& dst = move_data.external_device_read;

			if (success)
			{
				// Copy everything except device ID starting at pos 0x0B
				ensure(ext_buf.size() == dst.size() + 0x0B);
				std::memcpy(dst.data(), &ext_buf[0x0B], dst.size());
			}
			else
			{
				std::memset(dst.data(), 0, dst.size());
			}
		}
	}

	if (move_data.external_device_write_requested)
	{
		const auto& src = move_data.external_device_write;

		std::array<u8, 49> ext_buf{};
		ext_buf[0x00] = 0xE0; // Report ID
		ext_buf[0x01] = 0x00; // Read flag
		ext_buf[0x02] = 0xA0; // Target extension device's I²C slave address
		ext_buf[0x03] = src[0];         // Control Byte
		ext_buf[0x04] = static_cast<u8>(src.size() - 1); // Length

		std::memcpy(&ext_buf[0x09], &src[1], src.size() - 1);

		move_log.trace("ps_move_handler: trying to send data to external device: %s", fmt::buf_to_hexstring(ext_buf.data(), ext_buf.size(), 64));

		if (const int res = hid_send_feature_report(dev->hidDevice, ext_buf.data(), ext_buf.size()); res < 0)
		{
			move_log.error("get_extended_info: hid_send_feature_report 0xE0 (external_device) failed! result=%d, ext_buf[0]=0x%x, error=%s", res, ext_buf[0], hid_error(dev->hidDevice));
		}
	}

	move_data.external_device_read_requested = false;
	move_data.external_device_write_requested = false;
}

bool ps_move_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	// We also report the T button as left trigger
	return keyCode == ps_move_key_codes::L2 || keyCode == ps_move_key_codes::t;
}

bool ps_move_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	// We also report the Throttle button as right trigger
	return keyCode == ps_move_key_codes::R2 || keyCode == ps_move_key_codes::throttle;
}

std::unordered_map<u64, u16> ps_move_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> key_buf;
	ps_move_device* dev = static_cast<ps_move_device*>(device.get());
	if (!dev)
		return key_buf;

	const ps_move_input_report_common& input = dev->input_report_common();

	key_buf[ps_move_key_codes::cross]    = (input.buttons_2 & button_flags::cross)    ? 255 : 0;
	key_buf[ps_move_key_codes::square]   = (input.buttons_2 & button_flags::square)   ? 255 : 0;
	key_buf[ps_move_key_codes::circle]   = (input.buttons_2 & button_flags::circle)   ? 255 : 0;
	key_buf[ps_move_key_codes::triangle] = (input.buttons_2 & button_flags::triangle) ? 255 : 0;
	key_buf[ps_move_key_codes::start]    = (input.buttons_1 & button_flags::start)    ? 255 : 0;
	key_buf[ps_move_key_codes::select]   = (input.buttons_1 & button_flags::select)   ? 255 : 0;

	const u16 extra_buttons = input.sequence_number << 8 | input.buttons_3;
	key_buf[ps_move_key_codes::ps]   = (extra_buttons & button_flags::ps)   ? 255 : 0;
	key_buf[ps_move_key_codes::move] = (extra_buttons & button_flags::move) ? 255 : 0;
	key_buf[ps_move_key_codes::t]    = (extra_buttons & button_flags::t)    ? input.trigger_2 : 0;

	dev->battery_level = input.battery_level;

	// Handle external data
	if (dev->model == ps_move_model::ZCM1)
	{
		const bool external_device_connected = !!(extra_buttons & button_flags::ext_dev);

		if (external_device_connected)
		{
			const std::array<u8, 5>& ext_data = dev->input_report_ZCM1.ext_device_data;

			switch (dev->external_device_id)
			{
			case SHARP_SHOOTER_DEVICE_ID:
				key_buf[ps_move_key_codes::firing_mode_1] = (ext_data[0] & button_flags::ss_firing_mode_1) ? 255 : 0;
				key_buf[ps_move_key_codes::firing_mode_2] = (ext_data[0] & button_flags::ss_firing_mode_2) ? 255 : 0;
				key_buf[ps_move_key_codes::firing_mode_3] = (ext_data[0] & button_flags::ss_firing_mode_3) ? 255 : 0;
				key_buf[ps_move_key_codes::reload]        = (ext_data[0] & button_flags::ss_reload)        ? 255 : 0;
				//key_buf[ps_move_key_codes::t]             = (ext_data[0] & button_flags::ss_trigger)       ? 255 : 0; // This is already reported as normal trigger
				break;
			case RACING_WHEEL_DEVICE_ID:
				key_buf[ps_move_key_codes::dpad_up]      = (input.buttons_1 & button_flags::rw_d_pad_up)    ? 255 : 0;
				key_buf[ps_move_key_codes::dpad_right]   = (input.buttons_1 & button_flags::rw_d_pad_right) ? 255 : 0;
				key_buf[ps_move_key_codes::dpad_down]    = (input.buttons_1 & button_flags::rw_d_pad_down)  ? 255 : 0;
				key_buf[ps_move_key_codes::dpad_left]    = (input.buttons_1 & button_flags::rw_d_pad_left)  ? 255 : 0;
				key_buf[ps_move_key_codes::L1]           = (input.buttons_2 & button_flags::rw_l1)          ? 255 : 0;
				key_buf[ps_move_key_codes::R1]           = (input.buttons_2 & button_flags::rw_r1)          ? 255 : 0;
				key_buf[ps_move_key_codes::throttle]     = ext_data[0];
				key_buf[ps_move_key_codes::L2]           = ext_data[1];
				key_buf[ps_move_key_codes::R2]           = ext_data[2];
				key_buf[ps_move_key_codes::paddle_left]  = (ext_data[3] & button_flags::rw_paddle_l) ? 255 : 0;
				key_buf[ps_move_key_codes::paddle_right] = (ext_data[3] & button_flags::rw_paddle_r) ? 255 : 0;
				break;
			default:
				break;
			}
		}
	}

	return key_buf;
}

void ps_move_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	ps_move_device* dev = static_cast<ps_move_device*>(device.get());
	if (!dev || !pad)
		return;

	const ps_move_input_report_common& input = dev->input_report_common();

	// The default position is flat on the ground, pointing forward.
	// The accelerometers constantly measure G forces.
	// The gyros measure changes in orientation and will reset when the device isn't moved anymore.
	f32 accel_x = input.accel_x_1; // Increases if the device is rolled to the left
	f32 accel_y = input.accel_y_1; // Increases if the device is pitched upwards
	f32 accel_z = input.accel_z_1; // Increases if the device is moved upwards
	f32 gyro_x = input.gyro_x_1;   // Increases if the device is pitched upwards
	f32 gyro_y = input.gyro_y_1;   // Increases if the device is rolled to the right
	f32 gyro_z = input.gyro_z_1;   // Increases if the device is yawed to the left

	if (dev->model == ps_move_model::ZCM1)
	{
		accel_x -= static_cast<f32>(zero_shift);
		accel_y -= static_cast<f32>(zero_shift);
		accel_z -= static_cast<f32>(zero_shift);
		gyro_x -= static_cast<f32>(zero_shift);
		gyro_y -= static_cast<f32>(zero_shift);
		gyro_z -= static_cast<f32>(zero_shift);
	}

	if (!device->config || !device->config->orientation_enabled)
	{
		pad->move_data.reset_sensors();
	}
	else
	{
		// Apply calibration
		if (dev->calibration.is_valid)
		{
			accel_x = accel_x * dev->calibration.accel_x_factor + dev->calibration.accel_x_offset;
			accel_y = accel_y * dev->calibration.accel_y_factor + dev->calibration.accel_y_offset;
			accel_z = accel_z * dev->calibration.accel_z_factor + dev->calibration.accel_z_offset;
			gyro_x = (gyro_x - dev->calibration.gyro_x_offset) * dev->calibration.gyro_x_gain;
			gyro_y = (gyro_y - dev->calibration.gyro_y_offset) * dev->calibration.gyro_y_gain;
			gyro_z = (gyro_z - dev->calibration.gyro_z_offset) * dev->calibration.gyro_z_gain;
		}
		else
		{
			constexpr f32 MOVE_ONE_G = 4096.0f; // This is just a rough estimate and probably depends on the device

			accel_x /= MOVE_ONE_G;
			accel_y /= MOVE_ONE_G;
			accel_z /= MOVE_ONE_G;
			gyro_x /= MOVE_ONE_G;
			gyro_y /= MOVE_ONE_G;
			gyro_z /= MOVE_ONE_G;
		}

		pad->move_data.accelerometer_x = accel_x;
		pad->move_data.accelerometer_y = accel_y;
		pad->move_data.accelerometer_z = accel_z;
		pad->move_data.gyro_x = gyro_x;
		pad->move_data.gyro_y = gyro_y;
		pad->move_data.gyro_z = gyro_z;

		if (dev->model == ps_move_model::ZCM1)
		{
			const ps_move_input_report_ZCM1& input_zcm1 = dev->input_report_ZCM1;

			#define TWELVE_BIT_SIGNED(x) (((x) & 0x800) ? (-(((~(x)) & 0xFFF) + 1)) : (x))
			pad->move_data.magnetometer_x = static_cast<f32>(TWELVE_BIT_SIGNED(((input.magnetometer_x & 0x0F) << 8) | input_zcm1.magnetometer_x2));
			pad->move_data.magnetometer_y = static_cast<f32>(TWELVE_BIT_SIGNED((input_zcm1.magnetometer_y << 4) | (input_zcm1.magnetometer_yz & 0xF0) >> 4));
			pad->move_data.magnetometer_z = static_cast<f32>(TWELVE_BIT_SIGNED(((input_zcm1.magnetometer_yz & 0x0F) << 8) | input_zcm1.magnetometer_z));
		}
	}

	pad->move_data.temperature = ((input.temperature << 4) | ((input.magnetometer_x & 0xF0) >> 4));

	pad->m_sensors[0].m_value = Clamp0To1023(512.0f + (MOTION_ONE_G * accel_x * -1.0f));
	pad->m_sensors[1].m_value = Clamp0To1023(512.0f + (MOTION_ONE_G * accel_y * -1.0f));
	pad->m_sensors[2].m_value = Clamp0To1023(512.0f + (MOTION_ONE_G * accel_z));
	pad->m_sensors[3].m_value = Clamp0To1023(512.0f + (MOTION_ONE_G * gyro_z * -1.0f));

	handle_external_device(binding);
}

pad_preview_values ps_move_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
{
	return {
		std::max(::at32(data, ps_move_key_codes::L2), ::at32(data, ps_move_key_codes::t)),
		std::max(::at32(data, ps_move_key_codes::R2), ::at32(data, ps_move_key_codes::throttle)),
		0,
		0,
		0,
		0
	};
}

int ps_move_handler::send_output_report(ps_move_device* device)
{
	if (!device || !device->hidDevice)
		return -2;

	const cfg_pad* config = device->config;
	if (config == nullptr)
		return -2; // hid_write returns -1 on error

	device->output_report.type = 0x06;
	device->output_report.rumble = device->large_motor;

	// Override color if necessary (for example while we actually use the PS Move with cellGem)
	if (device->color_override_active)
	{
		device->output_report.r = device->color_override.r;
		device->output_report.g = device->color_override.g;
		device->output_report.b = device->color_override.b;
	}
	else
	{
		device->output_report.r = config->colorR;
		device->output_report.g = config->colorG;
		device->output_report.b = config->colorB;
	}

	const auto now = steady_clock::now();
	const auto elapsed = now - device->last_output_report_time;

	// Update LED at an interval or it will be disabled automatically
	device->new_output_data |= elapsed >= 4000ms;

	if (!device->new_output_data)
	{
		// Use LED update rate of 120ms
		if (elapsed < 120ms)
		{
			return -3;
		}

		device->new_output_data = std::memcmp(&device->output_report, &device->last_output_report, sizeof(ps_move_output_report));

		if (!device->new_output_data)
		{
			return -3;
		}
	}

	device->last_output_report_time = now;
	device->last_output_report = device->output_report;

	return hid_write(device->hidDevice, reinterpret_cast<u8*>(&device->output_report), sizeof(ps_move_output_report));
}

void ps_move_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	ps_move_device* dev = static_cast<ps_move_device*>(device.get());
	if (!dev || !dev->hidDevice || !dev->config || !pad)
		return;

	cfg_pad* config = dev->config;

	const u8 speed_large = config->get_large_motor_speed(pad->m_vibrateMotors);

	dev->new_output_data |= dev->large_motor != speed_large;
	dev->large_motor = speed_large;

	if (send_output_report(dev) >= 0)
	{
		dev->new_output_data = false;
	}
}

void ps_move_handler::SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool /*player_led*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	std::shared_ptr<ps_move_device> device = get_hid_device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
		return;

	device->large_motor = large_motor;
	device->small_motor = small_motor;
	device->player_id = player_id;
	device->config = get_config(padId);

	ensure(device->config);

	if (r >= 0 && g >= 0 && b >= 0 && r <= 255 && g <= 255 && b <= 255)
	{
		device->config->colorR.set(r);
		device->config->colorG.set(g);
		device->config->colorB.set(b);
	}

	if (send_output_report(device.get()) >= 0)
	{
		device->new_output_data = false;
	}
}

u32 ps_move_handler::get_battery_level(const std::string& padId)
{
	const std::shared_ptr<ps_move_device> device = get_hid_device(padId);
	if (!device || !device->hidDevice)
	{
		return 0;
	}

	switch (device->battery_level)
	{
	case battery_status::usb_charging:
	case battery_status::usb_charged:
		return 100;
	default:
		break;
	}

	// 0 to 5
	return std::clamp<u32>(device->battery_level * 20, 0, 100);
}
