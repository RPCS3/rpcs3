#include "stdafx.h"
#include "util/types.hpp"
#include "Utilities/StrUtil.h"
#include "np_handler.h"

#ifdef _WIN32
#include <WS2tcpip.h>
#endif

namespace np
{
	std::string ip_to_string(u32 ip_addr)
	{
		char ip_str[16];

		inet_ntop(AF_INET, &ip_addr, ip_str, sizeof(ip_str));
		return std::string(ip_str);
	}

	std::string ether_to_string(std::array<u8, 6>& ether)
	{
		return fmt::format("%02X:%02X:%02X:%02X:%02X:%02X", ether[0], ether[1], ether[2], ether[3], ether[4], ether[5]);
	}

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
