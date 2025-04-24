#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Io/usb_device.h"
#include "Utilities/StrUtil.h"
#include <libusb.h>

LOG_CHANNEL(sys_usbd);

extern void LIBUSB_CALL callback_transfer(struct libusb_transfer* transfer);

//////////////////////////////////////////////////////////////////
// ALL DEVICES ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

usb_device::usb_device(const std::array<u8, 7>& location)
{
	this->location = location;
}

void usb_device::get_location(u8* location) const
{
	memcpy(location, this->location.data(), 7);
}

void usb_device::read_descriptors()
{
}

u32 usb_device::get_configuration(u8* buf)
{
	*buf = current_config;
	return sizeof(u8);
}

bool usb_device::set_configuration(u8 cfg_num)
{
	current_config = cfg_num;
	return true;
}

bool usb_device::set_interface(u8 int_num)
{
	current_interface = int_num;
	return true;
}

u64 usb_device::get_timestamp()
{
	return (get_system_time() - Emu.GetPauseTime());
}

//////////////////////////////////////////////////////////////////
// PASSTHROUGH DEVICE ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
usb_device_passthrough::usb_device_passthrough(libusb_device* _device, libusb_device_descriptor& desc, const std::array<u8, 7>& location)
	: usb_device(location), lusb_device(_device)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{desc.bcdUSB, desc.bDeviceClass, desc.bDeviceSubClass, desc.bDeviceProtocol, desc.bMaxPacketSize0, desc.idVendor, desc.idProduct,
														  desc.bcdDevice, desc.iManufacturer, desc.iProduct, desc.iSerialNumber, desc.bNumConfigurations});
}

usb_device_passthrough::~usb_device_passthrough()
{
	if (lusb_handle)
	{
		libusb_release_interface(lusb_handle, 0);
		libusb_close(lusb_handle);
	}

	if (lusb_device)
	{
		libusb_unref_device(lusb_device);
	}
}

void usb_device_passthrough::send_libusb_transfer(libusb_transfer* transfer)
{
	while (true)
	{
		auto res = libusb_submit_transfer(transfer);
		switch (res)
		{
		case LIBUSB_SUCCESS: return;
		case LIBUSB_ERROR_BUSY: continue;
		default:
		{
			sys_usbd.error("Unexpected error from libusb_submit_transfer: %d(%s)", res, libusb_error_name(res));
			return;
		}
		}
	}
}

bool usb_device_passthrough::open_device()
{
	if (libusb_open(lusb_device, &lusb_handle) == LIBUSB_SUCCESS)
	{
#ifdef __linux__
		libusb_set_auto_detach_kernel_driver(lusb_handle, true);
#endif
		return true;
	}

	return false;
}

void usb_device_passthrough::read_descriptors()
{
	// Directly getting configuration descriptors from the device instead of going through libusb parsing functions as they're not needed
	for (u8 index = 0; index < device._device.bNumConfigurations; index++)
	{
		u8 buf[1000];
		int ssize = libusb_control_transfer(lusb_handle, +LIBUSB_ENDPOINT_IN | +LIBUSB_REQUEST_TYPE_STANDARD | +LIBUSB_RECIPIENT_DEVICE, LIBUSB_REQUEST_GET_DESCRIPTOR, 0x0200 | index, 0, buf, 1000, 0);
		if (ssize < 0)
		{
			sys_usbd.fatal("Couldn't get the config from the device: %d(%s)", ssize, libusb_error_name(ssize));
			continue;
		}

		// Minimalistic parse
		auto& conf = device.add_node(UsbDescriptorNode(buf[0], buf[1], &buf[2]));

		for (int index = buf[0]; index < ssize;)
		{
			conf.add_node(UsbDescriptorNode(buf[index], buf[index + 1], &buf[index + 2]));
			index += buf[index];
		}
	}
}

u32 usb_device_passthrough::get_configuration(u8* buf)
{
	return (libusb_get_configuration(lusb_handle, reinterpret_cast<int*>(buf)) == LIBUSB_SUCCESS) ? sizeof(u8) : 0;
};

bool usb_device_passthrough::set_configuration(u8 cfg_num)
{
	usb_device::set_configuration(cfg_num);
	return (libusb_set_configuration(lusb_handle, cfg_num) == LIBUSB_SUCCESS);
};

bool usb_device_passthrough::set_interface(u8 int_num)
{
	usb_device::set_interface(int_num);
	return (libusb_claim_interface(lusb_handle, int_num) == LIBUSB_SUCCESS);
}

void usb_device_passthrough::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, [[maybe_unused]] u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	if (transfer->setup_buf.size() < buf_size + LIBUSB_CONTROL_SETUP_SIZE)
		transfer->setup_buf.resize(buf_size + LIBUSB_CONTROL_SETUP_SIZE);

	transfer->control_destbuf = (bmRequestType & LIBUSB_ENDPOINT_IN) ? buf : nullptr;

	libusb_fill_control_setup(transfer->setup_buf.data(), bmRequestType, bRequest, wValue, wIndex, buf_size);
	memcpy(transfer->setup_buf.data() + LIBUSB_CONTROL_SETUP_SIZE, buf, buf_size);
	libusb_fill_control_transfer(transfer->transfer, lusb_handle, transfer->setup_buf.data(), callback_transfer, transfer, 0);
	send_libusb_transfer(transfer->transfer);
}

void usb_device_passthrough::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	libusb_fill_interrupt_transfer(transfer->transfer, lusb_handle, endpoint, buf, buf_size, callback_transfer, transfer, 0);
	send_libusb_transfer(transfer->transfer);
}

void usb_device_passthrough::isochronous_transfer(UsbTransfer* transfer)
{
	// TODO actual endpoint
	// TODO actual size?
	libusb_fill_iso_transfer(transfer->transfer, lusb_handle, 0x81, static_cast<u8*>(transfer->iso_request.buf.get_ptr()), 0xFFFF, transfer->iso_request.num_packets, callback_transfer, transfer, 0);

	for (u32 index = 0; index < transfer->iso_request.num_packets; index++)
	{
		transfer->transfer->iso_packet_desc[index].length = transfer->iso_request.packets[index];
	}

	send_libusb_transfer(transfer->transfer);
}

//////////////////////////////////////////////////////////////////
// EMULATED DEVICE ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
usb_device_emulated::usb_device_emulated(const std::array<u8, 7>& location)
	: usb_device(location)
{
}

usb_device_emulated::usb_device_emulated(const UsbDeviceDescriptor& _device, const std::array<u8, 7>& location)
	: usb_device(location)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, _device);
}

bool usb_device_emulated::open_device()
{
	return true;
}

u32 usb_device_emulated::get_descriptor(u8 type, u8 index, u8* buf, u32 buf_size)
{
	if (!buf)
	{
		return 0;
	}

	std::array<u8, 2> header;
	header = {static_cast<u8>(header.size()), type};

	u32 expected_count = std::min<u32>(static_cast<u32>(header.size()), buf_size);
	std::memcpy(buf, header.data(), expected_count);

	if (expected_count < header.size())
		return expected_count;

	switch (type)
	{
	case USB_DESCRIPTOR_DEVICE:
	{
		buf[0] = device.bLength;
		expected_count = std::min(device.bLength, ::narrow<u8>(buf_size));
		std::memcpy(buf + header.size(), device.data, expected_count - header.size());
		break;
	}
	case USB_DESCRIPTOR_CONFIG:
	{
		if (index < device.subnodes.size())
		{
			buf[0] = device.subnodes[index].bLength;
			expected_count = std::min(device.subnodes[index].bLength, ::narrow<u8>(buf_size));
			std::memcpy(buf + header.size(), device.subnodes[index].data, expected_count - header.size());
		}
		break;
	}
	case USB_DESCRIPTOR_STRING:
	{
		if (index < strings.size() + 1)
		{
			if (index == 0)
			{
				constexpr u8 len = static_cast<u8>(sizeof(u16) + header.size());
				buf[0] = len;
				expected_count = std::min(len, ::narrow<u8>(buf_size));
				constexpr le_t<u16> langid = 0x0409; // English (United States)
				std::memcpy(buf + header.size(), &langid, expected_count - header.size());
			}
			else
			{
				const std::u16string u16str = utf8_to_utf16(strings[index - 1]);
				const u8 len = static_cast<u8>(std::min(u16str.size() * sizeof(u16) + header.size(), static_cast<usz>(0xFF)));
				buf[0] = len;
				expected_count = std::min(len, ::narrow<u8>(std::min<u32>(255, buf_size)));
				std::memcpy(buf + header.size(), u16str.data(), expected_count - header.size());
			}
		}
		break;
	}
	default: sys_usbd.error("Unhandled DescriptorType: get_descriptor(type=0x%02x, index=0x%02x, buf=*0x%x, buf_size=0x%x)", type, index, buf, buf_size); break;
	}

	return expected_count;
}

u32 usb_device_emulated::get_status(bool self_powered, bool remote_wakeup, u8* buf, u32 buf_size)
{
	const u32 expected_count = buf ? std::min<u32>(sizeof(u16), buf_size) : 0;
	const u16 device_status = static_cast<int>(self_powered) | static_cast<int>(remote_wakeup) << 1;
	std::memcpy(buf, &device_status, expected_count);
	return expected_count;
}

void usb_device_emulated::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 /*wLength*/, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time   = usb_device::get_timestamp() + 100;

	switch (bmRequestType)
	{
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE: // 0x00
		switch (bRequest)
		{
		case LIBUSB_REQUEST_SET_CONFIGURATION: usb_device::set_configuration(::narrow<u8>(wValue)); break;
		default: sys_usbd.error("Unhandled control transfer(0x%02x): 0x%02x", bmRequestType, bRequest); break;
		}
		break;
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE: // 0x01
		switch (bRequest)
		{
		case LIBUSB_REQUEST_SET_INTERFACE: usb_device::set_interface(::narrow<u8>(wIndex)); break;
		default: sys_usbd.error("Unhandled control transfer(0x%02x): 0x%02x", bmRequestType, bRequest); break;
		}
		break;
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE: // 0x80
		switch (bRequest)
		{
		case LIBUSB_REQUEST_GET_STATUS: transfer->expected_count = get_status(false, false, buf, buf_size); break;
		case LIBUSB_REQUEST_GET_DESCRIPTOR: transfer->expected_count = get_descriptor(wValue >> 8, wValue & 0xFF, buf, buf_size); break;
		case LIBUSB_REQUEST_GET_CONFIGURATION: transfer->expected_count = get_configuration(buf); break;
		default: sys_usbd.error("Unhandled control transfer(0x%02x): 0x%02x", bmRequestType, bRequest); break;
		}
		break;
	default: sys_usbd.error("Unhandled control transfer: 0x%02x", bmRequestType); break;
	}
}

void usb_device_emulated::interrupt_transfer(u32 /*buf_size*/, u8* /*buf*/, u32 /*endpoint*/, UsbTransfer* /*transfer*/)
{
}

void usb_device_emulated::isochronous_transfer(UsbTransfer* /*transfer*/)
{
}

void usb_device_emulated::add_string(std::string str)
{
	strings.emplace_back(std::move(str));
}
