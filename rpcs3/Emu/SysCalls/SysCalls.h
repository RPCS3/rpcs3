#pragma once
#include "ErrorCodes.h"
#include "LogBase.h"
#include "Emu/IdManager.h"

//#define SYSCALLS_DEBUG

class SysCallBase;

namespace detail
{
	bool CheckIdID(u32 id, ID*& _id, const std::string& name);

	template<typename T> bool CheckId(u32 id, std::shared_ptr<T>& data, const std::string& name)
	{
		ID* id_data;
		if(!CheckIdID(id, id_data, name)) return false;
		data = id_data->GetData()->get<T>();
		return true;
	}
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
		return GetIdManager().CheckID(id) && GetIdManager().GetID(id).GetName() == GetName();
	}

	template<typename T>
	bool CheckId(u32 id, std::shared_ptr<T>& data) const
	{
		return detail::CheckId(id, data, GetName());
	}

	template<typename T>
	u32 GetNewId(std::shared_ptr<T>& data, IDType type = TYPE_OTHER)
	{
		return GetIdManager().GetNewID<T>(GetName(), data, type);
	}

	bool RemoveId(u32 id);
};

extern bool dump_enable;

class PPUThread;

class SysCalls
{
public:
	static void DoSyscall(PPUThread& CPU, u32 code);
	static std::string GetHLEFuncName(const u32 fid);
};
