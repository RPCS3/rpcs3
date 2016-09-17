#pragma once

#include "GCM.h"
#include "Utilities/types.h"

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
