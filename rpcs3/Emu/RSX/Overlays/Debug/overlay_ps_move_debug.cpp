#include "stdafx.h"
#include "overlay_ps_move_debug.h"

namespace rsx
{
	namespace overlays
	{
		ps_move_debug_overlay::ps_move_debug_overlay()
		{
			m_frame.set_pos(0, 0);
			m_frame.set_size(300, 300);
			m_frame.back_color.r = 0.0f;
			m_frame.back_color.g = 0.0f;
			m_frame.back_color.b = 0.0f;
			m_frame.back_color.a = 1.0f;

			m_text_view.set_pos(10, 10);
			m_text_view.set_padding(0, 0, 0, 0);
			m_text_view.set_font("n023055ms.ttf", 6);
			m_text_view.align_text(overlay_element::text_align::left);
			m_text_view.fore_color = { 0.3f, 1.f, 0.3f, 1.f };
			m_text_view.back_color.a = 0.f;
		}

		compiled_resource ps_move_debug_overlay::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			auto result = m_frame.get_compiled();
			result.add(m_text_view.get_compiled());

			// Move cylinder so its center is at origin
			//glTranslatef(0, 0, 0.5f);

			// Apply quaternion rotation
			//QMatrix4x4 model;
			//model.rotate(QQuaternion(m_quaternion[3], m_quaternion[0], m_quaternion[1], m_quaternion[2]));
			//glMultMatrixf(model.constData());

			// Move back to original position
			//glTranslatef(0, 0, -0.5f);

			// Draw controller body
			//glColor3ub(200, 200, 200);
			//drawCylinder(0.2f, 0.8f, 32);

			// Draw front sphere
			//glColor3f(m_rgb[0], m_rgb[1], m_rgb[2]);
			//glPushMatrix();
			//glTranslatef(0, 0, 0.8f); // move to front
			//drawSphere(0.3f, 32, 32);
			//glPopMatrix();

			// Draw button
			//glColor3ub(0, 0, 200);
			//glPushMatrix();
			//glTranslatef(0, 0.2f, 0.4f); // slightly in front of the sphere
			//drawButton(0.08f, 0.05f, 16);
			//glPopMatrix();

			return result;
		}

		void ps_move_debug_overlay::show(const ps_move_data& md, f32 r, f32 g, f32 b, f32 q0, f32 q1, f32 q2, f32 q3)
		{
			visible = true;

			if (m_rgb[0] == r && m_rgb[1] == g && m_rgb[2] == b &&
				m_quaternion[0] == q0 && m_quaternion[1] == q1 && m_quaternion[2] == q2 && m_quaternion[3] == q3 &&
				std::memcmp(static_cast<const void*>(&md), static_cast<const void*>(&m_move_data), sizeof(ps_move_data)) == 0)
			{
				return;
			}

			m_move_data = md;
			m_rgb = { r, g, b };
			m_quaternion = { q0, q1, q2, q3 };

			m_text_view.set_text(fmt::format(
				"> Quat X: %6.2f  Gyro X: %6.2f  Accel X: %6.2f  Mag X: %6.2f\n"
				"> Quat Y: %6.2f  Gyro Y: %6.2f  Accel Y: %6.2f  Mag Y: %6.2f\n"
				"> Quat Z: %6.2f  Gyro Z: %6.2f  Accel Z: %6.2f  Mag Z: %6.2f\n"
				"> Quat W: %6.2f\n\n"
				"> World\n"
				">    Vel X: %9.2f\n"
				">    Vel Y: %9.2f\n"
				">    Vel Z: %9.2f\n"
				">    Acc X: %9.2f\n"
				">    Acc Y: %9.2f\n"
				">    Acc Z: %9.2f\n"
				"> AngVel X: %9.2f\n"
				"> AngVel Y: %9.2f\n"
				"> AngVel Z: %9.2f\n"
				"> AngAcc X: %9.2f\n"
				"> AngAcc Y: %9.2f\n"
				"> AngAcc Z: %9.2f"
				,
				m_quaternion[0], md.gyro[0], md.accelerometer[0], md.magnetometer[0],
				m_quaternion[1], md.gyro[1], md.accelerometer[1], md.magnetometer[1],
				m_quaternion[2], md.gyro[2], md.accelerometer[2], md.magnetometer[2],
				m_quaternion[3],
				md.vel_world[0],
				md.vel_world[1],
				md.vel_world[2],
				md.accel_world[0],
				md.accel_world[1],
				md.accel_world[2],
				md.angvel_world[0],
				md.angvel_world[1],
				md.angvel_world[2],
				md.angaccel_world[0],
				md.angaccel_world[1],
				md.angaccel_world[2]
			));
			m_text_view.auto_resize();

			refresh();
		}
	} // namespace overlays
} // namespace rsx
