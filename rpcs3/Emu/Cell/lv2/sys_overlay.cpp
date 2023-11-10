#include "stdafx.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/system_config.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"
#include "Loader/ELF.h"

#include "sys_process.h"
#include "sys_overlay.h"
#include "sys_fs.h"

extern std::pair<std::shared_ptr<lv2_overlay>, CellError> ppu_load_overlay(const ppu_exec_object&, bool virtual_load, const std::string& path, s64 file_offset, utils::serial* ar = nullptr);

extern bool ppu_initialize(const ppu_module&, bool check_only = false, u64 file_size = 0);
extern void ppu_finalize(const ppu_module&);

LOG_CHANNEL(sys_overlay);

static error_code overlay_load_module(vm::ptr<u32> ovlmid, const std::string& vpath, u64 /*flags*/, vm::ptr<u32> entry, fs::file src = {}, s64 file_offset = 0)
{
	if (!src)
	{
		auto [fs_error, ppath, path, lv2_file, type] = lv2_file::open(vpath, 0, 0);

		if (fs_error)
		{
			return {fs_error, vpath};
		}

		src = std::move(lv2_file);
	}

	u128 klic = g_fxo->get<loaded_npdrm_keys>().last_key();

	ppu_exec_object obj = decrypt_self(std::move(src), reinterpret_cast<u8*>(&klic), nullptr, true);

	if (obj != elf_error::ok)
	{
		return {CELL_ENOEXEC, obj.operator elf_error()};
	}

	const auto [ovlm, error] = ppu_load_overlay(obj, false, vfs::get(vpath), file_offset);

	obj.clear();

	if (error)
	{
		if (error == CELL_CANCEL + 0u)
		{
			// Emulation stopped
			return {};
		}

		return error;
	}

	ppu_initialize(*ovlm);

	sys_overlay.success(u8"Loaded overlay: “%s” (id=0x%x)", vpath, idm::last_id());

	*ovlmid = idm::last_id();
	*entry  = ovlm->entry;

	return CELL_OK;
}

fs::file make_file_view(fs::file&& file, u64 offset, u64 size);

std::shared_ptr<void> lv2_overlay::load(utils::serial& ar)
{
	const std::string path = vfs::get(ar.operator std::string());
	const s64 offset = ar;

	std::shared_ptr<lv2_overlay> ovlm;

	fs::file file{path.substr(0, path.size() - (offset ? fmt::format("_x%x", offset).size() : 0))};

	if (file)
	{
		u128 klic = g_fxo->get<loaded_npdrm_keys>().last_key();
		file = make_file_view(std::move(file), offset, umax);
		ovlm = ppu_load_overlay(ppu_exec_object{ decrypt_self(std::move(file), reinterpret_cast<u8*>(&klic)) }, false, path, 0, &ar).first;
		ensure(ovlm);
	}
	else
	{
		ensure(g_cfg.savestate.state_inspection_mode.get());
	}

	return ovlm;
}

void lv2_overlay::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_prx_overlay);
	ar(vfs::retrieve(path), offset);
}

error_code sys_overlay_load_module(vm::ptr<u32> ovlmid, vm::cptr<char> path, u64 flags, vm::ptr<u32> entry)
{
	sys_overlay.warning("sys_overlay_load_module(ovlmid=*0x%x, path=%s, flags=0x%x, entry=*0x%x)", ovlmid, path, flags, entry);

	if (!g_ps3_process_info.ppc_seg)
	{
		// Process not permitted
		return CELL_ENOSYS;
	}

	if (!path)
	{
		return CELL_EFAULT;
	}

	return overlay_load_module(ovlmid, path.get_ptr(), flags, entry);
}

error_code sys_overlay_load_module_by_fd(vm::ptr<u32> ovlmid, u32 fd, u64 offset, u64 flags, vm::ptr<u32> entry)
{
	sys_overlay.warning("sys_overlay_load_module_by_fd(ovlmid=*0x%x, fd=%d, offset=0x%llx, flags=0x%x, entry=*0x%x)", ovlmid, fd, offset, flags, entry);

	if (!g_ps3_process_info.ppc_seg)
	{
		// Process not permitted
		return CELL_ENOSYS;
	}

	if (static_cast<s64>(offset) < 0)
	{
		return CELL_EINVAL;
	}

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::lock_guard lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	return overlay_load_module(ovlmid, offset ? fmt::format("%s_x%x", file->name.data(), offset) : file->name.data(), flags, entry, lv2_file::make_view(file, offset), offset);
}

error_code sys_overlay_unload_module(u32 ovlmid)
{
	sys_overlay.warning("sys_overlay_unload_module(ovlmid=0x%x)", ovlmid);

	if (!g_ps3_process_info.ppc_seg)
	{
		// Process not permitted
		return CELL_ENOSYS;
	}

	const auto _main = idm::withdraw<lv2_obj, lv2_overlay>(ovlmid);

	if (!_main)
	{
		return CELL_ESRCH;
	}

	for (auto& seg : _main->segs)
	{
		vm::dealloc(seg.addr);
	}

	ppu_finalize(*_main);

	return CELL_OK;
}
