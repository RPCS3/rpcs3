#pragma once

#include "util/types.hpp"
#include "Utilities/geometry.h"

#include <string>

struct vertex
{
	float values[4];

	vertex() = default;

	vertex(float x, float y)
	{
		vec2(x, y);
	}

	vertex(float x, float y, float z)
	{
		vec3(x, y, z);
	}

	vertex(float x, float y, float z, float w)
	{
		vec4(x, y, z, w);
	}

	vertex(int x, int y, int z, int w)
	{
		vec4(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z), static_cast<f32>(w));
	}

	float& operator[](int index)
	{
		return values[index];
	}

	float& x()
	{
		return values[0];
	}

	float& y()
	{
		return values[1];
	}

	float& z()
	{
		return values[2];
	}

	float& w()
	{
		return values[3];
	}

	void vec2(float x, float y)
	{
		values[0] = x;
		values[1] = y;
		values[2] = 0.f;
		values[3] = 1.f;
	}

	void vec3(float x, float y, float z)
	{
		values[0] = x;
		values[1] = y;
		values[2] = z;
		values[3] = 1.f;
	}

	void vec4(float x, float y, float z, float w)
	{
		values[0] = x;
		values[1] = y;
		values[2] = z;
		values[3] = w;
	}

	void operator += (const vertex& other)
	{
		values[0] += other.values[0];
		values[1] += other.values[1];
		values[2] += other.values[2];
		values[3] += other.values[3];
	}

	void operator -= (const vertex& other)
	{
		values[0] -= other.values[0];
		values[1] -= other.values[1];
		values[2] -= other.values[2];
		values[3] -= other.values[3];
	}
};


template<typename T>
struct vector3_base : public position3_base<T>
{
	using position3_base<T>::position3_base;

	vector3_base(T x, T y, T z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	vector3_base(const position3_base<T>& other)
	{
		this->x = other.x;
		this->y = other.y;
		this->z = other.z;
	}

	T dot(const vector3_base& rhs) const
	{
		return (this->x * rhs.x) + (this->y * rhs.y) + (this->z * rhs.z);
	}

	T distance(const vector3_base& rhs) const
	{
		const vector3_base d = *this - rhs;
		return d.dot(d);
	}
};

template<typename T>
vector3_base<T> operator * (const vector3_base<T>& lhs, const vector3_base<T>& rhs)
{
	return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

template<typename T>
vector3_base<T> operator * (const vector3_base<T>& lhs, T rhs)
{
	return { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
}

template<typename T>
vector3_base<T> operator * (T lhs, const vector3_base<T>& rhs)
{
	return { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z };
}

template<typename T>
void operator *= (const vector3_base<T>& lhs, const vector3_base<T>& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	lhs.z *= rhs.z;
}

template<typename T>
void operator *= (const vector3_base<T>& lhs, T rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
}

template<typename T>
void operator < (const vector3_base<T>& lhs, T rhs)
{
	return lhs.x < rhs.x && lhs.y < rhs.y && lhs.z < rhs.z;
}

using vector3i = vector3_base<int>;
using vector3f = vector3_base<float>;

std::string utf8_to_ascii8(const std::string& utf8_string);
std::string utf16_to_ascii8(const std::u16string& utf16_string);
std::u16string ascii8_to_utf16(const std::string& ascii_string);
std::u32string utf8_to_u32string(const std::string& utf8_string);
std::u16string u32string_to_utf16(const std::u32string& utf32_string);
std::u32string utf16_to_u32string(const std::u16string& utf16_string);
