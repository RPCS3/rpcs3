#pragma once

#include "Utilities/types.h"
#include "Utilities/StrFmt.h"
#include "util/logs.hpp"
#include "util/atomic.hpp"
#include "util/shared_cptr.hpp"

#include <utility>
#include <string>
#include <vector>
#include <set>
#include <map>

namespace cfg
{
	// Format min and max values
	std::vector<std::string> make_int_range(s64 min, s64 max);

	// Convert string to signed integer
	bool try_to_int64(s64* out, const std::string& value, s64 min, s64 max);

	// Internal hack
	bool try_to_enum_value(u64* out, decltype(&fmt_class_string<int>::format) func, const std::string&);

	// Internal hack
	std::vector<std::string> try_to_enum_list(decltype(&fmt_class_string<int>::format) func);

	// Config tree entry type.
	enum class type : uint
	{
		node = 0, // cfg::node type
		_bool, // cfg::_bool type
		_enum, // cfg::_enum type
		_int, // cfg::_int type
		string, // cfg::string type
		set, // cfg::set_entry type
		log,
	};

	// Config tree entry abstract base class
	class _base
	{
		const type m_type;

	protected:
		bool m_dynamic = true;

		// Ownerless entry constructor
		_base(type _type);

		// Owned entry constructor
		_base(type _type, class node* owner, const std::string& name, bool dynamic = true);

	public:
		_base(const _base&) = delete;

		_base& operator=(const _base&) = delete;

		// Get type
		type get_type() const { return m_type; }

		// Get dynamic property for reloading configs during games
		bool get_is_dynamic() const { return m_dynamic; };

		// Reset defaults
		virtual void from_default() = 0;

		// Convert to string (optional)
		virtual std::string to_string() const
		{
			return{};
		}

		// Try to convert from string (optional)
		virtual bool from_string(const std::string&, bool /*dynamic*/ = false);

		// Get string list (optional)
		virtual std::vector<std::string> to_list() const
		{
			return{};
		}

		// Set multiple values. Implementation-specific, optional.
		virtual bool from_list(std::vector<std::string>&&);
	};

	// Config tree node which contains another nodes
	class node : public _base
	{
		std::vector<std::pair<std::string, _base*>> m_nodes;

		friend class _base;

	public:
		// Root node constructor
		node()
			: _base(type::node)
		{
		}

		// Registered node constructor
		node(node* owner, const std::string& name, bool dynamic = true)
			: _base(type::node, owner, name, dynamic)
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
		bool from_string(const std::string& value, bool dynamic = false) override;

		// Set default values
		void from_default() override;
	};

	class _bool final : public _base
	{
		atomic_t<bool> m_value;

	public:
		bool def;

		_bool(node* owner, const std::string& name, bool def = false, bool dynamic = false)
			: _base(type::_bool, owner, name, dynamic)
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

		bool from_string(const std::string& value, bool /*dynamic*/ = false) override
		{
			if (value == "false")
				m_value = false;
			else if (value == "true")
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
	class _enum final : public _base
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

		bool from_string(const std::string& value, bool /*dynamic*/ = false) override
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
		using int_type = std::conditional_t<Min >= INT32_MIN && Max <= INT32_MAX, s32, s64>;

		atomic_t<int_type> m_value;

	public:
		int_type def;

		// Expose range
		static const s64 max = Max;
		static const s64 min = Min;

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

		bool from_string(const std::string& value, bool /*dynamic*/ = false) override
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
			m_value = static_cast<int_type>(value);
		}

		std::vector<std::string> to_list() const override
		{
			return make_int_range(Min, Max);
		}
	};

	// Alias for 32 bit int
	using int32 = _int<INT32_MIN, INT32_MAX>;

	// Alias for 64 bit int
	using int64 = _int<INT64_MIN, INT64_MAX>;

	// Simple string entry with mutex
	class string final : public _base
	{
		const std::string m_name;

		stx::atomic_cptr<std::string> m_value;

	public:
		std::string def;

		string(node* owner, std::string name, std::string def = {}, bool dynamic = false)
			: _base(type::string, owner, name, dynamic)
			, m_name(std::move(name))
			, m_value(m_value.make(def))
			, def(std::move(def))
		{
		}

		operator std::string() const
		{
			return *m_value.load().get();
		}

		std::pair<const std::string&, stx::shared_cptr<std::string>> get() const
		{
			auto v = m_value.load();

			if (auto s = v.get())
			{
				return {*s, std::move(v)};
			}
			else
			{
				static const std::string _empty;
				return {_empty, {}};
			}
		}

		const std::string& get_name() const
		{
			return m_name;
		}

		void from_default() override;

		std::string to_string() const override
		{
			return *m_value.load().get();
		}

		bool from_string(const std::string& value, bool /*dynamic*/ = false) override
		{
			m_value = m_value.make(value);
			return true;
		}
	};

	// Simple set entry with mutex (TODO: template for various types)
	class set_entry final : public _base
	{
		std::set<std::string> m_set;

	public:
		// Default value is empty list in current implementation
		set_entry(node* owner, const std::string& name, bool dynamic = false)
			: _base(type::set, owner, name, dynamic)
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
			return{ m_set.begin(), m_set.end() };
		}

		bool from_list(std::vector<std::string>&& list) override
		{
			m_set = { std::make_move_iterator(list.begin()), std::make_move_iterator(list.end()) };

			return true;
		}
	};

	class log_entry final : public _base
	{
		std::map<std::string, logs::level> m_map;

	public:
		log_entry(node* owner, const std::string& name)
			: _base(type::log, owner, name)
		{
		}

		const std::map<std::string, logs::level>& get_map() const
		{
			return m_map;
		}

		void set_map(std::map<std::string, logs::level>&& map);

		void from_default() override;
	};
}
