#pragma once

#include "Emu/Memory/vm_ptr.h"

// Error Codes
enum CellUserInfoError : u32
{
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

enum CellUserInfoListType
{
	CELL_USERINFO_LISTTYPE_ALL       = 0,
	CELL_USERINFO_LISTTYPE_NOCURRENT = 1,
};

enum cell_user_callback_result : u32
{
	CELL_USERINFO_RET_OK     = 0,
	CELL_USERINFO_RET_CANCEL = 1,
};

enum : u32
{
	CELL_SYSUTIL_USERID_CURRENT  = 0,
	CELL_SYSUTIL_USERID_MAX      = 99999999,
};

enum : u32
{
	CELL_USERINFO_FOCUS_LISTHEAD = 0xffffffff
};

// Structs
struct CellUserInfoUserStat
{
	be_t<u32> id;
	char name[CELL_USERINFO_USERNAME_SIZE];
};

struct CellUserInfoUserList
{
	be_t<u32> userId[CELL_USERINFO_USER_MAX];
};

struct CellUserInfoListSet
{
	vm::bptr<char> title;
	be_t<u32> focus; // id
	be_t<u32> fixedListNum;
	vm::bptr<CellUserInfoUserList> fixedList;
	vm::bptr<void> reserved;
};

struct CellUserInfoTypeSet
{
	vm::bptr<char> title;
	be_t<u32> focus; // id
	be_t<u32> type; // CellUserInfoListType
	vm::bptr<void> reserved;
};

using CellUserInfoFinishCallback = void(s32 result, vm::ptr<CellUserInfoUserStat> selectedUser, vm::ptr<void> userdata);
