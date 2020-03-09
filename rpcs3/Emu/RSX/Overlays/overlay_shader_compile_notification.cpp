#include "stdafx.h"
#include "overlay_shader_compile_notification.h"
#include "Emu/system_config.h"

namespace rsx
{
	namespace overlays
	{
		shader_compile_notification::shader_compile_notification()
		{
			const u16 pos_x = g_cfg.video.shader_compilation_hint.pos_x;
			const u16 pos_y = g_cfg.video.shader_compilation_hint.pos_y;

			m_text.set_font("Arial", 16);
			m_text.set_text("Compiling shaders");
			m_text.auto_resize();
			m_text.set_pos(pos_x, pos_y);

			m_text.back_color.a = 0.f;

			for (int n = 0; n < 3; ++n)
			{
				dots[n].set_size(2, 2);
				dots[n].back_color = color4f(1.f, 1.f, 1.f, 1.f);
				dots[n].set_pos(m_text.w + pos_x + 5 + (6 * n), pos_y + 20);
			}

			creation_time = get_system_time();
			expire_time   = creation_time + 1000000;

			// Disable forced refresh unless fps dips below 4
			min_refresh_duration_us = 250000;
			visible = true;
		}

		void shader_compile_notification::update_animation(u64 t)
		{
			// Update rate is twice per second
			auto elapsed = t - creation_time;
			elapsed /= 500000;

			auto old_dot = current_dot;
			current_dot  = elapsed % 3;

			if (old_dot != current_dot)
			{
				if (old_dot != 255)
				{
					dots[old_dot].set_size(2, 2);
					dots[old_dot].translate(0, 1);
				}

				dots[current_dot].translate(0, -1);
				dots[current_dot].set_size(3, 3);
			}
		}

		// Extends visible time by half a second. Also updates the screen
		void shader_compile_notification::touch()
		{
			if (urgency_ctr == 0 || urgency_ctr > 8)
			{
				refresh();
				urgency_ctr = 0;
			}

			expire_time = get_system_time() + 500000;
			urgency_ctr++;
		}

		void shader_compile_notification::update()
		{
			auto current_time = get_system_time();
			if (current_time > expire_time)
				close(false, false);

			update_animation(current_time);

			// Usually this method is called during a draw-to-screen operation. Reset urgency ctr
			urgency_ctr = 1;
		}

		compiled_resource shader_compile_notification::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			auto compiled = m_text.get_compiled();
			compiled.add(dots[0].get_compiled());
			compiled.add(dots[1].get_compiled());
			compiled.add(dots[2].get_compiled());

			return compiled;
		}
	} // namespace overlays
} // namespace rsx
