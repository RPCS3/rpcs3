#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_fs.h"
#include "cellSysutil.h"
#include "util/init_mutex.hpp"
#include "Utilities/StrUtil.h"

LOG_CHANNEL(cellSysutil);

template<>
void fmt_class_string<CellSysCacheError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SYSCACHE_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_SYSCACHE_ERROR_INTERNAL);
			STR_CASE(CELL_SYSCACHE_ERROR_NOTMOUNTED);
			STR_CASE(CELL_SYSCACHE_ERROR_PARAM);
		}

		return unknown;
	});
}

struct syscache_info
{
	const std::string cache_root = Emu.GetHdd1Dir() + "/caches/";

	stx::init_mutex init;

	std::string cache_id;

	syscache_info() noexcept
	{
		// Check if dev_hdd1 is mounted by parent process
		if (!Emu.hdd1.empty())
		{
			const auto lock = init.init();

			// Extract cache id from path
			cache_id = Emu.hdd1;
			if (cache_id.back() == '/')
				cache_id.resize(cache_id.size() - 1);
			cache_id = cache_id.substr(cache_id.find_last_of('/') + 1);

			cellSysutil.success("Retained cache from parent process: %s", Emu.hdd1);
			return;
		}

		// Find existing cache at startup
		const std::string prefix = Emu.GetTitleID() + '_';

		for (auto&& entry : fs::dir(cache_root))
		{
			if (entry.is_directory && entry.name.starts_with(prefix))
			{
				cache_id = std::move(entry.name);
				break;
			}
		}
	}

	void clear(bool remove_root) noexcept
	{
		// Clear cache
		if (!vfs::host::remove_all(cache_root + cache_id, cache_root, remove_root))
		{
			cellSysutil.fatal("cellSysCache: failed to clear cache directory '%s%s' (%s)", cache_root, cache_id, fs::g_tls_error);
		}

		// Poison opened files in /dev_hdd1 to return CELL_EIO on access
		if (remove_root)
		{
			idm::select<lv2_fs_object, lv2_file>([](u32 id, lv2_file& file)
			{
				if (std::memcmp("/dev_hdd1", file.name.data(), 9) == 0)
				{
					file.lock = 2;
				}
			});
		}
	}
};

error_code cellSysCacheClear()
{
	cellSysutil.notice("cellSysCacheClear()");

	const auto cache = g_fxo->get<syscache_info>();

	const auto lock = cache->init.access();

	if (!lock)
	{
		return CELL_SYSCACHE_ERROR_NOTMOUNTED;
	}

	// Clear existing cache
	if (!cache->cache_id.empty())
	{
		cache->clear(false);
	}

	return not_an_error(CELL_SYSCACHE_RET_OK_CLEARED);
}

error_code cellSysCacheMount(vm::ptr<CellSysCacheParam> param)
{
	cellSysutil.notice("cellSysCacheMount(param=*0x%x)", param);

	const auto cache = g_fxo->get<syscache_info>();

	if (!param || (param->cacheId[0] && sysutil_check_name_string(param->cacheId, 1, CELL_SYSCACHE_ID_SIZE) != 0))
	{
		return CELL_SYSCACHE_ERROR_PARAM;
	}

	// Full virtualized cache id (with title id included)
	std::string cache_id = vfs::escape(Emu.GetTitleID() + '_' + param->cacheId);

	// Full path to virtual cache root (/dev_hdd1)
	std::string new_path = cache->cache_root + cache_id + '/';

	// Set fixed VFS path
	strcpy_trunc(param->getCachePath, "/dev_hdd1");

	// Lock pseudo-mutex
	const auto lock = cache->init.init_always([&]
	{
	});

	// Check if can reuse existing cache (won't if cache id is an empty string)
	if (param->cacheId[0] && cache_id == cache->cache_id)
	{
		// Isn't mounted yet on first call to cellSysCacheMount
		vfs::mount("/dev_hdd1", new_path);

		cellSysutil.success("Mounted existing cache at %s", new_path);
		return not_an_error(CELL_SYSCACHE_RET_OK_RELAYED);
	}

	// Clear existing cache
	if (!cache->cache_id.empty())
	{
		cache->clear(true);
	}

	// Set new cache id
	cache->cache_id = std::move(cache_id);
	fs::create_dir(new_path);
	vfs::mount("/dev_hdd1", new_path);

	return not_an_error(CELL_SYSCACHE_RET_OK_CLEARED);
}


extern void cellSysutil_SysCache_init()
{
	REG_FUNC(cellSysutil, cellSysCacheMount);
	REG_FUNC(cellSysutil, cellSysCacheClear);
}
