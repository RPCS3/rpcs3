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

	void string_to_npid(const std::string& str, SceNpId* npid)
	{
		memset(npid, 0, sizeof(SceNpId));
		strcpy_trunc(npid->handle.data, str);
		// npid->reserved[0] = 1;
	}

	void string_to_online_name(const std::string& str, SceNpOnlineName* online_name)
	{
		strcpy_trunc(online_name->data, str);
	}

	void string_to_avatar_url(const std::string& str, SceNpAvatarUrl* avatar_url)
	{
		strcpy_trunc(avatar_url->data, str);
	}

	void string_to_communication_id(const std::string& str, SceNpCommunicationId* comm_id)
	{
		strcpy_trunc(comm_id->data, str);
	}
}
