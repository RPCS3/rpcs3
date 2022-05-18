#include "util/types.hpp"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"

namespace np
{
	std::string ip_to_string(u32 addr);
	std::string ether_to_string(std::array<u8, 6>& ether);

	void string_to_npid(std::string_view str, SceNpId* npid);
	void string_to_online_name(std::string_view str, SceNpOnlineName* online_name);
	void string_to_avatar_url(std::string_view str, SceNpAvatarUrl* avatar_url);
	void string_to_communication_id(std::string_view str, SceNpCommunicationId* comm_id);
}
