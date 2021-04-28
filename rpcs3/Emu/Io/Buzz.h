#pragma once

#include "Emu/Io/usb_device.h"

class usb_device_buzz : public usb_device_emulated
{
	int first_controller;
	int last_controller;

public:
	usb_device_buzz(int first_controller, int last_controller);
	~usb_device_buzz();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
};
