#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Crypto/unedat.h"
#include "sys_prx.h"

namespace vm { using namespace ps3; }

extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);
extern void ppu_unload_prx(const lv2_prx& prx);
extern void ppu_initialize(const ppu_module&);

logs::channel sys_prx("sys_prx");

static const std::unordered_map<std::string, int> s_prx_ignore
{
	{ "/dev_flash/sys/external/libad_async.sprx", 0 },
	{ "/dev_flash/sys/external/libad_billboard_util.sprx", 0 },
	{ "/dev_flash/sys/external/libad_core.sprx", 0 },
	{ "/dev_flash/sys/external/libaudio.sprx", 0 },
	{ "/dev_flash/sys/external/libbeisobmf.sprx", 0 },
	{ "/dev_flash/sys/external/libcamera.sprx", 0 },
	{ "/dev_flash/sys/external/libgem.sprx", 0 },
	{ "/dev_flash/sys/external/libhttp.sprx", 0 },
	{ "/dev_flash/sys/external/libio.sprx", 0 },
	{ "/dev_flash/sys/external/libmedi.sprx", 0 },
	{ "/dev_flash/sys/external/libmic.sprx", 0 },
	{ "/dev_flash/sys/external/libnet.sprx", 0 },
	{ "/dev_flash/sys/external/libnetctl.sprx", 0 },
	{ "/dev_flash/sys/external/librudp.sprx", 0 },
	{ "/dev_flash/sys/external/libssl.sprx", 0 },
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
	{ "/dev_flash/sys/external/libusbd.sprx", 0 },
	{ "/dev_flash/sys/external/libusbpspcm.sprx", 0 },
	{ "/dev_flash/sys/external/libvdec.sprx", 0 },
	{ "/dev_flash/sys/external/libvoice.sprx", 0 },
};

error_code prx_load_module(std::string path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	const auto name = path.substr(path.find_last_of('/') + 1);

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

	if (s_prx_ignore.count(path))
	{
		sys_prx.warning("Ignored module: %s", path);

		const auto prx = idm::make_ptr<lv2_obj, lv2_prx>();

		prx->name = name;
		prx->path = path;

		return not_an_error(idm::last_id());
	}

	const auto loadedkeys = fxm::get_always<LoadedNpdrmKeys_t>();

	const ppu_prx_object obj = decrypt_self(fs::file(vfs::get(path)), loadedkeys->devKlic.data());

	if (obj != elf_error::ok)
	{
		return CELL_PRX_ERROR_ILLEGAL_LIBRARY;
	}

	const auto prx = ppu_load_prx(obj, vfs::get(path));

	if (!prx)
	{
		return CELL_PRX_ERROR_ILLEGAL_LIBRARY;
	}

	ppu_initialize(*prx);

	sys_prx.success("Loaded module: %s", path);

	return not_an_error(idm::last_id());
}

error_code sys_prx_get_ppu_guid()
{
	sys_prx.todo("sys_prx_get_ppu_guid()");
	return CELL_OK;
}

error_code _sys_prx_load_module_by_fd(s32 fd, u64 offset, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.todo("_sys_prx_load_module_by_fd(fd=%d, offset=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, flags, pOpt);
	return CELL_OK;
}

error_code _sys_prx_load_module_on_memcontainer_by_fd(s32 fd, u64 offset, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.todo("_sys_prx_load_module_on_memcontainer_by_fd(fd=%d, offset=0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, mem_ct, flags, pOpt);
	return CELL_OK;
}

error_code _sys_prx_load_module_list(s32 count, vm::cpptr<char, u32, u64> path_list, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	sys_prx.warning("_sys_prx_load_module_list(count=%d, path_list=**0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, flags, pOpt, id_list);

	for (s32 i = 0; i < count; ++i)
	{
		auto path = path_list[i];
		std::string name = path.get_ptr();
		error_code result = prx_load_module(name, flags, pOpt);

		if (result < 0)
			return result;

		id_list[i] = result;
	}

	return CELL_OK;
}

error_code _sys_prx_load_module_list_on_memcontainer(s32 count, vm::cpptr<char, u32, u64> path_list, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	sys_prx.todo("_sys_prx_load_module_list_on_memcontainer(count=%d, path_list=**0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, mem_ct, flags, pOpt, id_list);

	return CELL_OK;
}

error_code _sys_prx_load_module_on_memcontainer(vm::cptr<char> path, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.todo("_sys_prx_load_module_on_memcontainer(path=%s, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", path, mem_ct, flags, pOpt);
	return CELL_OK;
}

error_code _sys_prx_load_module(vm::cptr<char> path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_load_module(path=%s, flags=0x%x, pOpt=*0x%x)", path, flags, pOpt);

	return prx_load_module(path.get_ptr(), flags, pOpt);
}

error_code _sys_prx_start_module(u32 id, u64 flags, vm::ptr<sys_prx_start_stop_module_option_t> pOpt)
{
	sys_prx.warning("_sys_prx_start_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	const auto prx = idm::get<lv2_obj, lv2_prx>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	//if (prx->is_started)
	//	return CELL_PRX_ERROR_ALREADY_STARTED;

	//prx->is_started = true;
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

	//if (!prx->is_started)
	//	return CELL_PRX_ERROR_ALREADY_STOPPED;

	//prx->is_started = false;
	pOpt->entry.set(prx->stop ? prx->stop.addr() : ~0ull);
	pOpt->entry2.set(prx->epilogue ? prx->epilogue.addr() : ~0ull);

	return CELL_OK;
}

error_code _sys_prx_unload_module(u32 id, u64 flags, vm::ptr<sys_prx_unload_module_option_t> pOpt)
{
	sys_prx.todo("_sys_prx_unload_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	// Get the PRX, free the used memory and delete the object and its ID
	const auto prx = idm::withdraw<lv2_obj, lv2_prx>(id);

	if (!prx)
	{
		return CELL_ESRCH;
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
	sys_prx.todo("_sys_prx_get_module_list(flags=%d, pInfo=*0x%x)", flags, pInfo);
	return CELL_OK;
}

error_code _sys_prx_get_module_info(u32 id, u64 flags, vm::ptr<sys_prx_module_info_option_t> pOpt)
{
	sys_prx.todo("_sys_prx_get_module_info(id=0x%x, flags=%d, pOpt=*0x%x)", id, flags, pOpt);
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
