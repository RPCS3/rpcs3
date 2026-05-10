#pragma once

#include "geometry.h"
#include "Emu/system_config_types.h"
#include "Utilities/StrUtil.h"
#include <map>
#include <mutex>

struct stereo_config
{
public:
	struct stereo_matrices
	{
		stereo_matrices() = default;
		stereo_matrices(const std::array<color3_base<f32>, 3>& l, const std::array<color3_base<f32>, 3>& r);

		mat3f left {};
		mat3f right {};
	};

	stereo_config(bool in_emulation) : m_in_emulation(in_emulation) {}

	void set_stereo_mode(stereo_render_mode_options mode) { m_stereo_mode = mode; }
	stereo_render_mode_options stereo_mode() const { return m_stereo_mode; }

	const stereo_matrices& matrices() const;

	void update_from_config(bool stereo_enabled);

	void set_custom_matrices(const stereo_matrices& matrices) { m_custom_matrices = matrices; }
	static void set_live_matrices(const stereo_matrices& matrices) { m_live_matrices = matrices; }

	std::map<std::string, std::string> get_custom_matrix(bool is_left) const;

	template <typename T>
	static void convert_matrix(mat3f& mat, const T& values)
	{
		mat[0] = {};
		mat[1] = {};
		mat[2] = {};

		if (values.empty())
		{
			return;
		}

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				const std::string key = fmt::format("%d%d", i, j);
				const auto it = values.find(key);
				if (it == values.cend()) continue;

				const std::string& val_s = it->second;
				if (val_s.empty()) continue;

				if (f64 val = 0.0f; try_to_float(&val, val_s, -10.0f, 10.0f))
				{
					mat[i].rgb[j] = static_cast<f32>(val);
				}
			}
		}
	}

	static std::map<std::string, std::string> convert_matrix(const mat3f& mat);

	static atomic_t<bool> s_live_preview_enabled;

private:
	stereo_matrices m_custom_matrices {};
	stereo_render_mode_options m_stereo_mode = stereo_render_mode_options::disabled;
	bool m_in_emulation = false;

	static stereo_matrices m_live_matrices;
	static const std::unordered_map<stereo_render_mode_options, stereo_matrices> m_matrices;
};
