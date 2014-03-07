#pragma once

template<typename T, u32 SQSize = 666>
class SQueue
{
	SMutex m_mutex;
	u32 m_pos;
	u32 m_count;
	T m_data[SQSize];

public:
	SQueue()
		: m_pos(0)
		, m_count(0)
	{
	}

	bool Push(const T& data)
	{
		while (true)
		{
			if (m_mutex.GetOwner() == m_mutex.GetDeadValue())
			{
				return false;
			}

			if (m_count >= SQSize)
			{
				if (Emu.IsStopped())
				{
					return false;
				}
				Sleep(1);
				continue;
			}

			{
				SMutexLocker lock(m_mutex);

				if (m_count >= SQSize) continue;

				m_data[(m_pos + m_count++) % SQSize] = data;

				return true;
			}
		}
	}

	bool Pop(T& data)
	{
		while (true)
		{
			if (m_mutex.GetOwner() == m_mutex.GetDeadValue())
			{
				return false;
			}

			if (!m_count)
			{
				if (Emu.IsStopped())
				{
					return false;
				}
				Sleep(1);
				continue;
			}

			{
				SMutexLocker lock(m_mutex);

				if (!m_count) continue;

				data = m_data[m_pos];
				m_pos = (m_pos + 1) % SQSize;
				m_count--;

				return true;
			}
		}
	}

	volatile u32 GetCount()
	{
		return m_count;
	}

	volatile bool IsEmpty()
	{
		return !m_count;
	}

	void Clear()
	{
		SMutexLocker lock(m_mutex);
		m_count = 0;
	}

	T& Peek(u32 pos = 0)
	{
		SMutexLocker lock(m_mutex);
		return m_data[(m_pos + pos) % SQSize];
	}
};
