#pragma once

#include "Emu/system_utils.hpp"
#include "Emu/Io/usb_device.h"

class usb_device_usio : public usb_device_emulated
{
public:
	usb_device_usio(const std::array<u8, 7>& location);
	~usb_device_usio();

	static std::shared_ptr<usb_device> make_instance(u32 controller_index, const std::array<u8, 7>& location);
	static u16 get_num_emu_devices();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

private:
	void load_backup();
	void save_backup();
	void translate_input_taiko();
	void translate_input_tekken();
	void usio_write(u8 channel, u16 reg, std::vector<u8>& data);
	void usio_read(u8 channel, u16 reg, u16 size);
	void usio_init(u8 channel, u16 reg, u16 size);

private:
	bool is_used = false;
	const std::string usio_backup_path = rpcs3::utils::get_hdd1_dir() + "/caches/usiobackup.bin";
	std::vector<u8> response;

	struct io_status
	{
		bool test_on = false;
		bool test_key_pressed = false;
		bool coin_key_pressed = false;
		le_t<u16> coin_counter = 0;
	};

	std::array<io_status, 2> m_io_status;
};
