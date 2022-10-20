#pragma once

#include "Emu/Io/usb_device.h"
#include <queue>

class usb_device_usio : public usb_device_emulated
{

public:
	usb_device_usio(const std::array<u8, 7>& location);
	~usb_device_usio();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

private:
	void load_backup();
	void save_backup();
	void translate_input();
	void usio_write(u8 channel, u16 reg, const std::vector<u8>& data);
	void usio_read(u8 channel, u16 reg, u16 size);

private:
	bool test_on = false;
	bool test_key_pressed = false;
	bool coin_key_pressed = false;
	le_t<u16> coin_counter = 0;
	std::string usio_backup_path;
	fs::file usio_backup_file;
	std::queue<std::vector<u8>> q_replies;
};
