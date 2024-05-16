#pragma once

#include "Emu/Io/usb_device.h"

class usb_device_guncon3 : public usb_device_emulated
{
public:
	usb_device_guncon3(u32 controller_index, const std::array<u8, 7>& location);
	~usb_device_guncon3();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

private:
	u32 m_controller_index;
	std::array<u8, 8> m_key{};
};
