#pragma once

#include "common.h"

namespace rsx
{
	class fragment_texture;
	class vertex_texture;
	class sampled_image_descriptor_base;
}

namespace gl
{
	class sampler_state
	{
		GLuint sampler_handle = 0;
		std::unordered_map<GLenum, GLuint> m_propertiesi;
		std::unordered_map<GLenum, GLfloat> m_propertiesf;

	public:

		void create()
		{
			glGenSamplers(1, &sampler_handle);
		}

		void remove()
		{
			if (sampler_handle)
			{
				glDeleteSamplers(1, &sampler_handle);
			}
		}

		void bind(int index) const
		{
			glBindSampler(index, sampler_handle);
		}

		void set_parameteri(GLenum pname, GLuint value)
		{
			auto prop = m_propertiesi.find(pname);
			if (prop != m_propertiesi.end() &&
				prop->second == value)
			{
				return;
			}

			m_propertiesi[pname] = value;
			glSamplerParameteri(sampler_handle, pname, value);
		}

		void set_parameterf(GLenum pname, GLfloat value)
		{
			auto prop = m_propertiesf.find(pname);
			if (prop != m_propertiesf.end() &&
				prop->second == value)
			{
				return;
			}

			m_propertiesf[pname] = value;
			glSamplerParameterf(sampler_handle, pname, value);
		}

		GLuint get_parameteri(GLenum pname)
		{
			auto prop = m_propertiesi.find(pname);
			return (prop == m_propertiesi.end()) ? 0 : prop->second;
		}

		GLfloat get_parameterf(GLenum pname)
		{
			auto prop = m_propertiesf.find(pname);
			return (prop == m_propertiesf.end()) ? 0 : prop->second;
		}

		void apply(const rsx::fragment_texture& tex, const rsx::sampled_image_descriptor_base* sampled_image);
		void apply(const rsx::vertex_texture& tex, const rsx::sampled_image_descriptor_base* sampled_image);

		void apply_defaults(GLenum default_filter = GL_NEAREST);

		operator bool() const { return sampler_handle != GL_NONE; }
	};

	struct saved_sampler_state
	{
		GLuint saved = GL_NONE;
		GLuint unit = 0;

		saved_sampler_state(GLuint _unit, const gl::sampler_state& sampler)
		{
			glActiveTexture(GL_TEXTURE0 + _unit);
			glGetIntegerv(GL_SAMPLER_BINDING, reinterpret_cast<GLint*>(&saved));

			unit = _unit;
			sampler.bind(_unit);
		}

		saved_sampler_state(const saved_sampler_state&) = delete;

		~saved_sampler_state()
		{
			glBindSampler(unit, saved);
		}
	};
}
