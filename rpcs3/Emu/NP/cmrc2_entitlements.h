#pragma once

#include <regex>

#include "Emu/Cell/Modules/sceNpCommerce2.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "util/yaml.hpp"

// Tickets v2.1 MAX = 500
// Tickets v3.0 MAX = 2500 (TODO, not impl in RPCS3)
static constexpr usz TICKET_MAX_ENTITLEMENT_IDS = 500;

namespace entitlement_key
{
	static const std::string version = "Version";
	static const std::string created = "Created Date";
	static const std::string expires = "Expire Date";
	static const std::string type = "Type";
	static const std::string remaining = "Remaining Count";
	static const std::string consumed = "Consumed Count";
}

inline static const std::string entitlements_engine_version = "1.0";
inline static const std::regex entitlement_id_pattern(R"(^([A-Za-z0-9]{4,6}-[A-Za-z0-9]{9}_[0-9]{2})-[A-Za-z0-9_-]{6,}$)"); // Group 1 = Service ID

namespace commerce2
{
	struct entitlement_info
	{
		std::string id;
		u32 type;
		s32 remaining;
		u32 consumed;
		s64 created_date;
		s64 expire_date;
	};

	std::unordered_map<std::string /* service ID */, std::vector<entitlement_info>> read_entitlements(const std::string title_id);
	bool write_entitlements(const std::string title_id, const std::unordered_map<std::string /* service ID */, std::vector<entitlement_info>>& entitlements_map);
}