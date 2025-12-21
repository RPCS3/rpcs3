#pragma once

struct ps_move_data
{
	template <int Size, typename T = f32>
	struct vect
	{
	public:
		constexpr vect() = default;
		constexpr vect(const std::array<T, Size>& vec) : data(vec) {};

		template <typename I>
		T& operator[](I i) { return data[i]; }

		template <typename I>
		const T& operator[](I i) const { return data[i]; }

		T x() const requires (Size >= 1) { return data[0]; }
		T y() const requires (Size >= 2) { return data[1]; }
		T z() const requires (Size >= 3) { return data[2]; }
		T w() const requires (Size >= 4) { return data[3]; }

		T& x() requires (Size >= 1) { return data[0]; }
		T& y() requires (Size >= 2) { return data[1]; }
		T& z() requires (Size >= 3) { return data[2]; }
		T& w() requires (Size >= 4) { return data[3]; }

	private:
		std::array<T, Size> data {};
	};

	ps_move_data();

	u32 external_device_id = 0;
	std::array<u8, 38> external_device_read{};  // CELL_GEM_EXTERNAL_PORT_DEVICE_INFO_SIZE
	std::array<u8, 40> external_device_write{}; // CELL_GEM_EXTERNAL_PORT_OUTPUT_SIZE
	std::array<u8, 5> external_device_data{};
	bool external_device_connected = false;
	bool external_device_read_requested = false;
	bool external_device_write_requested = false;

	bool calibration_requested = false;
	bool calibration_succeeded = false;

	bool magnetometer_enabled = false;
	bool orientation_enabled = false;

	// Disable IMU tracking of velocity and acceleration (massive drift)
	static constexpr bool use_imu_for_velocity = false;
	u64 last_velocity_update_time_us = 0;

	static const vect<4> default_quaternion;
	vect<4> quaternion {}; // quaternion orientation (x,y,z,w) of controller relative to default (facing the camera with buttons up)

	// Raw values (local)
	vect<3> accelerometer {}; // linear acceleration in G units (9.81 = 1 unit)
	vect<3> gyro {}; // angular velocity in rad/s
	vect<3> prev_gyro {}; // previous angular velocity in rad/s
	vect<3> angular_acceleration {}; // angular acceleration in rad/s²
	vect<3> magnetometer {};

	// In world coordinates
	vect<3> prev_pos_world {};
	vect<3> vel_world {};      // velocity of sphere in world coordinates (mm/s)
	vect<3> prev_vel_world {}; // previous velocity of sphere in world coordinates (mm/s)
	vect<3> accel_world {};    // acceleration of sphere in world coordinates (mm/s²)
	vect<3> angvel_world {};   // angular velocity of controller in world coordinates (radians/s)
	vect<3> angaccel_world {}; // angular acceleration of controller in world coordinates (radians/s²)

	s16 temperature = 0;

	void reset_sensors();
	void update_orientation(f32 delta_time);
	void update_velocity(u64 timestamp, be_t<f32> pos_world[4]);
};
