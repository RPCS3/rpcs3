#include "stdafx.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/System.h"
#include "sys_event.h"
#include "sys_fs.h"
#include "util/shared_ptr.hpp"

#include "sys_storage.h"
#include "sys_event.h"

LOG_CHANNEL(sys_storage);

namespace
{
	auto log_callback(ppu_thread& ppu)
	{	
		sys_storage.todo("Callstack:\n%s", ppu.dump_callstack());
	}

	struct storage_manager_impl
	{
		storage_manager_impl() {}
		storage_manager_impl(const storage_manager_impl&) = delete;
		int operator=(const storage_manager_impl&) = delete;

		void send_event(u64 device_id, u64 data1, u64 data2, u64 data3)
		{
			id_manager::g_process = 0;
			std::vector<shared_ptr<lv2_storage_medium_event_port>> ports;

			idm::select<lv2_storage_medium_event_port>([&](u32 id, u32 proc, lv2_storage_medium_event_port& port)
			{
				// Check port status
				if (port.savable() && (!port.device_id || port.device_id == device_id))
				{
					// Detached ports can be removed
					ports.emplace_back(ensure(idm::get_unlocked<lv2_storage_medium_event_port>(idm::id_index(id, proc))));
				}
			});

			u64 dummy_kernel_port_address = 0x800000000062d4c0;

			for (auto& port : ports)
			{
				dummy_kernel_port_address += 0x100;

				port->medium_port->send(dummy_kernel_port_address, data1, data2, data3);
			}
		}

		void operator()() noexcept
		{
			u32 events[] =
			{
				// First class
				//3,
				// 4,
				 7,
				// 8,

				// 0x101,
				// 0x102,
			};

			//u64 start_time = get_system_time();
			u64 start_count = 0;
			u64 event_index = 0;

			while (Emu.IsPausedOrReady())
			{
				thread_ctrl::wait_for(2500);
			}

			for (u32 ii = 0; ii < 10; ii++)
			{
				thread_ctrl::wait_for(1000 * 1000);
			}

			while (thread_ctrl::state() != thread_state::aborting)
			{
				thread_ctrl::wait_for(25000);

				start_count++;

				if (start_count == 1)
				{
					sys_storage.notice("storage_manager(): Sending 0x%x (Media ID = 0x%x)", events[event_index], start_count);


					if (true)
					{
						send_event(0x0101000000000006, 0x0000000000000101, 0x0000000000000000, 0x0101000000000006);
						thread_ctrl::wait_for(2500000);

						send_event(0x0101000000000006, 0x0000000000000003, 0x000000000000ff71, 0x0101000000000006);
						thread_ctrl::wait_for(2500000);

						send_event(0x0101000000000006, 0x0000000000000003, 0, 0x0101000000000004);
						thread_ctrl::wait_for(2500000);
						send_event(0x0101000000000006, 0x0000000000000003, 0, 0x0101000000000008);
						thread_ctrl::wait_for(2500000);
						send_event(0x0101000000000006, 0x0000000000000003, 0x000000000000ff71, 0x0101000000000003);
						thread_ctrl::wait_for(2500000);
					}

				}
			}
		}

		static constexpr auto thread_name = "VSH Storage Events"sv;
	};

	using storage_manager = named_thread<storage_manager_impl>;
}

lv2_storage::lv2_storage(utils::serial& ar) noexcept
	: lv2_obj{1}
	, device_id(ar)
	, mode(ar)
	, flags(ar)
{
	lv2_event_queue::load_ptr(ar, async_port, "lv2_storage");}

void lv2_storage::save(utils::serial& ar)
{
	ar(device_id, mode, flags);
	lv2_event_queue::save_ptr(ar, async_port.load().get());
}

lv2_storage_medium_event_port::lv2_storage_medium_event_port(utils::serial& ar) noexcept
	: device_id(ar)
{
	lv2_event_queue::load_ptr(ar, medium_port, "lv2_storage_medium_event_port");
}

void lv2_storage_medium_event_port::save(utils::serial& ar)
{
	ar(device_id);
	lv2_event_queue::save_ptr(ar, medium_port.get());
}

bool lv2_storage_medium_event_port::savable() const
{
	return lv2_obj::check(medium_port);
}

error_code sys_storage_open(ppu_thread& ppu, u64 device, u64 mode, vm::ptr<u32> fd, u64 flags)
{
	sys_storage.todo("sys_storage_open(device=0x%x, mode=0x%x, fd=*0x%x, flags=0x%x)", device, mode, fd, flags);
	log_callback(*cpu_thread::get_current<ppu_thread>());

	if (device == 0)
	{
		return CELL_ENOENT;
	}

	if (!fd)
	{
		return CELL_EFAULT;
	}

	[[maybe_unused]] u64 storage_id = device & 0xFFFFF00FFFFFFFF;
	fs::file file;

	thread_local u32 weird = 0;

	// if (device == 0x0101000000000006)
	// {
	// 	if (weird < 1)
	// 	{
	// 		weird++;
	// 		return CELL_ENOEXEC;
	// 	}
	// }

	if (const u32 id = idm::make<lv2_obj, lv2_storage>(device, std::move(file), mode, flags))
	{
		*fd = id;
		sys_storage.notice("sys_storage_open(): Handle=0x%x", id);
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code sys_storage_close(u32 fd)
{
	sys_storage.todo("sys_storage_close(fd=0x%x)", fd);

	ensure(idm::remove<lv2_obj, lv2_storage>(fd));

	return CELL_OK;
}

error_code sys_storage_read(u32 fd, u32 mode, u32 start_sector, u32 num_sectors, vm::ptr<void> bounce_buf, vm::ptr<u32> sectors_read, u64 flags)
{
	log_callback(*cpu_thread::get_current<ppu_thread>());
	sys_storage.todo("sys_storage_read(fd=0x%x, mode=0x%x, start_sector=0x%x, num_sectors=0x%x, bounce_buf=*0x%x, sectors_read=*0x%x, flags=0x%x)", fd, mode, start_sector, num_sectors, bounce_buf, sectors_read, flags);

	if (!bounce_buf || !sectors_read)
	{
		return CELL_EFAULT;
	}

	std::memset(bounce_buf.get_ptr(), 0, num_sectors * 0x200ull);
	const auto handle = idm::get_unlocked<lv2_obj, lv2_storage>(fd);

	if (!handle)
	{
		return CELL_ESRCH;
	}

	if (handle->file)
	{
		handle->file.seek(start_sector * 0x200ull);
		const u64 size = num_sectors * 0x200ull;
		const u64 result = lv2_file::op_read(handle->file, bounce_buf, size);
		num_sectors = ::narrow<u32>(result / 0x200ull);
	}

	*sectors_read = num_sectors;

	return CELL_OK;
}

error_code sys_storage_write(u32 fd, u32 mode, u32 start_sector, u32 num_sectors, vm::ptr<void> data, vm::ptr<u32> sectors_wrote, u64 flags)
{
	sys_storage.todo("sys_storage_write(fd=0x%x, mode=0x%x, start_sector=0x%x, num_sectors=0x%x, data=*=0x%x, sectors_wrote=*0x%x, flags=0x%llx)", fd, mode, start_sector, num_sectors, data, sectors_wrote, flags);

	if (!sectors_wrote)
	{
		return CELL_EFAULT;
	}

	const auto handle = idm::get_unlocked<lv2_obj, lv2_storage>(fd);

	if (!handle)
	{
		return CELL_ESRCH;
	}

	*sectors_wrote = num_sectors;

	return CELL_OK;
}

error_code sys_storage_send_device_command(u32 dev_handle, u64 cmd, vm::ptr<void> in, u64 inlen, vm::ptr<void> out, u64 outlen)
{
	sys_storage.todo("sys_storage_send_device_command(dev_handle=0x%x, cmd=0x%llx, in=*0x%, inlen=0x%x, out=*0x%x, outlen=0x%x)", dev_handle, cmd, in, inlen, out, outlen);
	log_callback(*cpu_thread::get_current<ppu_thread>());

	return CELL_OK;
}

error_code sys_storage_async_configure(u32 fd, u32 io_buf, u32 equeue_id, u32 unk)
{
	sys_storage.todo("sys_storage_async_configure(fd=0x%x, io_buf=0x%x, equeue_id=0x%x, unk=*0x%x)", fd, io_buf, equeue_id, unk);
	log_callback(*cpu_thread::get_current<ppu_thread>());

	const auto handle = idm::get_unlocked<lv2_obj, lv2_storage>(fd);

	if (!handle)
	{
		return CELL_ESRCH;
	}

	if (auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(equeue_id))
	{
		handle->async_port.store(queue);
	}
	else
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_storage_async_send_device_command(u32 dev_handle, u64 cmd, vm::ptr<void> in, u64 inlen, vm::ptr<void> out, u64 outlen, u64 operation_name)
{
	sys_storage.todo("sys_storage_async_send_device_command(dev_handle=0x%x, cmd=0x%llx, in=*0x%x, inlen=0x%x, out=*0x%x, outlen=0x%x, operation_name=0x%x)", dev_handle, cmd, in, inlen, out, outlen, operation_name);
	sys_storage.todo("sys_storage_async_send_device_command(): BUF: %s", std::span<u8>(vm::get_super_ptr(in.addr()), inlen));
	log_callback(*cpu_thread::get_current<ppu_thread>());

	auto& manager = g_fxo->get<storage_manager>();

	const auto handle = idm::get_unlocked<lv2_obj, lv2_storage>(dev_handle);

	if (!handle)
	{
		return CELL_ESRCH;
	}

	std::vector<u8> data_in(inlen);

	if (inlen && !vm::try_access(in.addr(), data_in.data(), inlen, false))
	{
		fmt::throw_exception("Failed to read input data!");
	}

	std::memset(out.get_ptr(), 0, outlen);

	struct input_output
	{
		u32 ID; // So we can identify the command later and fill response_base accordingly
		std::vector<u8> data_in; // Data input (masked)
		std::vector<u8> data_mask; // Input mask
		std::vector<u8> response_base; // Dat to be memcpy'ed to output, prior to possibly doing more modifications
		u64 response_event_data2; // Event data sent
		u64 response_event_data3; // Event data sent
	};

	static const std::vector<input_output> inputs_outputs
	{
		input_output
		{
			1, 
			std::vector<u8> // input
			{
				0x51, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x22, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x0C,
				0x00, 0x00, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x23,
				0x00, 0x00, 0x00, 0x03,
				0x00, 0x00, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x00
			},
			std::vector<u8> // input mask
			{
				0xFF, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0xFF, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0x00
			},
			std::vector<u8> // output base
			{
				0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00
			},
			0x8000000002050000, 0,
		},

		input_output
		{
			2, 
			std::vector<u8> // input
			{
				0xA4, 0x00, 0x00, 0x00,
		 		0x00, 0x00, 0x00, 0xE0,
		 		0x00, 0x08, 0x03, 0x00,
		 		0x00, 0x00, 0x00, 0x00,
		 		0x00, 0x00, 0x00, 0x00,
		 		0x00, 0x00, 0x00, 0x00,
		 		0x00, 0x00, 0x00, 0x00,
		 		0x00, 0x00, 0x00, 0x00,
		 		0x00, 0x00, 0x00, 0x0C,
		 		0x00, 0x00, 0x00, 0x01,
		 		0x00, 0x00, 0x00, 0x08,
		 		0x00, 0x00, 0x00, 0x03,
		 		0x00, 0x00, 0x00, 0x01,
		 		0x00, 0x00, 0x00, 0x00
			},
			std::vector<u8> // input mask
			{
				0xFF, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0xFF, 0xFF, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0x00
			},
			std::vector<u8> // output base
			{
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			},
			0, 0,
		},

		input_output
		{
			3,
			std::vector<u8> // input
			{
				0xAD, 0x01, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x73, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x0C,
				0x00, 0x00, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x73,
				0x00, 0x00, 0x00, 0x03,
				0x00, 0x00, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x00
			},
			std::vector<u8> // input mask
			{
				0xFF, 0xFF, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0xFF, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,//
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0xFF,
				0x00, 0x00, 0x00, 0x00
			},
			std::vector<u8> // outout base
			{
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x00,
			},
			0, 0
		}
	};

	std::vector<u8> data_for_comparison;

	u32 found_ID = umax;
	u64 response_event_data2 = 0;
	u64 response_event_data3 = 0;

	for (const input_output& info : inputs_outputs)
	{
		if (data_in.size() != info.data_in.size())
		{
			continue;
		}

		data_for_comparison.resize(data_in.size());
		std::memcpy(data_for_comparison.data(), data_in.data(), data_in.size());

		for (u32 index = 0; index < data_for_comparison.size(); index++)
		{
			data_for_comparison[index] &= info.data_mask[index];
		}

		if (data_for_comparison == data_in)
		{
			if (info.response_base.size() != outlen)
			{
				fmt::throw_exception("Misidentification of input data type! ID=x%d (response size: %d)", info.ID, info.response_base.size());
			}

			found_ID = info.ID;
			response_event_data2 = info.response_event_data2;
			response_event_data3 = info.response_event_data3;
			ensure(vm::try_access(out.addr(), const_cast<u8*>(info.response_base.data()), outlen, true));
		}
	}

	if (auto q = handle->async_port.load())
	{
		q->send(0, operation_name, response_event_data2, response_event_data3);
	}

	return CELL_OK;
}

error_code sys_storage_async_read()
{
	sys_storage.todo("sys_storage_async_read()");

	return CELL_OK;
}

error_code sys_storage_async_write()
{
	sys_storage.todo("sys_storage_async_write()");

	return CELL_OK;
}

error_code sys_storage_async_cancel()
{
	sys_storage.todo("sys_storage_async_cancel()");

	return CELL_OK;
}

error_code sys_storage_get_device_info(u64 device, vm::ptr<StorageDeviceInfo> buffer)
{
	sys_storage.todo("sys_storage_get_device_info(device=0x%x, buffer=*0x%x)", device, buffer);
	log_callback(*cpu_thread::get_current<ppu_thread>());

	if (!buffer)
	{
		return CELL_EFAULT;
	}

	memset(buffer.get_ptr(), 0, sizeof(StorageDeviceInfo));

	u64 storage = device & 0xFFFFF00FFFFFFFF;
	u32 dev_num = (device >> 32) & 0xFF;

	if (storage == ATA_HDD) // dev_hdd?
	{
		if (dev_num > 2)
		{
			return not_an_error(-5);
		}

		std::string u = "unnamed";
		memcpy(buffer->name, u.c_str(), u.size());
		buffer->sector_size = 0x200;
		buffer->one = 1;
		buffer->one1 = 1;
		buffer->one2 = 1;
		buffer->flag5 = 1;

		// set partition size based on dev_num
		// stole these sizes from kernel dump, unknown if they are 100% correct
		// vsh reports only 2 partitions even though there is 3 sizes
		switch (dev_num)
		{
		case 0:
			buffer->sector_count = 0x2542EAB0; // possibly total size
			break;
		case 1:
			buffer->sector_count = 0x24FAEA98; // which makes this hdd0
			break;
		case 2:
			buffer->sector_count = 0x3FFFF8; // and this one hdd1
			break;
		}
	}
	else if (storage == BDVD_DRIVE) //	dev_bdvd?
	{
		if (dev_num > 0)
		{
			return not_an_error(-5);
		}

		std::string u = "unnamed";
		memcpy(buffer->name, u.c_str(), u.size());

		const bool connected = true;
		if (!connected)
		{
			buffer->sector_count = 0;
			buffer->sector_size = 0x7FFFFFFF;
		}
		else
		{
			buffer->sector_count = 0x1EC4B00;
			buffer->sector_size = 0x800;
		}
// [000] | 75 6E 6E 61 | 6D 65 64 00 |
// [008] | 00 00 00 00 | 00 00 00 00 |
// [010] | 00 00 00 00 | 00 00 00 00 |
// [018] | 00 00 00 00 | 00 00 00 00 |
// [020] | 00 00 00 00 | 00 00 00 00 |
// [028] | 00 00 00 00 | 01 EC 4B 00 |
// [030] | 00 00 02 00 | 00 00 00 01 |
// [038] | 01 01 01 00 | 01 01 00 01 |



// [000] | 75 6E 6E 61 | 6D 65 64 00 |
// [008] | 00 00 00 00 | 00 00 00 00 |
// [010] | 00 00 00 00 | 00 00 00 00 |
// [018] | 00 00 00 00 | 00 00 00 00 |
// [020] | 00 00 00 00 | 00 00 00 00 |
// [028] | 00 00 00 00 | 00 62 83 E0 |
// [030] | 00 00 08 00 | 00 00 00 01 |
// [038] | 00 01 01 00 | 00 00 00 01 |

		buffer->one = 1;
		buffer->connected = connected;
		buffer->one1 = 1;
		buffer->one2 = 1;
		buffer->flag3 = 0;
		//buffer->flags4 = 0;
		buffer->flag5 = 1;

		static const unsigned char dump2[0x40] =
		{
			0x75, 0x6E, 0x6E, 0x61, 0x6D, 0x65, 0x64, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x83, 0xE0,
			0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01,
			0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01
		};

		static const unsigned char data_mode_8[0x40] =
		{
		    0x75, 0x6E, 0x6E, 0x61, 0x6D, 0x65, 0x64, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x83, 0xE0,
		    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01,
		    0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01
		};


		std::memcpy(&*buffer, data_mode_8, sizeof(data_mode_8));
	//	buffer->sector_size = 0x200;
	}
	else if (storage == USB_MASS_STORAGE_1(0))
	{
		if (dev_num > 0)
		{
			return not_an_error(-5);
		}

		std::string u = "unnamed";
		memcpy(buffer->name, u.c_str(), u.size());
		/*buffer->sector_count = 0x4D955;*/
		buffer->sector_size = 0x200;
		buffer->one = 1;
		buffer->one1 = 1;
		buffer->one2 = 1;
		buffer->flag5 = 1;
	}
	else if (storage == NAND_FLASH)
	{
		if (dev_num > 6)
		{
			return not_an_error(-5);
		}

		std::string u = "unnamed";
		memcpy(buffer->name, u.c_str(), u.size());
		buffer->sector_size = 0x200;
		buffer->one = 1;
		buffer->one1 = 1;
		buffer->one2 = 1;
		buffer->flag5 = 1;

		// see ata_hdd for explanation
		switch (dev_num)
		{
		case 0: buffer->sector_count = 0x80000;
			break;
		case 1: buffer->sector_count = 0x75F8;
			break;
		case 2: buffer->sector_count = 0x63E00;
			break;
		case 3: buffer->sector_count = 0x8000;
			break;
		case 4: buffer->sector_count = 0x400;
			break;
		case 5: buffer->sector_count = 0x2000;
			break;
		case 6: buffer->sector_count = 0x200;
			break;
		}
	}
	else if (storage == NOR_FLASH)
	{
		if (dev_num > 3)
		{
			return not_an_error(-5);
		}

		std::string u = "unnamed";
		memcpy(buffer->name, u.c_str(), u.size());
		buffer->sector_size = 0x200;
		buffer->one = 1;
		buffer->one1 = 0;
		buffer->one2 = 1;
		buffer->flag5 = 1;

		// see ata_hdd for explanation
		switch (dev_num)
		{
		case 0: buffer->sector_count = 0x8000;
			break;
		case 1: buffer->sector_count = 0x77F8;
			break;
		case 2: buffer->sector_count = 0x100; // offset, 0x20000
			break;
		case 3: buffer->sector_count = 0x400;
			break;
		}
	}
	else if (storage == NAND_UNK)
	{
		if (dev_num > 1)
		{
			return not_an_error(-5);
		}

		std::string u = "unnamed";
		memcpy(buffer->name, u.c_str(), u.size());
		buffer->sector_size = 0x800;
		buffer->one = 1;
		buffer->one1 = 0;
		buffer->one2 = 1;
		buffer->flag5 = 1;

		// see ata_hdd for explanation
		switch (dev_num)
		{
		case 0: buffer->sector_count = 0x7FFFFFFF;
			break;
		}
	}
	else
	{
		sys_storage.error("sys_storage_get_device_info(device=0x%x, buffer=*0x%x)", device, buffer);
	}

	return CELL_OK;
}

error_code sys_storage_get_device_config(vm::ptr<u32> storages, vm::ptr<u32> devices)
{
	sys_storage.todo("sys_storage_get_device_config(storages=*0x%x, devices=*0x%x)", storages, devices);
	log_callback(*cpu_thread::get_current<ppu_thread>());

	if (storages) *storages = 6; else return CELL_EFAULT;
	if (devices)  *devices = 17; else return CELL_EFAULT;

	return CELL_OK;
}

error_code sys_storage_report_devices(u32 storages, u32 start, u32 devices, vm::ptr<u64> device_ids)
{
	sys_storage.todo("sys_storage_report_devices(storages=0x%x, start=0x%x, devices=0x%x, device_ids=0x%x)", storages, start, devices, device_ids);
	log_callback(*cpu_thread::get_current<ppu_thread>());

	if (!device_ids)
	{
		return CELL_EFAULT;
	}

	if (storages != 6)
	{
		return -5;
	}

	static constexpr std::array<u64, 0x11> all_devs = []
	{
		std::array<u64, 0x11> all_devs{};
		all_devs[0] = 0x10300000000000A;

		for (int i = 0; i < 7; ++i)
		{
			all_devs[i + 1] = 0x100000000000001 | (static_cast<u64>(i) << 32);
		}

		for (int i = 0; i < 3; ++i)
		{
			all_devs[i + 8] = 0x101000000000007 | (static_cast<u64>(i) << 32);
		}

		all_devs[11] = 0x101000000000006;

		for (int i = 0; i < 4; ++i)
		{
			all_devs[i + 12] = 0x100000000000004 | (static_cast<u64>(i) << 32);
		}

		all_devs[16] = 0x100000000000003;
		return all_devs;
	}();

	if (!devices || start >= all_devs.size() || devices > all_devs.size() - start)
	{
		return CELL_EINVAL;
	}

	std::copy_n(all_devs.begin() + start, devices, device_ids.get_ptr());

	return CELL_OK;
}

error_code sys_storage_configure_medium_event(ppu_thread& ppu, u32 fd, u32 equeue_id, vm::ptr<u32> handle)
{
	sys_storage.todo("sys_storage_configure_medium_event(fd=0x%x, equeue_id=0x%x, c=0x%x)", fd, equeue_id, handle);
	log_callback(*cpu_thread::get_current<ppu_thread>());

	if (!ppu.has_root_perm)
	{
		return CELL_EPERM;
	}

	u64 device_id = 0; // 0 means global

	if (fd)
	{
		const auto storage = idm::get_unlocked<lv2_obj, lv2_storage>(fd);

		if (!storage)
		{
			return {CELL_ESRCH, "storage"};
		}

		// Oddly that is all it needs from the storage descriptor
		// It closes the handle right after device ID extraction
		// Perhaps because not calling sys_storage_open's routines saves on expensive "error checking" the device ID?
		device_id = storage->device_id;
	}

	auto& manager = *ensure(g_fxo->try_get<storage_manager>());

	if (auto queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(equeue_id))
	{
		while (!idm::make_ptr<lv2_storage_medium_event_port>(device_id, queue))
		{
			std::vector<std::pair<u32, u32>> cleanup_list;

			// Try cleanup
			id_manager::g_process = 0;
			idm::select<lv2_storage_medium_event_port>([&](u32 id, u32 proc, lv2_storage_medium_event_port& port)
			{
				// Check port status
				if (!port.savable())
				{
					// Detached ports can be removed
					cleanup_list.emplace_back(id, proc);
				}
			});

			bool success = false;

			for (auto [id, proc] : cleanup_list)
			{
				success = idm::remove<lv2_storage_medium_event_port>(idm::id_index(id, proc));
			}

			id_manager::g_process = ppu.proc_id;

			if (!success)
			{
				fmt::throw_exception("lv2_storage_medium_event_port() entries depletion, consider increases lv2_storage_medium_event_port::id_count!");
			}
		}
	}
	else
	{
		return CELL_ESRCH;
	}

	// No idea what this means, seems like it returns uninitialized memory
	*handle = 0x5D7280;
	return CELL_OK;
}

error_code sys_storage_set_medium_polling_interval(ppu_thread& ppu, u32 fd, u64 interval)
{
	sys_storage.todo("sys_storage_set_medium_polling_interval()");

	return CELL_OK;
}

error_code sys_storage_create_region()
{
	sys_storage.todo("sys_storage_create_region()");

	return CELL_OK;
}

error_code sys_storage_delete_region()
{
	sys_storage.todo("sys_storage_delete_region()");

	return CELL_OK;
}

error_code sys_storage_execute_device_command(u32 fd, u64 cmd, vm::ptr<char> cmdbuf, u64 cmdbuf_size, vm::ptr<char> databuf, u64 databuf_size, vm::ptr<u32> driver_status)
{
	sys_storage.todo("sys_storage_execute_device_command(fd=0x%x, cmd=0x%llx, cmdbuf=*0x%x, cmdbuf_size=0x%llx, databuf=*0x%x, databuf_size=0x%llx, driver_status=*0x%x)", fd, cmd, cmdbuf, cmdbuf_size, databuf, databuf_size, driver_status);

	// cmd == 2 is get device info,
	// databuf, first byte 0 == status ok?
	// byte 1, if < 0 , not ata device
	return CELL_OK;
}

error_code sys_storage_check_region_acl()
{
	sys_storage.todo("sys_storage_check_region_acl()");

	return CELL_OK;
}

error_code sys_storage_set_region_acl()
{
	sys_storage.todo("sys_storage_set_region_acl()");

	return CELL_OK;
}

error_code sys_storage_get_region_offset()
{
	sys_storage.todo("sys_storage_get_region_offset()");

	return CELL_OK;
}

error_code sys_storage_set_emulated_speed()
{
	sys_storage.todo("sys_storage_set_emulated_speed()");

	// todo: only debug kernel has this
	return CELL_ENOSYS;
}
