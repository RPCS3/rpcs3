#pragma once

#include "overlay_controls.h"

namespace rsx::overlays
{
	template <int Anchor, int Base, int Edge0, int Edge1>
		requires (Anchor >= 0 && Anchor < 4 && Base >= 0 && Base < 4 && Edge0 >= 0 && Edge0 < 4 && Edge1 >= 0 && Edge1 < 4)
	struct arrow_base : public overlay_element
	{
		compiled_resource& get_compiled() override
		{
			if (is_compiled())
			{
				return compiled_resources;
			}

			overlay_element::get_compiled();

			m_vertex_cache.resize(3);
			m_vertex_cache[0] = compiled_resources.draw_commands[0].verts[Anchor];
			m_vertex_cache[1] = compiled_resources.draw_commands[0].verts[Base];
			m_vertex_cache[2] = compiled_resources.draw_commands[0].verts[Edge0] + compiled_resources.draw_commands[0].verts[Edge1];
			m_vertex_cache[2] /= 2;
			compiled_resources.draw_commands[0].verts.swap(m_vertex_cache);
			compiled_resources.draw_commands[0].config.primitives = overlays::primitive_type::triangle_strip;

			m_is_compiled = true;
			return compiled_resources;
		}

	private:
		std::vector<vertex> m_vertex_cache;
	};

	struct arrow_up : public arrow_base<2, 3, 0, 1>
	{};

	struct arrow_down : public arrow_base<0, 1, 2, 3>
	{};

	struct arrow_left : public arrow_base<1, 2, 3, 0>
	{};

	struct arrow_right : public arrow_base<3, 0, 1, 2>
	{};
}
