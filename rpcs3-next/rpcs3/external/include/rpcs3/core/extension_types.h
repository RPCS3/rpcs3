#pragma once
#include "basic_types.h"

namespace rpcs3
{
	inline namespace core
	{
		union alignas(2) f16
		{
			u16 _u16;
			u8 _u8[2];

			f16() = default;
			f16(f32 value);

			operator f32() const;

		private:
			static f16 make(u16 raw);
			static f16 f32_to_f16(f32 value);
			static f32 f16_to_f32(f16 value);
		};

		enum class endianes_type
		{
			little,
			big
		};

		static constexpr endianes_type native_endianess_type = endianes_type::little;

		template<typename Type>
		struct native_endianess_storage
		{
		private:
			Type m_data;

		public:
		};

		template<typename Type>
		struct swapped_endianess_storage
		{
		private:
			Type m_data;

		public:
		};

		using le_endianess_storage = native_endianess_type == endianes_type::little ? native_endianess_storage : swapped_endianess_storage;

		template<typename Type, template<typename> typename StorageType>
		class endianess
		{

		};
	}
}