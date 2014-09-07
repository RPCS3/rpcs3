#pragma once
#include "ErrorCodes.h"
#include "LogBase.h"
#include "Emu/IdManager.h"

//#define SYSCALLS_DEBUG

class SysCallBase;

namespace detail{
	template<typename T> bool CheckId(u32 id, T*& data,const std::string &name)
	{
		ID* id_data;
		if(!CheckId(id, id_data,name)) return false;
		data = id_data->m_data->get<T>();
		return true;
	}

	template<> bool CheckId<ID>(u32 id, ID*& _id,const std::string &name);
}

class SysCallBase : public LogBase
{
private:
	std::string m_module_name;
	//u32 m_id;
	IdManager& GetIdManager() const;

public:
	SysCallBase(const std::string& name/*, u32 id*/)
		: m_module_name(name)
		//, m_id(id)
	{
	}

	virtual const std::string& GetName() const override
	{
		return m_module_name;
	}

	bool CheckId(u32 id) const
	{
		return GetIdManager().CheckID(id) && GetIdManager().GetID(id).m_name == GetName();
	}

	template<typename T>
	bool CheckId(u32 id, T*& data) const
	{
		return detail::CheckId(id,data,GetName());
	}

	template<typename T>
	u32 GetNewId(T* data, IDType type = TYPE_OTHER)
	{
		return GetIdManager().GetNewID<T>(GetName(), data, type);
	}

	bool RemoveId(u32 id);
};

extern bool dump_enable;

class SysCalls
{
public:
	static void DoSyscall(u32 code);
	static std::string GetHLEFuncName(const u32 fid);
};
