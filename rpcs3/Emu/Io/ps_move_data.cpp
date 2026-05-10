#include "stdafx.h"
#include "ps_move_data.h"

const ps_move_data::vect<4> ps_move_data::default_quaternion = ps_move_data::vect<4>({ 0.0f, 0.0f, 0.0f, 1.0f });

ps_move_data::ps_move_data()
	: quaternion(default_quaternion)
{
}

void ps_move_data::reset_sensors()
{
	quaternion = default_quaternion;
	accelerometer = {};
	gyro = {};
	prev_gyro = {};
	angular_acceleration = {};
	magnetometer = {};
	//prev_pos_world = {}; // probably no reset needed ?
	vel_world = {};
	prev_vel_world = {};
	accel_world = {};
	angvel_world = {};
	angaccel_world = {};
}

ps_move_data::vect<3> ps_move_data::rotate_vector(const vect<4>& q, const vect<3>& v)
{
	const auto cross = [](const vect<3>& a, const vect<3>& b)
	{
		return vect<3>({
			a.y() * b.z() - a.z() * b.y(),
			a.z() * b.x() - a.x() * b.z(),
			a.x() * b.y() - a.y() * b.x()
		});
	};

	// q = (x, y, z, w)
	const vect<3> q_vec({q.x(), q.y(), q.z()});

	// t = 2 * cross(q_vec, v)
	const vect<3> t = cross(q_vec, v) * 2.0f;

	// v' = v + w * t + cross(q_vec, t)
	const vect<3> v_prime = v + t * q.w() + cross(q_vec, t);

	return v_prime;
}

void ps_move_data::update_orientation(f32 delta_time)
{
	if (!delta_time)
		return;

	if constexpr (use_imu_for_velocity)
	{
		// Gravity in world frame
		constexpr f32 gravity = 9.81f;
		constexpr vect<3> g({0.0f, 0.0f, -gravity});

		// Rotate gravity into sensor frame
		const vect<3> g_sensor = rotate_vector(quaternion, g);

		// Remove gravity
		vect<3> linear_local;
		for (u32 i = 0; i < 3; i++)
		{
			linear_local[i] = (accelerometer[i] * gravity) - g_sensor[i];
		}

		// Linear acceleration (rotate to world coordinates)
		accel_world = rotate_vector(quaternion, linear_local);

		// convert to mm/s²
		for (u32 i = 0; i < 3; i++)
		{
			accel_world[i] *= 1000.0f;
		}

		// Linear velocity (integrate acceleration)
		for (u32 i = 0; i < 3; i++)
		{
			vel_world[i] = prev_vel_world[i] + accel_world[i] * delta_time;
		}

		prev_vel_world = vel_world;
	}

	// Compute raw angular acceleration
	for (u32 i = 0; i < 3; i++)
	{
		const f32 alpha = (gyro[i] - prev_gyro[i]) / delta_time;

		// Filtering
		constexpr f32 weight = 0.8f;
		constexpr f32 weight_inv = 1.0f - weight;
		angular_acceleration[i] = weight * angular_acceleration[i] + weight_inv * alpha;
	}

	// Angular velocity (rotate to world coordinates)
	angvel_world = rotate_vector(quaternion, gyro);

	// Angular acceleration (rotate to world coordinates)
	angaccel_world = rotate_vector(quaternion, angular_acceleration);

	prev_gyro = gyro;
}

void ps_move_data::update_velocity(u64 timestamp, be_t<f32> pos_world[4])
{
	if constexpr (use_imu_for_velocity)
		return;

	if (last_velocity_update_time_us == timestamp)
		return;

	// Get elapsed time since last update
	const f32 delta_time = (last_velocity_update_time_us == 0) ? 0.0f : ((timestamp - last_velocity_update_time_us) / 1'000'000.0f);
	last_velocity_update_time_us = timestamp;

	if (!delta_time)
		return;

	for (u32 i = 0; i < 3; i++)
	{
		// Linear velocity
		constexpr f32 weight = 0.8f;
		constexpr f32 weight_inv = 1.0f - weight;
		vel_world[i] = weight * ((pos_world[i] - prev_pos_world[i]) / delta_time) + weight_inv * prev_vel_world[i];
		prev_pos_world[i] = pos_world[i];

		// Linear acceleration
		accel_world[i] = (vel_world[i] - prev_vel_world[i]) / delta_time;
	}

	prev_vel_world = vel_world;
}
