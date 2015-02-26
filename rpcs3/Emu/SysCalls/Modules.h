#pragma once
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/IdManager.h"
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
	ppu_func_caller func;
	vm::ptr<void()> lle_func;

	ModuleFunc()
	{
	}

	ModuleFunc(u32 id, u32 flags, Module* module, ppu_func_caller func, vm::ptr<void()> lle_func = vm::ptr<void()>::make(0))
		: id(id)
		, flags(flags)
		, module(module)
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

	IdManager& GetIdManager() const;

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

public:
	bool CheckID(u32 id) const;

	template<typename T> bool CheckId(u32 id, std::shared_ptr<T>& data)
	{
		ID* id_data;

		if(!CheckID(id, id_data)) return false;

		data = id_data->GetData()->get<T>();

		return true;
	}

	template<typename T> bool CheckId(u32 id, std::shared_ptr<T>& data, IDType& type)
	{
		ID* id_data;

		if(!CheckID(id, id_data)) return false;

		data = id_data->GetData()->get<T>();
		type = id_data->GetType();

		return true;
	}

	bool CheckID(u32 id, ID*& _id) const;

	template<typename T>
	u32 GetNewId(std::shared_ptr<T>& data, IDType type = TYPE_OTHER)
	{
		return GetIdManager().GetNewID<T>(GetName(), data, type);
	}

	bool RemoveId(u32 id);
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

#define REG_FUNC(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), 0, &module, bind_func(name)))
#define REG_FUNC_FH(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), MFF_FORCED_HLE, &module, bind_func(name)))

#define REG_UNNAMED(module, nid) add_ppu_func(ModuleFunc(0x##nid, 0, &module, bind_func(_nid_##nid)))

#define REG_SUB(module, group, name, ...) \
	const SearchPatternEntry name##_table[] = {__VA_ARGS__}; \
	add_ppu_func_sub(group, name##_table, sizeof(name##_table) / sizeof(SearchPatternEntry), #name, &module, bind_func(name))

#define se_op(op) []() { SearchPatternEntry res = { SPET_MASKED_OPCODE }; s32 XXX = 0; res.data = (op); XXX = -1; res.mask = (op) ^ ~res.data; return res; }()
#define se_opt(op) []() { SearchPatternEntry res = { SPET_OPTIONAL_MASKED_OPCODE }; s32 XXX = 0; res.data = (op); XXX = -1; res.mask = (op) ^ ~res.data; return res; }()
#define se_label(label) { SPET_LABEL, (label) }
#define se_lbr(op, label) []() { SearchPatternEntry res = { SPET_BRANCH_TO_LABEL, 0, 0, (label) }; s32 XXX = 0; res.data = (op); XXX = -1; res.mask = (op) ^ ~res.data; return res; }()
#define se_call(op, name) []() { SearchPatternEntry res = { SPET_BRANCH_TO_FUNC, 0, 0, get_function_id(#name) }; s32 XXX = 0; res.data = (op); XXX = -1; res.mask = (op) ^ ~res.data; return res; }()

#define UNIMPLEMENTED_FUNC(module) module.Error("%s", __FUNCTION__)
