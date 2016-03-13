#pragma once
#include <unordered_map>
#include <string>
#include "convert.h"

class config_context_t
{
public:
	class entry_base;

protected:
	class group
	{
		group* m_parent;
		config_context_t* m_cfg;
		std::string m_name;
		std::vector<std::unique_ptr<entry_base>> m_entries;

		void init();

	public:
		std::unordered_map<std::string, entry_base *> entries;

		group(config_context_t* cfg, const std::string& name);
		group(group* parent, const std::string& name);
		void set_parent(config_context_t* cfg);

		std::string name() const;
		std::string full_name() const;

		template<typename T>
		void add_entry(const std::string& name, const T& def_value)
		{
			m_entries.emplace_back(std::make_unique<entry<T>>(this, name, def_value));
		}

		template<typename T>
		T get_entry_value(const std::string& name, const T& def_value)
		{
			if (!entries[name])
				add_entry(name, def_value);

			return convert::to<T>(entries[name]->string_value());
		}

		template<typename T>
		void set_entry_value(const std::string& name, const T& value)
		{
			if (entries[name])
				entries[name]->string_value(convert::to<std::string>(value));

			else
				add_entry(name, value);
		}

		friend config_context_t;
	};

public:
	class entry_base
	{
	public:
		virtual ~entry_base() = default;
		virtual std::string name() = 0;
		virtual void to_default() = 0;
		virtual std::string string_value() = 0;
		virtual void string_value(const std::string& value) = 0;
		virtual void value_from(const entry_base* rhs) = 0;
	};

	template<typename T>
	class entry : public entry_base
	{
		T m_default_value;
		T m_value;
		group* m_parent;
		std::string m_name;

	public:
		entry(group* parent, const std::string& name, const T& default_value)
			: m_parent(parent)
			, m_name(name)
			, m_default_value(default_value)
			, m_value(default_value)
		{
			if(!parent->entries[name])
				parent->entries[name] = this;
		}

		T default_value() const
		{
			return m_default_value;
		}

		T value() const
		{
			return m_value;
		}

		void value(const T& new_value)
		{
			m_value = new_value;
		}

		std::string name() override
		{
			return m_name;
		}

		void to_default() override
		{
			value(default_value());
		}

		std::string string_value() override
		{
			return convert::to<std::string>(value());
		}

		void string_value(const std::string &new_value) override
		{
			value(convert::to<T>(new_value));
		}

		void value_from(const entry_base* rhs) override
		{
			value(static_cast<const entry*>(rhs)->value());
		}

		entry& operator = (const T& new_value)
		{
			value(new_value);
			return *this;
		}

		template<typename T2>
		entry& operator = (const T2& new_value)
		{
			value(static_cast<T>(new_value));
			return *this;
		}

		explicit operator const T&() const
		{
			return m_value;
		}
	};

private:
	std::unordered_map<std::string, group*> m_groups;

public:
	config_context_t() = default;

	void assign(const config_context_t& rhs);

	void serialize(std::ostream& stream) const;
	void deserialize(std::istream& stream);

	void set_defaults();

	std::string to_string() const;
	void from_string(const std::string&);
};
