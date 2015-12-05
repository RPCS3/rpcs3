#pragma once
#include <cmath>
#include <string>

namespace common
{
	template<typename T>
	static constexpr T pi = T(3.141592653589793238462643383279502884197169399375105820974944592307816406286);

	template<int Dimension, typename Type>
	struct vector
	{
		static constexpr int dimension = Dimension;
		using type = Type;

	protected:
		type m_data[dimension];

	public:
		using iterator = type *;
		using const_iterator = const type *;

		vector(const type(&data)[dimension])
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] = data[i];
			}
		}

		template<typename... Args>
		constexpr vector(Args... args) : m_data{ args... }
		{
		}

		constexpr bool operator >(const vector& rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] <= rhs.m_data[i])
					return false;
			}
			return true;
		}
		constexpr bool operator >(Type rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] <= rhs)
					return false;
			}
			return true;
		}
		constexpr bool operator <(const vector& rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] >= rhs.m_data[i])
					return false;
			}
			return true;
		}
		constexpr bool operator <(Type rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] >= rhs)
					return false;
			}
			return true;
		}
		constexpr bool operator >=(const vector& rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] < rhs.m_data[i])
					return false;
			}
			return true;
		}
		constexpr bool operator >=(Type rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] < rhs)
					return false;
			}
			return true;
		}
		constexpr bool operator <=(const vector& rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] > rhs.m_data[i])
					return false;
			}
			return true;
		}
		constexpr bool operator <=(Type rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] > rhs)
					return false;
			}
			return true;
		}

		constexpr vector operator -(const vector& rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] - rhs.m_data[i];
			}

			return result;
		}
		constexpr vector operator -(Type rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] - rhs;
			}

			return result;
		}
		constexpr vector operator +(const vector& rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] + rhs.m_data[i];
			}

			return result;
		}
		constexpr vector operator +(Type rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] + rhs;
			}

			return result;
		}

		constexpr vector operator *(Type rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] * rhs;
			}

			return result;
		}
		constexpr vector operator *(const vector& rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] * rhs.m_data[i];
			}

			return result;
		}
		constexpr vector operator /(Type rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] / rhs;
			}

			return result;
		}
		constexpr vector operator /(const vector& rhs) const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result.m_data[i] = m_data[i] / rhs.m_data[i];
			}

			return result;
		}

		vector& operator -=(const vector& rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] -= rhs.m_data[i];
			}

			return *this;
		}
		vector& operator -=(Type rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] -= rhs;
			}
			return *this;
		}
		vector& operator +=(const vector& rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] += rhs.m_data[i];
			}
			return *this;
		}
		vector& operator +=(Type rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] += rhs;
			}
			return *this;
		}

		vector& operator *=(Type rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] *= rhs;
			}
			return *this;
		}
		vector& operator *=(const vector& rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] *= rhs.m_data[i];
			}
			return *this;
		}
		vector& operator /=(Type rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] /= rhs;
			}
			return *this;
		}
		vector& operator /=(const vector& rhs)
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] /= rhs.m_data[i];
			}
			return *this;
		}

		constexpr bool operator ==(const vector& rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] != rhs.m_data[i])
					return false;
			}

			return true;
		}

		constexpr bool operator ==(Type rhs) const
		{
			for (int i = 0; i < dimension; ++i)
			{
				if (m_data[i] != rhs)
					return false;
			}

			return true;
		}

		constexpr bool operator !=(const vector& rhs) const
		{
			return !(*this == rhs);
		}

		constexpr bool operator !=(Type rhs) const
		{
			return !(*this == rhs);
		}

		constexpr type operator[](std::size_t index) const
		{
			return m_data[index];
		}

		type& operator[](std::size_t index)
		{
			return m_data[index];
		}

		constexpr vector abs() const
		{
			vector result;

			for (int i = 0; i < dimension; ++i)
			{
				result[i] = std::abs(m_data[i]);
			}

			return result;
		}

		template<typename T>
		constexpr operator vector<Dimension, T>() const
		{
			vector<Dimension, T> result;

			for (int i = 0; i < dimension; ++i)
			{
				result[i] = static_cast<T>(m_data[i]);
			}

			return result;
		}

		constexpr double distance(const vector& to) const
		{
			double value = 0.0;

			for (int i = 0; i < dimension; ++i)
			{
				value += (m_data[i] - to.m_data[i]) * (m_data[i] - to.m_data[i]);
			}

			return std::sqrt(value);
		}

		iterator begin()
		{
			return m_data.begin();
		}

		constexpr const_iterator begin() const
		{
			return m_data.begin();
		}

		iterator end()
		{
			return m_data.end();
		}

		constexpr const_iterator end() const
		{
			return m_data.end();
		}
	};

	template<int Dimension>
	struct vector<Dimension, bool>
	{
		static constexpr int dimension = Dimension;
		using type = bool;

	protected:
		type m_data[dimension];

	public:
		using iterator = type *;
		using const_iterator = const type *;

		vector(const type(&data)[dimension])
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] = data[i];
			}
		}

		template<typename... Args>
		constexpr vector(Args... args) : m_data{ args... }
		{
		}

		constexpr bool all() const
		{
			for (bool value : m_data)
			{
				if (!value)
				{
					return false;
				}
			}

			return true;
		}

		constexpr bool any() const
		{
			for (bool value : m_data)
			{
				if (value)
				{
					return true;
				}
			}

			return false;
		}

		constexpr type operator[](std::size_t index) const
		{
			return m_data[index];
		}

		type& operator[](std::size_t index)
		{
			return m_data[index];
		}

		iterator begin()
		{
			return m_data.begin();
		}

		constexpr const_iterator begin() const
		{
			return m_data.begin();
		}

		iterator end()
		{
			return m_data.end();
		}

		constexpr const_iterator end() const
		{
			return m_data.end();
		}
	};

	template<typename Type, int RowCount, int ColumnCount>
	struct matrix
	{
		using type = Type;
		using vector_t = vector<ColumnCount, Type>;

		static constexpr int row_count = RowCount;
		static constexpr int column_count = ColumnCount;
		static constexpr int count = row_count;

	private:
		vector_t m_data[column_count];

	public:
		using iterator = vector_t *;
		using const_iterator = const vector_t *;

		matrix(const vector_t(&data)[row_count])
		{
			for (int i = 0; i < dimension; ++i)
			{
				m_data[i] = data[i];
			}
		}

		template<typename... Args>
		constexpr matrix(Args... args) : m_data{ args... }
		{
		}

		constexpr const vector_t &operator[](std::size_t index) const
		{
			return m_data[index];
		}

		vector_t& operator[](std::size_t index)
		{
			return m_data[index];
		}

		iterator begin()
		{
			return m_data.begin();
		}

		constexpr const_iterator begin() const
		{
			return m_data.begin();
		}

		iterator end()
		{
			return m_data.end();
		}

		constexpr const_iterator end() const
		{
			return m_data.end();
		}
	};

	template<int Dimension, typename Type>
	struct point : vector<Dimension, Type>
	{
		using base = vector<Dimension, Type>;

		constexpr point(const base& rhs) : base(rhs)
		{
		}

		constexpr point() = default;

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct point<1, Type> : vector<1, Type>
	{
		using base = vector<1, Type>;

		constexpr point(const base& rhs) : base(rhs)
		{
		}
		constexpr point(Type x_ = {}) : base(x_)
		{
		}

		constexpr Type x() const { return base::m_data[0]; }
		void x(Type value) { base::m_data[0] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct point<2, Type> : vector<2, Type>
	{
		using base = vector<2, Type>;

		constexpr point(const base& rhs) : base(rhs)
		{
		}
		constexpr point(Type x_ = {}, Type y_ = {}) : base(x_, y_)
		{
		}

		constexpr Type x() const { return base::m_data[0]; }
		void x(Type value) { base::m_data[0] = value; }
		constexpr Type y() const { return base::m_data[1]; }
		void y(Type value) { base::m_data[1] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct point<3, Type> : vector<3, Type>
	{
		using base = vector<3, Type>;

		constexpr point(const base& rhs) : base(rhs)
		{
		}
		constexpr point(Type x_ = {}, Type y_ = {}, Type z_ = {}) : base(x_, y_, z_)
		{
		}

		constexpr Type x() const { return base::m_data[0]; }
		void x(Type value) { base::m_data[0] = value; }
		constexpr Type y() const { return base::m_data[1]; }
		void y(Type value) { base::m_data[1] = value; }
		constexpr Type z() const { return base::m_data[2]; }
		void z(Type value) { base::m_data[2] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct point<4, Type> : vector<4, Type>
	{
		using base = vector<4, Type>;

		constexpr point(const base& rhs) : base(rhs)
		{
		}
		constexpr point(Type x_ = {}, Type y_ = {}, Type z_ = {}, Type w_ = { 1 })
			: base(x_, y_, z_, w_)
		{
		}

		constexpr Type x() const { return base::m_data[0]; }
		void x(Type value) { base::m_data[0] = value; }
		constexpr Type y() const { return base::m_data[1]; }
		void y(Type value) { base::m_data[1] = value; }
		constexpr Type z() const { return base::m_data[2]; }
		void z(Type value) { base::m_data[2] = value; }
		constexpr Type w() const { return base::m_data[3]; }
		void w(Type value) { base::m_data[3] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};


	template<int Dimension, typename Type>
	struct size : vector<Dimension, Type>
	{
		using base = vector<Dimension, Type>;

		constexpr size(const base& rhs) : base(rhs)
		{
		}
		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct size<1, Type> : vector<1, Type>
	{
		using base = vector<1, Type>;

		constexpr size(const base& rhs) : base(rhs)
		{
		}
		constexpr size(Type width_ = {}) : base(width_)
		{
		}
		constexpr Type width() const { return base::m_data[0]; }
		void width(Type value) { base::m_data[0] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct size<2, Type> : vector<2, Type>
	{
		using base = vector<2, Type>;

		constexpr size(const base& rhs) : base(rhs)
		{
		}
		constexpr size(Type width_ = {}, Type height_ = {}) : base(width_, height_)
		{
		}

		constexpr Type width() const { return base::m_data[0]; }
		void width(Type value) { base::m_data[0] = value; }
		constexpr Type height() const { return base::m_data[1]; }
		void height(Type value) { base::m_data[1] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct size<3, Type> : vector<3, Type>
	{
		using base = vector<3, Type>;

		constexpr size(const base& rhs) : base(rhs)
		{
		}
		constexpr size(Type width_ = {}, Type height_ = {}, Type depth_ = {})
			: base(width_, height_, depth_)
		{
		}

		constexpr Type width() const { return base::m_data[0]; }
		void width(Type value) { base::m_data[0] = value; }
		constexpr Type height() const { return base::m_data[1]; }
		void height(Type value) { base::m_data[1] = value; }
		constexpr Type depth() const { return base::m_data[2]; }
		void depth(Type value) { base::m_data[2] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<int Dimension, typename Type>
	struct color : vector<Dimension, Type>
	{
		using base = vector<Dimension, Type>;

		constexpr color(const base& rhs) : base(rhs)
		{
		}
		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct color<1, Type> : vector<1, Type>
	{
		using base = vector<1, Type>;

		constexpr color(const base& rhs) : base(rhs)
		{
		}
		constexpr color(Type r_ = {}) : base(r_)
		{
		}

		constexpr Type r() const { return base::m_data[0]; }
		void r(Type value) { base::base::m_data[0] = value; }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct color<2, Type> : vector<2, Type>
	{
		using base = vector<2, Type>;

		constexpr color(const base& rhs) : base(rhs)
		{
		}
		constexpr color(Type r_ = {}, Type g_ = {}) : base(r_, g_)
		{
		}

		constexpr Type r() const { return base::m_data[0]; }
		void r(Type value) { base::m_data[0] = value; }
		constexpr Type g() const { return base::m_data[1]; }
		void g(Type value) { base::m_data[1] = value; }

		constexpr const Type* rg() const { return base::m_data.data(); }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct color<3, Type> : vector<3, Type>
	{
		using base = vector<3, Type>;

		constexpr color(const base& rhs) : base(rhs)
		{
		}
		constexpr color(Type r_ = {}, Type g_ = {}, Type b_ = {}) : base(r_, g_, b_)
		{
		}

		constexpr Type r() const { return base::m_data[0]; }
		void r(Type value) { base::m_data[0] = value; }
		constexpr Type g() const { return base::m_data[1]; }
		void g(Type value) { base::m_data[1] = value; }
		constexpr Type b() const { return base::m_data[2]; }
		void b(Type value) { base::m_data[2] = value; }

		constexpr const Type* rgb() const { return base::m_data.data(); }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<typename Type>
	struct color<4, Type> : vector<4, Type>
	{
		using base = vector<4, Type>;

		constexpr color(const base& rhs) : base(rhs)
		{
		}
		constexpr color(Type r_ = {}, Type g_ = {}, Type b_ = {}, Type a_ = { 1 })
			: base(r_, g_, b_, a_)
		{
		}

		constexpr Type r() const { return base::m_data[0]; }
		void r(Type value) { base::m_data[0] = value; }
		constexpr Type g() const { return base::m_data[1]; }
		void g(Type value) { base::m_data[1] = value; }
		constexpr Type b() const { return base::m_data[2]; }
		void b(Type value) { base::m_data[2] = value; }
		constexpr Type a() const { return base::m_data[3]; }
		void a(Type value) { base::m_data[3] = value; }

		constexpr const Type* rgba() const { return base::m_data.data(); }

		using base::operator==;
		using base::operator!=;

		using base::operator>;
		using base::operator<;
		using base::operator>=;
		using base::operator<=;

		using base::operator+;
		using base::operator-;
		using base::operator/;
		using base::operator*;

		using base::operator+=;
		using base::operator-=;
		using base::operator/=;
		using base::operator*=;

		using base::operator[];
	};

	template<int Dimension, typename Type>
	struct coord;

	template<typename ValueType, typename RangeType>
	static bool in_range(const ValueType& test, RangeType min, RangeType max)
	{
		return test >= min && test <= max;
	}

	template<int Dimension, typename Type>
	struct area
	{
		static constexpr int dimension = Dimension;
		using Type = Type;

		point<dimension, Type> p1;
		point<dimension, Type> p2;

		constexpr operator coord<dimension, Type>() const
		{
			point<dimension, Type> pos;
			size<dimension, Type> size;

			for (int i = 0; i < dimension; ++i)
			{
				if (p1.m_data[i] < p2.m_data[i])
				{
					pos.m_data[i] = p1.m_data[i];
					size.m_data[i] = p2.m_data[i] - p1.m_data[i];
				}
				else
				{
					pos.m_data[i] = p2.m_data[i];
					size.m_data[i] = p1.m_data[i] - p2.m_data[i];
				}
			}

			return{ pos, size };
		}

		constexpr bool operator ==(const area& rhs) const
		{
			return p1 == rhs.p1 && p2 == rhs.p2;
		}

		constexpr bool operator !=(const area& rhs) const
		{
			return !(*this == rhs);
		}

		constexpr area flipped_vertical() const
		{
			return area{ { p1.x(), p2.y() },{ p2.x(), p1.y() } };
		}
	};

	template<int Dimension, typename Type>
	struct coord
	{
		static constexpr int dimension = Dimension;
		using Type = Type;

		point<dimension, Type> position;
		size<dimension, Type> size;

		constexpr operator area<dimension, Type>() const
		{
			return{ position, position + size };
		}

		constexpr bool operator ==(const coord& rhs) const
		{
			return position == rhs.position && size == rhs.size;
		}

		constexpr bool operator !=(const coord& rhs) const
		{
			return !(*this == rhs);
		}

		constexpr bool intersect(const coord& r) const
		{
			Type tw = size.width();
			Type th = size.height();
			Type rw = r.size.width();
			Type rh = r.size.height();

			if (rw <= Type{} || rh <= Type{} || tw <= Type{} || th <= Type{})
				return false;

			Type tx = position.x();
			Type ty = position.y();
			Type rx = r.position.x();
			Type ry = r.position.y();

			rw += rx;
			rh += ry;
			tw += tx;
			th += ty;

			return
				((rw < rx || rw > tx) &&
					(rh < ry || rh > ty) &&
					(tw < tx || tw > rx) &&
					(th < ty || th > ry));
		}
	};

	using point1i = point<1, int>;
	using point1f = point<1, float>;
	using point1d = point<1, double>;

	using point2i = point<2, int>;
	using point2f = point<2, float>;
	using point2d = point<2, double>;

	using point3i = point<3, int>;
	using point3f = point<3, float>;
	using point3d = point<3, double>;

	using point4i = point<4, int>;
	using point4f = point<4, float>;
	using point4d = point<4, double>;

	using size1i = size<1, int>;
	using size1f = size<1, float>;
	using size1d = size<1, double>;

	using size2i = size<2, int>;
	using size2f = size<2, float>;
	using size2d = size<2, double>;

	using size3i = size<3, int>;
	using size3f = size<3, float>;
	using size3d = size<3, double>;

	using color1i = color<1, int>;
	using color1f = color<1, float>;
	using color1d = color<1, double>;

	using color2i = color<2, int>;
	using color2f = color<2, float>;
	using color2d = color<2, double>;

	using color3i = color<3, int>;
	using color3f = color<3, float>;
	using color3d = color<3, double>;

	using color4i = color<4, int>;
	using color4f = color<4, float>;
	using color4d = color<4, double>;

	using area2i = area<2, int>;
	using area2f = area<2, float>;
	using area2d = area<2, double>;

	using coord2i = coord<2, int>;
	using coord2f = coord<2, float>;
	using coord2d = coord<2, double>;

	template<>
	struct to_impl_t<size2i, std::string>
	{
		static size2i func(const std::string& value)
		{
			const auto& data = fmt::split(value, { "x" });
			return{ std::stoi(data[0]), std::stoi(data[1]) };
		}
	};

	template<>
	struct to_impl_t<position2i, std::string>
	{
		static position2i func(const std::string& value)
		{
			const auto& data = fmt::split(value, { ":" });
			return{ std::stoi(data[0]), std::stoi(data[1]) };
		}
	};
}
