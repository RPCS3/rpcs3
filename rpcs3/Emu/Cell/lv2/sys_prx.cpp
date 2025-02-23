#include "stdafx.h"
#include "sys_prx.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Crypto/unself.h"
#include "Loader/ELF.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Crypto/unedat.h"
#include "Utilities/StrUtil.h"
#include "sys_fs.h"
#include "sys_process.h"
#include "sys_memory.h"
#include <span>

extern void dump_executable(std::span<const u8> data, const ppu_module<lv2_obj>* _module, std::string_view title_id);

extern shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, bool virtual_load, const std::string&, s64, utils::serial* = nullptr);
extern void ppu_unload_prx(const lv2_prx& prx);
extern bool ppu_initialize(const ppu_module<lv2_obj>&, bool check_only = false, u64 file_size = 0);
extern void ppu_finalize(const ppu_module<lv2_obj>& info, bool force_mem_release = false);
extern void ppu_manual_load_imports_exports(u32 imports_start, u32 imports_size, u32 exports_start, u32 exports_size, std::basic_string<char>& loaded_flags);

LOG_CHANNEL(sys_prx);

// <string: firmware sprx, int: should hle if 1>
extern const std::map<std::string_view, int> g_prx_list
{
	{ "/dev_flash/sys/internal/libfs_utility_init.sprx", 1 },
	{ "libaacenc.sprx", 0 },
	{ "libaacenc_spurs.sprx", 0 },
	{ "libac3dec.sprx", 0 },
	{ "libac3dec2.sprx", 0 },
	{ "libadec.sprx", 1 },
	{ "libadec2.sprx", 0 },
	{ "libadec_internal.sprx", 0 },
	{ "libad_async.sprx", 0 },
	{ "libad_billboard_util.sprx", 0 },
	{ "libad_core.sprx", 0 },
	{ "libapostsrc_mini.sprx", 0 },
	{ "libasfparser2_astd.sprx", 0 },
	{ "libat3dec.sprx", 0 },
	{ "libat3multidec.sprx", 0 },
	{ "libatrac3multi.sprx", 0 },
	{ "libatrac3plus.sprx", 0 },
	{ "libatxdec.sprx", 1 },
	{ "libatxdec2.sprx", 0 },
	{ "libaudio.sprx", 1 },
	{ "libavcdec.sprx", 0 },
	{ "libavcenc.sprx", 0 },
	{ "libavcenc_small.sprx", 0 },
	{ "libavchatjpgdec.sprx", 0 },
	{ "libbeisobmf.sprx", 0 },
	{ "libbemp2sys.sprx", 0 },
	{ "libcamera.sprx", 1 },
	{ "libcelp8dec.sprx", 0 },
	{ "libcelp8enc.sprx", 0 },
	{ "libcelpdec.sprx", 0 },
	{ "libcelpenc.sprx", 0 },
	{ "libddpdec.sprx", 0 },
	{ "libdivxdec.sprx", 0 },
	{ "libdmux.sprx", 0 },
	{ "libdmuxpamf.sprx", 0 },
	{ "libdtslbrdec.sprx", 0 },
	{ "libfiber.sprx", 0 },
	{ "libfont.sprx", 0 },
	{ "libfontFT.sprx", 0 },
	{ "libfreetype.sprx", 0 },
	{ "libfreetypeTT.sprx", 0 },
	{ "libfs.sprx", 0 },
	{ "libfs_155.sprx", 0 },
	{ "libgcm_sys.sprx", 0 },
	{ "libgem.sprx", 1 },
	{ "libgifdec.sprx", 0 },
	{ "libhttp.sprx", 0 },
	{ "libio.sprx", 1 },
	{ "libjpgdec.sprx", 0 },
	{ "libjpgenc.sprx", 0 },
	{ "libkey2char.sprx", 0 },
	{ "libl10n.sprx", 0 },
	{ "liblv2.sprx", 0 },
	{ "liblv2coredump.sprx", 0 },
	{ "liblv2dbg_for_cex.sprx", 0 },
	{ "libm2bcdec.sprx", 0 },
	{ "libm4aacdec.sprx", 0 },
	{ "libm4aacdec2ch.sprx", 0 },
	{ "libm4hdenc.sprx", 0 },
	{ "libm4venc.sprx", 0 },
	{ "libmedi.sprx", 1 },
	{ "libmic.sprx", 1 },
	{ "libmp3dec.sprx", 0 },
	{ "libmp4.sprx", 0 },
	{ "libmpl1dec.sprx", 0 },
	{ "libmvcdec.sprx", 0 },
	{ "libnet.sprx", 0 },
	{ "libnetctl.sprx", 1 },
	{ "libpamf.sprx", 1 },
	{ "libpngdec.sprx", 0 },
	{ "libpngenc.sprx", 0 },
	{ "libresc.sprx", 0 },
	{ "librtc.sprx", 1 },
	{ "librudp.sprx", 0 },
	{ "libsail.sprx", 0 },
	{ "libsail_avi.sprx", 0 },
	{ "libsail_rec.sprx", 0 },
	{ "libsjvtd.sprx", 0 },
	{ "libsmvd2.sprx", 0 },
	{ "libsmvd4.sprx", 0 },
	{ "libspurs_jq.sprx", 0 },
	{ "libsre.sprx", 0 },
	{ "libssl.sprx", 0 },
	{ "libsvc1d.sprx", 0 },
	{ "libsync2.sprx", 0 },
	{ "libsysmodule.sprx", 0 },
	{ "libsysutil.sprx", 1 },
	{ "libsysutil_ap.sprx", 1 },
	{ "libsysutil_authdialog.sprx", 1 },
	{ "libsysutil_avc2.sprx", 1 },
	{ "libsysutil_avconf_ext.sprx", 1 },
	{ "libsysutil_avc_ext.sprx", 1 },
	{ "libsysutil_bgdl.sprx", 1 },
	{ "libsysutil_cross_controller.sprx", 1 },
	{ "libsysutil_dec_psnvideo.sprx", 1 },
	{ "libsysutil_dtcp_ip.sprx", 1 },
	{ "libsysutil_game.sprx", 1 },
	{ "libsysutil_game_exec.sprx", 1 },
	{ "libsysutil_imejp.sprx", 1 },
	{ "libsysutil_misc.sprx", 1 },
	{ "libsysutil_music.sprx", 1 },
	{ "libsysutil_music_decode.sprx", 1 },
	{ "libsysutil_music_export.sprx", 1 },
	{ "libsysutil_np.sprx", 1 },
	{ "libsysutil_np2.sprx", 1 },
	{ "libsysutil_np_clans.sprx", 1 },
	{ "libsysutil_np_commerce2.sprx", 1 },
	{ "libsysutil_np_eula.sprx", 1 },
	{ "libsysutil_np_installer.sprx", 1 },
	{ "libsysutil_np_sns.sprx", 1 },
	{ "libsysutil_np_trophy.sprx", 1 },
	{ "libsysutil_np_tus.sprx", 1 },
	{ "libsysutil_np_util.sprx", 1 },
	{ "libsysutil_oskdialog_ext.sprx", 1 },
	{ "libsysutil_pesm.sprx", 1 },
	{ "libsysutil_photo_decode.sprx", 1 },
	{ "libsysutil_photo_export.sprx", 1 },
	{ "libsysutil_photo_export2.sprx", 1 },
	{ "libsysutil_photo_import.sprx", 1 },
	{ "libsysutil_photo_network_sharing.sprx", 1 },
	{ "libsysutil_print.sprx", 1 },
	{ "libsysutil_rec.sprx", 1 },
	{ "libsysutil_remoteplay.sprx", 1 },
	{ "libsysutil_rtcalarm.sprx", 1 },
	{ "libsysutil_savedata.sprx", 1 },
	{ "libsysutil_savedata_psp.sprx", 1 },
	{ "libsysutil_screenshot.sprx", 1 },
	{ "libsysutil_search.sprx", 1 },
	{ "libsysutil_storagedata.sprx", 1 },
	{ "libsysutil_subdisplay.sprx", 1 },
	{ "libsysutil_syschat.sprx", 1 },
	{ "libsysutil_sysconf_ext.sprx", 1 },
	{ "libsysutil_userinfo.sprx", 1 },
	{ "libsysutil_video_export.sprx", 1 },
	{ "libsysutil_video_player.sprx", 1 },
	{ "libsysutil_video_upload.sprx", 1 },
	{ "libusbd.sprx", 0 },
	{ "libusbpspcm.sprx", 0 },
	{ "libvdec.sprx", 1 },
	{ "libvoice.sprx", 1 },
	{ "libvpost.sprx", 0 },
	{ "libvpost2.sprx", 0 },
	{ "libwmadec.sprx", 0 },
};

bool ppu_register_library_lock(std::string_view libname, bool lock_lib);

static error_code prx_load_module(const std::string& vpath, u64 flags, vm::ptr<sys_prx_load_module_option_t> /*pOpt*/, fs::file src = {}, s64 file_offset = 0)
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

		fmt::throw_exception("sys_prx: Unimplemented fixed address allocations");
	}

	std::string vpath0;
	std::string path = vfs::get(vpath, nullptr, &vpath0);
	std::string name = vpath0.substr(vpath0.find_last_of('/') + 1);

	bool ignore = false;

	constexpr std::string_view firmware_sprx_dir = "/dev_flash/sys/external/";
	const bool is_firmware_sprx = vpath0.starts_with(firmware_sprx_dir) && g_prx_list.count(std::string_view(vpath0).substr(firmware_sprx_dir.size()));

	if (is_firmware_sprx)
	{
		if (g_cfg.core.libraries_control.get_set().count(name + ":lle"))
		{
			// Force LLE
			ignore = false;
		}
		else if (g_cfg.core.libraries_control.get_set().count(name + ":hle"))
		{
			// Force HLE
			ignore = true;
		}
		else
		{
			// Use list
			ignore = ::at32(g_prx_list, name) != 0;
		}
	}
	else if (vpath0.starts_with("/"))
	{
		// Special case : HLE for files outside of "/dev_flash/sys/external/"
		// Have to specify full path for them
		ignore = g_prx_list.count(vpath0) && ::at32(g_prx_list, vpath0);
	}

	auto hle_load = [&]()
	{
		const auto prx = idm::make_ptr<lv2_obj, lv2_prx>();

		prx->name = std::move(name);
		prx->path = std::move(path);

		sys_prx.warning("Ignored module: \"%s\" (id=0x%x)", vpath, idm::last_id());

		return not_an_error(idm::last_id());
	};

	if (ignore)
	{
		return hle_load();
	}

	if (!src)
	{
		auto [fs_error, ppath, path0, lv2_file, type] = lv2_file::open(vpath, 0, 0);

		if (fs_error)
		{
			if (fs_error + 0u == CELL_ENOENT && is_firmware_sprx)
			{
				sys_prx.error("firmware SPRX not found: \"%s\" (forcing HLE implementation)", vpath, idm::last_id());
				return hle_load();
			}

			return {fs_error, vpath};
		}

		src = std::move(lv2_file);
	}

	u128 klic = g_fxo->get<loaded_npdrm_keys>().last_key();

	src = decrypt_self(std::move(src), reinterpret_cast<u8*>(&klic));

	if (!src)
	{
		return {CELL_PRX_ERROR_UNSUPPORTED_PRX_TYPE, +"Failed to decrypt file"};
	}

	const auto src_data = g_cfg.core.ppu_debug ? src.to_vector<u8>() : std::vector<u8>{};

	ppu_prx_object obj = std::move(src);
	src.close();

	if (obj != elf_error::ok)
	{
		return {CELL_PRX_ERROR_UNSUPPORTED_PRX_TYPE, obj.get_error()};
	}

	const auto prx = ppu_load_prx(obj, false, path, file_offset);

	if (g_cfg.core.ppu_debug)
	{
		dump_executable({src_data.data(), src_data.size()}, prx.get(), Emu.GetTitleID());
	}

	obj.clear();

	if (!prx)
	{
		return CELL_PRX_ERROR_ILLEGAL_LIBRARY;
	}

	ppu_initialize(*prx);

	sys_prx.success("Loaded module: \"%s\" (id=0x%x)", vpath, idm::last_id());

	return not_an_error(idm::last_id());
}

fs::file make_file_view(fs::file&& file, u64 offset, u64 size);

std::function<void(void*)> lv2_prx::load(utils::serial& ar)
{
	[[maybe_unused]] const s32 version = GET_SERIALIZATION_VERSION(lv2_prx_overlay);

	const std::string path = vfs::get(ar.pop<std::string>());
	const s64 offset = ar;
	const u32 state = ar;

	usz seg_count = 0;
	ar.deserialize_vle(seg_count);

	shared_ptr<lv2_prx> prx;

	auto hle_load = [&]()
	{
		prx = make_shared<lv2_prx>();
		prx->path = path;
		prx->name = path.substr(path.find_last_of(fs::delim) + 1);
	};

	if (seg_count)
	{
		std::basic_string<char> loaded_flags, external_flags;

		ar(loaded_flags, external_flags);

		fs::file file{path.substr(0, path.size() - (offset ? fmt::format("_x%x", offset).size() : 0))};

		if (file)
		{
			u128 klic = g_fxo->get<loaded_npdrm_keys>().last_key();
			file = make_file_view(std::move(file), offset, umax);
			prx = ppu_load_prx(ppu_prx_object{decrypt_self(std::move(file), reinterpret_cast<u8*>(&klic))}, false, path, 0, &ar);
			prx->m_loaded_flags = std::move(loaded_flags);
			prx->m_external_loaded_flags = std::move(external_flags);

			if (state <= PRX_STATE_STARTED)
			{
				prx->restore_exports();
			}

			ensure(prx);
		}
		else
		{
			ensure(g_cfg.savestate.state_inspection_mode.get());

			hle_load();

			// Partially recover information
			for (usz i = 0; i < seg_count; i++)
			{
				auto& seg = prx->segs.emplace_back();
				seg.addr = ar;
				seg.size = 1; // TODO
			}
		}
	}
	else
	{
		hle_load();
	}

	prx->state = state;

	return [prx](void* storage)
	{
		*static_cast<atomic_ptr<lv2_obj>*>(storage) = prx;
	};
}

void lv2_prx::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_prx_overlay);

	ar(vfs::retrieve(path), offset, state);

	// Save segments count
	ar.serialize_vle(segs.size());

	if (!segs.empty())
	{
		ar(m_loaded_flags);
		ar(m_external_loaded_flags);
	}

	for (const ppu_segment& seg : segs)
	{
		if (seg.type == 0x1u && seg.size) ar(seg.addr);
	}
}

error_code sys_prx_get_ppu_guid(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("sys_prx_get_ppu_guid()");
	return CELL_OK;
}

error_code _sys_prx_load_module_by_fd(ppu_thread& ppu, s32 fd, u64 offset, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_load_module_by_fd(fd=%d, offset=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, flags, pOpt);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::lock_guard lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	return prx_load_module(offset ? fmt::format("%s_x%x", file->name.data(), offset) : file->name.data(), flags, pOpt, lv2_file::make_view(file, offset), offset);
}

error_code _sys_prx_load_module_on_memcontainer_by_fd(ppu_thread& ppu, s32 fd, u64 offset, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_load_module_on_memcontainer_by_fd(fd=%d, offset=0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", fd, offset, mem_ct, flags, pOpt);

	return _sys_prx_load_module_by_fd(ppu, fd, offset, flags, pOpt);
}

static error_code prx_load_module_list(ppu_thread& ppu, s32 count, vm::cpptr<char, u32, u64> path_list, u32 /*mem_ct*/, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
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

		fmt::throw_exception("sys_prx: Unimplemented fixed address allocations");
	}

	for (s32 i = 0; i < count; ++i)
	{
		const auto result = prx_load_module(path_list[i].get_ptr(), flags, pOpt);

		if (result < 0)
		{
			while (--i >= 0)
			{
				// Unload already loaded modules
				_sys_prx_unload_module(ppu, id_list[i], 0, vm::null);
			}

			// Fill with -1
			std::memset(id_list.get_ptr(), -1, count * sizeof(id_list[0]));
			return result;
		}

		id_list[i] = result;
	}

	return CELL_OK;
}

error_code _sys_prx_load_module_list(ppu_thread& ppu, s32 count, vm::cpptr<char, u32, u64> path_list, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_load_module_list(count=%d, path_list=**0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, flags, pOpt, id_list);

	return prx_load_module_list(ppu, count, path_list, SYS_MEMORY_CONTAINER_ID_INVALID, flags, pOpt, id_list);
}
error_code _sys_prx_load_module_list_on_memcontainer(ppu_thread& ppu, s32 count, vm::cpptr<char, u32, u64> path_list, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_load_module_list_on_memcontainer(count=%d, path_list=**0x%x, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x, id_list=*0x%x)", count, path_list, mem_ct, flags, pOpt, id_list);

	return prx_load_module_list(ppu, count, path_list, mem_ct, flags, pOpt, id_list);
}

error_code _sys_prx_load_module_on_memcontainer(ppu_thread& ppu, vm::cptr<char> path, u32 mem_ct, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_load_module_on_memcontainer(path=%s, mem_ct=0x%x, flags=0x%x, pOpt=*0x%x)", path, mem_ct, flags, pOpt);

	return prx_load_module(path.get_ptr(), flags, pOpt);
}

error_code _sys_prx_load_module(ppu_thread& ppu, vm::cptr<char> path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_load_module(path=%s, flags=0x%x, pOpt=*0x%x)", path, flags, pOpt);

	return prx_load_module(path.get_ptr(), flags, pOpt);
}

error_code _sys_prx_start_module(ppu_thread& ppu, u32 id, u64 flags, vm::ptr<sys_prx_start_stop_module_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_start_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	if (id == 0 || !pOpt)
	{
		return CELL_EINVAL;
	}

	const auto prx = idm::get_unlocked<lv2_obj, lv2_prx>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	switch (pOpt->cmd & 0xf)
	{
	case 1:
	{
		std::lock_guard lock(prx->mutex);

		if (!prx->state.compare_and_swap_test(PRX_STATE_INITIALIZED, PRX_STATE_STARTING))
		{
			if (prx->state == PRX_STATE_DESTROYED)
			{
				return CELL_ESRCH;
			}

			return CELL_PRX_ERROR_ERROR;
		}

		prx->load_exports();
		break;
	}
	case 2:
	{
		switch (const u64 res = pOpt->res)
		{
		case SYS_PRX_RESIDENT:
		{
			// No error code on invalid state, so throw on unexpected state
			ensure(prx->state.compare_and_swap_test(PRX_STATE_STARTING, PRX_STATE_STARTED));
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
				prx->unload_exports();
				_sys_prx_unload_module(ppu, id, 0, vm::null);

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

	ppu.check_state();
	pOpt->entry.set(prx->start ? prx->start.addr() : ~0ull);

	// This check is probably for older fw
	if (pOpt->size != 0x20u)
	{
		pOpt->entry2.set(prx->prologue ? prx->prologue.addr() : ~0ull);
	}

	return CELL_OK;
}

error_code _sys_prx_stop_module(ppu_thread& ppu, u32 id, u64 flags, vm::ptr<sys_prx_start_stop_module_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_stop_module(id=0x%x, flags=0x%x, pOpt=*0x%x)", id, flags, pOpt);

	const auto prx = idm::get_unlocked<lv2_obj, lv2_prx>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	if (!pOpt)
	{
		return CELL_EINVAL;
	}

	auto set_entry2 = [&](u64 addr)
	{
		if (pOpt->size != 0x20u)
		{
			pOpt->entry2.set(addr);
		}
	};

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
		case PRX_STATE_DESTROYED: return CELL_ESRCH;
		case PRX_STATE_STARTED: break;
		default:
			fmt::throw_exception("Invalid prx state (%d)", old);
		}

		ppu.check_state();
		pOpt->entry.set(prx->stop ? prx->stop.addr() : ~0ull);
		set_entry2(prx->epilogue ? prx->epilogue.addr() : ~0ull);
		return CELL_OK;
	}
	case 2:
	{
		switch (pOpt->res)
		{
		case 0:
		{
			// No error code on invalid state, so throw on unexpected state
			std::lock_guard lock(prx->mutex);
			ensure(prx->exports_end <= prx->exports_start || (prx->state == PRX_STATE_STOPPING));

			prx->unload_exports();

			ensure(prx->state.compare_and_swap_test(PRX_STATE_STOPPING, PRX_STATE_STOPPED));
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
		case PRX_STATE_DESTROYED: return CELL_ESRCH;
		case PRX_STATE_STARTED: break;
		default:
			fmt::throw_exception("Invalid prx state (%d)", old);
		}

		if (pOpt->cmd == 4u)
		{
			ppu.check_state();
			pOpt->entry.set(prx->stop ? prx->stop.addr() : ~0ull);
			set_entry2(prx->epilogue ? prx->epilogue.addr() : ~0ull);
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
}

error_code _sys_prx_unload_module(ppu_thread& ppu, u32 id, u64 flags, vm::ptr<sys_prx_unload_module_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	// Get the PRX, free the used memory and delete the object and its ID
	const auto prx = idm::withdraw<lv2_obj, lv2_prx>(id, [](lv2_prx& prx) -> CellPrxError
	{
		switch (prx.state.fetch_op([](u32& value)
		{
			if (value == PRX_STATE_INITIALIZED || value == PRX_STATE_STOPPED)
			{
				value = PRX_STATE_DESTROYED;
				return true;
			}

			return false;
		}).first)
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
		return {CELL_PRX_ERROR_UNKNOWN_MODULE, id};
	}

	if (prx.ret)
	{
		return {prx.ret, "%s (id=%s)", prx->name, id};
	}

	sys_prx.success("_sys_prx_unload_module(id=0x%x, flags=0x%x, pOpt=*0x%x): name='%s'", id, flags, pOpt, prx->name);

	prx->mutex.lock_unlock();

	ppu_unload_prx(*prx);

	ppu_finalize(*prx);

	//s32 result = prx->exit ? prx->exit() : CELL_OK;

	return CELL_OK;
}

void lv2_prx::load_exports()
{
	if (exports_end <= exports_start)
	{
		// Nothing to load
		return;
	}

	if (!m_loaded_flags.empty())
	{
		// Already loaded
		return;
	}

	ppu_manual_load_imports_exports(0, 0, exports_start, exports_end - exports_start, m_loaded_flags);
}

void lv2_prx::restore_exports()
{
	constexpr usz sizeof_export_data = 0x1C;

	std::basic_string<char> loaded_flags_empty;

	for (u32 start = exports_start, i = 0; start < exports_end; i++, start += vm::read8(start) ? vm::read8(start) : sizeof_export_data)
	{
		if (::at32(m_external_loaded_flags, i) || (!m_loaded_flags.empty() && ::at32(m_loaded_flags, i)))
		{
			loaded_flags_empty.clear();
			ppu_manual_load_imports_exports(0, 0, start, sizeof_export_data, loaded_flags_empty);
		}
	}
}

void lv2_prx::unload_exports()
{
	if (m_loaded_flags.empty())
	{
		// Not loaded
		return;
	}

	std::basic_string<char> merged = m_loaded_flags;

	for (usz i = 0; i < merged.size(); i++)
	{
		merged[i] |= ::at32(m_external_loaded_flags, i);
	}

	ppu_manual_load_imports_exports(0, 0, exports_start, exports_end - exports_start, merged);
}

error_code _sys_prx_register_module(ppu_thread& ppu, vm::cptr<char> name, vm::ptr<void> opt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("_sys_prx_register_module(name=%s, opt=*0x%x)", name, opt);

	if (!opt)
	{
		return CELL_EINVAL;
	}

	sys_prx_register_module_0x30_type_1_t info{};

	switch (const u64 size_struct = vm::read64(opt.addr()))
	{
	case 0x1c:
	case 0x20:
	{
		const auto _info = vm::static_ptr_cast<sys_prx_register_module_0x20_t>(opt);

		sys_prx.todo("_sys_prx_register_module(): opt size is 0x%x", size_struct);

		// Rebuild info with corresponding members of old structures
		// Weird that type is set to 0 because 0 means NO-OP in this syscall
		info.size = 0x30;
		info.lib_stub_size = _info->stubs_size;
		info.lib_stub_ea = _info->stubs_ea;
		info.error_handler = _info->error_handler;
		info.type = 0;
		break;
	}
	case 0x30:
	{
		std::memcpy(&info, opt.get_ptr(), sizeof(info));
		break;
	}
	default: return CELL_EINVAL;
	}

	sys_prx.warning("opt: size=0x%x, type=0x%x, unk3=0x%x, unk4=0x%x, lib_entries_ea=%s, lib_entries_size=0x%x"
		", lib_stub_ea=%s, lib_stub_size=0x%x, error_handler=%s", info.size, info.type, info.unk3, info.unk4
		, info.lib_entries_ea, info.lib_entries_size, info.lib_stub_ea, info.lib_stub_size, info.error_handler);

	if (info.type & 0x1)
	{
		if (Emu.IsVsh())
		{
			ppu_manual_load_imports_exports(info.lib_stub_ea.addr(), info.lib_stub_size, info.lib_entries_ea.addr(), info.lib_entries_size, *std::make_unique<std::basic_string<char>>());
		}
		else
		{
			// Only VSH is allowed to load it manually
			return not_an_error(CELL_PRX_ERROR_ELF_IS_REGISTERED);
		}
	}

	return CELL_OK;
}

error_code _sys_prx_query_module(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("_sys_prx_query_module()");
	return CELL_OK;
}

error_code _sys_prx_register_library(ppu_thread& ppu, vm::ptr<void> library)
{
	ppu.state += cpu_flag::wait;

	sys_prx.notice("_sys_prx_register_library(library=*0x%x)", library);

	if (!vm::check_addr(library.addr()))
	{
		return CELL_EFAULT;
	}

	constexpr u32 sizeof_lib = 0x1c;

	std::array<char, sizeof_lib> mem_copy{};
	std::memcpy(mem_copy.data(), library.get_ptr(), sizeof_lib);

	std::basic_string<char> flags;
	ppu_manual_load_imports_exports(0, 0, library.addr(), sizeof_lib, flags);

	if (flags.front())
	{
		const bool success = idm::select<lv2_obj, lv2_prx>([&](u32 /*id*/, lv2_prx& prx)
		{
			if (prx.state == PRX_STATE_INITIALIZED)
			{
				for (u32 lib_addr = prx.exports_start, index = 0; lib_addr < prx.exports_end; index++, lib_addr += vm::read8(lib_addr) ? vm::read8(lib_addr) : sizeof_lib)
				{
					if (std::memcpy(vm::base(lib_addr), mem_copy.data(), sizeof_lib) == 0)
					{
						atomic_storage<char>::release(prx.m_external_loaded_flags[index], true);
						return true;
					}
				}
			}

			return false;
		}).ret;

		if (!success)
		{
			sys_prx.error("_sys_prx_register_library(): Failed to associate library to PRX!");
		}
	}

	return CELL_OK;
}

error_code _sys_prx_unregister_library(ppu_thread& ppu, vm::ptr<void> library)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("_sys_prx_unregister_library(library=*0x%x)", library);
	return CELL_OK;
}

error_code _sys_prx_link_library(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("_sys_prx_link_library()");
	return CELL_OK;
}

error_code _sys_prx_unlink_library(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("_sys_prx_unlink_library()");
	return CELL_OK;
}

error_code _sys_prx_query_library(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("_sys_prx_query_library()");
	return CELL_OK;
}

error_code _sys_prx_get_module_list(ppu_thread& ppu, u64 flags, vm::ptr<sys_prx_get_module_list_option_t> pInfo)
{
	ppu.state += cpu_flag::wait;

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

error_code _sys_prx_get_module_info(ppu_thread& ppu, u32 id, u64 flags, vm::ptr<sys_prx_module_info_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_get_module_info(id=0x%x, flags=%d, pOpt=*0x%x)", id, flags, pOpt);

	const auto prx = idm::get_unlocked<lv2_obj, lv2_prx>(id);

	if (!pOpt)
	{
		return CELL_EFAULT;
	}

	if (pOpt->size != pOpt.size())
	{
		return CELL_EINVAL;
	}

	if (!pOpt->info)
	{
		return CELL_EFAULT;
	}

	if (pOpt->info->size != pOpt->info.size() && pOpt->info_v2->size != pOpt->info_v2.size())
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
		std::span dst(pOpt->info->filename.get_ptr(), pOpt->info->filename_size);
		strcpy_trunc(dst, vfs::retrieve(prx->path));
	}

	if (pOpt->info->segments)
	{
		u32 i = 0;
		for (; i < prx->segs.size() && i < pOpt->info->segments_num; i++)
		{
			if (!prx->segs[i].addr) continue; // TODO: Check this
			pOpt->info->segments[i].index = i;
			pOpt->info->segments[i].base = prx->segs[i].addr;
			pOpt->info->segments[i].filesz = prx->segs[i].filesz;
			pOpt->info->segments[i].memsz = prx->segs[i].size;
			pOpt->info->segments[i].type = prx->segs[i].type;
		}
		pOpt->info->segments_num = i;
	}

	if (pOpt->info_v2->size == pOpt->info_v2.size())
	{
		pOpt->info_v2->exports_addr = prx->exports_start;
		pOpt->info_v2->exports_size = prx->exports_end - prx->exports_start;
		pOpt->info_v2->imports_addr = prx->imports_start;
		pOpt->info_v2->imports_size = prx->imports_end - prx->imports_start;
	}

	return CELL_OK;
}

error_code _sys_prx_get_module_id_by_name(ppu_thread& ppu, vm::cptr<char> name, u64 flags, vm::ptr<sys_prx_get_module_id_by_name_option_t> pOpt)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_get_module_id_by_name(name=%s, flags=%d, pOpt=*0x%x)", name, flags, pOpt);

	std::string module_name;
	if (!vm::read_string(name.addr(), 28, module_name))
	{
		return CELL_EINVAL;
	}

	const auto [prx, id] = idm::select<lv2_obj, lv2_prx>([&](u32 id, lv2_prx& prx) -> u32
	{
		if (strncmp(module_name.c_str(), prx.module_info_name, sizeof(prx.module_info_name)) == 0)
		{
			return id;
		}

		return 0;
	});

	if (!id)
	{
		return CELL_PRX_ERROR_UNKNOWN_MODULE;
	}

	return not_an_error(id);
}

error_code _sys_prx_get_module_id_by_address(ppu_thread& ppu, u32 addr)
{
	ppu.state += cpu_flag::wait;

	sys_prx.warning("_sys_prx_get_module_id_by_address(addr=0x%x)", addr);

	if (!vm::check_addr(addr))
	{
		// Fast check for an invalid argument
		return {CELL_PRX_ERROR_UNKNOWN_MODULE, addr};
	}

	const auto [prx, id] = idm::select<lv2_obj, lv2_prx>([&](u32 id, lv2_prx& prx) -> u32
	{
		for (const ppu_segment& seg : prx.segs)
		{
			if (seg.size && addr >= seg.addr && addr < seg.addr + seg.size)
			{
				return id;
			}
		}

		return 0;
	});

	if (!id)
	{
		return {CELL_PRX_ERROR_UNKNOWN_MODULE, addr};
	}

	return not_an_error(id);
}

error_code _sys_prx_start(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

	sys_prx.todo("sys_prx_start()");
	return CELL_OK;
}

error_code _sys_prx_stop(ppu_thread& ppu)
{
	ppu.state += cpu_flag::wait;

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
		STR_CASE(CELL_PRX_ERROR_NO_EXIT_ENTRY);
		}

		return unknown;
	});
}
