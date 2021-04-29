#include "stdafx.h"
#include "Config.h"
#include "util/types.hpp"

#include "util/yaml.hpp"

#include <charconv>

LOG_CHANNEL(cfg_log, "CFG");

namespace cfg
{
	_base::_base(type _type)
		: m_type(_type)
	{
		if (_type != type::node)
		{
			cfg_log.fatal("Invalid root node");
		}
	}

	_base::_base(type _type, node* owner, const std::string& name, bool dynamic)
		: m_type(_type), m_dynamic(dynamic), m_name(name)
	{
		for (const auto& node : owner->m_nodes)
		{
			if (node->get_name() == name)
			{
				cfg_log.fatal("Node already exists: %s", name);
			}
		}

		owner->m_nodes.emplace_back(this);
	}

	bool _base::from_string(const std::string&, bool)
	{
		cfg_log.fatal("cfg::_base::from_string() purecall");
		return false;
	}

	bool _base::from_list(std::vector<std::string>&&)
	{
		cfg_log.fatal("cfg::_base::from_list() purecall");
		return false;
	}

	// Emit YAML
	static void encode(YAML::Emitter& out, const class _base& rhs);

	// Incrementally load config entries from YAML::Node.
	// The config value is preserved if the corresponding YAML node doesn't exist.
	static void decode(const YAML::Node& data, class _base& rhs, bool dynamic = false);
}

std::vector<std::string> cfg::make_int_range(s64 min, s64 max)
{
	return {std::to_string(min), std::to_string(max)};
}

bool cfg::try_to_int64(s64* out, const std::string& value, s64 min, s64 max)
{
	s64 result;
	const char* start = &value.front();
	const char* end = &value.back() + 1;
	int base = 10;
	int sign = +1;

	if (start[0] == '-')
	{
		sign = -1;
		start += 1;
	}

	if (start[0] == '0' && (start[1] == 'x' || start[1] == 'X'))
	{
		// Limited hex support
		base = 16;
		start += 2;
	}

	const auto ret = std::from_chars(start, end, result, base);

	if (ret.ec != std::errc() || ret.ptr != end || (start[0] == '-' && sign < 0))
	{
		if (out) cfg_log.error("cfg::try_to_int64('%s'): invalid integer", value);
		return false;
	}

	result *= sign;

	if (result < min || result > max)
	{
		if (out) cfg_log.error("cfg::try_to_int64('%s'): out of bounds (%d..%d)", value, min, max);
		return false;
	}

	if (out) *out = result;
	return true;
}

std::vector<std::string> cfg::make_uint_range(u64 min, u64 max)
{
	return {std::to_string(min), std::to_string(max)};
}

bool cfg::try_to_uint64(u64* out, const std::string& value, u64 min, u64 max)
{
	u64 result;
	const char* start = &value.front();
	const char* end = &value.back() + 1;
	int base = 10;

	if (start[0] == '0' && (start[1] == 'x' || start[1] == 'X'))
	{
		// Limited hex support
		base = 16;
		start += 2;
	}

	const auto ret = std::from_chars(start, end, result, base);

	if (ret.ec != std::errc() || ret.ptr != end)
	{
		if (out) cfg_log.error("cfg::try_to_uint64('%s'): invalid integer", value);
		return false;
	}

	if (result < min || result > max)
	{
		if (out) cfg_log.error("cfg::try_to_uint64('%s'): out of bounds (%u..%u)", value, min, max);
		return false;
	}

	if (out) *out = result;
	return true;
}

bool cfg::try_to_enum_value(u64* out, decltype(&fmt_class_string<int>::format) func, const std::string& value)
{
	u64 max = umax;

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

		max = i;
	}

	u64 result;
	const char* start = &value.front();
	const char* end = &value.back() + 1;
	int base = 10;

	if (start[0] == '0' && (start[1] == 'x' || start[1] == 'X'))
	{
		// Limited hex support
		base = 16;
		start += 2;
	}

	const auto ret = std::from_chars(start, end, result, base);

	if (ret.ec != std::errc() || ret.ptr != end)
	{
		if (out) cfg_log.error("cfg::try_to_enum_value('%s'): invalid enum or integer", value);
		return false;
	}

	if (result > max)
	{
		if (out) cfg_log.error("cfg::try_to_enum_value('%s'): out of bounds(0..%u)", value, max);
		return false;
	}

	if (out) *out = result;
	return true;
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

void cfg::encode(YAML::Emitter& out, const cfg::_base& rhs)
{
	switch (rhs.get_type())
	{
	case type::node:
	{
		out << YAML::BeginMap;
		for (const auto& node : static_cast<const node&>(rhs).get_nodes())
		{
			out << YAML::Key << node->get_name();
			out << YAML::Value;
			encode(out, *node);
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
	case type::log:
	{
		out << YAML::BeginMap;
		for (const auto& np : static_cast<const log_entry&>(rhs).get_map())
		{
			if (np.second == logs::level::notice) continue;
			out << YAML::Key << np.first;
			out << YAML::Value << fmt::format("%s", np.second);
		}

		out << YAML::EndMap;
		return;
	}
	default:
	{
		out << rhs.to_string();
		return;
	}
	}
}

void cfg::decode(const YAML::Node& data, cfg::_base& rhs, bool dynamic)
{
	if (dynamic && !rhs.get_is_dynamic())
	{
		return;
	}

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
			for (const auto& node : static_cast<node&>(rhs).get_nodes())
			{
				if (node->get_name() == pair.first.Scalar())
				{
					decode(pair.second, *node, dynamic);
				}
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
	case type::log:
	{
		if (data.IsScalar() || data.IsSequence())
		{
			return; // ???
		}

		std::map<std::string, logs::level> values;

		for (const auto& pair : data)
		{
			if (!pair.first.IsScalar() || !pair.second.IsScalar()) continue;

			u64 value;
			if (cfg::try_to_enum_value(&value, &fmt_class_string<logs::level>::format, pair.second.Scalar()))
			{
				values.emplace(pair.first.Scalar(), static_cast<logs::level>(static_cast<int>(value)));
			}
		}

		static_cast<log_entry&>(rhs).set_map(std::move(values));
		break;
	}
	default:
	{
		std::string value;

		if (YAML::convert<std::string>::decode(data, value))
		{
			rhs.from_string(value, dynamic);
		}

		break; // ???
	}
	}
}

std::string cfg::node::to_string() const
{
	YAML::Emitter out;
	cfg::encode(out, *this);

	return {out.c_str(), out.size()};
}

bool cfg::node::from_string(const std::string& value, bool dynamic)
{
	auto [result, error] = yaml_load(value);

	if (error.empty())
	{
		cfg::decode(result, *this, dynamic);
		return true;
	}

	cfg_log.fatal("Failed to load node: %s", error);
	return false;
}

void cfg::node::from_default()
{
	for (auto& node : m_nodes)
	{
		node->from_default();
	}
}

void cfg::_bool::from_default()
{
	m_value = def;
}

void cfg::string::from_default()
{
	m_value = def;
}

void cfg::set_entry::from_default()
{
	m_set = {};
}

void cfg::log_entry::set_map(std::map<std::string, logs::level>&& map)
{
	m_map = std::move(map);
}

void cfg::log_entry::from_default()
{
	set_map({});
}
