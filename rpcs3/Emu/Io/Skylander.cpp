#include "stdafx.h"
#include "Skylander.h"
#include "Emu/Cell/lv2/sys_usbd.h"

LOG_CHANNEL(skylander_log, "skylander");

sky_portal g_skylander;

void sky_portal::sky_load()
{
	if (!sky_file)
	{
		skylander_log.error("Tried to load skylander but no skylander is active");
		return;
	}

	{
		std::lock_guard lock(sky_mutex);

		sky_file.seek(0, fs::seek_set);
		sky_file.read(sky_dump, 0x40 * 0x10);
	}
}

void sky_portal::sky_save()
{
	if (!sky_file)
	{
		skylander_log.error("Tried to save skylander to file but no skylander is active");
		return;
	}

	{
		std::lock_guard lock(sky_mutex);

		sky_file.seek(0, fs::seek_set);
		sky_file.write(sky_dump, 0x40 * 0x10);
	}
}

usb_device_skylander::usb_device_skylander()
{
	device        = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x20, 0x1430, 0x0150, 0x0100, 0x01, 0x02, 0x00, 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x0029, 0x01, 0x01, 0x00, 0x80, 0x96}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 0x001d}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x0020, 0x01}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x01, 0x03, 0x0020, 0x01}));
}

usb_device_skylander::~usb_device_skylander()
{
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
				// Activate command
				verify(HERE), buf_size == 2;
				q_result = {0x41, buf[1], 0xFF, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				    0x00, 0x00};

				q_queries.push(q_result);
				break;
			case 'C':
				// Set LEDs colour
				verify(HERE), buf_size == 4;
				break;
			case 'Q':
				// Queries a block
				verify(HERE), buf_size == 3;

				q_result[0] = 'Q';
				q_result[1] = 0x10;
				q_result[2] = buf[2];

				{
					std::lock_guard lock(g_skylander.sky_mutex);
					memcpy(q_result.data() + 3, g_skylander.sky_dump + (16 * buf[2]), 16);
				}

				q_queries.push(q_result);
				break;
			case 'R':
				// Reset
				verify(HERE), buf_size == 2;
				q_result = {
				    0x52, 0x02, 0x0A, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				q_queries.push(q_result);
				break;
			case 'S':
				// ?
				verify(HERE), buf_size == 1;
				break;
			case 'W':
				// Write a block
				verify(HERE), buf_size == 19;
				q_result[0] = 'W';
				q_result[1] = 0x10;
				q_result[2] = buf[2];

				{
					std::lock_guard lock(g_skylander.sky_mutex);
					memcpy(g_skylander.sky_dump + (buf[2] * 16), &buf[3], 16);
				}

				g_skylander.sky_save();

				q_queries.push(q_result);
				break;
			default: skylander_log.error("Unhandled Query Type: 0x%02X", buf[0]); break;
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
	verify(HERE), buf_size == 0x20;

	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	// Interrupt transfers are slow(6ms, TODO accurate measurement)
	transfer->expected_time = get_timestamp() + 6000;

	if (!q_queries.empty())
	{
		memcpy(buf, q_queries.front().data(), 0x20);
		q_queries.pop();
	}
	else
	{
		std::lock_guard lock(g_skylander.sky_mutex);
		memset(buf, 0, 0x20);
		buf[0] = 0x53;
		// Varies per skylander type
		buf[1] = (g_skylander.sky_file && !g_skylander.sky_reload) ? 1 : 0;

		buf[5] = interrupt_counter++;
		buf[6] = 0x01;

		g_skylander.sky_reload = false;
	}
}
