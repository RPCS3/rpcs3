#include "stdafx.h"
#include "Skylander.h"
#include "Emu/Cell/lv2/sys_usbd.h"

LOG_CHANNEL(skylander_log, "skylander");

sky_portal g_skyportal;

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
	write_to_ptr<le_t<u16>>(reply_buf, 1, status);
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

u8 sky_portal::load_skylander(u8* buf, fs::file in_file)
{
	std::lock_guard lock(sky_mutex);

	const u32 sky_serial = read_from_ptr<le_t<u32>>(buf);
	u8 found_slot  = 0xFF;

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

	ensure(found_slot != 0xFF);

	skylander& thesky = skylanders[found_slot];
	memcpy(thesky.data.data(), buf, thesky.data.size());
	thesky.sky_file = std::move(in_file);
	thesky.status   = 3;
	thesky.queued_status.push(3);
	thesky.queued_status.push(1);
	thesky.last_id = sky_serial;

	return found_slot;
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
