#include "stdafx.h"
#include "ps_move_calibration.h"

LOG_CHANNEL(move_log, "Move");

// This is basically the same as in ps move api

static inline s32 psmove_calibration_decode_16bit_unsigned(const u8* data, u32 offset)
{
	const u8 low = data[offset] & 0xFF;
	const u8 high = (data[offset + 1]) & 0xFF;
	return (low | (high << 8)) - zero_shift;
}

static inline s16 psmove_calibration_decode_16bit_signed(const u8* data, u32 offset)
{
	const u16 low = data[offset] & 0xFF;
	const u16 high = (data[offset + 1]) & 0xFF;
	return static_cast<s16>(low | (high << 8));
}

static inline u32 psmove_calibration_decode_12bits(const u8* data, u32 offset)
{
	const u8 low = data[offset] & 0xFF;
	const u8 high = (data[offset + 1]) & 0xFF;
	return low | (high << 8);
}

static inline f32 psmove_calibration_decode_float(const u8* data, u32 offset)
{
	union
	{
		uint32_t u32;
		float f;
	} v {
		.u32 = static_cast<u32>( (data[offset]     & 0xFF) |
		                        ((data[offset + 1] & 0xFF) << 8) |
		                        ((data[offset + 2] & 0xFF) << 16) |
		                        ((data[offset + 3] & 0xFF) << 24))
	};

	return v.f;
}

static void psmove_dump_calibration(const reports::ps_move_calibration_blob& calibration, const ps_move_device& device)
{
	int t, x, y, z;
	float fx, fy, fz;

	const u8* data = calibration.data.data();

	std::string msg;

	switch (device.model)
	{
	case ps_move_model::ZCM1:
		t = psmove_calibration_decode_12bits(data, 0x02);
		fmt::append(msg, "Temperature: 0x%04X\n", t);
		for (int orientation = 0; orientation < 6; orientation++)
		{
			x = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * orientation);
			y = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * orientation + 2);
			z = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * orientation + 4);
			fmt::append(msg, "Orientation #%d:      (%5d | %5d | %5d)\n", orientation, x, y, z);
		}

		t = psmove_calibration_decode_12bits(data, 0x42);
		fmt::append(msg, "Temperature: 0x%04X\n", t);
		for (int orientation = 0; orientation < 3; orientation++)
		{
			x = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8 * orientation);
			y = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8 * orientation + 2);
			z = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8 * orientation + 4);
			fmt::append(msg, "Gyro %c, 80 rpm:      (%5d | %5d | %5d)\n", "XYZ"[orientation], x, y, z);
		}

		t = psmove_calibration_decode_12bits(data, 0x28);
		x = psmove_calibration_decode_16bit_unsigned(data, 0x2a);
		y = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 2);
		z = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 4);
		fmt::append(msg, "Temperature: 0x%04X\n", t);
		fmt::append(msg, "Gyro, 0 rpm (@0x2a): (%5d | %5d | %5d)\n", x, y, z);

		t = psmove_calibration_decode_12bits(data, 0x30);
		x = psmove_calibration_decode_16bit_unsigned(data, 0x32);
		y = psmove_calibration_decode_16bit_unsigned(data, 0x32 + 2);
		z = psmove_calibration_decode_16bit_unsigned(data, 0x32 + 4);
		fmt::append(msg, "Temperature: 0x%04X\n", t);
		fmt::append(msg, "Gyro, 0 rpm (@0x32): (%5d | %5d | %5d)\n", x, y, z);

		t = psmove_calibration_decode_12bits(data, 0x5c);
		fx = psmove_calibration_decode_float(data, 0x5e);
		fy = psmove_calibration_decode_float(data, 0x5e + 4);
		fz = psmove_calibration_decode_float(data, 0x5e + 8);
		fmt::append(msg, "Temperature: 0x%04X\n", t);
		fmt::append(msg, "Vector @0x5e: (%f | %f | %f)\n", fx, fy, fz);

		fx = psmove_calibration_decode_float(data, 0x6a);
		fy = psmove_calibration_decode_float(data, 0x6a + 4);
		fz = psmove_calibration_decode_float(data, 0x6a + 8);
		fmt::append(msg, "Vector @0x6a: (%f | %f | %f)\n", fx, fy, fz);

		fmt::append(msg, "byte @0x3f:  0x%02x\n", static_cast<u8>(data[0x3f]));
		fmt::append(msg, "float @0x76: %f\n", psmove_calibration_decode_float(data, 0x76));
		fmt::append(msg, "float @0x7a: %f\n", psmove_calibration_decode_float(data, 0x7a));
		break;
	case ps_move_model::ZCM2:
		for (int orientation = 0; orientation < 6; orientation++)
		{
			x = psmove_calibration_decode_16bit_signed(data, 0x04 + 6 * orientation);
			y = psmove_calibration_decode_16bit_signed(data, 0x04 + 6 * orientation + 2);
			z = psmove_calibration_decode_16bit_signed(data, 0x04 + 6 * orientation + 4);
			fmt::append(msg, "Orientation #%d:      (%5d | %5d | %5d)\n", orientation, x, y, z);
		}

		x = psmove_calibration_decode_16bit_signed(data, 0x26);
		y = psmove_calibration_decode_16bit_signed(data, 0x26 + 2);
		z = psmove_calibration_decode_16bit_signed(data, 0x26 + 4);
		fmt::append(msg, "Gyro Bias?, 0 rpm (@0x26): (%5d | %5d | %5d)\n", x, y, z);

		for (int orientation = 0; orientation < 6; orientation++)
		{
			x = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * orientation);
			y = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * orientation + 2);
			z = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * orientation + 4);
			fmt::append(msg, "Gyro %c, 90 rpm:      (%5d | %5d | %5d)\n", "XYZXYZ"[orientation], x, y, z);
		}
		break;
	}

	move_log.notice("Calibration:\n%s", msg);
}

void psmove_calibration_get_usb_accel_values(const reports::ps_move_calibration_blob& calibration, ps_move_device& device)
{
	const u8* data = calibration.data.data();

	int x1 = 0, x2 = 0, y1 = 0, y2 = 0, z1 = 0, z2 = 0;

	switch (device.model)
	{
	case ps_move_model::ZCM1:
		// Minimum (negative) value of each axis
		x1 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * 1);
		y1 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * 5 + 2);
		z1 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * 2 + 4);

		// Maximum (positive) value of each axis
		x2 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * 3);
		y2 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * 4 + 2);
		z2 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6 * 0 + 4);
		break;
	case ps_move_model::ZCM2:
		// Minimum (negative) value of each axis
		x1 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6 * 1);
		y1 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6 * 3 + 2);
		z1 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6 * 5 + 4);

		// Maximum (positive) value of each axis
		x2 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6 * 0);
		y2 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6 * 2 + 2);
		z2 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6 * 4 + 4);
		break;
	}

	device.calibration.accel_x_factor = 2.0f / static_cast<float>(x2 - x1);
	device.calibration.accel_y_factor = 2.0f / static_cast<float>(y2 - y1);
	device.calibration.accel_z_factor = 2.0f / static_cast<float>(z2 - z1);

	device.calibration.accel_x_offset = -(device.calibration.accel_x_factor * static_cast<float>(x1)) - 1.0f;
	device.calibration.accel_y_offset = -(device.calibration.accel_y_factor * static_cast<float>(x1)) - 1.0f;
	device.calibration.accel_z_offset = -(device.calibration.accel_z_factor * static_cast<float>(x1)) - 1.0f;
}

void psmove_calibration_get_usb_gyro_values(const reports::ps_move_calibration_blob& calibration, ps_move_device& device)
{
	const u8* data = calibration.data.data();

	constexpr f32 PI = 3.14159265f;
	constexpr f32 rpm_to_rad_per_sec = (2.0f * PI) / 60.0f;

	switch (device.model)
	{
	case ps_move_model::ZCM1:
	{
		const int bx = psmove_calibration_decode_16bit_unsigned(data, 0x2a);
		const int by = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 2);
		const int bz = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 4);

		const int x = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8 * 0) - bx;
		const int y = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8 * 1 + 2) - by;
		const int z = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8 * 2 + 4) - bz;

		constexpr f32 calibration_rpm = 80.0f;
		constexpr f32 factor = calibration_rpm * rpm_to_rad_per_sec;

		// Per frame drift taken into account using adjusted gain values
		device.calibration.gyro_x_gain = factor / static_cast<float>(x);
		device.calibration.gyro_y_gain = factor / static_cast<float>(y);
		device.calibration.gyro_z_gain = factor / static_cast<float>(z);
		device.calibration.gyro_x_offset = 0;
		device.calibration.gyro_y_offset = 0;
		device.calibration.gyro_z_offset = 0;
		break;
	}
	case ps_move_model::ZCM2:
	{
		// Minimum (negative) value of each axis
		const int x1 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * 3);
		const int y1 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * 4 + 2);
		const int z1 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * 5 + 4);

		// Maximum (positive) values of each axis
		const int x2 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * 0);
		const int y2 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * 1 + 2);
		const int z2 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6 * 2 + 4);

		const int dx = psmove_calibration_decode_16bit_signed(data, 0x26);
		const int dy = psmove_calibration_decode_16bit_signed(data, 0x26 + 2);
		const int dz = psmove_calibration_decode_16bit_signed(data, 0x26 + 4);

		constexpr f32 calibration_rpm = 90.0f;
		constexpr f32 calibration_hi = calibration_rpm * rpm_to_rad_per_sec;
		constexpr f32 calibration_low = -calibration_rpm * rpm_to_rad_per_sec;
		constexpr f32 factor = calibration_hi - calibration_low;

		// Compute the gain value (the slope of the gyro reading/angular speed line)
		device.calibration.gyro_x_gain = factor / static_cast<f32>(x2 - x1);
		device.calibration.gyro_y_gain = factor / static_cast<f32>(y2 - y1);
		device.calibration.gyro_z_gain = factor / static_cast<f32>(z2 - z1);
		device.calibration.gyro_x_offset = static_cast<f32>(dx);
		device.calibration.gyro_y_offset = static_cast<f32>(dy);
		device.calibration.gyro_z_offset = static_cast<f32>(dz);
		break;
	}
	}
}

void psmove_parse_calibration(const reports::ps_move_calibration_blob& calibration, ps_move_device& device)
{
	psmove_dump_calibration(calibration, device);
	psmove_calibration_get_usb_accel_values(calibration, device);
	psmove_calibration_get_usb_gyro_values(calibration, device);
}
