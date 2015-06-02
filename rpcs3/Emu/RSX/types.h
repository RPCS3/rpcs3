#pragma once
#include <typeinfo>

struct ignore
{
	template<typename T>
	ignore(T)
	{
	}
};

class any
{
	void *m_data = nullptr;
	int m_size = 0;
	size_t m_type_hash = 0;

public:
	template<typename T>
	any(const T& data)
	{
		assign(data);
	}
	
	~any()
	{
		clear();
	}

	template<typename T>
	any& operator =(const T& data)
	{
		reset(data);
		return this;
	}

	template<typename T>
	void reset(const T& data)
	{
		delete m_data;

		m_data = new char[sizeof(T)];
		memcpy(m_data, &data, sizeof(T));
		m_size = sizeof(T);
		m_type_hash = typeid(T).hash_code();
	}
	
	void clear()
	{
		delete m_data;
		m_data = nullptr;
		m_type_hash = 0;
		m_size = 0;
	}

	template<typename T>
	operator T&()
	{
		assert(sizeof(T) == m_size);
		assert(typeid(T).hash_code() == m_type_hash);
		return *(T*)m_data;
	}
	
	template<typename T>
	operator T() const
	{
		assert(sizeof(T) == m_size);
		assert(typeid(T).hash_code() == m_type_hash);
		return *(T*)m_data;
	}
	
	template<typename T>
	bool test() const
	{
		return sizeof(T) == m_size && typeid(T).hash_code() == m_type_hash;
	}

	int size() const
	{
		return m_size;
	}

	template<typename T=void>
	T* data() const
	{
		return (T*)m_data;
	}

	size_t hash_code() const
	{
		return m_type_hash;
	}
};

template<typename T>
struct position_base
{
	T x, y;
	/*
	position_base() : x{}, y{}
	{
	}

	position_base(T x, T y) : x{ x }, y{ y }
	{
	}
	*/

	bool operator == (const position_base& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

	bool operator != (const position_base& rhs) const
	{
		return x != rhs.x || y != rhs.y;
	}
};

template<typename T>
struct size_base
{
	T width, height;

	/*
	size_base() : width{}, height{}
	{
	}

	size_base(T width, T height) : width{ width }, height{ height }
	{
	}
	*/

	bool operator == (const size_base& rhs) const
	{
		return width == rhs.width && height == rhs.height;
	}

	bool operator != (const size_base& rhs) const
	{
		return width != rhs.width || height != rhs.height;
	}
};

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
		size_base<T> size;
		struct { T width, height; };
	};

	coord_base() : position{}, size{}
	{
	}

	coord_base(const position_base<T>& position, const size_base<T>& size) : position(position), size(size)
	{
	}

	coord_base(T x, T y, T width, T height) : x{ x }, y{ y }, width{ width }, height{ height }
	{
	}

	bool test(const position_base<T>& position)
	{
		if (position.x < x || position.x >= x + width)
			return false;

		if (position.y < y || position.y >= y + height)
			return false;

		return true;
	}

	bool operator == (const coord_base& rhs) const
	{
		return position == rhs.position && size == rhs.size;
	}

	bool operator != (const coord_base& rhs) const
	{
		return position != rhs.position || size != rhs.size;
	}
};

template<typename T>
struct area_base
{
	T x1, x2;
	T y1, y2;

	area_base() : x1{}, x2{}, y1{}, y2{}
	{
	}

	area_base(T x1, T y1, T x2, T y2) : x1{ x1 }, x2{ x2 }, y1{ y1 }, y2{ y2 }
	{
	}

	area_base(const coord_base<T>& coord) : x1{ coord.x }, x2{ coord.x + coord.width }, y1{ coord.y }, y2{ coord.y + coord.height }
	{
	}

	operator coord_base<T>() const
	{
		return{ x1, y1, x2 - x1, y2 - y1 };
	}

	void flip_vertical()
	{
		std::swap(y1, y2);
	}

	void flip_horizontal()
	{
		std::swap(x1, x2);
	}

	area_base flipped_vertical() const
	{
		return{ x1, y2, x2, y1 };
	}

	area_base flipped_horizontal() const
	{
		return{ x2, y1, x1, y2 };
	}

	bool operator == (const area_base& rhs) const
	{
		return x1 == rhs.x1 && x2 == rhs.x2 && y1 == rhs.y1 && y2 == rhs.y2;
	}

	bool operator != (const area_base& rhs) const
	{
		return !(*this == rhs);
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
};

template<typename T>
struct position4_base
{
	T x, y, z, w;
	/*
	position4_base() : x{}, y{}, z{}, w{}
	{
	}

	position4_base(T x, T y, T z, T w) : x{ x }, y{ y }, z{ z }, z{ w }
	{
	}
	*/
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

	coord3_base() : position{}, size{}
	{
	}

	coord3_base(const position3_base<T>& position, const size3_base<T>& size) : position{ position }, size{ size }
	{
	}

	coord3_base(T x, T y, T z, T width, T height, T depth) : x{ x }, y{ y }, z{ z }, width{ width }, height{ height }, depth{ depth }
	{
	}

	bool test(const position3_base<T>& position)
	{
		if (position.x < x || position.x >= x + width)
			return false;

		if (position.y < y || position.y >= y + height)
			return false;

		if (position.z < z || position.z >= z + depth)
			return false;

		return true;
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

	color4_base(T x = {}, T y = {}, T z = {}, T w = {})
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

	color3_base(T x = {}, T y = {}, T z = {})
		: x(x)
		, y(y)
		, z(z)
	{
	}

	bool operator == (const color3_base& rhs) const
	{
		return r == rhs.r && g == rhs.g && b == rhs.b;
	}

	bool operator != (const color3_base& rhs) const
	{
		return !(*this == rhs);
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

	color2_base(T x = {}, T y = {})
		: x(x)
		, y(y)
	{
	}

	bool operator == (const color2_base& rhs) const
	{
		return r == rhs.r && g == rhs.g;
	}

	bool operator != (const color2_base& rhs) const
	{
		return !(*this == rhs);
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

	color1_base(T x = {})
		: x(x)
	{
	}

	bool operator == (const color1_base& rhs) const
	{
		return r == rhs.r &&;
	}

	bool operator != (const color1_base& rhs) const
	{
		return !(*this == rhs);
	}
};

//specializations
typedef position_base<int> positioni;
typedef position_base<float> positionf;
typedef position_base<double> positiond;

typedef size_base<int> sizei;
typedef size_base<float> sizef;
typedef size_base<double> sized;

typedef coord_base<int> coordi;
typedef coord_base<float> coordf;
typedef coord_base<double> coordd;

typedef area_base<int> areai;
typedef area_base<float> areaf;
typedef area_base<double> aread;

typedef position3_base<int> position3i;
typedef position3_base<float> position3f;
typedef position3_base<double> position3d;

typedef size3_base<int> size3i;
typedef size3_base<float> size3f;
typedef size3_base<double> size3d;

typedef coord3_base<int> coord3i;
typedef coord3_base<float> coord3f;
typedef coord3_base<double> coord3d;

typedef color4_base<int> color4i;
typedef color4_base<float> color4f;
typedef color4_base<double> color4d;

typedef color3_base<int> color3i;
typedef color3_base<float> color3f;
typedef color3_base<double> color3d;

typedef color2_base<int> color2i;
typedef color2_base<float> color2f;
typedef color2_base<double> color2d;

typedef color1_base<int> color1i;
typedef color1_base<float> color1f;
typedef color1_base<double> color1d;

//defaults
typedef positionf position;
typedef sizef size;
typedef coordf coord;
typedef areai area;

typedef position3f position3;
typedef size3f size3;
typedef coord3f coord3;

typedef color4f color4;
typedef color3f color3;
typedef color2f color2;
typedef color1f color1;

typedef color4i colori;
typedef color4f colorf;
typedef color4d colord;

typedef colorf color;