#pragma once

#include "Emu/SysCalls/SC_FUNC.h"

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
		safe_delete(func);
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
		safe_delete(func);
	}
};

class Module
{
	std::string m_name;
	u16 m_id;
	bool m_is_loaded;
	void (*m_load_func)();
	void (*m_unload_func)();

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
	std::string GetName() const;
	void SetName(const std::string& name);

public:
	bool IsLogging()
	{
		return Ini.HLELogging.GetValue();
	}

	template<typename... Targs> void Notice(const u32 id, const char* fmt, Targs... args)
	{
		LOG_NOTICE(HLE, GetName() + fmt::Format("[%d]: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Notice(const char* fmt, Targs... args)
	{
		LOG_NOTICE(HLE, GetName() + ": " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> __forceinline void Log(const char* fmt, Targs... args)
	{
		if (IsLogging())
		{
			Notice(fmt, args...);
		}
	}

	template<typename... Targs> __forceinline void Log(const u32 id, const char* fmt, Targs... args)
	{
		if (IsLogging())
		{
			Notice(id, fmt, args...);
		}
	}

	template<typename... Targs> void Warning(const u32 id, const char* fmt, Targs... args)
	{
		LOG_WARNING(HLE, GetName() + fmt::Format("[%d] warning: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Warning(const char* fmt, Targs... args)
	{
		LOG_WARNING(HLE, GetName() + " warning: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const u32 id, const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + fmt::Format("[%d] error: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + " error: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const u32 id, const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + fmt::Format("[%d] TODO: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + " TODO: " + fmt::Format(fmt, args...));
	}

	bool CheckID(u32 id) const;
	template<typename T> bool CheckId(u32 id, T*& data)
	{
		ID* id_data;

		if(!CheckID(id, id_data)) return false;

		data = id_data->m_data->get<T>();

		return true;
	}

	template<typename T> bool CheckId(u32 id, T*& data, u32& attr)
	{
		ID* id_data;

		if(!CheckID(id, id_data)) return false;

		data = id_data->m_data->get<T>();
		attr = id_data->m_attr;

		return true;
	}
	bool CheckID(u32 id, ID*& _id) const;

	template<typename T>
	u32 GetNewId(T* data, u8 flags = 0)
	{
		return Emu.GetIdManager().GetNewID<T>(GetName(), data, flags);
	}

	template<typename T> __forceinline void AddFunc(u32 id, T func);
	template<typename T> __forceinline void AddFunc(const std::string& name, T func);
	template<typename T> __forceinline void AddFuncSub(const char group[8], const u64 ops[], const char* name, T func);
};

u32 getFunctionId(const std::string& name);

template<typename T>
__forceinline void Module::AddFunc(u32 id, T func)
{
	m_funcs_list.emplace_back(new ModuleFunc(id, bind_func(func)));
}

template<typename T>
__forceinline void Module::AddFunc(const std::string& name, T func)
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
	Emu.GetSFuncManager().push_back(sf);
}
