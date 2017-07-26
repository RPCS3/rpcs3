#include "stdafx.h"
#include "GLGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"
#include "GLHelpers.h"

namespace
{
	static constexpr std::array<const char*, 16> s_reg_table =
	{
		"in_pos_buffer", "in_weight_buffer", "in_normal_buffer",
		"in_diff_color_buffer", "in_spec_color_buffer",
		"in_fog_buffer",
		"in_point_size_buffer", "in_7_buffer",
		"in_tc0_buffer", "in_tc1_buffer", "in_tc2_buffer", "in_tc3_buffer",
		"in_tc4_buffer", "in_tc5_buffer", "in_tc6_buffer", "in_tc7_buffer"
	};
}

namespace
{
	u32 to_gl_internal_type(rsx::vertex_base_type type, u8 size)
	{
		/**
		* NOTE 1. The buffer texture spec only allows fetches aligned to 8, 16, 32, etc...
		* This rules out most 3-component formats, except for the 32-wide RGB32F, RGB32I, RGB32UI
		*
		* NOTE 2. While s1 & cmp types are signed normalized 16-bit integers, some GPU vendors dont support texture buffer access
		* using these formats. Pass a 16 bit unnormalized integer and convert it in the vertex shader
		*/
		const u32 vec1_types[] = { GL_R16I, GL_R32F, GL_R16F, GL_R8, GL_R16I, GL_RGBA16I, GL_R8UI };
		const u32 vec2_types[] = { GL_RG16I, GL_RG32F, GL_RG16F, GL_RG8, GL_RG16I, GL_RGBA16I, GL_RG8UI };
		const u32 vec3_types[] = { GL_RGBA16I, GL_RGB32F, GL_RGBA16F, GL_RGBA8, GL_RGBA16I, GL_RGBA16I, GL_RGBA8UI };	//VEC3 COMPONENTS NOT SUPPORTED!
		const u32 vec4_types[] = { GL_RGBA16I, GL_RGBA32F, GL_RGBA16F, GL_RGBA8, GL_RGBA16I, GL_RGBA16I, GL_RGBA8UI };

		const u32* vec_selectors[] = { 0, vec1_types, vec2_types, vec3_types, vec4_types };

		if (type > rsx::vertex_base_type::ub256)
			fmt::throw_exception("OpenGL error: unknown vertex base type 0x%x" HERE, (u32)type);

		return vec_selectors[size][(int)type];
	}

	void prepare_buffer_for_writing(void *data, rsx::vertex_base_type type, u8 vertex_size, u32 vertex_count)
	{
		switch (type)
		{
		case rsx::vertex_base_type::sf:
		{
			if (vertex_size == 3)
			{
				/**
				* Pad the 4th component for half-float arrays to 1, since texelfetch does not mask components
				*/
				u16 *dst = reinterpret_cast<u16*>(data);
				for (u32 i = 0, idx = 3; i < vertex_count; ++i, idx += 4)
					dst[idx] = 0x3c00;
			}

			break;
		}
		}
	}

	template<typename T, int count>
	struct apply_attrib_t;

	template<typename T>
	struct apply_attrib_t<T, 1>
	{
		static void func(gl::glsl::program& program, int location, const T* data)
		{
			program.attribs[location] = data[0];
		}
	};

	template<typename T>
	struct apply_attrib_t<T, 2>
	{
		static void func(gl::glsl::program& program, int location, const T* data)
		{
			program.attribs[location] = color2_base<T>{ data[0], data[1] };
		}
	};

	template<typename T>
	struct apply_attrib_t<T, 3>
	{
		static void func(gl::glsl::program& program, int location, const T* data)
		{
			program.attribs[location] = color3_base<T>{ data[0], data[1], data[2] };
		}
	};

	template<typename T>
	struct apply_attrib_t<T, 4>
	{
		static void func(gl::glsl::program& program, int location, const T* data)
		{
			program.attribs[location] = color4_base<T>{ data[0], data[1], data[2], data[3] };
		}
	};


	template<typename T, int count>
	void apply_attrib_array(gl::glsl::program& program, int location, const std::vector<u8>& data)
	{
		for (size_t offset = 0; offset < data.size(); offset += count * sizeof(T))
		{
			apply_attrib_t<T, count>::func(program, location, (T*)(data.data() + offset));
		}
	}

	gl::buffer_pointer::type gl_types(rsx::vertex_base_type type)
	{
		switch (type)
		{
		case rsx::vertex_base_type::s1: return gl::buffer_pointer::type::s16;
		case rsx::vertex_base_type::f: return gl::buffer_pointer::type::f32;
		case rsx::vertex_base_type::sf: return gl::buffer_pointer::type::f16;
		case rsx::vertex_base_type::ub: return gl::buffer_pointer::type::u8;
		case rsx::vertex_base_type::s32k: return gl::buffer_pointer::type::s32;
		case rsx::vertex_base_type::cmp: return gl::buffer_pointer::type::s16; // Needs conversion
		case rsx::vertex_base_type::ub256: gl::buffer_pointer::type::u8;
		}
		fmt::throw_exception("unknown vertex type" HERE);
	}

	bool gl_normalized(rsx::vertex_base_type type)
	{
		switch (type)
		{
		case rsx::vertex_base_type::s1:
		case rsx::vertex_base_type::ub:
		case rsx::vertex_base_type::cmp:
			return true;
		case rsx::vertex_base_type::f:
		case rsx::vertex_base_type::sf:
		case rsx::vertex_base_type::ub256:
		case rsx::vertex_base_type::s32k:
			return false;
		}
		fmt::throw_exception("unknown vertex type" HERE);
	}

	// return vertex count if primitive type is not native (empty array otherwise)
	std::tuple<u32, u32> get_index_array_for_emulated_non_indexed_draw(const std::vector<std::pair<u32, u32>> &first_count_commands, rsx::primitive_type primitive_mode, gl::ring_buffer &dst)
	{
		//This is an emulated buffer, so our indices only range from 0->original_vertex_array_length
		u32 vertex_count = 0;
		u32 element_count = 0;
		verify(HERE), !gl::is_primitive_native(primitive_mode);

		for (const auto &pair : first_count_commands)
		{
			element_count += (u32)get_index_count(primitive_mode, pair.second);
			vertex_count += pair.second;
		}

		auto mapping = dst.alloc_from_heap(element_count * sizeof(u16), 256);
		char *mapped_buffer = (char *)mapping.first;

		write_index_array_for_non_indexed_non_native_primitive_to_buffer(mapped_buffer, primitive_mode, vertex_count);
		return std::make_tuple(element_count, mapping.second);
	}

	std::tuple<u32, u32, u32> upload_index_buffer(gsl::span<const gsl::byte> raw_index_buffer, void *ptr, rsx::index_array_type type, rsx::primitive_type draw_mode, const std::vector<std::pair<u32, u32>> first_count_commands, u32 initial_vertex_count)
	{
		u32 min_index, max_index, vertex_draw_count = initial_vertex_count;

		if (!gl::is_primitive_native(draw_mode))
			vertex_draw_count = (u32)get_index_count(draw_mode, ::narrow<int>(vertex_draw_count));

		u32 type_size = ::narrow<u32>(get_index_type_size(type));
		u32 block_sz = vertex_draw_count * type_size;

		gsl::span<gsl::byte> dst{ reinterpret_cast<gsl::byte*>(ptr), ::narrow<u32>(block_sz) };
		std::tie(min_index, max_index) = write_index_array_data_to_buffer(dst, raw_index_buffer,
			type, draw_mode, rsx::method_registers.restart_index_enabled(), rsx::method_registers.restart_index(), first_count_commands,
			[](auto prim) { return !gl::is_primitive_native(prim); });

		return std::make_tuple(min_index, max_index, vertex_draw_count);
	}
}

namespace
{
	GLenum get_index_type(rsx::index_array_type type)
	{
		switch (type)
		{
		case rsx::index_array_type::u16: return GL_UNSIGNED_SHORT;
		case rsx::index_array_type::u32: return GL_UNSIGNED_INT;
		}
		throw;
	}


	struct vertex_buffer_visitor
	{
		vertex_buffer_visitor(u32 vtx_cnt, gl::ring_buffer& heap, gl::glsl::program* prog, gl::texture* attrib_buffer, u32 min_texbuffer_offset)
		    : vertex_count(vtx_cnt)
		    , m_attrib_ring_info(heap)
		    , m_program(prog)
		    , m_gl_attrib_buffers(attrib_buffer)
		    , m_min_texbuffer_alignment(min_texbuffer_offset)
		{
		}

		void operator()(const rsx::vertex_array_buffer& vertex_array)
		{
			int location;
			if (!m_program->uniforms.has_location(s_reg_table[vertex_array.index], &location))
				return;

			// Fill vertex_array
			u32 element_size = rsx::get_vertex_type_size_on_host(vertex_array.type, vertex_array.attribute_size);

			u32 data_size = vertex_count * element_size;
			u32 gl_type   = to_gl_internal_type(vertex_array.type, vertex_array.attribute_size);
			auto& texture = m_gl_attrib_buffers[vertex_array.index];

			u32 buffer_offset = 0;
			auto mapping      = m_attrib_ring_info.alloc_from_heap(data_size, m_min_texbuffer_alignment);
			gsl::byte* dst    = static_cast<gsl::byte*>(mapping.first);
			buffer_offset     = mapping.second;
			gsl::span<gsl::byte> dest_span(dst, data_size);

			write_vertex_array_data_to_buffer(dest_span, vertex_array.data, vertex_count, vertex_array.type, vertex_array.attribute_size, vertex_array.stride, rsx::get_vertex_type_size_on_host(vertex_array.type, vertex_array.attribute_size));
			prepare_buffer_for_writing(dst, vertex_array.type, vertex_array.attribute_size, vertex_count);

			texture.copy_from(m_attrib_ring_info, gl_type, buffer_offset, data_size);
		}

		void operator()(const rsx::vertex_array_register& vertex_register)
		{
			int location;
			if (!m_program->uniforms.has_location(s_reg_table[vertex_register.index], &location))
				return;

			const u32 element_size = rsx::get_vertex_type_size_on_host(vertex_register.type, vertex_register.attribute_size);
			const u32 gl_type   = to_gl_internal_type(vertex_register.type, vertex_register.attribute_size);
			const u32 data_size = element_size;

			auto& texture = m_gl_attrib_buffers[vertex_register.index];

			auto mapping = m_attrib_ring_info.alloc_from_heap(data_size, m_min_texbuffer_alignment);
			u8 *dst = static_cast<u8*>(mapping.first);

			memcpy(dst, vertex_register.data.data(), element_size);
			prepare_buffer_for_writing(dst, vertex_register.type, vertex_register.attribute_size, vertex_count);

			texture.copy_from(m_attrib_ring_info, gl_type, mapping.second, data_size);
		}

		void operator()(const rsx::empty_vertex_array& vbo)
		{
		}

	protected:
		u32 vertex_count;
		gl::ring_buffer& m_attrib_ring_info;
		gl::glsl::program* m_program;
		gl::texture* m_gl_attrib_buffers;
		GLint m_min_texbuffer_alignment;
	};

	struct draw_command_visitor
	{
		using attribute_storage = std::vector<
		    std::variant<rsx::vertex_array_buffer, rsx::vertex_array_register, rsx::empty_vertex_array>>;

		draw_command_visitor(gl::ring_buffer& index_ring_buffer, gl::ring_buffer& attrib_ring_buffer,
		    gl::texture* gl_attrib_buffers, gl::glsl::program* program, GLint min_texbuffer_alignment,
		    std::function<attribute_storage(rsx::rsx_state, std::vector<std::pair<u32, u32>>)> gvb)
		    : m_index_ring_buffer(index_ring_buffer)
		    , m_attrib_ring_buffer(attrib_ring_buffer)
		    , m_gl_attrib_buffers(gl_attrib_buffers)
		    , m_program(program)
		    , m_min_texbuffer_alignment(min_texbuffer_alignment)
		    , get_vertex_buffers(gvb)
		{
			for (u8 index = 0; index < rsx::limits::vertex_count; ++index) {
				if (rsx::method_registers.vertex_arrays_info[index].size() ||
				    rsx::method_registers.register_vertex_info[index].size)
				{
					max_vertex_attrib_size += 16;
				}
			}
		}

		std::tuple<u32, std::optional<std::tuple<GLenum, u32>>> operator()(
		    const rsx::draw_array_command& command)
		{
			u32 vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			u32 min_index    = rsx::method_registers.current_draw_clause.first_count_commands.front().first;
			u32 max_index    = vertex_count - 1 + min_index;

			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive)) {
				u32 index_count;
				u32 offset_in_index_buffer;
				std::tie(index_count, offset_in_index_buffer) = get_index_array_for_emulated_non_indexed_draw(
				    rsx::method_registers.current_draw_clause.first_count_commands,
				    rsx::method_registers.current_draw_clause.primitive, m_index_ring_buffer);

				upload_vertex_buffers(min_index, max_index, max_vertex_attrib_size);

				return std::make_tuple(index_count,
				    std::make_tuple(static_cast<GLenum>(GL_UNSIGNED_SHORT), offset_in_index_buffer));
			}

			upload_vertex_buffers(min_index, max_index, max_vertex_attrib_size);

			return std::make_tuple(vertex_count, std::optional<std::tuple<GLenum, u32>>());
		}

		std::tuple<u32, std::optional<std::tuple<GLenum, u32>>> operator()(
		    const rsx::draw_indexed_array_command& command)
		{
			u32 min_index = 0, max_index = 0;

			rsx::index_array_type type = rsx::method_registers.current_draw_clause.is_immediate_draw?
				rsx::index_array_type::u32:
				rsx::method_registers.index_type();
			
			u32 type_size              = ::narrow<u32>(get_index_type_size(type));

			u32 vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			u32 index_count = vertex_count;
			
			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
				index_count = (u32)get_index_count(rsx::method_registers.current_draw_clause.primitive, vertex_count);

			u32 max_size               = index_count * type_size;
			auto mapping               = m_index_ring_buffer.alloc_from_heap(max_size, 256);
			void* ptr                  = mapping.first;
			u32 offset_in_index_buffer = mapping.second;

			std::tie(min_index, max_index, index_count) = upload_index_buffer(
			    command.raw_index_buffer, ptr, type, rsx::method_registers.current_draw_clause.primitive,
			    rsx::method_registers.current_draw_clause.first_count_commands, vertex_count);
			
			upload_vertex_buffers(0, max_index, max_vertex_attrib_size);

			return std::make_tuple(index_count, std::make_tuple(get_index_type(type), offset_in_index_buffer));
		}

		std::tuple<u32, std::optional<std::tuple<GLenum, u32>>> operator()(
		    const rsx::draw_inlined_array& command)
		{
			// We need to go through array to determine vertex count so upload it
			u32 vertex_count = upload_inline_array(max_vertex_attrib_size);

			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive)) {
				u32 offset_in_index_buffer;
				u32 index_count;
				std::tie(index_count, offset_in_index_buffer) = get_index_array_for_emulated_non_indexed_draw(
					{ std::make_pair(0, vertex_count) },
				    rsx::method_registers.current_draw_clause.primitive, m_index_ring_buffer);
				return std::make_tuple(index_count,
				    std::make_tuple(static_cast<GLenum>(GL_UNSIGNED_SHORT), offset_in_index_buffer));
			}
			return std::make_tuple(vertex_count, std::optional<std::tuple<GLenum, u32>>());
		}

	private:
		u32 max_vertex_attrib_size = 0;
		gl::ring_buffer& m_index_ring_buffer;
		gl::ring_buffer& m_attrib_ring_buffer;
		gl::texture* m_gl_attrib_buffers;

		gl::glsl::program* m_program;
		GLint m_min_texbuffer_alignment;
		std::function<attribute_storage(rsx::rsx_state, std::vector<std::pair<u32, u32>>)>
		    get_vertex_buffers;

		void upload_vertex_buffers(u32 min_index, u32 max_index, const u32& max_vertex_attrib_size)
		{
			u32 verts_allocated = max_index - min_index + 1;

			vertex_buffer_visitor visitor(verts_allocated, m_attrib_ring_buffer,
			    m_program, m_gl_attrib_buffers, m_min_texbuffer_alignment);
			const auto& vertex_buffers =
			    get_vertex_buffers(rsx::method_registers, {{min_index, verts_allocated}});
			for (const auto& vbo : vertex_buffers) std::apply_visitor(visitor, vbo);
		}

		u32 upload_inline_array(const u32& max_vertex_attrib_size)
		{
			u32 stride                             = 0;
			u32 offsets[rsx::limits::vertex_count] = {0};

			for (u32 i = 0; i < rsx::limits::vertex_count; ++i) {
				const auto& info = rsx::method_registers.vertex_arrays_info[i];
				if (!info.size()) continue;

				offsets[i] = stride;
				stride += rsx::get_vertex_type_size_on_host(info.type(), info.size());
			}

			u32 vertex_draw_count =
			    (u32)(rsx::method_registers.current_draw_clause.inline_vertex_array.size() * sizeof(u32)) /
			    stride;

			for (int index = 0; index < rsx::limits::vertex_count; ++index) {
				auto& vertex_info = rsx::method_registers.vertex_arrays_info[index];

				int location;
				if (!m_program->uniforms.has_location(s_reg_table[index], &location)) continue;

				if (!vertex_info.size())
					continue;

				const u32 element_size =
				    rsx::get_vertex_type_size_on_host(vertex_info.type(), vertex_info.size());
				u32 data_size = element_size * vertex_draw_count;
				u32 gl_type   = to_gl_internal_type(vertex_info.type(), vertex_info.size());

				auto& texture = m_gl_attrib_buffers[index];

				u8* src =
				    reinterpret_cast<u8*>(rsx::method_registers.current_draw_clause.inline_vertex_array.data());
				auto mapping = m_attrib_ring_buffer.alloc_from_heap(data_size, m_min_texbuffer_alignment);
				u8* dst      = static_cast<u8*>(mapping.first);

				src += offsets[index];
				prepare_buffer_for_writing(dst, vertex_info.type(), vertex_info.size(), vertex_draw_count);

				// TODO: properly handle compressed data
				for (u32 i = 0; i < vertex_draw_count; ++i) {
					if (vertex_info.type() == rsx::vertex_base_type::ub && vertex_info.size() == 4) {
						dst[0] = src[3];
						dst[1] = src[2];
						dst[2] = src[1];
						dst[3] = src[0];
					}
					else
						memcpy(dst, src, element_size);

					src += stride;
					dst += element_size;
				}

				texture.copy_from(m_attrib_ring_buffer, gl_type, mapping.second, data_size);
			}
			return vertex_draw_count;
		}
	};
}

std::tuple<u32, std::optional<std::tuple<GLenum, u32>>> GLGSRender::set_vertex_buffer()
{
	std::chrono::time_point<steady_clock> then = steady_clock::now();
	auto result = std::apply_visitor(draw_command_visitor(*m_index_ring_buffer, *m_attrib_ring_buffer,
	                              m_gl_attrib_buffers, m_program, m_min_texbuffer_alignment,
	                              [this](const auto& state, const auto& list) {
		                              return this->get_vertex_buffers(state, list, 0);
		                             }),
	    get_draw_command(rsx::method_registers));

	std::chrono::time_point<steady_clock> now = steady_clock::now();
	m_vertex_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
	return result;
}

namespace
{
} // End anonymous namespace
