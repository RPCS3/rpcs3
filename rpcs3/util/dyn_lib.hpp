#pragma once

#include <string>

namespace utils
{
	class dynamic_library
	{
		void* m_handle = nullptr;

	public:
		dynamic_library() = default;
		dynamic_library(const std::string& path);

		~dynamic_library();

		bool load(const std::string& path);
		void close();

	private:
		void* get_impl(const std::string& name) const;

	public:
		template <typename Type = void>
		Type* get(const std::string& name) const
		{
			Type* result;
			*reinterpret_cast<void**>(&result) = get_impl(name);
			return result;
		}

		template <typename Type>
		bool get(Type*& function, const std::string& name) const
		{
			*reinterpret_cast<void**>(&function) = get_impl(name);

			return function != nullptr;
		}

		bool loaded() const;
		explicit operator bool() const;
	};

	// (assume the lib is always loaded)
	void* get_proc_address(const char* lib, const char* name);

	template <typename F>
	struct dynamic_import
	{
		static_assert(sizeof(F) == 0, "Invalid function type");
	};

	template <typename R, typename... Args>
	struct dynamic_import<R(Args...)>
	{
		atomic_t<std::uintptr_t> ptr;
		const char* const lib;
		const char* const name;

		// Constant initialization
		constexpr dynamic_import(const char* lib, const char* name) noexcept
		    : ptr(-1)
		    , lib(lib)
		    , name(name)
		{
		}

		void init() noexcept
		{
			ptr.release(reinterpret_cast<std::uintptr_t>(get_proc_address(lib, name)));
		}

		operator bool() noexcept
		{
			if (ptr == umax) [[unlikely]]
			{
				init();
			}

			return ptr != 0;
		}

		// Caller
		R operator()(Args... args) noexcept
		{
			if (ptr == umax) [[unlikely]]
			{
				init();
			}

			return reinterpret_cast<R (*)(Args...)>(ptr.load())(args...);
		}
	};
}

#define DYNAMIC_IMPORT(lib, name, ...) inline utils::dynamic_import<__VA_ARGS__> name(lib, #name);
