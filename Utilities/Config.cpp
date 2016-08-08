#include "stdafx.h"
#include "Config.h"

#include "yaml-cpp/yaml.h"

namespace cfg
{
	logs::channel cfg("CFG", logs::level::notice);

	entry_base::entry_base(type _type)
		: m_type(_type)
	{
		if (_type != type::node)
		{
			fmt::throw_exception<std::logic_error>("Invalid root node" HERE);
		}
	}

	entry_base::entry_base(type _type, node& owner, const std::string& name)
		: m_type(_type)
	{
		if (!owner.m_nodes.emplace(name, this).second)
		{
			fmt::throw_exception<std::logic_error>("Node already exists: %s" HERE, name);
		}
	}

	entry_base& entry_base::operator[](const std::string& name) const
	{
		if (m_type == type::node)
		{
			return *static_cast<const node&>(*this).m_nodes.at(name);
		}

		fmt::throw_exception<std::logic_error>("Invalid node type" HERE);
	}

	entry_base& entry_base::operator[](const char* name) const
	{
		if (m_type == type::node)
		{
			return *static_cast<const node&>(*this).m_nodes.at(name);
		}

		fmt::throw_exception<std::logic_error>("Invalid node type" HERE);
	}

	bool entry_base::from_string(const std::string&)
	{
		fmt::throw_exception<std::logic_error>("from_string() purecall" HERE);
	}

	bool entry_base::from_list(std::vector<std::string>&&)
	{
		fmt::throw_exception<std::logic_error>("from_list() purecall" HERE);
	}

	// Emit YAML
	static void encode(YAML::Emitter& out, const class entry_base& rhs);

	// Incrementally load config entries from YAML::Node.
	// The config value is preserved if the corresponding YAML node doesn't exist.
	static void decode(const YAML::Node& data, class entry_base& rhs);
}

bool cfg::try_to_int64(s64* out, const std::string& value, s64 min, s64 max)
{
	// TODO: this could be rewritten without exceptions (but it should be as safe as possible and provide logs)
	s64 result;
	std::size_t pos;

	try
	{
		result = std::stoll(value, &pos, 0 /* Auto-detect numeric base */);
	}
	catch (const std::exception& e)
	{
		if (out) cfg.error("cfg::try_to_int('%s'): exception: %s", value, e.what());
		return false;
	}

	if (pos != value.size())
	{
		if (out) cfg.error("cfg::try_to_int('%s'): unexpected characters (pos=%zu)", value, pos);
		return false;
	}

	if (result < min || result > max)
	{
		if (out) cfg.error("cfg::try_to_int('%s'): out of bounds (%lld..%lld)", value, min, max);
		return false;
	}

	if (out) *out = result;
	return true;
}

bool cfg::try_to_enum_value(u64* out, decltype(&fmt_class_string<int>::format) func, const std::string& value)
{
	for (u64 i = 0;; i++)
	{
		std::string var;
		func(var, i);

		if (var == value)
		{
			if (out) *out = i;
			return true;
		}

		std::string hex;
		fmt_class_string<u64>::format(hex, i);
		if (var == hex)
		{
			break;
		}
	}

	try
	{
		const auto val = std::stoull(value, nullptr, 0);

		if (out) *out = val;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

std::vector<std::string> cfg::try_to_enum_list(decltype(&fmt_class_string<int>::format) func)
{
	std::vector<std::string> result;

	for (u64 i = 0;; i++)
	{
		std::string var;
		func(var, i);

		std::string hex;
		fmt_class_string<u64>::format(hex, i);
		if (var == hex)
		{
			break;
		}

		result.emplace_back(std::move(var));
	}

	return result;
}

void cfg::encode(YAML::Emitter& out, const cfg::entry_base& rhs)
{
	switch (rhs.get_type())
	{
	case type::node:
	{
		out << YAML::BeginMap;
		for (const auto& np : static_cast<const node&>(rhs).get_nodes())
		{
			out << YAML::Key << np.first;
			out << YAML::Value; encode(out, *np.second);
		}

		out << YAML::EndMap;
		return;
	}
	case type::set:
	{
		out << YAML::BeginSeq;
		for (const auto& str : static_cast<const set_entry&>(rhs).get_set())
		{
			out << str;
		}

		out << YAML::EndSeq;
		return;
	}
	}

	out << rhs.to_string();
}

void cfg::decode(const YAML::Node& data, cfg::entry_base& rhs)
{
	switch (rhs.get_type())
	{
	case type::node:
	{
		if (data.IsScalar() || data.IsSequence())
		{
			return; // ???
		}

		for (const auto& pair : data)
		{
			if (!pair.first.IsScalar()) continue;

			// Find the key among existing nodes
			const auto name = pair.first.Scalar();
			const auto found = static_cast<node&>(rhs).get_nodes().find(name);

			if (found != static_cast<node&>(rhs).get_nodes().cend())
			{
				decode(pair.second, *found->second);
			}
			else
			{
				// ???
			}
		}

		break;
	}
	case type::set:
	{
		std::vector<std::string> values;

		if (YAML::convert<decltype(values)>::decode(data, values))
		{
			rhs.from_list(std::move(values));
		}

		break;
	}
	default:
	{
		std::string value;

		if (YAML::convert<std::string>::decode(data, value))
		{
			rhs.from_string(value);
		}

		break; // ???
	}
	}
}

std::string cfg::node::to_string() const
{
	YAML::Emitter out;
	cfg::encode(out, *this);

	return{ out.c_str(), out.size() };
}

bool cfg::node::from_string(const std::string& value)
{
	cfg::decode(YAML::Load(value), *this);
	return true;
}

void cfg::node::from_default()
{
	for (auto& node : m_nodes)
	{
		node.second->from_default();
	}
}

cfg::root_node& cfg::get_root()
{
	// Magic static
	static root_node root;
	return root;
}
