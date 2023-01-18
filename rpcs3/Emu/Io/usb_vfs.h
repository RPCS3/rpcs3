#pragma once

#include "Emu/Io/usb_device.h"
#include "Utilities/Config.h"

class usb_device_vfs : public usb_device_emulated
{
public:
	usb_device_vfs(const cfg::device_info& device_info, const std::array<u8, 7>& location);
	~usb_device_vfs();
};
