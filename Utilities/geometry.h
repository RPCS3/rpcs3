#pragma once

#include <cmath>

template<typename T>
struct size2_base
{
	T width, height;

	constexpr size2_base() : width{}, height{}
	{
	}

	constexpr size2_base(T width, T height) : width{ width }, height{ height }
	{
	}

	constexpr size2_base(const size2_base& rhs) : width{ rhs.width }, height{ rhs.height }
	{
	}

	constexpr size2_base operator -(const size2_base& rhs) const
	{
		return{ width - rhs.width, height - rhs.height };
	}
	constexpr size2_base operator -(T rhs) const
	{
		return{ width - rhs, height - rhs };
	}
	constexpr size2_base operator +(const size2_base& rhs) const
	{
		return{ width + rhs.width, height + rhs.height };
	}
	constexpr size2_base operator +(T rhs) const
	{
		return{ width + rhs, height + rhs };
	}
	constexpr size2_base operator /(const size2_base& rhs) const
	{
		return{ width / rhs.width, height / rhs.height };
	}
	constexpr size2_base operator /(T rhs) const
	{
		return{ width / rhs, height / rhs };
	}
	constexpr size2_base operator *(const size2_base& rhs) const
	{
		return{ width * rhs.width, height * rhs.height };
	}
	constexpr size2_base operator *(T rhs) const
	{
		return{ width * rhs, height * rhs };
	}

	size2_base& operator -=(const size2_base& rhs)
	{
		width -= rhs.width;
		height -= rhs.height;
		return *this;
	}
	size2_base& operator -=(T rhs)
	{
		width -= rhs;
		height -= rhs;
		return *this;
	}
	size2_base& operator +=(const size2_base& rhs)
	{
		width += rhs.width;
		height += rhs.height;
		return *this;
	}
	size2_base& operator +=(T rhs)
	{
		width += rhs;
		height += rhs;
		return *this;
	}
	size2_base& operator /=(const size2_base& rhs)
	{
		width /= rhs.width;
		height /= rhs.height;
		return *this;
	}
	size2_base& operator /=(T rhs)
	{
		width /= rhs;
		height /= rhs;
		return *this;
	}
	size2_base& operator *=(const size2_base& rhs)
	{
		width *= rhs.width;
		height *= rhs.height;
		return *this;
	}
	size2_base& operator *=(T rhs)
	{
		width *= rhs;
		height *= rhs;
		return *this;
	}

	constexpr bool operator == (const size2_base& rhs) const
	{
		return width == rhs.width && height == rhs.height;
	}

	constexpr bool operator != (const size2_base& rhs) const
	{
		return width != rhs.width || height != rhs.height;
	}

	template<typename NT>
	constexpr operator size2_base<NT>() const
	{
		return{ (NT)width, (NT)height };
	}
};

template<typename T>
struct position1_base
{
	T x;

	position1_base operator -(const position1_base& rhs) const
	{
		return{ x - rhs.x };
	}
	position1_base operator -(T rhs) const
	{
		return{ x - rhs };
	}
	position1_base operator +(const position1_base& rhs) const
	{
		return{ x + rhs.x };
	}
	position1_base operator +(T rhs) const
	{
		return{ x + rhs };
	}
	template<typename RhsT>
	position1_base operator *(RhsT rhs) const
	{
		return{ T(x * rhs) };
	}
	position1_base operator *(const position1_base& rhs) const
	{
		return{ T(x * rhs.x) };
	}
	template<typename RhsT>
	position1_base operator /(RhsT rhs) const
	{
		return{ x / rhs };
	}
	position1_base operator /(const position1_base& rhs) const
	{
		return{ x / rhs.x };
	}

	position1_base& operator -=(const position1_base& rhs)
	{
		x -= rhs.x;
		return *this;
	}
	position1_base& operator -=(T rhs)
	{
		x -= rhs;
		return *this;
	}
	position1_base& operator +=(const position1_base& rhs)
	{
		x += rhs.x;
		return *this;
	}
	position1_base& operator +=(T rhs)
	{
		x += rhs;
		return *this;
	}

	template<typename RhsT>
	position1_base& operator *=(RhsT rhs) const
	{
		x *= rhs;
		return *this;
	}
	position1_base& operator *=(const position1_base& rhs) const
	{
		x *= rhs.x;
		return *this;
	}
	template<typename RhsT>
	position1_base& operator /=(RhsT rhs) const
	{
		x /= rhs;
		return *this;
	}
	position1_base& operator /=(const position1_base& rhs) const
	{
		x /= rhs.x;
		return *this;
	}

	bool operator ==(const position1_base& rhs) const
	{
		return x == rhs.x;
	}

	bool operator ==(T rhs) const
	{
		return x == rhs;
	}

	bool operator !=(const position1_base& rhs) const
	{
		return !(*this == rhs);
	}

	bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	operator position1_base<NT>() const
	{
		return{ (NT)x };
	}

	double distance(const position1_base& to)
	{
		return abs(x - to.x);
	}
};

template<typename T>
struct position2_base
{
	T x, y;

	constexpr position2_base() : x{}, y{}
	{
	}

	constexpr position2_base(T x, T y) : x{ x }, y{ y }
	{
	}

	constexpr position2_base(const position2_base& rhs) : x{ rhs.x }, y{ rhs.y }
	{
	}

	constexpr bool operator >(const position2_base& rhs) const
	{
		return x > rhs.x && y > rhs.y;
	}
	constexpr bool operator >(T rhs) const
	{
		return x > rhs && y > rhs;
	}
	constexpr bool operator <(const position2_base& rhs) const
	{
		return x < rhs.x && y < rhs.y;
	}
	constexpr bool operator <(T rhs) const
	{
		return x < rhs && y < rhs;
	}
	constexpr bool operator >=(const position2_base& rhs) const
	{
		return x >= rhs.x && y >= rhs.y;
	}
	constexpr bool operator >=(T rhs) const
	{
		return x >= rhs && y >= rhs;
	}
	constexpr bool operator <=(const position2_base& rhs) const
	{
		return x <= rhs.x && y <= rhs.y;
	}
	constexpr bool operator <=(T rhs) const
	{
		return x <= rhs && y <= rhs;
	}

	constexpr position2_base operator -(const position2_base& rhs) const
	{
		return{ x - rhs.x, y - rhs.y };
	}
	constexpr position2_base operator -(T rhs) const
	{
		return{ x - rhs, y - rhs };
	}
	constexpr position2_base operator +(const position2_base& rhs) const
	{
		return{ x + rhs.x, y + rhs.y };
	}
	constexpr position2_base operator +(T rhs) const
	{
		return{ x + rhs, y + rhs };
	}
	template<typename RhsT>
	constexpr position2_base operator *(RhsT rhs) const
	{
		return{ T(x * rhs), T(y * rhs) };
	}
	constexpr position2_base operator *(const position2_base& rhs) const
	{
		return{ T(x * rhs.x),  T(y * rhs.y) };
	}
	template<typename RhsT>
	constexpr position2_base operator /(RhsT rhs) const
	{
		return{ x / rhs, y / rhs };
	}
	constexpr position2_base operator /(const position2_base& rhs) const
	{
		return{ x / rhs.x, y / rhs.y };
	}
	constexpr position2_base operator /(const size2_base<T>& rhs) const
	{
		return{ x / rhs.width, y / rhs.height };
	}

	position2_base& operator -=(const position2_base& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	position2_base& operator -=(T rhs)
	{
		x -= rhs;
		y -= rhs;
		return *this;
	}
	position2_base& operator +=(const position2_base& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	position2_base& operator +=(T rhs)
	{
		x += rhs;
		y += rhs;
		return *this;
	}

	template<typename RhsT>
	position2_base& operator *=(RhsT rhs)
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}
	position2_base& operator *=(const position2_base& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}
	template<typename RhsT>
	position2_base& operator /=(RhsT rhs)
	{
		x /= rhs;
		y /= rhs;
		return *this;
	}
	position2_base& operator /=(const position2_base& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}

	constexpr bool operator ==(const position2_base& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

	constexpr bool operator ==(T rhs) const
	{
		return x == rhs && y == rhs;
	}

	constexpr bool operator !=(const position2_base& rhs) const
	{
		return !(*this == rhs);
	}

	constexpr bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator position2_base<NT>() const
	{
		return{ (NT)x, (NT)y };
	}

	double distance(const position2_base& to) const
	{
		return std::sqrt(double((x - to.x) * (x - to.x) + (y - to.y) * (y - to.y)));
	}
};

template<typename T>
struct position3_base
{
	T x, y, z;
	/*
	position3_base() : x{}, y{}, z{}
	{
	}

	position3_base(T x, T y, T z) : x{ x }, y{ y }, z{ z }
	{
	}
	*/

	position3_base operator -(const position3_base& rhs) const
	{
		return{ x - rhs.x, y - rhs.y, z - rhs.z };
	}
	position3_base operator -(T rhs) const
	{
		return{ x - rhs, y - rhs, z - rhs };
	}
	position3_base operator +(const position3_base& rhs) const
	{
		return{ x + rhs.x, y + rhs.y, z + rhs.z };
	}
	position3_base operator +(T rhs) const
	{
		return{ x + rhs, y + rhs, z + rhs };
	}

	position3_base& operator -=(const position3_base& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}
	position3_base& operator -=(T rhs)
	{
		x -= rhs;
		y -= rhs;
		z -= rhs;
		return *this;
	}
	position3_base& operator +=(const position3_base& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
	position3_base& operator +=(T rhs)
	{
		x += rhs;
		y += rhs;
		z += rhs;
		return *this;
	}

	bool operator ==(const position3_base& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}

	bool operator ==(T rhs) const
	{
		return x == rhs && y == rhs && z == rhs;
	}

	bool operator !=(const position3_base& rhs) const
	{
		return !(*this == rhs);
	}

	bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	operator position3_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z };
	}
};

template<typename T>
struct position4_base
{
	T x, y, z, w;

	constexpr position4_base() : x{}, y{}, z{}, w{}
	{
	}

	constexpr position4_base(T x, T y = {}, T z = {}, T w = { T(1) }) : x{ x }, y{ y }, z{ z }, w{ w }
	{
	}

	constexpr position4_base operator -(const position4_base& rhs) const
	{
		return{ x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w };
	}
	constexpr position4_base operator -(T rhs) const
	{
		return{ x - rhs, y - rhs, z - rhs, w - rhs };
	}
	constexpr position4_base operator +(const position4_base& rhs) const
	{
		return{ x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
	}
	constexpr position4_base operator +(T rhs) const
	{
		return{ x + rhs, y + rhs, z + rhs, w + rhs };
	}

	position4_base& operator -=(const position4_base& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}
	position4_base& operator -=(T rhs)
	{
		x -= rhs;
		y -= rhs;
		z -= rhs;
		w -= rhs;
		return *this;
	}
	position4_base& operator +=(const position4_base& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}
	position4_base& operator +=(T rhs)
	{
		x += rhs;
		y += rhs;
		z += rhs;
		w += rhs;
		return *this;
	}

	constexpr bool operator ==(const position4_base& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
	}

	constexpr bool operator ==(T rhs) const
	{
		return x == rhs && y == rhs && z == rhs && w == rhs;
	}

	constexpr bool operator !=(const position4_base& rhs) const
	{
		return !(*this == rhs);
	}

	constexpr bool operator !=(T rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator position4_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z, (NT)w };
	}
};

template<typename T>
using position_base = position2_base<T>;

template<typename T>
struct coord_base
{
	union
	{
		position_base<T> position;
		struct { T x, y; };
	};

	union
	{
		size2_base<T> size;
		struct { T width, height; };
	};

	constexpr coord_base() : position{}, size{}
#ifdef _MSC_VER
		//compiler error
		, x{}, y{}, width{}, height{}
#endif
	{
	}

	constexpr coord_base(const position_base<T>& position, const size2_base<T>& size)
		: position{ position }, size{ size }
#ifdef _MSC_VER
		, x{ position.x }, y{ position.y }, width{ size.width }, height{ size.height }
#endif
	{
	}

	constexpr coord_base(T x, T y, T width, T height) : x{ x }, y{ y }, width{ width }, height{ height }
	{
	}

	constexpr bool test(const position_base<T>& position) const
	{
		if (position.x < x || position.x >= x + width)
			return false;

		if (position.y < y || position.y >= y + height)
			return false;

		return true;
	}

	constexpr bool operator == (const coord_base& rhs) const
	{
		return position == rhs.position && size == rhs.size;
	}

	constexpr bool operator != (const coord_base& rhs) const
	{
		return position != rhs.position || size != rhs.size;
	}

	template<typename NT>
	constexpr operator coord_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)width, (NT)height };
	}
};

template<typename T>
struct area_base
{
	T x1, x2;
	T y1, y2;

	constexpr area_base() : x1{}, x2{}, y1{}, y2{}
	{
	}

	constexpr area_base(T x1, T y1, T x2, T y2) : x1{ x1 }, x2{ x2 }, y1{ y1 }, y2{ y2 }
	{
	}

	constexpr area_base(const coord_base<T>& coord) : x1{ coord.x }, x2{ coord.x + coord.width }, y1{ coord.y }, y2{ coord.y + coord.height }
	{
	}

	constexpr operator coord_base<T>() const
	{
		return{ x1, y1, x2 - x1, y2 - y1 };
	}

	void flip_vertical()
	{
		T _y = y1; y1 = y2; y2 = _y;
	}

	void flip_horizontal()
	{
		T _x = x1; x1 = x2; x2 = _x;
	}

	constexpr area_base flipped_vertical() const
	{
		return{ x1, y2, x2, y1 };
	}

	constexpr area_base flipped_horizontal() const
	{
		return{ x2, y1, x1, y2 };
	}

	constexpr bool operator == (const area_base& rhs) const
	{
		return x1 == rhs.x1 && x2 == rhs.x2 && y1 == rhs.y1 && y2 == rhs.y2;
	}

	constexpr bool operator != (const area_base& rhs) const
	{
		return !(*this == rhs);
	}

	constexpr area_base operator - (const size2_base<T>& size) const
	{
		return{ x1 - size.width, y1 - size.height, x2 - size.width, y2 - size.height };
	}
	constexpr area_base operator - (const T& value) const
	{
		return{ x1 - value, y1 - value, x2 - value, y2 - value };
	}
	constexpr area_base operator + (const size2_base<T>& size) const
	{
		return{ x1 + size.width, y1 + size.height, x2 + size.width, y2 + size.height };
	}
	constexpr area_base operator + (const T& value) const
	{
		return{ x1 + value, y1 + value, x2 + value, y2 + value };
	}
	constexpr area_base operator / (const size2_base<T>& size) const
	{
		return{ x1 / size.width, y1 / size.height, x2 / size.width, y2 / size.height };
	}
	constexpr area_base operator / (const T& value) const
	{
		return{ x1 / value, y1 / value, x2 / value, y2 / value };
	}
	constexpr area_base operator * (const size2_base<T>& size) const
	{
		return{ x1 * size.width, y1 * size.height, x2 * size.width, y2 * size.height };
	}
	constexpr area_base operator * (const f32& value) const
	{
		return{ (T)(x1 * value), (T)(y1 * value), (T)(x2 * value), (T)(y2 * value) };
	}

	template<typename NT>
	constexpr operator area_base<NT>() const
	{
		return{ (NT)x1, (NT)y1, (NT)x2, (NT)y2 };
	}
};

template<typename T>
struct size3_base
{
	T width, height, depth;
	/*
	size3_base() : width{}, height{}, depth{}
	{
	}

	size3_base(T width, T height, T depth) : width{ width }, height{ height }, depth{ depth }
	{
	}
	*/
};

template<typename T>
struct coord3_base
{
	union
	{
		position3_base<T> position;
		struct { T x, y, z; };
	};

	union
	{
		size3_base<T> size;
		struct { T width, height, depth; };
	};

	constexpr coord3_base() : position{}, size{}
	{
	}

	constexpr coord3_base(const position3_base<T>& position, const size3_base<T>& size) : position{ position }, size{ size }
	{
	}

	constexpr coord3_base(T x, T y, T z, T width, T height, T depth) : x{ x }, y{ y }, z{ z }, width{ width }, height{ height }, depth{ depth }
	{
	}

	constexpr bool test(const position3_base<T>& position) const
	{
		if (position.x < x || position.x >= x + width)
			return false;

		if (position.y < y || position.y >= y + height)
			return false;

		if (position.z < z || position.z >= z + depth)
			return false;

		return true;
	}

	template<typename NT>
	constexpr operator coord3_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z, (NT)width, (NT)height, (NT)depth };
	}
};


template<typename T>
struct color4_base
{
	union
	{
		struct
		{
			T r, g, b, a;
		};

		struct
		{
			T x, y, z, w;
		};

		T rgba[4];
		T xyzw[4];
	};

	color4_base()
		: x{}
		, y{}
		, z{}
		, w{ T(1) }
	{
	}

	color4_base(T x, T y = {}, T z = {}, T w = {})
		: x(x)
		, y(y)
		, z(z)
		, w(w)
	{
	}

	bool operator == (const color4_base& rhs) const
	{
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}

	bool operator != (const color4_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	operator color4_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z, (NT)w };
	}
};

template<typename T>
struct color3_base
{
	union
	{
		struct
		{
			T r, g, b;
		};

		struct
		{
			T x, y, z;
		};

		T rgb[3];
		T xyz[3];
	};

	constexpr color3_base(T x = {}, T y = {}, T z = {})
		: x(x)
		, y(y)
		, z(z)
	{
	}

	constexpr bool operator == (const color3_base& rhs) const
	{
		return r == rhs.r && g == rhs.g && b == rhs.b;
	}

	constexpr bool operator != (const color3_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator color3_base<NT>() const
	{
		return{ (NT)x, (NT)y, (NT)z };
	}
};

template<typename T>
struct color2_base
{
	union
	{
		struct
		{
			T r, g;
		};

		struct
		{
			T x, y;
		};

		T rg[2];
		T xy[2];
	};

	constexpr color2_base(T x = {}, T y = {})
		: x(x)
		, y(y)
	{
	}

	constexpr bool operator == (const color2_base& rhs) const
	{
		return r == rhs.r && g == rhs.g;
	}

	constexpr bool operator != (const color2_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator color2_base<NT>() const
	{
		return{ (NT)x, (NT)y };
	}
};

template<typename T>
struct color1_base
{
	union
	{
		T r;
		T x;
	};

	constexpr color1_base(T x = {})
		: x(x)
	{
	}

	constexpr bool operator == (const color1_base& rhs) const
	{
		return r == rhs.r;
	}

	constexpr bool operator != (const color1_base& rhs) const
	{
		return !(*this == rhs);
	}

	template<typename NT>
	constexpr operator color1_base<NT>() const
	{
		return{ (NT)x };
	}
};

//specializations
using positionu = position_base<unsigned int>;
using positioni = position_base<int>;
using positionf = position_base<float>;
using positiond = position_base<double>;

using coordu = coord_base<unsigned int>;
using coordi = coord_base<int>;
using coordf = coord_base<float>;
using coordd = coord_base<double>;

using areau = area_base<unsigned int>;
using areai = area_base<int>;
using areaf = area_base<float>;
using aread = area_base<double>;

using position1u = position1_base<unsigned int>;
using position1i = position1_base<int>;
using position1f = position1_base<float>;
using position1d = position1_base<double>;

using position2u = position2_base<unsigned int>;
using position2i = position2_base<int>;
using position2f = position2_base<float>;
using position2d = position2_base<double>;

using position3u = position3_base<unsigned int>;
using position3i = position3_base<int>;
using position3f = position3_base<float>;
using position3d = position3_base<double>;

using position4u = position4_base<unsigned int>;
using position4i = position4_base<int>;
using position4f = position4_base<float>;
using position4d = position4_base<double>;

using size2u = size2_base<unsigned int>;
using size2i = size2_base<int>;
using size2f = size2_base<float>;
using size2d = size2_base<double>;

using sizeu = size2u;
using sizei = size2i;
using sizef = size2f;
using sized = size2d;

using size3u = size3_base<unsigned int>;
using size3i = size3_base<int>;
using size3f = size3_base<float>;
using size3d = size3_base<double>;

using coord3u = coord3_base<unsigned int>;
using coord3i = coord3_base<int>;
using coord3f = coord3_base<float>;
using coord3d = coord3_base<double>;

using color4u = color4_base<unsigned int>;
using color4i = color4_base<int>;
using color4f = color4_base<float>;
using color4d = color4_base<double>;

using color3u = color3_base<unsigned int>;
using color3i = color3_base<int>;
using color3f = color3_base<float>;
using color3d = color3_base<double>;

using color2u = color2_base<unsigned int>;
using color2i = color2_base<int>;
using color2f = color2_base<float>;
using color2d = color2_base<double>;

using color1u = color1_base<unsigned int>;
using color1i = color1_base<int>;
using color1f = color1_base<float>;
using color1d = color1_base<double>;
