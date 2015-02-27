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
	template<typename T>
	struct alloc_array
	{
		alloc_array(void*& dst, int& size, const T& data)
		{
			dst = new char[sizeof(T)];
			memcpy(dst, &data, sizeof(T));
			size = sizeof(T);
		}
	};

	template<>
	struct alloc_array<void>
	{
		alloc_array(void*& dst, int& size, ignore)
		{
			dst = nullptr;
			size = 0;
		}
	};

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

		alloc_array<T>(m_data, m_size, data);
		type_hash = typeid(T).hash_code();
	}
	
	void clear()
	{
		delete m_data;
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
		return (T*)data;
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

//defaults
typedef positionf position;
typedef sizef size;
typedef coordf coord;
typedef areai area;

typedef position3f position3;
typedef size3f size3;
typedef coord3f coord3;