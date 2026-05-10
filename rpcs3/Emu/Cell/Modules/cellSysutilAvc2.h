#pragma once

#include "sceNp2.h"

#include "Emu/Memory/vm_ptr.h"

// Error codes
enum CellSysutilAvc2Error : u32
{
	CELL_AVC2_ERROR_UNKNOWN               = 0x8002b701,
	CELL_AVC2_ERROR_NOT_SUPPORTED         = 0x8002b702,
	CELL_AVC2_ERROR_NOT_INITIALIZED       = 0x8002b703,
	CELL_AVC2_ERROR_ALREADY_INITIALIZED   = 0x8002b704,
	CELL_AVC2_ERROR_INVALID_ARGUMENT      = 0x8002b705,
	CELL_AVC2_ERROR_OUT_OF_MEMORY         = 0x8002b706,
	CELL_AVC2_ERROR_ERROR_BAD_ID          = 0x8002b707,
	CELL_AVC2_ERROR_INVALID_STATUS        = 0x8002b70a,
	CELL_AVC2_ERROR_TIMEOUT               = 0x8002b70b,
	CELL_AVC2_ERROR_NO_SESSION            = 0x8002b70d,
	CELL_AVC2_ERROR_WINDOW_ALREADY_EXISTS = 0x8002b70f,
	CELL_AVC2_ERROR_TOO_MANY_WINDOWS      = 0x8002b710,
	CELL_AVC2_ERROR_TOO_MANY_PEER_WINDOWS = 0x8002b711,
	CELL_AVC2_ERROR_WINDOW_NOT_FOUND      = 0x8002b712,
};

enum CellSysutilAvc2MediaType : u32
{
	CELL_SYSUTIL_AVC2_VOICE_CHAT = 0x00000001,
	CELL_SYSUTIL_AVC2_VIDEO_CHAT = 0x00000010,
};

enum CellSysutilAvc2VoiceQuality : u32
{
	CELL_SYSUTIL_AVC2_VOICE_QUALITY_NORMAL = 0x00000001,
};

enum CellSysutilAvc2VideoQuality : u32
{
	CELL_SYSUTIL_AVC2_VIDEO_QUALITY_NORMAL = 0x00000001,
};

enum CellSysutilAvc2FrameMode : u32
{
	CELL_SYSUTIL_AVC2_FRAME_MODE_NORMAL     = 0x00000001,
	CELL_SYSUTIL_AVC2_FRAME_MODE_INTRA_ONLY = 0x00000002,
};

enum CellSysutilAvc2CoordinatesForm : u32
{
	CELL_SYSUTIL_AVC2_VIRTUAL_COORDINATES  = 0x00000001,
	CELL_SYSUTIL_AVC2_ABSOLUTE_COORDINATES = 0x00000002,
};

enum CellSysutilAvc2VideoResolution : u32
{
	CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QQVGA = 0x00000001,
	CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QVGA  = 0x00000002,
};

enum CellSysutilAvc2ChatTargetMode : u32
{
	CELL_SYSUTIL_AVC2_CHAT_TARGET_MODE_ROOM    = 0x00000100,
	CELL_SYSUTIL_AVC2_CHAT_TARGET_MODE_TEAM    = 0x00000200,
	CELL_SYSUTIL_AVC2_CHAT_TARGET_MODE_PRIVATE = 0x00000300,
	CELL_SYSUTIL_AVC2_CHAT_TARGET_MODE_DIRECT  = 0x00001000,
};

enum CellSysutilAvc2AttributeId : u32
{
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_EVENT_TYPE    = 0x00001001,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_INTERVAL_TIME = 0x00001002,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_SIGNAL_LEVEL  = 0x00001003,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MAX_BITRATE          = 0x00001004,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DATA_FEC             = 0x00001005,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_PACKET_CONTENTION    = 0x00001006,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DTX_MODE             = 0x00001007,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_STATUS_DETECTION       = 0x00001008,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_SETTING_NOTIFICATION   = 0x00001009,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MUTING_NOTIFICATION  = 0x0000100A,
	CELL_SYSUTIL_AVC2_ATTRIBUTE_CAMERA_STATUS_DETECTION    = 0x0000100B,
};

enum CellSysutilAvc2WindowAttributeId : u32
{
	CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ALPHA               = 0x00002001,
	CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_TYPE     = 0x00002002,
	CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_DURATION = 0x00002003,
	CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_STRING_VISIBLE      = 0x00002004,
	CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ROTATION            = 0x00002005,
	CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ZORDER              = 0x00002006,
	CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_SURFACE             = 0x00002007,
};

enum CellSysutilAvc2TransitionType : u32
{
	CELL_SYSUTIL_AVC2_TRANSITION_NONE     = 0xffffffff,
	CELL_SYSUTIL_AVC2_TRANSITION_LINEAR   = 0x00000000,
	CELL_SYSUTIL_AVC2_TRANSITION_SLOWDOWN = 0x00000001,
	CELL_SYSUTIL_AVC2_TRANSITION_FASTUP   = 0x00000002,
	CELL_SYSUTIL_AVC2_TRANSITION_ANGULAR  = 0x00000003,
	CELL_SYSUTIL_AVC2_TRANSITION_EXPONENT = 0x00000004,
};

enum CellSysutilAvc2WindowZorderMode : u32
{
	CELL_SYSUTIL_AVC2_ZORDER_FORWARD_MOST = 0x00000001,
	CELL_SYSUTIL_AVC2_ZORDER_BEHIND_MOST  = 0x00000002,
};

enum
{
	CELL_AVC2_CAMERA_STATUS_DETACHED     = 0,
	CELL_AVC2_CAMERA_STATUS_ATTACHED_OFF = 1,
	CELL_AVC2_CAMERA_STATUS_ATTACHED_ON  = 2,
	CELL_AVC2_CAMERA_STATUS_UNKNOWN      = 3,
};

enum
{
	CELL_AVC2_MIC_STATUS_DETACHED     = 0,
	CELL_AVC2_MIC_STATUS_ATTACHED_OFF = 1,
	CELL_AVC2_MIC_STATUS_ATTACHED_ON  = 2,
	CELL_AVC2_MIC_STATUS_UNKNOWN      = 3,
};

enum
{
	CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL     = 0,
	CELL_SYSUTIL_AVC2_STREAMING_MODE_DIRECT_WAN = 1,
	CELL_SYSUTIL_AVC2_STREAMING_MODE_DIRECT_LAN = 2,
};

enum
{
	CELL_SYSUTIL_AVC2_VIDEO_SHARING_MODE_DISABLE = 0,
	CELL_SYSUTIL_AVC2_VIDEO_SHARING_MODE_1       = 1,
	CELL_SYSUTIL_AVC2_VIDEO_SHARING_MODE_2       = 2,
	CELL_SYSUTIL_AVC2_VIDEO_SHARING_MODE_3       = 3,
};

enum
{
	CELL_AVC2_EVENT_LOAD_SUCCEEDED   = 0x00000001,
	CELL_AVC2_EVENT_LOAD_FAILED      = 0x00000002,
	CELL_AVC2_EVENT_UNLOAD_SUCCEEDED = 0x00000003,
	CELL_AVC2_EVENT_UNLOAD_FAILED    = 0x00000004,
	CELL_AVC2_EVENT_JOIN_SUCCEEDED   = 0x00000005,
	CELL_AVC2_EVENT_JOIN_FAILED      = 0x00000006,
	CELL_AVC2_EVENT_LEAVE_SUCCEEDED  = 0x00000007,
	CELL_AVC2_EVENT_LEAVE_FAILED     = 0x00000008,
};

enum
{
	CELL_AVC2_EVENT_SYSTEM_NEW_MEMBER_JOINED          = 0x10000001,
	CELL_AVC2_EVENT_SYSTEM_MEMBER_LEFT                = 0x10000002,
	CELL_AVC2_EVENT_SYSTEM_SESSION_ESTABLISHED        = 0x10000003,
	CELL_AVC2_EVENT_SYSTEM_SESSION_CANNOT_ESTABLISHED = 0x10000004,
	CELL_AVC2_EVENT_SYSTEM_SESSION_DISCONNECTED       = 0x10000005,
	CELL_AVC2_EVENT_SYSTEM_VOICE_DETECTED             = 0x10000006,
	CELL_AVC2_EVENT_SYSTEM_MIC_DETECTED               = 0x10000007,
	CELL_AVC2_EVENT_SYSTEM_CAMERA_DETECTED            = 0x10000008,
};

enum
{
	CELL_AVC2_EVENT_PARAM_ERROR_UNKNOWN                = 0x0000000000000001,
	CELL_AVC2_EVENT_PARAM_ERROR_NOT_SUPPORTED          = 0x0000000000000002,
	CELL_AVC2_EVENT_PARAM_ERROR_INVALID_ARGUMENT       = 0x0000000000000003,
	CELL_AVC2_EVENT_PARAM_ERROR_OUT_OF_MEMORY          = 0x0000000000000004,
	CELL_AVC2_EVENT_PARAM_ERROR_INVALID_STATUS         = 0x0000000000000005,
	CELL_AVC2_EVENT_PARAM_ERROR_CONTEXT_DOES_NOT_EXIST = 0x0000000000000006,
	CELL_AVC2_EVENT_PARAM_ERROR_ROOM_DOES_NOT_EXIST    = 0x0000000000000007,
	CELL_AVC2_EVENT_PARAM_ERROR_NETWORK_ERROR          = 0x0000000000000008,
};

enum
{
	CELL_AVC2_REQUEST_ID_SYSTEM_EVENT = 0x00000000
};

enum
{
	CELL_SYSUTIL_AVC2_INIT_PARAM_VERSION = 140 // Older versions may be 100, 110, 120, 130
};

enum
{
	AVC2_SPECIAL_ROOM_MEMBER_ID_CUSTOM_VIDEO_WINDOW = 0xfff0
};

typedef u32 CellSysutilAvc2EventId;
typedef u64 CellSysutilAvc2EventParam;

using CellSysutilAvc2Callback = void(CellSysutilAvc2EventId event_id, CellSysutilAvc2EventParam event_param, vm::ptr<void> userdata);

struct CellSysutilAvc2VoiceInitParam
{
	be_t<CellSysutilAvc2VoiceQuality> voice_quality;
	be_t<u16> max_speakers;
	u8 mic_out_stream_sharing;
	u8 reserved[25];
};

struct CellSysutilAvc2VideoInitParam
{
	be_t<u32> video_quality;
	be_t<u32> frame_mode;
	be_t<u32> max_video_resolution;
	be_t<u16> max_video_windows;
	be_t<u16> max_video_framerate;
	be_t<u32> max_video_bitrate;
	be_t<u32> coordinates_form;
	u8 video_stream_sharing;
	u8 no_use_camera_device;
	u8 reserved[6];
};

struct CellSysutilAvc2StreamingModeParam
{
	be_t<u16> mode;
	be_t<u16> port;
	u8 reserved[10];
};

struct CellSysutilAvc2InitParam
{
	be_t<u16> avc_init_param_version;
	be_t<u16> max_players;
	be_t<u16> spu_load_average;
	union
	{
		be_t<u16> direct_streaming_mode;
		CellSysutilAvc2StreamingModeParam streaming_mode;
	};
	u8 reserved[18];
	be_t<u32> media_type;
	CellSysutilAvc2VoiceInitParam voice_param;
	CellSysutilAvc2VideoInitParam video_param;
	u8 reserved2[22];
};

struct CellSysutilAvc2RoomMemberList
{
	vm::bptr<SceNpMatching2RoomMemberId> member_id;
	u8 member_num;
};

struct CellSysutilAvc2MemberIpAndPortList
{
	vm::bptr<SceNpMatching2RoomMemberId> member_id;
	vm::bptr<u32> dst_addr; // in_addr
	vm::bptr<u16> dst_port; // in_port_t
	be_t<SceNpMatching2RoomMemberId> my_member_id;
	u8 member_num;
};

union CellSysutilAvc2AttributeParam
{
	be_t<u64> int_param;
	be_t<f32> float_param;
	vm::bptr<void> ptr_param;
};

struct CellSysutilAvc2Attribute
{
	be_t<CellSysutilAvc2AttributeId> attr_id;
	CellSysutilAvc2AttributeParam attr_param;
};

union CellSysutilAvc2WindowAttributeParam
{
	be_t<s32> int_vector[4];
	be_t<f32> float_vector[4];
	vm::bptr<void> ptr_vector[4];
};

struct CellSysutilAvc2WindowAttribute
{
	be_t<CellSysutilAvc2WindowAttributeId> attr_id;
	CellSysutilAvc2WindowAttributeParam attr_param;
};

struct CellSysutilAvc2PlayerInfo
{
	be_t<SceNpMatching2RoomMemberId> member_id;
	u8 joined;
	u8 connected;
	u8 mic_attached;
	u8 reserved[11];
};

struct CellSysutilAvc2StreamingTarget
{
	be_t<CellSysutilAvc2ChatTargetMode> target_mode;
	union
	{
		CellSysutilAvc2RoomMemberList room_member_list;
		SceNpMatching2TeamId team_id;
		CellSysutilAvc2MemberIpAndPortList ip_and_port_list;
	};
};
