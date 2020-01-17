#include "stdafx.h"
#include "sys_usbd.h"

#include <queue>
#include <thread>
#include "Emu/Memory/vm.h"
#include "Emu/System.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "sys_ppu_thread.h"

#include "Emu/Io/usb_device.h"
#include "Emu/Io/Skylander.h"

#include <libusb.h>

LOG_CHANNEL(sys_usbd);

template <>
void fmt_class_string<libusb_transfer>::format(std::string& out, u64 arg)
{
	const auto& transfer = get_object(arg);

	std::string datrace;
	const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	for (int index = 0; index < transfer.actual_length; index++)
	{
		datrace += hex[transfer.buffer[index] >> 4];
		datrace += hex[(transfer.buffer[index]) & 15];
		datrace += ' ';
	}

	fmt::append(out, "TR[r:%d][sz:%d] => %s", +transfer.status, transfer.actual_length, datrace);
}

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

	u8 endpoint = 0;
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
	void add_event(u64 arg1, u64 arg2, u64 arg3);

	// Transfers related functions
	u32 get_free_transfer_id();
	UsbTransfer& get_transfer(u32 transfer_id);

	// Map of devices actively handled by the ps3(device_id, device)
	std::map<u32, std::pair<UsbInternalDevice, std::shared_ptr<usb_device>>> handled_devices;
	// Fake transfers
	std::vector<UsbTransfer*> fake_transfers;

	shared_mutex mutex;
	atomic_t<bool> is_init = false;

	// sys_usbd_receive_event PPU Threads
	std::deque<ppu_thread*> sq;

	static constexpr auto thread_name = "Usb Manager Thread"sv;

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
	std::queue<std::tuple<u64, u64, u64>> usbd_events;

	// List of devices "connected" to the ps3
	std::vector<std::shared_ptr<usb_device>> usb_devices;

	libusb_context* ctx = nullptr;
};

void LIBUSB_CALL callback_transfer(struct libusb_transfer* transfer)
{
	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);
	if (!usbh->is_init)
		return;

	usbh->transfer_complete(transfer);
}

usb_handler_thread::usb_handler_thread()
{
	if (libusb_init(&ctx) < 0)
		return;

	// look if any device which we could be interested in is actually connected
	libusb_device** list;
	ssize_t ndev = libusb_get_device_list(ctx, &list);

	bool found_skylander = false;

	for (ssize_t index = 0; index < ndev; index++)
	{
		libusb_device_descriptor desc;
		libusb_get_device_descriptor(list[index], &desc);

		// clang-format off
		auto check_device = [&](const u16 id_vendor, const u16 id_product_min, const u16 id_product_max, const char* s_name) -> bool
		{
			if (desc.idVendor == id_vendor && desc.idProduct >= id_product_min && desc.idProduct <= id_product_max)
			{
				sys_usbd.success("Found device: %s", s_name);
				libusb_ref_device(list[index]);
				std::shared_ptr<usb_device_passthrough> usb_dev = std::make_shared<usb_device_passthrough>(list[index], desc);
				usb_devices.push_back(usb_dev);
				return true;
			}
			return false;
		};
		// clang-format on

		if (check_device(0x1430, 0x0150, 0x0150, "Skylanders Portal"))
		{
			found_skylander = true;
		}
		check_device(0x1415, 0x0000, 0x0000, "Singstar Microphone");
		check_device(0x12BA, 0x0100, 0x0100, "Guitar Hero Guitar");
		check_device(0x12BA, 0x0120, 0x0120, "Guitar Hero Drums");
		check_device(0x12BA, 0x074B, 0x074B, "Guitar Hero Live Guitar");
		check_device(0x12BA, 0x0140, 0x0140, "DJ Hero Turntable");
		check_device(0x12BA, 0x0200, 0x020F, "Harmonix Guitar");
		check_device(0x12BA, 0x0210, 0x021F, "Harmonix Drums");
		check_device(0x12BA, 0x2330, 0x233F, "Harmonix Keyboard");
		check_device(0x12BA, 0x2430, 0x243F, "Harmonix Button Guitar");
		check_device(0x12BA, 0x2530, 0x253F, "Harmonix Real Guitar");

		// GT5 Wheels&co
		check_device(0x046D, 0xC283, 0xC29B, "lgFF_c283_c29b");
		check_device(0x044F, 0xB653, 0xB653, "Thrustmaster RGT FFB Pro");
		check_device(0x044F, 0xB65A, 0xB65A, "Thrustmaster F430");
		check_device(0x044F, 0xB65D, 0xB65D, "Thrustmaster FFB");
		check_device(0x044F, 0xB65E, 0xB65E, "Thrustmaster TRS");
		check_device(0x044F, 0xB660, 0xB660, "Thrustmaster T500 RS Gear Shift");

		// Buzz controllers
		check_device(0x054C, 0x1000, 0x1040, "buzzer0");
		check_device(0x054C, 0x0001, 0x0041, "buzzer1");
		check_device(0x054C, 0x0042, 0x0042, "buzzer2");
		check_device(0x046D, 0xC220, 0xC220, "buzzer9");
	}

	if (ndev > 0)
		libusb_free_device_list(list, 1);

	if (!found_skylander)
	{
		sys_usbd.notice("Adding emulated skylander");
		usb_devices.push_back(std::make_shared<usb_device_skylander>());
	}

	for (u32 index = 0; index < MAX_SYS_USBD_TRANSFERS; index++)
	{
		transfers[index].transfer    = libusb_alloc_transfer(8);
		transfers[index].transfer_id = index;
	}
}

usb_handler_thread::~usb_handler_thread()
{
	// Ensures shared_ptr<usb_device> are all cleared before terminating libusb
	handled_devices.clear();
	open_pipes.clear();
	usb_devices.clear();

	if (ctx)
		libusb_exit(ctx);

	for (u32 index = 0; index < MAX_SYS_USBD_TRANSFERS; index++)
	{
		if (transfers[index].transfer)
			libusb_free_transfer(transfers[index].transfer);
	}
}

void usb_handler_thread::operator()()
{
	timeval lusb_tv{0, 200};

	while (thread_ctrl::state() != thread_state::aborting)
	{
		// Todo: Hotplug here?

		// Process asynchronous requests that are pending
		libusb_handle_events_timeout_completed(ctx, &lusb_tv, nullptr);

		// Process fake transfers
		if (!fake_transfers.empty())
		{
			std::lock_guard lock(this->mutex);

			u64 timestamp = get_system_time() - Emu.GetPauseTime();

			for (auto it = fake_transfers.begin(); it != fake_transfers.end(); it++)
			{
				auto transfer = *it;

				ASSERT(transfer->busy && transfer->fake);

				if (transfer->expected_time > timestamp)
					continue;

				transfer->result = transfer->expected_result;
				transfer->count  = transfer->expected_count;
				transfer->fake   = false;
				transfer->busy   = false;

				fake_transfers.erase(it--);

				send_message(SYS_USBD_TRANSFER_COMPLETE, transfer->transfer_id);
			}
		}

		// If there is no handled devices usb thread is not actively needed
		if (!handled_devices.size())
			std::this_thread::sleep_for(500ms);
		else
			std::this_thread::sleep_for(200us);
	}
}

void usb_handler_thread::send_message(u32 message, u32 tr_id)
{
	sys_usbd.trace("Sending event: arg1=0x%x arg2=0x%x arg3=0x00", message, tr_id);

	add_event(message, tr_id, 0x00);
}

void usb_handler_thread::transfer_complete(struct libusb_transfer* transfer)
{
	UsbTransfer* usbd_transfer = static_cast<UsbTransfer*>(transfer->user_data);

	if (transfer->status != 0)
	{
		sys_usbd.error("Transfer Error: %d", +transfer->status);
	}

	switch (transfer->status)
	{
	case LIBUSB_TRANSFER_COMPLETED: usbd_transfer->result = HC_CC_NOERR; break;
	case LIBUSB_TRANSFER_TIMED_OUT: usbd_transfer->result = EHCI_CC_XACT; break;
	case LIBUSB_TRANSFER_OVERFLOW: usbd_transfer->result = EHCI_CC_BABBLE; break;
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_NO_DEVICE:
	default: usbd_transfer->result = EHCI_CC_HALTED; break;
	}

	usbd_transfer->count = transfer->actual_length;

	for (s32 index = 0; index < transfer->num_iso_packets; index++)
	{
		u8 iso_status;
		switch (transfer->iso_packet_desc[index].status)
		{
		case LIBUSB_TRANSFER_COMPLETED: iso_status = USBD_HC_CC_NOERR; break;
		case LIBUSB_TRANSFER_TIMED_OUT: iso_status = USBD_HC_CC_XACT; break;
		case LIBUSB_TRANSFER_OVERFLOW: iso_status = USBD_HC_CC_BABBLE; break;
		case LIBUSB_TRANSFER_ERROR:
		case LIBUSB_TRANSFER_CANCELLED:
		case LIBUSB_TRANSFER_STALL:
		case LIBUSB_TRANSFER_NO_DEVICE:
		default: iso_status = USBD_HC_CC_MISSMF; break;
		}

		usbd_transfer->iso_request.packets[index] = ((iso_status & 0xF) << 12 | (transfer->iso_packet_desc[index].actual_length & 0xFFF));
	}

	usbd_transfer->busy = false;

	send_message(SYS_USBD_TRANSFER_COMPLETE, usbd_transfer->transfer_id);

	sys_usbd.trace("Transfer complete(0x%x): %s", usbd_transfer->transfer_id, *transfer);
}

u32 usb_handler_thread::add_ldd(vm::ptr<char> s_product, u16 slen_product, u16 id_vendor, u16 id_product_min, u16 id_product_max)
{
	UsbLdd new_ldd;
	new_ldd.name.resize(slen_product);
	memcpy(new_ldd.name.data(), s_product.get_ptr(), slen_product);
	new_ldd.id_vendor      = id_vendor;
	new_ldd.id_product_min = id_product_min;
	new_ldd.id_product_max = id_product_max;
	ldds.push_back(new_ldd);

	return ::size32(ldds); // TODO: to check
}

u32 usb_handler_thread::open_pipe(u32 device_handle, u8 endpoint)
{
	open_pipes.emplace(pipe_counter, UsbPipe{handled_devices[device_handle].second, endpoint});
	return pipe_counter++;
}

bool usb_handler_thread::close_pipe(u32 pipe_id)
{
	return open_pipes.erase(pipe_id) != 0;
}

bool usb_handler_thread::is_pipe(u32 pipe_id) const
{
	return open_pipes.count(pipe_id) != 0;
}

const UsbPipe& usb_handler_thread::get_pipe(u32 pipe_id) const
{
	return open_pipes.at(pipe_id);
}

void usb_handler_thread::check_devices_vs_ldds()
{
	for (const auto& dev : usb_devices)
	{
		for (const auto& ldd : ldds)
		{
			if (dev->device._device.idVendor == ldd.id_vendor && dev->device._device.idProduct >= ldd.id_product_min && dev->device._device.idProduct <= ldd.id_product_max && !dev->assigned_number)
			{
				if (!dev->open_device())
				{
					sys_usbd.error("Failed to open device for LDD(VID:0x%x PID:0x%x)", dev->device._device.idVendor, dev->device._device.idProduct);
					continue;
				}

				sys_usbd.success("Ldd device matchup for <%s>", ldd.name);

				dev->read_descriptors();

				dev->assigned_number = dev_counter;
				handled_devices.emplace(dev_counter, std::pair(UsbInternalDevice{0x00, dev_counter, 0x02, 0x40}, dev));
				send_message(SYS_USBD_ATTACH, dev_counter);
				dev_counter++;
				return;
			}
		}
	}
}

bool usb_handler_thread::get_event(vm::ptr<u64>& arg1, vm::ptr<u64>& arg2, vm::ptr<u64>& arg3)
{
	if (usbd_events.size())
	{
		const auto& usb_event = usbd_events.front();
		*arg1                 = std::get<0>(usb_event);
		*arg2                 = std::get<1>(usb_event);
		*arg3                 = std::get<2>(usb_event);
		usbd_events.pop();
		sys_usbd.trace("Received event: arg1=0x%x arg2=0x%x arg3=0x%x", *arg1, *arg2, *arg3);
		return true;
	}

	return false;
}

void usb_handler_thread::add_event(u64 arg1, u64 arg2, u64 arg3)
{
	// sys_usbd events use an internal event queue with SYS_SYNC_PRIORITY protocol
	if (const auto cpu = lv2_obj::schedule<ppu_thread>(sq, SYS_SYNC_PRIORITY))
	{
		cpu->gpr[4] = arg1;
		cpu->gpr[5] = arg2;
		cpu->gpr[6] = arg3;
		lv2_obj::awake(cpu);
	}
	else
	{
		usbd_events.emplace(arg1, arg2, arg3);
	}
}

u32 usb_handler_thread::get_free_transfer_id()
{
	do
	{
		transfer_counter++;

		if (transfer_counter >= MAX_SYS_USBD_TRANSFERS)
			transfer_counter = 0;
	} while (transfers[transfer_counter].busy);

	return transfer_counter;
}

UsbTransfer& usb_handler_thread::get_transfer(u32 transfer_id)
{
	return transfers[transfer_id];
}

error_code sys_usbd_initialize(vm::ptr<u32> handle)
{
	sys_usbd.warning("sys_usbd_initialize(handle=*0x%x)", handle);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	// Must not occur (lv2 allows multiple handles, cellUsbd does not)
	verify("sys_usbd Initialized twice" HERE), !usbh->is_init.exchange(true);

	*handle       = 0x115B;

	// TODO
	return CELL_OK;
}

error_code sys_usbd_finalize(ppu_thread& ppu, u32 handle)
{
	sys_usbd.warning("sys_usbd_finalize(handle=0x%x)", handle);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);
	usbh->is_init = false;

	// Forcefully awake all waiters
	for (auto& cpu : decltype(usbh->sq)(std::move(usbh->sq)))
	{
		// Special ternimation signal value
		cpu->gpr[4] = 4;
		cpu->gpr[5] = 0;
		cpu->gpr[6] = 0;
		lv2_obj::awake(cpu);
	}

	// TODO
	return CELL_OK;
}

error_code sys_usbd_get_device_list(u32 handle, vm::ptr<UsbInternalDevice> device_list, u32 max_devices)
{
	sys_usbd.warning("sys_usbd_get_device_list(handle=0x%x, device_list=*0x%x, max_devices=0x%x)", handle, device_list, max_devices);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);
	if (!usbh->is_init)
		return CELL_EINVAL;

	// TODO: was std::min<s32>
	u32 i_tocopy = std::min<u32>(max_devices, ::size32(usbh->handled_devices));

	for (u32 index = 0; index < i_tocopy; index++)
	{
		device_list[index] = usbh->handled_devices[index].first;
	}

	return not_an_error(i_tocopy);
}

error_code sys_usbd_register_extra_ldd(u32 handle, vm::ptr<char> s_product, u16 slen_product, u16 id_vendor, u16 id_product_min, u16 id_product_max)
{
	sys_usbd.warning("sys_usbd_register_extra_ldd(handle=0x%x, s_product=%s, slen_product=0x%x, id_vendor=0x%x, id_product_min=0x%x, id_product_max=0x%x)",
		handle, s_product, slen_product, id_vendor, id_product_min, id_product_max);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);
	if (!usbh->is_init)
		return CELL_EINVAL;

	s32 res = usbh->add_ldd(s_product, slen_product, id_vendor, id_product_min, id_product_max);
	usbh->check_devices_vs_ldds();

	return not_an_error(res); // To check
}

error_code sys_usbd_get_descriptor_size(u32 handle, u32 device_handle)
{
	sys_usbd.trace("sys_usbd_get_descriptor_size(handle=0x%x, deviceNumber=0x%x)", handle, device_handle);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.count(device_handle))
	{
		return CELL_EINVAL;
	}

	return not_an_error(usbh->handled_devices[device_handle].second->device.get_size());
}

error_code sys_usbd_get_descriptor(u32 handle, u32 device_handle, vm::ptr<void> descriptor, u32 desc_size)
{
	sys_usbd.trace("sys_usbd_get_descriptor(handle=0x%x, deviceNumber=0x%x, descriptor=0x%x, desc_size=0x%x)", handle, device_handle, descriptor, desc_size);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.count(device_handle))
	{
		return CELL_EINVAL;
	}

	u8* ptr = static_cast<u8*>(descriptor.get_ptr());
	usbh->handled_devices[device_handle].second->device.write_data(ptr);

	return CELL_OK;
}

// This function is used for psp(cellUsbPspcm), dongles in ps3 arcade cabinets(PS3A-USJ), ps2 cam(eyetoy), generic usb camera?(sample_usb2cam)
error_code sys_usbd_register_ldd(u32 handle, vm::ptr<char> s_product, u16 slen_product)
{
	sys_usbd.todo("sys_usbd_register_ldd(handle=0x%x, s_product=%s, slen_product=0x%x)", handle, s_product, slen_product);
	return CELL_OK;
}

error_code sys_usbd_unregister_ldd()
{
	sys_usbd.todo("sys_usbd_unregister_ldd()");
	return CELL_OK;
}

// TODO: determine what the unknown params are
error_code sys_usbd_open_pipe(u32 handle, u32 device_handle, u32 unk1, u64 unk2, u64 unk3, u32 endpoint, u64 unk4)
{
	sys_usbd.warning("sys_usbd_open_pipe(handle=0x%x, device_handle=0x%x, unk1=0x%x, unk2=0x%x, unk3=0x%x, endpoint=0x%x, unk4=0x%x)",
		handle, device_handle, unk1, unk2, unk3, endpoint, unk4);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.count(device_handle))
	{
		return CELL_EINVAL;
	}

	return not_an_error(usbh->open_pipe(device_handle, static_cast<u8>(endpoint)));
}

error_code sys_usbd_open_default_pipe(u32 handle, u32 device_handle)
{
	sys_usbd.trace("sys_usbd_open_default_pipe(handle=0x%x, device_handle=0x%x)", handle, device_handle);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.count(device_handle))
	{
		return CELL_EINVAL;
	}

	return not_an_error(usbh->open_pipe(device_handle, 0));
}

error_code sys_usbd_close_pipe(u32 handle, u32 pipe_handle)
{
	sys_usbd.todo("sys_usbd_close_pipe(handle=0x%x, pipe_handle=0x%x)", handle, pipe_handle);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->is_pipe(pipe_handle))
	{
		return CELL_EINVAL;
	}

	usbh->close_pipe(pipe_handle);

	return CELL_OK;
}

// From RE:
// In libusbd_callback_thread
// *arg1 = 4 will terminate CellUsbd libusbd_callback_thread
// *arg1 = 3 will do some extra processing right away(notification of transfer finishing)
// *arg1 < 1 || *arg1 > 4 are ignored(rewait instantly for event)
// *arg1 == 1 || *arg1 == 2 will send a sys_event to internal CellUsbd event queue with same parameters as received and loop(attach and detach event)
error_code sys_usbd_receive_event(ppu_thread& ppu, u32 handle, vm::ptr<u64> arg1, vm::ptr<u64> arg2, vm::ptr<u64> arg3)
{
	sys_usbd.trace("sys_usbd_receive_event(handle=%u, arg1=*0x%x, arg2=*0x%x, arg3=*0x%x)", handle, arg1, arg2, arg3);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	{
		std::lock_guard lock(usbh->mutex);

		if (!usbh->is_init)
			return CELL_EINVAL;

		if (usbh->get_event(arg1, arg2, arg3))
		{
			// hack for Guitar Hero Live
			// Attaching the device too fast seems to result in a nullptr along the way
			if (*arg1 == SYS_USBD_ATTACH)
				lv2_obj::wait_timeout(5000);

			return CELL_OK;
		}

		lv2_obj::sleep(ppu);
		usbh->sq.emplace_back(&ppu);
	}

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (ppu.is_stopped())
		{
			return 0;
		}

		thread_ctrl::wait();
	}

	*arg1 = ppu.gpr[4];
	*arg2 = ppu.gpr[5];
	*arg3 = ppu.gpr[6];
	return CELL_OK;
}

error_code sys_usbd_detect_event()
{
	sys_usbd.todo("sys_usbd_detect_event()");
	return CELL_OK;
}

error_code sys_usbd_attach(u32 handle)
{
	sys_usbd.todo("sys_usbd_attach(handle=0x%x)", handle);
	return CELL_OK;
}

error_code sys_usbd_transfer_data(u32 handle, u32 id_pipe, vm::ptr<u8> buf, u32 buf_size, vm::ptr<UsbDeviceRequest> request, u32 type_transfer)
{
	sys_usbd.trace("sys_usbd_transfer_data(handle=0x%x, id_pipe=0x%x, buf=*0x%x, buf_length=0x%x, request=*0x%x, type=0x%x)", handle, id_pipe, buf, buf_size, request, type_transfer);
	if (sys_usbd.enabled == logs::level::trace && request)
	{
		sys_usbd.trace("RequestType:0x%x, Request:0x%x, wValue:0x%x, wIndex:0x%x, wLength:0x%x", request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength);
		if ((request->bmRequestType & 0x80) == 0 && buf && buf_size != 0)
		{
			std::string datrace;
			const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

			for (u32 index = 0; index < buf_size; index++)
			{
				datrace += hex[(buf[index] >> 4) & 15];
				datrace += hex[(buf[index]) & 15];
				datrace += ' ';
			}

			sys_usbd.trace("Control sent: %s", datrace);
		}
	}

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->is_pipe(id_pipe))
	{
		return CELL_EINVAL;
	}

	u32 id_transfer  = usbh->get_free_transfer_id();
	const auto& pipe = usbh->get_pipe(id_pipe);
	auto& transfer   = usbh->get_transfer(id_transfer);

	// Default endpoint is control endpoint
	if (pipe.endpoint == 0)
	{
		if (!request)
		{
			sys_usbd.error("Tried to use control pipe without proper request pointer");
			return CELL_EINVAL;
		}

		// Claiming interface
		if (request->bmRequestType == 0 && request->bRequest == 0x09)
		{
			pipe.device->set_configuration(static_cast<u8>(+request->wValue));
			pipe.device->set_interface(0);
		}

		pipe.device->control_transfer(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf_size, buf.get_ptr(), &transfer);
		transfer.busy = true;
	}
	else
	{
		pipe.device->interrupt_transfer(buf_size, buf.get_ptr(), pipe.endpoint, &transfer);
		transfer.busy = true;
	}

	if (transfer.fake)
		usbh->fake_transfers.push_back(&transfer);

	// returns an identifier specific to the transfer
	return not_an_error(id_transfer);
}

error_code sys_usbd_isochronous_transfer_data(u32 handle, u32 id_pipe, vm::ptr<UsbDeviceIsoRequest> iso_request)
{
	sys_usbd.todo("sys_usbd_isochronous_transfer_data(handle=0x%x, id_pipe=0x%x, iso_request=*0x%x)", handle, id_pipe, iso_request);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->is_pipe(id_pipe))
	{
		return CELL_EINVAL;
	}

	u32 id_transfer  = usbh->get_free_transfer_id();
	const auto& pipe = usbh->get_pipe(id_pipe);
	auto& transfer   = usbh->get_transfer(id_transfer);

	memcpy(&transfer.iso_request, iso_request.get_ptr(), sizeof(UsbDeviceIsoRequest));
	pipe.device->isochronous_transfer(&transfer);

	// returns an identifier specific to the transfer
	return not_an_error(id_transfer);
}

error_code sys_usbd_get_transfer_status(u32 handle, u32 id_transfer, u32 unk1, vm::ptr<u32> result, vm::ptr<u32> count)
{
	sys_usbd.trace("sys_usbd_get_transfer_status(handle=0x%x, id_transfer=0x%x, unk1=0x%x, result=*0x%x, count=*0x%x)", handle, id_transfer, unk1, result, count);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init)
		return CELL_EINVAL;

	auto& transfer = usbh->get_transfer(id_transfer);

	*result = transfer.result;
	*count  = transfer.count;

	return CELL_OK;
}

error_code sys_usbd_get_isochronous_transfer_status(u32 handle, u32 id_transfer, u32 unk1, vm::ptr<UsbDeviceIsoRequest> request, vm::ptr<u32> result)
{
	sys_usbd.todo("sys_usbd_get_isochronous_transfer_status(handle=0x%x, id_transfer=0x%x, unk1=0x%x, request=*0x%x, result=*0x%x)", handle, id_transfer, unk1, request, result);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init)
		return CELL_EINVAL;

	auto& transfer = usbh->get_transfer(id_transfer);

	*result = transfer.result;
	memcpy(request.get_ptr(), &transfer.iso_request, sizeof(UsbDeviceIsoRequest));

	return CELL_OK;
}

error_code sys_usbd_get_device_location()
{
	sys_usbd.todo("sys_usbd_get_device_location()");
	return CELL_OK;
}

error_code sys_usbd_send_event()
{
	sys_usbd.todo("sys_usbd_send_event()");
	return CELL_OK;
}

error_code sys_usbd_event_port_send(u32 handle, u64 arg1, u64 arg2, u64 arg3)
{
	sys_usbd.warning("sys_usbd_event_port_send(handle=0x%x, arg1=0x%x, arg2=0x%x, arg3=0x%x)", handle, arg1, arg2, arg3);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init)
		return CELL_EINVAL;

	usbh->add_event(arg1, arg2, arg3);

	return CELL_OK;
}

error_code sys_usbd_allocate_memory()
{
	sys_usbd.todo("sys_usbd_allocate_memory()");
	return CELL_OK;
}

error_code sys_usbd_free_memory()
{
	sys_usbd.todo("sys_usbd_free_memory()");
	return CELL_OK;
}

error_code sys_usbd_get_device_speed()
{
	sys_usbd.todo("sys_usbd_get_device_speed()");
	return CELL_OK;
}
