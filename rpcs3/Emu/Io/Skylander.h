#pragma once

#include "Emu/Io/usb_device.h"
#include <queue>
#include <mutex>

struct sky_portal
{
	std::mutex sky_mutex;
	fs::file sky_file;
	bool sky_reload          = false;
	u8 sky_dump[0x40 * 0x10] = {};

	void sky_save();
	void sky_load();
};

extern sky_portal g_skylander;

class usb_device_skylander : public usb_device_emulated
{
public:
	usb_device_skylander();
	~usb_device_skylander();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

protected:
	u8 interrupt_counter = 0;
	std::queue<std::array<u8, 32>> q_queries;
};
