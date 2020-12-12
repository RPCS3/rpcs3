#pragma once

#include "stdafx.h"
#include "GLHelpers.h"
#include "../Common/TextGlyphs.h"

namespace gl
{
	class text_writer
	{
	private:

		gl::glsl::program m_program;
		gl::glsl::shader m_vs;
		gl::glsl::shader m_fs;

		gl::vao m_vao;
		gl::buffer m_text_buffer;
		gl::buffer m_scale_offsets_buffer;
		std::unordered_map<u8, std::pair<u32, u32>> m_offsets;

		bool initialized = false;
		bool enabled = false;

		void init_program()
		{
			std::string vs =
			{
				"#version 420\n"
				"#extension GL_ARB_shader_draw_parameters: enable\n"
				"layout(location=0) in vec2 pos;\n"
				"uniform vec2 offsets[255];\n"
				"uniform vec2 scale;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 offset = offsets[gl_DrawIDARB];\n"
				"	gl_Position = vec4(pos, 0., 1.);\n"
				"	gl_Position.xy = gl_Position.xy * scale + offset;\n"
				"}\n"
			};

			std::string fs =
			{
				"#version 420\n"
				"layout(location=0) out vec4 col0;\n"
				"uniform vec4 draw_color;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	col0 = draw_color;\n"
				"}\n"
			};

			m_fs.create(::glsl::program_domain::glsl_fragment_program, fs);
			m_fs.compile();

			m_vs.create(::glsl::program_domain::glsl_vertex_program, vs);
			m_vs.compile();

			m_program.create();
			m_program.attach(m_vs);
			m_program.attach(m_fs);
			m_program.link();
		}

		void load_program(float scale_x, float scale_y, float *offsets, size_t nb_offsets, color4f color)
		{
			float scale[] = { scale_x, scale_y };

			m_program.use();

			m_program.uniforms["draw_color"] = color;
			glProgramUniform2fv(m_program.id(), m_program.uniforms["offsets"].location(), static_cast<GLsizei>(nb_offsets), offsets);
			glProgramUniform2fv(m_program.id(), m_program.uniforms["scale"].location(), 1, scale);
		}

	public:

		text_writer() = default;
		~text_writer() = default;

		void init()
		{
			m_text_buffer.create();
			m_scale_offsets_buffer.create();

			GlyphManager glyph_source;
			auto points = glyph_source.generate_point_map();

			const size_t buffer_size = points.size() * sizeof(GlyphManager::glyph_point);

			m_text_buffer.data(buffer_size, points.data());
			m_offsets = glyph_source.get_glyph_offsets();

			m_scale_offsets_buffer.data(512 * 4 * sizeof(float));

			//Init VAO
			int old_vao;
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

			m_vao.create();
			m_vao.bind();

			//This is saved as part of the bound VAO's state
			m_vao.array_buffer = m_text_buffer;
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(GlyphManager::glyph_point), 0);

			glBindVertexArray(old_vao);

			init_program();
			initialized = true;
		}

		void set_enabled(bool state)
		{
			enabled = state;
		}

		void print_text(int x, int y, int target_w, int target_h, const std::string &text, color4f color = { 0.3f, 1.f, 0.3f, 1.f })
		{
			if (!enabled) return;

			ensure(initialized);

			std::vector<GLint> offsets;
			std::vector<GLsizei> counts;
			std::vector<float> shader_offsets;
			char *s = const_cast<char *>(text.c_str());

			//Y is in raster coordinates: convert to bottom-left origin
			y = (target_h - y - 16);

			//Compress [0, w] and [0, h] into range [-1, 1]
			float scale_x = 2.f / target_w;
			float scale_y = 2.f / target_h;

			float base_offset = 0.f;
			shader_offsets.reserve(text.length() * 2);

			while (u8 offset = static_cast<u8>(*s))
			{
				bool to_draw = false;	//Can be false for space or unsupported characters

				auto o = m_offsets.find(offset);
				if (o != m_offsets.end())
				{
					if (o->second.second > 0)
					{
						to_draw = true;
						offsets.push_back(o->second.first);
						counts.push_back(o->second.second);
					}
				}

				if (to_draw)
				{
					//Generate a scale_offset pair for this entry
					float offset_x = scale_x * (x + base_offset);
					offset_x -= 1.f;

					float offset_y = scale_y * y;
					offset_y -= 1.f;

					shader_offsets.push_back(offset_x);
					shader_offsets.push_back(offset_y);
				}

				base_offset += 9.f;
				s++;
			}

			//TODO: Add drop shadow if deemed necessary for visibility

			int old_vao;
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

			load_program(scale_x, scale_y, shader_offsets.data(), counts.size(), color);

			m_vao.bind();

			glMultiDrawArrays(GL_POINTS, offsets.data(), counts.data(), static_cast<GLsizei>(counts.size()));
			glBindVertexArray(old_vao);
		}

		void close()
		{
			if (initialized)
			{
				m_scale_offsets_buffer.remove();
				m_text_buffer.remove();
				m_vao.remove();

				m_program.remove();
				m_fs.remove();
				m_vs.remove();

				initialized = false;
			}
		}
	};
}
