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

		dynamic_library(const dynamic_library&) = delete;

		dynamic_library(dynamic_library&& other) noexcept
			: m_handle(other.m_handle)
		{
			other.m_handle = nullptr;
		}

		dynamic_library& operator=(const dynamic_library&) = delete;

		dynamic_library& operator=(dynamic_library&& other) noexcept
		{
			std::swap(m_handle, other.m_handle);
			return *this;
		}

		~dynamic_library();

		bool load(const std::string& path);
#ifdef _WIN32
		bool load(const std::wstring& path);
#endif
		void close();

	private:
		void* get_impl(const char* name) const;

	public:
		template <typename Type = void>
		Type get(const char* name) const
		{
			return reinterpret_cast<Type>(get_impl(name));
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
		atomic_t<uptr> ptr;
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
			ptr.release(reinterpret_cast<uptr>(get_proc_address(lib, name)));
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

#define DYNAMIC_IMPORT(lib, name, ...) inline constinit utils::dynamic_import<__VA_ARGS__> name(lib, #name);
#define DYNAMIC_IMPORT_RENAME(lib, declare_name, lib_func_name, ...) inline constinit utils::dynamic_import<__VA_ARGS__> declare_name(lib, lib_func_name);
