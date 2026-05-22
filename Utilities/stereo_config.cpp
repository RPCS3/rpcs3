#include "stdafx.h"
#include "stereo_config.h"
#include "Emu/system_config.h"

stereo_config::stereo_matrices::stereo_matrices(const std::array<color3_base<f32>, 3>& l, const std::array<color3_base<f32>, 3>& r)
{
	for (usz i = 0; i < 3; i++)
	{
		left[i] = l[i];
		right[i] = r[i];
	}
}

atomic_t<bool> stereo_config::s_live_preview_enabled = false;
stereo_config::stereo_matrices stereo_config::m_live_matrices = {};

const std::unordered_map<stereo_render_mode_options, stereo_config::stereo_matrices> stereo_config::m_matrices =
{
	{stereo_render_mode_options::anaglyph_red_cyan, stereo_matrices(
		{
			color3_base<f32>(1, 0, 0),
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 0, 0)
		},
		{
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 1, 0),
			color3_base<f32>(0, 0, 1)
		}
	)},
	{stereo_render_mode_options::anaglyph_red_green, stereo_matrices(
		{
			color3_base<f32>(1, 0, 0),
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 0, 0)
		},
		{
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 1, 0),
			color3_base<f32>(0, 0, 0)
		}
	)},
	{stereo_render_mode_options::anaglyph_red_blue, stereo_matrices(
		{
			color3_base<f32>(1, 0, 0),
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 0, 0)
		},
		{
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 0, 1)
		}
	)},
	{stereo_render_mode_options::anaglyph_magenta_cyan, stereo_matrices(
		{
			color3_base<f32>(1, 0, 0),
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 0, 0.5f)
		},
		{
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 1, 0),
			color3_base<f32>(0, 0, 0.5f)
		}
	)},
	{stereo_render_mode_options::anaglyph_trioscopic, stereo_matrices(
		{
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 1, 0),
			color3_base<f32>(0, 0, 0)
		},
		{
			color3_base<f32>(1, 0, 0),
			color3_base<f32>(0, 0, 0),
			color3_base<f32>(0, 0, 1)
		}
	)},
	{stereo_render_mode_options::anaglyph_amber_blue, stereo_matrices(
		{
			color3_base<f32>(1, 0, 0),
			color3_base<f32>(0, 1, 0),
			color3_base<f32>(0, 0, 0)
		},
		{
			color3_base<f32>(0, 0, 1.0f / 3.0f),
			color3_base<f32>(0, 0, 1.0f / 3.0f),
			color3_base<f32>(0, 0, 1.0f / 3.0f)
		}
	)},
};

void stereo_config::update_from_config(bool stereo_enabled)
{
	m_stereo_mode = stereo_enabled ? g_cfg.video.stereo_render_mode.get() : stereo_render_mode_options::disabled;

	if (m_stereo_mode == stereo_render_mode_options::anaglyph_custom)
	{
		stereo_config::convert_matrix(m_custom_matrices.left, g_cfg.video.custom_anaglyph_matrices.left.get_map());
		stereo_config::convert_matrix(m_custom_matrices.right, g_cfg.video.custom_anaglyph_matrices.right.get_map());
	}
}

const stereo_config::stereo_matrices& stereo_config::matrices() const
{
	if (m_in_emulation && s_live_preview_enabled)
	{
		return m_live_matrices;
	}

	switch (m_stereo_mode)
	{
	case stereo_render_mode_options::disabled:
	case stereo_render_mode_options::side_by_side:
	case stereo_render_mode_options::over_under:
	case stereo_render_mode_options::interlaced:
		break;
	case stereo_render_mode_options::anaglyph_red_green:
	case stereo_render_mode_options::anaglyph_red_blue:
	case stereo_render_mode_options::anaglyph_red_cyan:
	case stereo_render_mode_options::anaglyph_magenta_cyan:
	case stereo_render_mode_options::anaglyph_trioscopic:
	case stereo_render_mode_options::anaglyph_amber_blue:
		return ::at32(m_matrices, m_stereo_mode);
	case stereo_render_mode_options::anaglyph_custom:
		return m_custom_matrices;
	}

	static const stereo_matrices s_left_only_matrices = stereo_matrices(
		{
			color3_base<float>(1, 0, 0),
			color3_base<float>(0, 1, 0),
			color3_base<float>(0, 0, 1)
		},
		{
			color3_base<float>(0, 0, 0),
			color3_base<float>(0, 0, 0),
			color3_base<float>(0, 0, 0)
		});
	return s_left_only_matrices;
}

std::map<std::string, std::string> stereo_config::get_custom_matrix(bool is_left) const
{
	return convert_matrix(is_left ? m_custom_matrices.left : m_custom_matrices.right);
}

std::map<std::string, std::string> stereo_config::convert_matrix(const mat3f& mat)
{
	std::map<std::string, std::string> values {};
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			values[fmt::format("%d%d", i, j)] = fmt::format("%f", mat[i].rgb[j]);
		}
	}
	return values;
}
