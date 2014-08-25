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

	ModuleFunc(u32 id, func_caller* func)
		: id(id)
		, func(func)
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
	std::vector<ModuleFunc*> m_funcs_list;

	Module(u16 id, const char* name);
	Module(const char* name, void (*init)(), void (*load)() = nullptr, void (*unload)() = nullptr);
	Module(u16 id, void (*init)(), void (*load)() = nullptr, void (*unload)() = nullptr);

	Module(Module &other) = delete;
	Module(Module &&other);

	Module &operator =(Module &other) = delete;
	Module &operator =(Module &&other);

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
	template<typename T> bool CheckId(u32 id, T*& data)
	{
		ID* id_data;

		if(!CheckID(id, id_data)) return false;

		data = id_data->m_data->get<T>();

		return true;
	}

	template<typename T> bool CheckId(u32 id, T*& data, IDType& type)
	{
		ID* id_data;

		if(!CheckID(id, id_data)) return false;

		data = id_data->m_data->get<T>();
		type = id_data->m_type;

		return true;
	}
	bool CheckID(u32 id, ID*& _id) const;

	template<typename T>
	u32 GetNewId(T* data, IDType type = TYPE_OTHER)
	{
		return GetIdManager().GetNewID<T>(GetName(), data, type);
	}

	bool RemoveId(u32 id);

	template<typename T> __forceinline void AddFunc(u32 id, T func);
	template<typename T> __forceinline void AddFunc(const char* name, T func);
	template<typename T> __forceinline void AddFuncSub(const char group[8], const u64 ops[], const char* name, T func);
};

u32 getFunctionId(const char* name);

template<typename T>
__forceinline void Module::AddFunc(u32 id, T func)
{
	m_funcs_list.emplace_back(new ModuleFunc(id, bind_func(func)));
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

	//TODO: track down where this is supposed to be deleted
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
		op.crc = ops[i];
		if (op.mask) op.crc &= op.mask;
		op.mask = re(op.mask);
		op.crc = re(op.crc);
		sf->ops.push_back(op);
	}
	PushNewFuncSub(sf);
}

#define REG_SUB(module, group, name, ...) \
	static const u64 name ## _table[] = {__VA_ARGS__ , 0}; \
	module->AddFuncSub(group, name ## _table, #name, name)

#define REG_FUNC(module, name) module->AddFunc(getFunctionId(#name), name)

#define UNIMPLEMENTED_FUNC(module) module->Todo("%s", __FUNCTION__)