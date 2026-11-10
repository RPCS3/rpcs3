#include "stdafx.h"
#include "Emu/NP/cmrc2_entitlements.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"

#include "Utilities/File.h"
#include "util/logs.hpp"

LOG_CHANNEL(cmrc_entitlements, "cmrc2_entitlements");

namespace commerce2
{
	static std::vector<entitlement_info> flatten_entitlements(const std::unordered_map<std::string /* service ID */, std::vector<entitlement_info>>& entitlements)
	{
		std::vector<entitlement_info> flat;

		usz total = 0;
		for (const auto& [_, ents] : entitlements)
		{
			total += ents.size();
		}
		flat.reserve(total);

		for (const auto& [_, ents] : entitlements)
		{
			flat.insert(flat.end(), ents.begin(), ents.end());
		}

		return flat;
	}

	std::unordered_map<std::string /* service ID */, std::vector<entitlement_info>> read_entitlements(const std::string title_id)
	{
		if (title_id.empty())
		{
			cmrc_entitlements.error("Cannot load entitlements; no title ID provided");
			return {};
		}

		const std::string path = rpcs3::utils::get_commerce_dir_by_serial(title_id) + "/entitlements.yml";

		fs::file entitlement_file{path, fs::read + fs::create};
		if (!entitlement_file)
		{
			cmrc_entitlements.error("Error opening entitlements file: %s", path);
			return {};
		}

		auto [root, error] = yaml_load(entitlement_file.to_string());

		if (!error.empty())
		{
			cmrc_entitlements.error("Error parsing %s: %s", path, error);
			return {};
		}

		// Version checking
		std::string version;
		if (const auto version_node = root[entitlement_key::version])
		{
			version = version_node.Scalar();

			if (version != entitlements_engine_version)
			{
				cmrc_entitlements.error("File version %s does not match entitlements engine target version %s (location: %s, file: %s)",
					version, entitlements_engine_version, get_yaml_node_location(version_node), path);
				return {};
			}

			// We don't need the Version node in local memory anymore
			root.remove(entitlement_key::version);
		}
		else
		{
			cmrc_entitlements.error("No '%s' entry found. Entitlements engine version = %s (file: %s)", entitlement_key::version, entitlements_engine_version, path);
			return {};
		}

		s32 total_valid = 0;
		std::unordered_map<std::string /* service ID */, std::vector<entitlement_info>> entitlements_map;

		// Go through each entitlement in file
		for (auto pair : root)
		{
			// Get Entitlement ID
			std::string err;
			const std::string& ent_id = pair.first.Scalar();

			if (ent_id.empty())
			{
				cmrc_entitlements.error("Skipping empty entitlement (location: %s, file: %s)",
					get_yaml_node_location(pair.first), path);
				continue;
			}

			std::smatch match;
			if (!std::regex_match(ent_id, match, entitlement_id_pattern))
			{
				cmrc_entitlements.error("Skipping entitlement %s: ID must match regex ^([A-Za-z0-9]{4,6}-[A-Za-z0-9]{9}_[0-9]{2})-[A-Za-z0-9_-]{6,} (location: %s, file: %s)",
					ent_id, get_yaml_node_location(pair.first), path);
				continue;
			}

			// Extract Service ID from Entitlement ID
			const std::string service_id = match[1].str();

			if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
			{
				cmrc_entitlements.error("Skipping entitlement %s: expected Map, found %s (location: %s, file: %s)",
					ent_id, yml_type, get_yaml_node_location(pair.second), path);
				continue;
			}

			// Get 'Type' field
			const auto type_field = pair.second[entitlement_key::type];
			if (!type_field || !type_field.IsScalar())
			{
				cmrc_entitlements.error("Skipping entitlement %s: Invalid or missing 'Type' field (location: %s, file: %s)",
					ent_id, get_yaml_node_location(pair.second), path);
				continue;
			}

			u32 type = get_yaml_node_value<u32>(type_field, err);
			if (!err.empty() || (type != SCE_NP_ENTITLEMENT_TYPE_NON_CONSUMABLE && type != SCE_NP_ENTITLEMENT_TYPE_CONSUMABLE))
			{
				cmrc_entitlements.error("Skipping entitlement %s: Invalid value for 'Type' field, expect 0 or 1 but got '%s' (location: %s, file: %s)",
					ent_id, type_field.Scalar(), get_yaml_node_location(pair.second), path);
				continue;
			}

			// Get 'Created Date' field
			const auto created_field = pair.second[entitlement_key::created];
			if (!created_field || !created_field.IsScalar())
			{
				cmrc_entitlements.error("Skipping entitlement %s: Invalid or missing 'Created Date' field (location: %s, file: %s)",
					ent_id, get_yaml_node_location(pair.second), path);
				continue;
			}

			s64 created_date = get_yaml_node_value<s64>(created_field, err);
			if (!err.empty() || created_date < 0)
			{
				cmrc_entitlements.error("Skipping entitlement %s: Invalid value for 'Created Date' field, expected 64-bit timestamp but got '%s' (location: %s, file: %s)",
					ent_id, created_field.Scalar(), get_yaml_node_location(pair.second), path);
				continue;
			}

			// Default values for non-consumables
			s64 expire_date = -1;
			s32 remaining = -1;
			u32 consumed = 0;

			if (type == SCE_NP_ENTITLEMENT_TYPE_CONSUMABLE)
			{
				// Get 'Expire Date' field
				const auto expires_field = pair.second[entitlement_key::expires];
				if (!expires_field || !expires_field.IsScalar())
				{
					cmrc_entitlements.error("Skipping entitlement %s: Invalid or missing 'Expire Date' field (location: %s, file: %s)",
						ent_id, get_yaml_node_location(pair.second), path);
					continue;
				}

				expire_date = get_yaml_node_value<s64>(expires_field, err);
				if (!err.empty())
				{
					cmrc_entitlements.error("Skipping entitlement %s: Invalid value for 'Expire Date' field, expected 64-bit timestamp but got '%s' (location: %s, file: %s)",
						ent_id, expires_field.Scalar(), get_yaml_node_location(pair.second), path);
					continue;
				}

				// Get 'Remaining Count' field
				const auto remaining_field = pair.second[entitlement_key::remaining];
				if (!remaining_field || !remaining_field.IsScalar())
				{
					cmrc_entitlements.error("Skipping entitlement %s: Invalid or missing 'Remaining Count' field (location: %s, file: %s)",
						ent_id, get_yaml_node_location(pair.second), path);
					continue;
				}

				remaining = get_yaml_node_value<s32>(remaining_field, err);
				if (!err.empty())
				{
					cmrc_entitlements.error("Skipping entitlement %s: Invalid value for 'Remaining Count' field, expected an integer but got '%s' (location: %s, file: %s)",
						ent_id, remaining_field.Scalar(), get_yaml_node_location(pair.second), path);
					continue;
				}

				// Get 'Consumed Count' field
				const auto consumed_field = pair.second[entitlement_key::consumed];
				if (!consumed_field || !consumed_field.IsScalar())
				{
					cmrc_entitlements.error("Skipping entitlement %s: Invalid or missing 'Consumed Count' field (location: %s, file: %s)",
						ent_id, get_yaml_node_location(pair.second), path);
					continue;
				}

				consumed = get_yaml_node_value<u32>(consumed_field, err);
				if (!err.empty())
				{
					cmrc_entitlements.error("Skipping entitlement %s: Invalid value for 'Consumed Count' field, expected an integer but got '%s' (location: %s, file: %s)",
						ent_id, consumed_field.Scalar(), get_yaml_node_location(pair.second), path);
					continue;
				}
			}

			if (ent_id.size() > SCE_NP_ENTITLEMENT_ID_SIZE)
			{
				cmrc_entitlements.error("Skipping entitlement %s: Length %d exceeds SCE_NP_ENTITLEMENT_ID_SIZE(32) (location: %s, file: %s)",
					ent_id, ent_id.size(), get_yaml_node_location(pair.first), path);
				continue;
			}

			auto ent = entitlement_info{
				.id = ent_id,
				.type = type,
				.remaining = remaining,
				.consumed = consumed,
				.created_date = created_date,
				.expire_date = expire_date
			};

			auto& service_ents = entitlements_map[service_id];
			auto ent_it = std::find_if(service_ents.begin(), service_ents.end(),
				[&ent](const entitlement_info& existing)
				{
					return existing.id.compare(ent.id) == 0;
				});

			if (ent_it != service_ents.end())
			{
				cmrc_entitlements.error("Duplicate entitlement %s, overriding previous entry data (location: %s, file: %s)",
					ent_id, get_yaml_node_location(pair.first), path);
				*ent_it = ent;
			}
			else
			{
				entitlements_map[service_id].push_back(ent);
				total_valid++;
			}
		}

		cmrc_entitlements.success("Loaded %d entitlements from disk", total_valid);
		return entitlements_map;
	}

	bool write_entitlements(const std::string title_id, const std::unordered_map<std::string /* service ID */, std::vector<entitlement_info>>& entitlements_map)
	{
		if (title_id.empty())
		{
			cmrc_entitlements.error("Cannot save entitlements; no title ID provided");
			return false;
		}

		std::string path = rpcs3::utils::get_commerce_dir_by_serial(title_id) + "/entitlements.yml";

		fs::file entitlement_file{path, fs::rewrite};
		if (!entitlement_file)
		{
			cmrc_entitlements.error("Failed to open entitlements file %s (%s)", path, fs::g_tls_error);
			return false;
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << entitlement_key::version << entitlements_engine_version;

		const auto& entitlements = flatten_entitlements(entitlements_map);
		for (const auto& ent : entitlements)
		{
			out << YAML::Newline << YAML::Newline;
			out << ent.id << YAML::BeginMap;

			out << entitlement_key::type	  << ent.type;
			out << entitlement_key::created	  << ent.created_date;

			if (ent.type == SCE_NP_ENTITLEMENT_TYPE_CONSUMABLE)
			{
				out << entitlement_key::expires << ent.expire_date;
				out << entitlement_key::remaining << ent.remaining;
				out << entitlement_key::consumed  << ent.consumed;
			}

			out << YAML::EndMap;
			cmrc_entitlements.trace("Written entitlement %s to YAML emitter (%s)", ent.id, path);
		}

		entitlement_file.write(out.c_str(), out.size());

		cmrc_entitlements.success("Saved %u entitlements to file (%s)", entitlements.size(), path);

		return true;
	}
}