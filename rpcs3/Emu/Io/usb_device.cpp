#include "stdafx.h"
#include "Emu/System.h"

#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Io/usb_device.h"
#include <libusb.h>

LOG_CHANNEL(sys_usbd);

extern void LIBUSB_CALL callback_transfer(struct libusb_transfer* transfer);

//////////////////////////////////////////////////////////////////
// ALL DEVICES ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

usb_device::usb_device(std::string name) : name(name) {};

void usb_device::read_descriptors()
{
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

bool usb_device::get_device_location(u8* location)
{
	if (!location)
		return false;

	// for the dummy device we just give it a location of '0'
	memset(location, 0, MAX_USB_PATH_LEN);

	return true;
}

bool usb_device::get_device_speed(u8* speed)
{
	if (!speed)
		return false;
	*speed = SYS_USBD_DEVICE_SPEED_LS;
	return true;
}


//////////////////////////////////////////////////////////////////
// PASSTHROUGH DEVICE ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
usb_device_passthrough::usb_device_passthrough(std::string name, libusb_device* _device, libusb_device_descriptor& desc)
	: usb_device(name), lusb_device(_device)
{
	libusb_ref_device(_device);
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{desc.bcdUSB, desc.bDeviceClass, desc.bDeviceSubClass, desc.bDeviceProtocol, desc.bMaxPacketSize0, desc.idVendor, desc.idProduct,
	                                                      desc.bcdDevice, desc.iManufacturer, desc.iProduct, desc.iSerialNumber, desc.bNumConfigurations});
}

usb_device_passthrough::~usb_device_passthrough()
{
	if (lusb_handle)
	{
		// we claim interfaces when setting them, so on destruction we should release the current one
		libusb_release_interface(lusb_handle, current_interface);
		libusb_close(lusb_handle);
	}

	if (lusb_device)
		libusb_unref_device(lusb_device);
}

bool usb_device_passthrough::open_device()
{
	int rc = libusb_open(lusb_device, &lusb_handle);
	if (rc == LIBUSB_SUCCESS)
	{
		libusb_set_auto_detach_kernel_driver(lusb_handle, true);
		return true;
	}
	else
	{
		sys_usbd.error("Failed to open device for LDD(VID:0x%04X PID:0x%04X SN:0x%04X) : %s",
			device._device.idVendor, device._device.idProduct, device._device.iSerialNumber, libusb_strerror(rc));
		return false;
	}
}

void usb_device_passthrough::read_descriptors()
{
	// Directly getting configuration descriptors from the device instead of going through libusb parsing functions as they're not needed
	for (u8 index = 0; index < device._device.bNumConfigurations; index++)
	{
		u8 buf[1000];
		int ssize = libusb_control_transfer(lusb_handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE, LIBUSB_REQUEST_GET_DESCRIPTOR, 0x0200 | index, 0, buf, 1000, 0);
		if (ssize < 0)
		{
			sys_usbd.error("Couldn't get the config from the device: %d", ssize);
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

bool usb_device_passthrough::set_configuration(u8 cfg_num)
{
	// we avoid changing the configuration if we don't have to
	// this avoids lightweight usb reset
	int cur_cfg = -1;
	libusb_get_configuration(lusb_handle, &cur_cfg);
	usb_device::set_configuration(cfg_num);
	if (static_cast<u8>(cur_cfg) != cfg_num)
		return (libusb_set_configuration(lusb_handle, cfg_num) == LIBUSB_SUCCESS);
	else
		return true;
};

bool usb_device_passthrough::set_interface(u8 int_num)
{
	libusb_release_interface(lusb_handle, current_interface);
	usb_device::set_interface(int_num);
	return (libusb_claim_interface(lusb_handle, int_num) == LIBUSB_SUCCESS);
}

void usb_device_passthrough::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	if (transfer->setup_buf.size() < buf_size + 8)
		transfer->setup_buf.resize(buf_size + 8);

	libusb_fill_control_setup(transfer->setup_buf.data(), bmRequestType, bRequest, wValue, wIndex, buf_size);
	memcpy(transfer->setup_buf.data() + 8, buf, buf_size);
	libusb_fill_control_transfer(transfer->transfer, lusb_handle, transfer->setup_buf.data(), callback_transfer, transfer, 0);
	libusb_submit_transfer(transfer->transfer);
}

bool usb_device_passthrough::get_device_location(u8* location)
{
	if (!location)
		return false;
	return (libusb_get_port_numbers(lusb_device, location, 7)==LIBUSB_SUCCESS);
}

bool usb_device_passthrough::get_device_speed(u8* speed)
{
	if (!speed)
		return false;

	switch (libusb_get_device_speed(lusb_device))
	{
	case LIBUSB_SPEED_HIGH:
		*speed = SYS_USBD_DEVICE_SPEED_HS;
		return true;
	case LIBUSB_SPEED_FULL:
		*speed = SYS_USBD_DEVICE_SPEED_FS;
		return true;
	default:
		*speed = SYS_USBD_DEVICE_SPEED_LS;
		return true;
	}
}

bool usb_device_passthrough::get_descriptor(libusb_device_descriptor* desc)
{
	if(desc)
		return libusb_get_device_descriptor(lusb_device, desc) == LIBUSB_SUCCESS;
	return false;
}

void usb_device_passthrough::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	libusb_fill_interrupt_transfer(transfer->transfer, lusb_handle, endpoint, buf, buf_size, callback_transfer, transfer, 0);
	libusb_submit_transfer(transfer->transfer);
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

	libusb_submit_transfer(transfer->transfer);
}


//////////////////////////////////////////////////////////////////
// EMULATED DEVICE ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
usb_device_emulated::usb_device_emulated(std::string name) : usb_device(name)
{
}

usb_device_emulated::usb_device_emulated(std::string name, const UsbDeviceDescriptor& _device) : usb_device(name)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, _device);
}

std::shared_ptr<usb_device> usb_device_emulated::make_instance()
{
	sys_usbd.fatal("[Emulated]: Trying to make instance of [virtual] usb_device_emulated base class");
	return nullptr;
}

u8 usb_device_emulated::claim_next_available_instance_num()
{
	// NOTE: not thread safe,
	u8 instance = std::countr_one(emulated_instances);
	emulated_instances |= 1 << instance;
	return instance;
}

void usb_device_emulated::release_instance_num(u8 instance)
{
	emulated_instances &= ~(1 << instance);
}

u16 usb_device_emulated::get_num_emu_devices()
{
	return 0;
}

bool usb_device_emulated::open_device()
{
	return true;
}

s32 usb_device_emulated::get_descriptor(u8 type, u8 index, u8* ptr, u32 max_size)
{
	if (type == USB_DESCRIPTOR_STRING)
	{
		if (index < strings.size())
		{
			u8 string_len = ::narrow<u8>(strings[index].size());
			ptr[0]        = (string_len * 2) + 2;
			ptr[1]        = USB_DESCRIPTOR_STRING;
			for (u32 i = 0; i < string_len; i++)
			{
				ptr[2 + (i * 2)] = strings[index].data()[i];
				ptr[3 + (i * 2)] = 0;
			}
			return ptr[0];
		}
	}
	else
	{
		sys_usbd.error("[Emulated]: Trying to get a descriptor other than string descriptor");
	}

	return -1;
}

void usb_device_emulated::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time   = usb_device::get_timestamp() + 100;

	switch (bmRequestType)
	{
	case 0:
		switch (bRequest)
		{
		case 0x09: usb_device::set_configuration(::narrow<u8>(wValue)); break;
		default: sys_usbd.fatal("Unhandled control transfer(0): 0x%x", bRequest); break;
		}
		break;
	default: sys_usbd.fatal("Unhandled control transfer: 0x%x", bmRequestType); break;
	}
}

void usb_device_emulated::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
}

void usb_device_emulated::isochronous_transfer(UsbTransfer* transfer)
{
}

void usb_device_emulated::add_string(char* str)
{
	strings.emplace_back(str);
}
