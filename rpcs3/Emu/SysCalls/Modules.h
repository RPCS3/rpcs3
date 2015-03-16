#pragma once
#include "Emu/SysCalls/SC_FUNC.h"
#include "ErrorCodes.h"
#include "LogBase.h"

class Module;

// flags set in ModuleFunc
enum : u32
{
	MFF_FORCED_HLE = (1 << 0), // always call HLE function
};

// flags passed with index
enum : u32
{
	EIF_SAVE_RTOC   = (1 << 25), // save RTOC in [SP+0x28] before calling HLE/LLE function
	EIF_PERFORM_BLR = (1 << 24), // do BLR after calling HLE/LLE function

	EIF_FLAGS = 0x3000000, // all flags
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

	ModuleFunc(u32 id, u32 flags, Module* module, const char* name, ppu_func_caller func, vm::ptr<void()> lle_func = vm::ptr<void()>::make(0))
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

#define REG_FUNC(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), 0, &module, #name, bind_func(name)))
#define REG_FUNC_FH(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), MFF_FORCED_HLE, &module, #name, bind_func(name)))

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
