#pragma once

namespace vm
{
	template<typename T>
	class var
	{
		u32 m_addr;
		u32 m_size;
		u32 m_align;
		T* m_ptr;

	public:
		var(u32 size = sizeof(T), u32 align = sizeof(T))
			: m_size(size)
			, m_align(align)
		{
			alloc();
		}

		var(const var& r)
			: m_size(r.m_size)
			, m_align(r.m_align)
		{
			alloc();
			*m_ptr = *r.m_ptr;
		}

		~var()
		{
			dealloc();
		}

		void alloc()
		{
			m_addr = Memory.Alloc(size(), m_align);
			m_ptr = Memory.IsGoodAddr(m_addr, size()) ? get_ptr<T>(m_addr) : nullptr;
		}

		void dealloc()
		{
			if (check())
			{
				Memory.Free(m_addr);
				m_addr = 0;
				m_ptr = nullptr;
			}
		}
		
		static var make(u32 addr, u32 size = sizeof(T), u32 align = sizeof(T))
		{
			var res;

			res.m_addr = addr;
			res.m_size = size;
			res.m_align = align;
			res.m_ptr = Memory.IsGoodAddr(addr, size) ? get_ptr<T>(addr) : nullptr;

			return res;
		}

		T* operator -> ()
		{
			return m_ptr;
		}

		const T* operator -> () const
		{
			return m_ptr;
		}
		
		T* get_ptr()
		{
			return m_ptr;
		}

		const T* get_ptr() const
		{
			return m_ptr;
		}

		T& value()
		{
			return *m_ptr;
		}

		const T& value() const
		{
			return *m_ptr;
		}

		u32 addr() const
		{
			return m_addr;
		}

		u32 size() const
		{
			return m_size;
		}

		bool check() const
		{
			return m_ptr != nullptr;
		}

		/*
		operator const ref<T>() const
		{
			return addr();
		}
		*/
		
		operator T&()
		{
			return *m_ptr;
		}
		
		operator const T&() const
		{
			return *m_ptr;
		}
	};
	
	template<typename T>
	class var<T[]>
	{
		u32 m_addr;
		u32 m_count;
		u32 m_size;
		u32 m_align;
		T* m_ptr;

	public:
		var(u32 count, u32 size = sizeof(T), u32 align = sizeof(T))
			: m_count(count)
			, m_size(size)
			, m_align(align)
		{
			alloc();
		}

		~var()
		{
			dealloc();
		}

		var(const var& r)
			: m_size(r.m_size)
			, m_align(r.m_align)
		{
			alloc();
			memcpy(m_ptr, r.m_ptr, size());
		}

		void alloc()
		{
			m_addr = Memory.Alloc(size(), m_align);
			m_ptr = Memory.IsGoodAddr(m_addr, size()) ? get_ptr<T>(m_addr) : nullptr;
		}

		void dealloc()
		{
			if (check())
			{
				Memory.Free(m_addr);
				m_addr = 0;
				m_ptr = nullptr;
			}
		}

		static var make(u32 addr, u32 count, u32 size = sizeof(T), u32 align = sizeof(T))
		{
			var res;

			res.m_addr = addr;
			res.m_count = count;
			res.m_size = size;
			res.m_align = align;
			res.m_ptr = Memory.IsGoodAddr(addr, size * count) ? get_ptr<T>(addr) : nullptr;

			return res;
		}

		T* begin()
		{
			return m_ptr;
		}

		const T* begin() const
		{
			return m_ptr;
		}

		T* end()
		{
			return m_ptr + count();
		}

		const T* end() const
		{
			return m_ptr + count();
		}

		T* operator -> ()
		{
			return m_ptr;
		}

		T& get()
		{
			return m_ptr;
		}

		const T& get() const
		{
			return m_ptr;
		}

		const T* operator -> () const
		{
			return m_ptr;
		}

		uint addr() const
		{
			return m_addr;
		}

		uint size() const
		{
			return m_count * m_size;
		}

		uint count() const
		{
			return m_count;
		}

		bool check() const
		{
			return Memory.IsGoodAddr(m_addr, size());
		}

		template<typename T1>
		operator const T1() const
		{
			return T1(*m_ptr);
		}

		template<typename T1>
		operator T1()
		{
			return T1(*m_ptr);
		}

		operator const T&() const
		{
			return *m_ptr;
		}

		operator T&()
		{
			return *m_ptr;
		}

		operator const T*() const
		{
			return m_ptr;
		}

		operator T*()
		{
			return m_ptr;
		}

		T& operator [](int index)
		{
			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}

		const T& operator [](int index) const
		{
			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}

		T& at(uint index)
		{
			if (index >= count())
				throw std::out_of_range(std::to_string(index) + " >= " + count());

			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}

		const T& at(uint index) const
		{
			if (index >= count())
				throw std::out_of_range(std::to_string(index) + " >= " + count());

			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}

		T* ptr()
		{
			return m_ptr;
		}

		const T* ptr() const
		{
			return m_ptr;
		}

		/*
		operator const ptr<T>() const
		{
			return addr();
		}
		*/
		template<typename NT>
		NT* To(uint offset = 0)
		{
			return (NT*)(m_ptr + offset);
		}
	};
	
	template<typename T, int _count>
	class var<T[_count]>
	{
		u32 m_addr;
		u32 m_size;
		u32 m_align;
		T* m_ptr;

	public:
		var(u32 size = sizeof(T), u32 align = sizeof(T))
			: m_size(size)
			, m_align(align)
		{
			alloc();
		}

		~var()
		{
			dealloc();
		}

		var(const var& r)
			: m_size(r.m_size)
			, m_align(r.m_align)
		{
			alloc();
			memcpy(m_ptr, r.m_ptr, size());
		}

		void alloc()
		{
			m_addr = Memory.Alloc(size(), m_align);
			m_ptr = Memory.IsGoodAddr(m_addr, size()) ? get_ptr<T>(m_addr) : nullptr;
		}

		void dealloc()
		{
			if (check())
			{
				Memory.Free(m_addr);
				m_addr = 0;
				m_ptr = nullptr;
			}
		}

		T* operator -> ()
		{
			return m_ptr;
		}
		
		T* begin()
		{
			return m_ptr;
		}
		
		const T* begin() const
		{
			return m_ptr;
		}
		
		T* end()
		{
			return m_ptr + count();
		}
		
		const T* end() const
		{
			return m_ptr + count();
		}

		T& get()
		{
			return m_ptr;
		}

		const T& get() const
		{
			return m_ptr;
		}

		const T* operator -> () const
		{
			return m_ptr;
		}

		uint addr() const
		{
			return m_addr;
		}
		
		__forceinline uint count() const
		{
			return _count;
		}
		
		uint size() const
		{
			return _count * m_size;
		}

		bool check() const
		{
			return m_ptr != nullptr;
		}

		template<typename T1>
		operator const T1() const
		{
			return T1(*m_ptr);
		}

		template<typename T1>
		operator T1()
		{
			return T1(*m_ptr);
		}

		operator const T&() const
		{
			return *m_ptr;
		}

		operator T&()
		{
			return *m_ptr;
		}

		operator const T*() const
		{
			return m_ptr;
		}

		operator T*()
		{
			return m_ptr;
		}

		T& operator [](uint index)
		{
			assert(index < count());
			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}
		
		const T& operator [](uint index) const
		{
			assert(index < count());
			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}
		
		T& at(uint index)
		{
			if (index >= count())
				throw std::out_of_range(std::to_string(index) + " >= " + count());

			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}
		
		const T& at(uint index) const
		{
			if (index >= count())
				throw std::out_of_range(std::to_string(index) + " >= " + count());

			return *(T*)((u8*)m_ptr + (m_size < m_align ? m_align : m_size) * index);
		}

		/*
		operator const ptr<T>() const
		{
			return addr();
		}
		*/
		template<typename NT>
		NT* To(uint offset = 0)
		{
			return (NT*)(m_ptr + offset);
		}
	};
}