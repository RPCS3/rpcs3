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

void ps_move_data::update_orientation(f32 delta_time)
{
	if (!delta_time)
		return;

	// Rotate vector v by quaternion q
	const auto rotate_vector = [](const vect<4>& q, const vect<3>& v)
	{
		const vect<4> qv({0.0f, v.x(), v.y(), v.z()});
		const vect<4> q_inv({q.w(), -q.x(), -q.y(), -q.z()});

		// t = q * v
		vect<4> t;
		t.w() = -q.x() * qv.x() - q.y() * qv.y() - q.z() * qv.z();
		t.x() =  q.w() * qv.x() + q.y() * qv.z() - q.z() * qv.y();
		t.y() =  q.w() * qv.y() - q.x() * qv.z() + q.z() * qv.x();
		t.z() =  q.w() * qv.z() + q.x() * qv.y() - q.y() * qv.x();

		// r = t * q_inv
		vect<4> r;
		r.w() = -t.x() * q_inv.x() - t.y() * q_inv.y() - t.z() * q_inv.z();
		r.x() =  t.w() * q_inv.x() + t.y() * q_inv.z() - t.z() * q_inv.y();
		r.y() =  t.w() * q_inv.y() - t.x() * q_inv.z() + t.z() * q_inv.x();
		r.z() =  t.w() * q_inv.z() + t.x() * q_inv.y() - t.y() * q_inv.x();

		return vect<3>({r.x(), r.y(), r.z()});
	};

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
