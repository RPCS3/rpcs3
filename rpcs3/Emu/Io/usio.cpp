// v406 USIO emulator

#include "stdafx.h"
#include "usio.h"
#include "Input/pad_thread.h"
#include "Emu/Io/usio_config.h"
#include "Emu/IdManager.h"

LOG_CHANNEL(usio_log, "USIO");

template <>
void fmt_class_string<usio_btn>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](usio_btn value)
	{
		switch (value)
		{
		case usio_btn::test: return "Test";
		case usio_btn::coin: return "Coin";
		case usio_btn::service: return "Service";
		case usio_btn::enter: return "Enter/Start";
		case usio_btn::up: return "Up";
		case usio_btn::down: return "Down";
		case usio_btn::left: return "Left";
		case usio_btn::right: return "Right";
		case usio_btn::taiko_hit_side_left: return "Taiko Hit Side Left";
		case usio_btn::taiko_hit_side_right: return "Taiko Hit Side Right";
		case usio_btn::taiko_hit_center_left: return "Taiko Hit Center Left";
		case usio_btn::taiko_hit_center_right: return "Taiko Hit Center Right";
		case usio_btn::tekken_button1: return "Tekken Button 1";
		case usio_btn::tekken_button2: return "Tekken Button 2";
		case usio_btn::tekken_button3: return "Tekken Button 3";
		case usio_btn::tekken_button4: return "Tekken Button 4";
		case usio_btn::tekken_button5: return "Tekken Button 5";
		case usio_btn::count: return "Count";
		}

		return unknown;
	});
}

struct usio_memory
{
	std::vector<u8> backup_memory;

	usio_memory() = default;
	usio_memory(const usio_memory&) = delete;
	usio_memory& operator=(const usio_memory&) = delete;

	void init()
	{
		backup_memory.clear();
		backup_memory.resize(page_size * page_count);
	}

	static constexpr usz page_size = 0x10000;
	static constexpr usz page_count = 0x10;
};

usb_device_usio::usb_device_usio(const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	// Initialize dependencies
	g_fxo->need<usio_memory>();

	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
		UsbDeviceDescriptor{
			.bcdUSB             = 0x0110,
			.bDeviceClass       = 0xff,
			.bDeviceSubClass    = 0x00,
			.bDeviceProtocol    = 0xff,
			.bMaxPacketSize0    = 0x8,
			.idVendor           = 0x0b9a,
			.idProduct          = 0x0910,
			.bcdDevice          = 0x0910,
			.iManufacturer      = 0x01,
			.iProduct           = 0x02,
			.iSerialNumber      = 0x00,
			.bNumConfigurations = 0x01});

	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
		UsbDeviceConfiguration{
			.wTotalLength        = 39,
			.bNumInterfaces      = 0x01,
			.bConfigurationValue = 0x01,
			.iConfiguration      = 0x00,
			.bmAttributes        = 0xc0,
			.bMaxPower           = 0x32 // ??? 100ma
		}));

	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
		UsbDeviceInterface{
			.bInterfaceNumber   = 0x00,
			.bAlternateSetting  = 0x00,
			.bNumEndpoints      = 0x03,
			.bInterfaceClass    = 0x00,
			.bInterfaceSubClass = 0x00,
			.bInterfaceProtocol = 0x00,
			.iInterface         = 0x00}));

	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint{
			.bEndpointAddress = 0x01,
			.bmAttributes     = 0x02,
			.wMaxPacketSize   = 0x0040,
			.bInterval        = 0x00}));

	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint{
			.bEndpointAddress = 0x82,
			.bmAttributes     = 0x02,
			.wMaxPacketSize   = 0x0040,
			.bInterval        = 0x00}));

	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint{
			.bEndpointAddress = 0x83,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x0008,
			.bInterval        = 16}));

	load_backup();
}

usb_device_usio::~usb_device_usio()
{
	save_backup();
}

std::shared_ptr<usb_device> usb_device_usio::make_instance(u32, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_usio>(location);
}

u16 usb_device_usio::get_num_emu_devices()
{
	return 1;
}

void usb_device_usio::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// Control transfers are nearly instant
	//switch (bmRequestType)
	{
	//default:
		// Follow to default emulated handler
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
		//break;
	}
}

extern bool is_input_allowed();

void usb_device_usio::load_backup()
{
	usio_memory& memory = g_fxo->get<usio_memory>();
	memory.init();

	fs::file usio_backup_file;

	if (!usio_backup_file.open(usio_backup_path, fs::read))
	{
		usio_log.trace("Failed to load the USIO Backup file: %s", usio_backup_path);
		return;
	}

	const u64 file_size = memory.backup_memory.size();

	if (usio_backup_file.size() != file_size)
	{
		usio_log.trace("Invalid USIO Backup file detected: %s", usio_backup_path);
		return;
	}

	usio_backup_file.read(memory.backup_memory.data(), file_size);
}

void usb_device_usio::save_backup()
{
	if (!is_used)
		return;

	fs::file usio_backup_file;

	if (!usio_backup_file.open(usio_backup_path, fs::create + fs::write + fs::lock))
	{
		usio_log.error("Failed to save the USIO Backup file: %s", usio_backup_path);
		return;
	}

	const u64 file_size = g_fxo->get<usio_memory>().backup_memory.size();

	usio_backup_file.write(g_fxo->get<usio_memory>().backup_memory.data(), file_size);
	usio_backup_file.trunc(file_size);
}

void usb_device_usio::translate_input_taiko()
{
	std::lock_guard lock(pad::g_pad_mutex);
	const auto handler = pad::get_pad_thread();

	std::vector<u8> input_buf(0x60);
	constexpr le_t<u16> c_hit = 0x1800;
	le_t<u16> digital_input = 0;

	const auto translate_from_pad = [&](usz pad_number, usz player)
	{
		const usz offset = player * 8ULL;
		auto& status = m_io_status[0];

		if (const auto& pad = ::at32(handler->GetPads(), pad_number); pad->is_connected() && !pad->is_copilot() && is_input_allowed())
		{
			const auto& cfg = ::at32(g_cfg_usio.players, pad_number);
			cfg->handle_input(pad, false, [&](usio_btn btn, pad_button /*pad_btn*/, u16 /*value*/, bool pressed, bool& /*abort*/)
			{
				switch (btn)
				{
				case usio_btn::test:
					if (player != 0) break;
					if (pressed && !status.test_key_pressed) // Solve the need to hold the Test key
						status.test_on = !status.test_on;
					status.test_key_pressed = pressed;
					break;
				case usio_btn::coin:
					if (player != 0) break;
					if (pressed && !status.coin_key_pressed) // Ensure only one coin is inserted each time the Coin key is pressed
						status.coin_counter++;
					status.coin_key_pressed = pressed;
					break;
				case usio_btn::service:
					if (player == 0 && pressed)
						digital_input |= 0x4000;
					break;
				case usio_btn::enter:
					if (player == 0 && pressed)
						digital_input |= 0x200;
					break;
				case usio_btn::up:
					if (player == 0 && pressed)
						digital_input |= 0x2000;
					break;
				case usio_btn::down:
					if (player == 0 && pressed)
						digital_input |= 0x1000;
					break;
				case usio_btn::taiko_hit_side_left:
					if (pressed)
						std::memcpy(input_buf.data() + 32 + offset, &c_hit, sizeof(u16));
					break;
				case usio_btn::taiko_hit_center_right:
					if (pressed)
						std::memcpy(input_buf.data() + 36 + offset, &c_hit, sizeof(u16));
					break;
				case usio_btn::taiko_hit_side_right:
					if (pressed)
						std::memcpy(input_buf.data() + 38 + offset, &c_hit, sizeof(u16));
					break;
				case usio_btn::taiko_hit_center_left:
					if (pressed)
						std::memcpy(input_buf.data() + 34 + offset, &c_hit, sizeof(u16));
					break;
				default:
					break;
				}
			});
		}

		if (player == 0 && status.test_on)
			digital_input |= 0x80;
	};

	for (usz i = 0; i < g_cfg_usio.players.size(); i++)
		translate_from_pad(i, i);

	std::memcpy(input_buf.data(), &digital_input, sizeof(u16));
	std::memcpy(input_buf.data() + 16, &m_io_status[0].coin_counter, sizeof(u16));

	response = std::move(input_buf);
}

void usb_device_usio::translate_input_tekken()
{
	std::lock_guard lock(pad::g_pad_mutex);
	const auto handler = pad::get_pad_thread();

	std::vector<u8> input_buf(0x180);
	le_t<u64> digital_input[2]{};
	le_t<u16> digital_input_lm = 0;

	const auto translate_from_pad = [&](usz pad_number, usz player)
	{
		const usz shift = (player % 2) * 24ULL;
		auto& status = m_io_status[player / 2];
		auto& input = digital_input[player / 2];

		if (const auto& pad = ::at32(handler->GetPads(), pad_number); pad->is_connected() && !pad->is_copilot() && is_input_allowed())
		{
			const auto& cfg = ::at32(g_cfg_usio.players, pad_number);
			cfg->handle_input(pad, false, [&](usio_btn btn, pad_button /*pad_btn*/, u16 /*value*/, bool pressed, bool& /*abort*/)
			{
				switch (btn)
				{
				case usio_btn::test:
					if (player % 2 != 0)
						break;
					if (pressed && !status.test_key_pressed) // Solve the need to hold the Test button
						status.test_on = !status.test_on;
					status.test_key_pressed = pressed;
					break;
				case usio_btn::coin:
					if (player % 2 != 0)
						break;
					if (pressed && !status.coin_key_pressed) // Ensure only one coin is inserted each time the Coin button is pressed
						status.coin_counter++;
					status.coin_key_pressed = pressed;
					break;
				case usio_btn::service:
					if (player % 2 == 0 && pressed)
						input |= 0x4000;
					break;
				case usio_btn::enter:
					if (pressed)
					{
						input |= 0x800000ULL << shift;
						if (player == 0)
							digital_input_lm |= 0x800;
					}
					break;
				case usio_btn::up:
					if (pressed)
					{
						input |= 0x200000ULL << shift;
						if (player == 0)
							digital_input_lm |= 0x200;
					}
					break;
				case usio_btn::down:
					if (pressed)
					{
						input |= 0x100000ULL << shift;
						if (player == 0)
							digital_input_lm |= 0x400;
					}
					break;
				case usio_btn::left:
					if (pressed)
					{
						input |= 0x80000ULL << shift;
						if (player == 0)
							digital_input_lm |= 0x2000;
					}
					break;
				case usio_btn::right:
					if (pressed)
					{
						input |= 0x40000ULL << shift;
						if (player == 0)
							digital_input_lm |= 0x4000;
					}
					break;
				case usio_btn::tekken_button1:
					if (pressed)
					{
						input |= 0x20000ULL << shift;
						if (player == 0)
							digital_input_lm |= 0x100;
					}
					break;
				case usio_btn::tekken_button2:
					if (pressed)
						input |= 0x10000ULL << shift;
					break;
				case usio_btn::tekken_button3:
					if (pressed)
						input |= 0x40000000ULL << shift;
					break;
				case usio_btn::tekken_button4:
					if (pressed)
						input |= 0x20000000ULL << shift;
					break;
				case usio_btn::tekken_button5:
					if (pressed)
						input |= 0x80000000ULL << shift;
					break;
				default:
					break;
				}
			});
		}

		if (player % 2 == 0 && status.test_on)
		{
			input |= 0x80;
			if (player == 0)
				digital_input_lm |= 0x1000;
		}
	};

	for (usz i = 0; i < g_cfg_usio.players.size(); i++)
		translate_from_pad(i, i);

	for (usz i = 0; i < 2; i++)
	{
		std::memcpy(input_buf.data() - i * 0x80 + 0x100, &digital_input[i], sizeof(u64));
		std::memcpy(input_buf.data() - i * 0x80 + 0x100 + 0x10, &m_io_status[i].coin_counter, sizeof(u16));
	}

	std::memcpy(input_buf.data(), &digital_input_lm, sizeof(u16));

	input_buf[2] = 0b00010000; // DIP switches, 8 in total

	response = std::move(input_buf);
}

void usb_device_usio::usio_write(u8 channel, u16 reg, std::vector<u8>& data)
{
	const auto get_u16 = [&](std::string_view usio_func) -> u16
	{
		if (data.size() != 2)
		{
			usio_log.error("data.size() is %d, expected 2 for get_u16 in %s", data.size(), usio_func);
		}
		return *reinterpret_cast<const le_t<u16>*>(data.data());
	};

	if (channel == 0)
	{
		switch (reg)
		{
		case 0x0002:
		{
			usio_log.trace("SetSystemError: 0x%04X", get_u16("SetSystemError"));
			break;
		}
		case 0x000A:
		{
			if (get_u16("ClearSram") == 0x6666)
			    usio_log.trace("ClearSram");
			break;
		}
		case 0x0028:
		{
			usio_log.trace("SetExpansionMode: 0x%04X", get_u16("SetExpansionMode"));
			break;
		}
		case 0x0048:
		case 0x0058:
		case 0x0068:
		case 0x0078:
		{
			usio_log.trace("SetHopperRequest(Hopper: %d, Request: 0x%04X)", (reg - 0x48) / 0x10, get_u16("SetHopperRequest"));
			break;
		}
		case 0x004A:
		case 0x005A:
		case 0x006A:
		case 0x007A:
		{
			usio_log.trace("SetHopperRequest(Hopper: %d, Limit: 0x%04X)", (reg - 0x4A) / 0x10, get_u16("SetHopperLimit"));
			break;
		}
		default:
		{
			usio_log.trace("Unhandled channel 0 register write(reg: 0x%04X, size: 0x%04X, data: %s)", reg, data.size(), fmt::buf_to_hexstring(data.data(), data.size()));
			break;
		}
		}
	}
	else if (channel >= 2)
	{
		const u8 page = channel - 2;
		usio_log.trace("Usio write of sram(page: 0x%02X, addr: 0x%04X, size: 0x%04X, data: %s)", page, reg, data.size(), fmt::buf_to_hexstring(data.data(), data.size()));
		auto& memory = g_fxo->get<usio_memory>().backup_memory;
		const usz addr_end = reg + data.size();
		if (data.size() > 0 && page < usio_memory::page_count && addr_end <= usio_memory::page_size)
			std::memcpy(&memory[usio_memory::page_size * page + reg], data.data(), data.size());
		else
			usio_log.error("Usio sram invalid write operation(page: 0x%02X, addr: 0x%04X, size: 0x%04X, data: %s)", page, reg, data.size(), fmt::buf_to_hexstring(data.data(), data.size()));
	}
	else
	{
		// Channel 1 is the endpoint for firmware update.
		// We are not using any firmware since this is emulation.
		usio_log.trace("Unsupported write operation(channel: 0x%02X, addr: 0x%04X, size: 0x%04X, data: %s)", channel, reg, data.size(), fmt::buf_to_hexstring(data.data(), data.size()));
	}
}

void usb_device_usio::usio_read(u8 channel, u16 reg, u16 size)
{
	if (channel == 0)
	{
		switch (reg)
		{
		case 0x0000:
		{
			// Get Buffer, rarely gives a reply on real HW
			// First U16 seems to be a timestamp of sort
			// Purpose seems related to connectivity check
			response = {0x7E, 0xE4, 0x00, 0x00, 0x74, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			break;
		}
		case 0x0080:
		{
			// Card reader check - 1
			response = {0x02, 0x03, 0x06, 0x00, 0xFF, 0x0F, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x10, 0x00};
			break;
		}
		case 0x7000:
		{
			// Card reader check - 2
			// No data returned
			break;
		}
		case 0x1000:
		{
			// Often called, gets input from usio for Tekken
			translate_input_tekken();
			break;
		}
		case 0x1080:
		{
			// Often called, gets input from usio for Taiko
			translate_input_taiko();
			break;
		}
		case 0x1800:
		case 0x1880:
		{
			// Seems to contain a few extra bytes of info in addition to the firmware string
			// Firmware
			// "NBGI.;USIO01;Ver1.00;JPN,Multipurpose with PPG."
			constexpr std::array<u8, 0x180> info {0x4E, 0x42, 0x47, 0x49, 0x2E, 0x3B, 0x55, 0x53, 0x49, 0x4F, 0x30, 0x31, 0x3B, 0x56, 0x65, 0x72, 0x31, 0x2E, 0x30, 0x30, 0x3B, 0x4A, 0x50, 0x4E, 0x2C, 0x4D, 0x75, 0x6C, 0x74, 0x69, 0x70, 0x75, 0x72, 0x70, 0x6F, 0x73, 0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x50, 0x50, 0x47, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0x42, 0x47, 0x49, 0x31, 0x3B, 0x55, 0x53, 0x49, 0x4F, 0x30, 0x31, 0x3B, 0x56, 0x65, 0x72, 0x31, 0x2E, 0x30, 0x30, 0x3B, 0x4A, 0x50, 0x4E, 0x2C, 0x4D, 0x75, 0x6C, 0x74, 0x69, 0x70, 0x75, 0x72, 0x70, 0x6F, 0x73, 0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x50, 0x50, 0x47, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x13, 0x00, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x75, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0x42, 0x47, 0x49, 0x32, 0x3B, 0x55, 0x53, 0x49, 0x4F, 0x30, 0x31, 0x3B, 0x56, 0x65, 0x72, 0x31, 0x2E, 0x30, 0x30, 0x3B, 0x4A, 0x50, 0x4E, 0x2C, 0x4D, 0x75, 0x6C, 0x74, 0x69, 0x70, 0x75, 0x72, 0x70, 0x6F, 0x73, 0x65, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x50, 0x50, 0x47, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x13, 0x00, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x75, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			response = {info.begin() + (reg - 0x1800), info.end()};
			break;
		}
		default:
		{
			usio_log.trace("Unhandled channel 0 register read(reg: 0x%04X, size: 0x%04X)", reg, size);
			break;
		}
		}
	}
	else if (channel >= 2)
	{
		const u8 page = channel - 2;
		usio_log.trace("Usio read of sram(page: 0x%02X, addr: 0x%04X, size: 0x%04X)", page, reg, size);
		auto& memory = g_fxo->get<usio_memory>().backup_memory;
		const usz addr_end = reg + size;
		if (size > 0 && page < usio_memory::page_count && addr_end <= usio_memory::page_size)
			response.insert(response.end(), memory.begin() + (usio_memory::page_size * page + reg), memory.begin() + (usio_memory::page_size * page + addr_end));
		else
			usio_log.error("Usio sram invalid read operation(page: 0x%02X, addr: 0x%04X, size: 0x%04X)", page, reg, size);
	}
	else
	{
		// Channel 1 is the endpoint for firmware update.
		// We are not using any firmware since this is emulation.
		usio_log.trace("Unsupported read operation(channel: 0x%02X, addr: 0x%04X, size: 0x%04X)", channel, reg, size);
	}

	response.resize(size); // Always resize the response vector to the given size
}

void usb_device_usio::usio_init(u8 channel, u16 reg, u16 size)
{
	if (channel == 0)
	{
		switch (reg)
		{
		case 0x0008:
		{
			usio_log.trace("USIO Reset");
			break;
		}
		case 0x000A:
		{
			usio_log.trace("USIO ClearSram");
			g_fxo->get<usio_memory>().init();
			break;
		}
		default:
		{
			usio_log.trace("Unhandled channel 0 register init(reg: 0x%04X, size: 0x%04X)", reg, size);
			break;
		}
		}
	}
	else
	{
		usio_log.trace("Unsupported init operation(channel: 0x%02X, addr: 0x%04X, size: 0x%04X)", channel, reg, size);
	}
}

void usb_device_usio::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	constexpr u8 USIO_COMMAND_WRITE = 0x90;
	constexpr u8 USIO_COMMAND_READ  = 0x10;
	constexpr u8 USIO_COMMAND_INIT  = 0xA0;

	static bool expecting_data = false;
	static std::vector<u8> usio_data;
	static u32 response_seek = 0;
	static u8 usio_channel   = 0;
	static u16 usio_register = 0;
	static u16 usio_length   = 0;

	transfer->fake            = true;
	transfer->expected_result = HC_CC_NOERR;
	// The latency varies per operation but it doesn't seem to matter for this device so let's go fast!
	transfer->expected_time = get_timestamp() + 1'000;

	is_used = true;

	switch (endpoint)
	{
	case 0x01:
	{
		// Write endpoint
		transfer->expected_count = buf_size;

		if (expecting_data)
		{
			usio_data.insert(usio_data.end(), buf, buf + buf_size);
			usio_length -= buf_size;

			if (usio_length == 0)
			{
				expecting_data = false;
				usio_write(usio_channel, usio_register, usio_data);
			}
			return;
		}

		// Commands
		if (buf_size != 6)
		{
			usio_log.error("Expected a command but buf_size != 6");
			return;
		}

		usio_channel  = buf[0] & 0xF;
		usio_register = *reinterpret_cast<le_t<u16>*>(&buf[2]);
		usio_length   = *reinterpret_cast<le_t<u16>*>(&buf[4]);

		if ((buf[0] & USIO_COMMAND_WRITE) == USIO_COMMAND_WRITE)
		{
			usio_log.trace("UsioWrite(Channel: 0x%02X, Register: 0x%04X, Length: 0x%04X)", usio_channel, usio_register, usio_length);
			if (((~(usio_register >> 8)) & 0xF0) != buf[1])
			{
				usio_log.error("Invalid UsioWrite command");
				return;
			}
			expecting_data = true;
			usio_data.clear();
		}
		else if ((buf[0] & USIO_COMMAND_READ) == USIO_COMMAND_READ)
		{
			usio_log.trace("UsioRead(Channel: 0x%02X, Register: 0x%04X, Length: 0x%04X)", usio_channel, usio_register, usio_length);
			response_seek = 0;
			response.clear();
			usio_read(usio_channel, usio_register, usio_length);
		}
		else if ((buf[0] & USIO_COMMAND_INIT) == USIO_COMMAND_INIT)
		{
			usio_log.trace("UsioInit(Channel: 0x%02X, Register: 0x%04X, Length: 0x%04X)", usio_channel, usio_register, usio_length);
			usio_init(usio_channel, usio_register, usio_length);
		}
		else
		{
			usio_log.error("Received an unexpected command: 0x%02X", buf[0]);
		}
		break;
	}
	case 0x82:
	{
		// Read endpoint
		const u32 size = std::min(buf_size, static_cast<u32>(response.size() - response_seek));
		memcpy(buf, response.data() + response_seek, size);
		response_seek += size;
		transfer->expected_count = size;
		break;
	}
	default:
		usio_log.error("Unhandled endpoint: 0x%x", endpoint);
		break;
	}
}
