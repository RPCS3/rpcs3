#pragma once

#include "Emu/Io/usb_device.h"

class usb_device_ghltar : public usb_device_emulated
{
public:
	usb_device_ghltar();
	~usb_device_ghltar();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
};
