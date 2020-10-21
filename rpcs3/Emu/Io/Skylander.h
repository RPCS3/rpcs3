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

class usb_device_skylander_emu : public usb_device_emulated
{
public:
	usb_device_skylander_emu();
	~usb_device_skylander_emu();

	static std::shared_ptr<usb_device> make_instance();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
	static u16 get_num_emu_devices();
protected:
	u8 interrupt_counter = 0;
	std::queue<std::array<u8, 32>> q_queries;
private:
	u8 instance_num;
	static u32 emulated_instances; // bit field for currently existing instances of emulated devices
};
