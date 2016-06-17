#include <string>

namespace utils
{
	class dynamic_library
	{
		void *m_handle = nullptr;

	public:
		dynamic_library() = default;
		dynamic_library(const std::string &path);

		~dynamic_library();

		bool load(const std::string &path);
		void close();

	private:
		void *get_impl(const std::string &name) const;

	public:
		template<typename Type = void>
		Type *get(const std::string &name) const
		{
			Type *result;
			*(void **)(&result) = get_impl(name);
			return result;
		}

		template<typename Type>
		bool get(Type *&function, const std::string &name) const
		{
			*(void **)(&function) = get_impl(name);

			return !!function;
		}

		bool loaded() const;
		explicit operator bool() const;
	};
}
