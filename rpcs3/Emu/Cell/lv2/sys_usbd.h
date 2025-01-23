#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Input/product_info.h"

class ppu_thread;

#define MAX_SYS_USBD_TRANSFERS 0x44

// PS3 internal codes
enum PS3StandardUsbErrors : u32
{
	HC_CC_NOERR     = 0x00,
	EHCI_CC_MISSMF  = 0x10,
	EHCI_CC_XACT    = 0x20,
	EHCI_CC_BABBLE  = 0x30,
	EHCI_CC_DATABUF = 0x40,
	EHCI_CC_HALTED  = 0x50,
};

enum PS3IsochronousUsbErrors : u8
{
	USBD_HC_CC_NOERR   = 0x00,
	USBD_HC_CC_MISSMF  = 0x01,
	USBD_HC_CC_XACT    = 0x02,
	USBD_HC_CC_BABBLE  = 0x04,
	USBD_HC_CC_DATABUF = 0x08,
};

enum SysUsbdEvents : u32
{
	SYS_USBD_ATTACH            = 0x01,
	SYS_USBD_DETACH            = 0x02,
	SYS_USBD_TRANSFER_COMPLETE = 0x03,
	SYS_USBD_TERMINATE         = 0x04,
};

// PS3 internal structures
struct UsbInternalDevice
{
	u8 device_high; // System flag maybe (used in generating actual device number)
	u8 device_low;  // Just a number identifying the device (used in generating actual device number)
	u8 unk3;        // ? Seems to always be 2?
	u8 unk4;        // ?
};

struct UsbDeviceRequest
{
	u8 bmRequestType;
	u8 bRequest;
	be_t<u16> wValue;
	be_t<u16> wIndex;
	be_t<u16> wLength;
};

struct UsbDeviceIsoRequest
{
	vm::ptr<void> buf;
	be_t<u32> start_frame;
	be_t<u32> num_packets;
	be_t<u16> packets[8];
};

error_code sys_usbd_initialize(ppu_thread& ppu, vm::ptr<u32> handle);
error_code sys_usbd_finalize(ppu_thread& ppu, u32 handle);
error_code sys_usbd_get_device_list(ppu_thread& ppu, u32 handle, vm::ptr<UsbInternalDevice> device_list, u32 max_devices);
error_code sys_usbd_get_descriptor_size(ppu_thread& ppu, u32 handle, u32 device_handle);
error_code sys_usbd_get_descriptor(ppu_thread& ppu, u32 handle, u32 device_handle, vm::ptr<void> descriptor, u32 desc_size);
error_code sys_usbd_register_ldd(ppu_thread& ppu, u32 handle, vm::cptr<char> s_product, u16 slen_product);
error_code sys_usbd_unregister_ldd(ppu_thread& ppu, u32 handle, vm::cptr<char> s_product, u16 slen_product);
error_code sys_usbd_open_pipe(ppu_thread& ppu, u32 handle, u32 device_handle, u32 unk1, u64 unk2, u64 unk3, u32 endpoint, u64 unk4);
error_code sys_usbd_open_default_pipe(ppu_thread& ppu, u32 handle, u32 device_handle);
error_code sys_usbd_close_pipe(ppu_thread& ppu, u32 handle, u32 pipe_handle);
error_code sys_usbd_receive_event(ppu_thread& ppu, u32 handle, vm::ptr<u64> arg1, vm::ptr<u64> arg2, vm::ptr<u64> arg3);
error_code sys_usbd_detect_event(ppu_thread& ppu);
error_code sys_usbd_attach(ppu_thread& ppu, u32 handle, u32 unk1, u32 unk2, u32 device_handle);
error_code sys_usbd_transfer_data(ppu_thread& ppu, u32 handle, u32 id_pipe, vm::ptr<u8> buf, u32 buf_size, vm::ptr<UsbDeviceRequest> request, u32 type_transfer);
error_code sys_usbd_isochronous_transfer_data(ppu_thread& ppu, u32 handle, u32 id_pipe, vm::ptr<UsbDeviceIsoRequest> iso_request);
error_code sys_usbd_get_transfer_status(ppu_thread& ppu, u32 handle, u32 id_transfer, u32 unk1, vm::ptr<u32> result, vm::ptr<u32> count);
error_code sys_usbd_get_isochronous_transfer_status(ppu_thread& ppu, u32 handle, u32 id_transfer, u32 unk1, vm::ptr<UsbDeviceIsoRequest> request, vm::ptr<u32> result);
error_code sys_usbd_get_device_location(ppu_thread& ppu, u32 handle, u32 device_handle, vm::ptr<u8> location);
error_code sys_usbd_send_event(ppu_thread& ppu);
error_code sys_usbd_event_port_send(ppu_thread& ppu, u32 handle, u64 arg1, u64 arg2, u64 arg3);
error_code sys_usbd_allocate_memory(ppu_thread& ppu);
error_code sys_usbd_free_memory(ppu_thread& ppu);
error_code sys_usbd_get_device_speed(ppu_thread& ppu);
error_code sys_usbd_register_extra_ldd(ppu_thread& ppu, u32 handle, vm::cptr<char> s_product, u16 slen_product, u16 id_vendor, u16 id_product_min, u16 id_product_max);
error_code sys_usbd_unregister_extra_ldd(ppu_thread& ppu, u32 handle, vm::cptr<char> s_product, u16 slen_product);

void connect_usb_controller(u8 index, input::product_type);
void handle_hotplug_event(bool connected);
