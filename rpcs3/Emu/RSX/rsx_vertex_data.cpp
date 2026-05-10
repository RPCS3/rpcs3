#include "stdafx.h"
#include "rsx_vertex_data.h"
#include "rsx_methods.h"

namespace rsx
{
	void push_buffer_vertex_info::clear()
	{
		if (size)
		{
			data.clear();
			vertex_count = 0;
			dword_count = 0;
			size = 0;
		}
	}

	u8 push_buffer_vertex_info::get_vertex_size_in_dwords() const
	{
		// NOTE: Types are always provided to fit into 32-bits
		// i.e no less than 4 8-bit values and no less than 2 16-bit values

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
			fmt::throw_exception("Unsupported vertex base type %d", static_cast<u8>(type));
		}
	}

	u32 push_buffer_vertex_info::get_vertex_id() const
	{
		ensure(attr == 0);    // Only ask ATTR0 for vertex ID

		// Which is the current vertex ID to be written to?
		// NOTE: Fully writing to ATTR0 closes the current block
		return size ? (dword_count / get_vertex_size_in_dwords()) : 0;
	}

	void push_buffer_vertex_info::set_vertex_data(u32 attribute_id, u32 vertex_id, u32 sub_index, vertex_base_type type, u32 size, u32 arg)
	{
		if (vertex_count && (type != this->type || size != this->size))
		{
			// TODO: Should forcefully break the draw call on this step using an execution barrier.
			// While RSX can handle this behavior without problem, it can only be the product of nonsensical game design.
			rsx_log.error("Vertex attribute %u was respecced mid-draw (type = %d vs %d, size = %u vs %u). Indexed execution barrier required. Report this to developers.",
				attribute_id, static_cast<int>(type), static_cast<int>(this->type), size, this->size);
		}

		this->type = type;
		this->size = size;
		this->attr = attribute_id;

		const auto required_vertex_count = (vertex_id + 1);
		const auto vertex_size = get_vertex_size_in_dwords();

		if (vertex_count != required_vertex_count)
		{
			pad_to(required_vertex_count, true);
			ensure(vertex_count == required_vertex_count);
		}

		auto current_vertex = data.data() + ((vertex_count - 1) * vertex_size);
		current_vertex[sub_index] = arg;
		++dword_count;
	}

	void push_buffer_vertex_info::pad_to(u32 required_vertex_count, bool skip_last)
	{
		if (vertex_count >= required_vertex_count)
		{
			return;
		}

		const auto vertex_size = get_vertex_size_in_dwords();
		data.resize(vertex_size * required_vertex_count);

		// For all previous verts, copy over the register contents duplicated over the stream.
		// Internally it appears RSX actually executes the draw commands as they are encountered.
		// You can change register data contents mid-way for example and it will pick up for the next N draws.
		// This is how immediate mode is implemented internally.
		u32* src = rsx::method_registers.register_vertex_info[attr].data.data();
		u32* dst = data.data() + (vertex_count * vertex_size);
		u32* end = data.data() + ((required_vertex_count - (skip_last ? 1 : 0)) * vertex_size);

		while (dst < end)
		{
			std::memcpy(dst, src, vertex_size * sizeof(u32));
			dst += vertex_size;
		}

		vertex_count = required_vertex_count;
	}
}
