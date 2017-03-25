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
	u8 size;
	vertex_base_type type;

	u32 vertex_count = 0;
	u32 attribute_mask = ~0;
	std::vector<u32> data;

	void clear()
	{
		data.resize(0);
		attribute_mask = ~0;
		vertex_count = 0;
	}

	void append_vertex_data(u32 sub_index, u32 arg)
	{
		const u32 element_mask = (1 << sub_index);
		if (attribute_mask & element_mask)
		{
			attribute_mask = 0;

			vertex_count++;
			data.resize(vertex_count * size);
		}

		attribute_mask |= element_mask;
		u32* dst = data.data() + ((vertex_count - 1) * size) + sub_index;
		*dst = se_storage<u32>::swap(arg);
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
