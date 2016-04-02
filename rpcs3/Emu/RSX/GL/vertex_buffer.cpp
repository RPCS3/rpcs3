#include "stdafx.h"
#include "GLGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"
#include "gl_helpers.h"

namespace
{
	u32 to_gl_internal_type(rsx::vertex_base_type type, u8 size)
	{
		/**
		* The buffer texture spec only allows fetches aligned to 8, 16, 32, etc...
		* This rules out most 3-component formats, except for the 32-wide RGB32F, RGB32I, RGB32UI
		*/
		const u32 vec1_types[] = { GL_R16, GL_R32F, GL_R16F, GL_R8, GL_R16I, GL_R16, GL_R8UI };
		const u32 vec2_types[] = { GL_RG16, GL_RG32F, GL_RG16F, GL_RG8, GL_RG16I, GL_RG16, GL_RG8UI };
		const u32 vec3_types[] = { GL_RGBA16, GL_RGB32F, GL_RGBA16F, GL_RGBA8, GL_RGBA16I, GL_RGBA16, GL_RGBA8UI };	//VEC3 COMPONENTS NOT SUPPORTED!
		const u32 vec4_types[] = { GL_RGBA16, GL_RGBA32F, GL_RGBA16F, GL_RGBA8, GL_RGBA16I, GL_RGBA16, GL_RGBA8UI };

		const u32* vec_selectors[] = { 0, vec1_types, vec2_types, vec3_types, vec4_types };

		if (type > rsx::vertex_base_type::ub256)
			throw EXCEPTION("OpenGL error: unknown vertex base type 0x%X.", (u32)type);

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
		throw EXCEPTION("unknow vertex type");
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
		throw EXCEPTION("unknow vertex type");
	}

	// return vertex count and filled index array if primitive type is not native (empty array otherwise)
	std::tuple<u32, std::vector<u8>> get_index_array_for_emulated_non_indexed_draw(const std::vector<std::pair<u32, u32>> &first_count_commands, rsx::primitive_type primitive_mode)
	{
		u32 vertex_draw_count = 0;
		assert(!is_primitive_native(primitive_mode));

		for (const auto &pair : first_count_commands)
		{
			vertex_draw_count += (u32)get_index_count(primitive_mode, pair.second);
		}

		std::vector<u8> vertex_index_array(vertex_draw_count * sizeof(u16));
		u32 first = 0;
		char* mapped_buffer = (char*)vertex_index_array.data();
		for (const auto &pair : first_count_commands)
		{
			size_t element_count = get_index_count(primitive_mode, pair.second);
			write_index_array_for_non_indexed_non_native_primitive_to_buffer(mapped_buffer, primitive_mode, first, pair.second);
			mapped_buffer = (char*)mapped_buffer + element_count * sizeof(u16);
			first += pair.second;
		}

		return std::make_tuple(vertex_draw_count, vertex_index_array);
	}
}

void GLGSRender::set_vertex_buffer()
{
	//initialize vertex attributes

	//merge all vertex arrays
	std::vector<u8> vertex_arrays_data;

	const std::string reg_table[] =
	{
		"in_pos", "in_weight", "in_normal",
		"in_diff_color", "in_spec_color",
		"in_fog",
		"in_point_size", "in_7",
		"in_tc0", "in_tc1", "in_tc2", "in_tc3",
		"in_tc4", "in_tc5", "in_tc6", "in_tc7"
	};

	u32 input_mask = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK];

	std::vector<u8> vertex_index_array;
	vertex_draw_count = 0;
	u32 min_index, max_index;

	if (draw_command == rsx::draw_command::indexed)
	{
		rsx::index_array_type type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);
		u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
		for (const auto& first_count : first_count_commands)
		{
			vertex_draw_count += first_count.second;
		}
		// Index count
		vertex_draw_count = (u32)get_index_count(draw_mode, gsl::narrow<int>(vertex_draw_count));
		vertex_index_array.resize(vertex_draw_count * type_size);

		switch (type)
		{
		case rsx::index_array_type::u32:
			std::tie(min_index, max_index) = write_index_array_data_to_buffer(gsl::span<u32>((u32*)vertex_index_array.data(), vertex_draw_count), draw_mode, first_count_commands);
			break;
		case rsx::index_array_type::u16:
			std::tie(min_index, max_index) = write_index_array_data_to_buffer(gsl::span<u16>((u16*)vertex_index_array.data(), vertex_draw_count), draw_mode, first_count_commands);
			break;
		}
	}

	if (draw_command == rsx::draw_command::inlined_array)
	{
		u32 stride = 0;
		u32 offsets[rsx::limits::vertex_count] = { 0 };

		for (u32 i = 0; i < rsx::limits::vertex_count; ++i)
		{
			const auto &info = vertex_arrays_info[i];
			if (!info.size) continue;

			offsets[i] = stride;
			stride += rsx::get_vertex_type_size_on_host(info.type, info.size);
		}

		vertex_draw_count = (u32)(inline_vertex_array.size() * sizeof(u32)) / stride;

		for (int index = 0; index < rsx::limits::vertex_count; ++index)
		{
			auto &vertex_info = vertex_arrays_info[index];

			int location;
			if (!m_program->uniforms.has_location(reg_table[index] + "_buffer", &location))
				continue;

			if (!vertex_info.size) // disabled, bind a null sampler
			{
				glActiveTexture(GL_TEXTURE0 + index + rsx::limits::textures_count);
				glBindTexture(GL_TEXTURE_BUFFER, 0);
				glProgramUniform1i(m_program->id(), location, index + rsx::limits::textures_count);
				continue;
			}

			const u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
			u32 data_size = element_size * vertex_draw_count;
			u32 gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);

			auto &buffer = m_gl_attrib_buffers[index].buffer;
			auto &texture = m_gl_attrib_buffers[index].texture;

			vertex_arrays_data.resize(data_size);
			u8 *src = reinterpret_cast<u8*>(inline_vertex_array.data());
			u8 *dst = vertex_arrays_data.data();

			src += offsets[index];
			prepare_buffer_for_writing(dst, vertex_info.type, vertex_info.size, vertex_draw_count);

			//TODO: properly handle compressed data
			for (u32 i = 0; i < vertex_draw_count; ++i)
			{
				if (vertex_info.type == rsx::vertex_base_type::ub && vertex_info.size == 4)
				{
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

			buffer->data(data_size, nullptr);
			buffer->sub_data(0, data_size, vertex_arrays_data.data());

			//Attach buffer to texture
			texture->copy_from(*buffer, gl_type);

			//Link texture to uniform
			m_program->uniforms.texture(location, index + rsx::limits::textures_count, *texture);
			if (!is_primitive_native(draw_mode))
			{
				std::tie(vertex_draw_count, vertex_index_array) = get_index_array_for_emulated_non_indexed_draw({ { 0, vertex_draw_count } }, draw_mode);
			}
		}
	}

	if (draw_command == rsx::draw_command::array)
	{
		for (const auto &first_count : first_count_commands)
		{
			vertex_draw_count += first_count.second;
		}
	}

	if (draw_command == rsx::draw_command::array || draw_command == rsx::draw_command::indexed)
	{
		for (int index = 0; index < rsx::limits::vertex_count; ++index)
		{
			int location;
			if (!m_program->uniforms.has_location(reg_table[index] + "_buffer", &location))
				continue;

			bool enabled = !!(input_mask & (1 << index));
			if (!enabled)
			{
				glActiveTexture(GL_TEXTURE0 + index + rsx::limits::textures_count);
				glBindTexture(GL_TEXTURE_BUFFER, 0);
				glProgramUniform1i(m_program->id(), location, index + rsx::limits::textures_count);
				continue;
			}

			if (vertex_arrays_info[index].size > 0)
			{
				auto &vertex_info = vertex_arrays_info[index];
				// Active vertex array
				std::vector<gsl::byte> vertex_array;

				// Fill vertex_array
				u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
				vertex_array.resize(vertex_draw_count * element_size);

				// Get source pointer
				u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
				u32 offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
				u32 address = base_offset + rsx::get_address(offset & 0x7fffffff, offset >> 31);
				const gsl::byte *src_ptr = gsl::narrow_cast<const gsl::byte*>(vm::base(address));

				if (draw_command == rsx::draw_command::array)
				{
					size_t offset = 0;
					gsl::span<gsl::byte> dest_span(vertex_array);
					prepare_buffer_for_writing(vertex_array.data(), vertex_info.type, vertex_info.size, vertex_draw_count);

					for (const auto &first_count : first_count_commands)
					{
						write_vertex_array_data_to_buffer(dest_span.subspan(offset), src_ptr, first_count.first, first_count.second, vertex_info.type, vertex_info.size, vertex_info.stride);
						offset += first_count.second * element_size;
					}
				}
				if (draw_command == rsx::draw_command::indexed)
				{
					vertex_array.resize((max_index + 1) * element_size);
					gsl::span<gsl::byte> dest_span(vertex_array);
					prepare_buffer_for_writing(vertex_array.data(), vertex_info.type, vertex_info.size, vertex_draw_count);

					write_vertex_array_data_to_buffer(dest_span, src_ptr, 0, max_index + 1, vertex_info.type, vertex_info.size, vertex_info.stride);
				}

				size_t size = vertex_array.size();
				size_t position = vertex_arrays_data.size();
				vertex_arrays_data.resize(position + size);

				u32 gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);

				auto &buffer = m_gl_attrib_buffers[index].buffer;
				auto &texture = m_gl_attrib_buffers[index].texture;

				buffer->data(static_cast<u32>(size), nullptr);
				buffer->sub_data(0, static_cast<u32>(size), vertex_array.data());

				//Attach buffer to texture
				texture->copy_from(*buffer, gl_type);

				//Link texture to uniform
				m_program->uniforms.texture(location, index + rsx::limits::textures_count, *texture);
			}
			else if (register_vertex_info[index].size > 0)
			{
				//Untested!
				auto &vertex_data = register_vertex_data[index];
				auto &vertex_info = register_vertex_info[index];

				switch (vertex_info.type)
				{
				case rsx::vertex_base_type::f:
				{
					const u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
					const u32 gl_type = to_gl_internal_type(vertex_info.type, vertex_info.size);
					const size_t data_size = vertex_data.size();

					auto &buffer = m_gl_attrib_buffers[index].buffer;
					auto &texture = m_gl_attrib_buffers[index].texture;

					buffer->data(data_size, nullptr);
					buffer->sub_data(0, data_size, vertex_data.data());

					//Attach buffer to texture
					texture->copy_from(*buffer, gl_type);

					//Link texture to uniform
					m_program->uniforms.texture(location, index + rsx::limits::textures_count, *texture);
					break;
				}
				default:
					LOG_ERROR(RSX, "bad non array vertex data format (type = %d, size = %d)", vertex_info.type, vertex_info.size);
					break;
				}
			}
			else
			{
				glActiveTexture(GL_TEXTURE0 + index + rsx::limits::textures_count);
				glBindTexture(GL_TEXTURE_BUFFER, 0);
				glProgramUniform1i(m_program->id(), location, index + rsx::limits::textures_count);
				continue;
			}
		}
		if (draw_command == rsx::draw_command::array && !is_primitive_native(draw_mode))
		{
			std::tie(vertex_draw_count, vertex_index_array) = get_index_array_for_emulated_non_indexed_draw(first_count_commands, draw_mode);
		}
	}

	//	glDraw* will fail without at least attrib0 defined if we are on compatibility profile
	//	Someone should really test AMD behaviour here, Nvidia is too permissive. There is no buffer currently bound, but on NV it works ok
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, 0);

	if (draw_command == rsx::draw_command::indexed)
	{
		m_ebo.data(vertex_index_array.size(), vertex_index_array.data());
	}
	else if (!is_primitive_native(draw_mode))
	{
		m_ebo.data(vertex_index_array.size(), vertex_index_array.data());
	}
}