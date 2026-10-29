#include "stdafx.h"
#include "Skylander.h"
#include "Emu/Cell/lv2/sys_usbd.h"

LOG_CHANNEL(skylander_log, "skylander");

sky_portal g_skyportal;

u16 skylander_crc16(u16 init_value, const u8* buffer, u32 size)
{
	constexpr unsigned short CRC_CCITT_TABLE[256] = {
	    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210, 0x3273,
	    0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528,
	    0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 0x48C4, 0x58E5, 0x6886,
	    0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF,
	    0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5,
	    0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2,
	    0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB, 0x95A8,
	    0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691,
	    0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 0xCB7D, 0xDB5C, 0xEB3F,
	    0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64,
	    0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};

	u16 crc = init_value;

	for (u32 i = 0; i < size; i++)
	{
		const u16 tmp = (crc >> 8) ^ buffer[i];
		crc = (crc << 8) ^ CRC_CCITT_TABLE[tmp];
	}

	return crc;
}

void skylander::save()
{
	if (!sky_file)
	{
		skylander_log.error("Tried to save skylander to file but no skylander is active!");
		return;
	}

	{
		sky_file.seek(0, fs::seek_set);
		sky_file.write(data.data(), 0x40 * 0x10);
	}
}

void sky_portal::activate()
{
	std::lock_guard lock(sky_mutex);
	if (activated)
	{
		// If the portal was already active no change is needed
		return;
	}

	// If not we need to advertise change to all the figures present on the portal
	for (auto& s : skylanders)
	{
		if (s.status & 1)
		{
			s.queued_status.push(3);
			s.queued_status.push(1);
		}
	}

	activated = true;
}

void sky_portal::deactivate()
{
	std::lock_guard lock(sky_mutex);

	for (auto& s : skylanders)
	{
		// check if at the end of the updates there would be a figure on the portal
		if (!s.queued_status.empty())
		{
			s.status        = s.queued_status.back();
			s.queued_status = std::queue<u8>();
		}

		s.status &= 1;
	}

	activated = false;
}

void sky_portal::set_leds(u8 r, u8 g, u8 b)
{
	std::lock_guard lock(sky_mutex);
	this->r = r;
	this->g = g;
	this->b = b;
}

void sky_portal::get_status(u8* reply_buf)
{
	std::lock_guard lock(sky_mutex);

	u16 status = 0;

	for (int i = 7; i >= 0; i--)
	{
		auto& s = skylanders[i];

		if (!s.queued_status.empty())
		{
			s.status = s.queued_status.front();
			s.queued_status.pop();
		}

		status <<= 2;
		status |= s.status;
	}

	std::memset(reply_buf, 0, 0x20);
	reply_buf[0] = 0x53;
	write_to_ptr_unsafe<le_t<u16>>(reply_buf, 1, status);
	reply_buf[5] = interrupt_counter++;
	reply_buf[6] = 0x01;
}

void sky_portal::query_block(u8 sky_num, u8 block, u8* reply_buf)
{
	std::lock_guard lock(sky_mutex);

	const auto& thesky = skylanders[sky_num];

	reply_buf[0] = 'Q';
	reply_buf[2] = block;
	if (thesky.status & 1)
	{
		reply_buf[1] = (0x10 | sky_num);
		memcpy(reply_buf + 3, thesky.data.data() + (16 * block), 16);
	}
	else
	{
		reply_buf[1] = sky_num;
	}
}

void sky_portal::write_block(u8 sky_num, u8 block, const u8* to_write_buf, u8* reply_buf)
{
	std::lock_guard lock(sky_mutex);

	auto& thesky = skylanders[sky_num];

	reply_buf[0] = 'W';
	reply_buf[2] = block;

	if (thesky.status & 1)
	{
		reply_buf[1] = (0x10 | sky_num);
		memcpy(thesky.data.data() + (block * 16), to_write_buf, 16);
		thesky.save();
	}
	else
	{
		reply_buf[1] = sky_num;
	}
}

bool sky_portal::remove_skylander(u8 sky_num)
{
	std::lock_guard lock(sky_mutex);
	auto& thesky = skylanders[sky_num];

	if (thesky.status & 1)
	{
		thesky.status = 2;
		thesky.queued_status.push(2);
		thesky.queued_status.push(0);
		thesky.sky_file.close();
		return true;
	}

	return false;
}

u8 sky_portal::load_skylander(const std::array<u8, 0x40 * 0x10>& data, fs::file in_file, int requested_slot)
{
	std::lock_guard lock(sky_mutex);

	const u32 sky_serial = read_from_ptr<le_t<u32>>(data);
	u8 found_slot  = 0xFF;

	if (requested_slot >= 0 && requested_slot <= 7)
	{
		if ((skylanders[requested_slot].status & 1) == 0)
			found_slot = static_cast<u8>(requested_slot);
	}
	else
	{
		// mimics spot retaining on the portal
		for (u8 i = 0; i < 8; i++)
		{
			if ((skylanders[i].status & 1) == 0)
			{
				if (skylanders[i].last_id == sky_serial)
				{
					found_slot = i;
					break;
				}

				if (i < found_slot)
				{
					found_slot = i;
				}
			}
		}
	}

	if (found_slot == 0xFF)
		return 0xFF;

	skylander& thesky = skylanders[found_slot];
	memcpy(thesky.data.data(), data.data(), thesky.data.size());
	thesky.sky_file = std::move(in_file);
	thesky.status   = 3;
	thesky.queued_status.push(3);
	thesky.queued_status.push(1);
	thesky.last_id = sky_serial;

	return found_slot;
}

void sky_portal::get_figure_info(u8 sky_num, u8& out_status, u16& out_id, u16& out_variant)
{
	ensure(sky_num < 8);
	std::lock_guard lock(sky_mutex);
	const auto& s = skylanders[sky_num];
	out_status = s.status;
	if (s.status & 1)
	{
		out_id      = read_from_ptr<le_t<u16>>(s.data, 0x10);
		out_variant = read_from_ptr<le_t<u16>>(s.data, 0x1C);
	}
	else
	{
		out_id      = 0;
		out_variant = 0;
	}
}

usb_device_skylander::usb_device_skylander(const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	device        = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x40, 0x1430, 0x0150, 0x0100, 0x01, 0x02, 0x00, 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x0029, 0x01, 0x01, 0x00, 0x80, 0xFA}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 0x001d}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x40, 0x01}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x02, 0x03, 0x40, 0x01}));
}

usb_device_skylander::~usb_device_skylander()
{
}

std::shared_ptr<usb_device> usb_device_skylander::make_instance(u32, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_skylander>(location);
}

u16 usb_device_skylander::get_num_emu_devices()
{
	return 1;
}

void usb_device_skylander::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// Control transfers are nearly instant
	switch (bmRequestType)
	{
	// HID Host 2 Device
	case 0x21:
		switch (bRequest)
		{
		case 0x09:
			transfer->expected_count  = buf_size;
			transfer->expected_result = HC_CC_NOERR;
			// 100 usec, control transfers are very fast
			transfer->expected_time = get_timestamp() + 100;

			std::array<u8, 32> q_result = {};

			switch (buf[0])
			{
			case 'A':
			{
				// Activate command
				ensure(buf_size == 2 || buf_size == 32);
				q_result = {0x41, buf[1], 0xFF, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				    0x00, 0x00};
				q_queries.push(q_result);
				g_skyportal.activate();
				break;
			}
			case 'C':
			{
				// Set LEDs colour
				ensure(buf_size == 4 || buf_size == 32);
				g_skyportal.set_leds(buf[1], buf[2], buf[3]);
				break;
			}
			case 'J':
			{
				// Sync status from game?
				ensure(buf_size == 7);
				q_result[0] = 0x4A;
				q_queries.push(q_result);
				break;
			}
			case 'L':
			{
				// Trap Team Portal Side Lights
				ensure(buf_size == 5);
				// TODO Proper Light side structs
				break;
			}
			case 'M':
			{
				// Audio Firmware version
				// Return version of 0 to prevent attempts to
				// play audio on the portal
				ensure(buf_size == 2);
				q_result = {0x4D, buf[1], 0x00, 0x19};
				q_queries.push(q_result);
				break;
			}
			case 'Q':
			{
				// Queries a block
				ensure(buf_size == 3 || buf_size == 32);

				const u8 sky_num = buf[1] & 0xF;
				ensure(sky_num < 8);
				const u8 block = buf[2];
				ensure(block < 0x40);

				g_skyportal.query_block(sky_num, block, q_result.data());
				q_queries.push(q_result);
				break;
			}
			case 'R':
			{
				// Shutdowns the portal
				ensure(buf_size == 2 || buf_size == 32);
				q_result = {
				    0x52, 0x02, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				q_queries.push(q_result);
				g_skyportal.deactivate();
				break;
			}
			case 'S':
			{
				// ?
				ensure(buf_size == 1 || buf_size == 32);
				break;
			}
			case 'V':
			{
				// ?
				ensure(buf_size == 4);
				break;
			}
			case 'W':
			{
				// Writes a block
				ensure(buf_size == 19 || buf_size == 32);

				const u8 sky_num = buf[1] & 0xF;
				ensure(sky_num < 8);
				const u8 block = buf[2];
				ensure(block < 0x40);

				g_skyportal.write_block(sky_num, block, &buf[3], q_result.data());
				q_queries.push(q_result);
				break;
			}
			default:
				skylander_log.error("Unhandled Query: buf_size=0x%02X, Type=0x%02X, bRequest=0x%02X, bmRequestType=0x%02X", buf_size, (buf_size > 0) ? buf[0] : -1, bRequest, bmRequestType);
				break;
			}
			break;
		}
		break;
	default:
		// Follow to default emulated handler
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
		break;
	}
}

void usb_device_skylander::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	ensure(buf_size == 0x20);

	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;

	if (endpoint == 0x02)
	{
		// Audio transfers are fairly quick(~1ms)
		transfer->expected_time = get_timestamp() + 1000;
		// The response is simply the request, echoed back
	}
	else
	{
		// Interrupt transfers are slow(~22ms)
		transfer->expected_time = get_timestamp() + 22000;
		if (!q_queries.empty())
		{
			memcpy(buf, q_queries.front().data(), 0x20);
			q_queries.pop();
		}
		else
		{
			g_skyportal.get_status(buf);
		}
	}
}
