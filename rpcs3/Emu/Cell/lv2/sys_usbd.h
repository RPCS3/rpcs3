#pragma once

#include <queue>
#include <libusb.h>
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Io/usb_device.h"

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

// HLE usbd
void LIBUSB_CALL callback_transfer(struct libusb_transfer* transfer);

struct UsbTransfer
{
	u32 transfer_id = 0;

	s32 result = 0;
	u32 count  = 0;
	UsbDeviceIsoRequest iso_request;

	std::vector<u8> setup_buf;
	libusb_transfer* transfer = nullptr;
	bool busy                 = false;

	// For fake transfers
	bool fake           = false;
	u64 expected_time   = 0;
	s32 expected_result = 0;
	u32 expected_count  = 0;
};

struct UsbLdd
{
	std::string name;
	u16 id_vendor;
	u16 id_product_min;
	u16 id_product_max;
};

struct UsbPipe
{
	std::shared_ptr<usb_device> device = nullptr;
	u8 endpoint                        = 0;
};

class usb_handler_thread
{
public:
	usb_handler_thread();
	~usb_handler_thread();

	// Thread loop
	void operator()();

	// Called by the libusb callback function to notify transfer completion
	void transfer_complete(libusb_transfer* transfer);

	// LDDs handling functions
	u32 add_ldd(vm::ptr<char> s_product, u16 slen_product, u16 id_vendor, u16 id_product_min, u16 id_product_max);
	void check_devices_vs_ldds();

	// Pipe functions
	u32 open_pipe(u32 device_handle, u8 endpoint);
	bool close_pipe(u32 pipe_id);
	bool is_pipe(u32 pipe_id) const;
	const UsbPipe& get_pipe(u32 pipe_id) const;

	// Events related functions
	bool get_event(vm::ptr<u64>& arg1, vm::ptr<u64>& arg2, vm::ptr<u64>& arg3);
	void add_to_receive_queue(ppu_thread* ppu);

	// Transfers related functions
	u32 get_free_transfer_id();
	UsbTransfer& get_transfer(u32 transfer_id);

	// Map of devices actively handled by the ps3(device_id, device)
	std::map<u32, std::pair<UsbInternalDevice, std::shared_ptr<usb_device>>> handled_devices;
	// Fake transfers
	std::vector<UsbTransfer*> fake_transfers;

private:
	void send_message(u32 message, u32 tr_id);

private:
	// Counters for device IDs, transfer IDs and pipe IDs
	atomic_t<u8> dev_counter = 1;
	u32 transfer_counter     = 0;
	u32 pipe_counter         = 0x10; // Start at 0x10 only for tracing purposes

	// List of device drivers
	std::vector<UsbLdd> ldds;

	// List of pipes
	std::map<u32, UsbPipe> open_pipes;
	// Transfers infos
	std::array<UsbTransfer, 0x44> transfers;

	// Queue of pending usbd events
	std::queue<std::tuple<u32, u32, u32>> usbd_events;
	// sys_usbd_receive_event PPU Threads
	std::queue<ppu_thread*> receive_threads;

	// List of devices "connected" to the ps3
	std::vector<std::shared_ptr<usb_device>> usb_devices;

	libusb_context* ctx       = nullptr;
	atomic_t<bool> is_running = false;
};

s32 sys_usbd_initialize(vm::ptr<u32> handle);
s32 sys_usbd_finalize(ppu_thread& ppu, u32 handle);
s32 sys_usbd_get_device_list(u32 handle, vm::ptr<UsbInternalDevice> device_list, u32 max_devices);
s32 sys_usbd_get_descriptor_size(u32 handle, u32 device_handle);
s32 sys_usbd_get_descriptor(u32 handle, u32 device_handle, vm::ptr<void> descriptor, u32 desc_size);
s32 sys_usbd_register_ldd(u32 handle, vm::ptr<char> s_product, u16 slen_product);
s32 sys_usbd_unregister_ldd();
s32 sys_usbd_open_pipe(u32 handle, u32 device_handle, u32 unk1, u64 unk2, u64 unk3, u32 endpoint, u64 unk4);
s32 sys_usbd_open_default_pipe(u32 handle, u32 device_handle);
s32 sys_usbd_close_pipe(u32 handle, u32 pipe_handle);
s32 sys_usbd_receive_event(ppu_thread& ppu, u32 handle, vm::ptr<u64> arg1, vm::ptr<u64> arg2, vm::ptr<u64> arg3);
s32 sys_usbd_detect_event();
s32 sys_usbd_attach(u32 handle);
s32 sys_usbd_transfer_data(u32 handle, u32 id_pipe, vm::ptr<u8> buf, u32 buf_size, vm::ptr<UsbDeviceRequest> request, u32 type_transfer);
s32 sys_usbd_isochronous_transfer_data(u32 handle, u32 id_pipe, vm::ptr<UsbDeviceIsoRequest> iso_request);
s32 sys_usbd_get_transfer_status(u32 handle, u32 id_transfer, u32 unk1, vm::ptr<u32> result, vm::ptr<u32> count);
s32 sys_usbd_get_isochronous_transfer_status(u32 handle, u32 id_transfer, u32 unk1, vm::ptr<UsbDeviceIsoRequest> request, vm::ptr<u32> result);
s32 sys_usbd_get_device_location();
s32 sys_usbd_send_event();
s32 sys_usbd_event_port_send();
s32 sys_usbd_allocate_memory();
s32 sys_usbd_free_memory();
s32 sys_usbd_get_device_speed();
s32 sys_usbd_register_extra_ldd(u32 handle, vm::ptr<char> s_product, u16 slen_product, u16 id_vendor, u16 id_product_min, u16 id_product_max);
