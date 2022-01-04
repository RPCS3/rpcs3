#pragma once

#include "gcm_enums.h"
#include "rsx_decode.h"
#include "Common/simple_array.hpp"
#include "util/types.hpp"

namespace rsx
{

struct data_array_format_info
{
private:
	u8 index;
	std::array<u32, 0x10000 / 4>& registers;

	auto decode_reg() const
	{
		const rsx::registers_decoder<NV4097_SET_VERTEX_DATA_ARRAY_FORMAT>::decoded_type
			   decoded_value(registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + index]);
		return decoded_value;
	}

public:
	data_array_format_info(int id, std::array<u32, 0x10000 / 4>& r)
		: index(id)
		, registers(r)
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

struct push_buffer_vertex_info
{
	u32 attr = 0;
	u32 size = 0;
	vertex_base_type type = vertex_base_type::f;

	u32 vertex_count = 0;
	u32 dword_count = 0;
	rsx::simple_array<u32> data;

	push_buffer_vertex_info() = default;
	~push_buffer_vertex_info() = default;

	u8 get_vertex_size_in_dwords() const;
	u32 get_vertex_id() const;

	void clear();
	void set_vertex_data(u32 attribute_id, u32 vertex_id, u32 sub_index, vertex_base_type type, u32 size, u32 arg);
	void pad_to(u32 required_vertex_count, bool skip_last);
};

struct register_vertex_data_info
{
	u16 frequency = 0;
	u8 stride = 0;
	u8 size = 0;
	vertex_base_type type = vertex_base_type::f;

	register_vertex_data_info() = default;
	std::array<u32, 4> data{};
};

}
