#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

enum Devices : u64
{
	ATA_HDD = 0x101000000000007,
	BDVD_DRIVE = 0x101000000000006,
	PATA0_HDD_DRIVE = 0x101000000000008,
	PATA0_BDVD_DRIVE = BDVD_DRIVE,
	PATA1_HDD_DRIVE = ATA_HDD,
	BUILTIN_FLASH = 0x100000000000001,
	NAND_FLASH = BUILTIN_FLASH,
	NAND_UNK = 0x100000000000003,
	NOR_FLASH = 0x100000000000004,
	MEMORY_STICK = 0x103000000000010,
	SD_CARD = 0x103000100000010,
	COMPACT_FLASH = 0x103000200000010,
	USB_MASS_STORAGE_1_BASE = 0x10300000000000A,
	USB_MASS_STORAGE_2_BASE = 0x10300000000001F,
};

struct lv2_storage
{
	static const u32 id_base = 0x45000000;
	static const u32 id_step = 1;
	static const u32 id_count = 2048;
	SAVESTATE_INIT_POS(45);

	const u64 device_id;
	const fs::file file;
	const u64 mode;
	const u64 flags;

	lv2_storage(u64 device_id, fs::file&& file, u64 mode, u64 flags)
		: device_id(device_id)
		, file(std::move(file))
		, mode(mode)
		, flags(flags)
	{
	}
};

struct StorageDeviceInfo
{
	u8 name[0x20];          // 0x0
	be_t<u32> zero;         // 0x20
	be_t<u32> zero2;        // 0x24
	be_t<u64> sector_count; // 0x28
	be_t<u32> sector_size;  // 0x30
	be_t<u32> one;          // 0x34
	u8 flags[8];            // 0x38
};

#define USB_MASS_STORAGE_1(n) (USB_MASS_STORAGE_1_BASE + n)       /* For 0-5 */
#define USB_MASS_STORAGE_2(n) (USB_MASS_STORAGE_2_BASE + (n - 6)) /* For 6-127 */

// SysCalls

error_code sys_storage_open(u64 device, u64 mode, vm::ptr<u32> fd, u64 flags);
error_code sys_storage_close(u32 fd);
error_code sys_storage_read(u32 fd, u32 mode, u32 start_sector, u32 num_sectors, vm::ptr<void> bounce_buf, vm::ptr<u32> sectors_read, u64 flags);
error_code sys_storage_write(u32 fd, u32 mode, u32 start_sector, u32 num_sectors, vm::ptr<void> data, vm::ptr<u32> sectors_wrote, u64 flags);
error_code sys_storage_send_device_command(u32 dev_handle, u64 cmd, vm::ptr<void> in, u64 inlen, vm::ptr<void> out, u64 outlen);
error_code sys_storage_async_configure(u32 fd, u32 io_buf, u32 equeue_id, u32 unk);
error_code sys_storage_async_read();
error_code sys_storage_async_write();
error_code sys_storage_async_cancel();
error_code sys_storage_get_device_info(u64 device, vm::ptr<StorageDeviceInfo> buffer);
error_code sys_storage_get_device_config(vm::ptr<u32> storages, vm::ptr<u32> devices);
error_code sys_storage_report_devices(u32 storages, u32 start, u32 devices, vm::ptr<u64> device_ids);
error_code sys_storage_configure_medium_event(u32 fd, u32 equeue_id, u32 c);
error_code sys_storage_set_medium_polling_interval();
error_code sys_storage_create_region();
error_code sys_storage_delete_region();
error_code sys_storage_execute_device_command(u32 fd, u64 cmd, vm::ptr<char> cmdbuf, u64 cmdbuf_size, vm::ptr<char> databuf, u64 databuf_size, vm::ptr<u32> driver_status);
error_code sys_storage_check_region_acl();
error_code sys_storage_set_region_acl();
error_code sys_storage_async_send_device_command(u32 dev_handle, u64 cmd, vm::ptr<void> in, u64 inlen, vm::ptr<void> out, u64 outlen, u64 unk);
error_code sys_storage_get_region_offset();
error_code sys_storage_set_emulated_speed();
