#pragma once
#include <mutex>
#include <array>
#include <cassert>

//Simple non-resizable FIFO Ringbuffer that can be simultaneously be read from and written to
//if we ever get to use boost please replace this with boost::circular_buffer, there's no reason
//why we would have to keep this amateur attempt at such a fundamental data-structure around
template<typename T, unsigned int MaxBufferSize>
class safe_ring_buffer
{
	std::array<T, MaxBufferSize> m_buffer;
	//this is a recursive mutex because the get methods lock it but the only
	//way to be sure that they do not block is to check the size and the only
	//way to check the size and use get atomically is to lock this mutex,
	//so it goes:
	//lock get mutex-->check size-->call get-->lock get mutex-->unlock get mutex-->return from get-->unlock get mutex
	std::recursive_mutex m_mtx_get;
	std::mutex m_mtx_put;

	size_t m_get = 0;
	size_t m_put = 0;

	size_t move_get(size_t by = 1)
	{
		return (m_get + by) % MaxBufferSize;
	}

	size_t move_put(size_t by = 1)
	{
		return (m_put + by) % MaxBufferSize;
	}

public:
	//blocks until there's something to get, so check "spaceLeft()" if you want to avoid blocking
	//also lock the get mutex around the spaceLeft() check and the pop if you want to avoid racing
	T pop()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtx_get);
		while (m_get == m_put)
		{
			//wait until there's actually something to get
			//throwing an exception might be better, blocking here is a little awkward
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}
		size_t result_index = m_get;
		m_get = move_get();
		return m_buffer[result_index];
	}

	//blocks if the buffer is full until there's enough room
	void push(T &put_ele)
	{
		std::lock_guard<std::mutex> lock(m_mtx_put);
		while (move_put() == m_get)
		{
			//if this is reached a lot it's time to increase the buffer size
			//or implement dynamic re-sizing
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		m_buffer[m_put] = std::forward(put_ele);
		m_put = move_put();
	}

	bool empty()
	{
		return m_get == m_put;
	}

	//returns the amount of free places, this is the amount of actual free spaces-1
	//since m_get==m_put signals an empty buffer we can't actually use the last free
	//space, so we shouldn't report it as free.
	size_t free_size() //apparently free() is a macro definition in msvc in some conditions
	{
		if (m_get < m_put)
		{
			return m_buffer.size() - (m_put - m_get) - 1;
		}

		if (m_get > m_put)
		{
			return m_get - m_put - 1;
		}

		return m_buffer.size() - 1;
	}

	size_t size()
	{
		//the magic -1 is the same magic 1 that is explained in the free_size() function
		return m_buffer.size() - free_size() - 1;
	}

	//takes random access iterator to T
	template<typename IteratorType>
	void push(IteratorType from, IteratorType until)
	{
		std::lock_guard<std::mutex> lock(m_mtx_put);
		size_t length = until - from;

		//if whatever we're trying to store is greater than the entire buffer the following loop will be infinite
		assert(m_buffer.size() > length);
		while (free_size() < length)
		{
			//if this is reached a lot it's time to increase the buffer size
			//or implement dynamic re-sizing
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		if (m_put + length <= m_buffer.size())
		{
			std::copy(from, until, m_buffer.begin() + m_put);
		}
		else
		{
			size_t till_end = m_buffer.size() - m_put;
			std::copy(from, from + till_end, m_buffer.begin() + m_put);
			std::copy(from + till_end, until, m_buffer.begin());
		}

		m_put = move_put(length);

	}

	//takes output iterator to T
	template<typename IteratorType>
	void pop(IteratorType output, size_t n)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtx_get);
		//make sure we're not trying to retrieve more than is in
		assert(n <= size());
		peek<IteratorType>(output, n);
		m_get = move_get(n);
	}

	//takes output iterator to T
	template<typename IteratorType>
	void peek(IteratorType output, size_t n)
	{
		size_t get = m_get;
		if (get + n <= m_buffer.size())
		{
			std::copy_n(m_buffer.begin() + get, n, output);
		}
		else
		{
			auto next = std::copy(m_buffer.begin() + get, m_buffer.end(), output);
			std::copy_n(m_buffer.begin(), n - (m_buffer.size() - get), next);
		}
	}

	//well this is just asking for trouble
	//but the comment above the declaration of mMutGet explains why it's there
	//if there's a better way please remove this
	void lock_get()
	{
		m_mtx_get.lock();
	}

	//well this is just asking for trouble
	//but the comment above the declaration of mMutGet explains why it's there
	//if there's a better way please remove this
	void unlock_get()
	{
		m_mtx_get.unlock();
	}
};
