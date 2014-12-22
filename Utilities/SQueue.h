#pragma once

static const volatile bool sq_no_wait = true;

template<typename T, u32 SQSize = 666>
class SQueue
{
	mutable std::mutex m_mutex;
	u32 m_pos;
	u32 m_count;
	T m_data[SQSize];

public:
	SQueue()
		: m_pos(0)
		, m_count(0)
	{
	}

	u32 GetSize() const
	{
		return SQSize;
	}

	bool IsFull() const volatile
	{
		return m_count == SQSize;
	}

	bool Push(const T& data, const volatile bool* do_exit)
	{
		while (true)
		{
			if (m_count >= SQSize)
			{
				if (Emu.IsStopped() || (do_exit && *do_exit))
				{
					return false;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
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

	bool Pop(T& data, const volatile bool* do_exit)
	{
		while (true)
		{
			if (!m_count)
			{
				if (Emu.IsStopped() || (do_exit && *do_exit))
				{
					return false;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
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

	void Clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_count = 0;
	}

	bool Peek(T& data, const volatile bool* do_exit, u32 pos = 0)
	{
		while (true)
		{
			if (m_count <= pos)
			{
				if (Emu.IsStopped() || (do_exit && *do_exit))
				{
					return false;
				}
				
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
				continue;
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				if (m_count > pos)
				{
					break;
				}
			}
		}
		data = m_data[(m_pos + pos) % SQSize];
		return true;
	}
};
