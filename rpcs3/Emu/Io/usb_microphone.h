#pragma once

#include "Emu/Io/usb_device.h"

enum class MicType
{
	SingStar,
	Logitech,
	Rocksmith,
};

enum
{
	SET_CUR = 0x01,
	GET_CUR = 0x81,
	SET_MIN = 0x02,
	GET_MIN = 0x82,
	SET_MAX = 0x03,
	GET_MAX = 0x83,
};

class usb_device_mic : public usb_device_emulated
{
public:
	usb_device_mic(u32 controller_index, const std::array<u8, 7>& location, MicType mic_type);
	
	static std::shared_ptr<usb_device> make_singstar(u32 controller_index, const std::array<u8, 7>& location);
	static std::shared_ptr<usb_device> make_logitech(u32 controller_index, const std::array<u8, 7>& location);
	static std::shared_ptr<usb_device> make_rocksmith(u32 controller_index, const std::array<u8, 7>& location);
	static u16 get_num_emu_devices();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void isochronous_transfer(UsbTransfer* transfer) override;

private:
	u32 m_controller_index;
	MicType m_mic_type;
	u32 m_sample_rate;
	s16 m_volume[2];
};
