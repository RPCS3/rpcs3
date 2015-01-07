#pragma once

class CPUThread;
class PPUThread;
class SPUThread;
class ARMv7Thread;

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
			m_addr = (u32)Memory.Alloc(size(), m_align);
			m_ptr = vm::get_ptr<T>(m_addr);
		}

		void dealloc()
		{
			if (m_addr)
			{
				Memory.Free(m_addr);
				m_addr = 0;
				m_ptr = vm::get_ptr<T>(0u);
			}
		}
		
		static var make(u32 addr, u32 size = sizeof(T), u32 align = sizeof(T))
		{
			var res;

			res.m_addr = addr;
			res.m_size = size;
			res.m_align = align;
			res.m_ptr = vm::get_ptr<T>(addr);

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

		/*
		operator const ref<T>() const
		{
			return addr();
		}
		*/

		template<typename AT> operator const ps3::ptr<T, 1, AT>() const
		{
			return ps3::ptr<T, 1, AT>::make(m_addr);
		}

		template<typename AT> operator const ps3::ptr<const T, 1, AT>() const
		{
			return ps3::ptr<const T, 1, AT>::make(m_addr);
		}
		
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
			m_addr = (u32)Memory.Alloc(size(), m_align);
			m_ptr = vm::get_ptr<T>(m_addr);
		}

		void dealloc()
		{
			if (m_addr)
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
			res.m_ptr = vm::get_ptr<T>(addr);

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
			m_addr = (u32)Memory.Alloc(size(), m_align);
			m_ptr = vm::get_ptr<T>(m_addr);
		}

		void dealloc()
		{
			if (m_addr)
			{
				Memory.Free(m_addr);
				m_addr = 0;
				m_ptr = vm::get_ptr<T>(0u);
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

	u32 stack_push(CPUThread& CPU, u32 size, u32 align, u32& old_pos);
	void stack_pop(CPUThread& CPU, u32 addr, u32 old_pos);

	template<typename T>
	class stackvar
	{
		T* m_ptr;
		u32 m_addr;
		u32 m_size;
		u32 m_align;
		u32 m_old_pos;
		CPUThread& m_thread;

		void alloc()
		{
			m_addr = stack_push(m_thread, m_size, m_align, m_old_pos);
			m_ptr = vm::get_ptr<T>(m_addr);
		}

		void dealloc()
		{
			if (m_addr)
			{
				stack_pop(m_thread, m_addr, m_old_pos);
				m_addr = 0;
				m_ptr = vm::get_ptr<T>(0u);
			}
		}

	public:
		stackvar(PPUThread& CPU, u32 size = sizeof(T), u32 align = __alignof(T))
			: m_size(size)
			, m_align(align)
			, m_thread(CPU)
		{
			alloc();
		}

		stackvar(SPUThread& CPU, u32 size = sizeof(T), u32 align = __alignof(T))
			: m_size(size)
			, m_align(align)
			, m_thread(CPU)
		{
			alloc();
		}

		stackvar(ARMv7Thread& CPU, u32 size = sizeof(T), u32 align = __alignof(T))
			: m_size(size)
			, m_align(align)
			, m_thread(CPU)
		{
			alloc();
		}

		stackvar(const stackvar& r)
			: m_size(r.m_size)
			, m_align(r.m_align)
			, m_thread(r.m_thread)
		{
			alloc();
			*m_ptr = *r.m_ptr;
		}

		stackvar(stackvar&& r)
			: m_ptr(r.m_ptr)
			, m_addr(r.m_addr)
			, m_size(r.m_size)
			, m_align(r.m_align)
			, m_old_pos(r.m_old_pos)
			, m_thread(r.m_thread)
		{
			r.m_addr = 0;
			r.m_ptr = vm::get_ptr<T>(0u);
		}

		~stackvar()
		{
			dealloc();
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

		/*
		operator const ref<T>() const
		{
		return addr();
		}
		*/

		template<typename AT> operator const ps3::ptr<T, 1, AT>() const
		{
			return ps3::ptr<T, 1, AT>::make(m_addr);
		}

		template<typename AT> operator const ps3::ptr<const T, 1, AT>() const
		{
			return ps3::ptr<const T, 1, AT>::make(m_addr);
		}

		operator T&()
		{
			return *m_ptr;
		}

		operator const T&() const
		{
			return *m_ptr;
		}
	};
}