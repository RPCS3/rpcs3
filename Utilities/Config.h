#pragma once

#include "Utilities/types.h"
#include "Utilities/Atomic.h"

#include <initializer_list>
#include <exception>
#include <utility>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <map>
#include <mutex>

namespace cfg
{
	// Convert string to signed integer
	bool try_to_int64(s64* out, const std::string& value, s64 min, s64 max);

	// Config tree entry type.
	enum class type : uint
	{
		node = 0, // cfg::node type
		boolean, // cfg::bool_entry type
		fixed_map, // cfg::map_entry type
		enumeration, // cfg::enum_entry type
		integer, // cfg::int_entry type
		string, // cfg::string_entry type
		set, // cfg::set_entry type
	};

	// Config tree entry abstract base class
	class entry_base
	{
		const type m_type;

	protected:
		// Ownerless entry constructor
		entry_base(type _type);

		// Owned entry constructor
		entry_base(type _type, class node& owner, const std::string& name);

	public:
		// Disallow copy/move constructors and assignments
		entry_base(const entry_base&) = delete;

		// Get type
		type get_type() const { return m_type; }

		// Access child node (must exist)
		entry_base& operator [](const std::string& name) const; entry_base& operator [](const char* name) const;

		// Reset defaults
		virtual void from_default() = 0;

		// Convert to string (optional)
		virtual std::string to_string() const
		{
			return{};
		}

		// Try to convert from string (optional)
		virtual bool from_string(const std::string&)
		{
			throw std::logic_error("from_string() not specified");
		}

		// Get string list (optional)
		virtual std::vector<std::string> to_list() const
		{
			return{};
		}

		// Set multiple values. Implementation-specific, optional.
		virtual bool from_list(std::vector<std::string>&&)
		{
			throw std::logic_error("from_list() not specified");
		}
	};

	// Config tree node which contains another nodes
	class node : public entry_base
	{
		std::map<std::string, entry_base*> m_nodes;

		friend class entry_base;

	public:
		// Root node constructor
		node()
			: entry_base(type::node)
		{
		}

		// Registered node constructor
		node(node& owner, const std::string& name)
			: entry_base(type::node, owner, name)
		{
		}

		// Get child nodes
		const std::map<std::string, entry_base*>& get_nodes() const
		{
			return m_nodes;
		}

		// Serialize node
		std::string to_string() const override;

		// Deserialize node
		bool from_string(const std::string& value) override;

		// Set default values
		void from_default() override;
	};

	struct bool_entry final : public entry_base
	{
		atomic_t<bool> value;

		const bool def;

		bool_entry(node& owner, const std::string& name, bool def = false)
			: entry_base(type::boolean, owner, name)
			, value(def)
			, def(def)
		{
		}

		explicit operator bool() const
		{
			return value.load();
		}

		bool_entry& operator =(bool value)
		{
			value = value;
			return *this;
		}

		void from_default() override
		{
			value = def;
		}

		std::string to_string() const override
		{
			return value.load() ? "true" : "false";
		}

		bool from_string(const std::string& value) override
		{
			if (value == "false")
				this->value = false;
			else if (value == "true")
				this->value = true;
			else
				return false;

			return true;
		}
	};

	// Value node with fixed set of possible values, each maps to a value of type T.
	template<typename T>
	struct map_entry final : public entry_base
	{
		using init_type  = std::initializer_list<std::pair<std::string, T>>;
		using map_type   = std::unordered_map<std::string, T>;
		using list_type  = std::vector<std::string>;
		using value_type = typename map_type::value_type;

		static map_type make_map(init_type init)
		{
			map_type map(init.size());

			for (const auto& v : init)
			{
				// Ensure elements are unique
				VERIFY(map.emplace(v.first, v.second).second);
			}

			return map;
		}

		static list_type make_list(init_type init)
		{
			list_type list; list.reserve(init.size());

			for (const auto& v : init)
			{
				list.emplace_back(v.first);
			}

			return list;
		}

	public:
		const map_type map;
		const list_type list; // Element list sorted in original order
		const value_type& def; // Pointer to the default value

	private:
		atomic_t<const value_type*> m_value;

	public:
		map_entry(node& owner, const std::string& name, const std::string& def, init_type init)
			: entry_base(type::fixed_map, owner, name)
			, map(make_map(init))
			, list(make_list(init))
			, def(*map.find(def))
			, m_value(&this->def)
		{
		}

		map_entry(node& owner, const std::string& name, std::size_t def_index, init_type init)
			: map_entry(owner, name, def_index < init.size() ? (init.begin() + def_index)->first : throw std::logic_error("Invalid default value index"), init)
		{
		}

		map_entry(node& owner, const std::string& name, init_type init)
			: map_entry(owner, name, 0, init)
		{
		}

		const T& get() const
		{
			return m_value.load()->second;
		}

		void from_default() override
		{
			m_value = &def;
		}

		std::string to_string() const override
		{
			return m_value.load()->first;
		}

		bool from_string(const std::string& value) override
		{
			const auto found = map.find(value);

			if (found == map.end())
			{
				return false;
			}
			else
			{
				m_value = &*found;
				return true;
			}
		}

		std::vector<std::string> to_list() const override
		{
			return list;
		}
	};

	// Value node with fixed set of possible values, each maps to an enum value of type T.
	template<typename T, bool External = false>
	class enum_entry final : public entry_base
	{
		// Value or reference
		std::conditional_t<External, atomic_t<T>&, atomic_t<T>> m_value;

	public:
		const T def;

		enum_entry(node& owner, const std::string& name, std::conditional_t<External, atomic_t<T>&, T> value)
			: entry_base(type::enumeration, owner, name)
			, m_value(value)
			, def(value)
		{
		}

		operator T() const
		{
			return m_value.load();
		}

		enum_entry& operator =(T value)
		{
			m_value = value;
			return *this;
		}

		void from_default() override
		{
			m_value = def;
		}

		std::string to_string() const override
		{
			for (std::size_t i = 0; i < sizeof(bijective<T, const char*>::map) / sizeof(bijective_pair<T, const char*>); i++)
			{
				if (bijective<T, const char*>::map[i].v1 == m_value)
				{
					return bijective<T, const char*>::map[i].v2;
				}
			}

			return{}; // TODO: ???
		}

		bool from_string(const std::string& value) override
		{
			for (std::size_t i = 0; i < sizeof(bijective<T, const char*>::map) / sizeof(bijective_pair<T, const char*>); i++)
			{
				if (bijective<T, const char*>::map[i].v2 == value)
				{
					m_value = bijective<T, const char*>::map[i].v1;
					return true;
				}
			}

			return false;
		}

		std::vector<std::string> to_list() const override
		{
			std::vector<std::string> result;

			for (std::size_t i = 0; i < sizeof(bijective<T, const char*>::map) / sizeof(bijective_pair<T, const char*>); i++)
			{
				result.emplace_back(bijective<T, const char*>::map[i].v2);
			}

			return result;
		}
	};

	// Signed 32/64-bit integer entry with custom Min/Max range.
	template<s64 Min, s64 Max>
	class int_entry final : public entry_base
	{
		static_assert(Min < Max, "Invalid cfg::int_entry range");

		// Prefer 32 bit type if possible
		using int_type = std::conditional_t<Min >= INT32_MIN && Max <= INT32_MAX, s32, s64>;

		atomic_t<int_type> m_value;

	public:
		const int_type def;

		int_entry(node& owner, const std::string& name, int_type def = std::min<int_type>(Max, std::max<int_type>(Min, 0)))
			: entry_base(type::integer, owner, name)
			, m_value(def)
			, def(def)
		{
		}

		operator int_type() const
		{
			return m_value.load();
		}

		int_entry& operator =(int_type value)
		{
			if (value < Min || value > Max)
			{
				throw fmt::exception("Value out of the valid range: %lld" HERE, s64{ value });
			}

			m_value = value;
			return *this;
		}

		void from_default() override
		{
			m_value = def;
		}

		std::string to_string() const override
		{
			return std::to_string(m_value.load());
		}

		bool from_string(const std::string& value) override
		{
			s64 result;
			if (try_to_int64(&result, value, Min, Max))
			{
				m_value = static_cast<int_type>(result);
				return true;
			}

			return false;
		}
	};

	// Alias for 32 bit int
	using int32_entry = int_entry<INT32_MIN, INT32_MAX>;

	// Alias for 64 bit int
	using int64_entry = int_entry<INT64_MIN, INT64_MAX>;

	// Simple string entry with mutex
	class string_entry final : public entry_base
	{
		mutable std::mutex m_mutex;
		std::string m_value;

	public:
		const std::string def;

		string_entry(node& owner, const std::string& name, const std::string& def = {})
			: entry_base(type::string, owner, name)
			, m_value(def)
			, def(def)
		{
		}

		operator std::string() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_value;
		}

		std::string get() const
		{
			return *this;
		}

		string_entry& operator =(const std::string& value)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_value = value;
			return *this;
		}

		std::size_t size() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_value.size();
		}

		void from_default() override
		{
			*this = def;
		}

		std::string to_string() const override
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_value;
		}

		bool from_string(const std::string& value) override
		{
			*this = value;
			return true;
		}
	};

	// Simple set entry with mutex (TODO: template for various types)
	class set_entry final : public entry_base
	{
		mutable std::mutex m_mutex;

		std::set<std::string> m_set;

	public:
		// Default value is empty list in current implementation
		set_entry(node& owner, const std::string& name)
			: entry_base(type::set, owner, name)
		{
		}

		std::set<std::string> get_set() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			return m_set;
		}

		void set_set(std::set<std::string>&& set)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_set = std::move(set);
		}

		void from_default() override
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_set = {};
		}

		std::vector<std::string> to_list() const override
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			return{ m_set.begin(), m_set.end() };
		}

		bool from_list(std::vector<std::string>&& list) override
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_set = { std::make_move_iterator(list.begin()), std::make_move_iterator(list.end()) };

			return true;
		}
	};

	// Root type with some predefined nodes. Don't change it, this is not mandatory for adding nodes.
	struct root_node : node
	{
		node core  { *this, "Core" };
		node vfs   { *this, "VFS" };
		node log   { *this, "Log" };
		node video { *this, "Video" };
		node audio { *this, "Audio" };
		node io    { *this, "Input/Output" };
		node sys   { *this, "System" };
		node net   { *this, "Net" };
		node misc  { *this, "Miscellaneous" };
	};

	// Get global configuration root instance
	extern root_node& get_root();

	// Global configuration root instance (cached reference)
	static root_node& root = get_root();
}

// Registered log channel
#define LOG_CHANNEL(name) extern logs::channel name; namespace logs { static cfg::enum_entry<logs::level, true> name(cfg::root.log, #name, ::name.enabled); }
