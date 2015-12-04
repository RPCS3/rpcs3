#include <iostream>
#include "geometry_types.h"
#include <utility>

namespace rpcs3
{
	struct serializer_slot
	{
	public:
		template<typename Type>
		std::enable_if_t<!std::is_polymorphic<Type>::value> entry(const Type& element)
		{
		}
	};

	struct serializable
	{
		void serialize() const
		{
			std::cout << "Ok! ";
		}

		std::string to_string() const
		{
			std::cout << "Ok! ";
			return "serializable";
		}

		using serialize_default = void;
	};


#define HAS_METHOD_DECLARE_BASE(name, defaultSign) \
	template <typename Type, typename FuncSign = defaultSign> \
	class has_##name##_method \
	{ \
		using yes = char; \
		using no = yes[2]; \
 \
		template<typename Type, Type> struct test_sign {}; \
		template<typename Type> static yes &test(test_sign<FuncSign, &Type::name> *); \
		template<typename> static no &test(...); \
 \
	public: \
		static constexpr bool value = sizeof(test<Type>(nullptr)) == sizeof(yes); \
	} \

#define HAS_METHOD_DECLARE(Name, ReturnType, Modifers, ...) HAS_METHOD_DECLARE_BASE(Name, ReturnType(Type::*)(__VA_ARGS__)Modifers)

	HAS_METHOD_DECLARE(serialize, void, const);
	HAS_METHOD_DECLARE(deserialize, void, const);
	HAS_METHOD_DECLARE(to_string, std::string, const);

	template<typename Type>
	std::enable_if_t<!has_to_string_method<Type>::value && has_serialize_method<Type>::value> serialize(const Type &obj)
	{
		//static_assert(false, "serialize");
	}

	template<typename Type>
	std::enable_if_t<has_to_string_method<Type>::value && !has_serialize_method<Type>::value> serialize(const Type &obj)
	{
		//static_assert(false, "to_string");
	}

	template<typename Type>
	std::enable_if_t<has_to_string_method<Type>::value && has_serialize_method<Type>::value> serialize(const Type &obj)
	{
		//static_assert(false, "to_string & serialize");
	}

	template<typename Type>
	std::enable_if_t<!has_to_string_method<Type>::value && !has_serialize_method<Type>::value && !std::is_polymorphic<Type>::value> serialize(const Type &obj)
	{
		//static_assert(false, "not polymorphic");
	}

	void test_func()
	{
		std::cout << "Done! ";
	}
}