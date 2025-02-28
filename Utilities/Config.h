#pragma once

#include "util/types.hpp"
#include "Utilities/StrUtil.h"
#include "util/logs.hpp"
#include "util/atomic.hpp"
#include "util/shared_ptr.hpp"

#include <algorithm>
#include <utility>
#include <string>
#include <vector>
#include <set>
#include <map>

namespace cfg
{
	// Format min and max values
	std::vector<std::string> make_int_range(s64 min, s64 max);

	// Format min and max unsigned values
	std::vector<std::string> make_uint_range(u64 min, u64 max);

	// Format min and max float values
	std::vector<std::string> make_float_range(f64 min, f64 max);

	// Internal hack
	bool try_to_enum_value(u64* out, decltype(&fmt_class_string<int>::format) func, std::string_view);

	// Internal hack
	std::vector<std::string> try_to_enum_list(decltype(&fmt_class_string<int>::format) func);

	// Config tree entry type.
	enum class type : unsigned
	{
		node = 0, // cfg::node type
		_bool, // cfg::_bool type
		_enum, // cfg::_enum type
		_int, // cfg::_int type
		uint, // cfg::uint type
		string, // cfg::string type
		set, // cfg::set_entry type
		map, // cfg::map_entry type
		node_map, // cfg::node_map_entry type
		log, // cfg::log_entry type
		device, // cfg::device_entry type
	};

	// Config tree entry abstract base class
	class _base
	{
		const type m_type{};

	protected:
		_base* m_parent = nullptr;
		bool m_dynamic = true;
		const std::string m_name{};

		static u32 id_counter;
		u32 m_id = 0;

		// Ownerless entry constructor
		_base(type _type);

		// Owned entry constructor
		_base(type _type, class node* owner, std::string name, bool dynamic);

	public:
		_base(const _base&) = delete;

		_base& operator=(const _base&) = delete;

		virtual ~_base() = default;

		// Get unique ID
		u32 get_id() const { return m_id; }

		// Get parent
		_base* get_parent() const { return m_parent; }

		// Get type
		type get_type() const { return m_type; }

		// Get name
		const std::string& get_name() const { return m_name; }

		// Get dynamic property for reloading configs during games
		bool get_is_dynamic() const { return m_dynamic; }

		// Reset defaults
		virtual void from_default() = 0;

		// Convert to string (optional)
		virtual std::string to_string() const
		{
			return {};
		}

		// Convert default to string (optional)
		virtual std::string def_to_string() const
		{
			return {};
		}

		// Try to convert from string (optional)
		virtual bool from_string(std::string_view, bool /*dynamic*/ = false);

		// Get string list (optional)
		virtual std::vector<std::string> to_list() const
		{
			return {};
		}

		// Set multiple values. Implementation-specific, optional.
		virtual bool from_list(std::vector<std::string>&&);

		bool save(std::string_view cfg_name) const;
	};

	// Config tree node which contains another nodes
	class node : public _base
	{
		std::vector<_base*> m_nodes{};

		friend class _base;

	public:
		// Root node constructor
		node()
			: _base(type::node)
		{
		}

		// Registered node constructor
		node(node* owner, std::string name, bool dynamic = true)
			: _base(type::node, owner, std::move(name), dynamic)
		{
		}

		// Get child nodes
		const auto& get_nodes() const
		{
			return m_nodes;
		}

		// Serialize node
		std::string to_string() const override;

		// Deserialize node
		bool from_string(std::string_view value, bool dynamic = false) override;

		// Set default values
		void from_default() override;
	};

	class _bool final : public _base
	{
		atomic_t<bool> m_value;

	public:
		bool def;

		_bool(node* owner, std::string name, bool def = false, bool dynamic = false)
			: _base(type::_bool, owner, std::move(name), dynamic)
			, m_value(def)
			, def(def)
		{
		}

		explicit operator bool() const
		{
			return m_value;
		}

		bool get() const
		{
			return m_value;
		}

		void from_default() override;

		std::string to_string() const override
		{
			return m_value ? "true" : "false";
		}

		std::string def_to_string() const override
		{
			return def ? "true" : "false";
		}

		bool from_string(std::string_view value, bool /*dynamic*/ = false) override
		{
			if (value.size() != 4 && value.size() != 5)
			{
				return false;
			}

			char copy[5];
			std::transform(value.begin(), value.end(), std::begin(copy), ::tolower);

			if (value.size() == 5 && std::string_view{copy, 5} == "false")
				m_value = false;
			else if (value.size() == 4 && std::string_view{copy, 4} == "true")
				m_value = true;
			else
				return false;

			return true;
		}

		void set(const bool& value)
		{
			m_value = value;
		}
	};

	// Value node with fixed set of possible values, each maps to an enum value of type T.
	template <typename T>
	class _enum : public _base
	{
		atomic_t<T> m_value;

	public:
		const T def;

		_enum(node* owner, const std::string& name, T value = {}, bool dynamic = false)
			: _base(type::_enum, owner, name, dynamic)
			, m_value(value)
			, def(value)
		{
		}

		operator T() const
		{
			return m_value;
		}

		T get() const
		{
			return m_value;
		}

		T get_default() const
		{
			return def;
		}

		void set(T value)
		{
			m_value = value;
		}

		void from_default() override
		{
			m_value = def;
		}

		std::string to_string() const override
		{
			std::string result;
			fmt_class_string<T>::format(result, fmt_unveil<T>::get(m_value.load()));
			return result; // TODO: ???
		}

		std::string def_to_string() const override
		{
			std::string result;
			fmt_class_string<T>::format(result, fmt_unveil<T>::get(def));
			return result; // TODO: ???
		}

		bool from_string(std::string_view value, bool /*dynamic*/ = false) override
		{
			u64 result;

			if (try_to_enum_value(&result, &fmt_class_string<T>::format, value))
			{
				// No narrowing check, it's hard to do right there
				m_value = static_cast<T>(static_cast<std::underlying_type_t<T>>(result));
				return true;
			}

			return false;
		}

		std::vector<std::string> to_list() const override
		{
			return try_to_enum_list(&fmt_class_string<T>::format);
		}
	};

	// Signed 32/64-bit integer entry with custom Min/Max range.
	template <s64 Min, s64 Max>
	class _int final : public _base
	{
		static_assert(Min < Max, "Invalid cfg::_int range");

		// Prefer 32 bit type if possible
		using int_type = std::conditional_t<Min >= s32{smin} && Max <= s32{smax}, s32, s64>;

		atomic_t<int_type> m_value;

	public:
		int_type def;

		// Expose range
		static constexpr s64 max = Max;
		static constexpr s64 min = Min;

		_int(node* owner, const std::string& name, int_type def = std::min<int_type>(Max, std::max<int_type>(Min, 0)), bool dynamic = false)
			: _base(type::_int, owner, name, dynamic)
			, m_value(def)
			, def(def)
		{
		}

		operator int_type() const
		{
			return m_value;
		}

		int_type get() const
		{
			return m_value;
		}

		void from_default() override
		{
			m_value = def;
		}

		std::string to_string() const override
		{
			return std::to_string(m_value);
		}

		std::string def_to_string() const override
		{
			return std::to_string(def);
		}

		bool from_string(std::string_view value, bool /*dynamic*/ = false) override
		{
			s64 result;
			if (try_to_int64(&result, value, Min, Max))
			{
				m_value = static_cast<int_type>(result);
				return true;
			}

			return false;
		}

		void set(const s64& value)
		{
			ensure(value >= Min && value <= Max);
			m_value = static_cast<int_type>(value);
		}

		std::vector<std::string> to_list() const override
		{
			return make_int_range(Min, Max);
		}
	};

	// Float entry with custom Min/Max range.
	template <s32 Min, s32 Max>
	class _float final : public _base
	{
		static_assert(Min < Max, "Invalid cfg::_float range");

		using float_type = f64;
		atomic_t<float_type> m_value;

	public:
		float_type def;

		// Expose range
		static constexpr float_type max = Max;
		static constexpr float_type min = Min;

		_float(node* owner, const std::string& name, float_type def = std::min<float_type>(Max, std::max<float_type>(Min, 0)), bool dynamic = false)
			: _base(type::_int, owner, name, dynamic)
			, m_value(def)
			, def(def)
		{
		}

		operator float_type() const
		{
			return m_value;
		}

		float_type get() const
		{
			return m_value;
		}

		void from_default() override
		{
			m_value = def;
		}

		std::string to_string() const override
		{
			std::string result;
			if (try_to_string(&result, m_value))
			{
				return result;
			}

			return "0.0";
		}

		std::string def_to_string() const override
		{
			std::string result;
			if (try_to_string(&result, def))
			{
				return result;
			}

			return "0.0";
		}

		bool from_string(std::string_view value, bool /*dynamic*/ = false) override
		{
			f64 result;
			if (try_to_float(&result, value, Min, Max))
			{
				m_value = static_cast<float_type>(result);
				return true;
			}

			return false;
		}

		void set(const f64& value)
		{
			ensure(value >= Min && value <= Max);
			m_value = static_cast<float_type>(value);
		}

		std::vector<std::string> to_list() const override
		{
			return make_float_range(Min, Max);
		}
	};

	// Alias for 32 bit int
	using int32 = _int<s32{smin}, s32{smax}>;

	// Alias for 64 bit int
	using int64 = _int<s64{smin}, s64{smax}>;

	// Unsigned 32/64-bit integer entry with custom Min/Max range.
	template <u64 Min, u64 Max>
	class uint final : public _base
	{
		static_assert(Min < Max, "Invalid cfg::uint range");

		// Prefer 32 bit type if possible
		using int_type = std::conditional_t<Max <= u32{umax}, u32, u64>;

		atomic_t<int_type> m_value;

	public:
		int_type def;

		// Expose range
		static constexpr u64 max = Max;
		static constexpr u64 min = Min;

		uint(node* owner, const std::string& name, int_type def = std::max<int_type>(Min, 0), bool dynamic = false)
			: _base(type::uint, owner, name, dynamic)
			, m_value(def)
			, def(def)
		{
		}

		operator int_type() const
		{
			return m_value;
		}

		int_type get() const
		{
			return m_value;
		}

		void from_default() override
		{
			m_value = def;
		}

		std::string to_string() const override
		{
			return std::to_string(m_value);
		}

		std::string def_to_string() const override
		{
			return std::to_string(def);
		}

		bool from_string(std::string_view value, bool /*dynamic*/ = false) override
		{
			u64 result;
			if (try_to_uint64(&result, value, Min, Max))
			{
				m_value = static_cast<int_type>(result);
				return true;
			}

			return false;
		}

		void set(const u64& value)
		{
			ensure(value >= Min && value <= Max);
			m_value = static_cast<int_type>(value);
		}

		std::vector<std::string> to_list() const override
		{
			return make_uint_range(Min, Max);
		}
	};

	// Alias for 32 bit uint
	using uint32 = uint<0, u32{umax}>;

	// Alias for 64 bit int
	using uint64 = uint<0, u64{umax}>;

	// Simple string entry with mutex
	class string : public _base
	{
		atomic_ptr<std::string> m_value;

	public:
		std::string def;

		string(node* owner, std::string name, std::string def = {}, bool dynamic = false)
			: _base(type::string, owner, std::move(name), dynamic)
			, m_value(def)
			, def(std::move(def))
		{
		}

		operator std::string() const
		{
			return *m_value.load().get();
		}

		void from_default() override;

		std::string to_string() const override
		{
			return *m_value.load().get();
		}

		std::string def_to_string() const override
		{
			return def;
		}

		bool from_string(std::string_view value, bool /*dynamic*/ = false) override
		{
			m_value = std::string(value);
			return true;
		}
	};

	// Simple set entry (TODO: template for various types)
	class set_entry final : public _base
	{
		std::set<std::string> m_set{};

	public:
		// Default value is empty list in current implementation
		set_entry(node* owner, const std::string& name)
			: _base(type::set, owner, name, false)
		{
		}

		const std::set<std::string>& get_set() const
		{
			return m_set;
		}

		void set_set(std::set<std::string>&& set)
		{
			m_set = std::move(set);
		}

		void from_default() override;

		std::vector<std::string> to_list() const override
		{
			return { m_set.begin(), m_set.end() };
		}

		bool from_list(std::vector<std::string>&& list) override
		{
			m_set = { std::make_move_iterator(list.begin()), std::make_move_iterator(list.end()) };

			return true;
		}
	};

	template<typename T>
	using map_of_type = std::map<std::string, T, std::less<>>;

	class map_entry : public _base
	{
		map_of_type<std::string> m_map{};

	public:
		map_entry(node* owner, const std::string& name, type _type = type::map)
			: _base(_type, owner, name, true)
		{
		}

		const map_of_type<std::string>& get_map() const
		{
			return m_map;
		}

		std::string get_value(std::string_view key);

		void set_value(std::string key, std::string value);
		void set_map(map_of_type<std::string>&& map);

		void erase(std::string_view key);

		void from_default() override;
	};

	class node_map_entry final : public map_entry
	{
	public:
		node_map_entry(node* owner, const std::string& name)
			: map_entry(owner, name, type::node_map)
		{
		}
	};

	class log_entry final : public _base
	{
		map_of_type<logs::level> m_map{};

	public:
		log_entry(node* owner, const std::string& name)
			: _base(type::log, owner, name, true)
		{
		}

		const map_of_type<logs::level>& get_map() const
		{
			return m_map;
		}

		void set_map(map_of_type<logs::level>&& map);

		void from_default() override;
	};

	struct device_info
	{
		std::string path;
		std::string serial;
		std::string vid;
		std::string pid;
		std::pair<u16, u16> get_usb_ids() const;
	};

	class device_entry final : public _base
	{
		map_of_type<device_info> m_map{};
		map_of_type<device_info> m_default{};

	public:
		device_entry(node* owner, const std::string& name, map_of_type<device_info> def = {})
			: _base(type::device, owner, name, true)
			, m_map(std::move(def))
		{
			m_default = m_map;
		}

		const map_of_type<device_info>& get_map() const
		{
			return m_map;
		}

		const map_of_type<device_info>& get_default() const
		{
			return m_default;
		}

		void set_map(map_of_type<device_info>&& map);

		void from_default() override;
	};
}
