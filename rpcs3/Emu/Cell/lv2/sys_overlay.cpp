#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"
#include "Loader/ELF.h"

#include "sys_process.h"
#include "sys_overlay.h"
#include "sys_fs.h"

extern std::shared_ptr<lv2_overlay> ppu_load_overlay(const ppu_exec_object&, const std::string& path);

extern void ppu_initialize(const ppu_module&);

LOG_CHANNEL(sys_overlay);

static error_code overlay_load_module(vm::ptr<u32> ovlmid, std::string path, u64 flags, vm::ptr<u32> entry)
{
	if (!g_ps3_process_info.ppc_seg)
	{
		// Process not permitted
		return CELL_ENOSYS;
	}

	const auto name = path.substr(path.find_last_of('/') + 1);

	const ppu_exec_object obj = decrypt_self(fs::file(vfs::get(path)), fxm::get_always<LoadedNpdrmKeys_t>()->devKlic.data());

	if (obj != elf_error::ok)
	{
		return {CELL_ENOEXEC, obj.operator elf_error()};
	}

	const auto ovlm = ppu_load_overlay(obj, path);

	ppu_initialize(*ovlm);

	const u32 id = idm::last_id();

	if (!ovlmid || !entry)
	{
		if (ovlmid)
		{
			*ovlmid = id;
		}

		sys_overlay_unload_module(id);
		return CELL_EFAULT;
	}

	*ovlmid = id;
	*entry  = ovlm->entry;

	sys_overlay.success("Loaded overlay: %s", path);
	return CELL_OK;
}

error_code sys_overlay_load_module(vm::ptr<u32> ovlmid, vm::cptr<char> path, u64 flags, vm::ptr<u32> entry)
{
	sys_overlay.warning("sys_overlay_load_module(ovlmid=*0x%x, path=%s, flags=0x%x, entry=*0x%x)", ovlmid, path, flags, entry);

	if (!path)
	{
		return CELL_EFAULT;
	}

	return overlay_load_module(ovlmid, path.get_ptr(), flags, entry);
}

error_code sys_overlay_load_module_by_fd(vm::ptr<u32> ovlmid, u32 fd, u64 offset, u64 flags, vm::ptr<u32> entry)
{
	sys_overlay.warning("sys_overlay_load_module_by_fd(ovlmid=*0x%x, fd=%d, flags=0x%x, entry=*0x%x)", ovlmid, fd, flags, entry);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	return overlay_load_module(ovlmid, fmt::format("%s_x%x", file->name.data(), offset), flags, entry);
}

error_code sys_overlay_unload_module(u32 ovlmid)
{
	sys_overlay.warning("sys_overlay_unload_module(ovlmid=0x%x)", ovlmid);

	const auto _main = idm::withdraw<lv2_obj, lv2_overlay>(ovlmid);

	if (!_main)
	{
		return CELL_ESRCH;
	}

	for (auto& seg : _main->segs)
	{
		vm::dealloc(seg.addr);
	}

	return CELL_OK;
}
