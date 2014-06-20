#pragma once

template<typename T, u32 SQSize = 666>
class SQueue
{
	std::mutex m_mutex;
	NamedThreadBase* push_waiter;
	NamedThreadBase* pop_waiter;
	u32 m_pos;
	u32 m_count;
	T m_data[SQSize];

public:
	SQueue()
		: m_pos(0)
		, m_count(0)
		, push_waiter(nullptr)
		, pop_waiter(nullptr)
	{
	}

	const u32 GetSize() const
	{
		return SQSize;
	}

	bool Push(const T& data)
	{
		NamedThreadBase* t = GetCurrentNamedThread();
		push_waiter = t;

		while (true)
		{
			if (m_count >= SQSize)
			{
				if (Emu.IsStopped())
				{
					return false;
				}

				SM_Sleep();
				continue;
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);

				if (m_count >= SQSize) continue;
				if (pop_waiter && !m_count) pop_waiter->Notify();

				m_data[(m_pos + m_count++) % SQSize] = data;

				push_waiter = nullptr;
				return true;
			}
		}
	}

	bool Pop(T& data)
	{
		NamedThreadBase* t = GetCurrentNamedThread();
		pop_waiter = t;

		while (true)
		{
			if (!m_count)
			{
				if (Emu.IsStopped())
				{
					return false;
				}

				SM_Sleep();
				continue;
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);

				if (!m_count) continue;
				if (push_waiter && m_count >= SQSize) push_waiter->Notify();

				data = m_data[m_pos];
				m_pos = (m_pos + 1) % SQSize;
				m_count--;

				pop_waiter = nullptr;
				return true;
			}
		}
	}

	volatile u32 GetCount() // may be thread unsafe
	{
		return m_count;
	}

	volatile bool IsEmpty() // may be thread unsafe
	{
		return !m_count;
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (push_waiter && m_count >= SQSize) push_waiter->Notify();
		m_count = 0;
	}

	T& Peek(u32 pos = 0)
	{
		NamedThreadBase* t = GetCurrentNamedThread();
		pop_waiter = t;

		while (true)
		{
			if (!m_count)
			{
				if (Emu.IsStopped())
				{
					break;
				}
				
				SM_Sleep();
				continue;
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				if (m_count)
				{
					pop_waiter = nullptr;
					break;
				}
			}
		}
		return m_data[(m_pos + pos) % SQSize];
	}
};
