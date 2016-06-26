#pragma once

#include "GCM.h"
#include "Utilities/types.h"

namespace rsx
{

struct data_array_format_info
{
private:
	u8 index;
	std::array<u32, 0x10000 / 4> &registers;
public:
	u16 frequency = 0;
	u8 stride = 0;
	u8 size = 0;
	vertex_base_type type = vertex_base_type::f;


	data_array_format_info(u8 idx, std::array<u32, 0x10000 / 4> &r) : index(idx), registers(r)
	{}

	data_array_format_info() = delete;

	void unpack_array(u32 data_array_format)
	{
		frequency = data_array_format >> 16;
		stride = (data_array_format >> 8) & 0xff;
		size = (data_array_format >> 4) & 0xf;
		type = to_vertex_base_type(data_array_format & 0xf);
	}

	u32 offset() const
	{
		return registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
	}
};

}
