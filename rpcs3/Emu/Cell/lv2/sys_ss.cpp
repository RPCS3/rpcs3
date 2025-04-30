#include "stdafx.h"
#include "sys_ss.h"

#include "sys_process.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/system_config.h"
#include "util/sysinfo.hpp"

#include <charconv>
#include <shared_mutex>
#include <unordered_set>

#ifdef _WIN32
#include <Windows.h>
#include <bcrypt.h>
#endif

struct lv2_update_manager
{
	lv2_update_manager()
	{
		std::string version_str = utils::get_firmware_version();

		// For example, 4.90 should be converted to 0x4900000000000
		std::erase(version_str, '.');
		if (std::from_chars(version_str.data(), version_str.data() + version_str.size(), system_sw_version, 16).ec != std::errc{})
			system_sw_version <<= 40;
		else
			system_sw_version = 0;
	}

	lv2_update_manager(const lv2_update_manager&) = delete;
	lv2_update_manager& operator=(const lv2_update_manager&) = delete;
	~lv2_update_manager() = default;

	u64 system_sw_version;

	std::unordered_map<u32, u8> eeprom_map // offset, value
	{
		// system language
		// *i think* this gives english
		{0x48C18, 0x00},
		{0x48C19, 0x00},
		{0x48C1A, 0x00},
		{0x48C1B, 0x01},
		// system language end

		// vsh target (seems it can be 0xFFFFFFFE, 0xFFFFFFFF, 0x00000001 default: 0x00000000 / vsh sets it to 0x00000000 on boot if it isn't 0x00000000)
		{0x48C1C, 0x00},
		{0x48C1D, 0x00},
		{0x48C1E, 0x00},
		{0x48C1F, 0x00}
		// vsh target end
	};
	mutable std::shared_mutex eeprom_mutex;

	std::unordered_set<u32> malloc_set;
	mutable std::shared_mutex malloc_mutex;

	// return address
	u32 allocate(u32 size)
	{
		std::unique_lock unique_lock(malloc_mutex);

		if (const auto addr = vm::alloc(size, vm::main); addr)
		{
			malloc_set.emplace(addr);
			return addr;
		}

		return 0;
	}

	// return size
	u32 deallocate(u32 addr)
	{
		std::unique_lock unique_lock(malloc_mutex);

		if (malloc_set.count(addr))
		{
			return vm::dealloc(addr, vm::main);
		}

		return 0;
	}
};

template<>
void fmt_class_string<sys_ss_rng_error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(SYS_SS_RNG_ERROR_INVALID_PKG);
		STR_CASE(SYS_SS_RNG_ERROR_ENOMEM);
		STR_CASE(SYS_SS_RNG_ERROR_EAGAIN);
		STR_CASE(SYS_SS_RNG_ERROR_EFAULT);
		STR_CASE(SYS_SS_RTC_ERROR_UNK);
		}

		return unknown;
	});
}

LOG_CHANNEL(sys_ss);

error_code sys_ss_random_number_generator(u64 pkg_id, vm::ptr<void> buf, u64 size)
{
	sys_ss.warning("sys_ss_random_number_generator(pkg_id=%u, buf=*0x%x, size=0x%x)", pkg_id, buf, size);

	if (pkg_id != 2)
	{
		if (pkg_id == 1)
		{
			if (!g_ps3_process_info.has_root_perm())
			{
				return CELL_ENOSYS;
			}

			sys_ss.todo("sys_ss_random_number_generator(): pkg_id=1");
			std::memset(buf.get_ptr(), 0, 0x18);
			return CELL_OK;
		}

		return SYS_SS_RNG_ERROR_INVALID_PKG;
	}

	// TODO
	if (size > 0x10000000)
	{
		return SYS_SS_RNG_ERROR_ENOMEM;
	}

	std::unique_ptr<u8[]> temp(new u8[size]);

#ifdef _WIN32
	if (auto ret = BCryptGenRandom(nullptr, temp.get(), static_cast<ULONG>(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG))
	{
		fmt::throw_exception("sys_ss_random_number_generator(): BCryptGenRandom failed (0x%08x)", ret);
	}
#else
	fs::file rnd{"/dev/urandom"};

	if (!rnd || rnd.read(temp.get(), size) != size)
	{
		fmt::throw_exception("sys_ss_random_number_generator(): Failed to generate pseudo-random numbers");
	}
#endif

	std::memcpy(buf.get_ptr(), temp.get(), size);
	return CELL_OK;
}

error_code sys_ss_access_control_engine(u64 pkg_id, u64 a2, u64 a3)
{
	sys_ss.success("sys_ss_access_control_engine(pkg_id=0x%llx, a2=0x%llx, a3=0x%llx)", pkg_id, a2, a3);

	const u64 authid = g_ps3_process_info.self_info.valid ?
		g_ps3_process_info.self_info.prog_id_hdr.program_authority_id : 0;

	switch (pkg_id)
	{
	case 0x1:
	{
		if (!g_ps3_process_info.debug_or_root())
		{
			return not_an_error(CELL_ENOSYS);
		}

		if (!a2)
		{
			return CELL_ESRCH;
		}

		ensure(a2 == static_cast<u64>(process_getpid()));
		vm::write64(vm::cast(a3), authid);
		break;
	}
	case 0x2:
	{
		vm::write64(vm::cast(a2), authid);
		break;
	}
	case 0x3:
	{
		if (!g_ps3_process_info.debug_or_root())
		{
			return CELL_ENOSYS;
		}

		break;
	}
	default:
		return 0x8001051du;
	}

	return CELL_OK;
}

error_code sys_ss_get_console_id(vm::ptr<u8> buf)
{
	sys_ss.notice("sys_ss_get_console_id(buf=*0x%x)", buf);

	return sys_ss_appliance_info_manager(0x19003, buf);
}

error_code sys_ss_get_open_psid(vm::ptr<CellSsOpenPSID> psid)
{
	sys_ss.notice("sys_ss_get_open_psid(psid=*0x%x)", psid);

	psid->high = g_cfg.sys.console_psid_high;
	psid->low = g_cfg.sys.console_psid_low;

	return CELL_OK;
}

error_code sys_ss_appliance_info_manager(u32 code, vm::ptr<u8> buffer)
{
	sys_ss.notice("sys_ss_appliance_info_manager(code=0x%x, buffer=*0x%x)", code, buffer);

	if (!g_ps3_process_info.has_root_perm())
		return CELL_ENOSYS;

	if (!buffer)
		return CELL_EFAULT;

	switch (code)
	{
	case 0x19002:
	{
		// AIM_get_device_type
		constexpr u8 product_code[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89 };
		std::memcpy(buffer.get_ptr(), product_code, 16);
		if (g_cfg.core.debug_console_mode)
			buffer[15] = 0x81; // DECR
		break;
	}
	case 0x19003:
	{
		// AIM_get_device_id
		constexpr u8 idps[] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x89, 0x00, 0x0B, 0x14, 0x00, 0xEF, 0xDD, 0xCA, 0x25, 0x52, 0x66 };
		std::memcpy(buffer.get_ptr(), idps, 16);
		if (g_cfg.core.debug_console_mode)
		{
			buffer[5] = 0x81; // DECR
			buffer[7] = 0x09; // DECR-1400
		}
		break;
	}
	case 0x19004:
	{
		// AIM_get_ps_code
		constexpr u8 pscode[] = { 0x00, 0x01, 0x00, 0x85, 0x00, 0x07, 0x00, 0x04 };
		std::memcpy(buffer.get_ptr(), pscode, 8);
		break;
	}
	case 0x19005:
	{
		// AIM_get_open_ps_id
		be_t<u64> psid[2] = { +g_cfg.sys.console_psid_high, +g_cfg.sys.console_psid_low };
		std::memcpy(buffer.get_ptr(), psid, 16);
		break;
	}
	case 0x19006:
	{
		// qa values (dex only) ??
		[[fallthrough]];
	}
	default: sys_ss.todo("sys_ss_appliance_info_manager(code=0x%x, buffer=*0x%x)", code, buffer);
	}

	return CELL_OK;
}

error_code sys_ss_get_cache_of_product_mode(vm::ptr<u8> ptr)
{
	sys_ss.todo("sys_ss_get_cache_of_product_mode(ptr=*0x%x)", ptr);

	if (!ptr)
	{
		return CELL_EINVAL;
	}
	// 0xff Happens when hypervisor call returns an error
	// 0 - disabled
	// 1 - enabled

	// except something segfaults when using 0, so error it is!
	*ptr = 0xFF;

	return CELL_OK;
}

error_code sys_ss_secure_rtc(u64 cmd, u64 a2, u64 a3, u64 a4)
{
	sys_ss.todo("sys_ss_secure_rtc(cmd=0x%llx, a2=0x%x, a3=0x%llx, a4=0x%llx)", cmd, a2, a3, a4);
	if (cmd == 0x3001)
	{
		if (a3 != 0x20)
			return 0x80010500; // bad packet id

		return CELL_OK;
	}
	else if (cmd == 0x3002)
	{
		// Get time
		if (a2 > 1)
			return 0x80010500; // bad packet id

		// a3 is actual output, not 100% sure, but best guess is its tb val
		vm::write64(::narrow<u32>(a3), get_timebased_time());
		// a4 is a pointer to status, non 0 on error
		vm::write64(::narrow<u32>(a4), 0);
		return CELL_OK;
	}
	else if (cmd == 0x3003)
	{
		return CELL_OK;
	}

	return 0x80010500; // bad packet id
}

error_code sys_ss_get_cache_of_flash_ext_flag(vm::ptr<u64> flag)
{
	sys_ss.todo("sys_ss_get_cache_of_flash_ext_flag(flag=*0x%x)", flag);

	if (!flag)
	{
		return CELL_EFAULT;
	}

	*flag = 0xFE; // nand vs nor from lsb

	return CELL_OK;
}

error_code sys_ss_get_boot_device(vm::ptr<u64> dev)
{
	sys_ss.todo("sys_ss_get_boot_device(dev=*0x%x)", dev);

	if (!dev)
	{
		return CELL_EINVAL;
	}

	*dev = 0x190;

	return CELL_OK;
}

error_code sys_ss_update_manager(u64 pkg_id, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)
{
	sys_ss.notice("sys_ss_update_manager(pkg=0x%x, a1=0x%x, a2=0x%x, a3=0x%x, a4=0x%x, a5=0x%x, a6=0x%x)", pkg_id, a1, a2, a3, a4, a5, a6);

	if (!g_ps3_process_info.has_root_perm())
		return CELL_ENOSYS;

	auto& update_manager = g_fxo->get<lv2_update_manager>();

	switch (pkg_id)
	{
	case 0x6001:
	{
		// update package async
		break;
	}
	case 0x6002:
	{
		// inspect package async
		break;
	}
	case 0x6003:
	{
		// get installed package info
		[[maybe_unused]] const auto type = ::narrow<u32>(a1);
		const auto info_ptr = ::narrow<u32>(a2);

		if (!info_ptr)
			return CELL_EFAULT;

		vm::write64(info_ptr, update_manager.system_sw_version);

		break;
	}
	case 0x6004:
	{
		// get fix instruction
		break;
	}
	case 0x6005:
	{
		// extract package async
		break;
	}
	case 0x6006:
	{
		// get extract package
		break;
	}
	case 0x6007:
	{
		// get flash initialized
		break;
	}
	case 0x6008:
	{
		// set flash initialized
		break;
	}
	case 0x6009:
	{
		// get seed token
		break;
	}
	case 0x600A:
	{
		// set seed token
		break;
	}
	case 0x600B:
	{
		// read eeprom
		const auto offset = ::narrow<u32>(a1);
		const auto value_ptr = ::narrow<u32>(a2);

		if (!value_ptr)
			return CELL_EFAULT;

		std::shared_lock shared_lock(update_manager.eeprom_mutex);

		if (const auto iterator = update_manager.eeprom_map.find(offset); iterator != update_manager.eeprom_map.end())
			vm::write8(value_ptr, iterator->second);
		else
			vm::write8(value_ptr, 0xFF); // 0xFF if not set

		break;
	}
	case 0x600C:
	{
		// write eeprom
		const auto offset = ::narrow<u32>(a1);
		const auto value = ::narrow<u8>(a2);

		std::unique_lock unique_lock(update_manager.eeprom_mutex);

		if (value != 0xFF)
			update_manager.eeprom_map[offset] = value;
		else
			update_manager.eeprom_map.erase(offset); // 0xFF: unset

		break;
	}
	case 0x600D:
	{
		// get async status
		break;
	}
	case 0x600E:
	{
		// allocate buffer
		const auto size = ::narrow<u32>(a1);
		const auto addr_ptr = ::narrow<u32>(a2);

		if (!addr_ptr)
			return CELL_EFAULT;

		const auto addr = update_manager.allocate(size);

		if (!addr)
			return CELL_ENOMEM;

		vm::write32(addr_ptr, addr);

		break;
	}
	case 0x600F:
	{
		// release buffer
		const auto addr = ::narrow<u32>(a1);

		if (!update_manager.deallocate(addr))
			return CELL_ENOMEM;

		break;
	}
	case 0x6010:
	{
		// check integrity
		break;
	}
	case 0x6011:
	{
		// get applicable version
		const auto addr_ptr = ::narrow<u32>(a2);

		if (!addr_ptr)
			return CELL_EFAULT;

		vm::write64(addr_ptr, 0x30040ULL << 32); // 3.40

		break;
	}
	case 0x6012:
	{
		// allocate buffer from memory container
		[[maybe_unused]] const auto mem_ct = ::narrow<u32>(a1);
		const auto size = ::narrow<u32>(a2);
		const auto addr_ptr = ::narrow<u32>(a3);

		if (!addr_ptr)
			return CELL_EFAULT;

		const auto addr = update_manager.allocate(size);

		if (!addr)
			return CELL_ENOMEM;

		vm::write32(addr_ptr, addr);

		break;
	}
	case 0x6013:
	{
		// unknown
		break;
	}
	default:
	{
		sys_ss.error("sys_ss_update_manager(): invalid packet id 0x%x ", pkg_id);
		return CELL_EINVAL;
	}
	}

	return CELL_OK;
}

error_code sys_ss_virtual_trm_manager(u64 pkg_id, u64 a1, u64 a2, u64 a3, u64 a4)
{
	sys_ss.todo("sys_ss_virtual_trm_manager(pkg=0x%llx, a1=0x%llx, a2=0x%llx, a3=0x%llx, a4=0x%llx)", pkg_id, a1, a2, a3, a4);

	return CELL_OK;
}

error_code sys_ss_individual_info_manager(u64 pkg_id, u64 a2, vm::ptr<u64> out_size, u64 a4, u64 a5, u64 a6)
{
	sys_ss.todo("sys_ss_individual_info_manager(pkg=0x%llx, a2=0x%llx, out_size=*0x%llx, a4=0x%llx, a5=0x%llx, a6=0x%llx)", pkg_id, a2, out_size, a4, a5, a6);

	switch (pkg_id)
	{
	// Read EID
	case 0x17002:
	{
		// TODO
		vm::write<u64>(static_cast<u32>(a5), a4); // Write back size of buffer
		break;
	}
	// Get EID size
	case 0x17001: *out_size = 0x100; break;
	default: break;
	}

	return CELL_OK;
}
