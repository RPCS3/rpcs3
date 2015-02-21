#pragma once
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/IdManager.h"
#include "ErrorCodes.h"
#include "LogBase.h"

class Module;

struct ModuleFunc
{
	u32 id;
	Module* module;
	ppu_func_caller func;
	vm::ptr<void()> lle_func;

	ModuleFunc(u32 id, Module* module, ppu_func_caller func, vm::ptr<void()> lle_func = vm::ptr<void()>::make(0))
		: id(id)
		, module(module)
		, func(func)
		, lle_func(lle_func)
	{
	}
};

struct SFuncOp
{
	u32 crc;
	u32 mask;
};

struct SFunc
{
	u32 index;
	const char* name;
	std::vector<SFuncOp> ops;
	u64 group;
	u32 found;
};

class StaticFuncManager;

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

u32 add_ppu_func_sub(SFunc sf);

__forceinline static u32 add_ppu_func_sub(const char group[8], const u64 ops[], const char* name, Module* module, ppu_func_caller func)
{
	SFunc sf;
	sf.index = add_ppu_func(ModuleFunc(get_function_id(name), module, func));
	sf.name = name;
	sf.group = *(u64*)group;
	sf.found = 0;

	// TODO: check for self-inclusions, use CRC
	for (u32 i = 0; ops[i]; i++)
	{
		SFuncOp op;
		op.mask = ops[i] >> 32;
		op.crc = (u32)ops[i];
		if (op.mask) op.crc &= op.mask;
		op.mask = re32(op.mask);
		op.crc = re32(op.crc);
		sf.ops.push_back(op);
	}

	return add_ppu_func_sub(sf);
}

#define REG_SUB(module, group, name, ...) \
	static const u64 name ## _table[] = {__VA_ARGS__ , 0}; \
	if (name ## _table[0]) add_ppu_func_sub(group, name ## _table, #name, &module, bind_func(name))

#define REG_FUNC(module, name) add_ppu_func(ModuleFunc(get_function_id(#name), &module, bind_func(name)))

#define REG_FUNC2(module, nid, name) add_ppu_func(ModuleFunc(nid, &module, bind_func(name)))

#define UNIMPLEMENTED_FUNC(module) module.Error("%s", __FUNCTION__)
