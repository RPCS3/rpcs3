#include "util/types.hpp"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"

namespace np
{
	std::string ip_to_string(u32 addr);
	std::string ether_to_string(std::array<u8, 6>& ether);

	void string_to_npid(const std::string& str, SceNpId* npid);
	void string_to_online_name(const std::string& str, SceNpOnlineName* online_name);
	void string_to_avatar_url(const std::string& str, SceNpAvatarUrl* avatar_url);
	void string_to_communication_id(const std::string& str, SceNpCommunicationId* comm_id);
}
