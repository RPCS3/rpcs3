#include "stdafx.h"
#include "skateboard_pad_handler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(skateboard_log, "Skateboard");

namespace
{
	constexpr id_pair SKATEBOARD_ID_0 = {0x12BA, 0x0400}; // Tony Hawk RIDE Skateboard

	enum
	{
		SKATEBOARD_COMMON_REPORT_SIZE = 1
	};

	struct output_report_common
	{
		u8 report_id;
	};

	static_assert(sizeof(output_report_common) == SKATEBOARD_COMMON_REPORT_SIZE);
}

skateboard_pad_handler::skateboard_pad_handler()
    : hid_pad_handler<skateboard_device>(pad_handler::skateboard, {SKATEBOARD_ID_0})
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
		{ skateboard_key_codes::none, "" },
	};

	init_configs();

	// Define border values
	thumb_max = 255;
	trigger_min = 0;
	trigger_max = 255;

	// Set capabilities
	b_has_config = true;
	b_has_rumble = false;
	b_has_motion = false;
	b_has_deadzones = false;
	b_has_led = false;
	b_has_rgb = false;
	b_has_player_led = false;
	b_has_battery = false;
	b_has_pressure_intensity_button = false;

	m_name_string = "Skateboard #";
	m_max_devices = CELL_PAD_MAX_PORT_NUM;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold   = thumb_max / 2;
}

void skateboard_pad_handler::check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view wide_serial)
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

	std::string serial;
	for (wchar_t ch : wide_serial)
		serial += static_cast<uchar>(ch);

	std::array<u8, 64> buf{};
	buf[0] = 0x20;

	if (hid_set_nonblocking(hidDevice, 1) == -1)
	{
		skateboard_log.error("check_add_device: hid_set_nonblocking failed! Reason: %s", hid_error(hidDevice));
		hid_close(hidDevice);
		return;
	}

	device->hidDevice = hidDevice;
	device->path = path;

	// Activate
	if (send_output_report(device) == -1)
	{
		skateboard_log.error("check_add_device: send_output_report failed! Reason: %s", hid_error(hidDevice));
	}

	// Get bluetooth information
	get_data(device);

	skateboard_log.notice("Added device: serial='%s', path='%s'", serial, device->path);
}

void skateboard_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	// TODO:
	cfg->ls_left.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->ls_down.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->ls_right.def = ::at32(button_list, skateboard_key_codes::none);
	cfg->ls_up.def    = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_left.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_down.def  = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_right.def = ::at32(button_list, skateboard_key_codes::none);
	cfg->rs_up.def    = ::at32(button_list, skateboard_key_codes::none);
	cfg->start.def    = ::at32(button_list, skateboard_key_codes::none);
	cfg->select.def   = ::at32(button_list, skateboard_key_codes::none);
	cfg->ps.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->square.def   = ::at32(button_list, skateboard_key_codes::none);
	cfg->cross.def    = ::at32(button_list, skateboard_key_codes::none);
	cfg->circle.def   = ::at32(button_list, skateboard_key_codes::none);
	cfg->triangle.def = ::at32(button_list, skateboard_key_codes::none);
	cfg->left.def     = ::at32(button_list, skateboard_key_codes::none);
	cfg->down.def     = ::at32(button_list, skateboard_key_codes::none);
	cfg->right.def    = ::at32(button_list, skateboard_key_codes::none);
	cfg->up.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->r1.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->r2.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->r3.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->l1.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->l2.def       = ::at32(button_list, skateboard_key_codes::none);
	cfg->l3.def       = ::at32(button_list, skateboard_key_codes::none);

	// Set default misc variables
	cfg->lstickdeadzone.def    = 40; // between 0 and 255
	cfg->rstickdeadzone.def    = 40; // between 0 and 255
	cfg->ltriggerthreshold.def = 0;  // between 0 and 255
	cfg->rtriggerthreshold.def = 0;  // between 0 and 255

	// apply defaults
	cfg->from_default();
}

skateboard_pad_handler::DataStatus skateboard_pad_handler::get_data(skateboard_device* device)
{
	if (!device)
		return DataStatus::ReadError;

	std::array<u8, 128> buf{};

	const int res = hid_read(device->hidDevice, buf.data(), 128);

	if (res == -1)
	{
		// looks like controller disconnected or read error
		return DataStatus::ReadError;
	}

	if (res == 0)
		return DataStatus::NoNewData;

	u8 offset = 0;
	memcpy(device->padData.data(), &buf[offset], device->padData.size());
	return DataStatus::NewData;
}

bool skateboard_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	// TODO
	return false && keyCode == 0;
}

bool skateboard_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	// TODO
	return false && keyCode == 0;
}

bool skateboard_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	// TODO
	return false && keyCode == 0;
}

bool skateboard_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	// TODO
	return false && keyCode == 0;
}

PadHandlerBase::connection skateboard_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	skateboard_device* skateboard_dev = static_cast<skateboard_device*>(device.get());
	if (!skateboard_dev || skateboard_dev->path.empty())
		return connection::disconnected;

	if (skateboard_dev->hidDevice == nullptr)
	{
		// try to reconnect
		hid_device* dev = hid_open_path(skateboard_dev->path.c_str());
		if (dev)
		{
			if (hid_set_nonblocking(dev, 1) == -1)
			{
				skateboard_log.error("Reconnecting Device %s: hid_set_nonblocking failed with error %s", skateboard_dev->path, hid_error(dev));
			}
			skateboard_dev->hidDevice = dev;
		}
		else
		{
			// nope, not there
			return connection::disconnected;
		}
	}

	if (get_data(skateboard_dev) == DataStatus::ReadError)
	{
		// this also can mean disconnected, either way deal with it on next loop and reconnect
		hid_close(skateboard_dev->hidDevice);
		skateboard_dev->hidDevice = nullptr;

		return connection::no_data;
	}

	return connection::connected;
}

void skateboard_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	skateboard_device* dev = static_cast<skateboard_device*>(device.get());
	if (!dev || !pad)
		return;

	// TODO
}

std::unordered_map<u64, u16> skateboard_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> key_buf;
	skateboard_device* dualsense_dev = static_cast<skateboard_device*>(device.get());
	if (!dualsense_dev)
		return key_buf;

	// TODO
	//const std::array<u8, 64>& buf = dualsense_dev->padData;

	return key_buf;
}

pad_preview_values skateboard_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
{
	// TODO
	return {};
}

skateboard_pad_handler::~skateboard_pad_handler()
{
}

int skateboard_pad_handler::send_output_report(skateboard_device* device)
{
	if (!device || !device->hidDevice)
		return -2;

	const cfg_pad* config = device->config;
	if (config == nullptr)
		return -2; // hid_write returns -1 on error

	output_report_common report{};

	// TODO

	return hid_write(device->hidDevice, &report.report_id, SKATEBOARD_COMMON_REPORT_SIZE);
}

void skateboard_pad_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	skateboard_device* dev = static_cast<skateboard_device*>(device.get());
	if (!dev || !dev->hidDevice || !dev->config || !pad)
		return;

	cfg_pad* config = dev->config;

	// TODO
	dev->new_output_data = false;

	if (dev->new_output_data)
	{
		if (send_output_report(dev) >= 0)
		{
			dev->new_output_data = false;
		}
	}
}

void skateboard_pad_handler::SetPadData(const std::string& padId, u8 player_id, u8 /*large_motor*/, u8 /*small_motor*/, s32 /*r*/, s32 /*g*/, s32 /*b*/, bool /*player_led*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	std::shared_ptr<skateboard_device> device = get_hid_device(padId);
	if (device == nullptr || device->hidDevice == nullptr)
		return;

	device->player_id = player_id;
	device->config = get_config(padId);

	ensure(device->config);

	// Start/Stop the engines :)
	//send_output_report(device.get());
}
