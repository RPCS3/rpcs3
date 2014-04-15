#pragma once

class Callback
{
protected:
	u64 m_addr;
	u32 m_slot;

	bool m_has_data;

	std::string m_name;

public:
	u64 a1;
	u64 a2;
	u64 a3;
	u64 a4;

	u32 GetSlot() const;
	u64 GetAddr() const;
	void SetSlot(u32 slot);
	void SetAddr(u64 addr);
	bool HasData() const;

	Callback(u32 slot = 0, u64 addr = 0);
	void Handle(u64 a1 = 0, u64 a2 = 0, u64 a3 = 0, u64 a4 = 0);
	void Branch(bool wait);
	void SetName(const std::string& name);

	operator bool() const;
};

struct Callback2 : public Callback
{
	Callback2(u32 slot, u64 addr, u64 userdata);
	void Handle(u64 status);
};

struct Callback3 : public Callback
{
	Callback3(u32 slot, u64 addr, u64 userdata);
	void Handle(u64 status, u64 param);
};

struct Callbacks
{
	std::vector<Callback> m_callbacks;
	bool m_in_manager;

	Callbacks() : m_in_manager(false)
	{
	}

	virtual void Register(u32 slot, u64 addr, u64 userdata)
	{
		Unregister(slot);
	}

	void Unregister(u32 slot)
	{
		for(u32 i=0; i<m_callbacks.size(); ++i)
		{
			if(m_callbacks[i].GetSlot() == slot)
			{
				m_callbacks.erase(m_callbacks.begin() + i);
				break;
			}
		}
	}

	virtual void Handle(u64 status, u64 param = 0)=0;

	bool Check()
	{
		bool handled = false;

		for(u32 i=0; i<m_callbacks.size(); ++i)
		{
			if(m_callbacks[i].HasData())
			{
				handled = true;
				m_callbacks[i].Branch(true);
			}
		}

		return handled;
	}
};

struct Callbacks2 : public Callbacks
{
	Callbacks2() : Callbacks()
	{
	}

	void Register(u32 slot, u64 addr, u64 userdata)
	{
		Callbacks::Register(slot, addr, userdata);
		Callback2 callback(slot, addr, userdata);
		m_callbacks.push_back(callback);
	}

	void Handle(u64 a1, u64 a2)
	{
		for(u32 i=0; i<m_callbacks.size(); ++i)
		{
			((Callback2&)m_callbacks[i]).Handle(a1);
		}
	}
};

struct Callbacks3 : public Callbacks
{
	Callbacks3() : Callbacks()
	{
	}

	void Register(u32 slot, u64 addr, u64 userdata)
	{
		Callbacks::Register(slot, addr, userdata);
		Callback3 callback(slot, addr, userdata);
		m_callbacks.push_back(callback);
	}

	void Handle(u64 a1, u64 a2)
	{
		for(u32 i=0; i<m_callbacks.size(); ++i)
		{
			((Callback3&)m_callbacks[i]).Handle(a1, a2);
		}
	}
};

struct CallbackManager
{
	std::vector<Callbacks *> m_callbacks;
	Callbacks3 m_exit_callback;

	void Add(Callbacks& c)
	{
		if(c.m_in_manager) return;

		c.m_in_manager = true;
		m_callbacks.push_back(&c);
	}

	void Init()
	{
		Add(m_exit_callback);
	}

	void Clear()
	{
		for(u32 i=0; i<m_callbacks.size(); ++i)
		{
			m_callbacks[i]->m_callbacks.clear();
			m_callbacks[i]->m_in_manager = false;
		}

		m_callbacks.clear();
	}
};
