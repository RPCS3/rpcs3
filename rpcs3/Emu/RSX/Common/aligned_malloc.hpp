#pragma once

#include <cstdlib>

namespace rsx
{
	namespace aligned_allocator
	{
		template <size_t Align>
			requires (Align != 0) && ((Align& (Align - 1)) == 0)
		size_t align_up(size_t size)
		{
			return (size + (Align - 1)) & ~(Align - 1);
		}

		template <size_t Align>
			requires (Align != 0) && ((Align& (Align - 1)) == 0)
		void* malloc(size_t size)
		{
#if defined(_WIN32)
			return _aligned_malloc(size, Align);
#elif defined(__APPLE__)
			constexpr size_t NativeAlign = std::max(Align, sizeof(void*));
			return std::aligned_alloc(NativeAlign, align_up<NativeAlign>(size));
#else
			return std::aligned_alloc(Align, align_up<Align>(size));
#endif
		}

		template <size_t Align>
			requires (Align != 0) && ((Align& (Align - 1)) == 0)
		void* realloc(void* prev_ptr, [[maybe_unused]] size_t prev_size, size_t new_size)
		{
			if (align_up<Align>(prev_size) >= new_size)
			{
				return prev_ptr;
			}

			ensure(reinterpret_cast<usz>(prev_ptr) % Align == 0, "Pointer not aligned to Align");
#if defined(_WIN32)
			return _aligned_realloc(prev_ptr, new_size, Align);
#else
#if defined(__APPLE__)
			constexpr size_t NativeAlign = std::max(Align, sizeof(void*));
			void* ret = std::aligned_alloc(NativeAlign, align_up<NativeAlign>(new_size));
#else
			void* ret = std::aligned_alloc(Align, align_up<Align>(new_size));
#endif
			std::memcpy(ret, prev_ptr, std::min(prev_size, new_size));
			std::free(prev_ptr);
			return ret;
#endif
		}

		static inline void free(void* ptr)
		{
#ifdef _WIN32
			_aligned_free(ptr);
#else
			std::free(ptr);
#endif
		}
	}

	template <typename T, int Alignment = sizeof(T)>
	class aligned_pointer_t
	{
	public:
		aligned_pointer_t(size_t size)
		{
			m_ptr = aligned_allocator::malloc<Alignment>(size);
		}

		virtual ~aligned_pointer_t()
		{
			aligned_allocator::free(m_ptr);
		}

		T* data() const { return m_ptr; }

		T& operator * () const { return *m_ptr; }

		T* operator -> () const { return m_ptr; }

	private:
		T* m_ptr;
	};
}
