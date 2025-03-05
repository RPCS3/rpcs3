#include "stdafx.h"
#include "Config.h"
#include "util/types.hpp"
#include "util/yaml.hpp"

#include <charconv>

LOG_CHANNEL(cfg_log, "CFG");

template <>
void fmt_class_string<cfg::node>::format(std::string& out, u64 arg)
{
 	out += get_object(arg).to_string();
}

namespace cfg
{
	u32 _base::id_counter = 0;

	_base::_base(type _type)
		: m_type(_type), m_id(id_counter++)
	{
		if (_type != type::node)
		{
			cfg_log.fatal("Invalid root node");
		}
	}

	_base::_base(type _type, node* owner, std::string name, bool dynamic)
		: m_type(_type), m_parent(owner), m_dynamic(dynamic), m_name(std::move(name)), m_id(id_counter++)
	{
		for (const auto& node : owner->m_nodes)
		{
			if (node->get_name() == m_name)
			{
				cfg_log.fatal("Node already exists: %s", m_name);
			}
		}

		owner->m_nodes.emplace_back(this);
	}

	bool _base::from_string(std::string_view, bool)
	{
		cfg_log.fatal("cfg::_base::from_string() purecall");
		return false;
	}

	bool _base::from_list(std::vector<std::string>&&)
	{
		cfg_log.fatal("cfg::_base::from_list() purecall");
		return false;
	}

	bool _base::save(std::string_view cfg_name) const
	{
		if (fs::pending_file cfg_file(cfg_name); !!cfg_file.file)
		{
			cfg_file.file.write(to_string());
			return cfg_file.commit();
		}

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

bool try_to_int64(s64* out, std::string_view value, s64 min, s64 max)
{
	if (value.empty())
	{
		if (out) cfg_log.error("cfg::try_to_int64(): called with an empty string");
		return false;
	}

	s64 result;
	const char* start = value.data();
	const char* end = start + value.size();
	int base = 10;
	int sign = +1;

	if (start[0] == '-')
	{
		sign = -1;
		start += 1;
	}

	if (start[0] == '0' && value.size() >= 2 && (start[1] == 'x' || start[1] == 'X'))
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
		if (out) cfg_log.error("cfg::try_to_int64('%s'): out of bounds (val=%d, min=%d, max=%d)", value, result, min, max);
		return false;
	}

	if (out) *out = result;
	return true;
}

std::vector<std::string> cfg::make_uint_range(u64 min, u64 max)
{
	return {std::to_string(min), std::to_string(max)};
}

bool try_to_uint64(u64* out, std::string_view value, u64 min, u64 max)
{
	if (value.empty())
	{
		if (out) cfg_log.error("cfg::try_to_uint64(): called with an empty string");
		return false;
	}

	u64 result;
	const char* start = value.data();
	const char* end = start + value.size();
	int base = 10;

	if (start[0] == '0' && value.size() >= 2 && (start[1] == 'x' || start[1] == 'X'))
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
		if (out) cfg_log.error("cfg::try_to_uint64('%s'): out of bounds (val=%u, min=%u, max=%u)", value, result, min, max);
		return false;
	}

	if (out) *out = result;
	return true;
}

std::vector<std::string> cfg::make_float_range(f64 min, f64 max)
{
	return {std::to_string(min), std::to_string(max)};
}

bool try_to_float(f64* out, std::string_view value, f64 min, f64 max)
{
	if (value.empty())
	{
		if (out) cfg_log.error("cfg::try_to_float(): called with an empty string");
		return false;
	}

	// std::from_chars float is yet to be implemented on Xcode it seems
	// And strtod doesn't support ranged view so we need to ensure it meets a null terminator
	const std::string str = std::string{value};

	char* end_check{};
	const double result = std::strtod(str.data(), &end_check);

	if (end_check != str.data() + str.size())
	{
		if (out) cfg_log.error("cfg::try_to_float('%s'): invalid float", value);
		return false;
	}

	if (result < min || result > max)
	{
		if (out) cfg_log.error("cfg::try_to_float('%s'): out of bounds (val=%f, min=%f, max=%f)", value, result, min, max);
		return false;
	}

	if (out) *out = result;
	return true;
}

bool try_to_string(std::string* out, const f64& value)
{
#ifdef __APPLE__
	if (out) *out = std::to_string(value);
	return true;
#else
	std::array<char, 32> str{};

	if (auto [ptr, ec] = std::to_chars(str.data(), str.data() + str.size(), value, std::chars_format::fixed); ec == std::errc())
	{
		if (out) *out = std::string(str.data(), ptr);
		return true;
	}
	else
	{
		if (out) cfg_log.error("cfg::try_to_string(): could not convert value '%f' to string. error='%s'", value, std::make_error_code(ec).message());
		return false;
	}
#endif
}

bool cfg::try_to_enum_value(u64* out, decltype(&fmt_class_string<int>::format) func, std::string_view value)
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
	const char* start = value.data();
	const char* end = start + value.size();
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
		if (out) cfg_log.error("cfg::try_to_enum_value('%s'): out of bounds(val=%u, min=0, max=%u)", value, result, max);
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
	case type::map:
	{
		out << YAML::BeginMap;
		for (const auto& np : static_cast<const map_entry&>(rhs).get_map())
		{
			out << YAML::Key << np.first;
			out << YAML::Value << fmt::format("%s", np.second);
		}
		out << YAML::EndMap;
		return;
	}
	case type::node_map:
	{
		//out << YAML::Block; // Does nothing, output is in Flow mode still (TODO)
		out << YAML::BeginMap;
		for (const auto& np : static_cast<const node_map_entry&>(rhs).get_map())
		{
			out << YAML::Key << np.first;
			out << YAML::Value << fmt::format("%s", np.second);
		}
		out << YAML::EndMap;
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
	case type::device:
	{
		out << YAML::BeginMap;
		for (const auto& [key, info] : static_cast<const device_entry&>(rhs).get_map())
		{
			out << YAML::Key << key;
			out << YAML::BeginMap;
			out << YAML::Key << "Path" << YAML::Value << info.path;
			out << YAML::Key << "Serial" << YAML::Value << info.serial;
			out << YAML::Key << "VID" << YAML::Value << info.vid;
			out << YAML::Key << "PID" << YAML::Value << info.pid;
			out << YAML::EndMap;
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
	case type::map:
	case type::node_map:
	{
		if (!data.IsMap())
		{
			return;
		}

		map_of_type<std::string> values;

		for (const auto& pair : data)
		{
			if (!pair.first.IsScalar() || !pair.second.IsScalar()) continue;

			values.emplace(pair.first.Scalar(), pair.second.Scalar());
		}

		static_cast<map_entry&>(rhs).set_map(std::move(values));
		break;
	}
	case type::log:
	{
		if (data.IsScalar() || data.IsSequence())
		{
			return; // ???
		}

		map_of_type<logs::level> values;

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
	case type::device:
	{
		if (!data.IsMap())
		{
			return; // ???
		}

		map_of_type<device_info> values;

		for (const auto& pair : data)
		{
			if (!pair.first.IsScalar() || !pair.second.IsMap()) continue;

			device_info info{};

			for (const auto& key_value : pair.second)
			{
				if (!key_value.first.IsScalar() || !key_value.second.IsScalar()) continue;

				if (key_value.first.Scalar() == "Path")
					info.path = key_value.second.Scalar();

				if (key_value.first.Scalar() == "Serial")
					info.serial = key_value.second.Scalar();

				if (key_value.first.Scalar() == "VID")
					info.vid = key_value.second.Scalar();

				if (key_value.first.Scalar() == "PID")
					info.pid = key_value.second.Scalar();
			}

			values.emplace(pair.first.Scalar(), std::move(info));
		}

		static_cast<device_entry&>(rhs).set_map(std::move(values));
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

bool cfg::node::from_string(std::string_view value, bool dynamic)
{
	auto [result, error] = yaml_load(std::string(value));

	if (error.empty())
	{
		cfg::decode(result, *this, dynamic);
		return true;
	}

	cfg_log.error("Failed to load node: %s", error);
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

std::string cfg::map_entry::get_value(std::string_view key)
{
	if (auto it = m_map.find(key); it != m_map.end())
	{
		return it->second;
	}

	return {};
}

void cfg::map_entry::set_value(std::string key, std::string value)
{
	m_map[std::move(key)] = std::move(value);
}

void cfg::map_entry::set_map(map_of_type<std::string>&& map)
{
	m_map = std::move(map);
}

void cfg::map_entry::erase(std::string_view key)
{
	if (auto it = m_map.find(key); it != m_map.end())
	{
		m_map.erase(it);
	}
}

void cfg::map_entry::from_default()
{
	set_map({});
}

void cfg::log_entry::set_map(map_of_type<logs::level>&& map)
{
	m_map = std::move(map);
}

void cfg::log_entry::from_default()
{
	set_map({});
}

std::pair<u16, u16> cfg::device_info::get_usb_ids() const
{
	auto string_to_hex = [](const std::string& str) -> u16
	{
		u16 value = 0x0000;
		if (!str.empty() && std::from_chars(str.data(), str.data() + str.size(), value, 16).ec != std::errc{})
			cfg_log.error("Failed to parse hex from string \"%s\"", str);
		return value;
	};
	return {string_to_hex(vid), string_to_hex(pid)};
}

void cfg::device_entry::set_map(map_of_type<device_info>&& map)
{
	m_map = std::move(map);
}

void cfg::device_entry::from_default()
{
	m_map = m_default;
}
