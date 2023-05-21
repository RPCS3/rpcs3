#include "stdafx.h"
#include "usb_vfs.h"

LOG_CHANNEL(usb_vfs);

usb_device_vfs::usb_device_vfs(const cfg::device_info& device_info, const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	const auto [vid, pid] = device_info.get_usb_ids();

	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
		UsbDeviceDescriptor{
			.bcdUSB = 0x0200,
			.bDeviceClass = 0x00,
			.bDeviceSubClass = 0x00,
			.bDeviceProtocol = 0x00,
			.bMaxPacketSize0 = 0x40,
			.idVendor = vid,
			.idProduct = pid,
			.bcdDevice = pid,
			.iManufacturer = 0x01,
			.iProduct = 0x02,
			.iSerialNumber = 0x03,
			.bNumConfigurations = 0x01});

	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
		UsbDeviceConfiguration{
			.wTotalLength = 0x0020,
			.bNumInterfaces = 0x01,
			.bConfigurationValue = 0x01,
			.iConfiguration = 0x00,
			.bmAttributes = 0x80,
			.bMaxPower = 0x32}));

	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
		UsbDeviceInterface{
			.bInterfaceNumber = 0x00,
			.bAlternateSetting = 0x00,
			.bNumEndpoints = 0x02,
			.bInterfaceClass = 0x08,
			.bInterfaceSubClass = 0x06,
			.bInterfaceProtocol = 0x50,
			.iInterface = 0x00}));

	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint{
			.bEndpointAddress = 0x81,
			.bmAttributes = 0x02,
			.wMaxPacketSize = 0x0200,
			.bInterval = 0xFF}));

	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint{
			.bEndpointAddress = 0x02,
			.bmAttributes = 0x02,
			.wMaxPacketSize = 0x0200,
			.bInterval = 0xFF}));

	strings = {"SMI Corporation", "USB DISK", device_info.serial}; // Manufacturer, Product, SerialNumber
}

usb_device_vfs::~usb_device_vfs()
{
}
