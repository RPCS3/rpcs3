#pragma once

template<typename T, u32 SQSize = 666>
class SQueue
{
	std::mutex m_mutex;
	u32 m_pos;
	u32 m_count;
	T m_data[SQSize];

public:
	SQueue()
		: m_pos(0)
		, m_count(0)
	{
	}

	const u32 GetSize() const
	{
		return SQSize;
	}

	bool Push(const T& data)
	{
		while (true)
		{
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
				std::lock_guard<std::mutex> lock(m_mutex);

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
				std::lock_guard<std::mutex> lock(m_mutex);

				if (!m_count) continue;

				data = m_data[m_pos];
				m_pos = (m_pos + 1) % SQSize;
				m_count--;

				return true;
			}
		}
	}

	volatile u32 GetCount() // may be not safe
	{
		return m_count;
	}

	volatile bool IsEmpty() // may be not safe
	{
		return !m_count;
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_count = 0;
	}

	T& Peek(u32 pos = 0)
	{
		while (true)
		{
			if (!m_count)
			{
				if (Emu.IsStopped())
				{
					break;
				}
				Sleep(1);
				continue;
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				if (m_count) break;
			}
		}
		return m_data[(m_pos + pos) % SQSize];
	}
};
