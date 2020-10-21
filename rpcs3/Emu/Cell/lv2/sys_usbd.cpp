#include "stdafx.h"
#include "sys_usbd.h"

#include <algorithm>
#include <queue>
#include <thread>
#include <unordered_set>
#include "Emu/Memory/vm.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "sys_ppu_thread.h"

#include "Emu/Io/usb_device.h"
#include "Emu/Io/Skylander.h"
#include "Emu/Io/GHLtar.h"

#include <libusb.h>
#include <xxhash.h>

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

enum : u16
{
	USBD_VID_ANY = 0x0000
};
enum : u16
{
	USBD_PID_ANY = 0x0000
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

	u8 endpoint = 0;
};

// template specializations to allow us to use libusb_device_descriptors in unordered_map/unordered_set containers.
namespace std {
	template<> struct hash<libusb_device_descriptor>
	{
		std::size_t operator()(const libusb_device_descriptor& f) const
		{
			return XXH3_64bits(&f, sizeof(libusb_device_descriptor));
		}
	};
	template<> struct equal_to<libusb_device_descriptor>
	{
		bool operator() (libusb_device_descriptor const& data1, libusb_device_descriptor const& data2) const
		{
			return memcmp(&data1, &data2, sizeof(libusb_device_descriptor))==0;
		}
	};
}

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
	u32 remove_ldd(vm::ptr<char> s_product, u16 slen_product);
	void advise_ldd_of_devices(UsbLdd& ldd);

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
	void perform_device_scan(bool force_scan);
	void advise_ldds_of_attach(std::shared_ptr<usb_device> dev);
	void advise_ldds_of_detach(std::shared_ptr<usb_device> dev);
	void close_pipes_for_device(std::shared_ptr<usb_device> dev);

private:
	// Counters for device IDs, transfer IDs and pipe IDs
	atomic_t<u8> dev_counter = 1;
	u32 transfer_counter     = 0;
	u32 pipe_counter         = 0x10; // Start at 0x10 only for tracing purposes

	// List of device drivers
	std::vector<UsbLdd> ldds;

	// list of libusb device paths the last time we scanned
	std::unordered_set<libusb_device_descriptor> device_scan_list;

	// USB device allow_list, only items on this list will be configured for rpcs3 libusb control
	// tuple elements are: VID, PID_min, PID_max, name, emulation instantiator
	const std::set<std::tuple<u16, u16, u16, std::string_view, u16(*)(void), std::shared_ptr<usb_device>(*)(void)>> device_allow_list
	{
		{0x1430, 0x0150, 0x0150, "Skylanders Portal", &usb_device_skylander_emu::get_num_emu_devices, &usb_device_skylander_emu::make_instance},

		{0x0E6F, 0x0241, 0x0241, "Lego Dimensions Portal", nullptr, nullptr},
		{0x0E6F, 0x0129, 0x0129, "Disney Infinity Portal", nullptr, nullptr},

		{0x1415, 0x0000, 0x0000, "Singstar Microphone", nullptr, nullptr},
		{0x12BA, 0x00FF, 0x00FF, "Rocksmith Guitar Adapter", nullptr, nullptr},
		{0x12BA, 0x0100, 0x0100, "Guitar Hero Guitar", nullptr, nullptr},
		{0x12BA, 0x0120, 0x0120, "Guitar Hero Drums", nullptr, nullptr},
		{0x12BA, 0x074B, 0x074B, "Guitar Hero Live Guitar", &usb_device_ghltar_emu::get_num_emu_devices, &usb_device_ghltar_emu::make_instance},

		{0x12BA, 0x0140, 0x0140, "DJ Hero Turntable", nullptr, nullptr},
		{0x12BA, 0x0200, 0x020F, "Harmonix Guitar", nullptr, nullptr},
		{0x12BA, 0x0210, 0x021F, "Harmonix Drums", nullptr, nullptr},
		{0x12BA, 0x2330, 0x233F, "Harmonix Keyboard", nullptr, nullptr},
		{0x12BA, 0x2430, 0x243F, "Harmonix Button Guitar", nullptr, nullptr},
		{0x12BA, 0x2530, 0x253F, "Harmonix Real Guitar", nullptr, nullptr},

		// GT5 Wheels&co
		{0x046D, 0xC283, 0xC29B, "lgFF_c283_c29b", nullptr, nullptr},
		{0x044F, 0xB653, 0xB653, "Thrustmaster RGT FFB Pro", nullptr, nullptr},
		{0x044F, 0xB65A, 0xB65A, "Thrustmaster F430", nullptr, nullptr},
		{0x044F, 0xB65D, 0xB65D, "Thrustmaster FFB", nullptr, nullptr},
		{0x044F, 0xB65E, 0xB65E, "Thrustmaster TRS", nullptr, nullptr},
		{0x044F, 0xB660, 0xB660, "Thrustmaster T500 RS Gear Shift", nullptr, nullptr},

		// Buzz controllers
		{0x054C, 0x1000, 0x1040, "buzzer0", nullptr, nullptr},
		{0x054C, 0x0001, 0x0041, "buzzer1", nullptr, nullptr},
		{0x054C, 0x0042, 0x0042, "buzzer2", nullptr, nullptr},
		{0x046D, 0xC220, 0xC220, "buzzer9", nullptr, nullptr},

		// GCon3 Gun
		{0x0B9A, 0x0800, 0x0800, "guncon3", nullptr, nullptr},

		// PSP Devices
		{0x054C, 0x01C8, 0x01C8, "PSP Type A", nullptr, nullptr},
		{0x054C, 0x01C9, 0x01C9, "PSP Type B", nullptr, nullptr},
		{0x054C, 0x01CA, 0x01CA, "PSP Type C", nullptr, nullptr},
		{0x054C, 0x01CB, 0x01CB, "PSP Type D", nullptr, nullptr},
		{0x054C, 0x02D2, 0x02D2, "PSP Slim", nullptr, nullptr},

		// Sony Stereo Headsets
		{0x12BA, 0x0032, 0x0032, "Wireless Stereo Headset", nullptr, nullptr},
		{0x12BA, 0x0042, 0x0042, "Wireless Stereo Headset", nullptr, nullptr},
	};

	// List of pipes
	std::map<u32, UsbPipe> open_pipes;
	// Transfers infos
	std::array<UsbTransfer, 0x44> transfers;

	// Queue of pending usbd events
	std::queue<std::tuple<u64, u64, u64>> usbd_events;

	// List of devices "connected" to the ps3
	std::vector<std::shared_ptr<usb_device>> usb_devices;

	libusb_context* ctx = nullptr;
	libusb_hotplug_callback_handle hotplug_callback_handle = 0;
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

	for (u32 index = 0; index < MAX_SYS_USBD_TRANSFERS; index++)
	{
		transfers[index].transfer    = libusb_alloc_transfer(8);
		transfers[index].transfer_id = index;
	}

}

usb_handler_thread::~usb_handler_thread()
{
	timeval lusb_tv{ 0, 200 };
	// first we cancel any transfers that might be in progress
	for (u32 index = 0; index < MAX_SYS_USBD_TRANSFERS; index++)
	{
		libusb_cancel_transfer(transfers[index].transfer);

		// and transfer cancellation is async, so we need to process
		// any events that may have triggered from cancelling them
		libusb_handle_events_timeout_completed(ctx, &lusb_tv, nullptr);

		libusb_free_transfer(transfers[index].transfer);
	}

	// Ensures shared_ptr<usb_device> are all cleared before terminating libusb
	handled_devices.clear();
	open_pipes.clear();

	usb_devices.clear();

	libusb_exit(ctx);
}

void usb_handler_thread::operator()()
{
	timeval lusb_tv{0, 200};
	u64 usb_hotplug_timeout = 0;

	while (thread_ctrl::state() != thread_state::aborting)
	{
		// Process asynchronous requests that are pending
		libusb_handle_events_timeout_completed(ctx, &lusb_tv, nullptr);

		// 'Hotplug' scan here
		if (get_system_time() > usb_hotplug_timeout)
		{
			// If we did the hotplug scan each cycle the game performance was significantly degraded, so we only perform this scan
			// every 4 seconds.  True hotplug events would be better than this scanning, but libusb hotplug isn't supported under
			// Windows or FreeBSD
			perform_device_scan(false);
			usb_hotplug_timeout = get_system_time() + 4'000'000ull;
		}

		// Process fake transfers
		if (!fake_transfers.empty())
		{
			std::lock_guard lock(this->mutex);

			const u64 timestamp = get_system_time() - Emu.GetPauseTime();

			for (auto it = fake_transfers.begin(); it != fake_transfers.end();)
			{
				auto transfer = *it;

				ASSERT(transfer->busy && transfer->fake);

				if (transfer->expected_time > timestamp)
				{
					++it;
					continue;
				}


				transfer->result = transfer->expected_result;
				transfer->count  = transfer->expected_count;
				transfer->fake   = false;
				transfer->busy   = false;

				send_message(SYS_USBD_TRANSFER_COMPLETE, transfer->transfer_id);
				it = fake_transfers.erase(it); // if we've processed this, then we erase this entry (replacing the iterator with the new reference)
			}
		}

		// If there are no handled devices the usb thread is not actively needed, so we can sleep here for a while
		if (handled_devices.empty())
			std::this_thread::sleep_for(500ms);
		else
			std::this_thread::sleep_for(200us);
	}
}

void usb_handler_thread::perform_device_scan(bool force_scan)
{
	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();
	std::lock_guard lock(usbh->mutex);

	std::unordered_set<libusb_device_descriptor> detach_scan{ device_scan_list };

	// look if any device which we could be interested in is actually connected
	libusb_device** list = nullptr;
	ssize_t ndev = libusb_get_device_list(ctx, &list);
	if (ndev != static_cast<ssize_t>(device_scan_list.size()) || force_scan) // we will skip the more expensive scan if we have the same number of devices as last time
	{
		for (ssize_t index = 0; index < ndev; index++)
		{
			libusb_device_descriptor desc;
			int rc = libusb_get_device_descriptor(list[index], &desc);
			if (rc != LIBUSB_SUCCESS)
			{
				sys_usbd.error("Error obtaining device descriptor: %s", libusb_strerror(rc));
				continue;
			}
			if (!device_scan_list.contains(desc))
			{
				for (const auto& [id_vendor, id_product_min, id_product_max, s_name, emulate, fn_make_instance] : device_allow_list)
				{
					// attach
					if (desc.idVendor == id_vendor
						&& desc.idProduct >= id_product_min
						&& desc.idProduct <= id_product_max)
					{
						sys_usbd.success("Found device: %s", std::basic_string(s_name));
						auto usb_dev = std::make_shared<usb_device_passthrough>(std::string(s_name), list[index], desc);
						usb_devices.push_back(usb_dev);
						advise_ldds_of_attach(usb_dev);
					}
				}
				device_scan_list.emplace(desc);
			}

			if (detach_scan.contains(desc))
				detach_scan.erase(desc);
		}

		for (auto& item : detach_scan)
		{
			// detach
			for (auto dev = usb_devices.begin(); dev != usb_devices.end(); )
			{
				auto usb_dev = dynamic_cast<usb_device_passthrough*>(dev->get());
				if (!usb_dev)
					continue;

				libusb_device_descriptor desc;
				usb_dev->get_descriptor(&desc);

				if (desc.idVendor == item.idVendor
					&& desc.idProduct == item.idProduct
					&& desc.iSerialNumber == item.iSerialNumber)
				{
					sys_usbd.success("Removing device: %s", usb_dev->get_name());
					//cancel_outstanding_transfers(*dev);
					advise_ldds_of_detach(*dev);
					close_pipes_for_device(*dev);
					dev = usb_devices.erase(dev);
				}
				else
					++dev;
			}

			device_scan_list.erase(item);
		}


		// we'll get a copy of the allow_list for devices we might emulate, and will then remove any that we already have in our usb_devices.
		auto emulate_device_check_list{ device_allow_list };
		for (const auto& dev : usb_devices)
		{
			for (auto itr = emulate_device_check_list.begin(); itr != emulate_device_check_list.end(); ) {
				u16 idVendor = dev->device._device.idVendor;
				u16 idProduct = dev->device._device.idProduct;
				if (std::get<4>(*itr) == nullptr  /* number of emulated devices function */
					|| (idVendor == std::get<0>(*itr)
						&& idProduct >= std::get<1>(*itr)
						&& idProduct <= std::get<2>(*itr))
					)
				{
					itr = emulate_device_check_list.erase(itr);
				}
				else
				{
					++itr;
				}
			}
		}
		for (const auto& [id_vendor, id_product_min, id_product_max, s_name, emulate, fn_make_instance] : emulate_device_check_list)
		{
			// TODO: the call to emulate() here should really be replaced by a counting of the number of emulated devices above
			// and then a creation of the number of devices required below...
			if (emulate && (emulate() > 0) && fn_make_instance)
			{
				sys_usbd.success("Emulating device: %s", std::basic_string(s_name));
				auto usb_dev = fn_make_instance();
				usb_devices.emplace_back(usb_dev);
				advise_ldds_of_attach(usb_dev);
			}
		}
	}

	libusb_free_device_list(list, 1);
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
	advise_ldd_of_devices(new_ldd);
	return ::size32(ldds); // TODO: to check
}

u32 usb_handler_thread::remove_ldd(vm::ptr<char> s_product, u16 slen_product)
{
	const auto itr = std::find_if(ldds.begin(), ldds.end(), [&](const auto& val) { return (strcmp(val.name.data(), s_product.get_ptr()) == 0); });
	if (itr != ldds.end())
	{
		ldds.erase(itr);
		return CELL_OK;
	}
	return CELL_ESRCH; // TODO: to check
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

void usb_handler_thread::advise_ldd_of_devices(UsbLdd& ldd)
{
	for (const auto& dev : usb_devices)
	{
		if (dev->assigned_number)
			continue;

		if ((dev->device._device.idVendor == ldd.id_vendor || ldd.id_vendor == USBD_VID_ANY) &&
			((dev->device._device.idProduct >= ldd.id_product_min && dev->device._device.idProduct <= ldd.id_product_max) || (ldd.id_product_min == USBD_PID_ANY && ldd.id_product_max == USBD_PID_ANY)))
		{
			if (!dev->open_device())
				continue;

			dev->read_descriptors();
			dev->assigned_number = dev_counter++; // assign current dev_counter, and atomically increment

			sys_usbd.success("Ldd device matchup for <%s>, VID:0x%04X PID:0x%04X SN:0x%04X assigned as handled_device=%i, ATTACHED", ldd.name,
				dev->device._device.idVendor, dev->device._device.idProduct, dev->device._device.iSerialNumber, dev->assigned_number);

			handled_devices.emplace(dev->assigned_number, std::pair(UsbInternalDevice{ 0x00, narrow<u8>(dev->assigned_number), 0x02, 0x40 }, dev));
			send_message(SYS_USBD_ATTACH, dev->assigned_number);
		}
	}
}

void usb_handler_thread::advise_ldds_of_attach(std::shared_ptr<usb_device> dev)
{
	if (dev->assigned_number)
		return;

	for (const auto& ldd : ldds)
	{
		if ((dev->device._device.idVendor == ldd.id_vendor || ldd.id_vendor == USBD_VID_ANY) &&
			((dev->device._device.idProduct >= ldd.id_product_min && dev->device._device.idProduct <= ldd.id_product_max) || (ldd.id_product_min == USBD_PID_ANY && ldd.id_product_max == USBD_PID_ANY)))
		{
			if (!dev->open_device())
				continue;

			dev->read_descriptors();
			dev->assigned_number = dev_counter++; // assign current dev_counter, and atomically increment

			sys_usbd.success("Ldd device matchup for <%s>, VID:0x%04X PID:0x%04X SN:0x%04X assigned as handled_device=%i, ATTACHED", ldd.name,
				dev->device._device.idVendor, dev->device._device.idProduct, dev->device._device.iSerialNumber, dev->assigned_number);

			handled_devices.emplace(dev->assigned_number, std::pair(UsbInternalDevice{ 0x00, narrow<u8>(dev->assigned_number), 0x02, 0x40 }, dev));
			send_message(SYS_USBD_ATTACH, dev->assigned_number);
		}
	}
}

void usb_handler_thread::advise_ldds_of_detach(std::shared_ptr<usb_device> dev)
{
	if (!dev->assigned_number)
		return;

	for (const auto& ldd : ldds)
	{
		if ((dev->device._device.idVendor == ldd.id_vendor || ldd.id_vendor == USBD_VID_ANY) &&
			((dev->device._device.idProduct >= ldd.id_product_min && dev->device._device.idProduct <= ldd.id_product_max) || (ldd.id_product_min == USBD_PID_ANY && ldd.id_product_max == USBD_PID_ANY)))
		{
			send_message(SYS_USBD_DETACH, dev->assigned_number);
			sys_usbd.success("Ldd device matchup for <%s>, VID:0x%04X PID:0x%04X SN:0x%04X formerly assigned as handled_device=%i, DETACHED", ldd.name,
				dev->device._device.idVendor, dev->device._device.idProduct, dev->device._device.iSerialNumber, dev->assigned_number);

			handled_devices.erase(dev->assigned_number);
		}
	}
}

void usb_handler_thread::close_pipes_for_device(std::shared_ptr<usb_device> dev)
{
	std::erase_if(open_pipes, [&](const auto& val)
		{
			return (val.second.device.get() == dev.get());
		});
}

bool usb_handler_thread::get_event(vm::ptr<u64>& arg1, vm::ptr<u64>& arg2, vm::ptr<u64>& arg3)
{
	if (!usbd_events.empty())
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

	*handle = 0x115B;

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
	for (auto& cpu : ::as_rvalue(std::move(usbh->sq)))
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
	sys_usbd.warning("sys_usbd_get_device_list(handle=0x%x, device_list=*0x%x, max_devices=%i)", handle, device_list, max_devices);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);
	if (!usbh->is_init)
		return CELL_EINVAL;

	u32 i_tocopy = std::min<u32>(max_devices, ::size32(usbh->handled_devices));

	for (u32 index = 0; index < i_tocopy; index++)
	{
		device_list[index] = usbh->handled_devices[index].first;
	}

	return not_an_error(i_tocopy);
}

error_code sys_usbd_register_extra_ldd(u32 handle, vm::ptr<char> s_product, u16 slen_product, u16 id_vendor, u16 id_product_min, u16 id_product_max)
{
	sys_usbd.warning("sys_usbd_register_extra_ldd(handle=0x%x, s_product=%s, slen_product=0x%04X, id_vendor=0x%04X, id_product_min=0x%04X, id_product_max=0x%04X)", handle, s_product, slen_product, id_vendor,
	    id_product_min, id_product_max);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);
	if (!usbh->is_init)
		return CELL_EINVAL;

	s32 res = usbh->add_ldd(s_product, slen_product, id_vendor, id_product_min, id_product_max);

	return not_an_error(res); // To check
}

error_code sys_usbd_get_descriptor_size(u32 handle, u32 device_handle)
{
	sys_usbd.trace("sys_usbd_get_descriptor_size(handle=0x%x, deviceNumber=%i)", handle, device_handle);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.contains(device_handle))
	{
		return CELL_EINVAL;
	}

	return not_an_error(usbh->handled_devices[device_handle].second->device.get_size());
}

error_code sys_usbd_get_descriptor(u32 handle, u32 device_handle, vm::ptr<void> descriptor, u32 desc_size)
{
	sys_usbd.trace("sys_usbd_get_descriptor(handle=0x%x, deviceNumber=%i, descriptor=0x%x, desc_size=%i)", handle, device_handle, descriptor, desc_size);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.contains(device_handle))
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
	sys_usbd.warning("sys_usbd_register_ldd(handle=0x%x, s_product=%s, slen_product=%i)", handle, s_product, slen_product);
	return sys_usbd_register_extra_ldd(handle, s_product, slen_product, USBD_VID_ANY, USBD_PID_ANY, USBD_PID_ANY);
}

error_code sys_usbd_unregister_ldd(u32 handle, vm::ptr<char> s_product, u16 slen_product)
{
	sys_usbd.warning("sys_usbd_unregister_ldd(handle=0x%x, s_product=%s, slen_product=%i)", handle, s_product, slen_product);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);
	if (!usbh->is_init)
		return CELL_EINVAL;

	const s32 res = usbh->remove_ldd(s_product, slen_product);

	return not_an_error(res); // To check
}

error_code sys_usbd_unregister_extra_ldd(u32 handle, vm::ptr<char> s_product, u16 slen_product)
{
	// it seems that this should do something different than sys_usbd_unregister_ldd... but not sure what, so for now we'll just do
	// the same
	sys_usbd.todo("sys_usbd_unregister_extra_ldd(handle=0x%x, s_product=%s, slen_product=%i)", handle, s_product, slen_product);
	return sys_usbd_unregister_ldd(handle, s_product, slen_product);
}

// TODO: determine what the unknown params are
error_code sys_usbd_open_pipe(u32 handle, u32 device_handle, u32 unk1, u64 unk2, u64 unk3, u32 endpoint, u64 unk4)
{
	sys_usbd.warning("sys_usbd_open_pipe(handle=0x%x, device_handle=0x%x, unk1=0x%x, unk2=0x%x, unk3=0x%x, endpoint=0x%x, unk4=0x%x)", handle, device_handle, unk1, unk2, unk3, endpoint, unk4);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.contains(device_handle))
	{
		return CELL_EINVAL;
	}

	return not_an_error(usbh->open_pipe(device_handle, static_cast<u8>(endpoint)));
}

error_code sys_usbd_open_default_pipe(u32 handle, u32 device_handle)
{
	sys_usbd.trace("sys_usbd_open_default_pipe(handle=0x%x, device_handle=%i)", handle, device_handle);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.contains(device_handle))
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
				lv2_obj::sleep(ppu), lv2_obj::wait_timeout(5000);

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

	if (*arg1 == SYS_USBD_ATTACH)
		lv2_obj::sleep(ppu), lv2_obj::wait_timeout(5000);

	return CELL_OK;
}

error_code sys_usbd_detect_event(u32 handle)
{
	sys_usbd.todo("sys_usbd_detect_event(handle=0x%x)", handle);
	return CELL_OK;
}

error_code sys_usbd_attach(u32 handle)
{
	sys_usbd.todo("sys_usbd_attach(handle=0x%x)", handle);
	return CELL_OK;
}

error_code sys_usbd_transfer_data(u32 handle, u32 id_pipe, vm::ptr<u8> buf, u32 buf_size, vm::ptr<UsbDeviceRequest> request, u32 type_transfer)
{
	sys_usbd.trace("sys_usbd_transfer_data(handle=0x%x, id_pipe=0x%x, buf=*0x%x, buf_length=%i, request=*0x%x, type=0x%x)", handle, id_pipe, buf, buf_size, request, type_transfer);
	if (sys_usbd.enabled == logs::level::trace && request)
	{
		sys_usbd.trace("RequestType:0x%x, Request:0x%x, wValue:0x%x, wIndex:0x%x, wLength:%i", request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength);
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

error_code sys_usbd_get_device_location(u32 handle, u32 device_handle, vm::ptr<u8> location)
{
	sys_usbd.warning("sys_usbd_get_device_location(handle=0x%x, deviceNumber=%i, location=0x%x)", handle, device_handle, location);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.contains(device_handle) || !location)
	{
		return CELL_EINVAL;
	}

	usbh->handled_devices[device_handle].second->get_device_location(location.get_ptr());

	return CELL_OK;
}

error_code sys_usbd_send_event(u32 handle)
{
	sys_usbd.todo("sys_usbd_send_event(handle=0x%x)", handle);
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

error_code sys_usbd_allocate_memory(u32 handle, vm::pptr<void> ptr, size_t size)
{
	sys_usbd.fatal("sys_usbd_allocate_memory(handle=0x%x, ptr=0x%x, size=%ul)", handle, ptr, size);
	return CELL_OK;
}

error_code sys_usbd_free_memory(u32 handle, vm::ptr<void> ptr)
{
	sys_usbd.fatal("sys_usbd_free_memory((handle=0x%x, ptr=0x%x)", handle, ptr);
	return CELL_OK;
}

error_code sys_usbd_get_device_speed(u32 handle, u32 device_handle, vm::ptr<u8> speed)
{
	sys_usbd.warning("sys_usbd_get_device_speed(handle=0x%x, deviceNumber=%i, speed=0x%x)", handle, device_handle, speed);

	const auto usbh = g_fxo->get<named_thread<usb_handler_thread>>();

	std::lock_guard lock(usbh->mutex);

	if (!usbh->is_init || !usbh->handled_devices.contains(device_handle) || !speed)
	{
		return CELL_EINVAL;
	}

	usbh->handled_devices[device_handle].second->get_device_speed(speed.get_ptr());
	return CELL_OK;
}
