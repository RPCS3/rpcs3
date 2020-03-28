#include "stdafx.h"
#include "sys_prx.h"

#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"

#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Crypto/unedat.h"
#include "Utilities/StrUtil.h"
#include "Utilities/span.h"
#include "sys_fs.h"
#include "sys_process.h"
#include "sys_memory.h"

extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);
extern void ppu_unload_prx(const lv2_prx& prx);
extern void ppu_initialize(const ppu_module&);

LOG_CHANNEL(sys_prx);

static const std::unordered_map<std::string, int> s_prx_ignore
{
	{ "/dev_flash/sys/external/libaudio.sprx", 0 },
	{ "/dev_flash/sys/external/libcamera.sprx", 0 },
	{ "/dev_flash/sys/external/libgem.sprx", 0 },
	{ "/dev_flash/sys/external/libio.sprx", 0 },
	{ "/dev_flash/sys/external/libmedi.sprx", 0 },
	{ "/dev_flash/sys/external/libmic.sprx", 0 },
	{ "/dev_flash/sys/external/libnetctl.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_ap.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_authdialog.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_avc_ext.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_avc2.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_avconf_ext.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_bgdl.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_cross_controller.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_dec_psnvideo.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_dtcp_ip.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_game.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_game_exec.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_imejp.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_misc.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_music.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_music_decode.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_music_export.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_clans.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_commerce2.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_eula.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_installer.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_sns.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_trophy.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_tus.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np_util.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_np2.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_oskdialog_ext.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_pesm.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_photo_decode.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_photo_export.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_photo_export2.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_photo_import.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_photo_network_sharing.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_print.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_rec.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_remoteplay.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_rtcalarm.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_savedata.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_savedata_psp.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_screenshot.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_search.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_storagedata.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_subdisplay.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_syschat.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_sysconf_ext.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_userinfo.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_video_export.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_video_player.sprx", 0 },
	{ "/dev_flash/sys/external/libsysutil_video_upload.sprx", 0 },
	{ "/dev_flash/sys/external/libvdec.sprx", 0 },
	{ "/dev_flash/sys/external/libvoice.sprx", 0 },
};

static error_code prx_load_module(const std::string& vpath, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, fs::file src = {})
{
	if (flags != 0)
	{
		if (flags & SYS_PRX_LOAD_MODULE_FLAGS_INVALIDMASK)
		{
			return CELL_EINVAL;
		}

		if (flags & SYS_PRX_LOAD_MODULE_FLAGS_FIXEDADDR && !g_ps3_process_info.ppc_seg)
		{
			return CELL_ENOSYS;
		}

		fmt::throw_exception("sys_prx: Unimplemented fixed address allocations" HERE);
	}

	std::string name = vpath.substr(vpath.find_last_of('/') + 1);
	std::string path = vfs::get(vpath);

	const auto existing = idm::select<lv2_obj, lv2_prx>([&](u32, lv2_prx& prx)
	{
		if (prx.name == name && prx.path == path)
		{
			return true;
		}

		return false;
	});

	if (existing)
	{
		return CELL_PRX_ERROR_LIBRARY_FOUND;
	}

	bool ignore = false;

	if (g_cfg.core.lib_loading == lib_loading_type::liblv2list)
	{
		if (vpath.starts_with("/dev_flash/sys/external/") && vpath != "/dev_flash/sys/external/libsysmodule.sprx"sv)
		{
			ignore = g_cfg.core.load_libraries.get_set().count(name) == 0;
		}
	}
	else
	{
		ignore = s_prx_ignore.count(vpath) != 0;
	}

	if (ignore && (g_cfg.core.lib_loading == lib_loading_type::hybrid || g_cfg.core.lib_loading == lib_loading_type::liblv2both))
	{
		// Ignore ignore list if the library is selected in 'both' mode
		if (g_cfg.core.load_libraries.get_set().count(name) != 0)
		{
			ignore = false;
		}
	}

	if (ignore)
	{
		const auto prx = idm::make_ptr<lv2_obj, lv2_prx>();

		prx->name = std::move(name);
		prx->path = std::move(path);

		sys_prx.warning(u8"Ignored module: “%s” (id=0x%x)", vpath, idm::last_id());

		return not_an_error(idm::last_id());
	}

	if (!src)
	{
		auto [fs_error, ppath, lv2_file] = lv2_file::open(vpath, 0, 0);

		if (fs_error)
		{
			return {fs_error, vpath};
		}

		src = std::move(lv2_file);
	}

	const ppu_prx_object obj = decrypt_self(std::move(src), g_fxo->get<loaded_npdrm_keys>()->devKlic.data());

	if (obj != elf_error::ok)
	{
		return CELL_PRX_ERROR_ILLEGAL_LIBRARY;
	}

	const auto prx = ppu_load_prx(obj, path);

	if (!prx)
	{
		return CELL_PRX_ERROR_ILLEGAL_LIBRARY;
	}

	ppu_initialize(*prx);

	sys_prx.success(u8"Loaded module: “%s” (id=0x%x)", vpath, idm::last_id());

	return not_an_error(idm::last_id());
}

error_code sys_prx_get_ppu_guid()
{
	sys_prx.todo("sys_prx_get_ppu_guid()");
	return CELL_OK;
}

error_code _sys_prx_load_module_by_fd(s32 fd, u64 offset, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_load_module_by_fd(fd=%d, offset=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, flags, pOpt);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	return prx_load_module(fmt::format("%s_x%x", file->name.data(), offset), flags, pOpt, lv2_file::make_view(file, offset));
}

error_code _sys_prx_load_module_on_memcontainer_by_fd(s32 fd, u64 offset, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_load_module_on_memcontainer_by_fd(fd=%d, offset=0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, mem_ct, flags, pOpt);

	return _sys_prx_load_module_by_fd(fd, offset, flags, pOpt);
}

static error_code prx_load_module_list(s32 count, vm::cpptr<char, u32, u64> path_list, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	if (flags != 0)
	{
		if (flags & SYS_PRX_LOAD_MODULE_FLAGS_INVALIDMASK)
		{
			return CELL_EINVAL;
		}

		if (flags & SYS_PRX_LOAD_MODULE_FLAGS_FIXEDADDR && !g_ps3_process_info.ppc_seg)
		{
			return CELL_ENOSYS;
		}

		fmt::throw_exception("sys_prx: Unimplemented fixed address allocations" HERE);
	}

	for (s32 i = 0; i < count; ++i)
	{
		const auto result = prx_load_module(path_list[i].get_ptr(), flags, pOpt);

		if (result < 0)
		{
			while (--i >= 0)
			{
				// Unload already loaded modules
				_sys_prx_unload_module(id_list[i], 0, vm::null);
			}

			// Fill with -1
			std::memset(id_list.get_ptr(), -1, count * sizeof(id_list[0]));
			return result;
		}

		id_list[i] = result;
	}

	return CELL_OK;
}

error_code _sys_prx_load_module_list(s32 count, vm::cpptr<char, u32, u64> path_list, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	sys_prx.warning("_sys_prx_load_module_list(count=%d, path_list=**0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, flags, pOpt, id_list);

	return prx_load_module_list(count, path_list, SYS_MEMORY_CONTAINER_ID_INVALID, flags, pOpt, id_list);
}
error_code _sys_prx_load_module_list_on_memcontainer(s32 count, vm::cpptr<char, u32, u64> path_list, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	sys_prx.warning("_sys_prx_load_module_list_on_memcontainer(count=%d, path_list=**0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, mem_ct, flags, pOpt, id_list);

	return prx_load_module_list(count, path_list, mem_ct, flags, pOpt, id_list);
}

error_code _sys_prx_load_module_on_memcontainer(vm::cptr<char> path, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_load_module_on_memcontainer(path=%s, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", path, mem_ct, flags, pOpt);

	return prx_load_module(path.get_ptr(), flags, pOpt);
}

error_code _sys_prx_load_module(vm::cptr<char> path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_load_module(path=%s, flags=0x%x, pOpt=*0x%x)", path, flags, pOpt);

	return prx_load_module(path.get_ptr(), flags, pOpt);
}

error_code _sys_prx_start_module(u32 id, u64 flags, vm::ptr<sys_prx_start_stop_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_start_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	if (id == 0 || !pOpt)
	{
		return CELL_EINVAL;
	}

	const auto prx = idm::get<lv2_obj, lv2_prx>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	switch (pOpt->cmd & 0xf)
	{
	case 1:
	{
		if (!prx->state.compare_and_swap_test(PRX_STATE_INITIALIZED, PRX_STATE_STARTING))
		{
			// The only error code here
			return CELL_PRX_ERROR_ERROR;
		}

		pOpt->entry.set(prx->start ? prx->start.addr() : ~0ull);
		pOpt->entry2.set(prx->prologue ? prx->prologue.addr() : ~0ull);
		return CELL_OK;
	}
	case 2:
	{
		switch (const u64 res = pOpt->res)
		{
		case SYS_PRX_RESIDENT:
		{
			// No error code on invalid state, so throw on unexpected state
			verify(HERE), prx->state.compare_and_swap_test(PRX_STATE_STARTING, PRX_STATE_STARTED);
			return CELL_OK;
		}
		default:
		{
			if (res & 0xffff'ffffu)
			{
				// Unload the module (SYS_PRX_NO_RESIDENT expected)
				sys_prx.warning("_sys_prx_start_module(): Start entry function returned SYS_PRX_NO_RESIDENT (res=0x%llx)", res);

				// Thread-safe if called from liblv2.sprx, due to internal lwmutex lock before it
				prx->state = PRX_STATE_STOPPED;
				_sys_prx_unload_module(id, 0, vm::null);

				// Return the exact value returned by the start function (as an error)
				return static_cast<s32>(res);
			}

			// Return type of start entry function is s32
			// And according to RE this path results in weird behavior
			sys_prx.error("_sys_prx_start_module(): Start entry function returned weird value (res=0x%llx)", res);
			return CELL_OK;
		}
		}
	}
	default:
		return CELL_PRX_ERROR_ERROR;
	}

	pOpt->entry.set(prx->start ? prx->start.addr() : ~0ull);
	pOpt->entry2.set(prx->prologue ? prx->prologue.addr() : ~0ull);
	return CELL_OK;
}

error_code _sys_prx_stop_module(u32 id, u64 flags, vm::ptr<sys_prx_start_stop_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_stop_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	const auto prx = idm::get<lv2_obj, lv2_prx>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	if (!pOpt)
	{
		return CELL_EINVAL;
	}

	switch (pOpt->cmd & 0xf)
	{
	case 1:
	{
		switch (const auto old = prx->state.compare_and_swap(PRX_STATE_STARTED, PRX_STATE_STOPPING))
		{
		case PRX_STATE_INITIALIZED: return CELL_PRX_ERROR_NOT_STARTED;
		case PRX_STATE_STOPPED: return CELL_PRX_ERROR_ALREADY_STOPPED;
		case PRX_STATE_STOPPING: return CELL_PRX_ERROR_ALREADY_STOPPING; // Internal error
		case PRX_STATE_STARTING: return CELL_PRX_ERROR_ERROR; // Internal error
		case PRX_STATE_STARTED: break;
		default:
			fmt::throw_exception("Invalid prx state (%d)" HERE, old);
		}

		pOpt->entry.set(prx->stop ? prx->stop.addr() : ~0ull);
		pOpt->entry2.set(prx->epilogue ? prx->epilogue.addr() : ~0ull);
		return CELL_OK;
	}
	case 2:
	{
		switch (pOpt->res)
		{
		case 0:
		{
			// No error code on invalid state, so throw on unexpected state
			verify(HERE), prx->state.compare_and_swap_test(PRX_STATE_STOPPING, PRX_STATE_STOPPED);
			return CELL_OK;
		}
		case 1:
			return CELL_PRX_ERROR_CAN_NOT_STOP; // Internal error
		default:
			// Nothing happens (probably unexpected value)
			return CELL_OK;
		}
	}

	// These commands are not used by liblv2.sprx
	case 4: // Get start entry and stop functions
	case 8: // Disable stop function execution
	{
		switch (const auto old = +prx->state)
		{
		case PRX_STATE_INITIALIZED: return CELL_PRX_ERROR_NOT_STARTED;
		case PRX_STATE_STOPPED: return CELL_PRX_ERROR_ALREADY_STOPPED;
		case PRX_STATE_STOPPING: return CELL_PRX_ERROR_ALREADY_STOPPING; // Internal error
		case PRX_STATE_STARTING: return CELL_PRX_ERROR_ERROR; // Internal error
		case PRX_STATE_STARTED: break;
		default:
			fmt::throw_exception("Invalid prx state (%d)" HERE, old);
		}

		if (pOpt->cmd == 4u)
		{
			pOpt->entry.set(prx->stop ? prx->stop.addr() : ~0ull);
			pOpt->entry2.set(prx->epilogue ? prx->epilogue.addr() : ~0ull);
		}
		else
		{
			// Disables stop function execution (but the real value can be read through _sys_prx_get_module_info)
			sys_prx.todo("_sys_prx_stop_module(): cmd is 8 (stop function = *0x%x)", prx->stop);
			//prx->stop = vm::null;
		}
		
		return CELL_OK;
	}
	default:
		return CELL_PRX_ERROR_ERROR;
	}

	return CELL_OK;
}

error_code _sys_prx_unload_module(u32 id, u64 flags, vm::ptr<sys_prx_unload_module_option_t> pOpt)
{
	sys_prx.todo("_sys_prx_unload_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	// Get the PRX, free the used memory and delete the object and its ID
	const auto prx = idm::withdraw<lv2_obj, lv2_prx>(id, [](lv2_prx& prx) -> CellPrxError
	{
		switch (prx.state)
		{
		case PRX_STATE_INITIALIZED:
		case PRX_STATE_STOPPED:
			return {};
		default: break;
		}

		return CELL_PRX_ERROR_NOT_REMOVABLE;
	});

	if (!prx)
	{
		return CELL_PRX_ERROR_UNKNOWN_MODULE;
	}

	if (prx.ret)
	{
		return prx.ret;
	}

	ppu_unload_prx(*prx);

	//s32 result = prx->exit ? prx->exit() : CELL_OK;

	return CELL_OK;
}

error_code _sys_prx_register_module()
{
	sys_prx.todo("_sys_prx_register_module()");
	return CELL_OK;
}

error_code _sys_prx_query_module()
{
	sys_prx.todo("_sys_prx_query_module()");
	return CELL_OK;
}

error_code _sys_prx_register_library(vm::ptr<void> library)
{
	sys_prx.todo("_sys_prx_register_library(library=*0x%x)", library);
	return CELL_OK;
}

error_code _sys_prx_unregister_library(vm::ptr<void> library)
{
	sys_prx.todo("_sys_prx_unregister_library(library=*0x%x)", library);
	return CELL_OK;
}

error_code _sys_prx_link_library()
{
	sys_prx.todo("_sys_prx_link_library()");
	return CELL_OK;
}

error_code _sys_prx_unlink_library()
{
	sys_prx.todo("_sys_prx_unlink_library()");
	return CELL_OK;
}

error_code _sys_prx_query_library()
{
	sys_prx.todo("_sys_prx_query_library()");
	return CELL_OK;
}

error_code _sys_prx_get_module_list(u64 flags, vm::ptr<sys_prx_get_module_list_option_t> pInfo)
{
	if (flags & 0x1)
	{
		sys_prx.todo("_sys_prx_get_module_list(flags=%d, pInfo=*0x%x)", flags, pInfo);
	}
	else
	{
		sys_prx.warning("_sys_prx_get_module_list(flags=%d, pInfo=*0x%x)", flags, pInfo);
	}

	// TODO: Some action occurs if LSB of flags is set here

	if (!(flags & 0x2))
	{
		// Do nothing
		return CELL_OK;
	}

	if (pInfo->size == pInfo.size())
	{
		const u32 max_count = pInfo->max;
		const auto idlist = +pInfo->idlist;
		u32 count = 0;

		if (max_count)
		{
			const std::string liblv2_path = vfs::get("/dev_flash/sys/external/liblv2.sprx");

			idm::select<lv2_obj, lv2_prx>([&](u32 id, lv2_prx& prx)
			{
				if (count >= max_count)
				{
					return true;
				}

				if (prx.path == liblv2_path)
				{
					// Hide liblv2.sprx for now
					return false;
				}

				idlist[count++] = id;
				return false;
			});
		}

		pInfo->count = count;
	}
	else
	{
		// TODO: A different structure should be served here with sizeof == 0x18
		sys_prx.todo("_sys_prx_get_module_list(): Unknown structure specified (size=0x%llx)", pInfo->size);
	}
	
	return CELL_OK;
}

error_code _sys_prx_get_module_info(u32 id, u64 flags, vm::ptr<sys_prx_module_info_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_get_module_info(id=0x%x, flags=%d, pOpt=*0x%x)", id, flags, pOpt);

	const auto prx = idm::get<lv2_obj, lv2_prx>(id);

	if (!pOpt)
	{
		return CELL_EFAULT;
	}

	if (pOpt->size != pOpt.size() || !pOpt->info)
	{
		return CELL_EINVAL;
	}

	if (!prx)
	{
		return CELL_PRX_ERROR_UNKNOWN_MODULE;
	}

	strcpy_trunc(pOpt->info->name, prx->module_info_name);
	pOpt->info->version[0] = prx->module_info_version[0];
	pOpt->info->version[1] = prx->module_info_version[1];
	pOpt->info->modattribute = prx->module_info_attributes;
	pOpt->info->start_entry = prx->start.addr();
	pOpt->info->stop_entry = prx->stop.addr();
	pOpt->info->all_segments_num = ::size32(prx->segs);
	if (pOpt->info->filename)
	{
		gsl::span dst(pOpt->info->filename.get_ptr(), pOpt->info->filename_size);
		strcpy_trunc(dst, prx->name);
	}

	if (pOpt->info->segments)
	{
		u32 i = 0;
		for (; i < prx->segs.size() && i < pOpt->info->segments_num; i++)
		{
			pOpt->info->segments[i].index = i;
			pOpt->info->segments[i].base = prx->segs[i].addr;
			pOpt->info->segments[i].filesz = prx->segs[i].filesz;
			pOpt->info->segments[i].memsz = prx->segs[i].size;
			pOpt->info->segments[i].type = prx->segs[i].type;
		}
		pOpt->info->segments_num = i;
	}

	return CELL_OK;
}

error_code _sys_prx_get_module_id_by_name(vm::cptr<char> name, u64 flags, vm::ptr<sys_prx_get_module_id_by_name_option_t> pOpt)
{
	sys_prx.todo("_sys_prx_get_module_id_by_name(name=%s, flags=%d, pOpt=*0x%x)", name, flags, pOpt);

	//if (realName == "?") ...

	return CELL_PRX_ERROR_UNKNOWN_MODULE;
}

error_code _sys_prx_get_module_id_by_address(u32 addr)
{
	sys_prx.todo("_sys_prx_get_module_id_by_address(addr=0x%x)", addr);
	return CELL_OK;
}

error_code _sys_prx_start()
{
	sys_prx.todo("sys_prx_start()");
	return CELL_OK;
}

error_code _sys_prx_stop()
{
	sys_prx.todo("sys_prx_stop()");
	return CELL_OK;
}

template <>
void fmt_class_string<CellPrxError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellPrxError value)
	{
		switch (value)
		{
		STR_CASE(CELL_PRX_ERROR_ERROR);
		STR_CASE(CELL_PRX_ERROR_ILLEGAL_PERM);
		STR_CASE(CELL_PRX_ERROR_UNKNOWN_MODULE);
		STR_CASE(CELL_PRX_ERROR_ALREADY_STARTED);
		STR_CASE(CELL_PRX_ERROR_NOT_STARTED);
		STR_CASE(CELL_PRX_ERROR_ALREADY_STOPPED);
		STR_CASE(CELL_PRX_ERROR_CAN_NOT_STOP);
		STR_CASE(CELL_PRX_ERROR_NOT_REMOVABLE);
		STR_CASE(CELL_PRX_ERROR_LIBRARY_NOT_YET_LINKED);
		STR_CASE(CELL_PRX_ERROR_LIBRARY_FOUND);
		STR_CASE(CELL_PRX_ERROR_LIBRARY_NOTFOUND);
		STR_CASE(CELL_PRX_ERROR_ILLEGAL_LIBRARY);
		STR_CASE(CELL_PRX_ERROR_LIBRARY_INUSE);
		STR_CASE(CELL_PRX_ERROR_ALREADY_STOPPING);
		STR_CASE(CELL_PRX_ERROR_UNSUPPORTED_PRX_TYPE);
		STR_CASE(CELL_PRX_ERROR_INVAL);
		STR_CASE(CELL_PRX_ERROR_ILLEGAL_PROCESS);
		STR_CASE(CELL_PRX_ERROR_NO_LIBLV2);
		STR_CASE(CELL_PRX_ERROR_UNSUPPORTED_ELF_TYPE);
		STR_CASE(CELL_PRX_ERROR_UNSUPPORTED_ELF_CLASS);
		STR_CASE(CELL_PRX_ERROR_UNDEFINED_SYMBOL);
		STR_CASE(CELL_PRX_ERROR_UNSUPPORTED_RELOCATION_TYPE);
		STR_CASE(CELL_PRX_ERROR_ELF_IS_REGISTERED);
		}

		return unknown;
	});
}
