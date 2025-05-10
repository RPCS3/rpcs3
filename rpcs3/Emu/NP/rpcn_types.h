#pragma once

#include "util/types.hpp"

namespace rpcn
{
	enum class CommandType : u16
	{
		Login,
		Terminate,
		Create,
		SendToken,
		SendResetToken,
		ResetPassword,
		ResetState,
		AddFriend,
		RemoveFriend,
		AddBlock,
		RemoveBlock,
		GetServerList,
		GetWorldList,
		CreateRoom,
		JoinRoom,
		LeaveRoom,
		SearchRoom,
		GetRoomDataExternalList,
		SetRoomDataExternal,
		GetRoomDataInternal,
		SetRoomDataInternal,
		GetRoomMemberDataInternal,
		SetRoomMemberDataInternal,
		SetUserInfo,
		PingRoomOwner,
		SendRoomMessage,
		RequestSignalingInfos,
		RequestTicket,
		SendMessage,
		GetBoardInfos,
		RecordScore,
		RecordScoreData,
		GetScoreData,
		GetScoreRange,
		GetScoreFriends,
		GetScoreNpid,
		GetNetworkTime,
		TusSetMultiSlotVariable,
		TusGetMultiSlotVariable,
		TusGetMultiUserVariable,
		TusGetFriendsVariable,
		TusAddAndGetVariable,
		TusTryAndSetVariable,
		TusDeleteMultiSlotVariable,
		TusSetData,
		TusGetData,
		TusGetMultiSlotDataStatus,
		TusGetMultiUserDataStatus,
		TusGetFriendsDataStatus,
		TusDeleteMultiSlotData,
		SetPresence,
		CreateRoomGUI,
		JoinRoomGUI,
		LeaveRoomGUI,
		GetRoomListGUI,
		SetRoomSearchFlagGUI,
		GetRoomSearchFlagGUI,
		SetRoomInfoGUI,
		GetRoomInfoGUI,
		QuickMatchGUI,
		SearchJoinRoomGUI,
	};

	enum class NotificationType : u16
	{
		UserJoinedRoom,
		UserLeftRoom,
		RoomDestroyed,
		UpdatedRoomDataInternal,
		UpdatedRoomMemberDataInternal,
		FriendQuery,  // Other user sent a friend request
		FriendNew,    // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
		FriendLost,   // Remove friend from the friendlist(user removed friend or friend removed friend)
		FriendStatus, // Set status of friend to Offline or Online
		RoomMessageReceived,
		MessageReceived,
		FriendPresenceChanged,
		SignalingHelper,
		MemberJoinedRoomGUI,
		MemberLeftRoomGUI,
		RoomDisappearedGUI,
		RoomOwnerChangedGUI,
		UserKickedGUI,
		QuickMatchCompleteGUI,
	};

	enum class rpcn_state
	{
		failure_no_failure,
		failure_input,
		failure_wolfssl,
		failure_resolve,
		failure_binding,
		failure_connect,
		failure_id,
		failure_id_already_logged_in,
		failure_id_username,
		failure_id_password,
		failure_id_token,
		failure_protocol,
		failure_other,
	};

	enum class PacketType : u8
	{
		Request,
		Reply,
		Notification,
		ServerInfo,
	};

	enum class ErrorType : u8
	{
		NoError,                     // No error
		Malformed,                   // Query was malformed, critical error that should close the connection
		Invalid,                     // The request type is invalid(wrong stage?)
		InvalidInput,                // The Input doesn't fit the constraints of the request
		TooSoon,                     // Time limited operation attempted too soon
		LoginError,                  // An error happened related to login
		LoginAlreadyLoggedIn,        // Can't log in because you're already logged in
		LoginInvalidUsername,        // Invalid username
		LoginInvalidPassword,        // Invalid password
		LoginInvalidToken,           // Invalid token
		CreationError,               // An error happened related to account creation
		CreationExistingUsername,    // Specific to Account Creation: username exists already
		CreationBannedEmailProvider, // Specific to Account Creation: the email provider is banned
		CreationExistingEmail,       // Specific to Account Creation: that email is already registered to an account
		RoomMissing,                 // User tried to interact with a non existing room
		RoomAlreadyJoined,           // User tried to join a room he's already part of
		RoomFull,                    // User tried to join a full room
		RoomPasswordMismatch,        // Room password didn't match
		RoomPasswordMissing,         // A password was missing during room creation
		RoomGroupNoJoinLabel,        // Tried to join a group room without a label
		RoomGroupFull,               // Room group is full
		RoomGroupJoinLabelNotFound,  // Join label was invalid in some way
		RoomGroupMaxSlotMismatch,    // Mismatch between max_slot and the listed slots in groups
		Unauthorized,                // User attempted an unauthorized operation
		DbFail,                      // Generic failure on db side
		EmailFail,                   // Generic failure related to email
		NotFound,                    // Object of the query was not found(user, etc), use RoomMissing for rooms instead
		Blocked,                     // The operation can't complete because you've been blocked
		AlreadyFriend,               // Can't add friend because already friend
		ScoreNotBest,                // A better score is already registered for that user/character_id
		ScoreInvalid,                // Score for player was found but wasn't what was expected
		ScoreHasData,                // Score already has data
		CondFail,                    // Condition related to query failed
		Unsupported,
	};
} // namespace rpcn
