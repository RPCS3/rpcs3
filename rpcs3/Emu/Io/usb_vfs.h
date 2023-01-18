#pragma once

#include "Emu/Io/usb_device.h"

class usb_device_vfs : public usb_device_emulated
{
public:
	usb_device_vfs(const std::array<u8, 7>& location, const u16 vid, const u16 pid, const std::string& serial);
	~usb_device_vfs();
};
