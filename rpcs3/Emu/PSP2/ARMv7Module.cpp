#include "stdafx.h"
#include "Loader/ELF.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "ARMv7Thread.h"
#include "ARMv7Opcodes.h"
#include "ARMv7Function.h"
#include "ARMv7Module.h"
#include "Modules/sceLibKernel.h"

extern std::string arm_get_function_name(const std::string& module, u32 fnid);
extern std::string arm_get_variable_name(const std::string& module, u32 vnid);

// Function lookup table. Not supposed to grow after emulation start.
std::vector<arm_function_t> g_arm_function_cache;

std::vector<std::string> g_arm_function_names;

extern std::string arm_get_module_function_name(u32 index)
{
	if (index < g_arm_function_names.size())
	{
		return g_arm_function_names[index];
	}

	return fmt::format(".%u", index);
}

extern void arm_execute_function(ARMv7Thread& cpu, u32 index)
{
	if (index < g_arm_function_cache.size())
	{
		if (const auto func = g_arm_function_cache[index])
		{
			func(cpu);
			LOG_TRACE(ARMv7, "Function '%s' finished, r0=0x%x", arm_get_module_function_name(index), cpu.GPR[0]);
			return;
		}
	}

	fmt::throw_exception("Function not registered (%u)" HERE, index);
}

arm_static_module::arm_static_module(const char* name)
	: name(name)
{
	arm_module_manager::register_module(this);
}

std::unordered_map<std::string, arm_static_module*>& arm_module_manager::access()
{
	static std::unordered_map<std::string, arm_static_module*> map;

	return map;
}

void arm_module_manager::register_module(arm_static_module* module)
{
	access().emplace(module->name, module);
}

arm_static_function& arm_module_manager::access_static_function(const char* module, u32 fnid)
{
	return access().at(module)->functions[fnid];
}

arm_static_variable& arm_module_manager::access_static_variable(const char* module, u32 vnid)
{
	return access().at(module)->variables[vnid];
}

const arm_static_module* arm_module_manager::get_module(const std::string& name)
{
	const auto& map = access();
	const auto found = map.find(name);
	return found != map.end() ? found->second : nullptr;
}

static void arm_initialize_modules()
{
	const std::initializer_list<const arm_static_module*> registered
	{
		&arm_module_manager::SceAppMgr,
		&arm_module_manager::SceAppUtil,
		&arm_module_manager::SceAudio,
		&arm_module_manager::SceAudiodec,
		&arm_module_manager::SceAudioenc,
		&arm_module_manager::SceAudioIn,
		&arm_module_manager::SceCamera,
		&arm_module_manager::SceCodecEngine,
		&arm_module_manager::SceCommonDialog,
		&arm_module_manager::SceCpu,
		&arm_module_manager::SceCtrl,
		&arm_module_manager::SceDbg,
		&arm_module_manager::SceDebugLed,
		&arm_module_manager::SceDeci4p,
		&arm_module_manager::SceDeflt,
		&arm_module_manager::SceDipsw,
		&arm_module_manager::SceDisplay,
		&arm_module_manager::SceDisplayUser,
		&arm_module_manager::SceFiber,
		&arm_module_manager::SceFios,
		&arm_module_manager::SceFpu,
		&arm_module_manager::SceGxm,
		&arm_module_manager::SceHttp,
		&arm_module_manager::SceIme,
		&arm_module_manager::SceIofilemgr,
		&arm_module_manager::SceJpeg,
		&arm_module_manager::SceJpegEnc,
		&arm_module_manager::SceLibc,
		&arm_module_manager::SceLibKernel,
		&arm_module_manager::SceLibm,
		&arm_module_manager::SceLibstdcxx,
		&arm_module_manager::SceLibXml,
		&arm_module_manager::SceLiveArea,
		&arm_module_manager::SceLocation,
		&arm_module_manager::SceMd5,
		&arm_module_manager::SceModulemgr,
		&arm_module_manager::SceMotion,
		&arm_module_manager::SceMt19937,
		&arm_module_manager::SceNet,
		&arm_module_manager::SceNetCtl,
		&arm_module_manager::SceNgs,
		&arm_module_manager::SceNpBasic,
		&arm_module_manager::SceNpCommon,
		&arm_module_manager::SceNpManager,
		&arm_module_manager::SceNpMatching,
		&arm_module_manager::SceNpScore,
		&arm_module_manager::SceNpUtility,
		&arm_module_manager::ScePerf,
		&arm_module_manager::ScePgf,
		&arm_module_manager::ScePhotoExport,
		&arm_module_manager::SceProcessmgr,
		&arm_module_manager::SceRazorCapture,
		&arm_module_manager::SceRtc,
		&arm_module_manager::SceSas,
		&arm_module_manager::SceScreenShot,
		&arm_module_manager::SceSfmt,
		&arm_module_manager::SceSha,
		&arm_module_manager::SceSqlite,
		&arm_module_manager::SceSsl,
		&arm_module_manager::SceStdio,
		&arm_module_manager::SceSulpha,
		&arm_module_manager::SceSysmem,
		&arm_module_manager::SceSysmodule,
		&arm_module_manager::SceSystemGesture,
		&arm_module_manager::SceThreadmgr,
		&arm_module_manager::SceTouch,
		&arm_module_manager::SceUlt,
		&arm_module_manager::SceVideodec,
		&arm_module_manager::SceVoice,
		&arm_module_manager::SceVoiceQoS,
	};

	// Reinitialize function cache
	g_arm_function_cache = arm_function_manager::get();
	g_arm_function_names.clear();
	g_arm_function_names.resize(g_arm_function_cache.size());

	// "Use" all the modules for correct linkage
	for (auto& module : registered)
	{
		LOG_TRACE(LOADER, "Registered static module: %s", module->name);
	}

	for (auto& pair : arm_module_manager::get())
	{
		const auto module = pair.second;

		for (auto& function : module->functions)
		{
			LOG_TRACE(LOADER, "** 0x%08X: %s", function.first, function.second.name);
			g_arm_function_names.at(function.second.index) = fmt::format("%s.%s", module->name, function.second.name);
		}

		for (auto& variable : module->variables)
		{
			LOG_TRACE(LOADER, "** &0x%08X: %s (size=0x%x, align=0x%x)", variable.first, variable.second.name, variable.second.size, variable.second.align);
			variable.second.var->set(0);
		}
	}
}

struct psv_moduleinfo_t
{
	le_t<u16> attr; // ???
	u8 major; // ???
	u8 minor; // ???
	char name[24]; // ???
	le_t<u32> unk0;
	le_t<u32> unk1;
	le_t<u32> libent_top;
	le_t<u32> libent_end;
	le_t<u32> libstub_top;
	le_t<u32> libstub_end;
	le_t<u32> data[1]; // ...
};

struct psv_libent_t
{
	le_t<u16> size; // ???
	le_t<u16> version;
	le_t<u16> flags;
	le_t<u16> fcount;
	le_t<u32> vcount;
	le_t<u32> unk2;
	le_t<u32> module_nid;
	le_t<u32> module_name;	/* Pointer to name of this module */
	le_t<u32> nid_table;	/* Pointer to array of 32-bit NIDs to export */
	le_t<u32> entry_table;	/* Pointer to array of data pointers for each NID */
};

struct psv_libstub_t
{
	le_t<u16> size;		// 0x2C, 0x34
	le_t<u16> version;	// (usually 1, 5 for sceLibKernel)
	le_t<u16> flags;	// (usually 0)
	le_t<u16> fcount;
	le_t<u16> vcount;
	le_t<u16> unk2;
	le_t<u32> unk3;
	le_t<u32> module_nid;	/* NID of module to import */
	le_t<u32> module_name;	/* Pointer to name of imported module, for debugging */
	le_t<u32> reserved2;
	le_t<u32> func_nid_table;	/* Pointer to array of function NIDs to import */
	le_t<u32> func_entry_table;	/* Pointer to array of stub functions to fill */
	le_t<u32> var_nid_table;	/* Pointer to array of variable NIDs to import */
	le_t<u32> var_entry_table;	/* Pointer to array of data pointers to write to */
	le_t<u32> unk_nid_table;
	le_t<u32> unk_entry_table;
};

struct psv_libcparam_t
{
	le_t<u32> size;
	le_t<u32> unk0;

	vm::lcptr<u32> sceLibcHeapSize;
	vm::lcptr<u32> sceLibcHeapSizeDefault;
	vm::lcptr<u32> sceLibcHeapExtendedAlloc;
	vm::lcptr<u32> sceLibcHeapDelayedAlloc;

	le_t<u32> unk1;
	le_t<u32> unk2;

	vm::lptr<u32> __sce_libcmallocreplace;
	vm::lptr<u32> __sce_libcnewreplace;
};

struct psv_process_param_t
{
	le_t<u32> size; // 0x00000030
	nse_t<u32> ver; // 'PSP2'
	le_t<u32> unk0; // 0x00000005
	le_t<u32> unk1;

	vm::lcptr<char> sceUserMainThreadName;
	vm::lcptr<s32>  sceUserMainThreadPriority;
	vm::lcptr<u32>  sceUserMainThreadStackSize;
	vm::lcptr<u32>  sceUserMainThreadAttribute;
	vm::lcptr<char> sceProcessName;
	vm::lcptr<u32>  sce_process_preload_disabled;
	vm::lcptr<u32>  sceUserMainThreadCpuAffinityMask;

	vm::lcptr<psv_libcparam_t> sce_libcparam;
};

struct psv_reloc
{
	u32 addr;
	u32 type;
	u64 data;
};

/** \name Macros to get SCE reloc values
*  @{
*/
#define SCE_RELOC_SHORT_OFFSET(x) (((x).r_opt1 >> 20) | ((x).r_opt2 & 0x3FF) << 12)
#define SCE_RELOC_SHORT_ADDEND(x) ((x).r_opt2 >> 10)
#define SCE_RELOC_LONG_OFFSET(x) ((x).r_offset)
#define SCE_RELOC_LONG_ADDEND(x) ((x).r_addend)
#define SCE_RELOC_LONG_CODE2(x) (((x).r_type >> 20) & 0xFF)
#define SCE_RELOC_LONG_DIST2(x) (((x).r_type >> 28) & 0xF)
#define SCE_RELOC_IS_SHORT(x) (((x).r_type) & 0xF)
#define SCE_RELOC_CODE(x) (((x).r_type >> 8) & 0xFF)
#define SCE_RELOC_SYMSEG(x) (((x).r_type >> 4) & 0xF)
#define SCE_RELOC_DATSEG(x) (((x).r_type >> 16) & 0xF)
/** @}*/

/** \name Vita supported relocations
*  @{
*/
#define R_ARM_NONE              0
#define R_ARM_ABS32             2
#define R_ARM_REL32             3
#define R_ARM_THM_CALL          10
#define R_ARM_CALL              28
#define R_ARM_JUMP24            29
#define R_ARM_TARGET1           38
#define R_ARM_V4BX              40
#define R_ARM_TARGET2           41
#define R_ARM_PREL31            42
#define R_ARM_MOVW_ABS_NC       43
#define R_ARM_MOVT_ABS          44
#define R_ARM_THM_MOVW_ABS_NC   47
#define R_ARM_THM_MOVT_ABS      48
/** @}*/


static void arm_patch_refs(u32 refs, u32 addr)
{
	auto ptr = vm::cptr<u32>::make(refs);
	LOG_NOTICE(LOADER, "**** Processing refs at 0x%x:", ptr);

	if (ptr[0] != 0xff || ptr[1] != addr)
	{
		LOG_ERROR(LOADER, "**** Unexpected ref format ([0]=0x%x, [1]=0x%x)", ptr[0], ptr[1]);
	}
	else for (ptr += 2; *ptr; ptr++)
	{
		switch (u32 code = *ptr)
		{
		case 0x0000002f: // movw r*,# instruction
		{
			const u32 raddr = *++ptr;
			vm::write16(raddr + 0, vm::read16(raddr + 0) | (addr & 0x800) >> 1 | (addr & 0xf000) >> 12);
			vm::write16(raddr + 2, vm::read16(raddr + 2) | (addr & 0x700) << 4 | (addr & 0xff));

			LOG_NOTICE(LOADER, "**** MOVW written at *0x%x", raddr);
			break;
		}
		case 0x00000030: // movt r*,# instruction
		{
			const u32 raddr = *++ptr;
			vm::write16(raddr + 0, vm::read16(raddr + 0) | (addr & 0x8000000) >> 17 | (addr & 0xf0000000) >> 28);
			vm::write16(raddr + 2, vm::read16(raddr + 2) | (addr & 0x7000000) >> 12 | (addr & 0xff0000) >> 16);

			LOG_NOTICE(LOADER, "**** MOVT written at *0x%x", raddr);
			break;
		}
		default:
		{
			LOG_ERROR(LOADER, "**** Unknown ref code found (0x%08x)", code);
		}
		}
	}
	
}

void arm_load_exec(const arm_exec_object& elf)
{
	arm_initialize_modules();

	vm::cptr<psv_moduleinfo_t> module_info{};
	vm::cptr<psv_libent_t> libent{};
	vm::cptr<psv_libstub_t> libstub{};
	vm::cptr<psv_process_param_t> proc_param{};

	u32 entry_point{};
	u32 start_addr{};
	u32 arm_exidx{};
	u32 arm_extab{};
	u32 tls_faddr{};
	u32 tls_fsize{};
	u32 tls_vsize{};

	std::unique_ptr<u32[]> p_vaddr = std::make_unique<u32[]>(elf.progs.size());

	for (int i = 0; i < elf.progs.size(); i++)
	{
		const auto& prog = elf.progs[i];
		const u32 addr = vm::cast(prog.p_vaddr, HERE);
		const u32 size = ::narrow<u32>(prog.p_memsz, "p_memsz" HERE);
		const u32 type = prog.p_type;
		const u32 flag = prog.p_flags;

		if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			if (!(p_vaddr[i] = vm::alloc(size, vm::main)))
			{
				fmt::throw_exception("vm::alloc() failed (size=0x%x)", size);
			}

			if (elf.header.e_type == elf_type::psv1 && prog.p_paddr)
			{
				module_info.set(start_addr + (prog.p_paddr - prog.p_offset));
				LOG_NOTICE(LOADER, "Found program with p_paddr=0x%x", prog.p_paddr);
			}

			if (!start_addr)
			{
				start_addr = p_vaddr[i];
			}

			std::memcpy(vm::base(p_vaddr[i]), prog.bin.data(), prog.p_filesz);
		}
		else if (prog.p_type == 0x60000000) 
		{
			// Relocation code taken from
			// https://github.com/yifanlu/UVLoader/blob/master/relocate.c
			// Relocation information of the SCE_PPURELA segment
			typedef union sce_reloc
			{
				u32       r_type;
				struct
				{
					u32   r_opt1;
					u32   r_opt2;
				} r_short;
				struct
				{
					u32   r_type;
					u32   r_addend;
					u32   r_offset;
				} r_long;
			} sce_reloc_t;

			for (uint i = 0; i < prog.p_filesz; )
			{
				const auto& rel = reinterpret_cast<const sce_reloc_t&>(prog.bin[i]);
				u32 r_offset;
				u32 r_addend;
				u8 r_symseg;
				u8 r_datseg;
				s32 offset;
				u32 symval, addend, loc;
				u32 upper, lower, sign, j1, j2;
				u32 value;

				if (SCE_RELOC_IS_SHORT(rel))
				{
					r_offset = SCE_RELOC_SHORT_OFFSET(rel.r_short);
					r_addend = SCE_RELOC_SHORT_ADDEND(rel.r_short);
					LOG_NOTICE(LOADER, "SHORT RELOC %X %X %X", r_offset, r_addend, SCE_RELOC_CODE(rel));
					i += 8;
				}
				else
				{
					r_offset = SCE_RELOC_LONG_OFFSET(rel.r_long);
					r_addend = SCE_RELOC_LONG_ADDEND(rel.r_long);
					if (SCE_RELOC_LONG_CODE2(rel.r_long))
					{
						LOG_NOTICE(LOADER, "Code2 ignored for relocation at %X.", i);
					}
					LOG_NOTICE(LOADER, "LONG RELOC %X %X %X", r_offset, r_addend, SCE_RELOC_CODE(rel));
					i += 12;
				}

				// get values
				r_symseg = SCE_RELOC_SYMSEG(rel);
				r_datseg = SCE_RELOC_DATSEG(rel);
				symval = r_symseg == 15 ? 0 : p_vaddr[r_symseg];
				loc = p_vaddr[r_datseg] + r_offset;

				// perform relocation
				// taken from linux/arch/arm/kernel/module.c of Linux Kernel 4.0
				switch (SCE_RELOC_CODE(rel))
				{
				case R_ARM_V4BX:
				{
					/* Preserve Rm and the condition code. Alter
					* other bits to re-code instruction as
					* MOV PC,Rm.
					*/
					value = vm::_ref<u32>(loc & 0xf000000f) | 0x01a0f000;
				}
				break;
				case R_ARM_ABS32:
				case R_ARM_TARGET1:
				{
					value = r_addend + symval;
				}
				break;
				case R_ARM_REL32:
				case R_ARM_TARGET2:
				{
					value = r_addend + symval - loc;
				}
				break;
				case R_ARM_THM_CALL:
				{
					upper = vm::_ref<u16>(loc);
					lower = vm::_ref<u16>(loc + 2);

					/*
					* 25 bit signed address range (Thumb-2 BL and B.W
					* instructions):
					*   S:I1:I2:imm10:imm11:0
					* where:
					*   S     = upper[10]   = offset[24]
					*   I1    = ~(J1 ^ S)   = offset[23]
					*   I2    = ~(J2 ^ S)   = offset[22]
					*   imm10 = upper[9:0]  = offset[21:12]
					*   imm11 = lower[10:0] = offset[11:1]
					*   J1    = lower[13]
					*   J2    = lower[11]
					*/
					sign = (upper >> 10) & 1;
					j1 = (lower >> 13) & 1;
					j2 = (lower >> 11) & 1;
					offset = r_addend + symval - loc;

					if (offset <= (s32)0xff000000 ||
						offset >= (s32)0x01000000) 
					{
						LOG_NOTICE(LOADER, "reloc %x out of range: 0x%08X", i, symval);
						break;
					}

					sign = (offset >> 24) & 1;
					j1 = sign ^ (~(offset >> 23) & 1);
					j2 = sign ^ (~(offset >> 22) & 1);
					upper = (u16)((upper & 0xf800) | (sign << 10) |
						((offset >> 12) & 0x03ff));
					lower = (u16)((lower & 0xd000) |
						(j1 << 13) | (j2 << 11) |
						((offset >> 1) & 0x07ff));

					value = ((u32)lower << 16) | upper;
				}
				break;
				case R_ARM_CALL:
				case R_ARM_JUMP24:
				{
					offset = r_addend + symval - loc;
					if (offset <= (s32)0xfe000000 ||
						offset >= (s32)0x02000000) 
					{
						LOG_NOTICE(LOADER, "reloc %x out of range: 0x%08X", i, symval);
						break;
					}

					offset >>= 2;
					offset &= 0x00ffffff;

					value = vm::_ref<u32>(loc & 0xff000000) | offset;
				}
				break;
				case R_ARM_PREL31:
				{
					offset = r_addend + symval - loc;
					value = offset & 0x7fffffff;
				}
				break;
				case R_ARM_MOVW_ABS_NC:
				case R_ARM_MOVT_ABS:
				{
					offset = symval + r_addend;
					if (SCE_RELOC_CODE(rel) == R_ARM_MOVT_ABS)
						offset >>= 16;

					value = vm::_ref<u32>(loc);
					value &= 0xfff0f000;
					value |= ((offset & 0xf000) << 4) |
						(offset & 0x0fff);
				}
				break;
				case R_ARM_THM_MOVW_ABS_NC:
				case R_ARM_THM_MOVT_ABS:
				{
					upper = vm::_ref<u16>(loc);
					lower = vm::_ref<u16>(loc + 2);

					/*
					* MOVT/MOVW instructions encoding in Thumb-2:
					*
					* i    = upper[10]
					* imm4 = upper[3:0]
					* imm3 = lower[14:12]
					* imm8 = lower[7:0]
					*
					* imm16 = imm4:i:imm3:imm8
					*/
					offset = r_addend + symval;

					if (SCE_RELOC_CODE(rel) == R_ARM_THM_MOVT_ABS)
						offset >>= 16;

					upper = (u16)((upper & 0xfbf0) |
						((offset & 0xf000) >> 12) |
						((offset & 0x0800) >> 1));
					lower = (u16)((lower & 0x8f00) |
						((offset & 0x0700) << 4) |
						(offset & 0x00ff));

					value = ((u32)lower << 16) | upper;
				}
				break;
				case R_ARM_NONE:
					continue;
				
				default:
				{
					LOG_NOTICE(LOADER, "Unknown relocation code %u at %x", SCE_RELOC_CODE(rel), i);
					continue;
				}
			}

				LOG_NOTICE(LOADER, "Writing at  %X[%d], %X, %X, %d", p_vaddr[r_datseg], r_datseg, r_offset, value, sizeof(value));
				if ((r_offset + sizeof(value)) > elf.progs[r_datseg].p_filesz)
				{
					LOG_NOTICE(LOADER, "Relocation overflows segment");
					continue;
				}
				vm::_ref<u32>(p_vaddr[r_datseg]+r_offset) = value;
			}
		}
	}

	if (!module_info) module_info.set(start_addr + elf.header.e_entry);
	if (!libent) libent.set(start_addr + module_info->libent_top);
	if (!libstub) libstub.set(start_addr + module_info->libstub_top);

	LOG_NOTICE(LOADER, "__sce_moduleinfo(*0x%x) analysis...", module_info);

	if (module_info->data[2] == 0xffffffff)
	{
		arm_exidx = module_info->data[3];
		arm_extab = module_info->data[4];
		tls_faddr = module_info->data[5];
		tls_fsize = module_info->data[6];
		tls_vsize = module_info->data[7];
	}
	else if (module_info->data[5] == 0xffffffff || module_info->data[5] == 0)
	{
		tls_faddr = module_info->data[1]; // Guess
		tls_fsize = module_info->data[2];
		tls_vsize = module_info->data[3];
		arm_exidx = module_info->data[6];
		arm_extab = module_info->data[8];
	}
	else
	{
		LOG_ERROR(LOADER, "Failed to recognize __sce_moduleinfo format");
	}

	LOG_NOTICE(LOADER, "** arm_exidx=0x%x", arm_exidx);
	LOG_NOTICE(LOADER, "** arm_extab=0x%x", arm_extab);
	LOG_NOTICE(LOADER, "** tls_faddr=0x%x", tls_faddr);
	LOG_NOTICE(LOADER, "** tls_fsize=0x%x", tls_fsize);
	LOG_NOTICE(LOADER, "** tls_vsize=0x%x", tls_vsize);

	// Process exports
	while (libent.addr() < start_addr + module_info->libent_end)
	{
		const u32 size = libent->size;

		// TODO: check addresses
		if (size != 0x1c && size != 0x20)
		{
			LOG_ERROR(LOADER, "Unknown libent size (0x%x) at *0x%x", libent->size, libent);
		}
		else
		{
			LOG_NOTICE(LOADER, "Loading libent at *0x%x", libent);
			LOG_NOTICE(LOADER, "** Version: 0x%x, Flags: 0x%x", libent->version, libent->flags);
			LOG_NOTICE(LOADER, "** Functions: %u", libent->fcount);
			LOG_NOTICE(LOADER, "** Variables: %u", libent->vcount);
			LOG_NOTICE(LOADER, "** 0x%x, Module NID: 0x%08x", libent->unk2, libent->module_nid);

			const auto export_nids = vm::cptr<u32>::make(libent->nid_table);
			const auto export_data = vm::cptr<u32>::make(libent->entry_table);

			LOG_NOTICE(LOADER, "** Export Data: 0x%x, Export NIDs: 0x%08x", export_data, export_nids);
			
			for (u32 i = 0, count = export_data - export_nids; i < count; i++)
			{
				const u32 nid = export_nids[i];
				const u32 addr = export_data[i];

				// Known exports
				switch (nid)
				{
					case 0x935cd196: // set entry point
					{
						entry_point = addr;
						break;
					}

					case 0x6c2224ba: // __sce_moduleinfo
					{
						verify(HERE), addr == module_info.addr();
						break;
					}

					case 0x70fba1e7: // __sce_process_param
					{
						proc_param.set(addr);
						break;
					}

					default:
					{
						LOG_ERROR(LOADER, "** Unknown export '0x%08X' (*0x%x)", nid, addr);
					}
				}
			}
		}

		// Next entry
		libent.set(libent.addr() + libent->size);
	}

	// Process imports
	while (libstub.addr() < start_addr + module_info->libstub_end)
	{
		const u32 size = libstub->size;

		// TODO: check addresses
		if (size != 0x2c && size != 0x34)
		{
			LOG_ERROR(LOADER, "Unknown libstub size (0x%x) at *0x%x)", libstub->size, libstub);
		}
		else
		{
			const std::string module_name(vm::_ptr<char>(libstub->module_name));

			LOG_NOTICE(LOADER, "Loading libstub at 0x%x: %s", libstub, module_name);

			const auto _sm = arm_module_manager::get_module(module_name);

			if (!_sm)
			{
				LOG_ERROR(LOADER, "** Unknown module '%s'", module_name);
			}
			else
			{
				// Allocate HLE variables (TODO)
				for (auto& var : _sm->variables)
				{
					var.second.var->set(vm::alloc(var.second.size, vm::main, std::max<u32>(var.second.align, 4096)));
					LOG_WARNING(LOADER, "** Allocated variable '%s' in module '%s' at *0x%x", var.second.name, module_name, var.second.var->addr());
				}

				// Initialize HLE variables (TODO)
				for (auto& var : _sm->variables)
				{
					var.second.init();
				}
			}

			LOG_NOTICE(LOADER, "** Version: 0x%x, Flags: 0x%x", libstub->version, libstub->flags);
			LOG_NOTICE(LOADER, "** Functions: %u", libstub->fcount);
			LOG_NOTICE(LOADER, "** Variables: %u", libstub->vcount);
			LOG_NOTICE(LOADER, "** 0x%x, 0x%08x", libstub->unk2, libstub->unk3);

			const auto fnids = vm::cptr<u32>::make(libstub->func_nid_table);
			const auto fstubs = vm::cptr<u32>::make(libstub->func_entry_table);

			for (u32 j = 0; j < libstub->fcount; j++)
			{
				const u32 fnid = fnids[j];
				const u32 faddr = fstubs[j];

				u32 index = 0;

				const auto fstub = vm::ptr<u32>::make(faddr);
				const auto fname = arm_get_function_name(module_name, fnid);

				if (_sm && _sm->functions.count(fnid))
				{
					index = _sm->functions.at(fnid).index;
					LOG_NOTICE(LOADER, "** Imported function '%s' in module '%s' (*0x%x)", fname, module_name, faddr);
				}
				else
				{
					// TODO
					index = ::size32(g_arm_function_cache);
					g_arm_function_cache.emplace_back();
					g_arm_function_names.emplace_back(fmt::format("%s.%s", module_name, fname));

					LOG_ERROR(LOADER, "** Unknown function '%s' in module '%s' (*0x%x) -> index %u", fname, module_name, faddr, index);
				}

				// Check import stub
				if (fstub[2] != 0xE1A00000)
				{
					LOG_ERROR(LOADER, "**** Unexpected import function stub (*0x%x, [2]=0x%08x)", faddr, fstub[2]);
				}

				// Process refs
				if (const u32 refs = fstub[3])
				{
					arm_patch_refs(refs, faddr);
				}

				// Install HACK instruction (ARM)
				fstub[0] = 0xe0700090 | arm_code::hack<arm_encoding::A1>::index::insert(index);
			}

			const auto vnids = vm::cptr<u32>::make(libstub->var_nid_table);
			const auto vstub = vm::cptr<u32>::make(libstub->var_entry_table);

			for (u32 j = 0; j < libstub->vcount; j++)
			{
				const u32 vnid = vnids[j];
				const u32 refs = vstub[j];

				// Static variable
				if (const auto _sv = _sm && _sm->variables.count(vnid) ? &_sm->variables.at(vnid) : nullptr)
				{
					LOG_NOTICE(LOADER, "** Imported variable '%s' in module '%s' (refs=*0x%x)", arm_get_variable_name(module_name, vnid), module_name, refs);
					arm_patch_refs(refs, _sv->var->addr());
				}
				else
				{
					LOG_FATAL(LOADER, "** Unknown variable '%s' in module '%s' (refs=*0x%x)", arm_get_variable_name(module_name, vnid), module_name, refs);
				}
			}
		}

		// Next lib
		libstub.set(libstub.addr() + size);
	}

	LOG_NOTICE(LOADER, "__sce_process_param(*0x%x) analysis...", proc_param);

	verify(HERE), proc_param->size >= sizeof(psv_process_param_t);
	verify(HERE), proc_param->ver == "PSP2"_u32;

	LOG_NOTICE(LOADER, "*** size=0x%x; 0x%x, 0x%x, 0x%x", proc_param->size, proc_param->ver, proc_param->unk0, proc_param->unk1);

	LOG_NOTICE(LOADER, "*** &sceUserMainThreadName            = 0x%x", proc_param->sceUserMainThreadName);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadPriority        = 0x%x", proc_param->sceUserMainThreadPriority);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadStackSize       = 0x%x", proc_param->sceUserMainThreadStackSize);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadAttribute       = 0x%x", proc_param->sceUserMainThreadAttribute);
	LOG_NOTICE(LOADER, "*** &sceProcessName                   = 0x%x", proc_param->sceProcessName);
	LOG_NOTICE(LOADER, "*** &sce_process_preload_disabled     = 0x%x", proc_param->sce_process_preload_disabled);
	LOG_NOTICE(LOADER, "*** &sceUserMainThreadCpuAffinityMask = 0x%x", proc_param->sceUserMainThreadCpuAffinityMask);

	const auto libc_param = proc_param->sce_libcparam;

	if (libc_param) 
	{

		LOG_NOTICE(LOADER, "__sce_libcparam(*0x%x) analysis...", libc_param);

		verify(HERE), libc_param->size >= 0x1c;

		LOG_NOTICE(LOADER, "*** size=0x%x; 0x%x, 0x%x, 0x%x", libc_param->size, libc_param->unk0, libc_param->unk1, libc_param->unk2);

		LOG_NOTICE(LOADER, "*** &sceLibcHeapSize                  = 0x%x", libc_param->sceLibcHeapSize);
		LOG_NOTICE(LOADER, "*** &sceLibcHeapSizeDefault           = 0x%x", libc_param->sceLibcHeapSizeDefault);
		LOG_NOTICE(LOADER, "*** &sceLibcHeapExtendedAlloc         = 0x%x", libc_param->sceLibcHeapExtendedAlloc);
		LOG_NOTICE(LOADER, "*** &sceLibcHeapDelayedAlloc          = 0x%x", libc_param->sceLibcHeapDelayedAlloc);

	}

	const auto stop_code = vm::ptr<u32>::make(vm::alloc(3 * 4, vm::main));
	stop_code[0] = 0xf870; // HACK instruction (Thumb)
	stop_code[1] = 1; // Predefined function index (HLE return)
	arm_function_manager::addr = stop_code.addr();

	const std::string& thread_name = proc_param->sceUserMainThreadName ? proc_param->sceUserMainThreadName.get_ptr() : "main_thread";
	const u32 stack_size = proc_param->sceUserMainThreadStackSize ? proc_param->sceUserMainThreadStackSize->value() : 256 * 1024;
	const u32 priority = proc_param->sceUserMainThreadPriority ? proc_param->sceUserMainThreadPriority->value() : 160;

	auto thread = idm::make_ptr<ARMv7Thread>(thread_name, priority, stack_size);

	thread->write_pc(entry_point, 0);
	thread->TLS = fxm::make_always<arm_tls_manager>(tls_faddr + start_addr, tls_fsize, tls_vsize)->alloc();

	// Initialize args
	std::vector<char> argv_data;

	for (const auto& arg : Emu.argv)
	{
		argv_data.insert(argv_data.end(), arg.begin(), arg.end());
		argv_data.insert(argv_data.end(), '\0');

		thread->GPR[0]++; // argc
	}

	const u32 argv = vm::alloc(::size32(argv_data), vm::main);
	std::memcpy(vm::base(argv), argv_data.data(), argv_data.size()); // copy arg list
	thread->GPR[1] = argv;
}
