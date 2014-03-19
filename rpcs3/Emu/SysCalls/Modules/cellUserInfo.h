#pragma once

// Return Codes
enum
{
	CELL_USERINFO_RET_OK          = 0,
	CELL_USERINFO_RET_CANCEL      = 1,
	CELL_USERINFO_ERROR_BUSY      = 0x8002c301,
	CELL_USERINFO_ERROR_INTERNAL  = 0x8002c302,
	CELL_USERINFO_ERROR_PARAM     = 0x8002c303,
	CELL_USERINFO_ERROR_NOUSER    = 0x8002c304,
};

// Enums
enum CellUserInfoParamSize
{
	CELL_USERINFO_USER_MAX      = 16,
	CELL_USERINFO_TITLE_SIZE    = 256,
	CELL_USERINFO_USERNAME_SIZE = 64,
};

enum  CellUserInfoListType
{
	CELL_USERINFO_LISTTYPE_ALL       = 0,
	CELL_USERINFO_LISTTYPE_NOCURRENT = 1,
};

// Structs
struct CellUserInfoUserStat
{
	u32 id;
	u8 name[CELL_USERINFO_USERNAME_SIZE];
};

struct CellUserInfoUserList
{
	u32 userId[CELL_USERINFO_USER_MAX];
};

struct CellUserInfoListSet
{
	u32 title_addr; // (char*)
	u32 focus;
	u32 fixedListNum;
	mem_ptr_t<CellUserInfoUserList> fixedList;
	u32 reserved_addr; // (void*)
};

struct CellUserInfoTypeSet
{
	u32 title_addr; // (char*)
	u32 focus;
	CellUserInfoListType type;
	u32 reserved_addr; // (void*)
};
