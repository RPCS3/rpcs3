#pragma once

#include "util/types.hpp"
#include "Emu/Cell/Modules/sceNp.h"
#include "rpcn_client.h"

namespace np
{
	std::string ip_to_string(u32 addr);
	std::string ether_to_string(std::array<u8, 6>& ether);
	std::string communication_id_to_string(const SceNpCommunicationId& communicationId);

	void string_to_npid(std::string_view str, SceNpId& npid);
	void string_to_online_name(std::string_view str, SceNpOnlineName& online_name);
	void string_to_avatar_url(std::string_view str, SceNpAvatarUrl& avatar_url);
	void string_to_communication_id(std::string_view str, SceNpCommunicationId& comm_id);
	void strings_to_userinfo(std::string_view npid, std::string_view online_name, std::string_view avatar_url, SceNpUserInfo& user_info);

	template <typename T>
	void onlinedata_to_presencedetails(const rpcn::friend_online_data& data, bool same_context, T& details);

	bool is_valid_npid(const SceNpId& npid);
	bool is_same_npid(const SceNpId& npid_1, const SceNpId& npid_2);
}
