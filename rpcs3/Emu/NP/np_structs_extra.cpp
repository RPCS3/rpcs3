#include "Utilities/StrUtil.h"
#include "stdafx.h"
#include <span>
#include "np_structs_extra.h"

LOG_CHANNEL(sceNp);
LOG_CHANNEL(sceNp2);

// Helper functions for printing

namespace extra_nps
{
	void print_SceNpUserInfo2(const SceNpUserInfo2* user)
	{
		sceNp2.warning("SceNpUserInfo2:");
		sceNp2.warning("npid: %s", static_cast<const char*>(user->npId.handle.data));
		sceNp2.warning("onlineName: *0x%x(%s)", user->onlineName, user->onlineName ? static_cast<const char*>(user->onlineName->data) : "");
		sceNp2.warning("avatarUrl: *0x%x(%s)", user->avatarUrl, user->avatarUrl ? static_cast<const char*>(user->avatarUrl->data) : "");
	}

	void print_SceNpMatching2SignalingOptParam(const SceNpMatching2SignalingOptParam* opt)
	{
		sceNp2.warning("SceNpMatching2SignalingOptParam:");
		sceNp2.warning("type: %d", opt->type);
		sceNp2.warning("flag: %d", opt->flag);
		sceNp2.warning("hubMemberId: %d", opt->hubMemberId);
	}

	void print_int_attr(const SceNpMatching2IntAttr* attr)
	{
		sceNp2.warning("Id: 0x%x, num:%d(0x%x)", attr->id, attr->num, attr->num);
	}

	void print_SceNpMatching2BinAttr(const SceNpMatching2BinAttr* bin)
	{
		const auto ptr = +bin->ptr;
		const u32 size = bin->size;

		sceNp2.warning("Id: %d, Size: %d, ptr: *0x%x", bin->id, size, ptr);

		if (ptr && size)
		{
			sceNp2.warning("Data: %s", std::span<u8>{ptr.get_ptr(), size});
		}
	}

	void print_SceNpMatching2BinAttr_internal(const SceNpMatching2RoomBinAttrInternal* bin)
	{
		sceNp2.warning("updateDate: %llu updateMemberId: %d", bin->updateDate.tick, bin->updateMemberId);
		print_SceNpMatching2BinAttr(&bin->data);
	}

	void print_member_bin_attr_internal(const SceNpMatching2RoomMemberBinAttrInternal* bin)
	{
		sceNp2.warning("updateDate: %llu", bin->updateDate.tick);
		print_SceNpMatching2BinAttr(&bin->data);
	}

	void print_SceNpMatching2PresenceOptionData(const SceNpMatching2PresenceOptionData* opt)
	{
		sceNp2.warning("Data: %s", std::span<const u8>{std::data(opt->data), std::size(opt->data)});
	}

	void print_range(const SceNpMatching2Range* range)
	{
		sceNp2.warning("startIndex: %d", range->startIndex);
		sceNp2.warning("total: %d", range->total);
		sceNp2.warning("size: %d", range->size);
	}

	void print_SceNpMatching2RangeFilter(const SceNpMatching2RangeFilter* filt)
	{
		sceNp2.warning("startIndex: %d", filt->startIndex);
		sceNp2.warning("max: %d", filt->max);
	}

	void print_int_search_filter(const SceNpMatching2IntSearchFilter* filt)
	{
		sceNp2.warning("searchOperator: %s", filt->searchOperator);
		print_int_attr(&filt->attr);
	}

	void print_bin_search_filter(const SceNpMatching2BinSearchFilter* filt)
	{
		sceNp2.warning("searchOperator: %s", filt->searchOperator);
		print_SceNpMatching2BinAttr(&filt->attr);
	}

	void print_SceNpMatching2CreateJoinRoomRequest(const SceNpMatching2CreateJoinRoomRequest* req)
	{
		sceNp2.warning("SceNpMatching2CreateJoinRoomRequest:");
		sceNp2.warning("worldId: %d", req->worldId);
		sceNp2.warning("lobbyId: %d", req->lobbyId);
		sceNp2.warning("maxSlot: %d", req->maxSlot);
		sceNp2.warning("flagAttr: 0x%x", req->flagAttr);
		sceNp2.warning("roomBinAttrInternal: *0x%x", req->roomBinAttrInternal);
		sceNp2.warning("roomBinAttrInternalNum: %d", req->roomBinAttrInternalNum);

		for (u32 i = 0; i < req->roomBinAttrInternalNum && req->roomBinAttrInternal; i++)
			print_SceNpMatching2BinAttr(&req->roomBinAttrInternal[i]);

		sceNp2.warning("roomSearchableIntAttrExternal: *0x%x", req->roomSearchableIntAttrExternal);
		sceNp2.warning("roomSearchableIntAttrExternalNum: %d", req->roomSearchableIntAttrExternalNum);

		for (u32 i = 0; i < req->roomSearchableIntAttrExternalNum && req->roomSearchableIntAttrExternal; i++)
			print_int_attr(&req->roomSearchableIntAttrExternal[i]);
		
		sceNp2.warning("roomSearchableBinAttrExternal: *0x%x", req->roomSearchableBinAttrExternal);
		sceNp2.warning("roomSearchableBinAttrExternalNum: %d", req->roomSearchableBinAttrExternalNum);

		for (u32 i = 0; i < req->roomSearchableBinAttrExternalNum && req->roomSearchableBinAttrExternal; i++)
			print_SceNpMatching2BinAttr(&req->roomSearchableBinAttrExternal[i]);

		sceNp2.warning("roomBinAttrExternal: *0x%x", req->roomBinAttrExternal);
		sceNp2.warning("roomBinAttrExternalNum: %d", req->roomBinAttrExternalNum);

		for (u32 i = 0; i < req->roomBinAttrExternalNum && req->roomBinAttrExternal; i++)
			print_SceNpMatching2BinAttr(&req->roomBinAttrExternal[i]);

		sceNp2.warning("roomPassword: *0x%x", req->roomPassword);

		if (req->roomPassword)
			sceNp2.warning("data: %s", fmt::buf_to_hexstring(req->roomPassword->data, sizeof(req->roomPassword->data)));

		sceNp2.warning("groupConfig: *0x%x", req->groupConfig);
		sceNp2.warning("groupConfigNum: %d", req->groupConfigNum);
		sceNp2.warning("passwordSlotMask: *0x%x, value: 0x%x", req->passwordSlotMask, req->passwordSlotMask ? static_cast<u64>(*req->passwordSlotMask) : 0ull);
		sceNp2.warning("allowedUser: *0x%x", req->allowedUser);
		sceNp2.warning("allowedUserNum: %d", req->allowedUserNum);
		sceNp2.warning("blockedUser: *0x%x", req->blockedUser);
		sceNp2.warning("blockedUserNum: %d", req->blockedUserNum);
		sceNp2.warning("joinRoomGroupLabel: *0x%x", req->joinRoomGroupLabel);
		sceNp2.warning("roomMemberBinAttrInternal: *0x%x", req->roomMemberBinAttrInternal);
		sceNp2.warning("roomMemberBinAttrInternalNum: %d", req->roomMemberBinAttrInternalNum);

		for (u32 i = 0; i < req->roomMemberBinAttrInternalNum && req->roomMemberBinAttrInternal; i++)
			print_SceNpMatching2BinAttr(&req->roomMemberBinAttrInternal[i]);

		sceNp2.warning("teamId: %d", req->teamId);
		sceNp2.warning("sigOptParam: *0x%x", req->sigOptParam);

		if (req->sigOptParam)
			print_SceNpMatching2SignalingOptParam(req->sigOptParam.get_ptr());
	}

	void print_SceNpMatching2JoinRoomRequest(const SceNpMatching2JoinRoomRequest* req)
	{
		sceNp2.warning("SceNpMatching2JoinRoomRequest:");
		sceNp2.warning("roomId: %d", req->roomId);
		sceNp2.warning("roomPassword: *0x%x", req->roomPassword);
		sceNp2.warning("joinRoomGroupLabel: *0x%x", req->joinRoomGroupLabel);
		sceNp2.warning("roomMemberBinAttrInternal: *0x%x", req->roomMemberBinAttrInternal);
		sceNp2.warning("roomMemberBinAttrInternalNum: %d", req->roomMemberBinAttrInternalNum);
		print_SceNpMatching2PresenceOptionData(&req->optData);
		sceNp2.warning("teamId: %d", req->teamId);

		for (u32 i = 0; i < req->roomMemberBinAttrInternalNum && req->roomMemberBinAttrInternal; i++)
			print_SceNpMatching2BinAttr(&req->roomMemberBinAttrInternal[i]);
	}

	void print_SceNpMatching2SearchRoomRequest(const SceNpMatching2SearchRoomRequest* req)
	{
		sceNp2.warning("SceNpMatching2SearchRoomRequest:");
		sceNp2.warning("option: 0x%x", req->option);
		sceNp2.warning("worldId: %d", req->worldId);
		sceNp2.warning("lobbyId: %lld", req->lobbyId);
		print_SceNpMatching2RangeFilter(&req->rangeFilter);
		sceNp2.warning("flagFilter: 0x%x", req->flagFilter);
		sceNp2.warning("flagAttr: 0x%x", req->flagAttr);
		sceNp2.warning("intFilter: *0x%x", req->intFilter);
		sceNp2.warning("intFilterNum: %d", req->intFilterNum);
		for (u32 i = 0; i < req->intFilterNum && req->intFilter; i++)
			print_int_search_filter(&req->intFilter[i]);
		sceNp2.warning("binFilter: *0x%x", req->binFilter);
		sceNp2.warning("binFilterNum: %d", req->binFilterNum);
		for (u32 i = 0; i < req->binFilterNum && req->binFilter; i++)
			print_bin_search_filter(&req->binFilter[i]);
		sceNp2.warning("attrId: *0x%x", req->attrId);
		sceNp2.warning("attrIdNum: %d", req->attrIdNum);
		for (u32 i = 0; i < req->attrIdNum && req->attrId; i++)
			sceNp2.warning("attrId[%d] = 0x%x", i, req->attrId[i]);
	}

	void print_SceNpMatching2SearchRoomResponse(const SceNpMatching2SearchRoomResponse* resp)
	{
		sceNp2.warning("SceNpMatching2SearchRoomResponse:");
		print_range(&resp->range);

		const SceNpMatching2RoomDataExternal *room_ptr = resp->roomDataExternal.get_ptr();
		for (u32 i = 0; i < resp->range.total; i++)
		{
			sceNp2.warning("SceNpMatching2SearchRoomResponse[%d]:", i);
			print_SceNpMatching2RoomDataExternal(room_ptr);
			room_ptr = room_ptr->next.get_ptr();
		}
	}

	void print_SceNpMatching2RoomMemberDataInternal(const SceNpMatching2RoomMemberDataInternal* member)
	{
		sceNp2.warning("SceNpMatching2RoomMemberDataInternal:");
		sceNp2.warning("next: *0x%x", member->next);
		sceNp2.warning("npId: %s", member->userInfo.npId.handle.data);
		sceNp2.warning("onlineName: %s", member->userInfo.onlineName ? member->userInfo.onlineName->data : "");
		sceNp2.warning("avatarUrl: %s", member->userInfo.avatarUrl ? member->userInfo.avatarUrl->data : "");
		sceNp2.warning("joinDate: %lld", member->joinDate.tick);
		sceNp2.warning("memberId: %d", member->memberId);
		sceNp2.warning("teamId: %d", member->teamId);
		sceNp2.warning("roomGroup: *0x%x", member->roomGroup);
		sceNp2.warning("natType: %d", member->natType);
		sceNp2.warning("flagAttr: 0x%x", member->flagAttr);
		sceNp2.warning("roomMemberBinAttrInternal: *0x%x", member->roomMemberBinAttrInternal);
		sceNp2.warning("roomMemberBinAttrInternalNum: %d", member->roomMemberBinAttrInternalNum);
		for (u32 i = 0; i < member->roomMemberBinAttrInternalNum && member->roomMemberBinAttrInternal; i++)
			print_member_bin_attr_internal(&member->roomMemberBinAttrInternal[i]);
	}

	void print_SceNpMatching2RoomDataInternal(const SceNpMatching2RoomDataInternal* room)
	{
		sceNp2.warning("SceNpMatching2RoomDataInternal:");
		sceNp2.warning("serverId: %d", room->serverId);
		sceNp2.warning("worldId: %d", room->worldId);
		sceNp2.warning("lobbyId: %lld", room->lobbyId);
		sceNp2.warning("roomId: %lld", room->roomId);
		sceNp2.warning("passwordSlotMask: %lld", room->passwordSlotMask);
		sceNp2.warning("maxSlot: %d", room->maxSlot);

		sceNp2.warning("members: *0x%x", room->memberList.members);
		auto cur_member = room->memberList.members;
		while (cur_member)
		{
			print_SceNpMatching2RoomMemberDataInternal(cur_member.get_ptr());
			cur_member = cur_member->next;
		}
		sceNp2.warning("membersNum: %d", room->memberList.membersNum);
		sceNp2.warning("me: *0x%x", room->memberList.me);
		sceNp2.warning("owner: *0x%x", room->memberList.owner);

		sceNp2.warning("roomGroup: *0x%x", room->roomGroup);
		sceNp2.warning("roomGroupNum: %d", room->roomGroupNum);
		sceNp2.warning("flagAttr: 0x%x", room->flagAttr);
		sceNp2.warning("roomBinAttrInternal: *0x%x", room->roomBinAttrInternal);
		sceNp2.warning("roomBinAttrInternalNum: %d", room->roomBinAttrInternalNum);
		for (u32 i = 0; i < room->roomBinAttrInternalNum && room->roomBinAttrInternal; i++)
			print_SceNpMatching2BinAttr_internal(&room->roomBinAttrInternal[i]);
	}

	void print_SceNpMatching2RoomDataExternal(const SceNpMatching2RoomDataExternal* room)
	{
		sceNp2.warning("SceNpMatching2RoomDataExternal:");
		sceNp2.warning("next: *0x%x", room->next);
		sceNp2.warning("serverId: %d", room->serverId);
		sceNp2.warning("worldId: %d", room->worldId);
		sceNp2.warning("publicSlotNum: %d", room->publicSlotNum);
		sceNp2.warning("privateSlotNum: %d", room->privateSlotNum);
		sceNp2.warning("lobbyId: %d", room->lobbyId);
		sceNp2.warning("roomId: %d", room->roomId);
		sceNp2.warning("openPublicSlotNum: %d", room->openPublicSlotNum);
		sceNp2.warning("maxSlot: %d", room->maxSlot);
		sceNp2.warning("openPrivateSlotNum: %d", room->openPrivateSlotNum);
		sceNp2.warning("curMemberNum: %d", room->curMemberNum);
		sceNp2.warning("SceNpMatching2RoomPasswordSlotMask: 0x%x", room->passwordSlotMask);
		sceNp2.warning("owner: *0x%x", room->owner);

		if (room->owner)
			print_SceNpUserInfo2(room->owner.get_ptr());

		sceNp2.warning("roomGroup: *0x%x", room->roomGroup);
		// TODO: print roomGroup
		sceNp2.warning("roomGroupNum: %d", room->roomGroupNum);
		sceNp2.warning("flagAttr: 0x%x", room->flagAttr);
		sceNp2.warning("roomSearchableIntAttrExternal: *0x%x", room->roomSearchableIntAttrExternal);
		sceNp2.warning("roomSearchableIntAttrExternalNum: %d", room->roomSearchableIntAttrExternalNum);

		for (u32 i = 0; i < room->roomSearchableIntAttrExternalNum && room->roomSearchableIntAttrExternal; i++)
			print_int_attr(&room->roomSearchableIntAttrExternal[i]);

		sceNp2.warning("roomSearchableBinAttrExternal: *0x%x", room->roomSearchableBinAttrExternal);
		sceNp2.warning("roomSearchableBinAttrExternalNum: %d", room->roomSearchableBinAttrExternalNum);

		for (u32 i = 0; i < room->roomSearchableBinAttrExternalNum && room->roomSearchableBinAttrExternal; i++)
			print_SceNpMatching2BinAttr(&room->roomSearchableBinAttrExternal[i]);

		sceNp2.warning("roomBinAttrExternal: *0x%x", room->roomBinAttrExternal);
		sceNp2.warning("roomBinAttrExternalNum: %d", room->roomBinAttrExternalNum);

		for (u32 i = 0; i < room->roomBinAttrExternalNum && room->roomBinAttrExternal; i++)
			print_SceNpMatching2BinAttr(&room->roomBinAttrExternal[i]);
	}

	void print_SceNpMatching2CreateJoinRoomResponse(const SceNpMatching2CreateJoinRoomResponse* resp)
	{
		sceNp2.warning("SceNpMatching2CreateJoinRoomResponse:");
		sceNp2.warning("roomDataInternal: *0x%x", resp->roomDataInternal);
		if (resp->roomDataInternal)
			print_SceNpMatching2RoomDataInternal(resp->roomDataInternal.get_ptr());
	}

	void print_SceNpMatching2SetRoomDataExternalRequest(const SceNpMatching2SetRoomDataExternalRequest* req)
	{
		sceNp2.warning("SceNpMatching2SetRoomDataExternalRequest:");
		sceNp2.warning("roomId: %d", req->roomId);
		sceNp2.warning("roomSearchableIntAttrExternal: *0x%x", req->roomSearchableIntAttrExternal);
		sceNp2.warning("roomSearchableIntAttrExternalNum: %d", req->roomSearchableIntAttrExternalNum);

		for (u32 i = 0; i < req->roomSearchableIntAttrExternalNum && req->roomSearchableIntAttrExternal; i++)
			print_int_attr(&req->roomSearchableIntAttrExternal[i]);

		sceNp2.warning("roomSearchableBinAttrExternal: *0x%x", req->roomSearchableBinAttrExternal);
		sceNp2.warning("roomSearchableBinAttrExternalNum: %d", req->roomSearchableBinAttrExternalNum);

		for (u32 i = 0; i < req->roomSearchableBinAttrExternalNum && req->roomSearchableBinAttrExternal; i++)
			print_SceNpMatching2BinAttr(&req->roomSearchableBinAttrExternal[i]);

		sceNp2.warning("roomBinAttrExternal: *0x%x", req->roomBinAttrExternal);
		sceNp2.warning("roomBinAttrExternalNum: %d", req->roomBinAttrExternalNum);

		for (u32 i = 0; i < req->roomBinAttrExternalNum && req->roomBinAttrExternal; i++)
			print_SceNpMatching2BinAttr(&req->roomBinAttrExternal[i]);
	}

	void print_SceNpMatching2SetRoomDataInternalRequest(const SceNpMatching2SetRoomDataInternalRequest* req)
	{
		sceNp2.warning("SceNpMatching2SetRoomDataInternalRequest:");
		sceNp2.warning("roomId: %d", req->roomId);
		sceNp2.warning("flagFilter: 0x%x", req->flagFilter);
		sceNp2.warning("flagAttr: 0x%x", req->flagAttr);
		sceNp2.warning("roomBinAttrInternal: *0x%x", req->roomBinAttrInternal);
		sceNp2.warning("roomBinAttrInternalNum: %d", req->roomBinAttrInternalNum);

		for (u32 i = 0; i < req->roomBinAttrInternalNum && req->roomBinAttrInternal; i++)
			print_SceNpMatching2BinAttr(&req->roomBinAttrInternal[i]);

		sceNp2.warning("passwordConfig: *0x%x", req->passwordConfig);
		sceNp2.warning("passwordConfigNum: %d", req->passwordConfigNum);
		sceNp2.warning("passwordSlotMask: *0x%x", req->passwordSlotMask);
		sceNp2.warning("ownerPrivilegeRank: *0x%x", req->ownerPrivilegeRank);
		sceNp2.warning("ownerPrivilegeRankNum: %d", req->ownerPrivilegeRankNum);
	}

	void print_SceNpMatching2GetRoomMemberDataInternalRequest(const SceNpMatching2GetRoomMemberDataInternalRequest* req)
	{
		sceNp2.warning("SceNpMatching2GetRoomMemberDataInternalRequest:");
		sceNp2.warning("roomId: %d", req->roomId);
		sceNp2.warning("memberId: %d", req->memberId);
		sceNp2.warning("attrId: *0x%x", req->attrId);
		sceNp2.warning("attrIdNum: %d", req->attrIdNum);
		for (u32 i = 0; i < req->attrIdNum && req->attrId; i++)
		{
			sceNp2.warning("attrId[%d] = %d", i, req->attrId[i]);
		}
	}

	void print_SceNpMatching2SetRoomMemberDataInternalRequest(const SceNpMatching2SetRoomMemberDataInternalRequest* req)
	{
		sceNp2.warning("SceNpMatching2SetRoomMemberDataInternalRequest:");
		sceNp2.warning("roomId: %d", req->roomId);
		sceNp2.warning("memberId: %d", req->memberId);
		sceNp2.warning("teamId: %d", req->teamId);
		sceNp2.warning("flagFilter: 0x%x", req->flagFilter);
		sceNp2.warning("flagAttr: 0x%x", req->flagAttr);
		sceNp2.warning("roomMemberBinAttrInternal: *0x%x", req->roomMemberBinAttrInternal);
		sceNp2.warning("roomMemberBinAttrInternalNum: %d", req->roomMemberBinAttrInternalNum);
		for (u32 i = 0; i < req->roomMemberBinAttrInternalNum && req->roomMemberBinAttrInternal; i++)
			print_SceNpMatching2BinAttr(&req->roomMemberBinAttrInternal[i]);
	}

	void print_SceNpMatching2GetRoomDataExternalListRequest(const SceNpMatching2GetRoomDataExternalListRequest* req)
	{
		sceNp2.warning("SceNpMatching2GetRoomDataExternalListRequest:");
		sceNp2.warning("roomId: *0x%x", req->roomId);
		sceNp2.warning("roomIdNum: %d", req->roomIdNum);
		for (u32 i = 0; i < req->roomIdNum && req->roomId; i++)
		{
			sceNp2.warning("RoomId[%d] = %d", i, req->roomId[i]);
		}
		sceNp2.warning("attrId: *0x%x", req->attrId);
		sceNp2.warning("attrIdNum: %d", req->attrIdNum);
		for (u32 i = 0; i < req->attrIdNum && req->attrId; i++)
		{
			sceNp2.warning("attrId[%d] = %d", i, req->attrId[i]);
		}
	}

	void print_SceNpMatching2GetRoomDataExternalListResponse(const SceNpMatching2GetRoomDataExternalListResponse* resp)
	{
		sceNp2.warning("SceNpMatching2GetRoomDataExternalListResponse:");
		sceNp2.warning("roomDataExternal: *0x%x", resp->roomDataExternal);
		sceNp2.warning("roomDataExternalNum: %d", resp->roomDataExternalNum);

		vm::bptr<SceNpMatching2RoomDataExternal> cur_room = resp->roomDataExternal;

		for (u32 i = 0; i < resp->roomDataExternalNum && cur_room; i++)
		{
			sceNp2.warning("SceNpMatching2GetRoomDataExternalListResponse[%d]:", i);
			print_SceNpMatching2RoomDataExternal(cur_room.get_ptr());
			cur_room = cur_room->next;
		}
	}

	void print_SceNpMatching2GetLobbyInfoListRequest(const SceNpMatching2GetLobbyInfoListRequest* resp)
	{
		sceNp2.warning("SceNpMatching2GetLobbyInfoListRequest:");
		sceNp2.warning("worldId: %d", resp->worldId);
		print_SceNpMatching2RangeFilter(&resp->rangeFilter);
		sceNp2.warning("attrIdNum: %d", resp->attrIdNum);
		sceNp2.warning("attrId: *0x%x", resp->attrId);

		if (resp->attrId)
		{
			for (u32 i = 0; i < resp->attrIdNum; i++)
			{
				sceNp2.warning("attrId[%d] = %d", i, resp->attrId[i]);
			}
		}
	}

	void print_SceNpBasicAttachmentData(const SceNpBasicAttachmentData* data)
	{
		sceNp.warning("SceNpBasicAttachmentData:");
		sceNp.warning("id: 0x%x", data->id);
		sceNp.warning("size: %d", data->size);
	}

	void print_SceNpBasicExtendedAttachmentData(const SceNpBasicExtendedAttachmentData* data)
	{
		sceNp.warning("SceNpBasicExtendedAttachmentData:");

		sceNp.warning("flags: 0x%x", data->flags);
		sceNp.warning("msgId: %d", data->msgId);
		sceNp.warning("SceNpBasicAttachmentData.id: %d", data->data.id);
		sceNp.warning("SceNpBasicAttachmentData.size: %d", data->data.size);
		sceNp.warning("userAction: %d", data->userAction);
		sceNp.warning("markedAsUsed: %d", data->markedAsUsed);
	}

	void print_SceNpScoreRankData(const SceNpScoreRankData* data)
	{
		sceNp.warning("sceNpScoreRankData:");
		sceNp.warning("npId: %s", static_cast<const char*>(data->npId.handle.data));
		sceNp.warning("onlineName: %s", static_cast<const char*>(data->onlineName.data));
		sceNp.warning("pcId: %d", data->pcId);
		sceNp.warning("serialRank: %d", data->serialRank);
		sceNp.warning("rank: %d", data->rank);
		sceNp.warning("highestRank: %d", data->highestRank);
		sceNp.warning("scoreValue: %d", data->scoreValue);
		sceNp.warning("hasGameData: %d", data->hasGameData);
		sceNp.warning("recordDate: %d", data->recordDate.tick);
	}

	void print_SceNpScoreRankData_deprecated(const SceNpScoreRankData_deprecated* data)
	{
		sceNp.warning("sceNpScoreRankData_deprecated:");
		sceNp.warning("npId: %s", static_cast<const char*>(data->npId.handle.data));
		sceNp.warning("onlineName: %s", static_cast<const char*>(data->onlineName.data));
		sceNp.warning("serialRank: %d", data->serialRank);
		sceNp.warning("rank: %d", data->rank);
		sceNp.warning("highestRank: %d", data->highestRank);
		sceNp.warning("scoreValue: %d", data->scoreValue);
		sceNp.warning("hasGameData: %d", data->hasGameData);
		sceNp.warning("recordDate: %d", data->recordDate.tick);
	}

	void print_SceNpMatchingAttr(const SceNpMatchingAttr* data)
	{
		sceNp.warning("SceNpMatchingAttr:");
		sceNp.warning("next: 0x%x", data->next);
		sceNp.warning("type: %d", data->type);
		sceNp.warning("id: %d", data->id);

		if (data->type == SCE_NP_MATCHING_ATTR_TYPE_BASIC_BIN || data->type == SCE_NP_MATCHING_ATTR_TYPE_GAME_BIN)
		{
			sceNp.warning("ptr: *0x%x", data->value.data.ptr);
			sceNp.warning("size: %d", data->value.data.size);
			sceNp.warning("data:\n%s", fmt::buf_to_hexstring(static_cast<u8 *>(data->value.data.ptr.get_ptr()), data->value.data.size));
		}
		else
		{
			sceNp.warning("num: %d(0x%x)", data->value.num, data->value.num);
		}
	}

	void print_SceNpMatchingSearchCondition(const SceNpMatchingSearchCondition* data)
	{
		sceNp.warning("SceNpMatchingSearchCondition:");
		sceNp.warning("target_attr_type: %d", data->target_attr_type);
		sceNp.warning("target_attr_id: %d", data->target_attr_id);
		sceNp.warning("comp_type: %d", data->comp_type);
		sceNp.warning("comp_op: %d", data->comp_op);
		sceNp.warning("next: 0x%x", data->next);
		print_SceNpMatchingAttr(&data->compared);
	}

	void print_SceNpMatchingRoom(const SceNpMatchingRoom* data)
	{
		sceNp.warning("SceNpMatchingRoom:");
		sceNp.warning("next: 0x%x", data->next);
		print_SceNpRoomId(data->id);

		for (auto it = data->attr; it; it = it->next)
		{
			print_SceNpMatchingAttr(it.get_ptr());
		}
	}

	void print_SceNpMatchingRoomList(const SceNpMatchingRoomList* data)
	{
		sceNp.warning("SceNpMatchingRoomList:");
		sceNp.warning("lobbyid.opt: %s", fmt::buf_to_hexstring(data->lobbyid.opt, sizeof(data->lobbyid.opt), sizeof(data->lobbyid.opt)));
		sceNp.warning("start: %d", data->range.start);
		sceNp.warning("results: %d", data->range.results);
		sceNp.warning("total: %d", data->range.total);

		for (auto it = data->head; it; it = it->next)
		{
			print_SceNpMatchingRoom(it.get_ptr());
		}
	}

	void print_SceNpUserInfo(const SceNpUserInfo* data)
	{
		sceNp.warning("userId: %s", data->userId.handle.data);
		sceNp.warning("name: %s", data->name.data);
		sceNp.warning("icon: %s", data->icon.data);
	}

	void print_SceNpRoomId(const SceNpRoomId& room_id)
	{
		sceNp.warning("room_id: %s", fmt::buf_to_hexstring(room_id.opt, sizeof(room_id.opt), sizeof(room_id.opt)));
	}

	void print_SceNpMatchingRoomMember(const SceNpMatchingRoomMember* data)
	{
		sceNp.warning("SceNpMatchingRoomMember:");
		sceNp.warning("next: 0x%x", data->next);
		sceNp.warning("owner: %d", data->owner);
		print_SceNpUserInfo(&data->user_info);
	}

	void print_SceNpMatchingRoomStatus(const SceNpMatchingRoomStatus* data)
	{
		sceNp.warning("SceNpMatchingRoomStatus:");
		print_SceNpRoomId(data->id);
		sceNp.warning("members: 0x%x", data->members);
		sceNp.warning("num: %d", data->num);

		for (auto it = data->members; it; it = it->next)
		{
			print_SceNpMatchingRoomMember(it.get_ptr());
		}

		sceNp.warning("kick_actor: 0x%x", data->kick_actor);

		if (data->kick_actor)
		{
			sceNp.warning("kick_actor: %s", data->kick_actor->handle.data);
		}

		sceNp.warning("opt: 0x%x", data->kick_actor);
		sceNp.warning("opt_len: %d", data->opt_len);
	}

	void print_SceNpMatchingJoinedRoomInfo(const SceNpMatchingJoinedRoomInfo* data)
	{
		sceNp.warning("SceNpMatchingJoinedRoomInfo:");
		sceNp.warning("lobbyid.opt: %s", fmt::buf_to_hexstring(data->lobbyid.opt, sizeof(data->lobbyid.opt), sizeof(data->lobbyid.opt)));
		print_SceNpMatchingRoomStatus(&data->room_status);
	}

	void print_SceNpMatchingSearchJoinRoomInfo(const SceNpMatchingSearchJoinRoomInfo* data)
	{
		sceNp.warning("SceNpMatchingSearchJoinRoomInfo:");
		sceNp.warning("lobbyid.opt: %s", fmt::buf_to_hexstring(data->lobbyid.opt, sizeof(data->lobbyid.opt), sizeof(data->lobbyid.opt)));
		print_SceNpMatchingRoomStatus(&data->room_status);
		for (auto it = data->attr; it; it = it->next)
		{
			print_SceNpMatchingAttr(it.get_ptr());
		}
	}

} // namespace extra_nps
