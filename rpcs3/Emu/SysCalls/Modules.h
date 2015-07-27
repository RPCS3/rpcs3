#pragma once

#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "ErrorCodes.h"
#include "LogBase.h"

namespace vm { using namespace ps3; }

class Module;

// flags set in ModuleFunc
enum : u32
{
	MFF_FORCED_HLE = (1 << 0), // always call HLE function
	MFF_NO_RETURN  = (1 << 1), // uses EIF_USE_BRANCH flag with LLE, ignored with MFF_FORCED_HLE
};

// flags passed with index
enum : u32
{
	EIF_SAVE_RTOC   = (1 << 25), // save RTOC in [SP+0x28] before calling HLE/LLE function
	EIF_PERFORM_BLR = (1 << 24), // do BLR after calling HLE/LLE function
	EIF_USE_BRANCH  = (1 << 23), // do only branch, LLE must be set, last_syscall must be zero

	EIF_FLAGS = 0x3800000, // all flags
};

struct ModuleFunc
{
	u32 id;
	u32 flags;
	Module* module;
	const char* name;
	ppu_func_caller func;
	vm::ptr<void()> lle_func;

	ModuleFunc()
	{
	}

	ModuleFunc(u32 id, u32 flags, Module* module, const char* name, ppu_func_caller func, vm::ptr<void()> lle_func = vm::null)
		: id(id)
		, flags(flags)
		, module(module)
		, name(name)
		, func(func)
		, lle_func(lle_func)
	{
	}
};

enum : u32
{
	SPET_MASKED_OPCODE,
	SPET_OPTIONAL_MASKED_OPCODE,
	SPET_LABEL,
	SPET_BRANCH_TO_LABEL,
	SPET_BRANCH_TO_FUNC,
};

struct SearchPatternEntry
{
	u32 type;
	u32 data;
	u32 mask;
	u32 num; // supplement info
};

struct StaticFunc
{
	u32 index;
	const char* name;
	std::vector<SearchPatternEntry> ops;
	u64 group;
	u32 found;
	std::unordered_map<u32, u32> labels;
};

class Module : public LogBase
{
	std::string m_name;
	bool m_is_loaded;
	void(*m_init)();

	Module() = delete;

public:
	Module(const char* name, void(*init)());

	Module(Module &other) = delete;
	Module(Module &&other) = delete;

	Module &operator =(Module &other) = delete;
	Module &operator =(Module &&other) = delete;

	~Module();

	std::function<void()> on_load;
	std::function<void()> on_unload;
	std::function<void()> on_stop;
	std::function<void(s64 value, ModuleFunc* func)> on_error;

	void Init();
	void Load();
	void Unload();

	void SetLoaded(bool loaded = true);
	bool IsLoaded() const;

	virtual const std::string& GetName() const override;
	void SetName(const std::string& name);
};

u32 add_ppu_func(ModuleFunc func);
ModuleFunc* get_ppu_func_by_nid(u32 nid, u32* out_index = nullptr);
ModuleFunc* get_ppu_func_by_index(u32 index);
void execute_ppu_func_by_index(PPUThread& CPU, u32 id);
void clear_ppu_functions();
u32 get_function_id(const char* name);

u32 add_ppu_func_sub(StaticFunc sf);
u32 add_ppu_func_sub(const char group[8], const SearchPatternEntry ops[], size_t count, const char* name, Module* module, ppu_func_caller func);

void hook_ppu_funcs(vm::ptr<u32> base, u32 size);

bool patch_ppu_import(u32 addr, u32 index);

// call specified function directly if LLE is not available, call LLE equivalent in callback style otherwise
template<typename T, typename... Args> inline auto hle_call_func(PPUThread& CPU, T func, u32 index, Args&&... args) -> decltype(func(std::forward<Args>(args)...))
{
	const auto mfunc = get_ppu_func_by_index(index);

	if (mfunc && mfunc->lle_func && (mfunc->flags & MFF_FORCED_HLE) == 0 && (mfunc->flags & MFF_NO_RETURN) == 0)
	{
		const u32 pc = vm::read32(mfunc->lle_func.addr());
		const u32 rtoc = vm::read32(mfunc->lle_func.addr() + 4);

		return cb_call<decltype(func(std::forward<Args>(args)...)), Args...>(CPU, pc, rtoc, std::forward<Args>(args)...);
	}
	else
	{
		return func(std::forward<Args>(args)...);
	}
}

#define CALL_FUNC(cpu, func, ...) hle_call_func(cpu, func, g_ppu_func_index__##func, __VA_ARGS__)

#define REG_FUNC(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), 0, &module, #name, bind_func(name)))
#define REG_FUNC_FH(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), MFF_FORCED_HLE, &module, #name, bind_func(name)))
#define REG_FUNC_NR(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), MFF_NO_RETURN, &module, #name, bind_func(name)))

#define REG_UNNAMED(module, nid) add_ppu_func(ModuleFunc(0x##nid, 0, &module, "_nid_"#nid, bind_func(_nid_##nid)))

#define REG_SUB(module, group, ns, name, ...) \
	const SearchPatternEntry name##_table[] = {__VA_ARGS__}; \
	add_ppu_func_sub(group, name##_table, sizeof(name##_table) / sizeof(SearchPatternEntry), #name, &module, bind_func(ns::name))

#define se_op_all(type, op, sup) []() { s32 XXX = 0; SearchPatternEntry res = { (type), (op), 0, (sup) }; XXX = -1; res.mask = (op) ^ ~res.data; return res; }()
#define se_op(op) se_op_all(SPET_MASKED_OPCODE, op, 0)
#define se_opt_op(op) se_op_all(SPET_OPTIONAL_MASKED_OPCODE, op, 0)
#define se_label(label) { SPET_LABEL, (label) }
#define se_br_label(op, label) se_op_all(SPET_BRANCH_TO_LABEL, op, label)
#define se_func_call(op, name) se_op_all(SPET_BRANCH_TO_FUNC, op, get_function_id(#name))

#define UNIMPLEMENTED_FUNC(module) module.Error("%s", __FUNCTION__)
