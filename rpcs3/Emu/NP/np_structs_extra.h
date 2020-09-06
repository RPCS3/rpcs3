#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/NP/rpcn_client.h"

namespace extra_nps
{
	void print_sigoptparam(const SceNpMatching2SignalingOptParam* opt);
	void print_bin_attr(const SceNpMatching2BinAttr* bin);
	void print_presence_data(const SceNpMatching2PresenceOptionData* opt);
	void print_range_filter(const SceNpMatching2RangeFilter* filt);
	void print_room_data_internal(const SceNpMatching2RoomDataInternal* room);
	void print_room_member_data_internal(const SceNpMatching2RoomMemberDataInternal* member);

	void print_createjoinroom(const SceNpMatching2CreateJoinRoomRequest* req);
	void print_create_room_resp(const SceNpMatching2CreateJoinRoomResponse* resp);
	void print_joinroom(const SceNpMatching2JoinRoomRequest* req);
	void print_search_room(const SceNpMatching2SearchRoomRequest* req);
	void print_set_roomdata_ext_req(const SceNpMatching2SetRoomDataExternalRequest* req);
	void print_set_roomdata_int_req(const SceNpMatching2SetRoomDataInternalRequest* req);
} // namespace extra_nps
