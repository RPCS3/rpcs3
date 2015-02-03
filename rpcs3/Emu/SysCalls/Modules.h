#pragma once
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/IdManager.h"
#include "ErrorCodes.h"
#include "LogBase.h"

//TODO
struct ModuleFunc
{
	u32 id;
	func_caller* func;
	vm::ptr<void()> lle_func;

	ModuleFunc(u32 id, func_caller* func, vm::ptr<void()> lle_func = vm::ptr<void()>::make(0))
		: id(id)
		, func(func)
		, lle_func(lle_func)
	{
	}

	~ModuleFunc()
	{
		delete func;
	}
};

struct SFuncOp
{
	u32 crc;
	u32 mask;
};

struct SFunc
{
	func_caller* func;
	void* ptr;
	const char* name;
	std::vector<SFuncOp> ops;
	u64 group;
	u32 found;

	~SFunc()
	{
		delete func;
	}
};

class StaticFuncManager;

class Module : public LogBase
{
	std::string m_name;
	u16 m_id;
	bool m_is_loaded;
	void (*m_load_func)();
	void (*m_unload_func)();

	IdManager& GetIdManager() const;
	void PushNewFuncSub(SFunc* func);

public:
	std::unordered_map<u32, ModuleFunc*> m_funcs_list;

	Module(u16 id, const char* name, void(*load)() = nullptr, void(*unload)() = nullptr);

	Module(Module &other) = delete;
	Module(Module &&other);

	Module &operator =(Module &other) = delete;
	Module &operator =(Module &&other);
	
	ModuleFunc* GetFunc(u32 id)
	{
		auto res = m_funcs_list.find(id);

		if (res == m_funcs_list.end())
			return nullptr;

		return res->second;
	}

	~Module();

	void Load();
	void UnLoad();
	bool Load(u32 id);
	bool UnLoad(u32 id);

	void SetLoaded(bool loaded = true);
	bool IsLoaded() const;

	u16 GetID() const;
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
	
	void RegisterLLEFunc(u32 id, vm::ptr<void()> func)
	{
		if (auto f = GetFunc(id))
		{
			f->lle_func = func;
			return;
		}
		
		m_funcs_list[id] = new ModuleFunc(id, nullptr, func);
	}

	template<typename T> __forceinline void AddFunc(u32 id, T func);
	template<typename T> __forceinline void AddFunc(const char* name, T func);	
	template<typename T> __forceinline void AddFuncSub(const char group[8], const u64 ops[], const char* name, T func);
};

u32 getFunctionId(const char* name);

template<typename T>
__forceinline void Module::AddFunc(u32 id, T func)
{
	m_funcs_list[id] = new ModuleFunc(id, bind_func(func));
}

template<typename T>
__forceinline void Module::AddFunc(const char* name, T func)
{
	AddFunc(getFunctionId(name), func);
}

template<typename T>
__forceinline void Module::AddFuncSub(const char group[8], const u64 ops[], const char* name, T func)
{
	if (!ops[0]) return;

	SFunc* sf = new SFunc;
	sf->ptr = (void *)func;
	sf->func = bind_func(func);
	sf->name = name;
	sf->group = *(u64*)group;
	sf->found = 0;

	// TODO: check for self-inclusions, use CRC
	for (u32 i = 0; ops[i]; i++)
	{
		SFuncOp op;
		op.mask = ops[i] >> 32;
		op.crc = (u32)ops[i];
		if (op.mask) op.crc &= op.mask;
		op.mask = re32(op.mask);
		op.crc = re32(op.crc);
		sf->ops.push_back(op);
	}
	PushNewFuncSub(sf);
}

void fix_import(Module* module, u32 func, u32 addr);

#define FIX_IMPORT(module, func, addr) fix_import(module, getFunctionId(#func), addr)

void fix_relocs(Module* module, u32 lib, u32 start, u32 end, u32 seg2);

#define REG_SUB(module, group, name, ...) \
	static const u64 name ## _table[] = {__VA_ARGS__ , 0}; \
	module->AddFuncSub(group, name ## _table, #name, name)

#define REG_FUNC(module, name) module->AddFunc(getFunctionId(#name), name)

#define UNIMPLEMENTED_FUNC(module) module->Todo("%s", __FUNCTION__)