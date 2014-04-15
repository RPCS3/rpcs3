#pragma once

template<typename T, size_t size> class SizedStack
{
	T m_ptr[size];
	uint m_count;

public:
	SizedStack()
	{
		Clear();
	}

	~SizedStack()
	{
		Clear();
	}

	void Clear()
	{
		m_count = 0;
	}

	bool Pop(T& dst)
	{
		if(!m_count)
			return false;

		dst = m_ptr[--m_count];
		return true;
	}

	bool Push(const T& src)
	{
		if(m_count + 1 > size)
			return false;

		m_ptr[m_count++] = src;
		return true;
	}

	size_t GetFreeCount() const
	{
		return size - m_count;
	}

	size_t GetCount() const
	{
		return m_count;
	}

	size_t GetMaxCount() const
	{
		return size;
	}
};

template<typename T> struct ScopedPtr
{
private:
	T* m_ptr;

public:
	ScopedPtr() : m_ptr(nullptr)
	{
	}

	ScopedPtr(T* ptr) : m_ptr(ptr)
	{
	}

	~ScopedPtr()
	{
		Swap(nullptr);
	}

	operator T*() { return m_ptr; }
	operator const T*() const { return m_ptr; }

	T* operator ->() { return m_ptr; }
	const T* operator ->() const { return m_ptr; }

	void Swap(T* ptr)
	{
		delete m_ptr;
		m_ptr = ptr;
	}
};
