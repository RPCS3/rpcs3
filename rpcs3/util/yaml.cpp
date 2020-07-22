#include "util/yaml.hpp"
#include "Utilities/types.h"
#include "Utilities/cheat_info.h"
#include "Utilities/Config.h"

namespace YAML
{
	template <>
	struct convert<cheat_info>
	{
		static bool decode(const Node& node, cheat_info& rhs)
		{
			if (!node || node.Type() != NodeType::Map)
			{
				return false;
			}

			if (const auto key_node = node[cheat_key::type]; key_node && key_node.IsScalar())
			{
				u64 type64 = 0;
				if (!cfg::try_to_enum_value(&type64, &fmt_class_string<cheat_type>::format, key_node.Scalar()))
					return false;
				if (type64 >= cheat_type_max)
					return false;
				rhs.type = cheat_type{::narrow<u8>(type64)};
			}
			else
			{
				return false;
			}

			if (const auto key_node = node[cheat_key::value])
			{
				switch (rhs.type)
				{
				case cheat_type::signed_8_cheat:
				case cheat_type::signed_16_cheat:
				case cheat_type::signed_32_cheat:
				case cheat_type::signed_64_cheat:
					rhs.value.s = key_node.as<s64>();
					break;
				case cheat_type::unsigned_8_cheat:
				case cheat_type::unsigned_16_cheat:
				case cheat_type::unsigned_32_cheat:
				case cheat_type::unsigned_64_cheat:
					rhs.value.u = key_node.as<u64>();
					break;
				default:
					return false;
				}
			}
			else
			{
				return false;
			}

			if (const auto key_node = node[cheat_key::script]; key_node && key_node.IsScalar())
			{
				rhs.red_script = key_node.Scalar();
			}
			else
			{
				return false;
			}

			if (const auto key_node = node[cheat_key::apply_on_boot])
			{
				rhs.apply_on_boot = key_node.as<bool>(false);
			}
			else
			{
				return false;
			}

			return true;
		}
	};
} // namespace YAML

std::pair<YAML::Node, std::string> yaml_load(const std::string& from)
{
	YAML::Node result;

	try
	{
		result = YAML::Load(from);
	}
	catch(const std::exception& e)
	{
		return{YAML::Node(), std::string("YAML exception:\n") + e.what()};
	}

	return{result, ""};
}

template <typename T>
T get_yaml_node_value(YAML::Node node, std::string& error_message)
{
	try
	{
		return node.as<T>();
	}
	catch (const std::exception& e)
	{
		error_message = e.what();
	}

	return {};
}

template u32 get_yaml_node_value<u32>(YAML::Node, std::string&);
template u64 get_yaml_node_value<u64>(YAML::Node, std::string&);
template f64 get_yaml_node_value<f64>(YAML::Node, std::string&);
template cheat_info get_yaml_node_value<cheat_info>(YAML::Node, std::string&);
