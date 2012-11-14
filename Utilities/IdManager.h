#pragma once
#include "Array.h"

typedef u32 ID_TYPE;

struct ID
{
	bool m_used;
	wxString m_name;
	u8 m_attr;
	void* m_data;

	ID(bool used = false, const wxString& name = wxEmptyString, void* data = NULL, const u8 attr = 0)
		: m_used(used)
		, m_name(name)
		, m_data(data)
		, m_attr(attr)
	{
	}
};

class IdManager
{
	ArrayF<ID> IDs;

	static const u64 first_id = 1;

	void Cleanup()
	{
		while(IDs.GetCount())
		{
			const u32 num = IDs.GetCount()-1;
			ID& id = IDs[num];
			if(id.m_used) break;
			IDs.RemoveAt(num);
		}
	}
	
public:
	IdManager()
	{
	}
	
	~IdManager()
	{
		Clear();
	}
	
	static ID_TYPE GetMaxID()
	{
		return (ID_TYPE)-1;
	}
	
	bool CheckID(const u64 id)
	{
		if(id == 0 || id > (u64)NumToID(IDs.GetCount()-1) || id >= GetMaxID()) return false;

		return IDs[IDToNum(id)].m_used;
	}
	
	__forceinline const ID_TYPE NumToID(const ID_TYPE num) const
	{
		return num + first_id;
	}
	
	__forceinline const ID_TYPE IDToNum(const ID_TYPE id) const
	{
		return id - first_id;
	}

	void Clear()
	{
		IDs.Clear();
	}
	
	virtual ID_TYPE GetNewID(const wxString& name = wxEmptyString, void* data = NULL, const u8 attr = 0)
	{
		for(uint i=0; i<IDs.GetCount(); ++i)
		{
			if(IDs[i].m_used) continue;
			return NumToID(i);
		}

		const ID_TYPE pos = IDs.GetCount();
		if(NumToID(pos) >= GetMaxID()) return 0;
		IDs.Add(new ID(true, name, data, attr));
		return NumToID(pos);
	}
	
	ID& GetIDData(const ID_TYPE _id)
	{
		//if(!CheckID(_id)) return IDs.Get(0); //TODO
		return IDs[IDToNum(_id)];
	}
	

	bool GetFirst(ID& dst, ID_TYPE* _id = NULL)
	{
		u32 pos = 0;
		return GetNext(pos, dst, _id);
	}

	bool HasID(const s64 id)
	{
		if(id == wxID_ANY) return IDs.GetCount() != 0;
		return CheckID(id);
	}

	bool GetNext(u32& pos, ID& dst, ID_TYPE* _id = NULL)
	{
		while(pos < IDs.GetCount())
		{
			ID& id = IDs[pos++];
			if(!id.m_used) continue;
			dst = id;
			if(_id) *_id = NumToID(pos - 1);
			return true;
		}

		return false;
	}

	virtual bool RemoveID(const ID_TYPE _id, bool free_data = true)
	{
		if(!CheckID(_id)) return false;
		ID& id = IDs[IDToNum(_id)];
		if(!id.m_used) return false;
		id.m_used = false;
		id.m_attr = 0;
		id.m_name.Clear();
		if(free_data) free(id.m_data);
		id.m_data = NULL;
		if(IDToNum(_id) == IDs.GetCount()-1) Cleanup();
		return true;
	}
};