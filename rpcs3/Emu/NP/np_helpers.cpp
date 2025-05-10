#include "stdafx.h"
#include "util/types.hpp"
#include "Utilities/StrUtil.h"
#include "rpcn_client.h"

#ifdef _WIN32
#include <WS2tcpip.h>
#endif

namespace np
{
	std::string ip_to_string(u32 ip_addr)
	{
		char ip_str[16]{};

		inet_ntop(AF_INET, &ip_addr, ip_str, sizeof(ip_str));
		return std::string(ip_str);
	}

	std::string ether_to_string(std::array<u8, 6>& ether)
	{
		return fmt::format("%02X:%02X:%02X:%02X:%02X:%02X", ether[0], ether[1], ether[2], ether[3], ether[4], ether[5]);
	}

	std::string communication_id_to_string(const SceNpCommunicationId& communicationId)
	{
		return fmt::format("%s_%02d", communicationId.data, communicationId.num);
	}

	void strings_to_userinfo(std::string_view npid, std::string_view online_name, std::string_view avatar_url, SceNpUserInfo& user_info)
	{
		memset(&user_info, 0, sizeof(user_info));
		strcpy_trunc(user_info.userId.handle.data, npid);
		strcpy_trunc(user_info.name.data, online_name);
		strcpy_trunc(user_info.icon.data, avatar_url);
	}

	template <typename T>
	void onlinedata_to_presencedetails(const rpcn::friend_online_data& data, bool same_context, T& details)
	{
		memset(&details, 0, sizeof(T));
		details.state = data.online ? (same_context ? SCE_NP_BASIC_PRESENCE_STATE_IN_CONTEXT : SCE_NP_BASIC_PRESENCE_STATE_OUT_OF_CONTEXT) : SCE_NP_BASIC_PRESENCE_STATE_OFFLINE;
		strcpy_trunc(details.title, data.pr_title);
		strcpy_trunc(details.status, data.pr_status);
		strcpy_trunc(details.comment, data.pr_comment);
		details.size = std::min<u32>(::size32(data.pr_data), SCE_NP_BASIC_MAX_PRESENCE_SIZE);
		std::memcpy(details.data, data.pr_data.data(), details.size);

		if constexpr (std::is_same_v<T, SceNpBasicPresenceDetails2>)
		{
			details.struct_size = sizeof(SceNpBasicPresenceDetails2);
		}
	}

	template void onlinedata_to_presencedetails<SceNpBasicPresenceDetails>(const rpcn::friend_online_data& data, bool same_context, SceNpBasicPresenceDetails& details);
	template void onlinedata_to_presencedetails<SceNpBasicPresenceDetails2>(const rpcn::friend_online_data& data, bool same_context, SceNpBasicPresenceDetails2& details);

	void string_to_npid(std::string_view str, SceNpId& npid)
	{
		memset(&npid, 0, sizeof(npid));
		strcpy_trunc(npid.handle.data, str);
		// npid->reserved[0] = 1;
	}

	void string_to_online_name(std::string_view str, SceNpOnlineName& online_name)
	{
		memset(&online_name, 0, sizeof(online_name));
		strcpy_trunc(online_name.data, str);
	}

	void string_to_avatar_url(std::string_view str, SceNpAvatarUrl& avatar_url)
	{
		memset(&avatar_url, 0, sizeof(avatar_url));
		strcpy_trunc(avatar_url.data, str);
	}

	void string_to_communication_id(std::string_view str, SceNpCommunicationId& comm_id)
	{
		memset(&comm_id, 0, sizeof(comm_id));
		strcpy_trunc(comm_id.data, str);
	}

	bool is_valid_npid(const SceNpId& npid)
	{
		if (!std::all_of(npid.handle.data, npid.handle.data + 16, [](char c) { return std::isalnum(c) || c == '-' || c == '_' || c == 0; } )
			|| npid.handle.data[16] != 0
			|| !std::all_of(npid.handle.dummy, npid.handle.dummy + 3, [](char val) { return val == 0; }) )
		{
			return false;
		}

		return true;
	}

	bool is_same_npid(const SceNpId& npid_1, const SceNpId& npid_2)
	{
		// Unknown what this constant means
		// if (id1->reserved[0] != 1 || id2->reserved[0] != 1)
		// {
		// 	return SCE_NP_UTIL_ERROR_INVALID_NP_ID;
		// }

		if (strncmp(npid_1.handle.data, npid_2.handle.data, 16) == 0) // || id1->unk1[0] != id2->unk1[0])
		{
			return true;
		}

		// if (id1->unk1[1] != id2->unk1[1])
		// {
		// 	// If either is zero they match
		// 	if (id1->opt[4] && id2->opt[4])
		// 	{
		// 		return SCE_NP_UTIL_ERROR_NOT_MATCH;
		// 	}
		// }

		return false;
	}
}
