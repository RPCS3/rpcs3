#pragma once

#include "GCM.h"
#include "Utilities/types.h"
#include "Utilities/BEType.h"

namespace rsx
{

struct data_array_format_info
{
private:
	u8 index;
	std::array<u32, 0x10000 / 4>& registers;

	auto decode_reg() const
	{
		const typename rsx::registers_decoder<NV4097_SET_VERTEX_DATA_ARRAY_FORMAT>::decoded_type
			   decoded_value(registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + index]);
		return decoded_value;
	}

public:
	data_array_format_info(int id, std::array<u32, 0x10000 / 4>& r)
		   : registers(r)
		   , index(id)
	{
	}

	u32 offset() const
	{
		return registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
	}

	u8 stride() const
	{
		return decode_reg().stride();
	}

	u8 size() const
	{
		return decode_reg().size();
	}

	u16 frequency() const
	{
		return decode_reg().frequency();
	}

	vertex_base_type type() const
	{
		return decode_reg().type();
	}

	void reset()
	{
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + index] = 0x2;
	}
};

struct push_buffer_vertex_info
{
	u8 size = 0;
	vertex_base_type type = vertex_base_type::f;

	u32 vertex_count = 0;
	u32 attribute_mask = ~0;
	std::vector<u32> data;

	void clear()
	{
		data.resize(0);
		attribute_mask = ~0;
		vertex_count = 0;
		size = 0;
	}

	u8 get_vertex_size_in_dwords(vertex_base_type type)
	{
		//NOTE: Types are always provided to fit into 32-bits
		//i.e no less than 4 8-bit values and no less than 2 16-bit values

		switch (type)
		{
		case vertex_base_type::f:
			return size;
		case vertex_base_type::ub:
		case vertex_base_type::ub256:
			return 1;
		case vertex_base_type::s1:
		case vertex_base_type::s32k:
			return size / 2;
		default:
			fmt::throw_exception("Unsupported vertex base type %d", (u8)type);
		}
	}

	void append_vertex_data(u32 sub_index, vertex_base_type type, u32 arg)
	{
		const u32 element_mask = (1 << sub_index);
		const u8  vertex_size = get_vertex_size_in_dwords(type);

		if (attribute_mask & element_mask)
		{
			attribute_mask = 0;

			vertex_count++;
			data.resize(vertex_count * vertex_size);
		}

		attribute_mask |= element_mask;

		void* dst = data.data() + ((vertex_count - 1) * vertex_size) + sub_index;
		
		//NOTE: Endianness on wide types is converted to BE here because unified upload code assumes input in BE
		//TODO: Implement fast LE source inputs and remove the byteswap
		switch (type)
		{
		case vertex_base_type::f:
			*(u32*)dst = se_storage<u32>::swap(arg);
			break;
		case vertex_base_type::ub:
		case vertex_base_type::ub256:
			*(u32*)dst = arg;
			break;
		case vertex_base_type::s1:
		case vertex_base_type::s32k:
			((u16*)dst)[0] = se_storage<u16>::swap((u16)(arg & 0xffff));
			((u16*)dst)[1] = se_storage<u16>::swap((u16)(arg >> 16));
			break;
		default:
			fmt::throw_exception("Unsupported vertex base type %d", (u8)type);
		}
	}
};

struct register_vertex_data_info
{
	u16 frequency = 0;
	u8 stride = 0;
	u8 size = 0;
	vertex_base_type type = vertex_base_type::f;

	register_vertex_data_info() {}
	std::array<u32, 4> data;

};

}
